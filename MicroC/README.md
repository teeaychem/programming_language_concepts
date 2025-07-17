# MicroC

A project MicroC compiler, written in C++.

# Setup

## Building

``` shell
conan install . --build=missing
cmake --preset conan-release
cmake --build --preset conan-release
```

## Requirements

- Bison 3.8.2
- Flex 2.6.4
- LLVM 20

## Build

``` shell
cmake --preset default
cmake --build --preset default
```

# Notes

## AST Design

Roughy follows Lai's [outline](https://lesleylai.info/en/ast-in-cpp-part-1-variant/) of virtual enums.

Forward declarations in general `AST` header file.
Details and `pk` builder functions in node specific header files.

Not particularly ergonomic, and should implement a primary node struct.

## AST / Parsing

Mostly faithful to the F#.
With some exceptions.

### Types

Significant difference is handling of types.
Split into data types and non-data types.

Each type has a unique data type.
Data types are int, char, and void.
If the data type is not yet known during parsing the defualt data type void is used.
A type with void data type is considered incomplete, and any incomplete data type may be completed.
Completion is recursively applied until the base data type is found.


# LLVM 

``` shell
clang -S -O0 -emit-llvm -Xclang -disable-llvm-passes -fno-discard-value-names <c_file>
```

``` shell
lldb ./build/Release/mc -- ex/ex.c
process launch
bt
```

# Resources

- Sestoft (2017) [Programming Language Concepts](studwww.itu.dk/~sestoft/plc/): MicroC spec, etc.
- [Bison docs](https://www.gnu.org/software/bison/manual/bison.html): Initial flex and bison configuration.
- [Flex docs](https://westes.github.io/flex/manual/Indices.html#Indices): Hints on flex.
- Lai (2024) [Representing an Abstract Syntax Tree in C++](https://lesleylai.info/en/ast-in-cpp-part-1-variant/): AST design.
- [LLVM Fibonacci](https://github.com/llvm/llvm-project/tree/main/llvm/examples/Fibonacci)
