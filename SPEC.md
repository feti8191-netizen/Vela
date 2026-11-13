# Vela Language Specification (working draft)

Vela is an **interpreted** language. This document states intended runtime behavior.  
Where implementation and this SPEC disagree, that is a bug in one of them.

## 1. Source and evaluation

- Programs are UTF-8 text evaluated top to bottom.
- Evaluation is eager.
- Types are dynamic; optional annotations are checked at runtime when present.

## 2. Values

Nil, number, string, bool, array, map, set, range, function, native, record instance, enum variant.

## 3. Bindings

| Form | Meaning |
|------|---------|
| `x := e` | Bind `x` immutably. Error if `x` already bound in this scope. |
| `x <- e` | Assign to mutable `x`, or create mutable `x`. Error if `x` is immutable. |

## 4. Functions

- `name(params) ==> body` declares a function.
- `-> e` returns from the innermost function.
- Defaults: trailing `param := expr`, evaluated at call time when omitted.
- Pipe: `x |> f` means `f(x)`; `x |> f(y)` means `f(x, y)`.

## 5. Control

- `when` / `else when` / `else` — condition required before `{`
- `unless`, `until`, `loop`
- Loop exit: `bail` or `stop`; continue: `skip` or `next` (error outside a loop)
- `repeat i from A to B [by S]`, `through` for inclusive end
- `repeat x in collection`
- `repeat N times { ... }`

## 5b. Readable aliases

| Word form | Symbol form |
|-----------|-------------|
| `and` / `or` / `not` | `&&` / `\|\|` / `!` |
| `stop` / `next` | `bail` / `skip` |
| `ask` / `say` | `readline` / `print` |

## 5c. `is` and `in`

- `e is Type` — type or tag checks  
- `e is empty` / `e is not empty`  
- `x in collection` — membership  

## 6. Errors

- `throw e` raises; `try` / `catch` handle.
- `try_num` returns `nil` on failure; `num` throws.
- Division or modulo by zero throws.

## 7. Modules

- `import "path"` / `import "path" as name` / selective import when supported  
- Paths resolve relative to the importing file; modules are cached.

## 8. Quality

- Smoke tests: programs under `examples/`
- Interpreter entry: `./vela` and `./vela file.vel`
