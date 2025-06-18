# Programming Language Concepts

A work through the book Programming Language Concepts by Peter Sestoft.

The website for the book is located at: [https://studwww.itu.dk/~sestoft/plc/](https://studwww.itu.dk/~sestoft/plc/).

## Example code and exercises

Some example code from the website is included, when used for exercises, etc.

Included example code is likely modified to some degree, though the original header is always preserved.
By this I mean any example code will retain leading comments.
For example, `Intro/Intro1.fs` retains:

``` f#
(* Programming language concepts for software developers, 2010-08-28 *)
```

Though, most examples from the file have been recast as tests.

## Tests

Some example code is recast as tests, and I expect at least minimal tests to accompany the exercises.

Tests can be run via:

``` shell
cd ./ProgrammingLanguageConcepts.Tests/
dotnet test
```

## The stack machine

Intcomp contains a version of the stack machine written in C.
For slight confusion, shadow the unix program `machine`.

``` shell
cd ./Intcomp
clang -o machine machine.c
```

The machine reads a file, or runs a couple of internal tests.

``` shell
./machine <FILE>
./machine -t
```
