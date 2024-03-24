# compiler-project

LLVM/Clang Version: 14.0.0\
Operating System: Linux

## Problem Statement

Design a compiler pass (LLVM) to take a C++ application as input, generate the program dependence graph (basic block level) for a given program, and partition the program into secure or not secure partitions. Also, print the ID of the desired basic block. You can use numbers to represent the basic block ID.

## How to run

1. Create ll file:

```
clang++ -S -emit-llvm tests/sample.cpp -o sample.ll
```

2. Compile customPass:

```
clang++ -shared -o sample-pass.so sample-pass.cpp `llvm-config --cxxflags --ldflags --libs` -fPIC

```

3. Optimize using customPass:

```
opt --enable-new-pm=0 -load ./sample-pass.so -sample-pass < sample.ll > /dev/null
```

## Contributors

Aditi Agarwal\
Aman Yadav\
Anant Sharma

## References

Shen Liu, Gang Tan, and Trent Jaeger. 2017. PtrSplit: Supporting General Pointers in Automatic Program Partitioning. In Proceedings of the 2017 ACM SIGSAC Conference on Computer and Communications Security (CCS '17). Association for Computing Machinery, New York, NY, USA, 2359–2371. https://doi.org/10.1145/3133956.3134066
