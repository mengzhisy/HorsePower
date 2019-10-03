#!/bin/bash

usage() {
    printf '%s\n' \
        "Usage: $0 <cmd>" "" \
        " 1) $0 front               ## build flex & bison" "" \
        " 2.1) $0 interp  t/f <id>  ## test with benchmarks in tests/(t/f)/t<id>.hir" "" \
        " 2.2) $0 interp  q   <id>  ## test with TPC-H queries in <path>.hir" "" \
        " 3.1) $0 compile t/f <id>  ## test with benchmarks in tests/(t/f)/t<id>.hir" "" \
        " 3.2) $0 compile q   <id>  ## test with TPC-H queries in <path>.hir" "" \
        " 4.1) $0 opt     t/f <id>  ## compiler with optimizer" "" \
        " 4.2) $0 opt     q   <id>  ## compiler with optimizer" "" \
        " 5) $0 stats  <load/dump>  ## load/dump stats" ""

    echo "Example: run=1 sf=1 thread=1 ./run.sh interp q 6"
    echo "         opt=fa ./run.sh opt q 6   # automatic fusion for q6"
    exit 1
}

runInterpreter() {
    cmd=$1; tid=$2; sf=$3; run=$4; th=$5
    #echo $0  # --> ./run.sh
    if [ $cmd = "t" ]; then
        (set -x && ./horse -t -f ./tests/t/t${tid}.hir)
    elif [ $cmd = "f" ]; then
        (set -x && ./horse -t -f ./tests/f/t${tid}.hir)
    elif [ $cmd = "q" ]; then
        echo "Running TPC-H Query: q${tid}, sf$sf, run$run, thread$th"
        (set -x && ./horse -t -f ./scripts/q${tid}.hir --tpch=${tid})
    else
        usage
    fi
}

runCompiler() {
    cmd=$1; tid=$2
    if [ $cmd = "t" ]; then
        (set -x && ./horse -c cpu -f ./tests/t/t${tid}.hir)
    elif [ $cmd = "f" ]; then
        (set -x && ./horse -c cpu -f ./tests/f/t${tid}.hir)
    elif [ $cmd = "q" ]; then
        echo "Running TPC-H Query: q${tid}, sf$sf, run$run, thread$th"
        (set -x && ./horse -c cpu -f ./scripts/q${tid}.hir --tpch=${tid})
    else
        usage
    fi
}

runOptimizer() {
    cmd=$1; tid=$2; opt=$3
    opts="-o $opt"
    if [ $cmd = "t" ]; then
        (set -x && ./horse -c cpu ${opts} -f ./tests/t/t${tid}.hir)
    elif [ $cmd = "f" ]; then
        (set -x && ./horse -c cpu ${opts} -f ./tests/f/t${tid}.hir)
    elif [ $cmd = "q" ]; then
        echo "Running TPC-H Query: q${tid}, sf$sf, run$run, thread$th"
        (set -x && ./horse -c cpu ${opts} -f ./scripts/q${tid}.hir --tpch=${tid})
    else
        usage
    fi
}

runStats() {
    cmd=$1
    if [ $cmd = "dump" ]; then
        (set -x && ./horse -u --stats=dump)
    elif [ $cmd = "load" ]; then
        (set -x && ./horse -u --stats=load)
    else
        usage
    fi
}

if [ -z $sf ]; then
    sf=1
fi

if [ -z $run ]; then
    run=1
fi

if [ -z $thread ]; then
    thread=1
fi

if [ -z $opt ]; then
    opt=all
fi

export OMP_PLACES=cores
export OMP_PROC_BIND=spread
export OMP_NUM_THREADS=$thread

## Entry: main
if [ $# -eq 1 ]; then
    cmd=$1
    if [ $cmd = "front" ]; then
        (set -x && make test)
    else
        usage
    fi
elif [ $# -eq 2 ]; then
    mod=$1
    if [ $mod = "stats" ]; then
        runStats $2
    else
        usage
    fi
elif [ $# -eq 3 ]; then
    mod=$1
    if [ $mod = "interp" ]; then
        runInterpreter $2 $3 $sf $run $thread
    elif [ $mod = "compile" ]; then
        runCompiler $2 $3
    elif [ $mod = "opt" ]; then
        runOptimizer $2 $3 $opt
    else
        usage
    fi
else
    usage
fi

