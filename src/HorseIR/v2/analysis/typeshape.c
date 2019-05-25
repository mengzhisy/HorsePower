#include "../global.h"

typedef InfoNode* (*NilFunc)();
typedef InfoNode* (*MonFunc)(InfoNode*);
typedef InfoNode* (*DyaFunc)(InfoNode*, InfoNode*);
typedef InfoNode* (*AllFunc)(InfoNodeList*);
// InfoNodeList has a leading dummy node
#define isSingleType(n) (!(n)->next && !(n)->subInfo)

static void scanList(List *list);
static void scanNode(Node *n);
static InfoNode *addToInfoList(InfoNodeList *list, InfoNode *in);
static void cleanInfoList(InfoNodeList *in_list);
static InfoNode *scanType(Node *n);

static bool H_SHOW;
InfoNodeList *currentInList;

/*  ---- above declarations ---- */

static int totalElement(List *list){ // no dummy
    int c=0; while(list){c++; list=list->next;} return c;
}

int totalInfo(InfoNodeList *list){
    int c=0; while(list->next){list=list->next; c++;} return c;
}

static InfoNode *getNode(InfoNodeList *list, int k){
    while(list->next && k>=0){ list=list->next; k--; }
    return list->in;
}

static InfoNode *propagateBuiltin(char *funcName, InfoNodeList *list){
    int numArg = totalInfo(list);
    int valence = -2;
    InfoNode *newNode = NULL;
    void* funcRtn = fetchTypeRules(funcName, &valence);  /* entry */
    if(!funcRtn) {
        EP("Error type rules found in *%s*\n", funcName);
    }
    if(numArg == valence){
        if(valence == 2){ // binary
            DyaFunc func = (DyaFunc)funcRtn;
            newNode = func(getNode(list,1), getNode(list,0));
        }
        else if(valence == 1){
            MonFunc func = (MonFunc)funcRtn;
            newNode = func(getNode(list,0));
        }
        else if(valence > 2){
            AllFunc func = (AllFunc)funcRtn;
            newNode = func(list);
        }
        else if(valence == 0){
            NilFunc func = (NilFunc)funcRtn;
            newNode = func();
        }
        else EP("Valence must >= 0\n");
    }
    else if(valence == -1){ // match anything
        AllFunc func = (AllFunc)funcRtn;
        newNode = func(list);
    }
    else EP("Builtin function valence mismatch: %d vs. %d\n", numArg, valence);
    // return
    if(newNode) return newNode;
    else {
        DOIr(numArg, printInfoNode(getNode(list,i)))
        EP("NULL type rules found for function : %s\n",funcName);
    }
}

InfoNodeList *propagateType(Node *func, InfoNodeList *list){
    //printNode(func);
    char *funcName = func->val.name.id2;
    SymbolName *sn = getSymbol(func->val.name.st, funcName);
    switch(sn->kind){
        case builtinS:
            {
                InfoNode *rtn = propagateBuiltin(funcName, list);
                cleanInfoList(currentInList);
                addToInfoList(currentInList, rtn);
            } break;
        default: TODO("Support kind: %d\n", sn->kind);
    }
    return currentInList;
}

// --- below workspace

static InfoNode *addToInfoList(InfoNodeList *list, InfoNode *in){
    InfoNodeList *x = NEW(InfoNodeList);
    x->in = in;
    x->next = list->next;
    list->next = x;
    return in;
}

static void cleanInfoList(InfoNodeList *in_list){
    InfoNodeList *x = in_list->next;
    while(x){ InfoNodeList *t = x; x=x->next; free(t);}
    in_list->next = NULL;
}

static InfoNode *getInfoVector(Node *n){
    int num = totalElement(n->val.vec.val);
    InfoNode *in = scanType(n->val.vec.typ);
    ShapeNode *sn = newShapeNode(vectorH, SN_CONST, num);
    in->shape = sn;
    return in;
}

static InfoNode *getInfoFromNode(Node *n){
    if(instanceOf(n, typeK)){
        return n->val.type.in;
    }
    else if(instanceOf(n, nameK)){
        SymbolName *x = getSymbol(n->val.name.st, n->val.name.id2);
        Node *t = NULL;
        switch(x->kind){
            case globalS: t = x->val.global; break;
            case localS : t = x->val.local ; break;
            default: EP("Kind not supported:%d\n", x->kind);
        }
        return getInfoFromNode(t->val.param.typ);
    }
    else if(instanceOf(n, vectorK)){
        return getInfoVector(n);
    }
    else if(instanceOf(n, varK)){
        return getInfoFromNode(n->val.param.typ);
    }
    else { printNodeType(n); EP("Kind not supported\n"); }
    return NULL;
}

// true : exactly same
// false: not same
static bool infoCompatible(InfoNode *x, InfoNode *y){
    if(isW(y) || !y->shape)
        EP("Right hand side shape unknown\n");
    if(isW(x)) {
        if(x->shape) free(x->shape);
        x->shape = y->shape; // specialize type from right side
        x->type  = y->type;
        return true;
    }
    else {
        if(checkType(x,y) && checkShape(x,y)){
            bool partSub = false, partNext = false;
            // sub field
            if(!x->subInfo && !y->subInfo){
                partSub = true;
            }
            else if(x->subInfo && y->subInfo){
                partSub = infoCompatible(x->subInfo, y->subInfo);
            }
            // next field
            if(!x->next && !y->next) {
                partNext = true;
            }
            else if(x->next && y->next){
                partSub = infoCompatible(x->next, y->next);
            }
            return partSub && partNext;
        }
        else return false;
    }
}

static bool typeCompatible(Node *x, InfoNode *in){
    // x: varK
    InfoNode *ix = getInfoFromNode(x);
    return infoCompatible(ix, in);
}

static bool typelistCompatible(int num, List *x, InfoNodeList *list){
    //    x:  no dummy
    // list: has dummy
    InfoNodeList *p = list->next;
    for(int i=0;i<num;i++){
        Node *var0 = x->val;
        InfoNode *in = p->in;
        if(!typeCompatible(var0, in)) return false;
        x = x->next;
        p = p->next;
    }
    return true;
}

static void printInfoVars(List *list){
    if(list){
        printInfoVars(list->next);
        InfoNode *x = getInfoFromNode(list->val);
        printInfoNode(x);
    }
}

static void scanModule(Node *n){ scanList(n->val.module.body ); }
static void scanMethod(Node *n){ scanNode(n->val.method.block); }

#define scanListNode(n)   scanList(n->val.listS)
#define scanParams(n)     scanListNode(n)
#define scanArgExpr(n)    scanListNode(n)
#define scanBlockStmt(n)  scanListNode(n)
#define scanReturnStmt(n) scanListNode(n)

// TODO: a,_ = ...
static void scanAssignStmt(Node *n){
    if(H_SHOW) printNode(n);
    List *vars = n->val.assignStmt.vars;
    Node *expr = n->val.assignStmt.expr;
    scanNode(expr);
    /* handle returns */
    int numVar = totalElement(vars);
    if(instanceOf(expr, callK)){
        int numExpr = totalInfo(currentInList);
        if(numVar == numExpr){
            if(typelistCompatible(numVar, vars, currentInList));
            else EP("Type error in assignment\n");
        }
        else EP("Assignment need %d, but %d returned\n", numVar, numExpr);
    }
    else {
        if(numVar == 1 && typelistCompatible(1, vars, currentInList));
        else {
            printNode(n); EP("Type error in assignment\n");
        }
    }
    //printInfoNode(currentInList->next->in);
    printInfoVars(vars);
    cleanInfoList(currentInList);
}

static void scanGlobal(Node *n){
    scanNode(n->val.global.op);
    /* handle returns */
    int numExpr = totalInfo(currentInList);
    InfoNode *lhs = getInfoFromNode(n->val.global.typ);  // don't use scanType(..)
    InfoNode *rhs = currentInList->next?currentInList->next->in:NULL;
    if(numExpr == 1 && infoCompatible(lhs, rhs));
    else {
        printNode(n); EP("Type error in assignment\n");
    }
    cleanInfoList(currentInList);
}

static void scanCast(Node *n){
    scanNode(n->val.cast.exp);
}

static void scanIfStmt(Node *n){
    scanNode(n->val.ifStmt.condExpr );
    scanNode(n->val.ifStmt.thenBlock);
    scanNode(n->val.ifStmt.elseBlock);
}

static void scanWhileStmt(Node *n){
    scanNode(n->val.whileStmt.condExpr);
    scanNode(n->val.whileStmt.bodyBlock);
}

static InfoNodeList *scanRepeatStmt(Node *n){
    scanNode(n->val.repeatStmt.condExpr);
    scanNode(n->val.repeatStmt.bodyBlock);
    return NULL;
}

static void scanName(Node *n){
    SymbolName *x = getSymbol(n->val.name.st, n->val.name.id2);
    switch(x->kind){
        case globalS: {Node *t=x->val.global; scanType(t->val.global.typ); } break;
        case localS : {Node *t=x->val.local;  scanType(t->val.param.typ);  } break;
        default: EP("Kind not supported: %d\n", x->kind);
    }
}

static void scanVector(Node *n){
    getInfoVector(n);
}

static void scanCall(Node *n){
    Node *funcName = n->val.call.func;
    scanNode(n->val.call.param);
    propagateType(funcName, currentInList);
}

static InfoNode *scanType(Node *n){
    return addToInfoList(currentInList, n->val.type.in);
}

static void scanExprStmt(Node *n){
    scanNode(n->val.exprStmt.expr);
}

static void scanNode(Node *n){
    if(!n) R;
    switch(n->kind){
        case    moduleK: scanModule      (n); break; //
        case    methodK: scanMethod      (n); break; //
        case      stmtK: scanAssignStmt  (n); break; //
        case      castK: scanCast        (n); break; //
        case  exprstmtK: scanExprStmt    (n); break;
        case paramExprK: scanParams      (n); break;
        case      callK: scanCall        (n); break;
        case      nameK: scanName        (n); break;
        case   argExprK: scanArgExpr     (n); break;
        case    returnK: scanReturnStmt  (n); break;
        //case  continueK: scanContinueStmt(n); break;
        //case     breakK: scanBreakStmt   (n); break;
        case        ifK: scanIfStmt      (n); break; //
        case     whileK: scanWhileStmt   (n); break; //
        case    repeatK: scanRepeatStmt  (n); break; //
        case      typeK: scanType        (n); break;
        case     blockK: scanBlockStmt   (n); break; //
        case    vectorK: scanVector      (n); break; //
        case    globalK: scanGlobal      (n); break; //
    }
}

static void scanList(List *list){
    if(list){
        scanList(list->next);
        scanNode(list->val);
    }
}

/* entry */
void propagateTypeShape(Prog *root){
    printBanner("Type Shape Propagation (After symbol table)");
    currentInList = NEW(InfoNodeList);
    H_SHOW = true;
    scanList(root->module_list);
}
