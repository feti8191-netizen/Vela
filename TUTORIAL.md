# Vela — Complete Language Tutorial

This tutorial teaches **the whole language**.  
In every section you get:

1. **What it is for** (why the feature exists)  
2. **How it works** (the real rules)  
3. **What goes wrong** (common mistakes)  
4. **A short example** you can run  

You do not need to finish everything in one day. Follow the parts in order if you are new: **A → B → C → D**, then jump to **H** when you want to draw.

Vela is **interpreted**: you write text in a `.vel` file (or the REPL), and `./vela` runs it immediately. There is no separate compile step.

```bash
make
./vela                  # interactive REPL
./vela my_program.vel   # run a file
```

**Tip:** Keep a terminal open. After each example, paste it into a file (or the REPL) and run it so the idea sticks.

---

# Part A — Foundations

## A1. What kind of language Vela is

Vela is a **general-purpose scripting language** with:

- **Dynamic types** — a name can hold a number, string, array, etc.; the kind is known at runtime.
- **Explicit binding** — creating a name and changing a name use different operators (`:=` vs `<-`).
- **Lexical functions** — functions close over the environment where they were defined.
- **Its own syntax** — keywords and shapes are not copied from Python or JavaScript.

A program is a list of **statements** executed from top to bottom. Statements may contain **expressions** (things that produce a value).

---

## A2. Comments and layout

```vela
// Everything after // on a line is ignored.
print("still runs");
```

- Statements normally end with **`;`**.
- Blocks use **`{` … `}`**.
- Whitespace is mostly free; indentation is for humans, not enforced by the language.

---

## A3. Tokens — what the language actually reads

When Vela opens your file, the **lexer** cuts the text into **tokens**. If you understand tokens, parse errors become easier to read.

### Keywords (reserved)

These words always mean syntax, never a normal variable name:

| Keywords | Purpose |
|----------|---------|
| `when` `else` `unless` | Conditions |
| `loop` `repeat` `until` | Loops |
| `from` `to` `through` `by` `in` `times` | Pieces of `repeat` / membership |
| `stop` `bail` `next` `skip` | Loop control |
| `and` `or` `not` | Logic (same as `&&` `\|\|` `!`) |
| `is` | Type / state tests |
| `try` `catch` `throw` `assert` | Errors |
| `defer` | Cleanup on scope exit |
| `match` | Pattern matching |
| `record` `new` `enum` `trait` `impl` | Structured types |
| `import` `as` | Modules |
| `true` `false` `nil` | Literal values |

**Important:** because `from`, `to`, `by`, `new`, `in`, … are keywords, you **cannot** use them as parameter names:

```vela
// BAD:  replace_all(s, from, to) ==> ...
// GOOD: replace_all(s, src, dst) ==> ...
```

### Operators and punctuation

| Token | Role |
|-------|------|
| `:=` | Bind a **fixed** name |
| `<-` | Bind or update a **changeable** name |
| `==>` | Start a function body |
| `->` | Return a value from a function |
| `\|>` | Pipe: send left value into a call |
| `+ - * / % **` | Arithmetic (`**` is power) |
| `== != < <= > >=` | Comparison |
| `&& \|\| !` | Logic |
| `.` | Field or method access |
| `[]` | Indexing or array literal |
| `%{ }` | Map literal |
| `#{ }` | Set literal |
| `()` | Grouping and function calls |
| `,` `;` | List separator / end of statement |

### Literals (raw values in source)

- Numbers: `42`, `3.14`
- Strings: `"hello"` (supports `${expr}` interpolation)
- Booleans: `true`, `false`
- Nothing: `nil`
- Array: `[1, 2, 3]`
- Map: `%{key: value}`
- Set: `#{1, 2}`

```vela
print(42);
print("hi");
print([1, 2]);
print(%{a: 1});
print(#{1, 2});
```

---

## A4. Names: `:=` and `<-` (critical)

Vela forces you to say whether a name is **fixed** or **changeable**.

| Syntax | Meaning |
|--------|---------|
| `name := expression` | Create `name` **once**. It cannot be assigned again with `<-`. Creating it again with `:=` in the same scope is an error. |
| `name <- expression` | Create `name` as **mutable**, or **update** it if it already exists as mutable. |

```vela
title := "Vela";
count <- 0;
count <- count + 1;
print(title, count);
```

Why this exists:

- Bugs from accidental reassignment are harder when constants are truly fixed.
- Readers can see intent: `:=` means “this should not change.”

**In plain words:**

- `:=` is “make a name and lock it.”  
- `<-` is “make a name I can update” or “update that name.”

Rules to memorize:

1. Prefer `:=` for values that should stay the same (names, titles, fixed config).  
2. Use `<-` for counters, accumulators, flags, loop state.  
3. Do not mix: `x := 1` then `x <- 2` is an error.  
4. Do not re-`:=` the same name in the same scope.

If the language says *cannot change 'x' — it was created with :=*, you chose the wrong operator when `x` was first created. Create it with `<-` if it must change later.

---

## A5. Values and `type` / `is`

Every value has a **kind** at runtime. Common kinds: `nil`, `num`, `str`, `bool`, array, map, set, function, record instance, …

```vela
print(type(3));
print(type("a"));
print(type([1]));
print(3 is num);
print("" is empty);
print(nil is nil);
```

`is` is for questions like “what kind of thing is this?” or “is it empty?” — not for comparing two numbers (use `==` for that).

---

## A6. Print, say, input

```vela
print("value:", 42);
say("say is an alias of print");
```

Reading a line from the user:

```vela
// line := readline("Name: ");
// line := ask("Name: ");   // same thing
```

Turning text into a number **safely**:

| Function | Behavior |
|----------|----------|
| `try_num(text)` | Returns a number, or `nil` if the text is not a number |
| `num(text)` | Returns a number, or **throws** on bad input |

```vela
n := try_num("12");
print(n);
bad := try_num("nope");
print(bad);   // nil
```

**Never** do `try_num(input) + 1` without checking `nil` — math on `nil` is a runtime error.

---

## A7. Operators

Arithmetic and comparison work on numbers; `+` also concatenates strings.

```vela
print(2 + 3 * 4);
print(2 ** 10);
print(7 % 3);
print(1 < 2);
print("a" + "b");
```

Logic:

```vela
print(true and not false);
print(false or true);
print(true && true);   // same as and
```

Membership:

```vela
print(2 in [1, 2, 3]);
print("x" in %{x: 1});
```

Division or modulo by zero raises a runtime error.

### Mini checklist after Part A

- [ ] I can create a fixed name with `:=` and a counter with `<-`  
- [ ] I know `try_num` can return `nil`  
- [ ] I can print values and build a string with `${...}`  
- [ ] I know keywords cannot be used as parameter names  

---

# Part B — Control flow

## B1. `when` / `else when` / `else`

`when` is how Vela makes a **decision**.

**What it is for:** run one block of code only if a condition is true; optionally handle other cases with `else when` and `else`.

It **always** needs a condition expression before `{`.  
Think of it as: *when this is true, do that.*

```vela
x := -3;
when x > 0 {
    print("positive");
}
else when x < 0 {
    print("negative");
}
else {
    print("zero");
}
```

**How the chain works:**

1. Test the first `when`. If true, run its block and skip the rest of the chain.  
2. Otherwise test each `else when` in order.  
3. If none matched and there is `else`, run `else`.

What goes wrong for beginners:

- Writing `when { ... }` with no condition → parse error.  
- Code **after** a `when` block still runs unless it is inside that block. Vela does not stop the whole program just because a `when` finished.  
- Chain with `else when`, not a second bare `when`, if you want **only one** branch to run.  
- Comparing strings and numbers needs the same kind on both sides (`==` on two strings, or two numbers).

## B2. `unless`

`unless condition { body }` means “run body when condition is **false**.”

```vela
ready := false;
unless ready {
    print("still setting up");
}
```

## B3. `loop`, `stop`, `next`

**What it is for:** keep doing something until you decide to leave — menus, games, “read input until quit”, and so on.

`loop { ... }` repeats **forever** until `stop` (or `bail`) runs.

| Word | Alias | Meaning |
|------|-------|---------|
| `stop` | `bail` | Leave the loop entirely |
| `next` | `skip` | Skip the rest of this iteration, continue looping |

```vela
n <- 0;
loop {
    n <- n + 1;
    when n == 2 { next; }
    when n > 3 { stop; }
    print(n);
}
```

**How to read this example:**

- Each time around, `n` grows by 1.  
- When `n` is 2, `next` jumps to the next round (nothing is printed for 2).  
- When `n` is greater than 3, `stop` ends the loop.  
- So the printed values are 1 and 3.

Using `stop` or `next` **outside** any loop is a runtime error.

**Pattern — read until the user quits:**

Put `readline` / `ask` **inside** the loop so each turn asks again. If you ask once *before* the loop, the same answer is reused forever.

## B4. `repeat` forms

`repeat` is the structured loop.

**Inclusive range with `through`:**

```vela
repeat i from 0 through 3 {
    print(i);
}
```

**Range with step `by`:**

```vela
repeat i from 0 to 10 by 2 {
    print(i);
}
```

**Over a collection:**

```vela
repeat item in ["a", "b"] {
    print(item);
}
```

**Fixed count:**

```vela
repeat 3 times {
    print("tick");
}
```

## B5. `until`

```vela
n <- 0;
until n >= 3 {
    print(n);
    n <- n + 1;
}
```

Runs the body until the condition becomes true (condition checked like a negated loop guard).

---

# Part C — Functions

## C1. Declaring and calling

**What it is for:** give a name to a reusable piece of work so you do not copy the same code.

```vela
add(a, b) ==> a + b;
print(add(2, 3));
```

**Pieces of a function declaration:**

1. **Name** — `add`  
2. **Parameters** — `(a, b)` — names for incoming values  
3. **`==>`** — “here is the body”  
4. **Body** — either one expression (`a + b`) or a block `{ ... }`

Call with `name(arg1, arg2)`. Arguments are matched left to right.

## C2. Block body and `->`

```vela
abs_val(x) ==> {
    when x < 0 { -> -x; }
    else { -> x; }
};
print(abs_val(-5));
```

`-> expression` returns from the **current function** immediately.

## C3. Default parameters

```vela
greet(name, greeting := "Hi") ==> {
    print("${greeting}, ${name}");
};
greet("Ada");
greet("Ada", "Hello");
```

Defaults are evaluated when the argument is omitted. Default parameters must be trailing.

## C4. Pipe `|>`

```vela
double(n) ==> n * 2;
print(5 |> double);
```

`x |> f` means `f(x)`. For multi-arg calls, the piped value is inserted as a leading argument according to language rules.

---

# Part D — Collections and strings

## D1. Arrays

**What it is for:** an ordered list of values — scores, lines of text, points, anything in sequence.

```vela
xs := [10, 20, 30];
print(xs[0]);      // first element — index starts at 0
print(len(xs));    // how many elements
push(xs, 40);      // add at the end (array grows)
print(xs);
```

**Index:** `xs[0]` is the first item, `xs[1]` the second. Asking for an index outside the list is an error.

Useful builtins: `push`, `pop`, `map`, `filter`, `reduce`, `sum`, `sorted`, `reverse`, `unique`, `flatten`, `join`, `zip`, `chunks`, `windows`, …

## D2. Maps

```vela
person := %{name: "Ada", age: 36};
print(person.name);
print(person["age"]);
print(keys(person));
print(has(person, "name"));
```

Bare keys in `%{name: ...}` become string keys.

## D3. Sets

```vela
s := #{1, 2, 3};
print(set_has(s, 2));
print(set_union(s, #{3, 4}));
```

## D4. Strings

```vela
print(len("abc"));
print(upper("hi"));
print(contains("vela", "el"));
print(split("a,b,c", ","));
print(join(["a", "b"], "-"));
```

**Interpolation:** inside a string, `${expression}` inserts the value of that expression.

```vela
name := "Vela";
print("Hello, ${name}!");
print("2 + 2 = ${2 + 2}");
```

This is clearer than many `+` concatenations when you build messages.

---

# Part E — Modules and standard library

## E1. Import

```vela
import "std/math.vel" as M;
print(M.clamp(15, 0, 10));
```

- Path is relative to the file doing the import (or as resolved by the interpreter).
- `as Name` puts exports on an object-like namespace `Name`.

## E2. What ships in `std/`

| Module | Role |
|--------|------|
| `std/str.vel` | upper, lower, strip, starts/ends, replace_all, … |
| `std/list.vel` | head, tail, take, drop, uniq, flat, … |
| `std/math.vel` | clamp, sign, lerp, in_range, even, odd, … |
| `std/util.vel` | default, coalesce, repeat_val, identity |
| `std/map.vel` | get, put, has_key, … |
| `std/set.vel` | add, remove, unite, common, without |
| `std/json.vel` | parse, stringify |
| `std/io.vel` | read, write, append, exists, list, remove |
| `std/path.vel` | join2, basename, dirname, extension |
| `std/check.vel` | assert_true, assert_eq, assert_nil |
| `std/art.vel` | blob, ring, ngon, star, pick_ink |
| `std/stats.vel` | mean, min_of, max_of, sum_sq, count |
| `std/text.vel` | lines, words, count_char |
| `std/rand.vel` | unit, int_between, pick |
| … | counter, stack, queue, pair, result, iter, fmt, bag, … |

Use `std/check.vel` to write small tests in Vela itself.

```vela
import "std/check.vel" as C;
C.assert_eq(2 + 2, 4);
print("checks ok");
```

---

# Part F — Errors

```vela
try {
    throw "something went wrong";
} catch e {
    print("caught:", e);
}

try {
    print(1 / 0);
} catch {
    print("failed without binding the error");
}
```

- `throw` raises a value.  
- `catch e` binds it; `catch { }` ignores the value.  
- Many builtins throw on invalid use (bad types, missing files, division by zero).

---

# Part G — Records, enums, match (structured data)

## G1. Records

**What it is for:** a value with named fields that belong together (a point, a user, a settings object).

```vela
record Point {
    x: num,
    y: num,
}

p := new Point{x: 3, y: 4};
print(p.x, p.y);
```

**How to read this:**

1. `record Point { ... }` declares the shape.  
2. `new Point{x: 3, y: 4}` builds one value of that shape.  
3. `p.x` reads field `x`.

Methods can be attached to records (see method examples under `examples/`). For learning, fields alone are enough: store data, read it back, pass the whole record into functions.

## G2. Enums and match

**Enums** define a fixed set of named variants (for example colors or states).

**`match`** picks a branch by looking at a value’s tag or structure.

Idea in plain words:

- You write several **patterns**.  
- The first pattern that fits the value runs its branch.  
- Patterns can bind names to pieces inside arrays, maps, or enum payloads.

Full pattern demos live in the numbered `examples/` files. When you read them, ask: *what shape does this pattern expect?* and *which names does it bind?*

---

# Part H — Visual art (full drawing system)

Vela can **draw images** without a browser and without p5.js.  
You build a picture in memory, then save it as a file (`.bmp` or `.ppm`) and open it on your computer.

## H1. Mental model

Preferred **Vela-style** form — a `draw` block with indentation:

```vela
draw 300 by 200 {
    paper(26, 26, 46);
    paint(233, 69, 96);
    no_edge();
    circle(150, 100, 80);
}
print(save_art("pic.bmp"));
```

What this means:

1. **`draw W by H { ... }`** — create a canvas of that size, then run the body.  
2. **`paper(...)`** — paint the whole background (alias of `ground`).  
3. **`paint(...)`** — set the fill color (alias of `ink`).  
4. Shapes inside the block use the current style.  
5. **`save_art`** after the block writes the file.

You can still use `canvas(w, h)` + `ground` / `ink` if you prefer the older call style.

If you draw before any canvas/`draw`, you get: `no canvas — call canvas(width, height) first`.

**Coordinates:** `x` goes right, `y` goes down. `(0, 0)` is the **top-left** corner.

## H2. Creating and clearing

```vela
canvas(300, 200);          // width 300, height 200
ground("#1a1a2e");         // fill entire canvas with a color
// or:
// gradient("#000000", "#3333aa");   // vertical top → bottom
// gradient_h("#ff0000", "#0000ff"); // horizontal left → right
print(art_width(), art_height());
```

Limits: each dimension must be between 1 and 8192.

## H3. Colors (kept simple)

Pick **one** style and stay consistent:

| Style | Example | Notes |
|-------|---------|--------|
| **RGB numbers** | `paint(233, 69, 96)` | Clearest for many people |
| **`rgb` helper** | `paint(rgb(233, 69, 96))` | Same values, as one color value |
| **Hex string** | `paint("#e94560")` | Good when you copy from a design tool |
| **Gray** | `paper(32)` | Single number 0–255 |

Background and fill words:

| Word | Meaning |
|------|---------|
| `paper(...)` or `ground(...)` | Clear the whole canvas to that color |
| `paint(...)` or `ink(...)` | Set **fill** for the next shapes |
| `edge(...)` | Set **outline** color |
| `edge_w(n)` | Outline thickness in pixels |

```vela
draw 200 by 100 {
    paper(20, 20, 30);
    paint(233, 69, 96);
    edge(255, 255, 255);
    edge_w(2);
    circle(100, 50, 40);
}
```

## H4. Fill and stroke switches

| Call | Effect |
|------|--------|
| `ink(color)` | Set fill color and **enable** fill |
| `edge(color)` | Set stroke color and **enable** stroke |
| `no_ink()` | Shapes will not fill interiors |
| `no_edge()` | Shapes will not draw outlines |

Default style is usable after `canvas`, but art sketches usually set `ground` and `ink` explicitly.

## H5. Shapes — what each one means

### Point — `dot(x, y)`

Puts a point at pixel `(x, y)`. Uses stroke (or fill if you only set ink). Thickness follows `edge_w`.

### Line — `line(x1, y1, x2, y2)`

Straight segment from first point to second. Uses **edge** color and `edge_w`.

### Rectangle — `box(x, y, w, h)`

Axis-aligned rectangle. `(x,y)` is the **top-left** corner. `w` and `h` are width and height.  
Fill uses `ink` if fill is enabled; border uses `edge` if stroke is enabled.

### Circle — `circle(cx, cy, size)`

Center `(cx, cy)`. **`size` is the diameter** (not radius).

So `circle(100, 100, 50)` is 50 pixels **across** (radius 25).  
This is a common mistake if you are used to APIs that take a radius — in Vela, size means full width of the circle.

### Ellipse — `ellipse(cx, cy, width, height)`

Center + full width and height of the bounding box.

### Triangle — `tri(x1,y1, x2,y2, x3,y3)`

Three corners. Can be filled and/or stroked.

### Polygon — `poly([[x,y], [x,y], ...])`

Three or more points. Filled with a fan triangulation; outline connects the edges in order.

## H6. Pixels and noise

```vela
canvas(64, 64);
art_pixel(10, 10, "#ff0000");   // one pixel
n := noise(3.2, 4.5);           // smooth value in [0, 1)
print(n);
```

`noise(x)` or `noise(x, y)` is deterministic value noise — good for textures and terrain-like patterns.

## H7. Saving files

```vela
canvas(20, 20);
ground("#222222");
path := save_art("picture.bmp");
print(path);
```

| Path ending | Format | Notes |
|-------------|--------|--------|
| `.bmp` | Windows BMP | Best default — opens almost everywhere |
| `.ppm` | Portable Pixmap | Simple format; some tools prefer it |

**About PNG:** if you pass a name ending in `.png`, Vela still writes **BMP data** (often using a `.bmp` name). Real PNG encoding is not in v1. Prefer `.bmp` so the file extension matches the contents.

`save_art` returns the path string of the file it wrote.  
The file appears in the **current working directory** (the folder where you ran `./vela`), unless you pass a full path.

## H8. Helpers in `std/art.vel`

```vela
import "std/art.vel" as A;

canvas(200, 200);
ground("#0a0a12");
A.blob(100, 100, 40, "#e94560");     // filled circle, no edge
A.ring(100, 100, 80, "#ffffff", 2);  // stroke only
// A.ngon(cx, cy, r, sides, rotation)
// A.star(cx, cy, r_outer, r_inner, points)
// A.checker(cell, color0, color1)
// A.bar(x, width, height, color)
```

## H9. Generative pattern (how control flow + art combine)

```vela
canvas(400, 400);
ground("#050505");
no_edge();
ink("#00d2ff");
a <- 0;
repeat i from 0 through 150 {
    x := 200 + cos(a) * (i * 0.8);
    y := 200 + sin(a) * (i * 0.8);
    circle(x, y, 5);
    a <- a + 0.2;
}
print(save_art("spiral.bmp"));
```

Here **ordinary Vela** (`repeat`, `<-`, `cos`/`sin`) drives drawing. That is the intended style — not a hidden “draw loop” like p5’s `draw()`.

## H10. Art checklist

1. `canvas` before any draw  
2. `ground` or `gradient` so the background is intentional  
3. Set `ink` / `edge` / `no_ink` / `no_edge` before each kind of shape  
4. Remember **circle size = diameter**  
5. `save_art` at the end  
6. Open the **`.bmp`** (or `.ppm`) on your computer  

---

# Part I — Built-in functions (by category)

These are always available without `import`.

### Console and program

`print`, `say`, `eprint`, `readline`, `ask`, `exit`, `args`, `platform`, `cwd`, `chdir`, `sleep`, `now`, `getenv`, `setenv`, `cpu_time`, `mem_usage`, `stack_trace`, `call_depth`, `set_max_depth`

### Types and conversion

`type`, `str`, `num`, `try_num`, `bool`

### Math

`abs`, `floor`, `ceil`, `round`, `sign`, `sqrt`, `pow`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `log`, `log2`, `log10`, `exp`, `min`, `max`, `gcd`, `lcm`, `is_prime`, `random`

### Strings

`len` (also arrays/maps), `upper`/`to_upper`, `lower`/`to_lower`, `trim`, `split`, `join`, `contains`, `starts_with`, `ends_with`, `index_of`, `replace`, `substr`, `chars`, `pad_left`, `pad_right`, `hex_encode`

### Collections

`push`, `pop`, `map`, `filter`, `reduce`, `sum`, `sorted`, `reverse`, `unique`, `flatten`, `zip`, `enumerate`, `chunks`, `windows`, `partition`, `group_by`, `all`, `any`, `find`, `range`, `keys`, `values`, `has`

### Sets

`set`, `set_add`, `set_remove`, `set_has`, `set_union`, `set_intersect`, `set_difference` (and related)

### Files and process

`read_file`, `write_file`, `append_file`, `file_exists`, `remove_file`, `list_dir`, `run`

### JSON and regex

`json_parse`, `json_stringify`, `re_match`, `re_find_all`, `re_replace`

### Art

`canvas`, `ground`, `gradient`, `ink`, `edge`, `edge_w`, `no_ink`, `no_edge`, `dot`, `line`, `box`, `circle`, `ellipse`, `tri`, `poly`, `noise`, `art_pixel`, `art_width`, `art_height`, `save_art`

Each expects the argument kinds described in earlier sections; wrong kinds produce a **runtime error** with a message.

---

# Part J — REPL

```text
./vela
vela> 1 + 2
vela> .help
vela> .vars
vela> .type [1, 2]
vela> .load examples/01_hello.vel
vela> .clear
vela> exit
```

Multi-line input: if braces are still open, the prompt continues until the block is finished.

---

# Part K — Common errors (and what they mean)

| Message / situation | Cause | Fix |
|---------------------|-------|-----|
| `when needs a condition before '{'` | `when { }` | Write `when cond { }` |
| `cannot use := on 'x' again` | Second `:=` in same scope | Use `<-` if it must change; or pick a new name |
| `cannot change 'x' — it was created with :=` | `<-` on fixed name | Create with `<-` from the start |
| `'stop' only works inside a loop` | `stop` at top level | Only inside `loop` / `repeat` |
| `cannot use '+' with nil` | Math on failed `try_num` | Check `== nil` first |
| `no canvas — call canvas...` | Shape before canvas | Call `canvas(w,h)` first |
| `expected a value ... got 'from'` | Keyword used as name | Rename parameter |
| Parse error near `;` / `{` | Missing punctuation or unclosed block | Match `{` `}` and end statements with `;` |

---

# Part L — How to practice

Do these in order if you can:

1. **Bindings** — Read Part A until `:=` / `<-` feels natural. Write five lines that only use names and `print`.  
2. **Decisions** — Write a `when` / `else when` / `else` that classifies a number.  
3. **Loops** — Write a `loop` that reads with `ask` until the user types `exit` (put `ask` inside the loop).  
4. **Functions** — Write `double` and `sum_list` and call them.  
5. **Examples folder** — Run numbered files in `examples/` in order.  
6. **Audit** — Run `examples/45_full_language_audit.vel`.  
7. **Art** — Change colors and sizes in `examples/70_art_hello.vel` … `76_art_bars.vel` and open the BMP files.  
8. **Tests** — Import `std/check.vel` and `assert_eq` your own functions.

When something fails, read the error line and caret (`^`). The message usually names the rule (binding, canvas, nil math, keyword, …).

---

# Part M — Related files

| File | Role |
|------|------|
| `TUTORIAL.md` | This full tutorial |
| `README.md` | Short start + map of the project |
| `explain.md` | Narrative language notes |
| `SPEC.md` | Working behavior specification |
| `ROADMAP.md` | What is done vs later |
| `examples/` | Runnable programs |
| `std/` | Standard modules |

Vela is large; you do not need every builtin on day one. Master **names, control flow, functions, collections, and errors** first — then art and `std/` become tools on top of the same rules.
