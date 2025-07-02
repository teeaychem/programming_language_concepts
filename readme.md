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

# Missing exercises 

- 1.3
- 1.4

- 2.8: Fib seemed enough.

- 3.1: No copy of the book at hand.
- 3.3: No relevant tests to include.
- 3.4: No relevant tests to include.
- 3.8: No relevant tests to include.
- 3.9: Seemed to be additional practice with lexer and parser specifications (and a little too open-ended).

- 4.10
- 4.12
- 4.13

- 5.6: No interest.
- 5.8: Quite open (perhaps todo).
- 5.9: A little too much.

- 6.4: No relevant tests to include (though broadly the comparison and yes-return of f force the type of x).
- 6.6: Not too interesting.
- 6.7

- 7.7: Covered by 8.5.
- 7.8: No particular intereset.
- 7.9: Interesting, but a little too demanding for the payoff.

- 8.2: Too demanding to implement tests..
- 8.4: No relevant tests to include.
- 8.6: Ifs and labels, unless something more ambitious is attempted.

- 9.1
- 9.2

- 10.1: No relevant tests to include.
- 10.6: TODO
- 10.7: TODO

- 11.9: Nop particular interest, as no further use of Icon seems to be made.
- 11.10: Negative intereset.
- 11.11: Project
- 11.12: Project
- 11.13: Somewhat hairy project

- 12.4: No relevant tests (though modifications are present)
- 12.6: No relevant tests
- 12.7: Maybe TODO
