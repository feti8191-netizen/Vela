
## Language rules (how Vela fits together)

Vela is meant to feel **one language**, not a pile of features. These rules are the core:

1. **Names have one birth**
   - `:=` creates a name once (fixed).
   - `<-` creates or updates a changeable name.
   - Using `:=` again on the same name in the same scope is an error.

2. **Blocks need a reason**
   - `when` always needs a condition before `{`.
   - `loop` / `repeat` own `stop`/`bail` and `next`/`skip`. Outside a loop those words are errors.

3. **Words and symbols mean the same thing**
   - `and` / `or` / `not` = `&&` / `||` / `!`
   - `stop` / `next` = `bail` / `skip`
   - `ask` / `say` = `readline` / `print`
   - Pick the style you like; both are real Vela, not two dialects.

4. **`is` answers “what kind / what state?”**
   - Types: `x is num`, `x is str`, `x is Shape`
   - State: `x is empty`, `x is not empty`, `x is nil`
   - Same keyword, same place in the grammar.

5. **`in` means membership in expressions**
   - `3 in nums`, `"a" in "cat"`
   - In `repeat x in list`, `in` marks the collection to walk — same idea: “elements of”.

6. **Math needs numbers**
   - `try_num` may yield `nil`. Check before `+ - * /`, or errors will say so clearly.

7. **Top to bottom**
   - Lines after a `when` still run unless they sit inside that `when`/`else` chain.
   - Put “only if OK” code inside the matching branch.

These rules decide where features belong — not “because another language has it.”


# Vela

**Vela** is a small general-purpose programming language with its own syntax, implemented from scratch in portable **C++17**.

It is not a Python or JavaScript dialect. Bindings, mutation, functions, and control flow use dedicated forms so the language stays readable and distinct.

```bash
g++ -std=c++17 -O2 -o vela vela.cpp
# or: make
./vela examples/01_hello.vel
./vela                  # interactive REPL
```

---

## Quick start

```vela
name := "Vela";          // immutable binding
count <- 0;              // mutable binding
count <- count + 1;

add(a, b) ==> a + b;
greet(who, greeting := "Hello") ==> {
    print("${greeting}, ${who}!");
};

print(add(2, 3));
greet("Ada");
```

**Safe number input**

```vela
n := try_num(readline("Number: "));
when n == nil {
    print("not a number");
}
else {
    print("double =", n * 2);
}
```

---

## Build

Requires a C++17 compiler (`g++` or `clang++`).

```bash
make
make clean
```

Or:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic -o vela vela.cpp
```

### CLI

| Command | Meaning |
|---------|---------|
| `./vela` | Start the REPL |
| `./vela file.vel` | Run a script (interpreter) |
| `./vela build file.vel -o name` | Package a standalone native binary |
| `./vela --tokens file.vel` | Dump lexer tokens |
| `./vela --version` | Print version |
| `./vela --help` | Show help |

### Standalone binaries

```bash
./vela build examples/01_hello.vel -o hello
./hello
```

This embeds your program with the runtime and compiles with `g++`. The result is a normal OS binary. **Core subset** programs are lowered **AST → C++** (no source embedding). Unsupported constructs cause `vela build` to **fail** with a list of reasons (no silent embed).

Supported for build: bindings, numbers/strings/bools, arithmetic, logic (`&&` `||` `??`), ternary, `is`, ranges, compound assign, `assert`, `print`/named calls, **functions** (`==>`), **pipe** (`|>`), `when`/`else`, `loop`, `repeat`, arrays/maps (basic), index, field access, string interpolation.

Full language: `./vela file.vel` (interpreter).


Run `vela build` from the directory that contains `vela.cpp`. On Windows, use MinGW `g++` to produce `.exe`.

---

## Language overview

### Bindings (`:=` vs `<-`)

| Syntax | Meaning | Can change later? |
|--------|---------|-------------------|
| `x := value` | Create once (immutable) | No |
| `x <- value` | Create or update (mutable) | Yes |
| `x += 1` | Compound update (mutable only) | — |

If a value will change, start with `<-`.

### Functions

```vela
square(n) ==> n * n;
abs(n) ==> {
    when n < 0 { -> -n; }
    -> n;
};
dbl(x) ==> x * 2;
print(3 |> dbl |> dbl);
```

### Control flow

```vela
x := 5;
when x > 0 { print("positive"); }
else when x < 0 { print("negative"); }
else { print("zero"); }

repeat i from 0 through 3 { print(i); }
repeat i from 0 to 10 by 2 { print(i); }
repeat item in [10, 20] { print(item); }
repeat 3 times { print("tick"); }

n <- 0;
loop {
  n <- n + 1;
  when n == 2 { next; }   // skip rest of this turn
  when n >= 3 { stop; }   // leave the loop
  print(n);
}
```

`->` returns from a function.  
`stop` / `bail` leave a loop; `next` / `skip` continue to the next turn.  
Word forms `and` / `or` / `not` match `&&` / `||` / `!`.

### Data

Arrays, maps (`%{key: value}`), sets (`#{1, 2}`), ranges, records, enums, tuples, destructuring, slices, pipe (`|>`), match, modules (`import`), membership (`x in list`), emptiness (`x is empty`).

### OS and resources

| Area | Builtins |
|------|----------|
| Files | `read_file`, `write_file`, `append_file`, `list_dir`, `file_exists`, `remove_file` |
| Process | `run`, `exit`, `args`, `platform`, `cwd`, `chdir`, `sleep` |
| Env | `getenv`, `setenv` |
| Resources | `mem_usage`, `cpu_time`, `call_depth`, `set_max_depth`, `stack_trace` |
| Convert | `num` (throws), `try_num` (returns `nil`), `str`, `type`, `len` |

---

## Examples

See `examples/` (numbered `01`–`26` plus `utils_mod.vel`).

```bash
./vela examples/01_hello.vel
./vela examples/26_os.vel
```

---

## Project layout

```
vela/
├── vela.cpp       # Interpreter (single translation unit)
├── Makefile
├── README.md
├── explain.md     # Language explanation
├── SPEC.md        # Spec draft
├── ROADMAP.md     # Honest status / future work
└── examples/      # Feature examples
```

---

## What Vela is (and is not)

**Is:** a custom-syntax scripting language with a C++17 tree-walking interpreter, modules, collections, and host OS access (files, env, processes).

**Is not:** a finished systems language, GPU runtime, package manager, or full AOT compiler. See `ROADMAP.md` for open work.

---


### Readable syntax

| Write | Meaning |
|-------|---------|
| `and` / `or` | same as `&&` / `\|\|` |
| `stop` | same as `bail` (leave loop) |
| `next` | same as `skip` (next iteration) |
| `x is empty` | string/list/map/nil has no content |
| `x is not empty` | opposite |
| `not x` | same as `!x` |
| `x in list` | membership test |
| `repeat 5 times { }` | loop N times |

### Friendly I/O aliases

- `ask(prompt)` — same as `readline(prompt)`
- `say(...)` — same as `print(...)`

### Newer language features

- **Enums with data:** `enum Shape { Circle(r), Rect(w, h) }` and `match s { Shape.Circle(r) => ... }`
- **Record methods:** `record Point { x: num, y: num, dist() ==> ..., }` then `p.dist()`
- **Tuples:** `(1, "a", true)` with index and destructuring
- **Strings:** `to_upper`, `to_lower`, `index_of`, `pad_left`, `pad_right`, `substr`

### Traits and impl

```vela
trait Show { show() : str }
record Point { x: num, y: num }
impl Show for Point { show() ==> "Point(...)", }
impl Point { dist() ==> sqrt(self.x**2 + self.y**2), }
```

### Std modules

```vela
import "std/str.vel" as S;
import "std/list.vel" as L;
```

### Selective import

```vela
import { upper, pad } from "std/str.vel";
```

### Set operations

`union`, `intersect`, `difference` (also `set_union`, `set_intersect`, `set_difference`).

### REPL

`.help` `.vars` `.type <expr>` `.load <file>` `.clear`

## Examples

`examples/` covers the language surface (01–45+).  
Full matrix: `./vela examples/45_full_language_audit.vel` (prints `FULL_AUDIT_OK`).

Also see readable-syntax samples: `43_readable_syntax.vel`, `44_in_not_times.vel`.

## License

Copyright 2026 Vela contributors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
