# Vela — Language Explanation

Vela is a general-purpose programming language built around a few clear ideas: explicit binding, explicit mutation, explicit function bodies, and a syntax that does not copy mainstream scripting languages. This document explains the language itself.

---

## Nature of the language

Vela is dynamically typed. Values carry their kinds at runtime. Optional type annotations act as runtime checks and documentation.

Programs are sequences of statements. Expressions produce values. Statements bind names, control flow, or perform effects.

Evaluation is eager. Functions close over their defining environment (lexical closures).

```vela
print("Hello, Vela");
x := 1 + 2 * 3;
print(x);
```

---

## Values

| Kind | Role | Example |
|------|------|---------|
| **Nil** | Absence | `nil` |
| **Number** | Numeric value | `42`, `3.14` |
| **String** | Text | `"hello"` |
| **Boolean** | Truth | `true`, `false` |
| **Array** | Ordered sequence | `[1, 2, 3]` |
| **Map** | String-keyed data | `%{name: "Ada"}` |
| **Set** | Unique membership | `#{1, 2, 3}` |
| **Range** | Numeric progression | `0 to 10 by 2` |
| **Function** | User callable | `add(a, b) ==> a + b` |
| **Native** | Built-in callable | `print`, `len` |
| **Record** | Named structured instance | `new Point{x: 1, y: 2}` |
| **Enum** | Named variant | `Status.Ok` |

Equality is structural for arrays and maps, by value for numbers, strings, and booleans.

```vela
assert [1, 2] == [1, 2];
assert %{a: 1} == %{a: 1};
assert 3.0 == 3;
```

---

## Bindings and mutation

Vela separates **create once** from **create or update**.

| Form | Meaning | Can change later? |
|------|---------|-------------------|
| `name := value` | Immutable binding | No |
| `name <- value` | Mutable create or assign | Yes |
| `name += 1` | Compound update | Only if mutable |

**Rule:** if it will change, use `<-` from the first line. Use `:=` only for fixed names.

```vela
pi := 3.14159;
counter <- 0;
counter <- counter + 1;
counter += 1;
```

```vela
// Errors:
// pi <- 3;           // was created with :=
// counter := 5;      // already exists — use <-
// x := 1; x := 2;    // := only once per scope
```

Scopes nest. Closures see the environment where they were created, including later updates to mutable cells.

```vela
make_counter() ==> {
    n <- 0;
    -> () ==> {
        n <- n + 1;
        -> n;
    };
};
c := make_counter();
print(c());   // 1
print(c());   // 2
```

---

## Functions

```vela
square(n) ==> n * n;

abs(n) ==> {
    when n < 0 { -> -n; }
    -> n;
};
```

### Defaults and named arguments

```vela
greet(who, greeting := "Hello") ==> "${greeting}, ${who}!";
print(greet("Ada"));
print(greet(who := "Grace", greeting := "Hey"));
```

Defaults are evaluated at call time when the argument is omitted.

### Pipe

```vela
double(n) ==> n * 2;
print(5 |> double);
print(5 |> double |> double);
```

`x |> f` calls `f(x)`. `x |> f(y)` calls `f(x, y)`.

### Method-style calls

If a function’s first parameter is named **`self`**, then `obj.method(args)` passes `obj` as `self`.

---

## Operators

Arithmetic: `+` `-` `*` `/` `%` `**`  
Comparison and logic: `==` `!=` `<` `>` `<=` `>=` `&&` `||` `!`  
Readable words (same meaning): `and` `or` `not`  
Membership: `x in list`, `"a" in "cat"`  
Emptiness: `x is empty`, `x is not empty`  
Bitwise: `&` `|` `^` `~` `<<` `>>`  
Nullish and optional: `??` `?.`  
Ternary: `cond ? a : b`

Division or modulo by zero is a runtime error.  
`+` concatenates when either side is a string.

```vela
print(nil ?? "default");
print(%{}.missing ?? "n/a");
user := %{name: "Ada"};
print(user.address?.city);   // nil
```

---

## Readable names (aliases)

| Alias | Same as |
|-------|---------|
| `say(...)` | `print(...)` |
| `ask(prompt)` | `readline(prompt)` |
| `stop` | `bail` |
| `next` | `skip` |
| `and` / `or` / `not` | `&&` / `||` / `!` |

Same rules either way — pick the spelling that reads best for you.

## Control flow

### when / else

```vela
sign(n) ==> {
    when n > 0 { -> "positive"; }
    else when n < 0 { -> "negative"; }
    else { -> "zero"; }
};
```

### unless / until / loop

```vela
ready <- false;
unless ready {
    ready <- true;
    print("set up");
}

i <- 0;
loop {
    when i >= 3 { stop; }   // stop and bail are the same
    i <- i + 1;
}
print(i);
```

`stop` / `bail` leave the loop. `next` / `skip` jump to the next turn.

### repeat

```vela
repeat i from 0 to 5 { print(i); }           // end excluded
repeat i from 0 through 10 by 2 { print(i); } // end included
repeat item in [10, 20, 30] { print(item); }
repeat [k, v] in [[1, "a"], [2, "b"]] { print(k, v); }
repeat 3 times { print("tick"); }
```

---

## Pattern matching

```vela
classify(v) ==> match v {
    0 => "zero",
    n when n < 0 => "negative",
    [h, ...t] => "list",
    %{role: r} => "role=" + r,
    _ => "other",
};

grade := match {
    score >= 90 => "A",
    score >= 80 => "B",
    _ => "F",
};
```

Enums participate in match and `is` checks.

---

## Structured data

**Maps:** `%{name: "Ada", age: 36}` — field access, `??`, `?.`  
**Sets:** `#{1, 2, 3}` with set helpers  
**Ranges:** `1 to 5`, `1 through 5`, `0 to 10 by 2`  
**Records:** `record Point { x: num, y: num, }` then `new Point{x: 1, y: 2}`  
**Destructuring:** `[a, b] := pair`, `[h, ...t] := arr`, `%{"k": v} := map`

---

## Errors and defer

```vela
try {
    throw "something went wrong";
} catch e {
    print(e);
}

try { throw 1; } catch { print("ignored binding"); }

work() ==> {
    defer print("cleanup");
    print("body");
    -> 1;
};
```

`defer` runs on block exit (normal or error), last-in first-out.

**Conversion:** `num(s)` throws on bad input; `try_num(s)` returns `nil` instead.

---

## Modules

```vela
// utils_mod.vel
VERSION := "1.0";
double(x) ==> x * 2;

// main
import "utils_mod.vel" as u;
print(u.VERSION);
print(u.double(21));
```

Imports resolve relative to the importing file’s directory and are cached.

---

## Host environment

Vela scripts can talk to the host through builtins:

| Area | Examples |
|------|----------|
| Files | `read_file`, `write_file`, `list_dir`, `file_exists` |
| Process | `run`, `exit`, `args`, `platform` |
| Paths | `cwd`, `chdir` |
| Env | `getenv`, `setenv` |
| Time | `now`, `sleep` |
| Resources | `mem_usage`, `cpu_time`, `call_depth`, `set_max_depth` |

This is intentional host access, not a full systems-programming or GPU runtime.

---

## Evaluation model

Source is tokenized, parsed into an AST, then evaluated by walking that tree. Runtime errors report a line and, when possible, the source line with a caret and binding hints.

`vela build` packages a program with the runtime into a native OS binary; the embedded program is still interpreted unless a future codegen backend replaces that path.

---

## Design stance

1. **Explicitness** — binding vs mutation; explicit returns.  
2. **Own syntax** — not a thin skin over another language.  
3. **Compact core** — few value kinds and control forms.  
4. **Practical builtins** — scripting, text, math, files, process.

Not part of the current language: static generics, traits, enums with associated data, generators, full concurrency, or complete AOT native codegen.

---

## Summary

Vela is dynamically typed. Names are immutable or mutable by design. Functions are first-class. Data is built from numbers, strings, arrays, maps, sets, ranges, records, and enums. Control uses conditionals, loops, pattern matching, errors, and defer. Modules split programs across files. Builtins reach files, environment, and processes on the host.

The meaning of a program is sequential evaluation of its statements under nested environments until the program ends or an uncaught error stops it.


## Enums with associated data

```vela
enum Shape { Circle(r), Rect(w, h), Point }

area(s) ==> match s {
    Shape.Circle(r) => PI * r * r,
    Shape.Rect(w, h) => w * h,
    Shape.Point => 0,
    _ => -1,
};
```

Unit variants (no fields) stay strings like `Color.Red`. Data variants are maps with `__tag` and named fields.

## Record methods

```vela
record Point {
    x: num,
    y: num,
    dist() ==> sqrt(self.x ** 2 + self.y ** 2),
}
p := new Point{x: 3, y: 4};
print(p.dist());
```

Methods receive the instance as `self` automatically.

## Tuples

```vela
t := (1, "two", true);
print(t[0], t[1]);
[a, b, c] := t;
```

Tuple literals use parentheses with at least one comma (or empty `()`).


## Traits and impl

```vela
trait Show {
    show() : str,
}
record Point { x: num, y: num }
impl Show for Point {
    show() ==> "Point(${self.x}, ${self.y})",
}
impl Point {
    dist() ==> sqrt(self.x ** 2 + self.y ** 2),
}
```

`impl Trait for Type` requires all trait methods. `impl Type` adds inherent methods.

## Map iteration

```vela
repeat [k, v] in %{a: 1, b: 2} {
    print(k, v);
}
```

## Multiple results

Return a tuple and destructure:

```vela
pair() ==> -> (1, 2);
[a, b] := pair();
```


## Selective import

```vela
import { upper, pad } from "std/str.vel";
print(upper("hi"));
```

Only listed names are bound. Missing names are a runtime error.

## Set operations

```vela
union(#{1,2}, #{2,3})
intersect(#{1,2}, #{2,3})
difference(#{1,2}, #{2,3})
```
