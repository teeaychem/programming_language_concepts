# microC

A project MicroC to LLVM IR compiler with a (limited) JIT interpreter.

The project is 'lightly' polished.
That is, effort has been made to keep the code clean, annotated with comments, and with sensible architecture.
However, as learning about codegen was a goal of the project the code tend towards functional rather than ideal.

microC source is parsed to an AST with bison and flex, and the AST includes methods for LLVM IR codegen.

The JIT interpreter is built as `microCJIT` and supports passing a single argument to main, along with printouts related to parsing, the AST, and generated LLVM IR.

For example, to compile `src.c`, print the generated IR, and run `main` in `src` with an argument of `23`:

``` shell
microCJIT -m src.c 23
```

See `bin/microCJIT.cpp` for details on `microCJIT` and `src/` for details on the AST, codegen, and parsing.



# Setup

## Building microCJIT

CMake is used, and it is assumed the requirements (below) can be found using CMake's `find_package`.
Tests assume `microCJIT` is found in a build folder.
So, standard build commands are:

``` shell
cmake -S . -B ./build
cmake --build ./build
```

## Requirements

- Bison 3.8.2
- Flex 2.6.4
- LLVM 20



# Notes

## AST Design

Roughy follows Lai's [outline](https://lesleylai.info/en/ast-in-cpp-part-1-variant/) of virtual enums.

Forward declarations in general `AST` header file.
Details and `pk` builder functions in node specific header files.

Not particularly ergonomic, and should implement a primary node struct.


## AST / Parsing

Mostly faithful to the F#.
With some exceptions.


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

For codegen, whether or not to access the value of an expression is determined by the `Value` of an expression.
Values may be `L` or `R`, and follow the C/++ lvalue / rvalue distinction, with `L`roughly corresponding to the value of the left side of an assignment op, and `R` corresponding to the value on the right side.


#### Declaration order

The book allows out of order top level declarations (or perhaps only supports reverse order declarations).
In contrast, microC follows C and requires the prototype of a function to be declared before call.

For the moment protoype declarations are tied to function declarations, though this is primarily a limitation of parsing.
In particular, the AST has distinct nodes for prototypes and body declarations.


#### Casts

In order to smooth out some codegen basic support for casting is implemented.

For example, LLVM IR distinguishes between pointers and ints, while the book does not.
So, calls to print a pointer must either use a distinct fn or cast the pointer to an int.

Casts are internal to the AST and inferred.
Though, in principle the parser could be extended to support explicit casts.


#### Variadic main

JIT uses MCJIT which does not support main with variable arguments.
So, execution of microC code via microCJIT is limited to a single argument.

Note, this limitation is strictly external to codegen, and would not be present with a different JIT engine.
Still, for the moment interest is with first-pass codegen, rather than passes, so the JIT engine setup is kept as simple as known.


#### Print functions

`printi` and `println` are parsed as in the book, though evaluate to function calls.
As a result, `printi n` is equivalent to `printi(n)` and `println` is equivalent to `println()` in source.


#### Scope and type resolution

Scope is tracked during parsing, along with the type of variables and the return type of functions.

Within a block a distinction is made between fresh declarations, shadow declarations, and other statements.
Declarations are always generated before any other statements.


#### Types

##### Booleans

LLVM IR returns a boolean (i1) from comparison ops, etc.
Booleans are preserved in the AST, and cast as required.
However, parsing of booleans is not supported.

##### Completion

If the data type is not yet known during parsing the defualt data type void is used.
A type with void data type is considered incomplete, and any incomplete data type may be completed.
Completion is recursively applied until the base data type is found.


## LLVM debugging

Of significant help is contrasing clang IR to microC IR.

On macOS the following can be used to generate LLVM IR:

``` shell
clang -S -O0 -emit-llvm -Xclang -disable-llvm-passes -fno-discard-value-names <src.c>
```

And, on a related point, working with LLVM leads to working with (many) pointers.
Using `lldb` (or related) for backtraces is often very helpful.



# Resources

- Sestoft (2017) [Programming Language Concepts](studwww.itu.dk/~sestoft/plc/): MicroC spec, etc.
- [Bison docs](https://www.gnu.org/software/bison/manual/bison.html): Initial flex and bison configuration.
- [Flex docs](https://westes.github.io/flex/manual/Indices.html#Indices): Hints on flex.
- Lai (2024) [Representing an Abstract Syntax Tree in C++](https://lesleylai.info/en/ast-in-cpp-part-1-variant/): AST design.
- [LLVM Fibonacci](https://github.com/llvm/llvm-project/tree/main/llvm/examples/Fibonacci)
