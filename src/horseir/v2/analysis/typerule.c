#include "../global.h"

const char *FunctionUnaryStr[] = { /* unary 58 */
    "abs", "neg", "ceil", "floor", "round", "conj", "recip", "signum", "pi" ,
    "not", "log", "log2", "log10", "exp",  "cos",   "sin",   "tan", "acos",
    "asin",  "atan", "cosh", "sinh", "tanh", "acosh", "asinh", "atanh", "date",
    "date_year", "date_month", "date_day", "time", "time_hour", "time_minute",
    "time_second", "time_mill", "unique", "str", "len", "range", "fact",
    "rand", "seed", "flip", "reverse", "where", "group", "sum", "avg",
    "min", "max", "raze", "tolist", "keys", "values",
    "meta", "load_table", "fetch", "print"
};

const char *FunctionBinaryStr[] = { /* binary 32 */
    "lt" ,  "gt", "leq" , "geq"  , "eq", "neq" , "plus", "minus" , "mul",
    "div", "power", "logb", "mod", "and", "or", "nand", "nor" , "xor",
    "append", "like", "compress", "randk", "index_of", "take",
    "drop", "order", "member", "vector", "match", "index", "column_value",
    "sub_string"
};

const char *FunctionOtherStr[] = { /* special 13  */
    "each", "each_item", "each_left", "each_right", "enum", "dict", "table",
    "ktable", "index_a", "list", "join_index", "datetime_add",
    "datetime_sub"
};


const int  UnarySize = sizeof( FunctionUnaryStr)/sizeof(char*);
const int BinarySize = sizeof(FunctionBinaryStr)/sizeof(char*);
const int  OtherSize = sizeof( FunctionOtherStr)/sizeof(char*);

static int shapeId = 0;
static int scanId  = 100; /* starting from 100, must > 0 bcz of simple hash */
extern sHashTable *hashScan, *hashMeta; /* declared in typeshape.c */

typedef bool (*TypeCond)(InfoNode *);
static ShapeNode *decideShapeElementwise(InfoNode *x, InfoNode *y);

#define CASE(k, x) case k: return x;
#define DEFAULT(x) default: EP("NOT found: %s",x)

// rules
#define commonTrig commonArith1

/* monadic */
#define ruleAbs         commonArith1
#define ruleNeg         commonArith1
#define ruleCeil        commonArith1
#define ruleFloor       commonArith1
#define ruleRound       commonArith1
#define ruleConj        propConj
#define ruleRecip       propRecip
#define ruleSignum      propSignum
#define rulePi          commonRealClex
#define ruleNot         commonBool1
#define ruleLog         commonRealClex
#define ruleLog2        commonRealClex
#define ruleLog10       commonRealClex
#define ruleExp         commonRealClex
#define ruleCos         commonTrig
#define ruleSin         commonTrig
#define ruleTan         commonTrig
#define ruleAcos        commonTrig
#define ruleAsin        commonTrig
#define ruleAtan        commonTrig
#define ruleCosh        commonTrig
#define ruleSinh        commonTrig
#define ruleTanh        commonTrig
#define ruleAcosh       commonTrig
#define ruleAsinh       commonTrig
#define ruleAtanh       commonTrig
#define ruleDate        propDate
#define ruleYear        propYear
#define ruleMonth       propMonth
#define ruleDay         propDay
#define ruleTime        propTime
#define ruleHour        propHour
#define ruleMinute      propMinute
#define ruleSecond      propSecond
#define ruleMill        propMill
#define ruleUnique      specialUnique  //return indices
#define ruleStr         propStr
#define ruleLen         propLen
#define ruleRange       propRange
#define ruleFact        propFact
#define ruleRand        commonRand
#define ruleSeed        commonRand
#define ruleFlip        propFlip
#define ruleReverse     propReverse
#define ruleWhere       propWhere
#define ruleGroup       propGroup
#define ruleSum         reductionSum
#define ruleAvg         propAvg
#define ruleMin         propMaxMin
#define ruleMax         propMaxMin
#define ruleRaze        propRaze
#define ruleTolist      propToList
#define rulePrint       propPrint

/* dyadic */ 
#define ruleLt          commonCompare2
#define ruleGt          commonCompare2
#define ruleLeq         commonCompare2
#define ruleGeq         commonCompare2
#define ruleEq          commonCompare2
#define ruleNeq         commonCompare2
#define rulePlus        commonArith2
#define ruleMinus       commonArith2
#define ruleMul         commonArith2
#define ruleDiv         commonArith2
#define rulePower       commonPower
#define ruleLogBase     commonPower
#define ruleMod         commonArith2
#define ruleAnd         commonBool2
#define ruleOr          commonBool2
#define ruleNand        commonBool2
#define ruleNor         commonBool2
#define ruleXor         commonBool2
#define ruleDtadd       commonDtChange
#define ruleDtsub       commonDtChange
#define ruleAppend      propAppend
#define ruleLike        propLike
#define ruleCompress    specialCompress
#define ruleRandk       propRandK
#define ruleIndexof     propIndexof
#define ruleTake        commonTakeDrop
#define ruleDrop        commonTakeDrop
#define ruleOrder       propOrder
#define ruleMember      propMember
#define ruleVector      propVector
#define ruleMatch       propMatch
#define ruleIndex       propIndex
#define ruleColumnValue specialColumnValue
#define ruleSubString   propSubString

/* special */ 
#define ruleEach        propEach
#define ruleEachItem    propEachItem
#define ruleEachLeft    propEachLeft
#define ruleEachRight   propEachRight
#define ruleEnum        propEnum
#define ruleDict        propDict
#define ruleTable       specialTable
#define ruleKtable      specialKTable
#define ruleKeys        propKeys
#define ruleValues      propValues
#define ruleMeta        propMeta
#define ruleLoadTable   specialLoadTable
#define ruleFetch       propFetch
#define ruleIndexA      propIndexA
#define ruleList        propList
#define ruleJoinIndex   propJoinIndex

#define isT(n,t) (t==inType(n))

#define isBT(n) isT(n,boolT)
#define isIT(n) isT(n,i64T)||isT(n,i32T)||isT(n,i16T)||isT(n,i8T)
#define isFT(n) isT(n,f64T)||isT(n,f32T)
#define is32(n) isT(n,f32T)||isT(n,i32T)||isT(n,i16T)||isT(n,i8T)
#define is64(n) isT(n,i64T)||isT(n,f64T)
#define isXT(n) isT(n,clexT)
#define isCT(n) isT(n,charT)
#define isST(n) isT(n,symT)||isT(n,strT)
#define isDT(n) isT(n,monthT)||isT(n,dateT)||isT(n,dtT)
#define isTM(n) isT(n,timeT)||isT(n,minuteT)||isT(n,secondT)
#define isNT(n) isT(n,dictT)
#define isTT(n) isT(n,tableT)||isT(n,ktableT)
#define sameT(x,y) (inType(x)==inType(y))

bool isIntIN     (InfoNode *n) {return isIT(n);}
bool isFloatIN   (InfoNode *n) {return isFT(n);}
bool isBoolIN    (InfoNode *n) {return isBT(n);}
bool isClexIN    (InfoNode *n) {return isXT(n);}
bool isCharIN    (InfoNode *n) {return isCT(n);}
bool isRealIN    (InfoNode *n) {return isIT(n)||isFT(n)||isBT(n);}
bool isNumericIN (InfoNode *n) {return isRealIN(n)||isClexIN(n);}
bool isStringIN  (InfoNode *n) {return isST(n);} // TODO: split sym and str
bool isDateIN    (InfoNode *n) {return isDT(n);}
bool isTimeIN    (InfoNode *n) {return isTM(n);}
bool isDtIN      (InfoNode *n) {return isDT(n)||isTM(n);}
bool isBasicIN   (InfoNode *n) {return 
  isRealIN(n)||isCharIN(n)||isStringIN(n)||isDateIN(n)||isTimeIN(n);}
bool isDictIN    (InfoNode *n) {return isNT(n);}
bool isFuncIN    (InfoNode *n) {return isT(n,funcT);}
bool isTableIN   (InfoNode *n) {return isTT(n);}
bool isReal32IN  (InfoNode *n) {return is32(n);}
bool isReal64IN  (InfoNode *n) {return is64(n);}

static InfoNode *newInfoNode(Type type, ShapeNode *shape){
    InfoNode *in = NEW(InfoNode);
    in->type    = type;
    in->shape   = shape;
    in->subInfo = NULL;
    in->next    = NULL;
    return in;
}

static InfoNode *newInfoNodeAll(
        Type type, ShapeNode *shape, InfoNode *sub, InfoNode *next){
    InfoNode *in = NEW(InfoNode);
    in->type     = type;
    in->shape    = shape;
    in->subInfo  = sub;
    in->next     = next;
    return in;
}

// x,y have diff types, but compatible
static bool compatibleT(InfoNode *x, InfoNode *y){
    switch(inType(x)){
        case  i16T: return isT(y,boolT);
        case  i32T: return isT(y,boolT)||isT(y,i16T);
        case  i64T: return isT(y,boolT)||isT(y,i16T)||isT(y,i32T);
        case  f32T: return isIntIN(y);
        case  f64T: return isIntIN(y)||isT(y,f32T);
        default: break;
    }
    return false;
}

bool checkType(InfoNode *x, InfoNode *y){
    if(sameT(x,y)) return true;
    if(compatibleT(x,y)) return true;
    return false;
}

#define copyShapeNode(x, y) *(x)=*(y)

// maybe not used any more
bool checkShape(InfoNode *x, InfoNode *y){
    ShapeNode *sx = x->shape;
    ShapeNode *sy = y->shape;
    if(sx){
        if(sx->type == unknownH){
            copyShapeNode(sx, sy);
        }
        else if(sx->type != sy->type) return false;
        else {
            switch(sx->kind){
                case constSP:
                    if(sx->size != sy->size) { TODO("update size\n"); }
                    break;
                case symbolSP:
                    if(sx->sizeId != sy->sizeId) { TODO("fix size\n"); }
                    break;
                case scanSP:
                    if(sx->sizeScan != sy->sizeScan) { TODO("error size\n"); }
                    break;
            }
        }
        return true;
    }
    else { // if null, copy y's shape
        x->shape = sy;
        return true;
    }
}

ShapeNode *newShapeNode(ShapeType type, int kind, int size){
    ShapeNode *sn = NEW(ShapeNode);
    sn->type = type;
    sn->kind = kind;
    switch(type){
        case unknownH: sn->size = -1; break;
        case  vectorH: 
        case    listH:
        case   tableH:
              //P("kind = %d, size = %d\n", kind, size); getchar();
              if(isSNId(sn))
                  sn->sizeId = size<0?shapeId++:size;
              else if(isSNConst(sn))
                  sn->size = size;
              else if(isSNScan(sn))
                  sn->sizeScan = size;
              else EP("unknow kind = %d\n", kind); break;
              //if(isId) sn->sizeId = size<0?(shapeId++):size;
              //else sn->size = size; break;
        default: sn->size = -2; break;
    }
    return sn;
}

ShapeNode *newShapeNodeScan(ShapeNode *x){
    /* map(x, size) */
    int curId = lookupSimpleHash(hashScan, (L)x);
    if(!curId){ // not found
        addToSimpleHash(hashScan, (L)x, curId=scanId++);
    }
    ShapeNode *sn = NEW(ShapeNode);
    sn->type = vectorH; // default
    sn->kind = SN_SCAN; // default
    sn->sizeScan = curId;
    return sn;
}


/* monadic */
static InfoNode *commonElemementUnary(InfoNode *x, TypeCond cond, Type t){
    Type rtnType; 
    if(cond(x)||isW(x)) rtnType = t;
    else return NULL;
    return newInfoNode(rtnType, inShape(x));
}

static InfoNode *commonReduction(InfoNode *x, TypeCond cond, Type t){
    Type rtnType;
    if(cond(x)||isW(x)) rtnType = t;
    else return NULL;
    return newInfoNode(rtnType, newShapeNode(vectorH, SN_CONST, 1));
}

static InfoNode *commonArith1(InfoNode *x){
    return commonElemementUnary(x, &isRealIN, inType(x));
} 

static InfoNode *commonBool1(InfoNode *x){
    return commonElemementUnary(x, &isBoolIN, boolT);
}

static InfoNode *commonRealClex(InfoNode *x){
    Type rtnType;
    if(isReal32IN(x)){
        rtnType = f32T;
    }
    else if(isReal64IN(x)){
        rtnType = f64T;
    }
    else if(isClexIN(x)){
        rtnType = clexT;
    }
    else if(isW(x)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, inShape(x));
}

static InfoNode *reductionSum(InfoNode *x){
    Type rtnType;
    if(isIntIN(x)||isBoolIN(x)) rtnType = i64T;
    else if(isFloatIN(x)) rtnType = f64T;
    else if(isW(x)) rtnType = wildT;
    else return NULL;
    return newInfoNode(rtnType, newShapeNode(vectorH, SN_CONST, 1));
}

static InfoNode *specialUnique(InfoNode *x){
    return commonElemementUnary(x, &isBasicIN, i64T);
}

/* propagation functions */

static InfoNode *propGroup(InfoNode *x){
    return newInfoNode(dictT, newShapeNode(listH, SN_CONST, 2));
}

static InfoNode *propRaze(InfoNode *x){
    Type rtnType;
    if(isListT(x)){
        rtnType = getSubType(x);
    }
    else if(isBasicIN(x)){
        return x;
    }
    else if(isW(x)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, newShapeNode(vectorH, SN_ID, -1));
}

static InfoNode *propToList(InfoNode *x){ 
    if(isBasicIN(x)){
        Type rtnType = listT;
        ShapeType rtnShape = listH;
        ShapeNode *p = inShape(x);
        if(isShapeV(p) && isSNConst(p))
            return newInfoNode(rtnType, newShapeNode(rtnShape, SN_CONST, p->size));
        else
            return newInfoNode(rtnType, newShapeNode(rtnShape, SN_ID, -1));
    }
    else if(isW(x)){
        return newInfoNode(wildT, NULL);
    }
    else return NULL;
}

static InfoNode *propStr(InfoNode *x){ 
    return newInfoNode(strT, newShapeNode(vectorH, SN_CONST, 1));
}

/* return a scalar */
static InfoNode *propLen(InfoNode *x){ 
    return newInfoNode(i64T, newShapeNode(vectorH, SN_CONST, 1));
}

static InfoNode *propRange(InfoNode *x){ 
    Type rtnType;
    if(isIntIN(x)){
        rtnType = i64T;
    }
    else if(isW(x)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(i64T, newShapeNode(vectorH, SN_ID, -1));
}

static InfoNode *propFact(InfoNode *x){ 
    Type rtnType;
    if(isIntIN(x)){
        rtnType = i64T;
    }
    else if(isW(x)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, inShape(x));
}

static InfoNode *commonRand(InfoNode *x){ 
    Type rtnType;
    if(isIntIN(x)){
        rtnType = i64T;
    }
    else if(isW(x)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, newShapeNode(vectorH, SN_CONST, 1));
}

static InfoNode *propFlip(InfoNode *x){
    Type rtnType;
    ShapeType rtnShape;
    if(isDictIN(x)){
        rtnType = tableT;
        rtnShape = tableH;
    }
    else if(isTableIN(x)){
        rtnType = dictT;   // TODO: Implement
        rtnShape = dictH;
    }
    else if(isT(x,listT)){
        rtnType = listT;
        rtnShape = listH;
    }
    else if(isW(x)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, newShapeNode(rtnShape, SN_ID, -1));
}

static InfoNode *propReverse(InfoNode *x){
    Type rtnType;
    if(isBasicIN(x)){
        rtnType = inType(x);
    }
    else if(isW(x)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, inShape(x));
}

static InfoNode *propWhere(InfoNode *x){
    Type rtnType;
    if(isBoolIN(x)){
        rtnType = i64T;
    }
    else if(isW(x)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, newShapeNodeScan(inShape(x)));
    //return newInfoNode(rtnType, newShapeNode(vectorH, SN_ID, -1));
}

static InfoNode *propAvg(InfoNode *x){
    Type rtnType;
    if(isRealIN(x)){
        rtnType = f64T;
    }
    else if(isW(x)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, newShapeNode(vectorH, SN_CONST, 1));
}

static InfoNode *propMaxMin(InfoNode *x){
    Type rtnType;
    if(isRealIN(x)){
        rtnType = inType(x);
    }
    else if(isW(x)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, newShapeNode(vectorH, SN_CONST, 1));
}

static InfoNode *propDate(InfoNode *x){
    Type rtnType;
    if(isT(x, dtT)){
        rtnType = dateT;
    }
    else if(isW(x)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, inShape(x));
}

static InfoNode *propYear(InfoNode *x){
    Type rtnType;
    if(isDateIN(x)){
        rtnType = i16T;
    }
    else if(isW(x)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, inShape(x));
}

#define propMonth propYear

static InfoNode *propDay(InfoNode *x){
    Type rtnType;
    if(isT(x,dateT) && isT(x,dtT)){
        rtnType = i16T;
    }
    else if(isW(x)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, inShape(x));
}

static InfoNode *propTime(InfoNode *x){
    Type rtnType;
    if(isT(x, dtT)){
        rtnType = timeT;
    }
    else if(isW(x)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, inShape(x));
}

static InfoNode *propHour(InfoNode *x){
    Type rtnType;
    if(isT(x, dtT) || isTimeIN(x)){
        rtnType = i16T;
    }
    else if(isW(x)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, inShape(x));
}

#define propMinute propHour

static InfoNode *propSecond(InfoNode *x){
    Type rtnType;
    if(isT(x, dtT) || isT(x, secondT)){
        rtnType = i16T;
    }
    else if(isW(x)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, inShape(x));
}

static InfoNode *propMill(InfoNode *x){
    Type rtnType;
    if(isT(x, dtT)){
        rtnType = i16T;
    }
    else if(isW(x)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, inShape(x));
}

static InfoNode *propPrint(InfoNode *x){
    Type rtnType;
    if(isW(x)){
        rtnType = wildT;
    }
    else {
        rtnType = i64T;
    }
    return newInfoNode(rtnType, newShapeNode(vectorH, SN_CONST, 1));
     // or @print returns nothing
}

static InfoNode *propConj(InfoNode *x){
    Type rtnType;
    if(isClexIN(x)){
        rtnType = clexT;
    }
    else return NULL;
    return newInfoNode(rtnType, inShape(x));
}

static InfoNode *propRecip(InfoNode *x){
    Type rtnType;
    if(isReal32IN(x)){
        rtnType = f32T;
    }
    else if(isReal64IN(x)){
        rtnType = f64T;
    }
    else return NULL;
    return newInfoNode(rtnType, inShape(x));
}

static InfoNode *propSignum(InfoNode *x){
    if(isRealIN(x)){
        return newInfoNode(i8T, inShape(x));
    }
    else return NULL;
}


/* dyadic */
static InfoNode *commonArith2(InfoNode *x, InfoNode *y){
    Type rtnType;
    if(isRealIN(x) && isRealIN(x)){
        rtnType = MAX(inType(x), inType(y));
    }
    else if(isW(x) || isW(y)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, decideShapeElementwise(x,y));
}

static InfoNode *commonBool2(InfoNode *x, InfoNode *y){
    Type rtnType;
    if(isBoolIN(x)&&isBoolIN(y)){
        rtnType = boolT;
    }
    else if(isW(x) || isW(y)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, decideShapeElementwise(x,y));
}

static InfoNode *commonPower(InfoNode *x, InfoNode *y){
    Type rtnType;
    if(isRealIN(x) && isRealIN(y)){
        Type maxType = MAX(inType(x), inType(y));
        rtnType = (maxType==f64T || maxType==i64T)?f64T:f32T;
    }
    else if(isW(x) || isW(y)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, inShape(y));
}

static InfoNode *propMember(InfoNode *x, InfoNode *y){
    Type rtnType;
    //P("type: x(%s), y(%s)\n",getpTypeName(x->type),getpTypeName(y->type));
    if(sameT(x,y) && isBasicIN(x)){
        rtnType = boolT;
    }
    else if(isW(x) || isW(y)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, decideShapeLeft(x,y));
}

// decide shapes

static ShapeNode *decideShapeElementwiseV(ShapeNode *x, ShapeNode *y){
    // x and y are both vectors
    ShapeNode *rtnShape = NULL;
    if(isSNConst(x)){
        if(x->size == 1) rtnShape = y;
        else if(isSNConst(y)){
            if(x->size == y->size || y->size == 1) rtnShape = x;
            else EP("length of both sides should obey the elemetwise rule\n");
        }
        else rtnShape = newShapeNode(unknownH, SN_ID, -1);
    }
    else if(isSNId(x)){
        if(x->sizeId == y->sizeId) rtnShape = x;
        else if(isSNConst(y) && y->size == 1) rtnShape = x;
        else rtnShape = newShapeNode(unknownH, SN_ID, -1);
    }
    else if(isSNScan(x)){
        if(isSNScan(y) && x->sizeScan == y->sizeScan) rtnShape = x;
        else if(isSNConst(y) && y->size == 1) rtnShape = x;
        else rtnShape = newShapeNode(unknownH, SN_ID, -1);
    }
    else EP("unknown kind = %d\n",x->kind);
    return rtnShape;
}

static ShapeNode *decideShapeElementwise(InfoNode *x, InfoNode *y){
    ShapeNode *rtnShape = NULL;
    if(isShapeV(inShape(x)) && isShapeV(inShape(y))){
        rtnShape = decideShapeElementwiseV(inShape(x), inShape(y));
    }
    else if(isShapeU(inShape(x)) || isShapeU(inShape(y))){
        rtnShape = newShapeNode(unknownH, SN_ID, -1);
    }
    else error("unknown shape case for elementwise");
    return rtnShape;
}

static ShapeNode *decideShapeAppendV(ShapeNode *x, ShapeNode *y){
    if(isSNId(x) || isSNId(y)){
        return newShapeNode(vectorH, SN_ID, -1);
    }
    else {
        return newShapeNode(vectorH, SN_CONST, x->size+y->size);
    }
}

static ShapeNode *decideShapeAppend(InfoNode *x, InfoNode *y){
    ShapeNode *rtnShape = NULL;
    if(isShapeV(inShape(x)) && isShapeV(inShape(y))){
        rtnShape = decideShapeAppendV(inShape(x), inShape(y));
    }
    else if(isShapeU(inShape(x)) || isShapeU(inShape(y))){
        rtnShape = newShapeNode(unknownH, SN_ID, -1);
    }
    else EP("unknown shape case for append\n");
    return rtnShape;
}

static ShapeNode *decideShapeCompressV(ShapeNode *x, ShapeNode *y){
    if(isSNId(x) && isSNId(y)){
        if(x->sizeId == y->sizeId)
            return newShapeNodeScan(x);
        else
            EP("scan shape sizes must be the same: %d vs %d", x->sizeId, y->sizeId);
    }
    else if(isSNConst(x) && isSNConst(y)){
        if(x->size == y->size)
            return newShapeNode(vectorH, SN_ID, -1);
        else
            EP("shape size not equal for comrpess: %d vs %d", x->size, y->size);
    }
    else if(isSNScan(x) && isSNScan(y)){
        if(x->sizeScan == y->sizeScan)
            return newShapeNodeScan(x);
        else
            EP("scan shape sizes must be the same: %d vs %d", x->sizeScan, y->sizeScan);
    }
    else {
        printShapeNode(x); P("\n");
        printShapeNode(y); P("\n");
        EP("unknown ShapeNode for compress");
    }
}

static ShapeNode *decideShapeCompress(InfoNode *x, InfoNode *y){
    ShapeNode *rtnShape = NULL;
    if(isShapeV(inShape(x)) && isShapeV(inShape(y))){
        rtnShape = decideShapeCompressV(inShape(x), inShape(y));
    }
    else if(isShapeU(inShape(x)) || isShapeU(inShape(y))){
        rtnShape = newShapeNode(unknownH, SN_ID, -1);
    }
    else EP("unknown InfoNode for compress\n");
    return rtnShape;
}

/* decide shape functions stop here */

static InfoNode *commonCompare2(InfoNode *x, InfoNode *y){
    Type rtnType; 
    if(isRealIN(x) && isRealIN(y)){
        rtnType = boolT;
    }
    else if(sameT(x,y) && isBasicIN(x)){
        rtnType = boolT;
    }
    else if(isW(x) || isW(y)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, decideShapeElementwise(x,y));
}

static InfoNode *propOrder(InfoNode *x, InfoNode *y){
    if(isBoolIN(y) && (isListT(x) || isBasicIN(x))){
        return newInfoNode(i64T, newShapeNode(vectorH, SN_ID, -1));
    }
    else return NULL;
}

static InfoNode *propVector(InfoNode *x, InfoNode *y){
    if(isIntIN(x) && isBasicIN(y)){
        return newInfoNode(inType(y), newShapeNode(vectorH, SN_ID, -1));
    }
    else return NULL;
}

static InfoNode *propMatch(InfoNode *x, InfoNode *y){
    // TODO: check wildT?
    return newInfoNode(boolT, newShapeNode(vectorH, SN_CONST, 1));
}

// like(x,y) => size(x)
static InfoNode *propLike(InfoNode *x, InfoNode *y){
    Type rtnType;
    if(isStringIN(x) && isStringIN(y)){
        rtnType = boolT;
    }
    else if(isW(x)||isW(y)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, decideShapeLeft(x,y));
}

static InfoNode *propRandK(InfoNode *x, InfoNode *y){
    Type rtnType;
    if(isIntIN(x) && isIntIN(y)){
        rtnType = i64T;
    }
    else if(isW(x)||isW(y)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, newShapeNode(vectorH, SN_ID, -1));
}

static InfoNode *commonTakeDrop(InfoNode *x, InfoNode *y){
    Type rtnType;
    if(isIntIN(x)){
        rtnType = inType(y);
    }
    else if(isW(x)||isW(y)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, newShapeNode(inShapeType(y), SN_ID, -1)); // shape y
}

// indexof(x,y)
static InfoNode *propIndexof(InfoNode *x, InfoNode *y){
    Type rtnType;
    if(sameT(x,y) && isBasicIN(x) && isBasicIN(y)){
        rtnType = i64T;
    }
    else if(isW(x)||isW(y)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, inShape(y));
}

static InfoNode *propAppend(InfoNode *x, InfoNode *y){
    Type rtnType;
    if(isW(x) || isW(y)){
        rtnType = wildT;
    }
    else if(isBasicIN(x) && isBasicIN(y)){
        rtnType = MAX(inType(x), inType(y));
    }
    else if(sameT(x,y)){
        rtnType = inType(x);
    }
    else return NULL;
    return newInfoNode(rtnType, decideShapeAppend(x,y));
}

// sub_string(x, (1,2):i64)
//    --> can be more precise if literal values are known
static InfoNode *propSubString(InfoNode *x, InfoNode *y){
    Type rtnType;
    if(isStringIN(x) && isIntIN(y)){
        rtnType = inType(x);
    }
    else if(isW(x) || isW(y)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, newShapeNode(vectorH, SN_ID, -1));
}

/* special */

// table x:
//    column "name1" --> table "name2"  (mapping)

typedef struct RelationNode {
    S foreign;  // foreign table
    S primary;  // primary table
    S key;      // relation key (single key)
}RelationNode;

static RelationNode **tableRelation;
static I  relationSize;
static S tableInfo[199];
#define relationSizeMax 10

RelationNode *newRelationNode(S foreign, S primary, S key){
    RelationNode *r = NEW(RelationNode);
    r->foreign = foreign;
    r->primary = primary;
    r->key     = key;
    return r;
}

void addRelation(S f, S p, S k){
    if(relationSize < relationSizeMax)
        tableRelation[relationSize++] = newRelationNode(f, p, k);
    else
        EP("There are more than %d relations (need to increase)", relationSizeMax);
}

static void initTableRelations(){
    RelationNode **tableRelation = NEW2(RelationNode, relationSizeMax);
    addRelation("lineitem", "order", "orderkey");
}

static S findPrimaryTableName(S foreign, S key){
    DOI(relationSize, \
            if(sEQ(foreign, tableRelation[i]->foreign) && 
               sEQ(key, tableRelation[i]->key)) R tableRelation[i]->primary)
    R NULL;
}


static B isString1IN(InfoNode *x){
    R (isStringIN(x) && inShape(x)->kind == constSP && inShape(x)->size == 1);
}

static S getString1IN(InfoNode *x){
    // consts -> vector -> first node -> constant -> string
    R inShape(x)->consts->val.vec.val->val->val.nodeC->valS;
}

static L getTableIdFromTableName(S tableName){
    Q tableSymId = getSymbol(tableName);
    L hashKey = -1*(L)tableSymId;
    // negative keys for avoiding collisions with enum
    L d = lookupSimpleHash(hashMeta, hashKey);
    I tableId = -1;
    if(d){
        tableId = ((MetaData *)d)->meta.tableMeta.tableId;
    }
    else {
        MetaData *newMeta = NEW(MetaData);
        newMeta->meta.tableMeta.tableId = tableId = shapeId++;
        addToSimpleHash(hashMeta, hashKey, (L)newMeta);
    }
    return tableId;
}

static InfoNode *specialLoadTable(InfoNode *x){
    if(isString1IN(x)){
        S tableName  = getString1IN(x);
        L tableId    = getTableIdFromTableName(tableName);
        tableInfo[tableId] = strdup(tableName);
        return newInfoNode(tableT, newShapeNode(tableH, SN_ID, tableId));
    }
    else return NULL;
}

static InfoNode* setEnumKey(InfoNode *x, L key){
    MetaData *newMeta = NEW(MetaData);
    newMeta->meta.enumMeta.keyId = key;
    addToSimpleHash(hashMeta, (L)x, (L)newMeta);
    return x;
}

static InfoNode *specialColumnValue(InfoNode *x, InfoNode *y){
    Type rtnType;
    if(isTableIN(x) && isString1IN(y)){
        rtnType = wildT;
    }
    else if(isW(x) || isW(y)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, newShapeNode(vectorH, SN_ID, inShape(x)->sizeId));
}

// improve it later
// static InfoNode *specialColumnValue(InfoNode *x, InfoNode *y){
//     //P("type: column value\n"); printType(x->type); P(" "); printType(y->type); P("\n"); getchar();
//     Type rtnType;  B isForeignKey = false; L primaryKeyId = -1;
//     if(isTableIN(x) && isString1IN(y)){
//         S   tableName = tableInfo[x->shape->sizeId];
//         S  columnName = getString1IN(y);
//         S primaryName = findPrimaryTableName(tableName, columnName);
//         if(primaryName){
//             isForeignKey = true;
//             primaryKeyId = getTableIdFromTableName(primaryName);
//         }
//         rtnType = wildT;
//     }
//     else if(isW(x) || isW(y)){
//         rtnType = wildT;
//     }
//     else return NULL;
//     InfoNode *rtn = newInfoNode(rtnType, newShapeNode(vectorH, SN_ID, inShape(x)->sizeId));
//     //if(isForeignKey){
//     //    return setEnumKey(rtn, p
//     //}
//     TODO("Add actual return InfoNode");
//     return NULL;
// }

static InfoNode *specialCompress(InfoNode *x, InfoNode *y){
    Type rtnType; //ShapeNode *rtnShape=y->shape;
    if(isBoolIN(x) && (isBasicIN(y)||isEnumT(y))){
        rtnType = inType(y);
    }
    else if(isW(x) || isW(y)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(inType(y), decideShapeCompress(x,y));
}

static InfoNode *specialTable(InfoNode *x, InfoNode *y){
    Type rtnType;
    if(isStringIN(x) && isListT(y)){
        rtnType = tableT;
    }
    else if(isW(x) || isW(y)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, newShapeNode(tableH, SN_ID, -1));
}

static InfoNode *specialKTable(InfoNode *x, InfoNode *y){
    Type rtnType;
    if(isT(x,tableT) && isT(y,tableT)){
        rtnType = ktableT;
    }
    else if(isW(x) || isW(y)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, newShapeNode(tableH, SN_ID, -1));
}

/* 'Type t = -2;' leads to '0>t' returns false - very weird */
static InfoNode *propList(InfoNodeList *in_list){
    /* get length */
    InfoNodeList *temp = in_list;
    Type t = -2; int f = 0, k = 0;
    while(temp->next){
        k++; temp=temp->next;
        if(f==0) { t=inType(temp->in); f = 1; }
        else if(f>0 && t!=inType(temp->in)) { f = -1; }
    }
    ShapeNode *rtnShape = newShapeNode(vectorH, SN_CONST, k);
    return newInfoNodeAll(listT,
                          rtnShape,
                          newInfoNode(f<1?wildT:t, NULL),
                          NULL);
}

// for example, enum<i32>
static B isCellTypeSimple(InfoNode *x){
    if(inCell(x) && !inNext(x)){
        InfoNode *sub = inCell(x);
        return !inCell(sub) && !inNext(sub); // both NULL
    }
    return false;
}

/*
 * function `key`  : returns keyId as its shape id
 * function `fetch`: return the same shape as its enum
 * function `value`: return the same shape as its enum
*/
static I getEnumKeyIdFromMeta(InfoNode *x){
    L d = lookupSimpleHash(hashMeta, (L)x);
    if(d){
        return ((MetaData*)d)->meta.enumMeta.keyId;
    }
    else {
        MetaData *newMeta = NEW(MetaData);
        newMeta->meta.enumMeta.keyId = shapeId++;
        addToSimpleHash(hashMeta, (L)x, (L)newMeta);
        return newMeta->meta.enumMeta.keyId;
    }
}

static InfoNode *propKeyValues(InfoNode *x, bool f){
    if(isDictIN(x)){
        if(f) return newInfoNode(i64T, newShapeNode(vectorH, SN_ID, -1));
        else return newInfoNodeAll(listT,
                                    newShapeNode(listH, SN_ID, -1),
                                    newInfoNode(i64T, NULL), NULL);
    }
    else if(isEnumT(x)){ /* TODO: need to pass info to keys */
        if(f) { // pass key
            I rtnId = getEnumKeyIdFromMeta(x);
            //P("return ID: %d\n", rtnId); getchar();
            if(isCellTypeSimple(x))
                return newInfoNode(inType(inCell(x)),
                                    newShapeNode(vectorH, SN_ID, rtnId));
            else
                return newInfoNode(wildT,
                                    newShapeNode(unknownH, SN_ID, rtnId));
        }
        else { // pass value
            return newInfoNode(i64T, inShape(x));
        }
    }
    else EP("Add support for type %d\n", inType(x));
}

static InfoNode *propValues(InfoNode *x){
    return propKeyValues(x, false);
}

static InfoNode *propFetch(InfoNode *x){
    Type rtnType = isCellTypeSimple(x)?inType(inCell(x)):wildT;
    return newInfoNode(rtnType, inShape(x));
}

// TODO: what if keys(x) twice that returns two different IDs (need to be fixed)
static InfoNode *propKeys(InfoNode *x){
    return propKeyValues(x, true);
}

static InfoNode *propMeta(InfoNode *x){
    return newInfoNode(tableT, newShapeNode(tableH, SN_ID, -1));
}

static InfoNode *propIndex(InfoNode *x, InfoNode *y){
    if(isIntIN(y)){
        if(isListT(x) && isShapeScalar(inShape(y))){
            ShapeNode *subShape = getSubShape(x);
            ShapeNode *rtnShape = NULL;
            if(subShape && isSameShape(inShape(x), subShape)){ // special
                if(subShape->sizeId < 0){
                    rtnShape = newShapeNode(isListS(x)?listH:vectorH, SN_ID, -1);  //S: subtype
                    subShape->sizeId = rtnShape->sizeId;
                }
                else {
                    rtnShape = newShapeNode(isListS(x)?listH:vectorH, SN_ID, subShape->sizeId);
                }
            }
            else { // normal
                rtnShape = newShapeNode(isListS(x)?listH:vectorH, SN_ID, -1);
            }
            return newInfoNode(getSubType(x), rtnShape); // S: subtype
        }
        else return newInfoNode(x->type, y->shape);
    }
    else return NULL;
}

/* copy from typeshape.c, TODO: combine later */
static InfoNode *getNode(InfoNodeList *rt, int k){
    while(rt->next && k>=0){ rt=rt->next; k--; }
    return rt->in;
}

static InfoNode *commonDtChange(InfoNodeList *in_list){
    InfoNode *x = getNode(in_list, 2);
    InfoNode *y = getNode(in_list, 1);
    InfoNode *z = getNode(in_list, 0);
    Type rtnType;
    if(isDtIN(x) && isIntIN(y) && isT(z,symT)){
        rtnType = inType(x);
    }
    else if(isW(x) || isW(y)){
        rtnType = wildT;
    }
    else return NULL;
    return newInfoNode(rtnType, inShape(x));
}

static InfoNode *propEachDya(InfoNodeList *in_list, int side){
    InfoNode *fn = getNode(in_list, 2);
    InfoNode *x  = getNode(in_list, 1);
    InfoNode *y  = getNode(in_list, 0);
    if(isFuncIN(fn)){ /* TODO: add more accurate rules */
        Node *func = fn->funcs->val.listS->val; /* fix later, assume only one func  */
        //printNode(func); getchar(); // func now is a nameK
        if(side == 0){ // each_item
            if(isListT(x) && isListT(y)){
                InfoNodeList *args = NEW(InfoNodeList);
                addToInfoList(args, x->subInfo);
                addToInfoList(args, y->subInfo);
                InfoNodeList *rtns = propagateType(func, args);
                InfoNode *rtn = rtns->next->in;
                /* TODO: clean args (fine), rtns(not allowed) */
                return newInfoNodeAll(listT, x->shape, rtn, NULL);
            }
            else return NULL;
        }
        else if(side == 1){ // each_left
            if(isListT(x)){
                InfoNodeList *args = NEW(InfoNodeList);
                addToInfoList(args, x->subInfo);
                addToInfoList(args, y);
                InfoNodeList *rtns = propagateType(func, args);
                InfoNode *rtn = rtns->next->in;
                return newInfoNodeAll(listT, x->shape, rtn, NULL);
            }
            else return NULL;
        }
        else if(side == 2){ // each_right
            if(isListT(y)){
                InfoNodeList *args = NEW(InfoNodeList);
                addToInfoList(args, x);
                addToInfoList(args, y->subInfo);
                //printInfoNode(args->next->in);
                //printInfoNode(args->next->next->in);
                //printNode(func);
                InfoNodeList *rtns = propagateType(func, args);
                InfoNode *rtn = rtns->next->in;
                //printInfoNode(rtn); stop("check rtn info");
                /* TODO: clean args (fine), rtns(not allowed) */
                return newInfoNodeAll(listT, y->shape, rtn, NULL);
            }
            else return NULL;
        }
        else return NULL;
    }
    else {
        printInfoNode(fn);
        return NULL;
    }
}

static InfoNode *propEachItem(InfoNodeList *in_list){
    return propEachDya(in_list,0);
}
static InfoNode *propEachLeft(InfoNodeList *in_list){
    return propEachDya(in_list,1);
}
static InfoNode *propEachRight(InfoNodeList *in_list){
    return propEachDya(in_list,2);
}

/* TODO: free allocated memory */
static InfoNode *propEach(InfoNode *fn, InfoNode *x){
    if(isFuncIN(fn) && isListT(x)){
        // fetch function
        Node *func = fn->funcs->val.listS->val;
        // fetch args and save to arg list
        InfoNodeList *args = NEW(InfoNodeList);
        addToInfoList(args, x->subInfo);
        // pass func and args to type and shape propagation
        InfoNodeList *rtns = propagateType(func, args);
        // handle return type and shape
        InfoNode *rtn = rtns->next->in;
        // create a new info node and return
        return newInfoNodeAll(listT, x->shape, rtn, NULL);
    }
    else return NULL;
}

static InfoNode *propEnum(InfoNode *x, InfoNode *y){
    Type rtnType;
    InfoNode *subInfo;
    if(sameT(x,y) && isBasicIN(x) && isBasicIN(y)){
        rtnType = enumT;
        subInfo = newInfoNodeAll(inType(x), inShape(y), NULL, NULL);
        // insert for example, enum<i32>
    }
    else if(isW(x) || isW(y)){
        rtnType = wildT;
        subInfo = NULL;
    }
    else return NULL;
    return newInfoNodeAll(rtnType, inShape(y), subInfo, NULL);
}

static InfoNode *propDict(InfoNode *x, InfoNode *y){
    if(isW(x) || isW(y))
        return newInfoNode(wildT, NULL);
    else
        return newInfoNode(dictT, newShapeNode(dictH, SN_ID, -1));
}

/* v[x] = y; */
static InfoNode *propIndexA(InfoNodeList *in_list){
    InfoNode *v = getNode(in_list, 2);
    InfoNode *x = getNode(in_list, 1);
    InfoNode *y = getNode(in_list, 0);
    //P("type: v(%s), x(%s), y(%s)\n",getpTypeName(v->type),getpTypeName(x->type),getpTypeName(y->type));
    //P("shape: %d, %d\n", x->shape->type, y->shape->type); getchar();
    if(sameT(v,y) && isIntIN(x) && sameH(inShape(x),inShape(y))){
        return v;
    }
    else if(isW(v)||isW(x)||isW(y)){
        return v;
    }
    else return NULL;
}

static InfoNode *propJoinIndex(InfoNodeList *in_list){
    InfoNode *fn = getNode(in_list, 2);
    InfoNode *x  = getNode(in_list, 1);
    InfoNode *y  = getNode(in_list, 0);
    Type rtnType;
    if(isT(fn, funcT)){
        if(isW(x)||isW(y)){
            rtnType = wildT;
        }
        else {
            rtnType = listT;
        }
    }
    else return NULL;
    ShapeNode *listShape = newShapeNode(listH, SN_CONST, 2);
    ShapeNode *cellShape = newShapeNode(vectorH, SN_ID, -1);
    // length: 2; with two cells
    InfoNode *rtnInfo = newInfoNodeAll(
            rtnType, listShape, newInfoNode(i64T, cellShape), NULL);
    return rtnInfo;
}

// check function strings and numbers

void checkFuncNumber(){
    if(UnarySize != totalU || BinarySize != totalB || OtherSize != totalO)
        EP("FunctionStr and FunctionType should have the same # of elem.");
}

static int findNameFromSet(char *funcName, const char *set[], int size){
    DOI(size, if(sEQ(funcName, set[i])) return i)
    return -1;
}

static bool searchUnary(char *funcName, FuncUnit *x){
    int k = findNameFromSet(funcName, FunctionUnaryStr, UnarySize);
    if(k>=0){ x->kind = 1; x->u = k; return true; }
    return false;
}

static bool searchBinary(char *funcName, FuncUnit *x){
    int k = findNameFromSet(funcName, FunctionBinaryStr, BinarySize);
    if(k>=0){ x->kind = 2; x->b = k; return true; }
    return false;
}

static bool searchOther(char *funcName, FuncUnit *x){
    int k = findNameFromSet(funcName, FunctionOtherStr, OtherSize);
    if(k>=0){ x->kind = 3; x->t = k; return true; }
    return false;
}

const S obtainTypeUnary(TypeUnary t){
    if(t>=0 && t<UnarySize)
        return strdup(FunctionUnaryStr[t]);
    else
        EP("TypeUnary must be in range [0, %d)", UnarySize);
}

const S obtainTypeBinary(TypeBinary t){
    if(t>=0 && t<BinarySize)
        return strdup(FunctionBinaryStr[t]);
    else
        EP("TypeBinary must be in range [0, %d)", BinarySize);
}

const S obtainTypeOther(TypeOther t){
    if(t>=0 && t<OtherSize)
        return strdup(FunctionOtherStr[t]);
    else
        EP("TypeOther must be in range [0, %d)", OtherSize);
}

void getFuncIndexByName(char *name, FuncUnit *x){
    if(!searchUnary(name,x) && !searchBinary(name,x) && !searchOther(name,x)){
        if(sEQ(name, "le"))
            WP("Do you mean 'leq' instead of 'le'?\n");
        else if(sEQ(name, "ge"))
            WP("Do you mean 'geq' instead of 'ge'?\n");
        else if(sEQ(name, "sub"))
            WP("Do you mean 'minus' instead of 'sub'?\n");
        EP("Function name not found in built-in: %s\n", name);
    }
}

int getValenceOther(TypeOther x){
    switch(x){
        CASE(     eachF, 2);
        CASE( eachItemF, 3);
        CASE( eachLeftF, 3);
        CASE(eachRightF, 3);
        CASE(     enumF, 2);
        CASE(     dictF, 0);
        CASE(    tableF, 2);
        CASE(   ktableF, 0);
        CASE(   indexAF, 3);
        CASE(joinIndexF, 3);
        CASE(     listF, -1); //any
        CASE(    dtaddF, 3);
        CASE(    dtsubF, 3);
        DEFAULT(obtainTypeOther(x));
    }
}

static void *getUnaryRules(TypeUnary x){
    switch(x){ /* monadic */
        CASE(      absF, ruleAbs)
        CASE(      negF, ruleNeg)
        CASE(     ceilF, ruleCeil)
        CASE(    floorF, ruleFloor)
        CASE(    roundF, ruleRound)
        CASE(     conjF, ruleConj)
        CASE(    recipF, ruleRecip)
        CASE(   signumF, ruleSignum)
        CASE(       piF, rulePi)
        CASE(      notF, ruleNot)
        CASE(      logF, ruleLog)
        CASE(     log2F, ruleLog2)
        CASE(    log10F, ruleLog10)
        CASE(      expF, ruleExp)
        CASE(      cosF, ruleCos)
        CASE(      sinF, ruleSin)
        CASE(      tanF, ruleTan)
        CASE(     acosF, ruleAcos)
        CASE(     asinF, ruleAsin)
        CASE(     atanF, ruleAtan)
        CASE(     coshF, ruleCosh)
        CASE(     sinhF, ruleSinh)
        CASE(     tanhF, ruleTanh)
        CASE(    acoshF, ruleAcosh)
        CASE(    asinhF, ruleAsinh)
        CASE(    atanhF, ruleAtanh)
        CASE(     dateF, ruleDate)
        CASE(     yearF, ruleYear)
        CASE(    monthF, ruleMonth)
        CASE(     timeF, ruleTime)
        CASE(     hourF, ruleHour)
        CASE(   minuteF, ruleMinute)
        CASE(   secondF, ruleSecond)
        CASE(     millF, ruleMill)
        CASE(   uniqueF, ruleUnique)
        CASE(      strF, ruleStr)
        CASE(      lenF, ruleLen)
        CASE(    rangeF, ruleRange)
        CASE(     factF, ruleFact)
        CASE(     randF, ruleRand)
        CASE(     seedF, ruleSeed)
        CASE(     flipF, ruleFlip)
        CASE(  reverseF, ruleReverse)
        CASE(    whereF, ruleWhere)
        CASE(    groupF, ruleGroup)
        CASE(      sumF, ruleSum)
        CASE(      avgF, ruleAvg)
        CASE(      minF, ruleMin)
        CASE(      maxF, ruleMax)
        CASE(     razeF, ruleRaze)
        CASE(   tolistF, ruleTolist)
        CASE(     keysF, ruleKeys)
        CASE(   valuesF, ruleValues)
        CASE(     metaF, ruleMeta)
        CASE(loadTableF, ruleLoadTable)
        CASE(    fetchF, ruleFetch)
        CASE(  printF, rulePrint)
        DEFAULT(obtainTypeUnary(x));
    }
}

static void *getBinaryRules(TypeBinary x){
    switch(x){ /* dyadic */
        CASE(      ltF, ruleLt)
        CASE(      gtF, ruleGt)
        CASE(     leqF, ruleLeq)
        CASE(     geqF, ruleGeq)
        CASE(      eqF, ruleEq)
        CASE(     neqF, ruleNeq)
        CASE(    plusF, rulePlus)
        CASE(   minusF, ruleMinus)
        CASE(     mulF, ruleMul)
        CASE(     divF, ruleDiv)
        CASE(   powerF, rulePower)
        CASE(    logbF, ruleLogBase)
        CASE(     modF, ruleMod)
        CASE(     andF, ruleAnd)
        CASE(      orF, ruleOr)
        CASE(    nandF, ruleNand)
        CASE(     norF, ruleNor)
        CASE(     xorF, ruleXor)
        CASE(  appendF, ruleAppend)
        CASE(    likeF, ruleLike)
        CASE(compressF, ruleCompress)
        CASE(   randkF, ruleRandk)
        CASE( indexofF, ruleIndexof)
        CASE(    takeF, ruleTake)
        CASE(    dropF, ruleDrop)
        CASE(   orderF, ruleOrder)
        CASE(  memberF, ruleMember)
        CASE(  vectorF, ruleVector)
        CASE(   matchF, ruleMatch)
        CASE(   indexF, ruleIndex)
        CASE(columnValueF, ruleColumnValue)
        CASE(  subStringF, ruleSubString)
        DEFAULT(obtainTypeBinary(x));
    }
}

static void *getOtherRules(TypeOther x){
    switch(x){ /* special */
        CASE(       eachF, ruleEach)
        CASE(   eachItemF, ruleEachItem)
        CASE(   eachLeftF, ruleEachLeft)
        CASE(  eachRightF, ruleEachRight)
        CASE(       enumF, ruleEnum)
        CASE(       dictF, ruleDict)
        CASE(      tableF, ruleTable)
        CASE(     ktableF, ruleKtable)
        CASE(     indexAF, ruleIndexA)
        CASE(  joinIndexF, ruleJoinIndex)
        CASE(       listF, ruleList)
        CASE(      dtaddF, ruleDtadd)
        CASE(      dtsubF, ruleDtsub)
        DEFAULT(obtainTypeOther(x));
    }
}

int getValence(FuncUnit *x){
    switch(x->kind){
        case 1: return 1;
        case 2: return 2;
        case 3: return getValenceOther(x->t);
    }
    return -1;
}

void *getTypeRules(char *name, int* num){
    FuncUnit x;
    getFuncIndexByName(name, &x);
    switch(x.kind){
        case 1: *num = 1;
                return getUnaryRules(x.u);
        case 2: *num = 2;
                return getBinaryRules(x.b);
        case 3: *num = getValenceOther(x.t);
                return getOtherRules(x.t);
        default: EP("Not supported: %d", x.kind);
    }
}

