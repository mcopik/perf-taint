// RUN: %clangxx %cxx_flags %s -emit-llvm -o %t1.bc
// RUN: %opt %opt_flags < %t1.bc 2> /dev/null > %t1.tainted.bc
// RUN: %llc %llc_flags < %t1.tainted.bc > %t1.tainted.o
// RUN: %clangxx %link_flags %t1.tainted.o -o %t1.exe
// RUN: %execparams %t1.exe 10 10 > %t1.json
// RUN: diff -w %s.json %t1.json

// RUN: %opt %opt_flags %opt_cfsan < %t1.bc 2> /dev/null > %t2.tainted.bc
// RUN: %llc %llc_flags < %t2.tainted.bc > %t2.tainted.o
// RUN: %clangxx %link_flags %t2.tainted.o -o %t2.exe
// RUN: %execparams %t2.exe 10 10 > %t2.json
// RUN: diff -w %s.json %t2.json

#include <cmath>
#include <cstdlib>

#include "perf-taint/PerfTaint.hpp"

// The simplest possible example.

int f(int x, int y)
{
    int tmp = 0;
    for(int i = x; i < y; ++i)
        tmp += i;
    return tmp*10*x + y/2;
}

int main(int argc, char ** argv)
{
    int x1 EXTRAP = atoi(argv[1]);
    int x2 EXTRAP = 2*atoi(argv[2]);
    register_variable(&x1, VARIABLE_NAME(x1));
    register_variable(&x2, VARIABLE_NAME(x2));

    f(x1, x2);
    f(x2, x1);
    f(0, x1);
    f(0, x2);
    f(0, 10);

    return 0;
}
