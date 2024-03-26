# compiler-project

LLVM/Clang Version: 14.0.0\
Operating System: Linux

## Problem Statement

Design a compiler pass (LLVM) to take a C++ application as input, generate the program dependence graph (basic block level) for a given program, and partition the program into secure or not secure partitions. Also, print the ID of the desired basic block. You can use numbers to represent the basic block ID.

## How to run

1. Create ll file:

```bash
clang++ -S -emit-llvm tests/sample.cpp -o sample.ll
```

2. Compile customPass:

```bash
clang++ -shared -o ./pdg/custom-module-pass.so ./pdg/custom-module-pass.cpp `llvm-config --cxxflags --ldflags --libs` -fPIC
```

3. Optimize using customPass:

llvm v14.0.0

```bash
opt --enable-new-pm=0 -load ./pdg/custom-module-pass.so -custom-module-pass < ./tests/sample.ll > /dev/null
```

llvm v17.0.6

```bash
opt --bugpoint-enable-legacy-pm -load ./pdg/custom-module-pass.so -custom-module-pass < ./tests/sample.ll > /dev/null
```

## Contributors

Aditi Agarwal\
Aman Yadav\
Anant Sharma

## References

Shen Liu, Gang Tan, and Trent Jaeger. 2017. PtrSplit: Supporting General Pointers in Automatic Program Partitioning. In Proceedings of the 2017 ACM SIGSAC Conference on Computer and Communications Security (CCS '17). Association for Computing Machinery, New York, NY, USA, 2359–2371. https://doi.org/10.1145/3133956.3134066
