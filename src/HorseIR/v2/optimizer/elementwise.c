#include "../global.h"

typedef struct codegen_gnode{
    Node *node;
    I pnum; // # of parameter
    struct codegen_gnode *pnode[5]; // parameter nodes
}gNode;

extern List *compiledMethodList;
extern I qid, phTotal;
extern sHashTable *hashOpt;

extern I varNum;
extern S varNames[99];
extern C code[CODE_MAX_SIZE], *ptr;

static Node *currentMethod;

/* ------ declaration above ------ */

static void printChainUses(Chain *p){
    DOI(chainUseSize(p), {P(" --> "); printChain(chainUse(p,i)); P("\n"); })
}

static void printChainDefs(Chain *p){
    DOI(chainDefSize(p), {P(" <-- "); printChain(chainDef(p,i)); P("\n"); })
}

static B isMatchedDef(Chain *p, char *name){
    List *vars = getStmtVars(chainNode(p));
    while(vars){
        Node *n = vars->val;
        if(instanceOf(n, nameK) && getNameKind(n) == localS){
            if(sEQ(name, getName2(n))) return true;
        }
        else if(instanceOf(n, varK) && getVarKind(n) == localS){
            if(sEQ(name, getVarName(n))) return true;
        }
        vars = vars->next;
    }
    return false;
}

I findDefByName(Chain *p, char *name){
    I c = 0, x = -1;
    DOI(p->defSize, if(isMatchedDef(p->chain_defs[i], name)){c++; x=i;})
    return c==1?x:-1;
}

static B isMatchedUse(Chain *p, char *name){
    if(!instanceOf(chainNode(p), stmtK)) return false;
    Node *expr  = getStmtExpr(chainNode(p));
    List *param = expr->val.call.param->val.listS;
    while(param){
        Node *n = param->val;
        if(instanceOf(n, nameK) && getNameKind(n) == localS){
            if(sEQ(name, getName2(n))) return true;
        }
        param = param->next;
    }
    return false;
}

static I findUseByName(Chain *p, char *name){
    I c = 0, x = -1;
    DOI(p->useSize, if(isMatchedUse(chainUse(p,i), name)){c++; x=i;})
    return c==1?x:-1;
}

static gNode *initgNode(Node *node){
    gNode *x = NEW(gNode);
    x->node  = node;
    x->pnum  = totalElement(fetchParams(node));
    if(x->pnum >= 5)
        EP("Not enough space");
    return x;
}

static gNode *findFusionUp(Chain *chain){
    if(isChainVisited(chain)) return NULL;
    else chain->isVisited = true;
    Node *n = chainNode(chain);
    if(instanceOf(n, stmtK)){
        List *vars = getStmtVars(n);
        Node *call = getStmtCall(n);
        Node *func = getCallFunc(call);
        SymbolKind sk = getNameKind(func);
        if(!(sk == builtinS && isElementwise(getName2(func))))
            return NULL; // if not an elemetnwsie func
        //List *param = expr->val.call.param->val.listS;
        List *param = fetchParams(n);
        //printChainUses(chain); getchar();
        // -- useful debugging
        //printBanner("Gotcha");
        //printChain(chain); P("\n");
        gNode *rt = initgNode(chainNode(chain));
        B isOK = true;
        I cnt = 0;
        while(param){
            Node *p = param->val;
            if(instanceOf(p, nameK)){
                SymbolKind sk = getNameKind(p);
                if(sk == localS){
                    I c = findDefByName(chain, getName2(p));
                    //P("c4 = %d, s = %s\n", c, getName2(p)); getchar();
                    if(c < 0) isOK = false;
                    else {
                        rt->pnode[cnt] = findFusionUp(chain->chain_defs[c]);
                    }
                }
                else isOK = false;
            }
            else rt->pnode[cnt] = NULL;
            if(!isOK) {free(rt); return NULL;}  // stop and exit
            param = param->next;
            cnt++;
        }
        return rt;
        // isOK == true
        // isOK == true, good ==> save
    }
    return NULL;
}

static Chain *findFusionDown(Chain *chain){
    Node *n = chainNode(chain);
    if(instanceOf(n, stmtK)){
        List *vars = getStmtVars(n);
        Node *call = getStmtCall(n);
        Node *func = getCallFunc(call);
        SymbolKind sk = getNameKind(func);
        if(!(sk == builtinS && isElementwise(getName2(func))))
            return NULL; // if not an elemetnwsie func
        B isOK = true;
        while(vars){
            Node *p = vars->val;
            if(instanceOf(p, varK)){
                if(p->val.param.sn->kind == localS){
                    I c = findUseByName(chain, getVarName(p));
                    //P("c1 = %d, s = %s\n", c, getVarName(p)); getchar();
                    if(c < 0) isOK = false;
                    else {
                        Chain *foundChain = findFusionDown(chain->chain_uses[c]);
                        if(foundChain) return foundChain;
                    }
                }
                else isOK = false;
            }
            else if(instanceOf(p, nameK)){
                if(getNameKind(p) == localS){
                    I c = findUseByName(chain, getName2(p));
                    //P("c2 = %d, s = %s\n", c, getName2(p)); getchar();
                    if(c < 0) isOK = false;
                    else {
                        Chain *foundChain = findFusionDown(chain->chain_uses[c]);
                        if(foundChain) return foundChain;
                    }
                }
                else isOK = false;
            }
            else isOK = false;
            //printChain(chain);
            //P("isOK = %d\n", isOK); getchar();
            if(!isOK) return chain;
            vars = vars->next;
        }
        return chain;
    }
    return NULL;
}

static B isOK2Fuse(gNode *rt){
    // condition: more than 1 stmt
    DOI(rt->pnum, if(rt->pnode[i])R 1) R 0;
}

// TODO: remove duplicated items (only distinct values wanted)
static void totalInputs(gNode *rt, S *names){
    List *params = fetchParams(rt->node);
    DOI(rt->pnum, {I k=i2-i-1; gNode *t=rt->pnode[k]; \
            if(t) totalInputs(t,names); \
            else {Node *p = fetchParamsIndex(params,k)->val; \
                if(instanceOf(p,nameK)){ \
                  if(!isDuplicated(names,getName2(p))) \
                    names[varNum++]=getName2(p);}} })
}

static void genCodeElem(gNode *rt, B isRT){
    ChainExtra *extra = NEW(ChainExtra);
    extra->kind = isRT?OptG:SkipG;
    Node *n = rt->node;
    C temp[199];
    if(isRT){
        Node *z0 = getParamFromNode(n,0); S z0s = getNameStr(z0);
        C z0c = getTypeCodeByName(z0);
        SP(temp, "q%d_elementwise_%d",qid,phTotal++);
        glueCode(genDeclSingle(temp, '{')); glueLine();
        varNum = 0;
        totalInputs(rt, varNames);
        DOI(varNum, glueAny(indent "V x%lld = x[%lld]; // %s\n",i,i,varNames[i]))
        glueAny(indent "DOP(vn(z), v%c(z,i)=",z0c);
        //DOI(varNum, P(indent "V x%lld = x[%lld]; // %s\n",i,i,varNames[i]))
        //P(indent "DOP(vn(z), v%c(z,i)=",z0c);
        // setup invocation for final fusion
        extra->funcDecl = genDeclSingle(temp, ';');
        extra->funcInvc = genInvcSingle(z0s, temp, varNames, varNum);
    }
    Node *fn = fetchFuncNode(n);
    //P("%s(", genFuncNameC(getName2(fn)));
    glueAny("%s(", genFuncNameC(getName2(fn)));
    List *params = fetchParams(n);
    DOI(rt->pnum, {if(i>0)glueChar(','); I k=i2-i-1; gNode *t=rt->pnode[k]; \
            if(t) genCodeElem(t,false); \
            else {Node *p = fetchParamsIndex(params,k)->val; \
                if(instanceOf(p,nameK)) genCodeName(p,searchName(varNames,getName2(p))); \
                else genCodeNode(p);} })
    glueChar(')');
    if(isRT){
        glueCode(") R 0;\n}");
        extra->funcFunc = strdup(code);
    }
    addToSimpleHash(hashOpt, (L)(rt->node), (L)extra); // insert to hash
    //printChainExtra(extra);
}

static void findFusionSub(Chain *chain){
    Chain *bottom = findFusionDown(chain);
    if(bottom){
        // if num of chains > 1, likely fusion
        gNode *rt = findFusionUp(bottom);
        if(rt && isOK2Fuse(rt)){
            P("bottom chain found:\n");
            //printChain(bottom); getchar();
            cleanCode(); ptr = code;
            genCodeElem(rt,true);
            getchar();
        }
    }
    else {
        chain->isVisited = true;
        //WP("bottom chain not found at: \n\t");
        //printChain(chain); P("\n");
    }
}

static void findFusion(Chain *chain){
    Node *n = chainNode(chain);
    //printChain(chain); P("\n");
    if(instanceOf(n, stmtK)){
        findFusionSub(chain);
    }
}

static void analyzeChain(ChainList *list){
    if(list){
        analyzeChain(list->next);
        if(!isChainVisited(list->chain))
            findFusion(list->chain);
    }
}

static void compileMethod(Node *n){
    Node *prevMethod = currentMethod;
    currentMethod = n;
    ChainList *chains = getMethodChainList(n);
    analyzeChain(chains->next);
    currentMethod = prevMethod;
    //printChainList(chains); getchar(); // TODO: printChainListBasic
}

static void scanMethodList(List *list){
    if(list) { scanMethodList(list->next); compileMethod(list->val); }
}

static void init(){
    currentMethod = NULL;
}

// entry: fuse elementwise
void optElementwise(){
    printBanner("Fusion Elementwise");
    init();
    scanMethodList(compiledMethodList->next);
}
