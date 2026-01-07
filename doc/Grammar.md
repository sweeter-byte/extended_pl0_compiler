# Extended PL/0 Grammar Specification

This document provides the complete EBNF (Extended Backus-Naur Form) specification for the Extended PL/0 language supported by this compiler.

## Full EBNF Rule Set

```bnf
<prog>      ::= program <id> ";" <block>
<block>     ::= [<condecl>] [<vardecl>] [<proc>] <body>
<condecl>   ::= const <const> {"," <const>} ";"
<const>     ::= <id> ":=" <integer>
<vardecl>   ::= var <vardef> {"," <vardef>} ";"
<vardef>    ::= <id> [":" ("integer" | "pointer") | "[" <integer> "]"]
<proc>      ::= procedure <id> "(" [<id> {"," <id>}] ")" ";" <block> ";" {<proc>}
<body>      ::= begin <statement> {";" <statement>} end

<statement> ::= <id> ["[" <exp> "]"] ":=" <exp>
              | "*" <exp> ":=" <exp>
              | if <lexp> then <statement> [else <statement>]
              | while <lexp> do <statement>
              | for <id> ":=" <exp> ("to" | "downto") <exp> do <statement>
              | call <id> "(" [<exp> {"," <exp>}] ")"
              | <body>
              | read "(" <id> {"," <id>} ")"
              | write "(" <exp> {"," <exp>} ")"
              | new "(" <id> "," <exp> ")"
              | delete "(" <id> ")"

<lexp>      ::= <exp> <lop> <exp> | "odd" <exp>
<exp>       ::= ["+" | "-"] <term> {<aop> <term>}
<term>      ::= <factor> {<mop> <factor>}
<factor>    ::= <id> ["[" <exp> "]"] 
              | <integer> 
              | "(" <exp> ")" 
              | "&" <id> ["[" <exp> "]"] 
              | "*" <factor>

<id>        ::= l {l | d}   (* l: letter, d: digit *)
<integer>   ::= d {d}

<lop>       ::= "=" | "<>" | "<" | "<=" | ">" | ">="
<aop>       ::= "+" | "-"
<mop>       ::= "*" | "/" | "%"
```

## Language Features Details

### 1. Procedures and Scoping
- Procedures can be nested.
- Supports lexical scoping (Static Link).
- Supports recursion.

### 2. Modern Control Flow
- **if-then-else**: Standard conditional execution.
- **while-do**: Pre-condition loop.
- **for-loop**: Supports `to` (increment) and `downto` (decrement).

### 3. Data Structures
- **Arrays**: Single and multi-dimensional arrays are supported.
- **Pointers**: Address-of (`&`) and dereference (`*`) operators are provided.
- **Heap Allocation**: Direct control over heap memory via `new` and `delete`.

### 4. Operators
- **Relational**: `=`, `<>`, `<`, `<=`, `>`, `>=`
- **Arithmetic**: `+`, `-`, `*`, `/`, `%` (modulo)
- **Unary**: `+`, `-`, `odd`
