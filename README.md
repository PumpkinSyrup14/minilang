# MiniLang — Compiler Front End (Phase 1 + Phase 2)

Compiler Design Laboratory. Implements the pipeline through
**intermediate code generation**:

```
Source (.ml) -> Lexer (Flex) -> Parser (Bison, builds AST)
             -> Symbol Table (scoped) -> Semantic Analyzer (type + scope checks)
             -> TAC Generator (three-address code)
```

## Build

Requires `flex`, `bison`, `gcc`, `make`.

```bash
make            # builds the ./minilang binary
```

## Run

```bash
./minilang tests/valid1.ml            # run the full pipeline, prints generated TAC
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
  tac.h/.c      Phase 2: three-address code generator (AST -> quads -> printed TAC)
  main.c        Driver: runs lexer -> parser -> semantic analyzer -> TAC generator
tests/
  valid1.ml, valid2.ml        valid programs (vars, control flow, functions, promotion)
  tac_if_else.ml, tac_while.ml  isolated demos of if/else and while TAC translation
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

Not yet implemented (planned for Phase 3): string type, optimization passes,
and the interpreter/execution engine that actually runs the generated TAC.

## What each stage actually does

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
- **TAC generation** (Phase 2): walks the type-checked AST and emits
  three-address code as a flat list of quads `(op, arg1, arg2, result)`.
  One dedicated translation rule per AST node kind:
  - `BinOp`/`UnOp` -> new temporary, e.g. `t1 = a + b`
  - `Assign`/`VarDecl` (with initializer) -> `name = value`
  - `If`/`While` -> labels + `ifFalse ... goto` / `goto` jumps
  - `Call` -> `param` per argument, then `t = call f, n`
  - `Return`/`Print` -> `return v` / `print v`
  Functions become labeled blocks (`funcname:`); global variable
  initializers are emitted as straight-line code in source order.

## Known limitations (be ready to mention these in the viva)

- No string type yet.
- Panic-mode recovery can swallow a following statement when it resyncs at
  the next `;` — standard trade-off of simple panic-mode recovery.
- TAC generation only runs on semantically valid programs (errors stop the
  pipeline before codegen) — intentional, matches standard compiler behavior.
- No optimization pass over the generated TAC yet (e.g. constant folding,
  dead code elimination) — natural next step, not required for Phase 2.
- No execution/interpretation of the TAC yet — that's Phase 3 scope.
