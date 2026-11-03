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

- `when` / `else when` / `else`
- `unless`, `until`, `loop`, `bail`, `skip`
- `repeat i from A to B [by S]`, `through` for inclusive end
- `repeat x in collection`

## 6. Errors

- `throw e` raises; `try` / `catch` handle.
- `try_num` returns `nil` on failure; `num` throws.
- Division or modulo by zero throws.

## 7. Modules

- `import "path"` / `import "path" as name`
- Paths resolve relative to the importing file’s directory; modules are cached.

## 8. Quality process

See `tests/run_all.sh` and `ROADMAP.md`. Correctness claims require the full process, not only `examples/`.
