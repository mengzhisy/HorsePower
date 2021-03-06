#include "../global.h"

typedef I (*NiladicFunc)(V);
typedef I (*MonadicFunc)(V,V);
typedef I (*DyadicFunc)(V,V,V);
typedef I (*AnyadicFunc)(V,L,V[]);
typedef I (*EachMonadic)(V,V,MonadicFunc);
typedef I (*EachDyadic)(V,V,V,DyadicFunc);
typedef I (*EachTriple)(V,V,V,V);

#define MonFuncSize totalU
#define DyaFuncSize totalB
#define JoinOperation EachTriple
#define LIMIT_ROW 20

#define VAR_MAX_SIZE 32

extern Node *entryMain;
static Node *currentMethod;

static O runList(List *list, I *isR);
static O runNode(Node *n, I *isR);
static O runMethod(Node *method, VList *param);

#define runListKind(n, r) runList(n->val.listS, r)
#define runBlock(n,r)     runListKind(n,r)
#define runArgExpr(n)     runListKind(n,NULL)

// isR == 0, means normal status
#define isRtn (isR && *isR == 1) // return
#define isBrk (isR && *isR == 2) // break
#define isCtn (isR && *isR == 3) // continue
#define isCtlAny (isR && *isR > 0)

static VList *paramList, *rtnList;

// TODO: test a variable is used before assigned

/* ---- above declarations ---- */
// string arrays in compiler.c

MonadicFunc monFunc[MonFuncSize] = {
    pfnAbs, pfnNeg, pfnCeil, pfnFloor, pfnRound, pfnConj, pfnRecip, pfnSignum,
    pfnPi  , pfnNot , pfnLog, pfnLog2, pfnLog10, pfnExp, pfnTrigCos,
    pfnTrigSin, pfnTrigTan, pfnTrigAcos, pfnTrigAsin, pfnTrigAtan,
    pfnHyperCosh, pfnHyperSinh, pfnHyperTanh, pfnHyperAcosh, pfnHyperAsinh,
    pfnHyperAtanh, pfnDate, pfnDateYear, pfnDateMonth, pfnDateDay, pfnTime,
    pfnTimeHour, pfnTimeMinute, pfnTimeSecond, pfnTimeMill, pfnUnique, pfnStr,
    pfnLen, pfnRange, pfnFact, pfnRand, pfnSeed, pfnFlip, pfnReverse, pfnWhere,
    pfnGroup, pfnSum, pfnAvg, pfnMin, pfnMax, pfnRaze,
    pfnToList, pfnKeys, pfnValues, pfnMeta, pfnLoadTable, pfnFetch, pfnPrint
};

DyadicFunc  dyaFunc[DyaFuncSize] = {
    pfnLt, pfnGt, pfnLeq, pfnGeq, pfnEq, pfnNeq, pfnPlus, pfnMinus, pfnMul,
    pfnDiv, pfnPower, pfnLogBase, pfnMod, pfnAnd, pfnOr, pfnNand, pfnNor,
    pfnXor, pfnAppend, pfnLike, pfnCompress, pfnRandK, pfnIndexOf, pfnTake,
    pfnDrop, pfnOrderBy, pfnMember, pfnVector, pfnMatch, pfnIndex, pfnColumnValue,
    pfnSubString
};

static V executeMon(MonadicFunc f, V *p){
    V z = NEW(V0);
    I status = (*f)(z,p[0]);
    if(status==0) return z;
    else {WP("[Monadic]"); printErrMsg(status); return 0;}
}

static V executeDya(DyadicFunc f, V *p){
    V z = NEW(V0);
    I status = (*f)(z, p[0], p[1]);
    if(status==0) return z;
    else {WP("[Dyadic]"); printErrMsg(status); return 0;}
}

static V executeAny(AnyadicFunc f, V *p, L n){
    V z = NEW(V0);
    I status = (*f)(z, n, p);
    if(status==0) return z;
    else {WP("[Anyadic]"); printErrMsg(status); return 0;}
}

static V executeIndexA(V *p){
    V z = NEW(V0);
    I status = pfnIndexA(z, p[0], p[1], p[2]);
    if(status==0) return z;
    else {WP("[IndexA]"); printErrMsg(status); return 0;}
}

/* 3 params: pfnDatetimeAdd, pfnDatetimeSub */
static V executeTriple(EachTriple f, V *p){
    if(H_DEBUG) WP("executeTriple\n");
    V z = NEW(V0);
    I status = (*f)(z, p[0], p[1], p[2]);
    if(status==0) return z;
    else {printErrMsg(status); return 0;}
}

static V executeJoinIndex(JoinOperation f, V *p){
    if(H_DEBUG) WP("executeJoinIndex\n");
    V z = NEW(V0);
    I status = (*f)(z, p[1], p[2], p[0]);
    if(status==0) return z;
    else {printErrMsg(status); return 0;}
}

static V executeEachDya(EachDyadic f, V *p){
    if(H_DEBUG) WP("executeEachDya\n");
    V z = NEW(V0);
    S funcName = getSymbolStr(vq(p[0]));
    FuncUnit fu;
    getFuncIndexByName(funcName, &fu);
    if(fu.kind == 1){ // unary
        EP("[EachDya] (%s) not supported.", funcName);
    }
    else if(fu.kind == 2){ // binary
        I status = (*f)(z, p[1], p[2], dyaFunc[fu.b]);
        if(status==0) return z;
        else {WP("[EachDyadic]"); printErrMsg(status);}
    }
    else {
        EP("Dyadic op expected for each_left/right, not %s", funcName);
    } R 0;
}

static V executeEachMon(EachMonadic f, V *p){
    if(H_DEBUG) WP("executeEachMon\n");
    V z = NEW(V0);
    S funcName = getSymbolStr(vq(p[0]));
    FuncUnit fu;
    getFuncIndexByName(funcName, &fu);
    if(fu.kind == 1){ // unary
        I status = (*f)(z, p[1], monFunc[fu.u]);
        if(status==0) return z;
        else {WP("[EachMonadic]"); printErrMsg(status);}
    }
    else if(fu.kind == 2){ // binary
        EP("Not supported: %s", funcName);
    }
    else {
        EP("Monadic op expected for each, not %s", funcName);
    } R 0;
}

V executeOther(TypeOther x, V *params, I numParams){
    switch(x){
        /* monadic */
        case        eachF: return executeEachMon(&pfnEach, params); break;
        /* dyadic */
        case        enumF: return executeDya(&pfnEnum  , params); break;
        case       tableF: return executeDya(&pfnTable , params); break;
        case      ktableF: return executeDya(&pfnKTable, params); break;
        case        dictF: return executeDya(&pfnDict  , params); break;
        case   eachRightF: return executeEachDya(&pfnEachRight, params); break;
        case    eachLeftF: return executeEachDya(&pfnEachLeft , params); break;
        case    eachItemF: return executeEachDya(&pfnEachItem , params); break;
        /* anyadic */
        case        listF: return executeAny(&pfnList, params, numParams); break;
        /* others */
        case      indexAF: return executeIndexA(params); break;
        case   joinIndexF: return executeJoinIndex(&pfnJoinIndex, params); break;
        case       dtaddF: return executeTriple(&pfnDatetimeAdd, params); break;
        case       dtsubF: return executeTriple(&pfnDatetimeSub, params); break;
        default: EP("Pending ... for %s", obtainTypeOther(x));
    }
    R NULL;
}

static B H_LINE;
static O showLine(Node *n){
    WP(">> running line %d: ", n->lineno);
    printNode(n);
}

static I totalVList1(VList *list){ // with dummy
    I c=0; while((list=list->next))c++; R c;
}

static O addParam(VList *list, V x){
    VList *t = NEW(VList);
    t->v = x;
    t->next = list->next;
    list->next = t;
}

static O cleanVList(VList *x){ // free x->v also?
    VList *p = x->next;
    while(p){
        VList *t = p; p = p->next; free(t);
    }
    x->next = NULL;
    //if(x && x->next){ cleanVList(x->next); free(x->next); x->next = NULL; }
}

static void printVList(VList *list){
    if(list){
        printVList(list->next);
        printV2(list->v, LIMIT_ROW); // default: 20
    }
}

static G runStack;
static L stackPtr, stackPeak;
#define STACK_SIZE 67108864  // 64MB

static O saveToStackSub(SymbolNameList *list){
    if(!list) R ;
    L size = totalSymbolNameList(list) * sizeof(V);
    if(stackPtr + size < STACK_SIZE){
        V *x = (V*)(runStack + stackPtr);
        L i = 0;
        while(list){
            //WP("added: %s\n", list->symName->name);
            x[i++] = list->symName->value;
            list=list->next;
        }
        //WP(">> save i = %lld\n", i);
        stackPtr += size;
    }
    else EP("Stack overflow");
}

static O saveToStack(Node *method){
    //WP("save to stack\n");
    saveToStackSub(method->val.method.meta->paramVars);
    saveToStackSub(method->val.method.meta->localVars);
    if(stackPtr > stackPeak)
        stackPeak = stackPtr;
}

static O loadFromStackSub(SymbolNameList *list){
    L size = totalSymbolNameList(list) * sizeof(V);
    stackPtr -= size;
    V *x = (V*)(runStack + stackPtr);
    L i = 0;
    while(list){
        //WP("loaded: %s\n", list->symName->name);
        list->symName->value = x[i++];
        list=list->next;
    }
    //WP(">> load i = %lld\n", i); getchar();
}

static O loadFromStack(Node *method){ // localVars first, then paramVars
    //WP("load from stack\n"); getchar();
    loadFromStackSub(method->val.method.meta->localVars);
    loadFromStackSub(method->val.method.meta->paramVars);
}

static O loadParams(Node *method, VList *param){
    SymbolNameList *list = method->val.method.meta->paramVars;
    param = param->next;
    while(list){
        //WP("name = %s\n", list->symName->name);
        //WP("value = \n"); printV(param->v);
        list->symName->value = param->v;
        list = list->next;
        param = param->next;
    }
}

static I getParamsSub(VList *list, V *rtn){
    if(list && list->next){
        I k = getParamsSub(list->next, rtn);
        rtn[k] = list->next->v;
        return k+1;
    }
    return 0;
}

static V *getParamsV(VList *list){
    I tot = totalVList1(list);
    V *vals = NEW2(V0, tot);
    getParamsSub(list, vals);
    return vals;
}

static V invokeBuiltin(char *funcName, VList *list){
    FuncUnit fu;
    getFuncIndexByName(funcName, &fu);
    I valence = getValence(&fu);
    I numArg = totalVList1(list);
    V *params = getParamsV(list);
    if(numArg == valence || valence == -1){ //-1: any
        switch(fu.kind){
            case 1: return executeMon(monFunc[fu.u], params);
            case 2: return executeDya(dyaFunc[fu.b], params);
            case 3: return executeOther(fu.t, params, numArg);
        }
    }
    else EP("Valence error: %d expected, but %d found", valence, numArg);
    // TODO: free params
    return 0;
}

static O copyParamToRtn(VList *param, VList *rtn){
    if(param && param->next){
        VList *p = param->next;
        copyParamToRtn(p, rtn);
        addParam(rtn, p->v);
    }
}

static O invokeFunction(Node *func, VList *param){
    char *funcName = func->val.name.id2;
    SymbolName *sn = getSymbolName(func->val.name.st, funcName);
    switch(sn->kind){
        case builtinS: {
              V rtn = invokeBuiltin(funcName, param);
              //printV(rtn); getchar();
              addParam(rtnList, rtn);
             } break;
        case  methodS: {
                //WP("stackPtr = %lld\n", stackPtr);
                //printV(param->next->v); getchar();
                runMethod(sn->val.method, param);
                //rtnList->next = paramList->next; // save link (dangerous)
                cleanVList(rtnList);  // must be cleaned before copy
                copyParamToRtn(paramList, rtnList);
             } break;
        default: TODO("Support kind: %d\n", sn->kind);
    }
}

static O runCall(Node *n){
    Node *funcName = n->val.call.func;
    Node *param = n->val.call.param;
    //char *funcName = n->val.call.func;
    cleanVList(rtnList);
    cleanVList(paramList);
    if(param) runNode(param, NULL); 
    //if(n->lineno == 66){
    //    WP("size = %d\n", totalVList(paramList)); getchar();
    //    V p0 = paramList->next->v;
    //    V p1 = paramList->next->next->v;
    //    getInfoVar(p0);
    //    getInfoVar(p1);
    //    getchar();
    //}
    invokeFunction(funcName, paramList);
    //if(n->lineno == 64){
    //    V r0 = rtnList->next->v;
    //    getInfoVar(r0);
    //    getchar();
    //}
}

static L loadVector(List *list, V x, I t){
    if(list){
        L i = loadVector(list->next, x, t);
        loadConst(list->val, x, i, t);
        R i+1;
    }
    R 0;
}

V getVector(Node *n){
    I t = getHType(getType(n->val.vec.typ));
    List *items = n->val.vec.val;
    V x = allocNode();
    L size = totalList(items);
    initV(x, t, size);
    loadVector(items, x, t);
    return x;
}

static O runVector(Node *n){
    addParam(paramList, getVector(n));
}


static O assignVars(List *vars, VList *list){
    if(vars){
        assignVars(vars->next, list->next);
        Node *n = vars->val;
        if(instanceOf(n, varK)){
            n->val.param.sn->value = list->v;
        }
        else if(instanceOf(n, nameK)){
            if(n->val.name.sn)  // NOT '_'
                n->val.name.sn->value = list->v;
            //printNode(n); printV(list->v); getchar();
        }
        else TODO("Unknown kind: %d\n", n->kind);
    }
}

static O checkRtns(List *vars, VList *list){
    I numVars = totalVar(vars);
    I numRtns = totalVList1(list);
    if(numVars == numRtns){
        assignVars(vars, list->next);
    }
    else EP("%d expects, but %d returned", numVars, numRtns);
}

static O runAssignStmt(Node *n){
    if(H_LINE) showLine(n);
    cleanVList(paramList);
    Node *expr = n->val.assignStmt.expr;
    List *vars = n->val.assignStmt.vars;
    runNode(expr, NULL);
    if(instanceOf(expr, callK)){
        checkRtns(vars, rtnList);
    }
    else if(instanceOf(expr, castK)){
        checkRtns(vars, rtnList);
    }
    else {
        checkRtns(vars, paramList);
    }
}

static O runCastStmt(Node *n){
    Node *expr = n->val.cast.exp;
    runNode(expr, NULL);
}

static O runExprStmt(Node *n){
    Node *expr = n->val.exprStmt.expr;
    runNode(expr, NULL);
    // discard returns
}

static O runName(Node *n){
    if(!n->val.name.isUS){
        SymbolName *sn = n->val.name.sn;
        if(sn->value) addParam(paramList, sn->value);
        else EP("Variable must be initialized before used: %s", sn->name);
    }
    else EP("Underscore '%s' can't be used in a name reference", uscore);
}

static V initSymVector(S s){
    V x = allocNode();
    initV(x, H_Q, 1);
    xq = getSymbol(s);
    return x;
}

static S getFuncNameStr(Node *funcName){
    SymbolName *sn = funcName->val.name.sn;
    //printV(literalSymbol(sn->name)); getchar();
    switch(sn->kind){
        case builtinS: return sn->name; 
        case  methodS: return sn->name;
        default: TODO("Add more support for %d", sn->kind);
    }
}

static O addFuncList(List *list, V x, I dep){
    if(list) {
        addFuncList(list->next,x,dep+1);
        xQ(dep) = getSymbol(getFuncNameStr(list->val));
    }
}

static O runFunc(Node *n){
    V x = allocNode();
    initV(x, H_Q, totalList(n->val.listS));
    addFuncList(n->val.listS, x, 0);
    addParam(paramList, x);
}

// static B checkInfoNodeWithValue(InfoNode *in, V v){
//     // check main type and cell types with return types
//     if(!in->subInfo && !in->next){
//     }
//     else TODO("Need to check cell types\n");
// }

static InfoNode *getInfoNodeFromV(V x){
    InfoNode *in = NEW(InfoNode);
    in->type = getTypeFromV(x);
    return in;
}

static B checkReturnTypeSub(Node *n, V v){
    InfoNode *in = n->val.type.in;
    if(isW(in)) return true;
    else {
        InfoNode *nv = getInfoNodeFromV(v);
        B rtn =  checkType(in, nv);
        freeInfoNode(nv);
        return rtn;
    }
}

static O checkReturnType(List *nlist, VList *plist){
    if(nlist){
        checkReturnType(nlist->next, plist->next);
        if(!checkReturnTypeSub(nlist->val, plist->v))
            EP("Return type is not expected");
    }
}

static O runReturn(Node *n, I *isR){
    if(H_LINE) showLine(n);
    cleanVList(paramList);
    runList(n->val.listS, NULL);
    checkReturnType(currentMethod->val.method.typ, paramList->next);
    *isR = 1;
}

static O runIf(Node *n, I *isR){
    cleanVList(paramList);
    runNode(n->val.ifStmt.condExpr, isR);
    V x = paramList->next->v; // only 1st value node
    if(xn == 1 && xp == H_B){
        Node *thenBlock = n->val.ifStmt.thenBlock;
        Node *elseBlock = n->val.ifStmt.elseBlock;
        if(xb) {
            runNode(thenBlock, isR);
        }
        else if(elseBlock) {
            runNode(elseBlock, isR);
        }
    }
    else EP("If-condition must be a single bool: size = %lld, type = %s", xn, getTypeName(xp));
}

static O runWhile(Node *n, I *isR){
    cleanVList(paramList);
    runNode(n->val.whileStmt.condExpr, isR);
    V x = paramList->next->v;
    if(xn == 1 && xp == H_B){
        if(xb) {
            runNode(n->val.whileStmt.bodyBlock, isR);
            if(isRtn) R;
            else if(isBrk) { *isR = 0; R; }
            else if(isCtn) { *isR = 0; runWhile(n, isR); }
            else runWhile(n, isR);
        }
    }
    else EP("While-condition must be a single bool: size = %lld, type = %s", xn, getTypeName(xp));
}

static O runRepeat(Node *n, I *isR){
    cleanVList(paramList);
    runNode(n->val.whileStmt.condExpr, isR);
    V x = paramList->next->v;
    if(xn == 1 && isIntegers(x)){
        L t = getInteger1(x);
        while(t--) {
            runNode(n->val.repeatStmt.bodyBlock, isR);
            //WP("t = %lld, isR = %d\n", t, *isR); getchar();
            if(isRtn) break;
            else if(isBrk) { *isR = 0; break; }
            else if(isCtn) { *isR = 0; }
        }
    }
    else EP("Repeat-condition must be a single bool: size = %lld, type = %s", xn, getTypeName(xp));
}

static O runBreak(I *isR){
    *isR = 2;
}

static O runContinue(I *isR){
    *isR = 3;
}

#define runVarDecl(n, r) ; // do nothing

static O runNode(Node *n, I *isR){
    if(isCtlAny) R;
    switch(n->kind){
        case     stmtK: runAssignStmt(n);      break;
        case     castK: runCastStmt  (n);      break;
        case exprstmtK: runExprStmt  (n);      break;
        case     callK: runCall      (n);      break;
        case   vectorK: runVector    (n);      break;
        case  argExprK: runArgExpr   (n);      break;
        case     nameK: runName      (n);      break;
        case     funcK: runFunc      (n);      break;
        case   returnK: runReturn    (n, isR); break;
        case    blockK: runBlock     (n, isR); break;
        case       ifK: runIf        (n, isR); break;
        case    whileK: runWhile     (n, isR); break;
        case   repeatK: runRepeat    (n, isR); break;
        case    breakK: runBreak     (isR);    break;
        case continueK: runContinue  (isR);    break;
        case  varDeclK: runVarDecl   (n,isR);  break;
        default: EP("Kind not supported yet: %s", getNodeTypeStr(n));
    }
}


static O runList(List *list, I *isR){
    if(list){
        runList(list->next, isR);
        if(isCtlAny) R; // if any control stmt occurs, then stop
        if(list->val)
            runNode(list->val, isR);
    }
}

static O runMethod(Node *method, VList *param){
    if(H_LINE) WP("method name %s\n", method->val.method.fname);
    Node *prevNode = currentMethod;
    if(prevNode != NULL)
        saveToStack(prevNode);
    if(param != NULL)
        loadParams(method, param);
    currentMethod = method;
    List *rtnTypes = method->val.method.typ;
    I isR=0; runNode(method->val.method.block, &isR);
    //WP("return code: %d\n", isR);
    if(rtnTypes){
        // output - if it has returns
        if(method == entryMain){
            WP("Output: \n");
            printVList(paramList->next);
        }
    }
    if(prevNode != NULL)
        loadFromStack(prevNode);
    currentMethod = prevNode;
}

static O init(){
    paramList = NEW(VList);
    rtnList = NEW(VList);
    runStack = (G)malloc(STACK_SIZE);
    stackPtr = stackPeak = 0;
    H_LINE = true; // debug
}

static O resultReport(){
    WP(">> stack pointer current: %lld, peak = %lld, max = %lld\n", stackPtr, stackPeak, (L)STACK_SIZE);
}

/* entry */
I HorseInterpreter(){
    printBanner("Interpreting");
    init();
    runMethod(entryMain, NULL);
    //printSymInfo();
    //printHeapInfo();
    resultReport();
    R 0;
}

/*
 * TODO:
 * - Need to check variable types to see if they are the same as their
 *   designated types at runtime
 * - Update from passing by reference to passing by value (maybe reference count)
 */


