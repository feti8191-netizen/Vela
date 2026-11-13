# Vela roadmap — honest status

Vela is an **interpreted** language with a C++17 tree-walking implementation, REPL, examples, and a growing standard library.

## Working today

- Distinct syntax and full interpreter (`./vela`, `./vela file.vel`)
- Bindings (`:=` / `<-`), control flow, functions, data structures
- Match, records, enums, modules, readable aliases (`and`/`or`/`stop`/`next`/…)
- File and process builtins, many collection helpers
- Examples including `45_full_language_audit.vel`

## Explicitly out of scope (for now)

| Item | Status |
|------|--------|
| `vela build` / native compile step | **Removed** — language is interpreted by design |
| GPU programming | Out of scope |
| Package manager | Open |
| Full formal SPEC for every edge | Draft (`SPEC.md`) |
| Debugger / profiler | Open |
| Heavy concurrency (async/channels) | Open |

## Near-term priority

1. Keep all `examples/` green on the interpreter  
2. Clearer errors across more constructs  
3. Grow `std/` modules  
4. Widen automated tests when present  
5. SPEC accuracy vs implementation  

## Design boundary

Vela runs by interpreting source. OS access is through **explicit builtins** (files, env, processes). That is intentional for a readable scripting-oriented language.
