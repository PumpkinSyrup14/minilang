# MiniLang — Compiler Front End (Phase 1 Prototype)

Compiler Design Laboratory — Phase 1 initial prototype.
Implements the pipeline through **semantic analysis**, as required:

```
Source (.ml) -> Lexer (Flex) -> Parser (Bison, builds AST)
             -> Symbol Table (scoped) -> Semantic Analyzer (type + scope checks)
```

## Build

Requires `flex`, `bison`, `gcc`, `make`.

```bash
make            # builds the ./minilang binary
```

## Run

```bash
./minilang tests/valid1.ml            # run the pipeline on a source file
./minilang --ast tests/valid1.ml      # also print the AST (before and after
                                       # semantic analysis, so you can see
                                       # types get filled in by that pass)
make test                              # run every file in tests/ and print output
```

## Project layout

```
src/
  lexer.l       Flex lexer: tokens, keywords, operators, lexical error detection
  parser.y      Bison grammar: builds the AST, panic-mode error recovery
  ast.h/.c      AST node types, constructors, printer
  symtab.h/.c   Scoped symbol table (stack of scopes, insert/lookup)
  semantic.c    Semantic analysis pass: scope + type checking over the AST
  main.c        Driver: runs lexer -> parser -> semantic analyzer, reports counts
tests/
  valid1.ml, valid2.ml        valid programs (vars, control flow, functions, promotion)
  err_lexical.ml              malformed number literal + invalid character
  err_syntax.ml               missing semicolons -> panic-mode recovery demo
  err_undeclared.ml           undeclared variable / undeclared function call
  err_typemismatch.ml         type mismatches in decl/condition/call/return
  err_duplicate.ml            duplicate variable, function, and parameter names
```

## MiniLang language (subset implemented so far)

- Types: `int`, `float`, `bool`, `void` (function return type only)
- Declarations: `type name;` / `type name = expr;`
- Functions: `type name(type p1, type p2, ...) { ... }`
- Statements: assignment, `if`/`else`, `while`, `return`, `print(expr)`, blocks
- Expressions: arithmetic `+ - * / %`, relational `< > <= >=`, equality `== !=`,
  logical `&& || !`, function calls, parentheses
- Implicit `int -> float` promotion on assignment/initialization/return/arguments

Not yet implemented (planned for Phase 2/3): string type, intermediate code
generation (three-address code), optimization, and the interpreter/execution engine.

## What each phase actually checks

- **Lexical analysis**: tokenizes source; reports invalid characters and
  malformed numeric literals as lexical errors, then keeps scanning.
- **Syntax analysis**: recursive grammar built with Bison; on a syntax error,
  a panic-mode `error ';'` rule discards tokens up to the next `;` and resumes,
  so multiple syntax errors in one file are all reported instead of stopping
  at the first one.
- **Semantic analysis**: walks the AST using a scoped symbol table to catch:
  undeclared variables/functions, redeclaration in the same scope, duplicate
  parameter names, type mismatches in declarations/assignments/conditions/
  returns/call arguments, wrong argument counts, and `void` used as a variable
  type.

## Known limitations (be ready to mention these in the viva)

- No string type yet.
- Panic-mode recovery can swallow a following statement when it resyncs at
  the next `;` — standard trade-off of simple panic-mode recovery.
- No intermediate code / execution yet — that's Phase 2/3 scope.
