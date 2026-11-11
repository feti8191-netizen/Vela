## Completed in language-focus pass

- Enums with associated data + match unpack
- Record methods (`self`)
- Tuple literals `(a, b)`
- String helpers: to_upper/lower, index_of, pad_left/right
- Collection helpers already present: flatten, unique, group_by, partition, chunks, windows
- Examples 27–30

## Completed in readability / consistency pass

- Word aliases: `and`/`or`/`not`, `stop`/`next`, `ask`/`say`
- `x is empty` / `x is not empty`
- Membership expression: `x in collection`
- `repeat N times { ... }`
- Clearer errors for bindings, `when`, nil math, control outside loops
- Language rules written in README
- Full surface audit example: `examples/45_full_language_audit.vel`

# Vela roadmap — honest status

Vela is a **real interpreter** with a growing standard library. It is **not** a systems language, not a package ecosystem like Python/npm, and not a GPU/compute runtime. Below is what exists vs what is still open.

## Working today

- Distinct syntax, interpreter, REPL, examples, tests (`bash tests/run_all.sh`)
- Files: `read_file`, `write_file`, `append_file`, `list_dir`, `file_exists`, `remove_file`
- Process/OS: `run`, `exit`, `getenv`/`setenv`, `args`, `cwd`/`chdir`, `sleep`, `platform`
- Resources: `mem_usage`, `cpu_time`, `call_depth`, `set_max_depth`, `stack_trace`
- Data: arrays, maps, sets, ranges, records, enums, match, modules
- `vela build` → standalone binary that still **embeds + interprets** (not full AOT)

## Not done (and not faked)

| Request | Status |
|---------|--------|
| Full AST → native machine code | Open |
| GPU programming | Out of scope for this runtime design |
| Thread/async concurrency | Open |
| Package/library manager (“pip for Vela”) | Open |
| Formal memory ownership / GC controls | Partial (limits + RSS only) |
| Debugger / profiler | Open |
| Complete SPEC for every edge | Draft only (`SPEC.md`) |
| Zero ASan leaks | Open (ASan still reports leaks) |

## Quality process (real)

```bash
bash tests/run_all.sh
bash tests/run_all.sh --asan   # may fail on known leaks
```

Feature / property / stress tests live under `tests/`. Examples are smoke tests only.

## Near-term priority (useful, not theater)

1. Fix ASan leaks  
2. Keep `examples/45_full_language_audit.vel` green as features grow  
3. Grow OS stdlib (paths, permissions, sockets if needed)  
4. Module layout for stdlib (`import "std/fs"`)  
5. Expand AST→C++ codegen subset without silent embed fallback  
6. True native codegen when the interpreter surface is stable  

## Design boundary

Vela talks to the OS through **explicit builtins** (files, env, processes). It does not expose raw kernel APIs or GPU drivers. That is a deliberate scope boundary unless the project pivots to a different backend (LLVM, C API bindings, etc.).
