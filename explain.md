# Vela — Language Explanation

Vela is a general-purpose programming language designed around a small set of explicit ideas: clear binding, clear mutation, clear function bodies, and a syntax that does not imitate mainstream scripting languages. This document explains what the language is, how its concepts fit together, and shows each idea with short examples.

---

## Nature of the language

Vela is dynamically typed and expression-oriented where it helps, statement-oriented where control flow needs structure. Values carry their types at runtime. Optional type annotations act as runtime checks and documentation, not as a full static type system.

Programs are sequences of statements. Expressions produce values (arithmetic, calls, match, literals). Statements orchestrate binding, control flow, or side effects (declarations, loops, import, defer).

Evaluation is eager. Functions close over their defining environment (lexical closures).

```vela
print("Hello, Vela");
x := 1 + 2 * 3;
print(x);
```

---

## Values

Every runtime value belongs to one of these kinds:

| Kind | Role | Example |
|------|------|---------|
| **Nil** | Absence of a value | `nil` |
| **Number** | Numeric quantity | `42`, `3.14` |
| **String** | Text | `"hello"` |
| **Boolean** | Truth value | `true`, `false` |
| **Array** | Ordered sequence | `[1, 2, 3]` |
| **Map** | String-keyed data | `%{name: "Ada"}` |
| **Set** | Unique membership | `#{1, 2, 3}` |
| **Range** | Numeric progression | `0 to 10 by 2` |
| **Function** | User callable | `add(a, b) ==> a + b` |
| **Native** | Built-in callable | `print`, `len` |
| **Record** | Named structured instance | `new Point{x: 1, y: 2}` |
| **Enum** | Named variant | `Status.Ok` |

```vela
print(type(nil));      // nil
print(type(42));       // num
print(type("hi"));     // str
print(type(true));     // bool
print(type([1, 2]));   // arr
print(type(%{a: 1}));  // map
```

Equality is structural for arrays and maps (contents compared deeply), and by value for numbers, strings, and booleans.

```vela
assert [1, 2] == [1, 2];
assert %{a: 1} == %{a: 1};
assert 3.0 == 3;
```

---

## Bindings and mutation

Vela separates **definition** from **assignment**.

| Form | Meaning |
|------|---------|
| `name := value` | Immutable binding — cannot reassign that name |
| `name <- value` | Mutable binding — create or update |
| `name += 1` | Compound update (also `-=`, `*=`, `/=`, `%=`) |

```vela
pi := 3.14159;       // fixed name
counter <- 0;        // may change
counter <- counter + 1;
counter += 1;

// pi <- 3;          // error: cannot assign to immutable variable
```

Scopes nest. Inner names may shadow outer ones. Closures see the environment where they were created, including later updates to mutable cells.

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

A function has parameters and a body. The body is either one expression or a block. **`->`** returns a value from a block.

```vela
// Expression body
square(n) ==> n * n;

// Block body with return
abs(n) ==> {
    when n < 0 { -> -n; }
    -> n;
};

print(square(5));
print(abs(-7));
```

### Default and named arguments

```vela
greet(who, greeting := "Hello") ==> "${greeting}, ${who}!";

print(greet("Ada"));
print(greet("Bob", "Hi"));
print(greet(who := "Grace", greeting := "Hey"));
```

Defaults are evaluated at call time when the argument is omitted.

### Lambdas and first-class functions

```vela
apply_twice(f, x) ==> f(f(x));
print(apply_twice((n) ==> n * 2, 3));   // 12
```

### Pipe

The pipe operator threads a value into a call on the right.

```vela
double(n) ==> n * 2;
inc(n) ==> n + 1;
print(5 |> double);           // 10
print(5 |> double |> inc);    // 11
```

### Method-style calls

If a function is stored on a map/record and its first parameter is named **`self`**, a call `obj.method(args)` passes `obj` as `self`.

```vela
dist(self) ==> sqrt(self.x ** 2 + self.y ** 2);
p := %{x: 3, y: 4, dist: dist};
print(p.dist());   // 5
```

---

## Operators and expressions

### Arithmetic

```vela
print(1 + 2 * 3);     // 7
print((1 + 2) * 3);   // 9
print(2 ** 8);        // 256
print(10 % 3);        // 1
print(-5);            // -5
```

Division or modulo by zero is a runtime error. `+` concatenates if either side is a string.

```vela
print("n=" + str(42));
print("hi" + "!");
```

### Comparison, logic, bitwise

```vela
print(3 < 5 && 5 < 10);
print(false || true);
print(!false);
print(5 & 3);    // 1
print(5 | 2);    // 7
print(1 << 4);   // 16
```

### Null-coalescing and optional access

```vela
print(nil ?? "default");     // default
print(0 ?? 99);              // 0  (only nil triggers ??)
print(%{}.missing ?? "n/a");

user := %{name: "Ada"};
print(user.address?.city);   // nil — no error
```

### Ternary

```vela
x := 7;
print(x > 10 ? "big" : "small");
```

### Indexing and slicing

```vela
a := ["a", "b", "c", "d", "e"];
print(a[0]);          // a
print(a[-1]);         // e
print(a[1:4]);        // [b, c, d]
print(a[::-1]);       // reversed
print(a[::2]);        // every second

// Slice assignment
buf <- [1, 2, 3, 4, 5];
buf[1:4] <- [9, 8];
print(buf);           // [1, 9, 8, 5]
```

### Spread and interpolation

```vela
more := [...a, "f"];
print("Hello, ${"Vela"}!");
```

---

## Control flow

### when / else

```vela
sign(n) ==> {
    when n > 0 { -> "positive"; }
    else when n < 0 { -> "negative"; }
    else { -> "zero"; }
};
```

### unless / until

```vela
unless ready {
    setup();
}

until done {
    step();
}
```

### repeat

```vela
// Numeric range (exclusive end)
repeat i from 0 to 5 {
    print(i);          // 0 1 2 3 4
}

// Inclusive end + step
repeat i from 0 through 10 by 2 {
    print(i);          // 0 2 4 6 8 10
}

// Over a collection
repeat item in [10, 20, 30] {
    print(item);
}

// Destructure each element
repeat [k, v] in [[1, "a"], [2, "b"]] {
    print(k, v);
}
```

### loop, bail, skip

```vela
repeat i from 0 through 10 {
    when i % 2 == 0 { skip; }    // continue
    print(i);
    when i >= 7 { bail; }        // break
}
```

### Return

```vela
early(x) ==> {
    when x == 0 { -> "zero"; }
    -> "other";
};
```

---

## Pattern matching

Match compares a subject to arms: pattern, optional guard, body. First success wins.

```vela
classify(v) ==> match v {
    0 => "zero",
    n when n < 0 => "negative",
    [x] => "singleton",
    [h, ...t] => "list head=" + str(h),
    %{role: r} => "role=" + r,
    _ => "other",
};
```

### Enums in match

```vela
enum Status { Ok, Warn, Error }

describe(s) ==> match s {
    Status.Ok => "all good",
    Status.Warn => "careful",
    Status.Error => "failed",
    _ => "unknown",
};
```

### Subjectless match (cond-style)

```vela
grade := match {
    score >= 90 => "A",
    score >= 80 => "B",
    score >= 70 => "C",
    _ => "F",
};
```

---

## Structured data

### Maps

```vela
user := %{
    name: "Ada",
    age: 36,
};
print(user.name);
user.age <- 37;
```

### Sets

```vela
s := #{1, 2, 3};
print(set_has(s, 2));
set_add(s, 4);
set_remove(s, 1);
print(set_union(#{1, 2}, #{2, 3}));
print(set_intersect(#{1, 2}, #{2, 3}));
print(set_difference(#{1, 2}, #{2, 3}));
```

### Ranges

```vela
r := 1 to 5;           // 1, 2, 3, 4
r2 := 1 through 5;     // 1, 2, 3, 4, 5
r3 := 0 to 10 by 2;
print(len(r));
```

### Records

```vela
record Point { x: num, y: num, }
p := new Point{x: 3, y: 4};
print(p.x, p.y);
print(p is Point);
```

### Enums

```vela
enum Color { Red, Green, Blue }
c := Color.Green;
print(c is Color);
```

### Tuples

```vela
t := (1, "two", true);
print(t[0], t[1], t[2]);
```

### Destructuring

```vela
[x, y] := [3, 4];
[head, ...tail] := [1, 2, 3, 4];
%{"name": n, "age": a} := %{name: "Ada", age: 36};
```

---

## Errors and defer

```vela
safe_div(a, b) ==> {
    when b == 0 { throw "division by zero"; }
    -> a / b;
};

try {
    print(safe_div(1, 0));
} catch e {
    print("caught:", e);
}

try {
    throw 404;
} catch {
    print("ignored binding");
}
```

**defer** registers work to run when the block exits (normal return or error), in last-in-first-out order.

```vela
work() ==> {
    defer print("bye");
    print("hello");
    -> 1;
};
// prints: hello, then bye
```

---

## Modules

```vela
// utils.vel
VERSION := "1.0";
double(x) ==> x * 2;

// main.vel
import "utils.vel" as u;
print(u.VERSION);
print(u.double(21));
```

Imports are cached; the same path shares one module environment.

---

## Type tests and assertions

```vela
print(42 is num);
print("hi" is str);
print(p is Point);

assert 1 + 1 == 2;
assert len([1, 2]) == 2;
```

Optional annotations are checked at runtime:

```vela
square(n: num) : num ==> n * n;
```

---

## Standard library (overview)

### I/O and files

- `print`, `eprint`, `readline`
- `read_file`, `write_file`, `append_file`, `list_dir`

### Collections

- `map`, `filter`, `reduce`, `sum`, `sorted`, `reverse`
- `unique`, `flatten`, `concat`, `zip`, `enumerate`
- `chunks`, `windows`, `partition`, `group_by`, `any`, `all`

### Strings and regex

- `chars`, `trim`, `replace`, `starts_with`, `ends_with`
- `re_match`, `re_find_all`, `re_replace`

### JSON and encoding

- `json_parse`, `json_stringify`
- `base64_encode`, `hex_encode`

### Math

- `sqrt`, `sin`, `cos`, `pow`, `abs`, `floor`, `ceil`, `round`, `min`, `max`
- `gcd`, `lcm`, `is_prime`, `log`, `random`
- Constants: `PI`, `E`, `TAU`, `INF`
- `now` — timestamp

```vela
print(map([1, 2, 3], (n) ==> n * n));
print(filter([1, 2, 3, 4], (n) ==> n % 2 == 0));
print(gcd(48, 18));
print(json_stringify(%{ok: true}));
```

---

## Evaluation model

Source is tokenized, parsed into an abstract syntax tree, then evaluated by walking that tree. There is no bytecode VM in the current design. Recursion depth is limited by a guard. Runtime errors report a line number and, when possible, the source line with a caret.

The REPL keeps a persistent global environment across inputs so definitions accumulate during a session.

---

## Design stance

Vela prioritizes:

1. **Explicitness** — binding vs mutation, and explicit returns.
2. **A non-imitative syntax** — recognizable as its own language.
3. **A compact core** — few value kinds and control forms, still enough for structured programs.
4. **Practical builtins** — scripting, text, math, and files without a large package ecosystem.

Not part of the current language: static generics, traits, enums with associated data payloads, generators, tail-call elimination, concurrency, or a bytecode compiler. Those are possible future layers.

---

## Summary

Vela is dynamically typed. Names are introduced as immutable or mutable by design. Functions are first-class and close over their environment. Data is built from numbers, strings, arrays, maps, sets, ranges, records, and enums. Control flow uses conditionals, loops, pattern matching, errors, and deferred cleanup. Modules divide programs across files. A standard library of native functions covers I/O, collections, text, and math.

The meaning of a program is the sequential evaluation of its statements and expressions under nested environments, with values flowing through calls, operators, and matches until the program ends or an uncaught error stops it.

---

## More examples by topic

### Numbers and formulas

```vela
a := 12;
b := 5;
print("sum", a + b);
print("diff", a - b);
print("prod", a * b);
print("quot", a / b);
print("rem", a % b);
print("power", a ** 2);

// Hypotenuse
hyp(dx, dy) ==> sqrt(dx ** 2 + dy ** 2);
print(hyp(3, 4));   // 5

// Clamp a value into [lo, hi]
clamp(x, lo, hi) ==> {
    when x < lo { -> lo; }
    when x > hi { -> hi; }
    -> x;
};
print(clamp(15, 0, 10));   // 10
print(clamp(-3, 0, 10));   // 0
```

### Strings and text

```vela
first := "Vela";
last := "Lang";
print(first + " " + last);
print(len(first));
print(chars("abc"));              // ["a", "b", "c"]
print(trim("  spaced  "));
print(replace("foo bar foo", "foo", "x"));
print(starts_with("hello", "he"));
print(ends_with("hello", "lo"));

lang := "Vela";
print("Using ${lang} today");

print(re_match("\\d+", "room 42"));
print(re_find_all("\\w+", "one two 3"));
print(re_replace("\\s+", "a  b   c", "-"));
```

### Arrays in practice

```vela
nums := [3, 1, 4, 1, 5, 9, 2, 6];
print(sorted(nums));
print(unique(nums));
print(sum(nums));
print(map(nums, (n) ==> n * n));
print(filter(nums, (n) ==> n > 4));
print(any(nums, (n) ==> n > 8));
print(all(nums, (n) ==> n > 0));

// Build a new list with spread
extra := [...nums, 0, 0];
print(len(extra));

// Sliding windows of length 3
print(windows([1, 2, 3, 4, 5], 3));
// [[1,2,3], [2,3,4], [3,4,5]]

// Partition into evens and odds
[evens, odds] := partition(nums, (n) ==> n % 2 == 0);
print(evens);
print(odds);
```

### Maps as records of data

```vela
book := %{
    title: "Principia",
    year: 1687,
    tags: ["math", "physics"],
};

print(book.title);
print(book.tags[0]);
book.year <- 1687;

// Safe navigation
print(book.publisher?.name);   // nil
print(book.publisher?.name ?? "unknown");
```

### Nested functions and closures

```vela
make_adder(n) ==> (x) ==> x + n;
add10 := make_adder(10);
add100 := make_adder(100);
print(add10(5));    // 15
print(add100(5));   // 105

// Running average
make_avg() ==> {
    total <- 0;
    count <- 0;
    -> (x) ==> {
        total <- total + x;
        count <- count + 1;
        -> total / count;
    };
};
avg := make_avg();
print(avg(10));
print(avg(20));
print(avg(30));
```

### Recursion

```vela
fib(n) ==> {
    when n < 2 { -> n; }
    -> fib(n - 1) + fib(n - 2);
};
print(fib(10));   // 55

// Sum with match + rest pattern
sum_list(xs) ==> match xs {
    [] => 0,
    [h, ...t] => h + sum_list(t),
};
print(sum_list([1, 2, 3, 4, 5]));   // 15
```

### Range loops and accumulation

```vela
// Sum of multiples of 3 below 30
total <- 0;
repeat i from 0 to 30 by 3 {
    total <- total + i;
}
print(total);

// Countdown
repeat i from 5 through 1 by -1 {
    print(i);
}
```

### Match for control decisions

```vela
http_status(code) ==> match code {
    200 => "ok",
    201 => "created",
    400 => "bad request",
    404 => "not found",
    n when n >= 500 => "server error",
    _ => "other",
};
print(http_status(404));
print(http_status(503));

// Shape-like data with maps
area(shape) ==> match shape {
    %{kind: "circle", r: r} => PI * r * r,
    %{kind: "rect", w: w, h: h} => w * h,
    _ => 0,
};
print(area(%{kind: "circle", r: 2}));
print(area(%{kind: "rect", w: 3, h: 4}));
```

### Enums for states

```vela
enum Phase { Init, Run, Done }

step(phase) ==> match phase {
    Phase.Init => Phase.Run,
    Phase.Run => Phase.Done,
    Phase.Done => Phase.Done,
    _ => Phase.Init,
};

p <- Phase.Init;
p <- step(p);
print(p);
p <- step(p);
print(p);
```

### try / catch patterns

```vela
parse_positive(s) ==> {
    n := num(s);
    when n == nil { throw "not a number"; }
    when n <= 0 { throw "must be positive"; }
    -> n;
};

try {
    print(parse_positive("12"));
    print(parse_positive("-3"));
} catch e {
    print("error:", e);
}
```

### Defer for cleanup ordering

```vela
ordered() ==> {
    defer print("third");
    defer print("second");
    print("first");
    -> nil;
};
ordered();
// first
// second
// third   (LIFO: last defer runs first)
```

### Pipe pipelines

```vela
nums := [1, 2, 3, 4, 5, 6];
// Conceptual pipeline style with intermediate names
squared := map(nums, (n) ==> n * n);
big := filter(squared, (n) ==> n > 10);
print(sum(big));

// Pipe into single-argument functions
triple(n) ==> n * 3;
print(4 |> triple |> triple);
```

### Sets for membership

```vela
allowed := #{"read", "write", "list"};
action := "write";
when set_has(allowed, action) {
    print("permitted");
}

a := #{1, 2, 3, 4};
b := #{3, 4, 5, 6};
print(set_has(set_intersect(a, b), 3));
print(set_has(set_difference(a, b), 1));
print(set_has(set_difference(a, b), 3));   // false
```

### JSON round-trip

```vela
doc := %{
    name: "Vela",
    version: 1,
    tags: ["lang", "cpp"],
};
text := json_stringify(doc);
back := json_parse(text);
print(back.name);
print(back.tags[0]);
```

### Combining features

```vela
// Score a list of results and grade them
results := [92, 78, 55, 88, 100];

grade(n) ==> match {
    n >= 90 => "A",
    n >= 80 => "B",
    n >= 70 => "C",
    n >= 60 => "D",
    _ => "F",
};

repeat score in results {
    print(score, "->", grade(score));
}

average := sum(results) / len(results);
print("average", average, "grade", grade(average));
```

```vela
// Simple contact list
contacts := [
    %{name: "Ada", mail: "ada@example.com"},
    %{name: "Alan", mail: "alan@example.com"},
];

find_mail(list, who) ==> {
    repeat c in list {
        when c.name == who { -> c.mail; }
    }
    -> nil;
};

print(find_mail(contacts, "Ada"));
print(find_mail(contacts, "Grace") ?? "not found");
```

```vela
// FizzBuzz with when
repeat i from 1 through 20 {
    when i % 15 == 0 { print("FizzBuzz"); }
    else when i % 3 == 0 { print("Fizz"); }
    else when i % 5 == 0 { print("Buzz"); }
    else { print(i); }
}
```

---

## End-to-end sketch

A small program shape that uses bindings, functions, data, match, and a loop:

```vela
record Student { name: str, score: num, }

grade_of(s) ==> match {
    s.score >= 90 => "A",
    s.score >= 80 => "B",
    s.score >= 70 => "C",
    _ => "F",
};

students := [
    new Student{name: "Ada", score: 95},
    new Student{name: "Alan", score: 82},
    new Student{name: "Grace", score: 67},
];

repeat s in students {
    print(s.name, s.score, grade_of(s));
}

total <- 0;
repeat s in students {
    total <- total + s.score;
}
print("class average", total / len(students));
```

This sketch is representative: immutable and mutable names, a pure function over a record, a list of structured values, a subjectless match for grading, and loops for reporting and aggregation.

---

## Detailed walkthrough examples

The following examples are longer on purpose. Each one states a small goal, shows the full code, then explains what happens step by step.

---

### Walkthrough 1 — Temperature table

**Goal:** Convert a list of Celsius values to Fahrenheit and print a small table.

```vela
to_f(c) ==> c * 9 / 5 + 32;

temps_c := [-10, 0, 15, 22, 37, 100];

print("C\tF");
repeat c in temps_c {
    f := to_f(c);
    print(str(c) + "\t" + str(f));
}
```

**What this does**

1. `to_f` is a pure function: one input, one expression body, no side effects.
2. `temps_c` is an immutable array of sample temperatures.
3. The loop binds each element to `c` in turn.
4. Inside the loop, `f` is a new immutable binding for the converted value.
5. `str` turns numbers into text so they can be concatenated for display.

**Variant with filtering**

```vela
// Only show temperatures at or below freezing
print("Freezing or below:");
repeat c in temps_c {
    when c <= 0 {
        print(c, "C =", to_f(c), "F");
    }
}
```

---

### Walkthrough 2 — Word stats from a sentence

**Goal:** Split text into words, count them, and find the longest word.

```vela
sentence := "vela is a small language with clear bindings";
words := re_find_all("\\w+", sentence);

print("word count:", len(words));
print("words:", words);

// Longest word
best <- "";
repeat w in words {
    when len(w) > len(best) {
        best <- w;
    }
}
print("longest:", best, "(" + str(len(best)) + " chars)");

// Frequency map: word -> count
freq <- %{};
repeat w in words {
    prev := freq[w] ?? 0;
    freq[w] <- prev + 1;
}
print("frequencies:", freq);
```

**What this does**

1. `re_find_all` returns an array of strings matching the pattern (word characters).
2. `best` is mutable so it can be updated as a longer word is found.
3. Map indexing with a missing key yields `nil`; `?? 0` turns that into a usable count.
4. `freq[w] <- ...` writes back into the map under that word key.

---

### Walkthrough 3 — Stack with explicit operations

**Goal:** Model a stack using an array and functions that return a new stack or a pair of values.

```vela
// Stack is just an array; top is the last element
empty() ==> [];

push(stack, value) ==> [...stack, value];

// Returns [value, new_stack] or throws if empty
pop(stack) ==> {
    when len(stack) == 0 {
        throw "pop from empty stack";
    }
    value := stack[-1];
    rest := stack[0:len(stack) - 1];
    -> [value, rest];
};

peek(stack) ==> {
    when len(stack) == 0 { -> nil; }
    -> stack[-1];
};

// Demo
s <- empty();
s <- push(s, 10);
s <- push(s, 20);
s <- push(s, 30);
print("stack", s);
print("peek", peek(s));

pair := pop(s);
value := pair[0];
s <- pair[1];
print("popped", value, "remaining", s);
```

**What this does**

1. Immutable-style updates: `push` builds a new array with spread instead of mutating in place.
2. `pop` returns two pieces of data in a small array (tuple-like).
3. The caller unpacks with indexing; destructuring could also be used: `[value, s] := pop(s)`.
4. Empty-stack `pop` uses `throw` so misuse is visible.

**Same pop with destructuring**

```vela
s <- push(push(empty(), 1), 2);
[top, s] := pop(s);
print(top);    // 2
print(s);      // [1]
```

---

### Walkthrough 4 — Pipeline of list transforms

**Goal:** Start from raw numbers, keep only even values, square them, and sum the result.

```vela
raw := [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

is_even(n) ==> n % 2 == 0;
square(n) ==> n * n;

evens := filter(raw, is_even);
squares := map(evens, square);
total := sum(squares);

print("raw", raw);
print("evens", evens);
print("squares", squares);
print("total", total);   // 2²+4²+…+10² = 220
```

**Step-by-step values**

| Stage | Value |
|-------|--------|
| raw | `[1, 2, …, 10]` |
| after filter | `[2, 4, 6, 8, 10]` |
| after map | `[4, 16, 36, 64, 100]` |
| after sum | `220` |

**Same idea with nested calls**

```vela
print(sum(map(filter(raw, is_even), square)));
```

---

### Walkthrough 5 — Configuration with defaults

**Goal:** Merge user options onto defaults using maps and null-coalescing.

```vela
defaults := %{
    host: "localhost",
    port: 8080,
    debug: false,
    name: "app",
};

// User only overrides some keys
user_opts := %{
    port: 3000,
    debug: true,
};

resolve(defaults, user) ==> %{
    host: user.host ?? defaults.host,
    port: user.port ?? defaults.port,
    debug: user.debug ?? defaults.debug,
    name: user.name ?? defaults.name,
};

cfg := resolve(defaults, user_opts);
print(cfg);
// host localhost, port 3000, debug true, name app
```

**What this does**

1. Each field prefers the user value when present and non-nil.
2. Missing user fields fall back through `??`.
3. The result is a complete config map ready for the rest of the program.

---

### Walkthrough 6 — State machine with enums

**Goal:** Model a door that can be opened, closed, or locked, and reject illegal transitions.

```vela
enum Door { Closed, Open, Locked }

transition(state, action) ==> match [state, action] {
    [Door.Closed, "open"]  => Door.Open,
    [Door.Closed, "lock"]  => Door.Locked,
    [Door.Open,   "close"] => Door.Closed,
    [Door.Locked, "unlock"] => Door.Closed,
    _ => {
        throw "illegal transition";
        -> state;   // unreachable, keeps type shape clear in spirit
    },
};

// Safer version: return a result map instead of throwing
try_transition(state, action) ==> {
    try {
        next := match [state, action] {
            [Door.Closed, "open"]   => Door.Open,
            [Door.Closed, "lock"]   => Door.Locked,
            [Door.Open,   "close"]  => Door.Closed,
            [Door.Locked, "unlock"] => Door.Closed,
            _ => nil,
        };
        when next == nil {
            -> %{ok: false, state: state, error: "illegal"};
        }
        -> %{ok: true, state: next, error: nil};
    } catch e {
        -> %{ok: false, state: state, error: e};
    }
};

d <- Door.Closed;
print(try_transition(d, "open"));    // ok, Open
d <- Door.Open;
print(try_transition(d, "lock"));    // illegal from Open
```

**What this does**

1. Enum variants are the only legal door states.
2. Matching on a two-element array pairs current state with an action string.
3. The `try_transition` form returns a structured result so callers can handle failure without an exception if they prefer.

---

### Walkthrough 7 — Grouping and reporting

**Goal:** Group scores by pass/fail and print a short report.

```vela
scores := [42, 81, 77, 55, 90, 61, 39, 88];

bucket(n) ==> n >= 60 ? "pass" : "fail";
groups := group_by(scores, bucket);

print("passes", groups.pass);
print("fails", groups.fail);

print("pass count", len(groups.pass ?? []));
print("fail count", len(groups.fail ?? []));

avg(xs) ==> {
    when len(xs) == 0 { -> 0; }
    -> sum(xs) / len(xs);
};
print("pass average", avg(groups.pass ?? []));
```

**What this does**

1. `group_by` builds a map from key strings to arrays of items.
2. Keys here are `"pass"` and `"fail"` from the ternary inside `bucket`.
3. `?? []` protects against a missing group when every score fell in the other bucket.

---

### Walkthrough 8 — Recursive directory-style paths (logical)

**Goal:** Normalize a list of path segments and join them (no real filesystem required).

```vela
join_path(parts) ==> {
    // Drop empty segments
    cleaned := filter(parts, (p) ==> len(p) > 0);
    // Join with /
    out <- "";
    i <- 0;
    repeat seg in cleaned {
        when i == 0 {
            out <- seg;
        }
        else {
            out <- out + "/" + seg;
        }
        i <- i + 1;
    }
    -> out;
};

print(join_path(["", "home", "user", "docs"]));
// home/user/docs

// Depth of a nested array (ragged tree)
depth(v) ==> match v {
    [h, ...t] => {
        left := depth(h);
        right := depth(t);
        -> 1 + (left > right ? left : right);
    },
    _ => 0,
};
print(depth([1, [2, [3]], 4]));
```

---

### Walkthrough 9 — Input, validation, and retry logic (conceptual)

**Goal:** Read a number in a range; on failure, explain and use a default (as you would in a REPL or script).

```vela
read_score(prompt, lo, hi, fallback) ==> {
    raw := readline(prompt);
    when raw == nil {
        -> fallback;
    }
    n := num(raw);
    when n == nil {
        print("not a number, using", fallback);
        -> fallback;
    }
    when n < lo || n > hi {
        print("out of range, using", fallback);
        -> fallback;
    }
    -> n;
};

// In an interactive run:
// score := read_score("Score 0-100: ", 0, 100, 0);
// print("recorded", score);
```

**What this does**

1. `readline` returns a string or `nil` at end-of-input.
2. `num` converts text; failed conversion yields `nil` in typical use as a signal.
3. Range checks use logical `||`.
4. Every failure path still returns a usable number so the caller stays simple.

---

### Walkthrough 10 — Mini student roster

**Goal:** Store students as records, filter by grade, and print a ranked summary.

```vela
record Student { name: str, score: num, }

alice := new Student{name: "Alice", score: 91};
bob   := new Student{name: "Bob",   score: 74};
cara  := new Student{name: "Cara",  score: 88};
drew  := new Student{name: "Drew",  score: 62};

roster := [alice, bob, cara, drew];

grade_letter(score) ==> match {
    score >= 90 => "A",
    score >= 80 => "B",
    score >= 70 => "C",
    score >= 60 => "D",
    _ => "F",
};

print("Roster");
repeat s in roster {
    print(s.name, s.score, grade_letter(s.score));
}

// Students with B or better
strong := filter(roster, (s) ==> s.score >= 80);
print("strong count", len(strong));
repeat s in strong {
    print(" *", s.name);
}

// Class average
total <- 0;
repeat s in roster {
    total <- total + s.score;
}
avg := total / len(roster);
print("average", avg, "class grade", grade_letter(avg));
```

**What this does**

1. `record` defines a reusable shape; `new` fills fields.
2. `grade_letter` is a subjectless match: conditions are tested in order.
3. `filter` keeps whole student values, so later loops still have `.name` and `.score`.
4. Aggregation uses a mutable accumulator, then a final immutable `avg`.

---

## How to read these examples

Across the walkthroughs, the same habits repeat:

| Habit | Where you see it |
|-------|------------------|
| Immutable by default | `:=` for values that do not change |
| Mutable when needed | `<-` for counters, accumulators, current state |
| Small pure functions | `to_f`, `square`, `grade_letter` |
| Data then transform | arrays/maps built first, then `map` / `filter` / `match` |
| Explicit failure | `throw` or structured `%{ok: false, ...}` |
| Match for decisions | enums, grades, HTTP-like codes, shape tags |

That combination is the everyday style the language is built for: clear names, clear updates, and structured values flowing through functions and matches.
