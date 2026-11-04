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
```

`->` returns from a function. `bail` breaks a loop; `skip` continues.

### Data

Arrays, maps (`%{key: value}`), sets (`#{1, 2}`), ranges, records, enums, tuples, destructuring, slices, pipe (`|>`), match, modules (`import`).

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
