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

## Revisions

### AST

At a fairly high level of abstraction the AST from the book was set up to ease code generation by encoding detailed information about the generation requred in a node.
Perhaps the clearest example of this is with access nodes, which specify how a variable is to be handled.

The AST from the book works well within the scope of the book, and is quite elegant, though this led to various issues, especially with respect to code generation and expanding valid source.

So, the AST diverges from the book, primarily by simplifying nodes and moving work done by the parser into code generation (or other steps).

Some additional notes follow.

#### Access nodes

Assignment nodes have been removed from the AST and expressions have been expanded.

Of note, both `*` and `&` are implemented as unary operators (when not used in type specification).

#### Print functions

`printi` and `println` are parsed as in the book, though evaluate to function calls.
As a result, `printi n` is equivalent to `printi(n)` and `println` is equivalent to `println()` in source.

#### Scope and type resolution

Scope is tracked during parsing, along with the type of variables and the return type of functions.

Within a block a distinction is made between fresh declarations, shadow declarations, and other statements.
Declarations are always generated before any other statements.


# LLVM 

``` shell
clang -S -O0 -emit-llvm -Xclang -disable-llvm-passes -fno-discard-value-names <file.c>
```

``` shell
lldb ./build/Release/mc -- <file.c>
process launch
bt
```

# Resources

- Sestoft (2017) [Programming Language Concepts](studwww.itu.dk/~sestoft/plc/): MicroC spec, etc.
- [Bison docs](https://www.gnu.org/software/bison/manual/bison.html): Initial flex and bison configuration.
- [Flex docs](https://westes.github.io/flex/manual/Indices.html#Indices): Hints on flex.
- Lai (2024) [Representing an Abstract Syntax Tree in C++](https://lesleylai.info/en/ast-in-cpp-part-1-variant/): AST design.
- [LLVM Fibonacci](https://github.com/llvm/llvm-project/tree/main/llvm/examples/Fibonacci)
