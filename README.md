# Vela

**Vela** is an interpreted general-purpose language with its own syntax — not a Python clone.  
You write `.vel` files and run them with the Vela interpreter (or use the REPL).

```bash
make
./vela                         # REPL
./vela examples/01_hello.vel   # run a file
```

**Learn the language in depth:** see **[TUTORIAL.md](TUTORIAL.md)** — full tutorial (tokens, bindings, control flow, functions, collections, modules, errors, records, **visual art**, builtins, std, REPL, common mistakes).  
**Behavior notes:** [SPEC.md](SPEC.md) · **Design status:** [ROADMAP.md](ROADMAP.md) · **Narrative guide:** [explain.md](explain.md)

---

## Core rules

1. **`:=`** creates a fixed name once. **`<-`** creates or updates a changeable name.  
2. **`when`** always needs a condition before `{`.  
3. **`stop` / `next`** only work inside `loop` / `repeat` (`bail` / `skip` are the same).  
4. **`and` / `or` / `not`** work like `&&` / `||` / `!`.  
5. **User input to math:** `try_num(...)` then check for `nil`.  
6. Keywords such as `from`, `to`, `by`, `new` cannot be parameter names.

---

## Quick examples

```vela
name := "Vela";
count <- 0;
count <- count + 1;
print(name, count);
```

```vela
add(a, b) ==> a + b;
print(add(2, 3));
print(3 |> add(4));
```

```vela
x := 5;
when x > 0 {
    print("positive");
}
else {
    print("not positive");
}

repeat i from 0 through 2 {
    print(i);
}
```

```vela
import "std/math.vel" as M;
print(M.clamp(20, 0, 10));
```

```vela
draw 200 by 200 {
    paper(26, 26, 46);      // background as r,g,b
    paint(233, 69, 96);     // fill color
    no_edge();
    circle(100, 100, 80);
}
print(save_art("hello.bmp"));
```

---

## Running Vela

| Command | Meaning |
|---------|---------|
| `./vela` | REPL |
| `./vela file.vel` | Run a program |
| `./vela --tokens file.vel` | Dump tokens |
| `./vela --version` | Version |
| `./vela --help` | Help |

REPL: `.help` · `.vars` · `.type <expr>` · `.load <file>` · `.clear` · `exit`

---

## Feature map

| Area | Highlights |
|------|------------|
| Data | arrays, maps `%{}`, sets `#{}`, strings, numbers |
| Control | `when`, `loop`, `repeat`, `until`, `unless`, `match` |
| Functions | `name(params) ==> body`, defaults, pipe `\|>` |
| Modules | `import "path" as Name` |
| Std | `std/str`, `list`, `math`, `check`, `art`, … |
| Art | `canvas`, shapes, `noise`, `save_art` → BMP/PPM |

---

## Examples folder

Numbered programs under `examples/` exercise the language.  
Broad check: `./vela examples/45_full_language_audit.vel`  
Art: `examples/70_art_hello.vel` … `76_art_bars.vel`

---

## Project layout

| Path | Role |
|------|------|
| `vela.cpp` | Interpreter (lexer, parser, runtime, art, REPL) |
| `Makefile` | `make` builds `./vela` |
| `TUTORIAL.md` | Full language tutorial |
| `examples/` | Sample programs |
| `std/` | Standard modules |

---

## License

Copyright 2026 Vela contributors  

Licensed under the Apache License, Version 2.0.  
See <http://www.apache.org/licenses/LICENSE-2.0>
