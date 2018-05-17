#include "global.h"

void *newM(int n){
    void *x = malloc(n);
    memset(x, 0, n);
    if(!x){ error("Memory allocation fails"); }
    return x;
}

void error(const char *s){
    fprintf(stderr,"%s\n", s);
    exit(1);
}

/* initialization */

/*
 * if FunctionType in typerule.h is changed,
 *   the list below may be updated as well
 */
const pFunc ElementWiseFunc[] = {
    /* unary 30 */
    absF, negF, ceilF, floorF, roundF, piF, notF,
    logF, expF, cosF, sinF, tanF, acosF, asinF, atanF, coshF, sinhF,
    tanhF, acoshF, asinhF, atanhF,
    dateF, yearF, monthF, dayF,
    timeF, hourF, minuteF, secondF, millF,
    /* binary 17 */
    ltF, gtF, leqF, geqF, eqF, neqF, plusF, minusF, mulF, divF,
    powerF, modF, andF, orF, nandF, norF, xorF
};

FuncKind FuncMap[999]; /* max # of functions, must > totalFunc */

static void initFuncKind(){
    if(totalFunc >= 999) error("Size of FuncMap is not enough");
    memset(FuncMap, -1, sizeof(FuncKind)*999);
    DOI(sizeof(ElementWiseFunc)/sizeof(pFunc), FuncMap[ElementWiseFunc[i]]=elemKind)
}



/* entry */
void initGlobal(){
    initTrie();
    initUDChain();
    initFuncKind();
}


