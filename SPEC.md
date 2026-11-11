# Vela Language Specification (working draft)

This document states intended behavior. Where implementation and SPEC disagree, that is a bug (in either the SPEC or the code). Tests under `tests/` encode executable checks against this document.

## 1. Source and evaluation

- Programs are UTF-8 text evaluated top to bottom.
- Evaluation is eager.
- Types are dynamic; optional annotations are checked at runtime when present.

## 2. Values

Nil, number, string, bool, array, map, set, range, function, native, record instance, enum variant.

- `nil` and `false` are falsey; all other values are truthy in boolean contexts (including `0` and `""` — **numbers: zero is falsey** if the implementation treats it as such; see tests).
- Array and map equality is deep (structural).

## 3. Bindings

| Form | Meaning |
|------|---------|
| `x := e` | Bind `x` immutably in the current scope. Error if `x` already bound here. |
| `x <- e` | Assign to mutable `x`, or create mutable `x` if unbound. Error if `x` is immutable. |

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
- `repeat N times { ... }` — N iterations, no bind variable

## 5b. Readable aliases (identical semantics)

| Word form | Symbol form |
|-----------|-------------|
| `and` / `or` / `not` | `&&` / `\|\|` / `!` |
| `stop` / `next` | `bail` / `skip` |
| `ask` / `say` | `readline` / `print` |

## 5c. `is` and `in`

- `e is Type` — type or enum/record tag checks
- `e is empty` / `e is not empty` — nil, empty string, empty array, empty map/set
- `e is nil` — nil check
- `x in collection` — membership (array, string substring, map key, set)

## 6. Errors

- `throw e` raises; `try` / `catch` handle.
- `try_num` returns `nil` on failure; `num` throws.
- Division or modulo by zero throws.

## 7. Modules

- `import "path"` / `import "path" as name`
- Paths resolve relative to the importing file’s directory; modules are cached.

## 8. Quality process

- Smoke: all files under `examples/` (includes `45_full_language_audit.vel`)
- Optional: `tests/run_all.sh` when present
- See `ROADMAP.md` for known gaps

Correctness claims should cover bindings, control, data, functions, errors, and modules — not only loops or a single demo.
