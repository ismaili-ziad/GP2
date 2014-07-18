================================
Author: Chris Bak (February 21 2014)
GP2: Graph Programming
================================

GP2 is a graph programming language. The programmer creates graph programs in a graphical editor, which are executed by an underlying machine. The editor generates two text files: one representing the program and one representing the input graph. The program is compiled to native code (one day) and the input graph is converted to a machine readable format.

Currently there is a lexer, a parser, a semantic analyser and a pretty printer. The first two components are generated by Flex and Bison respectively. The semantic analyser is written in C. The pretty printing module is used for debugging purposes. It outputs abstract syntax trees in the DOT language and prints the symbol table.

Build Instructions
---------------------
To make the executable, run:

> make gpparse

To make the executable and automatically run it on input files, run:

> make gpparse path/to/gp_program_file path/to/gp_graph_file

This will build gpparse and execute it on the inpit file. Currently it assumes extensionless file names.

The program has functionality to print two abstract syntax trees for debugging: once before semantic analysis and once after semantic analysis. This is because the semantic analysis phase manipulates the AST while performing static semantic checking.
