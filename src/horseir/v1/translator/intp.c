#include "../global.h"

typedef I (*NiladicFunc)(V);
typedef I (*MonadicFunc)(V,V);
typedef I (*DyadicFunc)(V,V,V);
typedef I (*AnyadicFunc)(V,L,V[]);
typedef I (*EachMonadic)(V,V,MonadicFunc);
typedef I (*EachDyadic)(V,V,V,DyadicFunc);
typedef I (*EachTriple)(V,V,V,V);

#define MonFuncSize 60
#define DyaFuncSize 33
#define OuterProduct EachDyadic
#define JoinOperation EachTriple

/* pfnRand, pfnSeed, pfnCount */
MonadicFunc monFunc[MonFuncSize] = {
    pfnAbs, pfnNeg, pfnCeil, pfnFloor, pfnRound, pfnConj, pfnRecip, pfnSignum, pfnPi  , pfnNot ,
    pfnLog, pfnLog2, pfnLog10, pfnExp, pfnTrigCos, pfnTrigSin, pfnTrigTan, pfnTrigAcos, pfnTrigAsin,
    pfnTrigAtan, pfnHyperCosh, pfnHyperSinh, pfnHyperTanh, pfnHyperAcosh, pfnHyperAsinh, pfnHyperAtanh,
    pfnDate, pfnDateYear, pfnDateMonth, pfnDateDay,
    pfnTime, pfnTimeHour, pfnTimeMinute, pfnTimeSecond, pfnTimeMill,
    pfnUnique, NULL, pfnLen, pfnRange, pfnFact, NULL, NULL, pfnFlip, pfnReverse,
    pfnWhere, pfnGroup, NULL, pfnSum, pfnAvg, pfnMin, pfnMax, pfnRaze, pfnEnlist, pfnToList,
    NULL, pfnKeys, pfnValues, pfnMeta, pfnLoadTable, pfnFetch
};

/* pfnRandk, pfnDrop, pfnTake, pfnDatetimeAdd, pfnDatetimeSub */
DyadicFunc  dyaFunc[DyaFuncSize] = {
    pfnLt, pfnGt, pfnLeq, pfnGeq, pfnEq, pfnNeq, pfnPlus, pfnMinus, pfnMul, pfnDiv,
    pfnPower, pfnLogBase, pfnMod, pfnAnd, pfnOr, pfnNand, pfnNor, pfnXor,
    NULL,
    pfnAppend, pfnLike, pfnCompress, NULL, pfnIndexOf, NULL, NULL, pfnOrderBy,
    pfnMember, pfnVector, pfnMatch, pfnIndex, pfnColumnValue, pfnSubString
};

static V executeMon(MonadicFunc f, V *p){
    V z = NEW(V0);
    I status = (*f)(z,p[0]);
    if(status==0) return z;
    else {P("[Monadic]"); printErrMsg(status);}
}

static V executeDya(DyadicFunc f, V *p){
    V z = NEW(V0);
    I status = (*f)(z, p[0], p[1]);
    if(status==0) return z;
    else {P("[Dyadic]"); printErrMsg(status);}
}

static V executeAny(AnyadicFunc f, V *p, L n){
    V z = NEW(V0);
    I status = (*f)(z, n, p);
    if(status==0) return z;
    else {P("[Anyadic]"); printErrMsg(status);}
}

static V executeIndexA(V *p){
    V z = NEW(V0);
    I status = pfnIndexA(z, p[0], p[1], p[2]);
    if(status==0) return z;
    else {P("[IndexA]"); printErrMsg(status);}
}

static V executeOuter(OuterProduct f, V *p){
    if(H_DEBUG) P("executeOuter\n");
    V z = NEW(V0);
    S funcName = getSymbolStr(vq(p[0]));
    pFunc fIndex = getFuncIndexByName(funcName);
    if(fIndex >= eachF || fIndex <ltF) EP("[Product/JoinIndex] (%s) not supported.\n", funcName);
    L valence  = getValence(fIndex);
    if(valence != 2) EP("dyadic op expected for each_left/right, not %s\n", funcName);
    I status = (*f)(z, p[1], p[2], dyaFunc[fIndex-ltF]);
    if(status==0) return z;
    else {printErrMsg(status);}
}

static V executeTriple(EachTriple f, V *p){
    if(H_DEBUG) P("executeTriple\n");
    V z = NEW(V0);
    I status = (*f)(z, p[0], p[1], p[2]);
    if(status==0) return z;
    else {printErrMsg(status);}
}

static V executeJoinIndex(JoinOperation f, V *p){
    if(H_DEBUG) P("executeJoinIndex\n");
    V z = NEW(V0);
    I status = (*f)(z, p[1], p[2], p[0]);
    if(status==0) return z;
    else {printErrMsg(status);}
}

static V executeEachDya(EachDyadic f, V *p){
    if(H_DEBUG) P("executeEachDya\n");
    V z = NEW(V0);
    S funcName = getSymbolStr(vq(p[0]));
    pFunc fIndex = getFuncIndexByName(funcName);
    if(fIndex >= eachF || fIndex <ltF) EP("[EachDya] (%s) not supported.\n", funcName);
    L valence  = getValence(fIndex);
    if(valence != 2) EP("dyadic op expected for each_left/right, not %s\n", funcName);
    I status = (*f)(z, p[1], p[2], dyaFunc[fIndex-ltF]);
    if(status==0) return z;
    else {P("[EachDyadic]"); printErrMsg(status);}
}

static V executeEachMon(EachMonadic f, V *p){
    if(H_DEBUG) P("executeEachMon\n");
    V z = NEW(V0);
    S funcName = getSymbolStr(vq(p[0]));
    pFunc fIndex = getFuncIndexByName(funcName);
    if(fIndex >= ltF) EP("Not supported: %s\n", funcName);
    L valence  = getValence(fIndex);
    if(valence != 1) EP("Monadic op expected for each, not %s\n", funcName);
    I status = (*f)(z, p[1], monFunc[fIndex]);
    if(status==0) return z;
    else {WP("[EachMonadic]"); printErrMsg(status);}
}

static V loadParam2V(Node *param){
    Node *n = param->val.nodeS;
    //printNodeKind(n);
    if(instanceOf(n, idK)){
        //P("loading: %s\n", fetchName(n));
        return getValueFromNameTable(fetchName(n));
    }
    else {
        return getLiteralFromNode(param);
    }
}

static V* getParamsFromExpr(Node *expr, I *nParam){
    Node *param = expr->val.expr.param;
    List *ptr = param->val.listS;
    I tot = 0;
    while(ptr){ tot++; ptr = ptr->next; }
    V *vals = NEW2(V0, tot); //
    ptr = param->val.listS;
    tot=0; while(ptr){ vals[tot]=loadParam2V(ptr->val); ptr = ptr->next; tot++; }
    *nParam = tot;
    return vals;
}

static V processStmtCommon(Node *expr){
    I np = 0;
    S funcName = fetchName(expr->val.expr.func);
    //P("funcName: %s\n", funcName);
    V *params  = getParamsFromExpr(expr, &np);
    pFunc fIndex = getFuncIndexByName(funcName);
    I valence = getValence(fIndex);
    //P("valence = %d, np = %d\n", valence, np);
    if(np == valence || valence == -1){ //-1: any
        if(fIndex < eachF) {
            if(valence == 1){
                //P("monadic %d\n", fIndex);
                return executeMon(monFunc[fIndex], params);
            }
            else if(valence == 2){
                //P("dyadic %d\n", fIndex-ltF);
                return executeDya(dyaFunc[fIndex-ltF], params);
            }
            else EP("valence == %d not expected\n", valence);
        }
        else {
            switch(fIndex){
                /* monadic */
                case        eachF: return executeEachMon(&pfnEach   , params); break;
                /* dyadic */
                case        enumF: return executeDya(&pfnEnum       , params); break;
                case       tableF: return executeDya(&pfnTable      , params); break;
                case   eachRightF: return executeEachDya(&pfnEachRight , params); break;
                case    eachLeftF: return executeEachDya(&pfnEachLeft  , params); break;
                case    eachItemF: return executeEachDya(&pfnEachItem  , params); break;
                case      indexAF: return executeIndexA(params); break;
                /* anyadic */
                case        listF: return executeAny(&pfnList       , params, np); break;
                /* others */
                case       outerF: return executeOuter(&pfnOuter, params); break;
                case   joinIndexF: return executeJoinIndex(&pfnJoinIndex, params); break;
                case       dtaddF: return executeTriple(&pfnDatetimeAdd, params); break;
                case       dtsubF: return executeTriple(&pfnDatetimeSub, params); break;
                default: EP("pending ... for %d\n", fIndex);
            }
        }
    }
    else EP("valence error: %d expected, but %d found\n", valence, np);
    return NULL;
}

static void runSimpleStmt(Node *stmt){
    S writeName = fetchName(stmt->val.simpleStmt.name);
    Node *expr = stmt->val.simpleStmt.expr;
    if(expr->val.expr.func){
        saveToNameTable(writeName, processStmtCommon(expr));
        // if(!strcmp(writeName, "t2")){
        //     printV(getValueFromNameTable(writeName));
        // }
    }
    else { // assignment
        Node *param = expr->val.expr.param;
        saveToNameTable(writeName, loadParam2V(param));
    }
}
static void runCastStmt(Node *stmt){
    S writeName = fetchName(stmt->val.simpleStmt.name);
    Node *expr = stmt->val.castStmt.expr;
    // check type casting before assigning?
    saveToNameTable(writeName, processStmtCommon(expr));
}
static V runReturnStmt(Node *stmt){
    Node *param = stmt->val.nodeS;
    return loadParam2V(param);
}

static I runSingleStmt(Node *stmt, V *rtn){
#ifdef PROFILE
    struct timeval tv0, tv1;
    gettimeofday(&tv0, NULL);
#endif
    switch(stmt->kind){
        case simpleStmtK: runSimpleStmt(stmt); break;
        case   castStmtK: runCastStmt(stmt);   break;
        case     returnK: *rtn = runReturnStmt(stmt); break;
        default: printNodeKind(stmt);
                 EP("Kind not supported yet: %s\n", getKindName(stmt->kind));
    }
#ifdef PROFILE
    gettimeofday(&tv1, NULL);
    E elapsed = calcInterval(tv0, tv1);
    if(elapsed >= 1)
        P(">> Time (ms): %g\n\n", elapsed);
#endif
    return 0;
}

static I runMethod(Node *method){
    List *stmts = method->val.method.list;
    V rtn = NULL;
    while(stmts){
        P("[%3d]-> ",stmts->val->lineno); prettyNode(stmts->val); P("\n");
        runSingleStmt(stmts->val, &rtn);
        if(instanceOf(stmts->val, returnK)) break;
        stmts = stmts->next;
    }
    if(rtn){
        P("Result:\n"); printV2(rtn, 20);
    }
    return 0;
}

/* entry */
I HorseInterpreter(Prog *rt){
    Node *method = findMethod(findModule(rt->module_list, "default")->val.module.body, "main");
    runMethod(method);
    printSymInfo();
    printHeapInfo();
    return 0;
}

