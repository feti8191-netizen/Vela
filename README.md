# Vela

**Vela** is a small general-purpose programming language with its own syntax, implemented from scratch in portable **C++17**.

It is not a Python or JavaScript dialect. Bindings, functions, and control flow use dedicated operators so the language reads distinctly while staying simple enough for scripting, math, and everyday automation.

```bash
g++ -std=c++17 -O2 -o vela vela.cpp
./vela examples/01_hello.vel
./vela                  # interactive REPL
```

---

## Quick start

```vela
// Variables
name := "Vela";          // immutable
count <- 0;              // mutable
count <- count + 1;

// Functions
add(a, b) ==> a + b;
greet(who, greeting := "Hello") ==> {
    print("${greeting}, ${who}!");
};

print(add(2, 3));
greet("Ada");
greet("Bob", "Hi");
```

**Input**

```vela
line := readline("Your name: ");
print("Hello,", line);
n := num(readline("Number: "));
print("double =", n * 2);
```

---

## Build

Requirements: a C++17 compiler (`g++` or `clang++`).

```bash
make              # builds ./vela
make clean
```

Or directly:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic -o vela vela.cpp
```

### CLI

| Command | Meaning |
|---------|---------|
| `./vela` | Start the REPL |
| `./vela file.vel` | Run a source file |
| `./vela --tokens file.vel` | Dump lexer tokens |
| `./vela --help` | Show help |

**REPL commands:** `.help` · `.vars` · `.clear` · `exit`

---

## Language overview

### Bindings

| Syntax | Meaning |
|--------|---------|
| `x := value` | Immutable binding |
| `x <- value` | Mutable binding / assignment |
| `x += 1` | Compound assignment (`-=`, `*=`, `/=`, `%=`) |

### Functions

```vela
square(n) ==> n * n;
factorial(n) ==> {
    when n <= 1 { -> 1; }
    else { -> n * factorial(n - 1); }
};

// Defaults and named arguments
rect(w, h, unit := "px") ==> "${w}x${h}${unit}";
print(rect(10, 20));
print(rect(w := 3, h := 4, unit := "em"));

// Lambdas and pipe
double(x) ==> x * 2;
print(5 |> double |> double);
```

### Control flow

```vela
when x > 0 { print("positive"); }
else when x < 0 { print("negative"); }
else { print("zero"); }

unless ready { setup(); }
until done { step(); }

repeat i from 0 through 10 { print(i); }
repeat i from 0 to 100 by 5 { print(i); }
repeat item in list { print(item); }
repeat [k, v] in pairs { print(k, v); }

loop {
    when finished { bail; }   // break
    when skip_me { skip; }    // continue
}
```

`-> expr` returns from a function. `bail` / `skip` break or continue loops.

### Data

```vela
// Arrays, slices, spread
a := [1, 2, 3, 4, 5];
print(a[1:4], a[::-1], a[::2]);
b := [...a, 6];
a[1:3] <- [9, 9];

// Maps
user := %{name: "Ada", age: 36};
print(user.name);
print(user.missing ?? "n/a");
print(user.address?.city);

// Sets
s := #{1, 2, 3};
set_add(s, 4);

// Ranges
r := 1 to 10;            // exclusive end
r2 := 1 through 10;      // inclusive
r3 := 0 to 100 by 5;

// Tuples (fixed sequences)
t := (1, "two", true);
```

### Records, enums, match

```vela
record Point { x: num, y: num, }
p := new Point{x: 3, y: 4};

enum Status { Ok, Err }
print(Status.Ok is Status);

print(match value {
    0 => "zero",
    n when n < 0 => "negative",
    [h, ...t] => "list",
    %{type: "user", name: n} => n,
    _ => "other",
});

// Subjectless match (cond-style)
print(match {
    score >= 90 => "A",
    score >= 80 => "B",
    _ => "C",
});
```

### Errors and cleanup

```vela
try {
    throw "something went wrong";
} catch e {
    print(e);
}

f() ==> {
    defer print("cleanup");
    -> do_work();
};
```

### Modules

```vela
import "utils.vel";
import "utils.vel" as u;
print(u.helper(1));
```

---

## Standard library (selected)

| Area | Functions |
|------|-----------|
| I/O | `print`, `eprint`, `readline`, `read_file`, `write_file`, `append_file`, `list_dir` |
| Types | `type`, `str`, `num`, `len`, `assert` |
| Collections | `map`, `filter`, `reduce`, `sum`, `sorted`, `reverse`, `unique`, `flatten`, `concat`, `zip`, `enumerate`, `chunks`, `windows`, `partition`, `group_by`, `any`, `all`, `contains` |
| Sets | `set`, `set_has`, `set_add`, `set_remove`, `set_union`, `set_intersect`, `set_difference` |
| Strings | `chars`, `trim`, `replace`, `starts_with`, `ends_with` |
| Regex | `re_match`, `re_find_all`, `re_replace` |
| JSON | `json_parse`, `json_stringify` |
| Encoding | `base64_encode`, `hex_encode` |
| Math | `sqrt`, `sin`, `cos`, `pow`, `abs`, `floor`, `ceil`, `round`, `min`, `max`, `random`, `gcd`, `lcm`, `is_prime`, `log`, … |
| Constants | `PI`, `E`, `TAU`, `INF` |
| Time | `now` |

---

## Examples

Each major feature has a dedicated example under `examples/`:

| File | Topic |
|------|--------|
| `01_hello.vel` | Printing |
| `02_bindings.vel` | `:=` / `<-` |
| `03_functions.vel` | Functions, defaults, named args |
| `04_lambdas_pipe.vel` | Lambdas, pipe |
| `05_control.vel` | when / skip / bail |
| `06_until_unless.vel` | until, unless |
| `07_ranges.vel` | Ranges and steps |
| `08_arrays_slices.vel` | Arrays, slices, spread |
| `09_destructure.vel` | Destructuring |
| `10_maps.vel` | Maps, `?.`, `??` |
| `11_records.vel` | Records |
| `12_enums_match.vel` | Enums and match |
| `13_errors_defer.vel` | throw / try / defer |
| `14_closures.vel` | Closures, recursion |
| `15_math.vel` | Math library |
| `16_collections.vel` | Collection helpers |
| `17_strings.vel` | Strings and regex |
| `18_sets.vel` | Sets |
| `19_json_files.vel` | JSON and files |
| `20_bitwise_types.vel` | Bitwise ops, `is`, tuples |
| `21_modules.vel` | Imports |
| `22_operators.vel` | Operators, ternary |
| `23_cond_match.vel` | Subjectless match |
| `24_encoding_dir.vel` | Encoding, `list_dir` |

```bash
./vela examples/01_hello.vel
./vela examples/14_closures.vel
```

---

## Project layout

```
vela/
├── vela.cpp          # Full interpreter (single translation unit)
├── Makefile
├── README.md
└── examples/         # Feature examples (.vel)
```

The implementation is a classic tree-walking interpreter: lexer → recursive-descent parser → AST → evaluator, plus a small standard library of native functions.

---

## Design notes

- **Immutable vs mutable** is explicit (`:=` vs `<-`).
- **Functions** use `==>` for bodies and `->` for returns.
- **Method-style calls** `obj.fn(args)` inject `obj` as the first argument only when the parameter is named `self`.
- **Division or modulo by zero** raises a runtime error.
- Runtime errors print the source line and a caret when possible.

---

## License

Add your preferred license here (MIT, Apache-2.0, etc.).
