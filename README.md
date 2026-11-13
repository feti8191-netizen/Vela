# Vela

**Vela** is an **interpreted** general-purpose programming language.  
You write `.vel` files (or use the REPL); the Vela program reads them and runs them. There is no separate compile/build step.

Implemented from scratch in portable **C++17**.

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic -o vela vela.cpp
# or: make

./vela                  # interactive REPL
./vela examples/01_hello.vel
```

---

## Language rules

These rules apply everywhere in the language:

1. **Names**
   - `:=` creates a name once (fixed).
   - `<-` creates or updates a changeable name.
   - Using `:=` again on the same name in the same scope is an error.

2. **Blocks**
   - `when` always needs a condition before `{`.
   - `stop` / `bail` and `next` / `skip` only work inside `loop` / `repeat`.

3. **Same meaning, two spellings**
   - `and` / `or` / `not` = `&&` / `||` / `!`
   - `stop` / `next` = `bail` / `skip`
   - `ask` / `say` = `readline` / `print`

4. **`is`** — kind or state: `x is num`, `x is empty`, `x is nil`

5. **`in`** — membership: `3 in nums`, or `repeat x in list`

6. **Math needs numbers** — use `try_num(...)` for user input; check for `nil` before `+ - * /`.

7. **Top to bottom** — code after a `when` still runs unless it sits inside that branch.

---

## Quick start

```vela
name := "Vela";
count <- 0;
count <- count + 1;

add(a, b) ==> a + b;
print(add(2, 3));

when count > 0 {
    say("count is", count);
}
```

Safe number input:

```vela
n := try_num(ask("Number: "));
when n == nil {
    print("not a number");
}
else {
    print("double =", n * 2);
}
```

---

## Running programs

| Command | Meaning |
|---------|---------|
| `./vela` | Start the REPL |
| `./vela file.vel` | Run a script |
| `./vela --tokens file.vel` | Dump lexer tokens |
| `./vela --version` | Print version |
| `./vela --help` | Show help |

REPL helpers: `.help`, `.vars`, `.type <expr>`, `.load <file>`, `.clear`, `exit`

---

## Language overview

### Bindings

| Syntax | Meaning |
|--------|---------|
| `x := value` | Create once (immutable) |
| `x <- value` | Create or update (mutable) |
| `x += 1` | Compound update (mutable only) |

### Control flow

```vela
x := 5;
when x > 0 { print("positive"); }
else when x < 0 { print("negative"); }
else { print("zero"); }

repeat i from 0 through 3 { print(i); }
repeat item in [10, 20] { print(item); }
repeat 3 times { print("tick"); }

n <- 0;
loop {
    n <- n + 1;
    when n == 2 { next; }
    when n >= 3 { stop; }
}
```

### Functions

```vela
square(n) ==> n * n;
greet(who, greeting := "Hello") ==> {
    print("${greeting}, ${who}!");
};
print(3 |> square);
```

### Data

- Arrays: `[1, 2, 3]`, index, slices  
- Maps: `%{name: "Ada"}`  
- Sets: `#{1, 2}`  
- Records, enums, match, tuples, destructuring  
- Modules: `import "path.vel"`

### Readable forms

| Write | Meaning |
|-------|---------|
| `and` / `or` / `not` | `&&` / `\|\|` / `!` |
| `stop` / `next` | leave loop / next iteration |
| `x is empty` | no content |
| `x in list` | membership |
| `ask` / `say` | input / output |

---

## Examples

The `examples/` folder exercises the language surface.  
Full matrix: `./vela examples/45_full_language_audit.vel` (prints `FULL_AUDIT_OK`).

---

## Project layout

| Path | Role |
|------|------|
| `vela.cpp` | Interpreter (lexer, parser, runtime, REPL) |
| `Makefile` | `make` → build `vela` |
| `examples/` | Sample programs |
| `std/` | Optional standard modules |
| `explain.md` | Deeper language explanation |
| `SPEC.md` | Working behavior notes |
| `ROADMAP.md` | Honest status and open work |

---

## License

Copyright 2026 Vela contributors  

Licensed under the Apache License, Version 2.0.  
See <http://www.apache.org/licenses/LICENSE-2.0>
