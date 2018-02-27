# HorsePower

HorsePower is designed for optimizing database queries with modern hardware.
At its core is **HorseIR**, which is a well-designed array-based intermediate
representation (IR) for database queries.  Based on HorseIR, sophisticated
compiler optimizations can be applied for database operations.  Moreover, using
array programming offers a promising option for performance speedup with
fine-grained parallelism.


## Quick entries

- Design:
    [Grammar](src/HorseIR/frontend/grammar/HorseIR.g4)
    | [IR Design](docs/mkdocs/docs/horseir/)
    | [Valid examples](src/HorseIR/tests/valid)
    | [TPC-H](docs/tpch)
    | [Reference](docs/mkdocs/docs)
    | [IR Type](docs/horsetype)
    |
- Implementation: 
    [Details](docs/implementation)
    | [Project libraries](libs/)
    |
- Paper:
    [Related](docs/study)
    |


## Project Overview

In the summer 2017, we started this project from scratch.  We planned to build
the framework in the first three months (12 weeks). Thus, we could have another
month to improve it and draft several technical reports.

Figure 1 describes the workflow of the Horse framework.  A candidate for the
source language is our *Horse* language which is an extension of standard SQL.
The Horse language is designed for data analytics with extended SQL features
and array programming.  Also, we provide a front end for parsing and
transforming source code to HorseIR.  After the optimization phases, several
back ends are supported, i.e. LLVM, C/C++ and interpreter.  A couple of
optimizations take place before bitcode is generated.  On the other hand, the
interpreter is able to output result on the fly, so that it could save the
compilation time.  That means it may be faster than compiled code.

<p align="center"><img src="docs/figures/horse-flow.png" /></p>
<p align="center">Figure 1. The workflow of the Horse framework.</p>

<!--
Figure 2 introduces the design of three levels of IRs.  The workflow dependence
of IRs consists of an acyclic graph.  The workflow is described as follows.

1. Source code is converted to medium-level IR (MIR, 3-address code);
2. MIR has two options, either to Low-level IR (LIR) or to High-level IR (HIR);
3. HIR is designed for optimizations with high-level perspectives;
4. LIR is close to target code which usually is relatively low-level;
5. In each level, IR code is optimized with different optimizations.

<p align="center"><img src="docs/figures/horse-ir.png" /></p>
<p align="center">Figure 2. The design of 3 IRs.</p>

-->

In HorsePower, we focus on the following parts.

    - High-level source language design
    - Fine-grained primitives and highly tuned library
    - Static analysis for an array-based IR (i.e. HorseIR)
    - Query optimizations with heuristics
    - Query optimizations with data-flow analyses

### Settings

- Platform       : Cross-platform
- Languages      : C/C++
- Auto tool      : ANTLR4
- Parallelism    : OpenMP/Pthread/CUDA/OpenCL
- Conventions    : [docs/conventions](docs/conventions)
- GitHub Issue   : [Issues](https://github.com/Sable/HorsePower/issues)

### Deployment

Requirements (suggest)

- gcc 5.4 or higher
- uuid-dev library

#### Step 1. Download libraries

    (cd libs && sh deploy_linux.sh)

Installation time (On Sable-Intel)  **about 13 mins**

    > time (cd libs && sh deploy_linux.sh)
    
    real     13m0.499s
    user     11m8.760s
    sys      1m13.444s

New folders created

    libs/include
    libs/lib
    libs/antlr4
    libs/pcre2

#### Step 2. Build front-end

    (cd src/HorseIR/frontend && mkdir -p build && cd build && cmake ../ && make lib -C ../../backend && make)

#### Step 3. Build back-end

    (cd src/HorseIR && make && ./horse)

### External links

Software

- MonetDB : [home](https://www.monetdb.org/Home) | [reference](https://www.monetdb.org/Documentation/SQLreference)
- Q/KDB+  : [kx](kx.com) | [reference](code.kx.com)

About Horse

- [HorseIR design](http://www.sable.mcgill.ca/~hanfeng.c/f17/horse_spec/) | Horse: An array programming language with Database components
- [Horse program](http://www.sable.mcgill.ca/~hanfeng.c/f17/horseir/) | Design of Horse program

Related work

- [Introduction](http://www.sable.mcgill.ca/~hanfeng.c/f17/talk2/slide.html) : A query compiler for IMDB
- [Query compiler](http://www.sable.mcgill.ca/~hanfeng.c/f17/qcompiler/) : Shaikhha et al., How to Architect a Query Compiler, SIGMOD 2016
- LegoBase : Klonatos et al., [Building Efficient Query Engines in a High-Level Language](http://dl.acm.org/citation.cfm?id=2732959), VLDB 2014
- Aquery   : Cambronero and Shasha, AQuery, a query language for order in data analytics: Language, Optimization, and Experiments, to appear

SQL front-end

- [H2 database](http://www.h2database.com/html/grammar.html) | H2: an in-memory db written in Java
- [hyrise/sql-parser](https://github.com/hyrise/sql-parser)

### Chat room

[Enter here](https://gitter.im/Sable/HorsePower)

Let's rock the summer 2017!
