#include "../global.h"
/* check typeshape.h */

/* constants */
const char *FUNCTIONS[] = {
    /* unary 53 */
    "abs", "neg", "ceil", "floor", "round", "conj", "recip", "signum", "pi" , "not",
    "log", "exp",  "cos",   "sin",   "tan", "acos",  "asin",  "atan", "cosh", "sinh",
    "tanh", "acosh", "asinh", "atanh",
    "date", "date_year", "date_month", "date_day",
    "time", "time_hour", "time_minute", "time_second", "time_mill",
    "unique", "str", "len", "range", "fact", "rand", "seed", "flip", "reverse",
    "where", "group", "count", "sum", "avg", "min", "max", "raze", "enlist", "tolist",
    "format",
    /* binary 32 */
    "lt" ,  "gt", "leq" , "geq"  , "eq", "neq" , "plus", "minus" , "mul", "div",
    "power", "log2", "mod", "and", "or", "nand", "nor" , "xor",
    "datetime_diff", "datetime_add", "datetime_sub",
    "append", "like", "compress", "randk", "index_of", "take", "drop", "order",
    "member", "vector", "match",
    /* special 16 */
    "each", "each_item", "each_left", "each_right", "enum", "dict", "table",
    "ktable", "keys", "values", "meta", "column_value", "load_table", "fetch",
    "index", "index_a"
};

static ShapeNode *decideShapeElementwise(InfoNode *x, InfoNode *y);

#define CASE(c, k, x) case k: *num=c; return x; break;
#define commonTrig commonArith1
/* monadic */
#define ruleAbs     commonArith1
#define ruleNeg     commonArith1
#define ruleCeil    commonArith1
#define ruleFloor   commonArith1
#define ruleRound   commonArith1
#define ruleConj    NULL
#define ruleRecip   NULL
#define ruleSignum  NULL
#define rulePi      NULL
#define ruleNot     commonBool1
#define ruleLog     NULL
#define ruleExp     NULL
#define ruleCos     commonTrig
#define ruleSin     commonTrig
#define ruleTan     commonTrig
#define ruleAcos    commonTrig
#define ruleAsin    commonTrig
#define ruleAtan    commonTrig
#define ruleCosh    commonTrig
#define ruleSinh    commonTrig
#define ruleTanh    commonTrig
#define ruleAcosh   commonTrig
#define ruleAsinh   commonTrig
#define ruleAtanh   commonTrig
#define ruleDate    NULL
#define ruleYear    NULL
#define ruleMonth   NULL
#define ruleDay     NULL
#define ruleTime    NULL
#define ruleHour    NULL
#define ruleMinute  NULL
#define ruleSecond  NULL
#define ruleMill    NULL
#define ruleUnique  specialUnique  //return indices
#define ruleStr     NULL
#define ruleLen     NULL
#define ruleRange   NULL
#define ruleFact    NULL
#define ruleRand    NULL
#define ruleSeed    NULL
#define ruleFlip    NULL
#define ruleReverse NULL
#define ruleWhere   NULL
#define ruleGroup   NULL
#define ruleCount   reductionCount
#define ruleSum     reductionSum
#define ruleAvg     NULL
#define ruleMin     NULL
#define ruleMax     NULL
#define ruleRaze    NULL
#define ruleEnlist  specialEnlist
#define ruleTolist  NULL
#define ruleFormat  NULL
/* dyadic */ 
#define ruleLt       commonCompare2
#define ruleGt       commonCompare2
#define ruleLeq      commonCompare2
#define ruleGeq      commonCompare2
#define ruleEq       commonCompare2
#define ruleNeq      commonCompare2
#define rulePlus     commonArith2
#define ruleMinus    commonArith2
#define ruleMul      commonArith2
#define ruleDiv      commonArith2
#define rulePower    commonArith2
#define ruleLog2     NULL
#define ruleMod      commonArith2
#define ruleAnd      commonBool2
#define ruleOr       commonBool2
#define ruleNand     commonBool2
#define ruleNor      commonBool2
#define ruleXor      commonBool2
#define ruleDtdiff   NULL
#define ruleDtadd    NULL
#define ruleDtsub    NULL
#define ruleAppend   NULL
#define ruleLike     NULL
#define ruleCompress specialCompress
#define ruleRandk    NULL
#define ruleIndexof  NULL
#define ruleTake     NULL
#define ruleDrop     NULL
#define ruleOrder    NULL
#define ruleMember   NULL
#define ruleVector   NULL
#define ruleMatch    NULL
/* special */ 
#define ruleEach        NULL
#define ruleEachItem    NULL
#define ruleEachLeft    NULL
#define ruleEachRight   NULL
#define ruleEnum        NULL
#define ruleDict        NULL
#define ruleTable       specialTable
#define ruleKtable      NULL
#define ruleKeys        NULL
#define ruleValues      NULL
#define ruleMeta        NULL
#define ruleColumnValue specialColumnValue
#define ruleLoadTable   specialLoadTable
#define ruleFetch       NULL
#define ruleIndex       NULL
#define ruleIndexA      NULL

#define isT(t) (t==n->type)

#define isBT isT(boolT)
#define isIT isT(i64T)||isT(i32T)||isT(i16T)||isT(i8T)
#define isFT isT(f64T)||isT(f32T)
#define isST isT(symT)||isT(strT)
#define isDT isT(monthT)||isT(dateT)
#define isTT isT(tableT)||isT(ktableT)
#define sameT(x,y) ((x)->type==(y)->type)

bool isInt   (InfoNode *n) {return isIT;}
bool isFloat (InfoNode *n) {return isFT;}
bool isBool  (InfoNode *n) {return isBT;}
bool isReal  (InfoNode *n) {return isIT||isFT||isBT;}
bool isString(InfoNode *n) {return isST;}
bool isDate  (InfoNode *n) {return isDT;}
bool isBasic (InfoNode *n) {return isReal(n)||isDate(n);}
bool isTable (InfoNode *n) {return isTT;}
#define isU(n) (unknownT==(n)->type)
#define isList(n) (listT==(n)->type)

#define isS(n, t) (t == n->type)
#define isShapeU(n) isS(n, unknownH)
#define isShapeV(n) isS(n,  vectorH)
#define isShapeL(n) isS(n,    listH)
#define isShapeT(n) isS(n,   tableH)

typedef bool (*TypeCond)(InfoNode *);
int shapeId = 0;

static InfoNode *newInfoNode(pType type, ShapeNode *shape){
    InfoNode *in = NEW(InfoNode);
    in->type     = type;
    in->shape    = shape;
    return in;
}

ShapeNode *newShapeNode(pShape type, bool isId, int size){
    ShapeNode *sn = NEW(ShapeNode);
    sn->type = type;
    sn->isId = isId;
    switch(type){
        case unknownH: sn->size   = -1;   break;
        case  vectorH: 
        case    listH:
        case   tableH:
              if(isId) sn->sizeId = size<0?(shapeId++):size;
              else sn->size = size; break;
        default: sn->size   = -2;   break;
    }
    return sn;
}

/* monadic */
static InfoNode *commonElemementSingle(InfoNode *x, TypeCond cond, pType t){
    pType rtnType; 
    if(cond(x)||isU(x)) rtnType = t;
    else return NULL;
    return newInfoNode(rtnType, x->shape);
}

static InfoNode *commonReduction(InfoNode *x, TypeCond cond, pType t){
    pType rtnType;
    if(cond(x)||isU(x)) rtnType = t;
    else return NULL;
    return newInfoNode(rtnType, newShapeNode(vectorH, false, 1));
}

static InfoNode *commonArith1(InfoNode *x){
    return commonElemementSingle(x, &isReal, x->type);
} 

static InfoNode *commonBool1(InfoNode *x){
    return commonElemementSingle(x, &isBool, boolT);
}

static InfoNode *reductionSum(InfoNode *x){
    pType rtnType;
    if(isInt(x)||isBool(x)) rtnType = i64T;
    else if(isFloat(x)) rtnType = f64T;
    else if(isU(x)) rtnType = unknownT;
    else return NULL;
    return newInfoNode(rtnType, newShapeNode(vectorH, false, 1));
}

static InfoNode *reductionCount(InfoNode *x){
    return commonReduction(x, &isBasic, i64T);
}

static InfoNode *specialEnlist(InfoNode *x){
    return commonReduction(x, &isBasic, listT);
}

static InfoNode *specialUnique(InfoNode *x){
    return commonElemementSingle(x, &isBasic, i64T);
}

/* dyadic */
static InfoNode *commonArith2(InfoNode *x, InfoNode *y){
    pType rtnType;
    if(isReal(x)&&isReal(x)){
        rtnType = MAX(x->type, y->type);
    }
    else if(isU(x)||isU(y)){
        rtnType = unknownT;
    }
    else return NULL;
    return newInfoNode(rtnType, decideShapeElementwise(x,y));
}

static InfoNode *commonBool2(InfoNode *x, InfoNode *y){
    pType rtnType;
    if(isBool(x)&&isBool(y)){
        rtnType = boolT;
    }
    else if(isU(x) || isU(y)){
        rtnType = unknownT;
    }
    else return NULL;
    return newInfoNode(rtnType, decideShapeElementwise(x,y));
}

static ShapeNode *decideShapeV(ShapeNode *x, ShapeNode *y){
    ShapeNode *rtnShape = NULL;
    if(!(x->isId)){
        if(x->size == 1) rtnShape = y;
        else if(isShapeV(y) && !(y->isId)){
            if(x->size == y->size) rtnShape = y;
            else error("length of both sides should obey the elemetwise rule");
        }
        else rtnShape = newShapeNode(unknownH, true, -1);
    }
    else {
        if(isShapeV(y)){
            if(y->isId && x->sizeId == y->sizeId) rtnShape = x;
            else if(!(y->isId) && y->size == 1) rtnShape = x;
            else rtnShape = newShapeNode(unknownH, true, -1);
        }
        else rtnShape = newShapeNode(unknownH, true, -1);
    }
    return rtnShape;
}

static ShapeNode *decideShapeElementwise(InfoNode *x, InfoNode *y){
    ShapeNode *rtnShape = NULL;
    if(isShapeV(x->shape) || isShapeV(y->shape)){
        rtnShape = decideShapeV(x->shape,y->shape);
    }
    else if(isShapeU(x->shape) || isShapeU(y->shape)){
        rtnShape = newShapeNode(unknownH, true, -1);
    }
    else error("unknown shape case for elementwise");
    return rtnShape;
}

static InfoNode *commonCompare2(InfoNode *x, InfoNode *y){
    pType rtnType; 
    if(isReal(x) && isReal(y)){
        rtnType = boolT;
    }
    else if(isDate(x) && sameT(x,y)){
        rtnType = boolT;
    }
    else if(isU(x) || isU(y)){
        rtnType = unknownT;
    }
    else return NULL;
    return newInfoNode(rtnType, decideShapeElementwise(x,y));
}

/* special */

static InfoNode *specialLoadTable(InfoNode *x){
    if(isString(x)){
        return newInfoNode(tableT, newShapeNode(tableH,true,-1));
    }
    else return NULL;
}
static InfoNode *specialColumnValue(InfoNode *x, InfoNode *y){
    //P("type: column value\n"); printType(x->type); P(" "); printType(y->type); P("\n");
    ShapeNode *rtnShape = newShapeNode(vectorH, true, x->shape->sizeId);
    if(isTable(x) && isString(y)){
        return newInfoNode(unknownT, rtnShape);
    }
    else if(isU(x) || isU(y)){
        return newInfoNode(unknownT, rtnShape);
    }
    else return NULL;
}

static InfoNode *specialCompress(InfoNode *x, InfoNode *y){
    pType rtnType; ShapeNode *rtnShape=y->shape;
    if(isBool(x) && (isBasic(y)||isU(y))){
        rtnType = y->type;
    }
    else if(isU(x) || isU(y)){
        rtnType = unknownT;
    }
    else return NULL;
    return newInfoNode(y->type, rtnShape);
}
static InfoNode *specialTable(InfoNode *x, InfoNode *y){
    ShapeNode *rtnShape = newShapeNode(tableH, true, -1);
    if(isString(x) && isList(y)){
        return newInfoNode(tableT, rtnShape);
    }
    else if(isU(x) || isU(y)){
        return newInfoNode(unknownT, rtnShape);
    }
    else return NULL;
}

/* main */ 
static int findInBuiltinSet(char *funcName, const char *set[]){
    DOI(totalFunc, if(!strcmp(funcName, set[i])) return i)
    return -1;
}

void *fetchTypeRules(char *name, int* num){
    if(sizeof(FUNCTIONS)/8 != totalFunc){
        P("%d vs. %d\n", sizeof(FUNCTIONS)/8, totalFunc);
        error("FUNCTIONS and FunctionType should have the same # of elem.");
    }
    int k = findInBuiltinSet(name, FUNCTIONS);
    if(k>=0){
        switch(k){
            /* monadic */
            CASE(1,    absF, ruleAbs)
            CASE(1,    negF, ruleNeg)
            CASE(1,   ceilF, ruleCeil)
            CASE(1,  floorF, ruleFloor)
            CASE(1,  roundF, ruleRound)
            CASE(1,   conjF, ruleConj)
            CASE(1,  recipF, ruleRecip)
            CASE(1, signumF, ruleSignum)
            CASE(1,     piF, rulePi)
            CASE(1,    notF, ruleNot)
            CASE(1,    logF, ruleLog)
            CASE(1,    expF, ruleExp)
            CASE(1,    cosF, ruleCos)
            CASE(1,    sinF, ruleSin)
            CASE(1,    tanF, ruleTan)
            CASE(1,   acosF, ruleAcos)
            CASE(1,   asinF, ruleAsin)
            CASE(1,   atanF, ruleAtan)
            CASE(1,   coshF, ruleCosh)
            CASE(1,   sinhF, ruleSinh)
            CASE(1,   tanhF, ruleTanh)
            CASE(1,  acoshF, ruleAcosh)
            CASE(1,  asinhF, ruleAsinh)
            CASE(1,  atanhF, ruleAtanh)
            CASE(1,   dateF, ruleDate)
            CASE(1,   yearF, ruleYear)
            CASE(1,  monthF, ruleMonth)
            CASE(1,   timeF, ruleTime)
            CASE(1,   hourF, ruleHour)
            CASE(1, minuteF, ruleMinute)
            CASE(1, secondF, ruleSecond)
            CASE(1,   millF, ruleMill)
            CASE(1, uniqueF, ruleUnique)
            CASE(1,    strF, ruleStr)
            CASE(1,    lenF, ruleLen)
            CASE(1,  rangeF, ruleRange)
            CASE(1,   factF, ruleFact)
            CASE(1,   randF, ruleRand)
            CASE(1,   seedF, ruleSeed)
            CASE(1,   flipF, ruleFlip)
            CASE(1,reverseF, ruleReverse)
            CASE(1,  whereF, ruleWhere)
            CASE(1,  groupF, ruleGroup)
            CASE(1,  countF, ruleCount)
            CASE(1,    sumF, ruleSum)
            CASE(1,    avgF, ruleAvg)
            CASE(1,    minF, ruleMin)
            CASE(1,    maxF, ruleMax)
            CASE(1,   razeF, ruleRaze)
            CASE(1, enlistF, ruleEnlist)
            CASE(1, tolistF, ruleTolist)
            CASE(1, formatF, ruleFormat)
            /* dyadic */
            CASE(2,      ltF, ruleLt)
            CASE(2,      gtF, ruleGt)
            CASE(2,     leqF, ruleLeq)
            CASE(2,     geqF, ruleGeq)
            CASE(2,      eqF, ruleEq)
            CASE(2,     neqF, ruleNeq)
            CASE(2,    plusF, rulePlus)
            CASE(2,   minusF, ruleMinus)
            CASE(2,     mulF, ruleMul)
            CASE(2,     divF, ruleDiv)
            CASE(2,   powerF, rulePower)
            CASE(2,    log2F, ruleLog2)
            CASE(2,     modF, ruleMod)
            CASE(2,     andF, ruleAnd)
            CASE(2,      orF, ruleOr)
            CASE(2,    nandF, ruleNand)
            CASE(2,     norF, ruleNor)
            CASE(2,     xorF, ruleXor)
            CASE(2,  dtdiffF, ruleDtdiff)
            CASE(2,   dtaddF, ruleDtadd)
            CASE(2,   dtsubF, ruleDtsub)
            CASE(2,  appendF, ruleAppend)
            CASE(2,    likeF, ruleLike)
            CASE(2,compressF, ruleCompress)
            CASE(2,   randkF, ruleRandk)
            CASE(2, indexofF, ruleIndexof)
            CASE(2,    takeF, ruleTake)
            CASE(2,    dropF, ruleDrop)
            CASE(2,   orderF, ruleOrder)
            CASE(2,  memberF, ruleMember)
            CASE(2,  vectorF, ruleVector)
            CASE(2,   matchF, ruleMatch)
            /* special */
            CASE(0,        eachF, ruleEach)
            CASE(0,    eachItemF, ruleEachItem)
            CASE(0,    eachLeftF, ruleEachLeft)
            CASE(0,   eachRightF, ruleEachRight)
            CASE(0,        enumF, ruleEnum)
            CASE(0,        dictF, ruleDict)
            CASE(2,       tableF, ruleTable)
            CASE(0,      ktableF, ruleKtable)
            CASE(0,        keysF, ruleKeys)
            CASE(0,      valuesF, ruleValues)
            CASE(0,        metaF, ruleMeta)
            CASE(2, columnValueF, ruleColumnValue)
            CASE(1,   loadTableF, ruleLoadTable)
            CASE(0,       fetchF, ruleFetch)
            CASE(0,       indexF, ruleIndex)
            CASE(0,      indexAF, ruleIndexA)
            default: error("type rules not defined.");
        }
    }
    else error("primitive not defined");
}

int findFuncIndex(char *funcName){
    return findInBuiltinSet(funcName, FUNCTIONS);
}

