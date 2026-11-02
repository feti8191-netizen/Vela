// Vela Programming Language — syntax revision
// A modern, custom, general-purpose language with first-class math support.
// Implemented from scratch in C++17.
//
// Build:  g++ -std=c++17 -O2 -Wall -Wextra -pedantic -o vela vela.cpp
// Run:    ./vela program.vel
// REPL:   ./vela

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <dirent.h>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
struct Expr;
struct Stmt;
struct BlockStmt;
struct Function;
struct NativeFunction;
struct Interpreter;
struct Env;

using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;
using EnvPtr  = std::shared_ptr<Env>;

struct Param {
    std::string name;
    std::string type = "any";
    ExprPtr defaultValue; // nullptr if required
};

// ---------------------------------------------------------------------------
// Errors
// ---------------------------------------------------------------------------
struct ParseError : std::runtime_error {
    int line;
    ParseError(int line, const std::string& msg) : std::runtime_error(msg), line(line) {}
};

struct RuntimeError : std::runtime_error {
    int line;
    RuntimeError(int line, const std::string& msg) : std::runtime_error(msg), line(line) {}
};

// ---------------------------------------------------------------------------
// Tokens
// ---------------------------------------------------------------------------
enum class Tok {
    Number, String, Ident,
    When, Else, Loop, Repeat, From, To, Through, In, Bail, Skip,
    Assert, Is, Throw, Try, Catch, Record, New, Match, Import, As, Enum, Defer, By, Until, Unless,
    True, False, Nil,
    Plus, Minus, Star, Slash, Percent, StarStar,
    Amp, Pipe, Caret, Tilde, Shl, Shr,
    Bang, EqEq, BangEq, Lt, LtEq, Gt, GtEq,
    AndAnd, OrOr, NullCoal,
    PlusEq, MinusEq, StarEq, SlashEq, PercentEq,
    Define,       // :=
    Bind,         // <-
    ReturnArrow,  // ->
    DefineFunc,   // ==>
    FatArrow,     // =>
    PipeOp,       // |>
    Question, Colon, Dot, OptDot, DotDotDot,
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    MapOpen,      // %{
    SetOpen,      // #{
    Comma, Semi,
    Eof
};

struct Token {
    Tok type = Tok::Eof;
    std::string lexeme;
    double num = 0.0;
    std::string str;
    int line = 1;
};

// ---------------------------------------------------------------------------
// Lexer
// ---------------------------------------------------------------------------
class Lexer {
public:
    explicit Lexer(std::string src) : src(std::move(src)) {}

    std::vector<Token> scan() {
        std::vector<Token> out;
        while (true) {
            skipWhitespaceAndComments();
            if (pos >= src.size()) break;

            int l = line;
            char c = src[pos];

            if (std::isdigit(static_cast<unsigned char>(c))) { out.push_back(number()); continue; }
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') { out.push_back(identifier()); continue; }
            if (c == '"') { out.push_back(string()); continue; }

            pos++;
            auto emit = [&](Tok t, const std::string& lex) {
                Token tok; tok.type = t; tok.lexeme = lex; tok.line = l; out.push_back(tok);
            };

            switch (c) {
                case '+':
                    if (pos < src.size() && src[pos] == '=') { pos++; emit(Tok::PlusEq, "+="); }
                    else emit(Tok::Plus, "+");
                    break;
                case '-':
                    if (pos < src.size() && src[pos] == '>') { pos++; emit(Tok::ReturnArrow, "->"); }
                    else if (pos < src.size() && src[pos] == '=') { pos++; emit(Tok::MinusEq, "-="); }
                    else emit(Tok::Minus, "-");
                    break;
                case '*':
                    if (pos < src.size() && src[pos] == '*') { pos++; emit(Tok::StarStar, "**"); }
                    else if (pos < src.size() && src[pos] == '=') { pos++; emit(Tok::StarEq, "*="); }
                    else emit(Tok::Star, "*");
                    break;
                case '/':
                    if (pos < src.size() && src[pos] == '=') { pos++; emit(Tok::SlashEq, "/="); }
                    else emit(Tok::Slash, "/");
                    break;
                case '%':
                    if (pos < src.size() && src[pos] == '{') { pos++; emit(Tok::MapOpen, "%{"); }
                    else if (pos < src.size() && src[pos] == '=') { pos++; emit(Tok::PercentEq, "%="); }
                    else emit(Tok::Percent, "%");
                    break;
                case '#':
                    if (pos < src.size() && src[pos] == '{') { pos++; emit(Tok::SetOpen, "#{"); }
                    else throw ParseError(l, "unexpected '#' (did you mean '#{'?)");
                    break;
                case '!':
                    if (pos < src.size() && src[pos] == '=') { pos++; emit(Tok::BangEq, "!="); }
                    else emit(Tok::Bang, "!");
                    break;
                case '=':
                    if (pos < src.size() && src[pos] == '=' && pos + 1 < src.size() && src[pos + 1] == '>') {
                        pos += 2; emit(Tok::DefineFunc, "==>");
                    } else if (pos < src.size() && src[pos] == '=') {
                        pos++; emit(Tok::EqEq, "==");
                    } else if (pos < src.size() && src[pos] == '>') {
                        pos++; emit(Tok::FatArrow, "=>");
                    } else {
                        throw ParseError(l, "unexpected '=' (expected '==', '==>', '=>', ':=' or '<-')");
                    }
                    break;
                case '<':
                    if (pos < src.size() && src[pos] == '<') { pos++; emit(Tok::Shl, "<<"); }
                    else if (pos < src.size() && src[pos] == '-') { pos++; emit(Tok::Bind, "<-"); }
                    else if (pos < src.size() && src[pos] == '=') { pos++; emit(Tok::LtEq, "<="); }
                    else emit(Tok::Lt, "<");
                    break;
                case '>':
                    if (pos < src.size() && src[pos] == '>') { pos++; emit(Tok::Shr, ">>"); }
                    else if (pos < src.size() && src[pos] == '=') { pos++; emit(Tok::GtEq, ">="); }
                    else emit(Tok::Gt, ">");
                    break;
                case '&':
                    if (pos < src.size() && src[pos] == '&') { pos++; emit(Tok::AndAnd, "&&"); }
                    else emit(Tok::Amp, "&");
                    break;
                case '|':
                    if (pos < src.size() && src[pos] == '|') { pos++; emit(Tok::OrOr, "||"); }
                    else if (pos < src.size() && src[pos] == '>') { pos++; emit(Tok::PipeOp, "|>"); }
                    else emit(Tok::Pipe, "|");
                    break;
                case '^': emit(Tok::Caret, "^"); break;
                case '~': emit(Tok::Tilde, "~"); break;
                case ':':
                    if (pos < src.size() && src[pos] == '=') { pos++; emit(Tok::Define, ":="); }
                    else emit(Tok::Colon, ":");
                    break;
                case '?':
                    if (pos < src.size() && src[pos] == '?') { pos++; emit(Tok::NullCoal, "??"); }
                    else if (pos < src.size() && src[pos] == '.') { pos++; emit(Tok::OptDot, "?."); }
                    else emit(Tok::Question, "?");
                    break;
                case '.':
                    if (pos < src.size() && src[pos] == '.' && pos + 1 < src.size() && src[pos + 1] == '.') {
                        pos += 2; emit(Tok::DotDotDot, "...");
                    } else {
                        emit(Tok::Dot, ".");
                    }
                    break;
                case '(': emit(Tok::LParen, "("); break;
                case ')': emit(Tok::RParen, ")"); break;
                case '{': emit(Tok::LBrace, "{"); break;
                case '}': emit(Tok::RBrace, "}"); break;
                case '[': emit(Tok::LBracket, "["); break;
                case ']': emit(Tok::RBracket, "]"); break;
                case ',': emit(Tok::Comma, ","); break;
                case ';': emit(Tok::Semi, ";"); break;
                default:
                    throw ParseError(l, std::string("unexpected character '") + c + "'");
            }
        }
        Token eof; eof.type = Tok::Eof; eof.line = line;
        out.push_back(eof);
        return out;
    }

private:
    std::string src;
    size_t pos = 0;
    int line = 1;

    char peek(size_t off = 0) const {
        size_t p = pos + off;
        return p < src.size() ? src[p] : '\0';
    }

    void skipWhitespaceAndComments() {
        while (pos < src.size()) {
            char c = src[pos];
            if (c == ' ' || c == '\t' || c == '\r') { pos++; }
            else if (c == '\n') { line++; pos++; }
            else if (c == '/' && peek(1) == '/') {
                while (pos < src.size() && src[pos] != '\n') pos++;
            }
            else if (c == '/' && peek(1) == '*') {
                pos += 2;
                while (pos < src.size() && !(src[pos] == '*' && pos + 1 < src.size() && src[pos + 1] == '/')) {
                    if (src[pos] == '\n') line++;
                    pos++;
                }
                if (pos < src.size()) pos += 2;
                else throw ParseError(line, "unterminated block comment");
            }
            else break;
        }
    }

    Token number() {
        int l = line;
        size_t start = pos;
        while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) pos++;
        if (pos < src.size() && src[pos] == '.' && pos + 1 < src.size() &&
            std::isdigit(static_cast<unsigned char>(src[pos + 1]))) {
            pos++;
            while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) pos++;
        }
        if (pos < src.size() && (src[pos] == 'e' || src[pos] == 'E')) {
            size_t save = pos;
            pos++;
            if (pos < src.size() && (src[pos] == '+' || src[pos] == '-')) pos++;
            if (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) {
                while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) pos++;
            } else {
                pos = save;
            }
        }
        std::string text = src.substr(start, pos - start);
        Token t;
        t.type = Tok::Number;
        t.lexeme = text;
        t.num = std::stod(text);
        t.line = l;
        return t;
    }

    Token identifier() {
        int l = line;
        size_t start = pos;
        while (pos < src.size() &&
               (std::isalnum(static_cast<unsigned char>(src[pos])) || src[pos] == '_')) pos++;
        std::string text = src.substr(start, pos - start);

        Tok type = Tok::Ident;
        if      (text == "when")    type = Tok::When;
        else if (text == "else")    type = Tok::Else;
        else if (text == "loop")    type = Tok::Loop;
        else if (text == "repeat")  type = Tok::Repeat;
        else if (text == "from")    type = Tok::From;
        else if (text == "to")      type = Tok::To;
        else if (text == "through") type = Tok::Through;
        else if (text == "in")      type = Tok::In;
        else if (text == "bail")    type = Tok::Bail;
        else if (text == "skip")    type = Tok::Skip;
        else if (text == "assert")  type = Tok::Assert;
        else if (text == "is")      type = Tok::Is;
        else if (text == "throw")   type = Tok::Throw;
        else if (text == "try")     type = Tok::Try;
        else if (text == "catch")   type = Tok::Catch;
        else if (text == "record")  type = Tok::Record;
        else if (text == "new")     type = Tok::New;
        else if (text == "match")   type = Tok::Match;
        else if (text == "import")  type = Tok::Import;
        else if (text == "as")      type = Tok::As;
        else if (text == "enum")    type = Tok::Enum;
        else if (text == "defer")   type = Tok::Defer;
        else if (text == "by")      type = Tok::By;
        else if (text == "until")   type = Tok::Until;
        else if (text == "unless")  type = Tok::Unless;
        else if (text == "true")    type = Tok::True;
        else if (text == "false")   type = Tok::False;
        else if (text == "nil")     type = Tok::Nil;

        Token t; t.type = type; t.lexeme = text; t.line = l;
        return t;
    }

    Token string() {
        int l = line;
        pos++; // opening quote
        std::string out;
        while (pos < src.size()) {
            char c = src[pos++];
            if (c == '"') {
                Token t; t.type = Tok::String; t.str = out; t.lexeme = out; t.line = l;
                return t;
            }
            if (c == '\\') {
                if (pos >= src.size()) break;
                char e = src[pos++];
                switch (e) {
                    case 'n':  out += '\n'; break;
                    case 't':  out += '\t'; break;
                    case 'r':  out += '\r'; break;
                    case '\\': out += '\\'; break;
                    case '"':  out += '"';  break;
                    case '0':  out += '\0'; break;
                    default:   out += e;    break;
                }
            } else {
                if (c == '\n') line++;
                out += c;
            }
        }
        throw ParseError(l, "unterminated string literal");
    }
};

// ---------------------------------------------------------------------------
// Values
// ---------------------------------------------------------------------------
struct Value {
    enum class Kind { Nil, Num, Str, Bool, Arr, Map, Range, Func, Native };

    Kind kind = Kind::Nil;
    double n = 0.0;
    bool b = false;
    std::string s;
    std::shared_ptr<std::vector<Value>> arr;
    std::shared_ptr<std::map<std::string, Value>> map;
    double rangeStart = 0.0, rangeEnd = 0.0, rangeStep = 1.0;
    bool rangeIncl = false;
    std::shared_ptr<Function> func;
    std::shared_ptr<NativeFunction> native;

    Value() = default;

    static Value Nil();
    static Value Num(double v);
    static Value Str(std::string v);
    static Value Bool(bool v);
    static Value Arr(std::shared_ptr<std::vector<Value>> v);
    static Value Map(std::shared_ptr<std::map<std::string, Value>> v);
    static Value Range(double a, double b, bool inc, double step = 1.0);
    static Value Func(std::shared_ptr<Function> f);
    static Value Native(std::shared_ptr<NativeFunction> f);

    bool isNil() const { return kind == Kind::Nil; }
    std::string toString(bool nested = false) const;
};

Value Value::Nil() { return Value(); }
Value Value::Num(double v) { Value x; x.kind = Kind::Num; x.n = v; return x; }
Value Value::Str(std::string v) { Value x; x.kind = Kind::Str; x.s = std::move(v); return x; }
Value Value::Bool(bool v) { Value x; x.kind = Kind::Bool; x.b = v; return x; }
Value Value::Arr(std::shared_ptr<std::vector<Value>> v) {
    Value x; x.kind = Kind::Arr; x.arr = std::move(v); return x;
}
Value Value::Map(std::shared_ptr<std::map<std::string, Value>> v) {
    Value x; x.kind = Kind::Map; x.map = std::move(v); return x;
}
Value Value::Range(double a, double b, bool inc, double step) {
    Value x; x.kind = Kind::Range; x.rangeStart = a; x.rangeEnd = b; x.rangeIncl = inc; x.rangeStep = step == 0 ? 1.0 : step; return x;
}
Value Value::Func(std::shared_ptr<Function> f) {
    Value x; x.kind = Kind::Func; x.func = std::move(f); return x;
}
Value Value::Native(std::shared_ptr<NativeFunction> f) {
    Value x; x.kind = Kind::Native; x.native = std::move(f); return x;
}

struct NativeFunction {
    std::function<Value(std::vector<Value>&)> fn;
};

// ---------------------------------------------------------------------------
// Environment
// ---------------------------------------------------------------------------
struct Env {
    EnvPtr parent;
    std::unordered_map<std::string, Value> vars;
    std::unordered_set<std::string> consts;

    explicit Env(EnvPtr p = nullptr) : parent(std::move(p)) {}

    void define(const std::string& name, const Value& v, bool isConst) {
        vars[name] = v;
        if (isConst) consts.insert(name);
        else         consts.erase(name);
    }

    Value* lookup(const std::string& name) {
        Env* e = this;
        while (e) {
            auto it = e->vars.find(name);
            if (it != e->vars.end()) return &it->second;
            e = e->parent.get();
        }
        return nullptr;
    }

    Env* owner(const std::string& name) {
        Env* e = this;
        while (e) {
            if (e->vars.find(name) != e->vars.end()) return e;
            e = e->parent.get();
        }
        return nullptr;
    }
};

// ---------------------------------------------------------------------------
// Control-flow signals
// ---------------------------------------------------------------------------
struct ReturnSignal   { Value value; };
struct BreakSignal    {};
struct ContinueSignal {};
struct ErrorSignal    { Value value; };

// ---------------------------------------------------------------------------
// Expression AST
// ---------------------------------------------------------------------------
struct Expr {
    int line = 1;
    virtual ~Expr() = default;
    virtual Value eval(const EnvPtr& env, Interpreter& I) = 0;
};

struct LiteralExpr : Expr {
    Value value;
    LiteralExpr(Value v, int l) : value(std::move(v)) { line = l; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

struct VarExpr : Expr {
    std::string name;
    VarExpr(std::string n, int l) : name(std::move(n)) { line = l; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

struct AssignExpr : Expr {
    ExprPtr target;
    std::string op;
    ExprPtr value;
    AssignExpr(ExprPtr t, std::string o, ExprPtr v, int l)
        : target(std::move(t)), op(std::move(o)), value(std::move(v)) { line = l; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

struct BinaryExpr : Expr {
    std::string op;
    ExprPtr left, right;
    BinaryExpr(std::string o, ExprPtr l, ExprPtr r, int ln)
        : op(std::move(o)), left(std::move(l)), right(std::move(r)) { line = ln; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

struct UnaryExpr : Expr {
    std::string op;
    ExprPtr operand;
    UnaryExpr(std::string o, ExprPtr e, int l)
        : op(std::move(o)), operand(std::move(e)) { line = l; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

struct CallArg {
    std::string name; // empty if positional
    ExprPtr expr;
};

struct CallExpr : Expr {
    ExprPtr callee;
    std::vector<CallArg> args;
    CallExpr(ExprPtr c, std::vector<CallArg> a, int l)
        : callee(std::move(c)), args(std::move(a)) { line = l; }
    // convenience for positional-only construction
    CallExpr(ExprPtr c, std::vector<ExprPtr> a, int l) : callee(std::move(c)) {
        line = l;
        for (auto& e : a) args.push_back({"", std::move(e)});
    }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

struct IndexExpr : Expr {
    ExprPtr object, index;
    IndexExpr(ExprPtr o, ExprPtr i, int l)
        : object(std::move(o)), index(std::move(i)) { line = l; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

struct FieldExpr : Expr {
    ExprPtr object;
    std::string field;
    bool optional;
    FieldExpr(ExprPtr o, std::string f, bool opt, int l)
        : object(std::move(o)), field(std::move(f)), optional(opt) { line = l; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

struct SliceExpr : Expr {
    ExprPtr object, start, end, step;
    SliceExpr(ExprPtr o, ExprPtr s, ExprPtr e, ExprPtr st, int l)
        : object(std::move(o)), start(std::move(s)), end(std::move(e)), step(std::move(st)) { line = l; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

struct InterpPart {
    bool isLiteral;
    std::string text;
    ExprPtr expr;
};

struct StringInterpExpr : Expr {
    std::vector<InterpPart> parts;
    StringInterpExpr(std::vector<InterpPart> p, int l) : parts(std::move(p)) { line = l; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

struct RangeExpr : Expr {
    ExprPtr start, end, step;
    bool inclusive;
    RangeExpr(ExprPtr s, ExprPtr e, bool inc, ExprPtr st, int l)
        : start(std::move(s)), end(std::move(e)), step(std::move(st)), inclusive(inc) { line = l; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

struct PipeExpr : Expr {
    ExprPtr left, right;
    PipeExpr(ExprPtr l, ExprPtr r, int ln) : left(std::move(l)), right(std::move(r)) { line = ln; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

// Pattern matching
struct Pattern {
    enum class PKind { Wildcard, Binding, Literal, Array, Map };
    PKind kind = PKind::Wildcard;
    std::string bind;
    Value literal;
    std::vector<Pattern> elems;
    std::string rest; // for [x, ...rest]
    std::vector<std::pair<std::string, Pattern>> mapEntries;
    int line = 1;

    bool match(const Value& v, const EnvPtr& binds) const;
};

struct MatchArm {
    Pattern pat;
    ExprPtr guard;
    ExprPtr body;
    std::shared_ptr<BlockStmt> blockBody;
    int line = 1;
};

struct MatchExpr : Expr {
    ExprPtr subject;
    std::vector<MatchArm> arms;
    MatchExpr(ExprPtr s, std::vector<MatchArm> a, int l)
        : subject(std::move(s)), arms(std::move(a)) { line = l; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

struct IsExpr : Expr {
    ExprPtr operand;
    std::string typeName;
    IsExpr(ExprPtr o, std::string t, int l)
        : operand(std::move(o)), typeName(std::move(t)) { line = l; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

struct ArrayExpr : Expr {
    struct Elem { ExprPtr expr; bool spread; };
    std::vector<Elem> elems;
    ArrayExpr(std::vector<Elem> e, int l) : elems(std::move(e)) { line = l; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

struct MapExpr : Expr {
    std::vector<std::pair<ExprPtr, ExprPtr>> pairs;
    MapExpr(std::vector<std::pair<ExprPtr, ExprPtr>> p, int l) : pairs(std::move(p)) { line = l; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

struct LambdaExpr : Expr {
    std::vector<Param> params;
    std::shared_ptr<BlockStmt> body;
    std::string returnType;
    LambdaExpr(std::vector<Param> p,
               std::shared_ptr<BlockStmt> b, std::string rt, int l)
        : params(std::move(p)), body(std::move(b)), returnType(std::move(rt)) { line = l; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

struct TernaryExpr : Expr {
    ExprPtr cond, thenE, elseE;
    TernaryExpr(ExprPtr c, ExprPtr t, ExprPtr e, int l)
        : cond(std::move(c)), thenE(std::move(t)), elseE(std::move(e)) { line = l; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

// ---------------------------------------------------------------------------
// Statement AST
// ---------------------------------------------------------------------------
struct Stmt {
    int line = 1;
    virtual ~Stmt() = default;
    virtual Value exec(const EnvPtr& env, Interpreter& I) = 0;
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> stmts;
    BlockStmt(std::vector<StmtPtr> s, int l) : stmts(std::move(s)) { line = l; }
    Value exec(const EnvPtr& env, Interpreter& I) override;
};

struct ExprStmt : Stmt {
    ExprPtr expr;
    ExprStmt(ExprPtr e, int l) : expr(std::move(e)) { line = l; }
    Value exec(const EnvPtr& env, Interpreter& I) override;
};

struct BindStmt : Stmt {
    ExprPtr target;
    std::string type;
    std::string op;
    ExprPtr value;
    BindStmt(ExprPtr t, std::string ty, std::string o, ExprPtr v, int l)
        : target(std::move(t)), type(std::move(ty)), op(std::move(o)), value(std::move(v)) { line = l; }
    Value exec(const EnvPtr& env, Interpreter& I) override;
};

struct IfStmt : Stmt {
    ExprPtr cond;
    std::shared_ptr<BlockStmt> thenB;
    StmtPtr elseB;
    IfStmt(ExprPtr c, std::shared_ptr<BlockStmt> t, StmtPtr e, int l)
        : cond(std::move(c)), thenB(std::move(t)), elseB(std::move(e)) { line = l; }
    Value exec(const EnvPtr& env, Interpreter& I) override;
};

struct WhileStmt : Stmt {
    ExprPtr cond;
    std::shared_ptr<BlockStmt> body;
    WhileStmt(ExprPtr c, std::shared_ptr<BlockStmt> b, int l)
        : cond(std::move(c)), body(std::move(b)) { line = l; }
    Value exec(const EnvPtr& env, Interpreter& I) override;
};

struct ForRangeStmt : Stmt {
    std::string var;
    ExprPtr start, end, step;
    bool inclusive;
    std::shared_ptr<BlockStmt> body;
    ForRangeStmt(std::string v, ExprPtr s, ExprPtr e, bool inc,
                 std::shared_ptr<BlockStmt> b, int l, ExprPtr st = nullptr)
        : var(std::move(v)), start(std::move(s)), end(std::move(e)),
          step(std::move(st)), inclusive(inc), body(std::move(b)) { line = l; }
    Value exec(const EnvPtr& env, Interpreter& I) override;
};

struct ForEachStmt : Stmt {
    std::string var;
    std::vector<std::string> destructure; // if non-empty, bind elements of each item
    ExprPtr iterable;
    std::shared_ptr<BlockStmt> body;
    ForEachStmt(std::string v, ExprPtr it, std::shared_ptr<BlockStmt> b, int l)
        : var(std::move(v)), iterable(std::move(it)), body(std::move(b)) { line = l; }
    ForEachStmt(std::vector<std::string> d, ExprPtr it, std::shared_ptr<BlockStmt> b, int l)
        : destructure(std::move(d)), iterable(std::move(it)), body(std::move(b)) { line = l; }
    Value exec(const EnvPtr& env, Interpreter& I) override;
};

struct FuncDeclStmt : Stmt {
    std::string name;
    std::vector<Param> params;
    std::string returnType;
    std::shared_ptr<BlockStmt> body;
    FuncDeclStmt(std::string n, std::vector<Param> p,
                 std::string rt, std::shared_ptr<BlockStmt> b, int l)
        : name(std::move(n)), params(std::move(p)),
          returnType(std::move(rt)), body(std::move(b)) { line = l; }
    Value exec(const EnvPtr& env, Interpreter& I) override;
};

struct ReturnStmt : Stmt {
    ExprPtr value;
    ReturnStmt(ExprPtr v, int l) : value(std::move(v)) { line = l; }
    Value exec(const EnvPtr& env, Interpreter& I) override;
};

struct BreakStmt : Stmt {
    explicit BreakStmt(int l) { line = l; }
    Value exec(const EnvPtr&, Interpreter&) override;
};

struct ContinueStmt : Stmt {
    explicit ContinueStmt(int l) { line = l; }
    Value exec(const EnvPtr&, Interpreter&) override;
};

struct AssertStmt : Stmt {
    ExprPtr cond;
    ExprPtr message;
    AssertStmt(ExprPtr c, ExprPtr m, int l) : cond(std::move(c)), message(std::move(m)) { line = l; }
    Value exec(const EnvPtr& env, Interpreter& I) override;
};

struct DeferStmt : Stmt {
    StmtPtr body;
    DeferStmt(StmtPtr b, int l) : body(std::move(b)) { line = l; }
    Value exec(const EnvPtr& env, Interpreter& I) override;
};

struct ThrowStmt : Stmt {
    ExprPtr value;
    ThrowStmt(ExprPtr v, int l) : value(std::move(v)) { line = l; }
    Value exec(const EnvPtr& env, Interpreter& I) override;
};

struct TryCatchStmt : Stmt {
    std::shared_ptr<BlockStmt> tryBlock;
    std::string catchVar;
    std::shared_ptr<BlockStmt> catchBlock;
    TryCatchStmt(std::shared_ptr<BlockStmt> tb, std::string cv, std::shared_ptr<BlockStmt> cb, int l)
        : tryBlock(std::move(tb)), catchVar(std::move(cv)), catchBlock(std::move(cb)) { line = l; }
    Value exec(const EnvPtr& env, Interpreter& I) override;
};

struct RecordDeclStmt : Stmt {
    std::string name;
    std::vector<std::pair<std::string, std::string>> fields;
    RecordDeclStmt(std::string n, std::vector<std::pair<std::string, std::string>> f, int l)
        : name(std::move(n)), fields(std::move(f)) { line = l; }
    Value exec(const EnvPtr& env, Interpreter& I) override;
};

struct ImportStmt : Stmt {
    std::string path;
    std::string asName;
    ImportStmt(std::string p, std::string a, int l) : path(std::move(p)), asName(std::move(a)) { line = l; }
    Value exec(const EnvPtr& env, Interpreter& I) override;
};

struct RecordConstructExpr : Expr {
    std::string typeName;
    std::vector<std::pair<std::string, ExprPtr>> fields;
    RecordConstructExpr(std::string t, std::vector<std::pair<std::string, ExprPtr>> f, int l)
        : typeName(std::move(t)), fields(std::move(f)) { line = l; }
    Value eval(const EnvPtr& env, Interpreter& I) override;
};

// ---------------------------------------------------------------------------
// Function object
// ---------------------------------------------------------------------------
struct Function {
    std::string name;
    std::vector<Param> params;
    std::string returnType = "any";
    std::shared_ptr<BlockStmt> body;
    EnvPtr closure;
};

// ---------------------------------------------------------------------------
// Interpreter
// ---------------------------------------------------------------------------
struct RecordType {
    std::string name;
    std::vector<std::pair<std::string, std::string>> fields; // name, type
};

struct Interpreter {
    EnvPtr global;
    int depth = 0;
    std::mt19937_64 rng;
    std::unordered_map<std::string, std::shared_ptr<RecordType>> recordTypes;
    std::unordered_map<std::string, EnvPtr> moduleCache;
    std::vector<std::string> dirStack;
    std::vector<std::pair<EnvPtr, StmtPtr>> defers;
    size_t deferScope = 0;

    Interpreter();
    void run(const std::vector<StmtPtr>& program);
    Value call(const Value& callee, std::vector<Value>& args, int line);
    EnvPtr loadModule(const std::string& path, int line);
};

// ---------------------------------------------------------------------------
// Value helpers
// ---------------------------------------------------------------------------
static std::string numToString(double v) {
    if (std::isnan(v)) return "nan";
    if (std::isinf(v)) return v > 0 ? "inf" : "-inf";
    if (v == std::floor(v) && std::abs(v) < 1e15) {
        std::ostringstream oss;
        oss << static_cast<long long>(v);
        return oss.str();
    }
    std::ostringstream oss;
    oss << std::setprecision(15) << v;
    return oss.str();
}

static std::string quoteString(const std::string& s) {
    std::string r = "\"";
    for (char c : s) {
        switch (c) {
            case '"':  r += "\\\""; break;
            case '\\': r += "\\\\"; break;
            case '\n': r += "\\n";  break;
            case '\t': r += "\\t";  break;
            case '\r': r += "\\r";  break;
            default:   r += c;      break;
        }
    }
    r += "\"";
    return r;
}

std::string Value::toString(bool nested) const {
    switch (kind) {
        case Kind::Nil:  return "nil";
        case Kind::Num:  return numToString(n);
        case Kind::Bool: return b ? "true" : "false";
        case Kind::Str:  return nested ? quoteString(s) : s;
        case Kind::Arr: {
            std::string r = "[";
            for (size_t i = 0; i < arr->size(); i++) {
                if (i) r += ", ";
                r += (*arr)[i].toString(true);
            }
            r += "]";
            return r;
        }
        case Kind::Map: {
            auto rit = map->find("__record");
            if (rit != map->end() && rit->second.kind == Kind::Str) {
                std::string r = rit->second.s + "{";
                bool first = true;
                for (const auto& kv : *map) {
                    if (kv.first == "__record") continue;
                    if (!first) r += ", ";
                    first = false;
                    r += kv.first + ": " + kv.second.toString(true);
                }
                r += "}";
                return r;
            }
            std::string r = "%{";
            bool first = true;
            for (const auto& kv : *map) {
                if (!first) r += ", ";
                first = false;
                r += quoteString(kv.first) + ": " + kv.second.toString(true);
            }
            r += "}";
            return r;
        }
        case Kind::Range: {
            return "<range " + std::to_string(static_cast<long long>(rangeStart)) +
                   (rangeIncl ? " through " : " to ") +
                   std::to_string(static_cast<long long>(rangeEnd)) + ">";
        }
        case Kind::Func:
            return (func && !func->name.empty() && func->name != "<lambda>")
                       ? ("<fn " + func->name + ">") : "<fn>";
        case Kind::Native: return "<native fn>";
    }
    return "nil";
}

static bool valuesEqual(const Value& a, const Value& b) {
    if (a.kind != b.kind) return false;
    switch (a.kind) {
        case Value::Kind::Nil:  return true;
        case Value::Kind::Num:  return a.n == b.n;
        case Value::Kind::Str:  return a.s == b.s;
        case Value::Kind::Bool: return a.b == b.b;
        case Value::Kind::Arr: {
            if (a.arr->size() != b.arr->size()) return false;
            for (size_t i = 0; i < a.arr->size(); i++)
                if (!valuesEqual((*a.arr)[i], (*b.arr)[i])) return false;
            return true;
        }
        case Value::Kind::Map: {
            if (a.map->size() != b.map->size()) return false;
            for (const auto& kv : *a.map) {
                auto it = b.map->find(kv.first);
                if (it == b.map->end() || !valuesEqual(kv.second, it->second)) return false;
            }
            return true;
        }
        case Value::Kind::Range:
            return a.rangeStart == b.rangeStart && a.rangeEnd == b.rangeEnd &&
                   a.rangeIncl == b.rangeIncl && a.rangeStep == b.rangeStep;
        case Value::Kind::Func: return a.func == b.func;
        case Value::Kind::Native: return false;
    }
    return false;
}

static bool truthy(const Value& v) {
    switch (v.kind) {
        case Value::Kind::Nil:  return false;
        case Value::Kind::Bool: return v.b;
        case Value::Kind::Num:  return v.n != 0.0;
        case Value::Kind::Str:  return !v.s.empty();
        case Value::Kind::Arr:  return !v.arr->empty();
        case Value::Kind::Map:  return !v.map->empty();
        default: return true;
    }
}

static std::string typeName(const Value& v) {
    switch (v.kind) {
        case Value::Kind::Nil:  return "nil";
        case Value::Kind::Num:  return "num";
        case Value::Kind::Str:  return "str";
        case Value::Kind::Bool: return "bool";
        case Value::Kind::Arr:  return "arr";
        case Value::Kind::Map:  return "map";
        case Value::Kind::Range: return "range";
        case Value::Kind::Func: return "fn";
        case Value::Kind::Native: return "fn";
    }
    return "?";
}

static void checkType(const Value& v, const std::string& type, int line, bool isReturn = false) {
    if (type.empty() || type == "any") return;
    bool ok = false;
    if      (type == "num")  ok = v.kind == Value::Kind::Num;
    else if (type == "str")  ok = v.kind == Value::Kind::Str;
    else if (type == "bool") ok = v.kind == Value::Kind::Bool;
    else if (type == "arr")  ok = v.kind == Value::Kind::Arr;
    else if (type == "map")  ok = v.kind == Value::Kind::Map;
    else if (type == "fn")   ok = v.kind == Value::Kind::Func || v.kind == Value::Kind::Native;
    else if (type == "nil")  ok = v.kind == Value::Kind::Nil;
    else throw RuntimeError(line, "unknown type '" + type + "'");

    if (!ok) {
        throw RuntimeError(line, std::string(isReturn ? "return" : "binding") +
                                    " type mismatch: expected " + type +
                                    ", got " + typeName(v));
    }
}

static long long toIndex(double d, int line) {
    if (!std::isfinite(d)) throw RuntimeError(line, "index must be a finite number");
    return static_cast<long long>(d);
}

static Value applyBinary(const std::string& op, const Value& a, const Value& b, int line) {
    if (op == "+") {
        if (a.kind == Value::Kind::Str || b.kind == Value::Kind::Str) {
            return Value::Str(a.toString(false) + b.toString(false));
        }
        if (a.kind == Value::Kind::Num && b.kind == Value::Kind::Num) {
            return Value::Num(a.n + b.n);
        }
        throw RuntimeError(line, "operator '+' expects numbers or strings");
    }
    if (op == "==") return Value::Bool(valuesEqual(a, b));
    if (op == "!=") return Value::Bool(!valuesEqual(a, b));

    if (op == "<" || op == "<=" || op == ">" || op == ">=") {
        if (a.kind == Value::Kind::Num && b.kind == Value::Kind::Num) {
            if (op == "<")  return Value::Bool(a.n <  b.n);
            if (op == "<=") return Value::Bool(a.n <= b.n);
            if (op == ">")  return Value::Bool(a.n >  b.n);
            return Value::Bool(a.n >= b.n);
        }
        if (a.kind == Value::Kind::Str && b.kind == Value::Kind::Str) {
            if (op == "<")  return Value::Bool(a.s <  b.s);
            if (op == "<=") return Value::Bool(a.s <= b.s);
            if (op == ">")  return Value::Bool(a.s >  b.s);
            return Value::Bool(a.s >= b.s);
        }
        throw RuntimeError(line, "operator '" + op + "' expects two numbers or two strings");
    }

    if (a.kind != Value::Kind::Num || b.kind != Value::Kind::Num) {
        throw RuntimeError(line, "operator '" + op + "' expects numbers");
    }
    double x = a.n, y = b.n;
    if (op == "-")  return Value::Num(x - y);
    if (op == "*")  return Value::Num(x * y);
    if (op == "/") {
        if (y == 0.0) throw RuntimeError(line, "division by zero");
        return Value::Num(x / y);
    }
    if (op == "%") {
        if (y == 0.0) throw RuntimeError(line, "modulo by zero");
        return Value::Num(std::fmod(x, y));
    }
    if (op == "**") return Value::Num(std::pow(x, y));
    // Bitwise (operate on integer values)
    long long ix = static_cast<long long>(x);
    long long iy = static_cast<long long>(y);
    if (op == "&")  return Value::Num(static_cast<double>(ix & iy));
    if (op == "|")  return Value::Num(static_cast<double>(ix | iy));
    if (op == "^")  return Value::Num(static_cast<double>(ix ^ iy));
    if (op == "<<") {
        if (iy < 0 || iy >= 63) throw RuntimeError(line, "shift amount out of range");
        return Value::Num(static_cast<double>(ix << iy));
    }
    if (op == ">>") {
        if (iy < 0 || iy >= 63) throw RuntimeError(line, "shift amount out of range");
        return Value::Num(static_cast<double>(ix >> iy));
    }
    throw RuntimeError(line, "unknown operator '" + op + "'");
}

static Value indexGet(const Value& obj, const Value& idx, int line) {
    if (obj.kind == Value::Kind::Arr) {
        if (idx.kind != Value::Kind::Num) throw RuntimeError(line, "array index must be a number");
        long long i = toIndex(idx.n, line);
        long long n = static_cast<long long>(obj.arr->size());
        if (i < 0) i += n;
        if (i < 0 || i >= n) throw RuntimeError(line, "array index out of bounds");
        return (*obj.arr)[static_cast<size_t>(i)];
    }
    if (obj.kind == Value::Kind::Str) {
        if (idx.kind != Value::Kind::Num) throw RuntimeError(line, "string index must be a number");
        long long i = toIndex(idx.n, line);
        long long n = static_cast<long long>(obj.s.size());
        if (i < 0) i += n;
        if (i < 0 || i >= n) throw RuntimeError(line, "string index out of bounds");
        return Value::Str(std::string(1, obj.s[static_cast<size_t>(i)]));
    }
    if (obj.kind == Value::Kind::Map) {
        if (idx.kind != Value::Kind::Str) throw RuntimeError(line, "map key must be a string");
        auto it = obj.map->find(idx.s);
        if (it == obj.map->end()) return Value::Nil();
        return it->second;
    }
    throw RuntimeError(line, "cannot index value of type " + typeName(obj));
}

static void indexSet(Value& obj, const Value& idx, const Value& val, int line) {
    if (obj.kind == Value::Kind::Arr) {
        if (idx.kind != Value::Kind::Num) throw RuntimeError(line, "array index must be a number");
        long long i = toIndex(idx.n, line);
        long long n = static_cast<long long>(obj.arr->size());
        if (i < 0) i += n;
        if (i < 0 || i >= n) throw RuntimeError(line, "array index out of bounds");
        (*obj.arr)[static_cast<size_t>(i)] = val;
        return;
    }
    if (obj.kind == Value::Kind::Map) {
        if (idx.kind != Value::Kind::Str) throw RuntimeError(line, "map key must be a string");
        (*obj.map)[idx.s] = val;
        return;
    }
    throw RuntimeError(line, "cannot index-assign value of type " + typeName(obj));
}

// ---------------------------------------------------------------------------
// Expression evaluation
// ---------------------------------------------------------------------------
Value LiteralExpr::eval(const EnvPtr&, Interpreter&) { return value; }

Value VarExpr::eval(const EnvPtr& env, Interpreter&) {
    Value* v = env->lookup(name);
    if (!v) throw RuntimeError(line, "undefined variable '" + name + "'");
    return *v;
}

Value AssignExpr::eval(const EnvPtr& env, Interpreter& I) {
    std::string bop = op.substr(0, op.size() - 1);
    if (auto ve = dynamic_cast<VarExpr*>(target.get())) {
        Env* owner = env->owner(ve->name);
        if (!owner) throw RuntimeError(line, "undefined variable '" + ve->name + "'");
        if (owner->consts.count(ve->name)) {
            throw RuntimeError(line, "cannot assign to immutable variable '" + ve->name + "'");
        }
        Value rhs = value->eval(env, I);
        Value& slot = owner->vars[ve->name];
        slot = applyBinary(bop, slot, rhs, line);
        return slot;
    }
    if (auto ie = dynamic_cast<IndexExpr*>(target.get())) {
        Value obj = ie->object->eval(env, I);
        Value idx = ie->index->eval(env, I);
        Value cur = indexGet(obj, idx, line);
        Value rhs = value->eval(env, I);
        Value nv = applyBinary(bop, cur, rhs, line);
        indexSet(obj, idx, nv, line);
        return nv;
    }
    throw RuntimeError(line, "invalid assignment target");
}

Value BinaryExpr::eval(const EnvPtr& env, Interpreter& I) {
    if (op == "&&") {
        Value l = left->eval(env, I);
        if (!truthy(l)) return Value::Bool(false);
        return Value::Bool(truthy(right->eval(env, I)));
    }
    if (op == "||") {
        Value l = left->eval(env, I);
        if (truthy(l)) return Value::Bool(true);
        return Value::Bool(truthy(right->eval(env, I)));
    }
    if (op == "??") {
        Value l = left->eval(env, I);
        if (!l.isNil()) return l;
        return right->eval(env, I);
    }
    Value l = left->eval(env, I);
    Value r = right->eval(env, I);
    return applyBinary(op, l, r, line);
}

Value UnaryExpr::eval(const EnvPtr& env, Interpreter& I) {
    Value v = operand->eval(env, I);
    if (op == "-") {
        if (v.kind != Value::Kind::Num) throw RuntimeError(line, "unary '-' expects a number");
        return Value::Num(-v.n);
    }
    if (op == "!") return Value::Bool(!truthy(v));
    if (op == "~") {
        if (v.kind != Value::Kind::Num) throw RuntimeError(line, "unary '~' expects a number");
        return Value::Num(static_cast<double>(~static_cast<long long>(v.n)));
    }
    throw RuntimeError(line, "unknown unary operator '" + op + "'");
}

Value CallExpr::eval(const EnvPtr& env, Interpreter& I) {
    Value calleeVal;
    Value methodSelf;
    bool hasSelf = false;
    // Method sugar: only if first param is named "self" (so import namespaces still work)
    if (auto fe = dynamic_cast<FieldExpr*>(this->callee.get())) {
        methodSelf = fe->object->eval(env, I);
        if (methodSelf.kind == Value::Kind::Map) {
            auto it = methodSelf.map->find(fe->field);
            if (it != methodSelf.map->end() && it->second.kind == Value::Kind::Func) {
                calleeVal = it->second;
                if (!calleeVal.func->params.empty() && calleeVal.func->params[0].name == "self") {
                    hasSelf = true;
                }
            } else if (it != methodSelf.map->end()) {
                calleeVal = it->second;
            } else {
                calleeVal = this->callee->eval(env, I);
            }
        } else {
            calleeVal = this->callee->eval(env, I);
        }
    } else {
        calleeVal = this->callee->eval(env, I);
    }
    Value callee = calleeVal;
    // Named args: resolve against function params
    bool anyNamed = false;
    for (const auto& a : args) if (!a.name.empty()) anyNamed = true;

    if (anyNamed && callee.kind == Value::Kind::Func) {
        auto fn = callee.func;
        std::vector<Value> vals(fn->params.size());
        std::vector<bool> filled(fn->params.size(), false);
        size_t positional = 0;
        for (const auto& a : args) {
            Value v = a.expr->eval(env, I);
            if (a.name.empty()) {
                while (positional < fn->params.size() && filled[positional]) positional++;
                if (positional >= fn->params.size())
                    throw RuntimeError(line, "too many arguments to '" + fn->name + "'");
                vals[positional] = v;
                filled[positional] = true;
                positional++;
            } else {
                bool found = false;
                for (size_t i = 0; i < fn->params.size(); i++) {
                    if (fn->params[i].name == a.name) {
                        vals[i] = v;
                        filled[i] = true;
                        found = true;
                        break;
                    }
                }
                if (!found) throw RuntimeError(line, "unknown argument name '" + a.name + "'");
            }
        }
        // Fill defaults for missing
        for (size_t i = 0; i < fn->params.size(); i++) {
            if (!filled[i]) {
                if (fn->params[i].defaultValue) {
                    // defaults filled inside call — pass a sentinel by using call with partial
                    // Simpler: build args list only for filled + let call handle defaults
                } else {
                    throw RuntimeError(line, "missing argument '" + fn->params[i].name + "'");
                }
            }
        }
        // Build compact args vector matching param order for filled+defaults
        std::vector<Value> ordered;
        for (size_t i = 0; i < fn->params.size(); i++) {
            if (filled[i]) ordered.push_back(vals[i]);
            else break; // rest must be defaults trailing
        }
        // If holes exist before trailing defaults, error
        for (size_t i = 0; i < fn->params.size(); i++) {
            if (!filled[i] && !fn->params[i].defaultValue)
                throw RuntimeError(line, "missing argument '" + fn->params[i].name + "'");
            if (!filled[i] && fn->params[i].defaultValue) {
                // stop ordered at first default hole — call() fills defaults
                break;
            }
        }
        // Rebuild properly: pass all filled in order, call fills trailing defaults
        ordered.clear();
        for (size_t i = 0; i < fn->params.size(); i++) {
            if (filled[i]) ordered.push_back(vals[i]);
            else if (fn->params[i].defaultValue) {
                // don't push — call will eval default
                // but call expects leading required then trailing defaults by count
                // So we need to pass values for all leading params
                break;
            }
        }
        // Final approach: evaluate defaults here for missing
        ordered.clear();
        EnvPtr tmp = std::make_shared<Env>(fn->closure);
        for (size_t i = 0; i < fn->params.size(); i++) {
            if (filled[i]) {
                ordered.push_back(vals[i]);
                tmp->define(fn->params[i].name, vals[i], false);
            } else if (fn->params[i].defaultValue) {
                Value d = fn->params[i].defaultValue->eval(tmp, I);
                ordered.push_back(d);
                tmp->define(fn->params[i].name, d, false);
            } else {
                throw RuntimeError(line, "missing argument '" + fn->params[i].name + "'");
            }
        }
        return I.call(callee, ordered, line);
    }

    std::vector<Value> vals;
    if (hasSelf) vals.push_back(methodSelf);
    vals.reserve(vals.size() + args.size());
    for (const auto& a : args) vals.push_back(a.expr->eval(env, I));
    return I.call(callee, vals, line);
}

Value IndexExpr::eval(const EnvPtr& env, Interpreter& I) {
    Value obj = object->eval(env, I);
    Value idx = index->eval(env, I);
    return indexGet(obj, idx, line);
}

Value FieldExpr::eval(const EnvPtr& env, Interpreter& I) {
    Value obj = object->eval(env, I);
    if (obj.isNil()) {
        if (optional) return Value::Nil();
        throw RuntimeError(line, "cannot access field on nil");
    }
    if (obj.kind == Value::Kind::Map) {
        auto it = obj.map->find(field);
        if (it == obj.map->end()) {
            if (optional) return Value::Nil();
            return Value::Nil();
        }
        return it->second;
    }
    if (optional) return Value::Nil();
    throw RuntimeError(line, "cannot access field on type " + typeName(obj));
}

Value SliceExpr::eval(const EnvPtr& env, Interpreter& I) {
    Value obj = object->eval(env, I);
    long long stepVal = 1;
    if (step) {
        Value stv = step->eval(env, I);
        if (stv.kind != Value::Kind::Num) throw RuntimeError(line, "slice step must be number");
        stepVal = static_cast<long long>(stv.n);
        if (stepVal == 0) throw RuntimeError(line, "slice step cannot be 0");
    }
    auto resolve = [&](ExprPtr& ep, long long def, long long n) -> long long {
        if (!ep) return def;
        Value v = ep->eval(env, I);
        if (v.kind != Value::Kind::Num) throw RuntimeError(line, "slice index must be number");
        long long i = static_cast<long long>(v.n);
        if (i < 0) i += n;
        return i;
    };
    if (obj.kind == Value::Kind::Arr) {
        long long n = static_cast<long long>(obj.arr->size());
        long long s, e;
        if (stepVal > 0) {
            s = resolve(start, 0, n);
            e = resolve(end, n, n);
            if (s < 0) s = 0;
            if (e > n) e = n;
        } else {
            s = resolve(start, n - 1, n);
            e = resolve(end, -1, n);
            if (s >= n) s = n - 1;
        }
        auto out = std::make_shared<std::vector<Value>>();
        if (stepVal > 0) {
            for (long long i = s; i < e; i += stepVal)
                if (i >= 0 && i < n) out->push_back((*obj.arr)[static_cast<size_t>(i)]);
        } else {
            for (long long i = s; i > e; i += stepVal)
                if (i >= 0 && i < n) out->push_back((*obj.arr)[static_cast<size_t>(i)]);
        }
        return Value::Arr(out);
    }
    if (obj.kind == Value::Kind::Str) {
        long long n = static_cast<long long>(obj.s.size());
        long long s, e;
        if (stepVal > 0) {
            s = resolve(start, 0, n);
            e = resolve(end, n, n);
            if (s < 0) s = 0;
            if (e > n) e = n;
        } else {
            s = resolve(start, n - 1, n);
            e = resolve(end, -1, n);
            if (s >= n) s = n - 1;
        }
        std::string out;
        if (stepVal > 0) {
            for (long long i = s; i < e; i += stepVal)
                if (i >= 0 && i < n) out += obj.s[static_cast<size_t>(i)];
        } else {
            for (long long i = s; i > e; i += stepVal)
                if (i >= 0 && i < n) out += obj.s[static_cast<size_t>(i)];
        }
        return Value::Str(out);
    }
    throw RuntimeError(line, "slice expects array or string");
}

Value IsExpr::eval(const EnvPtr& env, Interpreter& I) {
    Value v = operand->eval(env, I);
    // Built-in types
    std::string actual = ::typeName(v);
    if (actual == this->typeName) return Value::Bool(true);
    // Record types
    if (v.kind == Value::Kind::Map) {
        auto it = v.map->find("__record");
        if (it != v.map->end() && it->second.kind == Value::Kind::Str) {
            if (it->second.s == this->typeName) return Value::Bool(true);
        }
    }
    // Enum variant: value is string "Color.Red", type name is "Color"
    if (v.kind == Value::Kind::Str) {
        std::string prefix = this->typeName + ".";
        if (v.s.size() > prefix.size() && v.s.compare(0, prefix.size(), prefix) == 0)
            return Value::Bool(true);
    }
    // Enum type object itself
    if (v.kind == Value::Kind::Map) {
        auto it = v.map->find("__enum");
        if (it != v.map->end() && it->second.kind == Value::Kind::Str &&
            it->second.s == this->typeName)
            return Value::Bool(true);
    }
    return Value::Bool(false);
}

Value StringInterpExpr::eval(const EnvPtr& env, Interpreter& I) {
    std::string out;
    for (const auto& p : parts) {
        if (p.isLiteral) out += p.text;
        else out += p.expr->eval(env, I).toString(false);
    }
    return Value::Str(out);
}

Value RangeExpr::eval(const EnvPtr& env, Interpreter& I) {
    Value s = start->eval(env, I);
    Value e = end->eval(env, I);
    if (s.kind != Value::Kind::Num || e.kind != Value::Kind::Num)
        throw RuntimeError(line, "range bounds must be numbers");
    double st = 1.0;
    if (step) {
        Value sv = step->eval(env, I);
        if (sv.kind != Value::Kind::Num) throw RuntimeError(line, "range step must be a number");
        st = sv.n;
    }
    return Value::Range(s.n, e.n, inclusive, st);
}

Value PipeExpr::eval(const EnvPtr& env, Interpreter& I) {
    Value leftVal = left->eval(env, I);
    // If right is a call, prepend leftVal to its arguments
    if (auto call = dynamic_cast<CallExpr*>(right.get())) {
        Value callee = call->callee->eval(env, I);
        std::vector<Value> args;
        args.push_back(leftVal);
        for (const auto& a : call->args) args.push_back(a.expr->eval(env, I));
        return I.call(callee, args, line);
    }
    // Otherwise treat right as a function and call it with leftVal
    Value fn = right->eval(env, I);
    std::vector<Value> args{leftVal};
    return I.call(fn, args, line);
}

bool Pattern::match(const Value& v, const EnvPtr& binds) const {
    switch (kind) {
        case PKind::Wildcard:
            return true;
        case PKind::Binding:
            binds->define(bind, v, true);
            return true;
        case PKind::Literal:
            return valuesEqual(v, literal);
        case PKind::Array: {
            if (v.kind != Value::Kind::Arr) return false;
            const auto& arr = *v.arr;
            if (!rest.empty()) {
                if (arr.size() < elems.size()) return false;
                for (size_t i = 0; i < elems.size(); i++) {
                    if (!elems[i].match(arr[i], binds)) return false;
                }
                auto restArr = std::make_shared<std::vector<Value>>();
                for (size_t i = elems.size(); i < arr.size(); i++) restArr->push_back(arr[i]);
                binds->define(rest, Value::Arr(restArr), true);
                return true;
            }
            if (arr.size() != elems.size()) return false;
            for (size_t i = 0; i < elems.size(); i++) {
                if (!elems[i].match(arr[i], binds)) return false;
            }
            return true;
        }
        case PKind::Map: {
            if (v.kind != Value::Kind::Map) return false;
            for (const auto& e : mapEntries) {
                auto it = v.map->find(e.first);
                Value mv = (it != v.map->end()) ? it->second : Value::Nil();
                if (!e.second.match(mv, binds)) return false;
            }
            return true;
        }
    }
    return false;
}

Value MatchExpr::eval(const EnvPtr& env, Interpreter& I) {
    Value subj = subject->eval(env, I);
    for (const auto& arm : arms) {
        EnvPtr binds = std::make_shared<Env>(env);
        if (!arm.pat.match(subj, binds)) continue;
        if (arm.guard) {
            try {
                if (!truthy(arm.guard->eval(binds, I))) continue;
            } catch (const RuntimeError&) {
                continue; // guard type errors => arm does not match
            }
        }
        if (arm.blockBody) return arm.blockBody->exec(binds, I);
        if (arm.body) return arm.body->eval(binds, I);
        return Value::Nil();
    }
    throw RuntimeError(line, "no matching arm in match expression");
}

Value ArrayExpr::eval(const EnvPtr& env, Interpreter& I) {
    auto v = std::make_shared<std::vector<Value>>();
    for (const auto& e : elems) {
        Value val = e.expr->eval(env, I);
        if (e.spread) {
            if (val.kind == Value::Kind::Arr) {
                for (const auto& x : *val.arr) v->push_back(x);
            } else if (val.kind == Value::Kind::Range) {
                double cur = val.rangeStart;
                while (val.rangeIncl ? cur <= val.rangeEnd : cur < val.rangeEnd) {
                    v->push_back(Value::Num(cur));
                    cur += 1.0;
                }
            } else {
                throw RuntimeError(line, "spread expects array or range");
            }
        } else {
            v->push_back(val);
        }
    }
    return Value::Arr(v);
}

Value MapExpr::eval(const EnvPtr& env, Interpreter& I) {
    auto m = std::make_shared<std::map<std::string, Value>>();
    for (const auto& p : pairs) {
        Value k = p.first->eval(env, I);
        if (k.kind != Value::Kind::Str) throw RuntimeError(line, "map keys must be strings");
        (*m)[k.s] = p.second->eval(env, I);
    }
    return Value::Map(m);
}

Value LambdaExpr::eval(const EnvPtr& env, Interpreter&) {
    auto fn = std::make_shared<Function>();
    fn->name = "<lambda>";
    fn->params = params;
    fn->returnType = returnType;
    fn->body = body;
    fn->closure = env;
    return Value::Func(fn);
}

Value TernaryExpr::eval(const EnvPtr& env, Interpreter& I) {
    if (truthy(cond->eval(env, I))) return thenE->eval(env, I);
    return elseE->eval(env, I);
}

// ---------------------------------------------------------------------------
// Statement execution
// ---------------------------------------------------------------------------
Value BlockStmt::exec(const EnvPtr& env, Interpreter& I) {
    // line == -1 means flat block (destructuring) — share parent env
    if (line == -1) {
        for (const auto& s : stmts) s->exec(env, I);
        return Value::Nil();
    }
    EnvPtr child = std::make_shared<Env>(env);
    size_t deferMark = I.defers.size();
    try {
        for (const auto& s : stmts) s->exec(child, I);
    } catch (...) {
        // run defers even on exception
        while (I.defers.size() > deferMark) {
            auto d = I.defers.back();
            I.defers.pop_back();
            try { d.second->exec(d.first, I); } catch (...) {}
        }
        throw;
    }
    while (I.defers.size() > deferMark) {
        auto d = I.defers.back();
        I.defers.pop_back();
        d.second->exec(d.first, I);
    }
    return Value::Nil();
}

Value ExprStmt::exec(const EnvPtr& env, Interpreter& I) {
    return expr->eval(env, I);
}

Value BindStmt::exec(const EnvPtr& env, Interpreter& I) {
    Value v = value->eval(env, I);
    checkType(v, type, line);

    if (auto ve = dynamic_cast<VarExpr*>(target.get())) {
        if (op == ":=") {
            if (env->vars.count(ve->name)) {
                throw RuntimeError(line, "variable '" + ve->name + "' already bound in this scope");
            }
            env->define(ve->name, v, true);
        } else {
            Env* owner = env->owner(ve->name);
            if (owner) {
                if (owner->consts.count(ve->name)) {
                    throw RuntimeError(line, "cannot assign to immutable variable '" + ve->name + "'");
                }
                owner->vars[ve->name] = v;
            } else {
                env->define(ve->name, v, false);
            }
        }
        return Value::Nil();
    }

    if (auto ie = dynamic_cast<IndexExpr*>(target.get())) {
        if (op == ":=") {
            throw RuntimeError(line, "':=' cannot be used for index targets (use '<-')");
        }
        Value obj = ie->object->eval(env, I);
        Value idx = ie->index->eval(env, I);
        indexSet(obj, idx, v, line);
        return Value::Nil();
    }

    if (auto fe = dynamic_cast<FieldExpr*>(target.get())) {
        if (op == ":=") {
            throw RuntimeError(line, "':=' cannot be used for field targets (use '<-')");
        }
        Value obj = fe->object->eval(env, I);
        if (obj.kind != Value::Kind::Map) {
            throw RuntimeError(line, "cannot assign field on type " + typeName(obj));
        }
        (*obj.map)[fe->field] = v;
        return Value::Nil();
    }

    // Slice assignment: a[1:3] <- [x, y]  or  a[2:4] <- []
    if (auto se = dynamic_cast<SliceExpr*>(target.get())) {
        if (op == ":=") {
            throw RuntimeError(line, "':=' cannot be used for slice targets (use '<-')");
        }
        Value obj = se->object->eval(env, I);
        if (obj.kind != Value::Kind::Arr) {
            throw RuntimeError(line, "slice assignment expects an array");
        }
        if (v.kind != Value::Kind::Arr) {
            throw RuntimeError(line, "slice assignment value must be an array");
        }
        long long n = static_cast<long long>(obj.arr->size());
        long long s = 0, e = n;
        if (se->start) {
            Value sv = se->start->eval(env, I);
            if (sv.kind != Value::Kind::Num) throw RuntimeError(line, "slice start must be number");
            s = static_cast<long long>(sv.n);
            if (s < 0) s += n;
        }
        if (se->end) {
            Value ev = se->end->eval(env, I);
            if (ev.kind != Value::Kind::Num) throw RuntimeError(line, "slice end must be number");
            e = static_cast<long long>(ev.n);
            if (e < 0) e += n;
        }
        if (s < 0) s = 0;
        if (e > n) e = n;
        if (s > e) s = e;
        // Rebuild array: prefix + replacement + suffix
        auto out = std::make_shared<std::vector<Value>>();
        for (long long i = 0; i < s; i++) out->push_back((*obj.arr)[static_cast<size_t>(i)]);
        for (const auto& x : *v.arr) out->push_back(x);
        for (long long i = e; i < n; i++) out->push_back((*obj.arr)[static_cast<size_t>(i)]);
        *obj.arr = *out;
        return Value::Nil();
    }

    throw RuntimeError(line, "invalid binding target");
}

Value IfStmt::exec(const EnvPtr& env, Interpreter& I) {
    if (truthy(cond->eval(env, I))) return thenB->exec(env, I);
    if (elseB) return elseB->exec(env, I);
    return Value::Nil();
}

Value WhileStmt::exec(const EnvPtr& env, Interpreter& I) {
    while (truthy(cond->eval(env, I))) {
        try {
            body->exec(env, I);
        } catch (const BreakSignal&) {
            break;
        } catch (const ContinueSignal&) {
            continue;
        }
    }
    return Value::Nil();
}

Value ForRangeStmt::exec(const EnvPtr& env, Interpreter& I) {
    Value sv = start->eval(env, I);
    Value ev = end->eval(env, I);
    if (sv.kind != Value::Kind::Num || ev.kind != Value::Kind::Num) {
        throw RuntimeError(line, "range bounds must be numbers");
    }
    double s = sv.n, e = ev.n;
    double st = 1.0;
    if (step) {
        Value stv = step->eval(env, I);
        if (stv.kind != Value::Kind::Num) throw RuntimeError(line, "range step must be a number");
        st = stv.n;
        if (st == 0.0) throw RuntimeError(line, "range step cannot be 0");
    }
    if (!std::isfinite(s) || !std::isfinite(e) || !std::isfinite(st)) {
        throw RuntimeError(line, "range bounds must be finite");
    }
    auto cont = [&](double i) {
        if (st > 0) return inclusive ? (i <= e) : (i < e);
        return inclusive ? (i >= e) : (i > e);
    };
    for (double i = s; cont(i); i += st) {
        EnvPtr iter = std::make_shared<Env>(env);
        iter->define(var, Value::Num(i), false);
        try {
            body->exec(iter, I);
        } catch (const BreakSignal&) {
            break;
        } catch (const ContinueSignal&) {
            continue;
        }
    }
    return Value::Nil();
}

Value ForEachStmt::exec(const EnvPtr& env, Interpreter& I) {
    Value it = iterable->eval(env, I);

    if (it.kind == Value::Kind::Arr) {
        std::vector<Value> snapshot = *it.arr;
        for (const auto& v : snapshot) {
            EnvPtr iter = std::make_shared<Env>(env);
            if (!destructure.empty()) {
                if (v.kind == Value::Kind::Arr) {
                    for (size_t i = 0; i < destructure.size(); i++) {
                        Value el = (i < v.arr->size()) ? (*v.arr)[i] : Value::Nil();
                        iter->define(destructure[i], el, false);
                    }
                } else {
                    throw RuntimeError(line, "cannot destructure non-array in repeat");
                }
            } else {
                iter->define(var, v, false);
            }
            try {
                body->exec(iter, I);
            } catch (const BreakSignal&) {
                break;
            } catch (const ContinueSignal&) {
                continue;
            }
        }
        return Value::Nil();
    }

    if (it.kind == Value::Kind::Range) {
        double cur = it.rangeStart;
        double end = it.rangeEnd;
        double step = it.rangeStep == 0 ? 1.0 : it.rangeStep;
        bool incl = it.rangeIncl;
        auto cont = [&](double c) {
            if (step > 0) return incl ? c <= end : c < end;
            return incl ? c >= end : c > end;
        };
        while (cont(cur)) {
            EnvPtr iter = std::make_shared<Env>(env);
            iter->define(var, Value::Num(cur), false);
            try {
                body->exec(iter, I);
            } catch (const BreakSignal&) {
                break;
            } catch (const ContinueSignal&) {
                cur += step;
                continue;
            }
            cur += step;
        }
        return Value::Nil();
    }

    if (it.kind == Value::Kind::Str) {
        for (char c : it.s) {
            EnvPtr iter = std::make_shared<Env>(env);
            iter->define(var, Value::Str(std::string(1, c)), false);
            try {
                body->exec(iter, I);
            } catch (const BreakSignal&) {
                break;
            } catch (const ContinueSignal&) {
                continue;
            }
        }
        return Value::Nil();
    }

    if (it.kind == Value::Kind::Map) {
        std::vector<std::string> keys;
        keys.reserve(it.map->size());
        for (const auto& kv : *it.map) keys.push_back(kv.first);

        for (const auto& k : keys) {
            auto mit = it.map->find(k);
            if (mit == it.map->end()) continue;
            EnvPtr iter = std::make_shared<Env>(env);
            iter->define(var, Value::Str(k), false);
            try {
                body->exec(iter, I);
            } catch (const BreakSignal&) {
                break;
            } catch (const ContinueSignal&) {
                continue;
            }
        }
        return Value::Nil();
    }

    throw RuntimeError(line, "cannot iterate over value of type " + typeName(it));
}

Value FuncDeclStmt::exec(const EnvPtr& env, Interpreter&) {
    auto fn = std::make_shared<Function>();
    fn->name = name;
    fn->params = params;
    fn->returnType = returnType;
    fn->body = body;
    fn->closure = env;
    env->define(name, Value::Func(fn), true);
    return Value::Nil();
}

Value ReturnStmt::exec(const EnvPtr& env, Interpreter& I) {
    Value v = value ? value->eval(env, I) : Value::Nil();
    throw ReturnSignal{v};
}

Value BreakStmt::exec(const EnvPtr&, Interpreter&) {
    throw BreakSignal{};
}

Value ContinueStmt::exec(const EnvPtr&, Interpreter&) {
    throw ContinueSignal{};
}

Value AssertStmt::exec(const EnvPtr& env, Interpreter& I) {
    if (!truthy(cond->eval(env, I))) {
        std::string msg = "assertion failed";
        if (message) msg = message->eval(env, I).toString(false);
        throw RuntimeError(line, msg);
    }
    return Value::Nil();
}

Value DeferStmt::exec(const EnvPtr& env, Interpreter& I) {
    I.defers.push_back({env, body});
    return Value::Nil();
}

Value ThrowStmt::exec(const EnvPtr& env, Interpreter& I) {
    throw ErrorSignal{value->eval(env, I)};
}

Value TryCatchStmt::exec(const EnvPtr& env, Interpreter& I) {
    try {
        return tryBlock->exec(env, I);
    } catch (const ErrorSignal& e) {
        auto local = std::make_shared<Env>(env);
        if (!catchVar.empty()) local->define(catchVar, e.value, true);
        return catchBlock->exec(local, I);
    } catch (const RuntimeError& e) {
        auto local = std::make_shared<Env>(env);
        if (!catchVar.empty()) local->define(catchVar, Value::Str(e.what()), true);
        return catchBlock->exec(local, I);
    }
}

Value RecordDeclStmt::exec(const EnvPtr& env, Interpreter& I) {
    auto rt = std::make_shared<RecordType>();
    rt->name = name;
    rt->fields = fields;
    I.recordTypes[name] = rt;
    // Also bind the name so `is Point` and construction can find it
    env->define(name, Value::Str(name), true);
    return Value::Nil();
}

// loadModule implemented after Parser class

Value ImportStmt::exec(const EnvPtr& env, Interpreter& I) {
    EnvPtr mod = I.loadModule(path, line);
    if (!asName.empty()) {
        // import "x.vel" as u  →  bind a map of all exports
        auto m = std::make_shared<std::map<std::string, Value>>();
        for (const auto& kv : mod->vars) {
            if (kv.first.size() >= 2 && kv.first[0] == '_' && kv.first[1] == '_') continue;
            (*m)[kv.first] = kv.second;
        }
        env->define(asName, Value::Map(m), true);
    } else {
        // import "x.vel"  →  copy all bindings into current env
        for (const auto& kv : mod->vars) {
            if (kv.first.size() >= 2 && kv.first[0] == '_' && kv.first[1] == '_') continue;
            env->define(kv.first, kv.second, true);
        }
    }
    return Value::Nil();
}

Value RecordConstructExpr::eval(const EnvPtr& env, Interpreter& I) {
    auto it = I.recordTypes.find(typeName);
    if (it == I.recordTypes.end()) {
        throw RuntimeError(line, "unknown record type '" + typeName + "'");
    }
    auto m = std::make_shared<std::map<std::string, Value>>();
    (*m)["__record"] = Value::Str(typeName);
    for (const auto& f : fields) {
        (*m)[f.first] = f.second->eval(env, I);
    }
    return Value::Map(m);
}

// ---------------------------------------------------------------------------
// Interpreter implementation
// ---------------------------------------------------------------------------
namespace {
struct DepthGuard {
    int& d;
    explicit DepthGuard(int& d) : d(d) { ++d; }
    ~DepthGuard() { --d; }
};
} // namespace

Value Interpreter::call(const Value& callee, std::vector<Value>& args, int line) {
    if (callee.kind == Value::Kind::Native) {
        return callee.native->fn(args);
    }
    if (callee.kind != Value::Kind::Func) {
        throw RuntimeError(line, "value of type " + typeName(callee) + " is not callable");
    }

    auto fn = callee.func;
    // Count required (no default) params
    size_t required = 0;
    for (const auto& p : fn->params) {
        if (!p.defaultValue) required++;
        else break; // defaults must be trailing
    }
    if (args.size() < required || args.size() > fn->params.size()) {
        throw RuntimeError(line, "function '" + fn->name + "' expects " +
                                    std::to_string(required) +
                                    (required < fn->params.size() ? " to " + std::to_string(fn->params.size()) : "") +
                                    " arguments, got " + std::to_string(args.size()));
    }

    EnvPtr callEnv = std::make_shared<Env>(fn->closure);
    for (size_t i = 0; i < fn->params.size(); i++) {
        Value argVal;
        if (i < args.size()) {
            argVal = args[i];
        } else {
            argVal = fn->params[i].defaultValue->eval(callEnv, *this);
        }
        checkType(argVal, fn->params[i].type, line);
        callEnv->define(fn->params[i].name, argVal, false);
    }

    DepthGuard guard(depth);
    if (depth > 2000) throw RuntimeError(line, "maximum call depth exceeded");

    Value result = Value::Nil();
    try {
        if (fn->body) fn->body->exec(callEnv, *this);
    } catch (const ReturnSignal& r) {
        result = r.value;
    } catch (const BreakSignal&) {
        throw RuntimeError(line, "'bail' outside loop");
    } catch (const ContinueSignal&) {
        throw RuntimeError(line, "'skip' outside loop");
    }

    checkType(result, fn->returnType, line, true);
    return result;
}

void Interpreter::run(const std::vector<StmtPtr>& program) {
    try {
        for (const auto& s : program) s->exec(global, *this);
    } catch (const ReturnSignal&) {
        throw RuntimeError(0, "'->' outside function");
    } catch (const BreakSignal&) {
        throw RuntimeError(0, "'bail' outside loop");
    } catch (const ContinueSignal&) {
        throw RuntimeError(0, "'skip' outside loop");
    }
}

Interpreter::Interpreter() : rng(std::random_device{}()) {
    global = std::make_shared<Env>();

    auto def = [&](const std::string& name,
                   std::function<Value(std::vector<Value>&)> f) {
        auto nf = std::make_shared<NativeFunction>();
        nf->fn = std::move(f);
        global->define(name, Value::Native(nf), true);
    };

    def("print", [](std::vector<Value>& args) -> Value {
        for (size_t i = 0; i < args.size(); i++) {
            if (i) std::cout << ' ';
            std::cout << args[i].toString(false);
        }
        std::cout << '\n';
        return Value::Nil();
    });

    def("len", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1) throw RuntimeError(0, "len expects 1 argument");
        const Value& v = args[0];
        if (v.kind == Value::Kind::Str) return Value::Num(static_cast<double>(v.s.size()));
        if (v.kind == Value::Kind::Arr) return Value::Num(static_cast<double>(v.arr->size()));
        if (v.kind == Value::Kind::Map) {
            // Sets store a "__set" marker; records store "__record" — don't count markers
            size_t n = v.map->size();
            if (v.map->count("__set")) n--;
            if (v.map->count("__record")) n--;
            return Value::Num(static_cast<double>(n));
        }
        if (v.kind == Value::Kind::Range) {
            double s = v.rangeStart, e = v.rangeEnd;
            double st = v.rangeStep == 0 ? 1.0 : v.rangeStep;
            if (st > 0) {
                if (s > e || (!v.rangeIncl && s == e)) return Value::Num(0);
                double last = v.rangeIncl ? e : e - 1e-12;
                return Value::Num(std::max(0.0, std::floor((last - s) / st) + 1));
            } else {
                if (s < e || (!v.rangeIncl && s == e)) return Value::Num(0);
                double last = v.rangeIncl ? e : e + 1e-12;
                return Value::Num(std::max(0.0, std::floor((s - last) / (-st)) + 1));
            }
        }
        throw RuntimeError(0, "len expects a string, array, map, set, or range");
    });

    def("type", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1) throw RuntimeError(0, "type expects 1 argument");
        const Value& v = args[0];
        if (v.kind == Value::Kind::Map && v.map->count("__set")) return Value::Str("set");
        if (v.kind == Value::Kind::Map && v.map->count("__record")) {
            auto it = v.map->find("__record");
            if (it != v.map->end() && it->second.kind == Value::Kind::Str)
                return Value::Str(it->second.s);
        }
        return Value::Str(typeName(v));
    });

    def("str", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1) throw RuntimeError(0, "str expects 1 argument");
        return Value::Str(args[0].toString(false));
    });

    def("num", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1) throw RuntimeError(0, "num expects 1 argument");
        const Value& v = args[0];
        if (v.kind == Value::Kind::Num) return v;
        if (v.kind == Value::Kind::Bool) return Value::Num(v.b ? 1.0 : 0.0);
        if (v.kind == Value::Kind::Str) {
            try {
                size_t idx = 0;
                double d = std::stod(v.s, &idx);
                while (idx < v.s.size() && std::isspace(static_cast<unsigned char>(v.s[idx]))) idx++;
                if (idx != v.s.size()) throw std::invalid_argument("trailing");
                return Value::Num(d);
            } catch (...) {
                throw RuntimeError(0, "cannot convert string '" + v.s + "' to number");
            }
        }
        throw RuntimeError(0, "cannot convert " + typeName(v) + " to number");
    });

    // Soft conversion: returns nil instead of throwing on failure
    def("try_num", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1) throw RuntimeError(0, "try_num expects 1 argument");
        const Value& v = args[0];
        if (v.kind == Value::Kind::Num) return v;
        if (v.kind == Value::Kind::Bool) return Value::Num(v.b ? 1.0 : 0.0);
        if (v.kind == Value::Kind::Nil) return Value::Nil();
        if (v.kind == Value::Kind::Str) {
            if (v.s.empty()) return Value::Nil();
            try {
                size_t idx = 0;
                double d = std::stod(v.s, &idx);
                while (idx < v.s.size() && std::isspace(static_cast<unsigned char>(v.s[idx]))) idx++;
                if (idx != v.s.size()) return Value::Nil();
                return Value::Num(d);
            } catch (...) {
                return Value::Nil();
            }
        }
        return Value::Nil();
    });

    def("bool", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1) throw RuntimeError(0, "bool expects 1 argument");
        return Value::Bool(truthy(args[0]));
    });

    def("push", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2) throw RuntimeError(0, "push expects 2 arguments");
        if (args[0].kind != Value::Kind::Arr) throw RuntimeError(0, "push expects an array as first argument");
        args[0].arr->push_back(args[1]);
        return Value::Nil();
    });

    def("pop", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1) throw RuntimeError(0, "pop expects 1 argument");
        if (args[0].kind != Value::Kind::Arr) throw RuntimeError(0, "pop expects an array");
        if (args[0].arr->empty()) throw RuntimeError(0, "pop from empty array");
        Value v = args[0].arr->back();
        args[0].arr->pop_back();
        return v;
    });

    def("keys", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Map)
            throw RuntimeError(0, "keys expects a map");
        auto out = std::make_shared<std::vector<Value>>();
        for (const auto& kv : *args[0].map) out->push_back(Value::Str(kv.first));
        return Value::Arr(out);
    });

    def("values", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Map)
            throw RuntimeError(0, "values expects a map");
        auto out = std::make_shared<std::vector<Value>>();
        for (const auto& kv : *args[0].map) out->push_back(kv.second);
        return Value::Arr(out);
    });

    def("has", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Map || args[1].kind != Value::Kind::Str)
            throw RuntimeError(0, "has expects a map and a string key");
        return Value::Bool(args[0].map->count(args[1].s) > 0);
    });

    def("sum", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Arr)
            throw RuntimeError(0, "sum expects an array");
        double total = 0.0;
        for (const auto& v : *args[0].arr) {
            if (v.kind != Value::Kind::Num)
                throw RuntimeError(0, "sum expects an array of numbers");
            total += v.n;
        }
        return Value::Num(total);
    });

    def("map", [this](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Arr)
            throw RuntimeError(0, "map expects an array and a function");
        std::vector<Value> snapshot = *args[0].arr;
        auto out = std::make_shared<std::vector<Value>>();
        out->reserve(snapshot.size());
        for (auto& v : snapshot) {
            std::vector<Value> a{v};
            out->push_back(call(args[1], a, 0));
        }
        return Value::Arr(out);
    });

    def("filter", [this](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Arr)
            throw RuntimeError(0, "filter expects an array and a function");
        std::vector<Value> snapshot = *args[0].arr;
        auto out = std::make_shared<std::vector<Value>>();
        for (auto& v : snapshot) {
            std::vector<Value> a{v};
            if (truthy(call(args[1], a, 0))) out->push_back(v);
        }
        return Value::Arr(out);
    });

    def("reduce", [this](std::vector<Value>& args) -> Value {
        if (args.size() != 3 || args[0].kind != Value::Kind::Arr)
            throw RuntimeError(0, "reduce expects an array, a function, and an initial value");
        Value acc = args[2];
        std::vector<Value> snapshot = *args[0].arr;
        for (auto& v : snapshot) {
            std::vector<Value> a{acc, v};
            acc = call(args[1], a, 0);
        }
        return acc;
    });

    def("upper", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Str)
            throw RuntimeError(0, "upper expects a string");
        std::string s = args[0].s;
        for (auto& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return Value::Str(s);
    });

    def("lower", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Str)
            throw RuntimeError(0, "lower expects a string");
        std::string s = args[0].s;
        for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return Value::Str(s);
    });

    def("split", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Str || args[1].kind != Value::Kind::Str)
            throw RuntimeError(0, "split expects 2 strings");
        auto out = std::make_shared<std::vector<Value>>();
        const std::string& s = args[0].s;
        const std::string& sep = args[1].s;
        if (sep.empty()) {
            for (char c : s) out->push_back(Value::Str(std::string(1, c)));
            return Value::Arr(out);
        }
        size_t start = 0;
        size_t p;
        while ((p = s.find(sep, start)) != std::string::npos) {
            out->push_back(Value::Str(s.substr(start, p - start)));
            start = p + sep.size();
        }
        out->push_back(Value::Str(s.substr(start)));
        return Value::Arr(out);
    });

    def("join", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Arr || args[1].kind != Value::Kind::Str)
            throw RuntimeError(0, "join expects an array and a string");
        std::string r;
        for (size_t i = 0; i < args[0].arr->size(); i++) {
            if (i) r += args[1].s;
            r += (*args[0].arr)[i].toString(false);
        }
        return Value::Str(r);
    });

    def("substr", [](std::vector<Value>& args) -> Value {
        if (args.size() != 3 || args[0].kind != Value::Kind::Str ||
            args[1].kind != Value::Kind::Num || args[2].kind != Value::Kind::Num)
            throw RuntimeError(0, "substr expects a string, start, and length");
        long long start = static_cast<long long>(args[1].n);
        long long len = static_cast<long long>(args[2].n);
        const std::string& s = args[0].s;
        long long n = static_cast<long long>(s.size());
        if (start < 0) start += n;
        if (start < 0) start = 0;
        if (start > n) start = n;
        if (len < 0) len = 0;
        if (start + len > n) len = n - start;
        return Value::Str(s.substr(static_cast<size_t>(start), static_cast<size_t>(len)));
    });

    def("find", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Str || args[1].kind != Value::Kind::Str)
            throw RuntimeError(0, "find expects 2 strings");
        size_t p = args[0].s.find(args[1].s);
        return Value::Num(p == std::string::npos ? -1.0 : static_cast<double>(p));
    });

    auto addMath1 = [&](const std::string& name, std::function<double(double)> fn) {
        std::string nm = name;
        def(name, [fn, nm](std::vector<Value>& args) -> Value {
            if (args.size() != 1 || args[0].kind != Value::Kind::Num)
                throw RuntimeError(0, nm + " expects a number");
            return Value::Num(fn(args[0].n));
        });
    };

    addMath1("sqrt",  [](double x) { return std::sqrt(x); });
    addMath1("sin",   [](double x) { return std::sin(x); });
    addMath1("cos",   [](double x) { return std::cos(x); });
    addMath1("tan",   [](double x) { return std::tan(x); });
    addMath1("asin",  [](double x) { return std::asin(x); });
    addMath1("acos",  [](double x) { return std::acos(x); });
    addMath1("atan",  [](double x) { return std::atan(x); });
    addMath1("log",   [](double x) { return std::log(x); });
    addMath1("log2",  [](double x) { return std::log2(x); });
    addMath1("log10", [](double x) { return std::log10(x); });
    addMath1("exp",   [](double x) { return std::exp(x); });
    addMath1("abs",   [](double x) { return std::fabs(x); });
    addMath1("floor", [](double x) { return std::floor(x); });
    addMath1("ceil",  [](double x) { return std::ceil(x); });
    addMath1("round", [](double x) { return std::round(x); });
    addMath1("sign",  [](double x) { return (x > 0) - (x < 0); });

    def("pow", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Num || args[1].kind != Value::Kind::Num)
            throw RuntimeError(0, "pow expects 2 numbers");
        return Value::Num(std::pow(args[0].n, args[1].n));
    });

    def("atan2", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Num || args[1].kind != Value::Kind::Num)
            throw RuntimeError(0, "atan2 expects 2 numbers");
        return Value::Num(std::atan2(args[0].n, args[1].n));
    });

    def("min", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Num || args[1].kind != Value::Kind::Num)
            throw RuntimeError(0, "min expects 2 numbers");
        return Value::Num(std::min(args[0].n, args[1].n));
    });

    def("max", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Num || args[1].kind != Value::Kind::Num)
            throw RuntimeError(0, "max expects 2 numbers");
        return Value::Num(std::max(args[0].n, args[1].n));
    });

    def("random", [this](std::vector<Value>& args) -> Value {
        if (!args.empty()) throw RuntimeError(0, "random expects no arguments");
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return Value::Num(dist(rng));
    });

    def("contains", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2) throw RuntimeError(0, "contains expects 2 arguments");
        if (args[0].kind == Value::Kind::Arr) {
            for (const auto& v : *args[0].arr) {
                if (valuesEqual(v, args[1])) return Value::Bool(true);
            }
            return Value::Bool(false);
        }
        if (args[0].kind == Value::Kind::Str && args[1].kind == Value::Kind::Str) {
            return Value::Bool(args[0].s.find(args[1].s) != std::string::npos);
        }
        if (args[0].kind == Value::Kind::Map) {
            if (args[0].map->count("__set")) {
                return Value::Bool(args[0].map->count(args[1].toString(false)) > 0);
            }
            if (args[1].kind == Value::Kind::Str)
                return Value::Bool(args[0].map->count(args[1].s) > 0);
            return Value::Bool(false);
        }
        throw RuntimeError(0, "contains expects an array, string, map, or set");
    });

    def("sorted", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Arr)
            throw RuntimeError(0, "sorted expects an array");
        auto out = std::make_shared<std::vector<Value>>(*args[0].arr);
        std::sort(out->begin(), out->end(), [](const Value& a, const Value& b) {
            if (a.kind == Value::Kind::Num && b.kind == Value::Kind::Num) return a.n < b.n;
            return a.toString(false) < b.toString(false);
        });
        return Value::Arr(out);
    });

    def("reverse", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Arr)
            throw RuntimeError(0, "reverse expects an array");
        auto out = std::make_shared<std::vector<Value>>(*args[0].arr);
        std::reverse(out->begin(), out->end());
        return Value::Arr(out);
    });

    def("any", [this](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Arr)
            throw RuntimeError(0, "any expects (array, fn)");
        for (auto& v : *args[0].arr) {
            std::vector<Value> a{v};
            if (truthy(call(args[1], a, 0))) return Value::Bool(true);
        }
        return Value::Bool(false);
    });

    def("all", [this](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Arr)
            throw RuntimeError(0, "all expects (array, fn)");
        for (auto& v : *args[0].arr) {
            std::vector<Value> a{v};
            if (!truthy(call(args[1], a, 0))) return Value::Bool(false);
        }
        return Value::Bool(true);
    });

    def("zip", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Arr || args[1].kind != Value::Kind::Arr)
            throw RuntimeError(0, "zip expects two arrays");
        size_t n = std::min(args[0].arr->size(), args[1].arr->size());
        auto out = std::make_shared<std::vector<Value>>();
        for (size_t i = 0; i < n; i++) {
            auto pair = std::make_shared<std::vector<Value>>();
            pair->push_back((*args[0].arr)[i]);
            pair->push_back((*args[1].arr)[i]);
            out->push_back(Value::Arr(pair));
        }
        return Value::Arr(out);
    });

    def("range", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Num || args[1].kind != Value::Kind::Num)
            throw RuntimeError(0, "range expects two numbers");
        return Value::Range(args[0].n, args[1].n, false);
    });

    def("chars", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Str)
            throw RuntimeError(0, "chars expects a string");
        auto out = std::make_shared<std::vector<Value>>();
        for (char c : args[0].s) out->push_back(Value::Str(std::string(1, c)));
        return Value::Arr(out);
    });

    def("replace", [](std::vector<Value>& args) -> Value {
        if (args.size() != 3 || args[0].kind != Value::Kind::Str ||
            args[1].kind != Value::Kind::Str || args[2].kind != Value::Kind::Str)
            throw RuntimeError(0, "replace expects (str, from, to)");
        std::string s = args[0].s, from = args[1].s, to = args[2].s;
        if (from.empty()) return Value::Str(s);
        std::string out;
        size_t pos = 0;
        while (true) {
            size_t found = s.find(from, pos);
            if (found == std::string::npos) { out += s.substr(pos); break; }
            out += s.substr(pos, found - pos);
            out += to;
            pos = found + from.size();
        }
        return Value::Str(out);
    });

    def("trim", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Str)
            throw RuntimeError(0, "trim expects a string");
        const std::string& s = args[0].s;
        size_t start = 0, end = s.size();
        while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) start++;
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) end--;
        return Value::Str(s.substr(start, end - start));
    });

    def("starts_with", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Str || args[1].kind != Value::Kind::Str)
            throw RuntimeError(0, "starts_with expects two strings");
        return Value::Bool(args[0].s.compare(0, args[1].s.size(), args[1].s) == 0);
    });

    def("ends_with", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Str || args[1].kind != Value::Kind::Str)
            throw RuntimeError(0, "ends_with expects two strings");
        if (args[1].s.size() > args[0].s.size()) return Value::Bool(false);
        return Value::Bool(args[0].s.compare(args[0].s.size() - args[1].s.size(), args[1].s.size(), args[1].s) == 0);
    });

    // ---- Sets (represented as maps with __set marker) ----
    def("set", [](std::vector<Value>& args) -> Value {
        auto m = std::make_shared<std::map<std::string, Value>>();
        (*m)["__set"] = Value::Bool(true);
        for (auto& a : args) {
            (*m)[a.toString(false)] = a;
        }
        return Value::Map(m);
    });

    def("set_has", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Map)
            throw RuntimeError(0, "set_has expects (set, value)");
        return Value::Bool(args[0].map->count(args[1].toString(false)) > 0);
    });

    def("set_add", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Map)
            throw RuntimeError(0, "set_add expects (set, value)");
        (*args[0].map)[args[1].toString(false)] = args[1];
        return args[0];
    });

    def("set_remove", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Map)
            throw RuntimeError(0, "set_remove expects (set, value)");
        args[0].map->erase(args[1].toString(false));
        return args[0];
    });

    def("set_union", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Map || args[1].kind != Value::Kind::Map)
            throw RuntimeError(0, "set_union expects two sets");
        auto m = std::make_shared<std::map<std::string, Value>>(*args[0].map);
        for (const auto& kv : *args[1].map) (*m)[kv.first] = kv.second;
        (*m)["__set"] = Value::Bool(true);
        return Value::Map(m);
    });
    def("set_intersect", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Map || args[1].kind != Value::Kind::Map)
            throw RuntimeError(0, "set_intersect expects two sets");
        auto m = std::make_shared<std::map<std::string, Value>>();
        (*m)["__set"] = Value::Bool(true);
        for (const auto& kv : *args[0].map) {
            if (kv.first == "__set") continue;
            if (args[1].map->count(kv.first)) (*m)[kv.first] = kv.second;
        }
        return Value::Map(m);
    });
    def("set_difference", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Map || args[1].kind != Value::Kind::Map)
            throw RuntimeError(0, "set_difference expects two sets");
        auto m = std::make_shared<std::map<std::string, Value>>();
        (*m)["__set"] = Value::Bool(true);
        for (const auto& kv : *args[0].map) {
            if (kv.first == "__set") continue;
            if (!args[1].map->count(kv.first)) (*m)[kv.first] = kv.second;
        }
        return Value::Map(m);
    });

    // ---- File I/O ----
    def("read_file", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Str)
            throw RuntimeError(0, "read_file expects a path string");
        std::ifstream in(args[0].s, std::ios::binary);
        if (!in) throw RuntimeError(0, "cannot open file '" + args[0].s + "'");
        std::stringstream ss; ss << in.rdbuf();
        return Value::Str(ss.str());
    });

    def("write_file", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Str || args[1].kind != Value::Kind::Str)
            throw RuntimeError(0, "write_file expects (path, content)");
        std::ofstream out(args[0].s, std::ios::binary);
        if (!out) throw RuntimeError(0, "cannot write file '" + args[0].s + "'");
        out << args[1].s;
        return Value::Nil();
    });

    def("append_file", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Str || args[1].kind != Value::Kind::Str)
            throw RuntimeError(0, "append_file expects (path, content)");
        std::ofstream out(args[0].s, std::ios::binary | std::ios::app);
        if (!out) throw RuntimeError(0, "cannot append file '" + args[0].s + "'");
        out << args[1].s;
        return Value::Nil();
    });

    def("list_dir", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Str)
            throw RuntimeError(0, "list_dir expects a path string");
        DIR* d = opendir(args[0].s.c_str());
        if (!d) throw RuntimeError(0, "cannot open directory '" + args[0].s + "'");
        auto out = std::make_shared<std::vector<Value>>();
        while (dirent* ent = readdir(d)) {
            std::string n = ent->d_name;
            if (n == "." || n == "..") continue;
            out->push_back(Value::Str(n));
        }
        closedir(d);
        return Value::Arr(out);
    });

    def("base64_encode", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Str)
            throw RuntimeError(0, "base64_encode expects a string");
        static const char* T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        const std::string& in = args[0].s;
        std::string out;
        int val = 0, valb = -6;
        for (unsigned char c : in) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                out.push_back(T[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back(T[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return Value::Str(out);
    });

    def("hex_encode", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Str)
            throw RuntimeError(0, "hex_encode expects a string");
        static const char* H = "0123456789abcdef";
        std::string out;
        for (unsigned char c : args[0].s) {
            out.push_back(H[c >> 4]);
            out.push_back(H[c & 0xF]);
        }
        return Value::Str(out);
    });

    // ---- JSON (minimal) ----
    def("json_stringify", [](std::vector<Value>& args) -> Value {
        if (args.size() < 1) throw RuntimeError(0, "json_stringify expects a value");
        std::function<std::string(const Value&)> ser;
        ser = [&](const Value& v) -> std::string {
            switch (v.kind) {
                case Value::Kind::Nil: return "null";
                case Value::Kind::Bool: return v.b ? "true" : "false";
                case Value::Kind::Num: {
                    std::ostringstream o; o << std::setprecision(15) << v.n; return o.str();
                }
                case Value::Kind::Str: {
                    std::string r = "\"";
                    for (char c : v.s) {
                        if (c == '"') r += "\\\"";
                        else if (c == '\\') r += "\\\\";
                        else if (c == '\n') r += "\\n";
                        else if (c == '\t') r += "\\t";
                        else r += c;
                    }
                    return r + "\"";
                }
                case Value::Kind::Arr: {
                    std::string r = "[";
                    for (size_t i = 0; i < v.arr->size(); i++) {
                        if (i) r += ",";
                        r += ser((*v.arr)[i]);
                    }
                    return r + "]";
                }
                case Value::Kind::Map: {
                    std::string r = "{";
                    bool first = true;
                    for (const auto& kv : *v.map) {
                        if (kv.first.size() >= 2 && kv.first[0] == '_' && kv.first[1] == '_') continue;
                        if (!first) r += ",";
                        first = false;
                        r += ser(Value::Str(kv.first)) + ":" + ser(kv.second);
                    }
                    return r + "}";
                }
                default: return "null";
            }
        };
        return Value::Str(ser(args[0]));
    });

    def("json_parse", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Str)
            throw RuntimeError(0, "json_parse expects a string");
        const std::string& s = args[0].s;
        size_t i = 0;
        auto skip = [&]() { while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) i++; };
        std::function<Value()> parse;
        parse = [&]() -> Value {
            skip();
            if (i >= s.size()) throw RuntimeError(0, "json: unexpected end");
            if (s[i] == 'n') { i += 4; return Value::Nil(); }
            if (s[i] == 't') { i += 4; return Value::Bool(true); }
            if (s[i] == 'f') { i += 5; return Value::Bool(false); }
            if (s[i] == '"') {
                i++;
                std::string out;
                while (i < s.size() && s[i] != '"') {
                    if (s[i] == '\\' && i + 1 < s.size()) {
                        char e = s[++i];
                        if (e == 'n') out += '\n';
                        else if (e == 't') out += '\t';
                        else out += e;
                        i++;
                    } else out += s[i++];
                }
                if (i < s.size()) i++;
                return Value::Str(out);
            }
            if (s[i] == '[') {
                i++;
                auto arr = std::make_shared<std::vector<Value>>();
                skip();
                if (i < s.size() && s[i] == ']') { i++; return Value::Arr(arr); }
                while (true) {
                    arr->push_back(parse());
                    skip();
                    if (i < s.size() && s[i] == ',') { i++; continue; }
                    break;
                }
                skip();
                if (i >= s.size() || s[i] != ']') throw RuntimeError(0, "json: expected ]");
                i++;
                return Value::Arr(arr);
            }
            if (s[i] == '{') {
                i++;
                auto m = std::make_shared<std::map<std::string, Value>>();
                skip();
                if (i < s.size() && s[i] == '}') { i++; return Value::Map(m); }
                while (true) {
                    skip();
                    Value k = parse();
                    if (k.kind != Value::Kind::Str) throw RuntimeError(0, "json: object key must be string");
                    skip();
                    if (i >= s.size() || s[i] != ':') throw RuntimeError(0, "json: expected :");
                    i++;
                    (*m)[k.s] = parse();
                    skip();
                    if (i < s.size() && s[i] == ',') { i++; continue; }
                    break;
                }
                skip();
                if (i >= s.size() || s[i] != '}') throw RuntimeError(0, "json: expected }");
                i++;
                return Value::Map(m);
            }
            // number
            size_t start = i;
            if (s[i] == '-') i++;
            while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.' || s[i] == 'e' || s[i] == 'E' || s[i] == '+' || s[i] == '-')) i++;
            try { return Value::Num(std::stod(s.substr(start, i - start))); }
            catch (...) { throw RuntimeError(0, "json: invalid number"); }
        };
        return parse();
    });

    // ---- More collections ----
    def("flatten", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Arr)
            throw RuntimeError(0, "flatten expects an array");
        auto out = std::make_shared<std::vector<Value>>();
        for (const auto& v : *args[0].arr) {
            if (v.kind == Value::Kind::Arr) {
                for (const auto& x : *v.arr) out->push_back(x);
            } else out->push_back(v);
        }
        return Value::Arr(out);
    });

    def("unique", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Arr)
            throw RuntimeError(0, "unique expects an array");
        auto out = std::make_shared<std::vector<Value>>();
        std::unordered_set<std::string> seen;
        for (const auto& v : *args[0].arr) {
            std::string k = v.toString(false);
            if (seen.insert(k).second) out->push_back(v);
        }
        return Value::Arr(out);
    });

    def("concat", [](std::vector<Value>& args) -> Value {
        auto out = std::make_shared<std::vector<Value>>();
        for (auto& a : args) {
            if (a.kind == Value::Kind::Arr) {
                for (const auto& x : *a.arr) out->push_back(x);
            } else out->push_back(a);
        }
        return Value::Arr(out);
    });

    def("enumerate", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Arr)
            throw RuntimeError(0, "enumerate expects an array");
        auto out = std::make_shared<std::vector<Value>>();
        for (size_t i = 0; i < args[0].arr->size(); i++) {
            auto pair = std::make_shared<std::vector<Value>>();
            pair->push_back(Value::Num(static_cast<double>(i)));
            pair->push_back((*args[0].arr)[i]);
            out->push_back(Value::Arr(pair));
        }
        return Value::Arr(out);
    });

    def("chunks", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Arr || args[1].kind != Value::Kind::Num)
            throw RuntimeError(0, "chunks expects (array, size)");
        size_t n = static_cast<size_t>(args[1].n);
        if (n == 0) throw RuntimeError(0, "chunks size must be > 0");
        auto out = std::make_shared<std::vector<Value>>();
        for (size_t i = 0; i < args[0].arr->size(); i += n) {
            auto chunk = std::make_shared<std::vector<Value>>();
            for (size_t j = i; j < i + n && j < args[0].arr->size(); j++)
                chunk->push_back((*args[0].arr)[j]);
            out->push_back(Value::Arr(chunk));
        }
        return Value::Arr(out);
    });

    def("eprint", [](std::vector<Value>& args) -> Value {
        for (size_t i = 0; i < args.size(); i++) {
            if (i) std::cerr << ' ';
            std::cerr << args[i].toString(false);
        }
        std::cerr << '\n';
        return Value::Nil();
    });

    def("readline", [](std::vector<Value>& args) -> Value {
        if (!args.empty()) std::cout << args[0].toString(false) << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) return Value::Nil();
        return Value::Str(line);
    });

    def("gcd", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Num || args[1].kind != Value::Kind::Num)
            throw RuntimeError(0, "gcd expects two numbers");
        long long a = std::abs(static_cast<long long>(args[0].n));
        long long b = std::abs(static_cast<long long>(args[1].n));
        while (b) { long long t = b; b = a % b; a = t; }
        return Value::Num(static_cast<double>(a));
    });

    def("lcm", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Num || args[1].kind != Value::Kind::Num)
            throw RuntimeError(0, "lcm expects two numbers");
        long long a = std::abs(static_cast<long long>(args[0].n));
        long long b = std::abs(static_cast<long long>(args[1].n));
        long long g = a, bb = b;
        while (bb) { long long t = bb; bb = g % bb; g = t; }
        if (g == 0) return Value::Num(0);
        return Value::Num(static_cast<double>((a / g) * b));
    });

    def("is_prime", [](std::vector<Value>& args) -> Value {
        if (args.size() != 1 || args[0].kind != Value::Kind::Num)
            throw RuntimeError(0, "is_prime expects a number");
        long long n = static_cast<long long>(args[0].n);
        if (n < 2) return Value::Bool(false);
        if (n < 4) return Value::Bool(true);
        if (n % 2 == 0 || n % 3 == 0) return Value::Bool(false);
        for (long long i = 5; i * i <= n; i += 6)
            if (n % i == 0 || n % (i + 2) == 0) return Value::Bool(false);
        return Value::Bool(true);
    });

    def("now", [](std::vector<Value>& args) -> Value {
        (void)args;
        using namespace std::chrono;
        auto ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        return Value::Num(static_cast<double>(ms));
    });

    def("partition", [this](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Arr)
            throw RuntimeError(0, "partition expects (array, fn)");
        auto yes = std::make_shared<std::vector<Value>>();
        auto no = std::make_shared<std::vector<Value>>();
        for (auto& v : *args[0].arr) {
            std::vector<Value> a{v};
            if (truthy(call(args[1], a, 0))) yes->push_back(v);
            else no->push_back(v);
        }
        auto out = std::make_shared<std::vector<Value>>();
        out->push_back(Value::Arr(yes));
        out->push_back(Value::Arr(no));
        return Value::Arr(out);
    });

    def("windows", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Arr || args[1].kind != Value::Kind::Num)
            throw RuntimeError(0, "windows expects (array, size)");
        long long n = static_cast<long long>(args[1].n);
        if (n <= 0) throw RuntimeError(0, "windows size must be > 0");
        auto out = std::make_shared<std::vector<Value>>();
        long long len = static_cast<long long>(args[0].arr->size());
        for (long long i = 0; i + n <= len; i++) {
            auto w = std::make_shared<std::vector<Value>>();
            for (long long j = 0; j < n; j++) w->push_back((*args[0].arr)[static_cast<size_t>(i + j)]);
            out->push_back(Value::Arr(w));
        }
        return Value::Arr(out);
    });

    def("group_by", [this](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Arr)
            throw RuntimeError(0, "group_by expects (array, fn)");
        auto m = std::make_shared<std::map<std::string, Value>>();
        for (auto& v : *args[0].arr) {
            std::vector<Value> a{v};
            Value key = call(args[1], a, 0);
            std::string ks = key.toString(false);
            if (!m->count(ks)) {
                (*m)[ks] = Value::Arr(std::make_shared<std::vector<Value>>());
            }
            (*m)[ks].arr->push_back(v);
        }
        return Value::Map(m);
    });

    def("re_match", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Str || args[1].kind != Value::Kind::Str)
            throw RuntimeError(0, "re_match expects (pattern, string)");
        try {
            std::regex re(args[0].s);
            return Value::Bool(std::regex_search(args[1].s, re));
        } catch (const std::regex_error& e) {
            throw RuntimeError(0, std::string("invalid regex: ") + e.what());
        }
    });

    def("re_find_all", [](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].kind != Value::Kind::Str || args[1].kind != Value::Kind::Str)
            throw RuntimeError(0, "re_find_all expects (pattern, string)");
        try {
            std::regex re(args[0].s);
            auto out = std::make_shared<std::vector<Value>>();
            auto begin = std::sregex_iterator(args[1].s.begin(), args[1].s.end(), re);
            auto end = std::sregex_iterator();
            for (auto it = begin; it != end; ++it) out->push_back(Value::Str(it->str()));
            return Value::Arr(out);
        } catch (const std::regex_error& e) {
            throw RuntimeError(0, std::string("invalid regex: ") + e.what());
        }
    });

    def("re_replace", [](std::vector<Value>& args) -> Value {
        if (args.size() != 3 || args[0].kind != Value::Kind::Str ||
            args[1].kind != Value::Kind::Str || args[2].kind != Value::Kind::Str)
            throw RuntimeError(0, "re_replace expects (pattern, string, replacement)");
        try {
            std::regex re(args[0].s);
            return Value::Str(std::regex_replace(args[1].s, re, args[2].s));
        } catch (const std::regex_error& e) {
            throw RuntimeError(0, std::string("invalid regex: ") + e.what());
        }
    });

    global->define("PI",  Value::Num(3.14159265358979323846), true);
    global->define("E",   Value::Num(2.71828182845904523536), true);
    global->define("TAU", Value::Num(6.28318530717958647692), true);
    global->define("INF", Value::Num(std::numeric_limits<double>::infinity()), true);
}

// ---------------------------------------------------------------------------
// Parser
// ---------------------------------------------------------------------------
class Parser {
public:
    explicit Parser(std::vector<Token> t) : toks(std::move(t)) {}

    std::vector<StmtPtr> parseProgram() {
        std::vector<StmtPtr> prog;
        while (!check(Tok::Eof)) {
            if (check(Tok::Semi)) { advance(); continue; }
            prog.push_back(statement());
        }
        return prog;
    }

    ExprPtr parseExpression() { return expression(); }

private:
    std::vector<Token> toks;
    size_t pos = 0;

    const Token& peek(size_t off = 0) const {
        size_t p = pos + off;
        if (p >= toks.size()) return toks.back();
        return toks[p];
    }

    bool check(Tok t) const { return peek().type == t; }
    bool match(Tok t) { if (check(t)) { pos++; return true; } return false; }

    Token advance() {
        Token t = peek();
        if (pos < toks.size()) pos++;
        return t;
    }

    int prevLine() const { return pos > 0 ? toks[pos - 1].line : 1; }

    Token expect(Tok t, const std::string& msg) {
        if (!check(t)) throw ParseError(peek().line, msg);
        return advance();
    }

    std::string typeName() {
        Token t = expect(Tok::Ident, "expected type name");
        return t.lexeme;
    }

    std::shared_ptr<BlockStmt> block() {
        int l = peek().line;
        expect(Tok::LBrace, "expected '{'");
        std::vector<StmtPtr> stmts;
        while (!check(Tok::RBrace) && !check(Tok::Eof)) {
            if (check(Tok::Semi)) { advance(); continue; }
            stmts.push_back(statement());
        }
        expect(Tok::RBrace, "expected '}'");
        return std::make_shared<BlockStmt>(std::move(stmts), l);
    }

    StmtPtr statement() {
        if (check(Tok::When))    { advance(); return whenStmt(); }
        if (check(Tok::Unless)) {
            int l = advance().line;
            ExprPtr cond = expression();
            auto body = block();
            // unless cond { body }  =>  when !cond { body }
            auto notCond = std::make_shared<UnaryExpr>("!", cond, l);
            return std::make_shared<IfStmt>(notCond, body, nullptr, l);
        }
        if (check(Tok::Until)) {
            int l = advance().line;
            ExprPtr cond = expression();
            auto body = block();
            // until cond { body }  =>  loop !cond { body }
            auto notCond = std::make_shared<UnaryExpr>("!", cond, l);
            return std::make_shared<WhileStmt>(notCond, body, l);
        }
        if (check(Tok::Loop))    { advance(); return loopStmt(); }
        if (check(Tok::Repeat))  { advance(); return repeatStmt(); }
        if (check(Tok::ReturnArrow)) { advance(); return returnStmt(); }
        if (check(Tok::Bail)) {
            int l = advance().line;
            expect(Tok::Semi, "expected ';' after 'bail'");
            return std::make_shared<BreakStmt>(l);
        }
        if (check(Tok::Skip)) {
            int l = advance().line;
            expect(Tok::Semi, "expected ';' after 'skip'");
            return std::make_shared<ContinueStmt>(l);
        }
        if (check(Tok::Assert)) {
            int l = advance().line;
            ExprPtr cond = expression();
            ExprPtr msg = nullptr;
            if (match(Tok::Comma)) msg = expression();
            expect(Tok::Semi, "expected ';' after assert");
            return std::make_shared<AssertStmt>(cond, msg, l);
        }
        if (check(Tok::Defer)) {
            int l = advance().line;
            // defer accepts a full statement (bind or expression)
            StmtPtr body;
            if (auto bd = tryBind()) {
                body = bd;
            } else {
                ExprPtr e = expression();
                expect(Tok::Semi, "expected ';' after defer");
                body = std::make_shared<ExprStmt>(e, l);
            }
            return std::make_shared<DeferStmt>(body, l);
        }
        if (check(Tok::Throw)) {
            int l = advance().line;
            ExprPtr v = expression();
            expect(Tok::Semi, "expected ';' after throw");
            return std::make_shared<ThrowStmt>(v, l);
        }
        if (check(Tok::Try)) {
            int l = advance().line;
            auto tb = block();
            expect(Tok::Catch, "expected 'catch' after try block");
            std::string errVar;
            if (check(Tok::Ident)) errVar = advance().lexeme;
            auto cb = block();
            return std::make_shared<TryCatchStmt>(tb, errVar, cb, l);
        }
        if (check(Tok::Record)) {
            int l = advance().line;
            Token name = expect(Tok::Ident, "expected record name");
            expect(Tok::LBrace, "expected '{' after record name");
            std::vector<std::pair<std::string, std::string>> fields;
            while (!check(Tok::RBrace) && !check(Tok::Eof)) {
                Token fname = expect(Tok::Ident, "expected field name");
                expect(Tok::Colon, "expected ':' after field name");
                Token ftype = expect(Tok::Ident, "expected field type");
                fields.push_back({fname.lexeme, ftype.lexeme});
                if (!match(Tok::Comma)) break;
            }
            expect(Tok::RBrace, "expected '}' after record body");
            return std::make_shared<RecordDeclStmt>(name.lexeme, std::move(fields), l);
        }
        if (check(Tok::Enum)) {
            int l = advance().line;
            Token name = expect(Tok::Ident, "expected enum name");
            expect(Tok::LBrace, "expected '{' after enum name");
            std::vector<std::string> variants;
            while (!check(Tok::RBrace) && !check(Tok::Eof)) {
                variants.push_back(expect(Tok::Ident, "expected variant name").lexeme);
                if (!match(Tok::Comma)) break;
            }
            expect(Tok::RBrace, "expected '}' after enum body");
            // Expand to: Color := %{ Red: "Color.Red", ... }; + register type
            auto m = std::make_shared<std::map<std::string, Value>>();
            for (const auto& v : variants) {
                (*m)[v] = Value::Str(name.lexeme + "." + v);
            }
            (*m)["__enum"] = Value::Str(name.lexeme);
            auto mapVal = Value::Map(m);
            // Store as a bind statement
            return std::make_shared<BindStmt>(
                std::make_shared<VarExpr>(name.lexeme, l),
                "any", ":=",
                std::make_shared<LiteralExpr>(mapVal, l),
                l);
        }
        if (check(Tok::Import)) {
            int l = advance().line;
            Token pathTok = expect(Tok::String, "expected string path after 'import'");
            std::string asName;
            if (match(Tok::As)) asName = expect(Tok::Ident, "expected identifier after 'as'").lexeme;
            expect(Tok::Semi, "expected ';' after import");
            return std::make_shared<ImportStmt>(pathTok.str, asName, l);
        }
        if (check(Tok::LBrace)) return block();

        if (auto fd = tryFuncDecl()) return fd;
        if (auto bd = tryBind()) return bd;

        int l = peek().line;
        ExprPtr e = expression();
        expect(Tok::Semi, "expected ';' after expression");
        return std::make_shared<ExprStmt>(e, l);
    }

    StmtPtr tryFuncDecl() {
        size_t save = pos;
        if (!check(Tok::Ident)) return nullptr;
        Token name = advance();
        if (!check(Tok::LParen)) { pos = save; return nullptr; }
        advance();

        std::vector<Param> params;
        bool ok = true;
        if (!check(Tok::RParen)) {
            while (true) {
                if (!check(Tok::Ident)) { ok = false; break; }
                Token pn = advance();
                Param p;
                p.name = pn.lexeme;
                if (check(Tok::Colon)) {
                    advance();
                    if (!check(Tok::Ident)) { ok = false; break; }
                    p.type = advance().lexeme;
                }
                if (check(Tok::Define)) {
                    advance();
                    p.defaultValue = expression();
                }
                params.push_back(std::move(p));
                if (check(Tok::Comma)) { advance(); continue; }
                break;
            }
        }
        if (!ok || !check(Tok::RParen)) { pos = save; return nullptr; }
        advance();

        std::string rt = "any";
        if (check(Tok::Colon)) {
            advance();
            if (!check(Tok::Ident)) { pos = save; return nullptr; }
            rt = advance().lexeme;
        }
        if (!check(Tok::DefineFunc)) { pos = save; return nullptr; }
        advance();

        if (check(Tok::LBrace)) {
            auto blk = block();
            return std::make_shared<FuncDeclStmt>(name.lexeme, std::move(params), std::move(rt), blk, name.line);
        }
        ExprPtr expr = expression();
        expect(Tok::Semi, "expected ';' after function body");
        std::vector<StmtPtr> stmts;
        stmts.push_back(std::make_shared<ReturnStmt>(expr, name.line));
        auto blk = std::make_shared<BlockStmt>(std::move(stmts), name.line);
        return std::make_shared<FuncDeclStmt>(name.lexeme, std::move(params), std::move(rt), blk, name.line);
    }

    StmtPtr tryBind() {
        size_t save = pos;

        // Array destructuring: [a, b] := arr;  [first, ...rest] := arr;
        if (check(Tok::LBracket)) {
            int l = peek().line;
            advance();
            std::vector<std::string> names;
            std::string restName;
            if (!check(Tok::RBracket)) {
                do {
                    if (check(Tok::DotDotDot)) {
                        advance();
                        restName = expect(Tok::Ident, "expected name after '...'").lexeme;
                        match(Tok::Comma);
                        break;
                    }
                    names.push_back(expect(Tok::Ident, "expected name in destructure").lexeme);
                } while (match(Tok::Comma) && !check(Tok::RBracket));
            }
            expect(Tok::RBracket, "expected ']'");
            if (!check(Tok::Define) && !check(Tok::Bind)) { pos = save; return nullptr; }
            Token op = advance();
            ExprPtr val = expression();
            expect(Tok::Semi, "expected ';' after binding");
            // Expand into multiple binds via a small block
            std::vector<StmtPtr> stmts;
            // tmp := val
            std::string tmp = "__des" + std::to_string(l);
            stmts.push_back(std::make_shared<BindStmt>(
                std::make_shared<VarExpr>(tmp, l), "any", op.lexeme, val, l));
            for (size_t i = 0; i < names.size(); i++) {
                auto idx = std::make_shared<LiteralExpr>(Value::Num(static_cast<double>(i)), l);
                auto access = std::make_shared<IndexExpr>(
                    std::make_shared<VarExpr>(tmp, l), idx, l);
                stmts.push_back(std::make_shared<BindStmt>(
                    std::make_shared<VarExpr>(names[i], l), "any", op.lexeme, access, l));
            }
            if (!restName.empty()) {
                auto start = std::make_shared<LiteralExpr>(Value::Num(static_cast<double>(names.size())), l);
                auto slice = std::make_shared<SliceExpr>(
                    std::make_shared<VarExpr>(tmp, l), start, nullptr, nullptr, l);
                stmts.push_back(std::make_shared<BindStmt>(
                    std::make_shared<VarExpr>(restName, l), "any", op.lexeme, slice, l));
            }
            return std::make_shared<BlockStmt>(std::move(stmts), -1);
        }

        // Map destructuring: %{"name": name, "age": age} := person;
        if (check(Tok::MapOpen)) {
            int l = peek().line;
            advance();
            std::vector<std::pair<std::string, std::string>> bindings; // key -> var
            if (!check(Tok::RBrace)) {
                do {
                    std::string key;
                    if (check(Tok::String)) {
                        key = advance().str;
                    } else if (check(Tok::Ident)) {
                        key = advance().lexeme;
                    } else { pos = save; return nullptr; }
                    expect(Tok::Colon, "expected ':' in map destructure");
                    std::string var = expect(Tok::Ident, "expected variable name").lexeme;
                    bindings.push_back({key, var});
                } while (match(Tok::Comma) && !check(Tok::RBrace));
            }
            expect(Tok::RBrace, "expected '}'");
            if (!check(Tok::Define) && !check(Tok::Bind)) { pos = save; return nullptr; }
            Token op = advance();
            ExprPtr val = expression();
            expect(Tok::Semi, "expected ';' after binding");
            std::vector<StmtPtr> stmts;
            std::string tmp = "__desm" + std::to_string(l);
            stmts.push_back(std::make_shared<BindStmt>(
                std::make_shared<VarExpr>(tmp, l), "any", op.lexeme, val, l));
            for (const auto& b : bindings) {
                auto keyLit = std::make_shared<LiteralExpr>(Value::Str(b.first), l);
                auto access = std::make_shared<IndexExpr>(
                    std::make_shared<VarExpr>(tmp, l), keyLit, l);
                stmts.push_back(std::make_shared<BindStmt>(
                    std::make_shared<VarExpr>(b.second, l), "any", op.lexeme, access, l));
            }
            return std::make_shared<BlockStmt>(std::move(stmts), -1);
        }

        if (!check(Tok::Ident)) return nullptr;
        Token name = advance();
        ExprPtr target = std::make_shared<VarExpr>(name.lexeme, name.line);
        bool hasIndex = false;
        while (true) {
            if (check(Tok::LBracket)) {
                int bl = advance().line;
                if (check(Tok::Colon)) {
                    advance();
                    ExprPtr endE = nullptr;
                    ExprPtr stepE = nullptr;
                    if (!check(Tok::RBracket) && !check(Tok::Colon)) endE = expression();
                    if (match(Tok::Colon)) {
                        if (!check(Tok::RBracket)) stepE = expression();
                    }
                    expect(Tok::RBracket, "expected ']'");
                    target = std::make_shared<SliceExpr>(target, nullptr, endE, stepE, bl);
                } else {
                    ExprPtr idx = expression();
                    if (match(Tok::Colon)) {
                        ExprPtr endE = nullptr;
                        ExprPtr stepE = nullptr;
                        if (!check(Tok::RBracket) && !check(Tok::Colon)) endE = expression();
                        if (match(Tok::Colon)) {
                            if (!check(Tok::RBracket)) stepE = expression();
                        }
                        expect(Tok::RBracket, "expected ']'");
                        target = std::make_shared<SliceExpr>(target, idx, endE, stepE, bl);
                    } else {
                        expect(Tok::RBracket, "expected ']'");
                        target = std::make_shared<IndexExpr>(target, idx, bl);
                    }
                }
                hasIndex = true;
            } else if (check(Tok::Dot)) {
                advance();
                Token fname = expect(Tok::Ident, "expected field name");
                target = std::make_shared<FieldExpr>(target, fname.lexeme, false, name.line);
                hasIndex = true;
            } else break;
        }

        std::string type = "any";
        if (!hasIndex && check(Tok::Colon)) {
            advance();
            if (!check(Tok::Ident)) { pos = save; return nullptr; }
            type = advance().lexeme;
        }

        if (check(Tok::Define) || check(Tok::Bind)) {
            Token op = advance();
            ExprPtr val = expression();
            expect(Tok::Semi, "expected ';' after binding");
            return std::make_shared<BindStmt>(target, std::move(type), op.lexeme, val, name.line);
        }

        pos = save;
        return nullptr;
    }

    StmtPtr whenStmt() {
        int l = prevLine();
        ExprPtr cond = expression();
        auto thenB = block();
        StmtPtr elseB = nullptr;
        if (match(Tok::Else)) {
            if (check(Tok::When)) { advance(); elseB = whenStmt(); }
            else elseB = block();
        }
        return std::make_shared<IfStmt>(cond, thenB, elseB, l);
    }

    StmtPtr loopStmt() {
        int l = prevLine();
        ExprPtr cond;
        if (check(Tok::LBrace)) {
            // infinite loop: loop { ... }
            cond = std::make_shared<LiteralExpr>(Value::Bool(true), l);
        } else {
            cond = expression();
        }
        auto body = block();
        return std::make_shared<WhileStmt>(cond, body, l);
    }

    StmtPtr repeatStmt() {
        int l = prevLine();
        // Destructure form: repeat [a, b] in items { }
        if (check(Tok::LBracket)) {
            advance();
            std::vector<std::string> names;
            if (!check(Tok::RBracket)) {
                do {
                    names.push_back(expect(Tok::Ident, "expected name in repeat destructure").lexeme);
                } while (match(Tok::Comma) && !check(Tok::RBracket));
            }
            expect(Tok::RBracket, "expected ']'");
            expect(Tok::In, "expected 'in' after repeat destructure");
            ExprPtr iterable = expression();
            auto body = block();
            return std::make_shared<ForEachStmt>(std::move(names), iterable, body, l);
        }
        Token var = expect(Tok::Ident, "expected loop variable");

        if (match(Tok::From)) {
            // Use logicOr to avoid rangeOp consuming 'to'/'through'
            ExprPtr start = logicOr();
            bool inclusive = false;
            if (match(Tok::To)) inclusive = false;
            else if (match(Tok::Through)) inclusive = true;
            else throw ParseError(peek().line, "expected 'to' or 'through'");
            ExprPtr end = logicOr();
            ExprPtr stepExpr = nullptr;
            if (match(Tok::By)) stepExpr = logicOr();
            auto body = block();
            return std::make_shared<ForRangeStmt>(var.lexeme, start, end, inclusive, body, l, stepExpr);
        }
        if (match(Tok::In)) {
            ExprPtr iterable = expression();
            auto body = block();
            return std::make_shared<ForEachStmt>(var.lexeme, iterable, body, l);
        }
        throw ParseError(peek().line, "expected 'from' or 'in' after 'repeat'");
    }

    StmtPtr returnStmt() {
        int l = prevLine();
        ExprPtr v = nullptr;
        if (!check(Tok::Semi)) v = expression();
        expect(Tok::Semi, "expected ';' after '->'");
        return std::make_shared<ReturnStmt>(v, l);
    }

    ExprPtr expression() { return assignment(); }

    ExprPtr assignment() {
        ExprPtr e = ternary();
        if (check(Tok::PlusEq) || check(Tok::MinusEq) || check(Tok::StarEq) ||
            check(Tok::SlashEq) || check(Tok::PercentEq)) {
            Token op = advance();
            ExprPtr v = assignment();
            if (!dynamic_cast<VarExpr*>(e.get()) && !dynamic_cast<IndexExpr*>(e.get())) {
                throw ParseError(op.line, "invalid assignment target");
            }
            return std::make_shared<AssignExpr>(e, op.lexeme, v, op.line);
        }
        return e;
    }

    ExprPtr ternary() {
        ExprPtr c = nullCoal();
        if (match(Tok::Question)) {
            int l = prevLine();
            ExprPtr t = expression();
            expect(Tok::Colon, "expected ':' in ternary expression");
            ExprPtr e = ternary();
            return std::make_shared<TernaryExpr>(c, t, e, l);
        }
        return c;
    }

    ExprPtr nullCoal() {
        ExprPtr e = rangeOp();
        while (match(Tok::NullCoal)) {
            int l = prevLine();
            ExprPtr r = rangeOp();
            e = std::make_shared<BinaryExpr>("??", e, r, l);
        }
        return e;
    }

    ExprPtr rangeOp() {
        ExprPtr e = logicOr();
        if (check(Tok::To) || check(Tok::Through)) {
            bool inc = check(Tok::Through);
            advance();
            ExprPtr r = logicOr();
            ExprPtr st = nullptr;
            if (match(Tok::By)) st = logicOr();
            return std::make_shared<RangeExpr>(e, r, inc, st, e->line);
        }
        return e;
    }

    ExprPtr logicOr() {
        ExprPtr e = logicAnd();
        while (match(Tok::OrOr)) {
            int l = prevLine();
            ExprPtr r = logicAnd();
            e = std::make_shared<BinaryExpr>("||", e, r, l);
        }
        return e;
    }

    ExprPtr logicAnd() {
        ExprPtr e = bitOr();
        while (match(Tok::AndAnd)) {
            int l = prevLine();
            ExprPtr r = bitOr();
            e = std::make_shared<BinaryExpr>("&&", e, r, l);
        }
        return e;
    }

    ExprPtr bitOr() {
        ExprPtr e = bitXor();
        while (match(Tok::Pipe)) {
            int l = prevLine();
            ExprPtr r = bitXor();
            e = std::make_shared<BinaryExpr>("|", e, r, l);
        }
        return e;
    }

    ExprPtr bitXor() {
        ExprPtr e = bitAnd();
        while (match(Tok::Caret)) {
            int l = prevLine();
            ExprPtr r = bitAnd();
            e = std::make_shared<BinaryExpr>("^", e, r, l);
        }
        return e;
    }

    ExprPtr bitAnd() {
        ExprPtr e = equality();
        while (match(Tok::Amp)) {
            int l = prevLine();
            ExprPtr r = equality();
            e = std::make_shared<BinaryExpr>("&", e, r, l);
        }
        return e;
    }

    ExprPtr equality() {
        ExprPtr e = pipeOp();
        while (check(Tok::EqEq) || check(Tok::BangEq)) {
            Token op = advance();
            ExprPtr r = pipeOp();
            e = std::make_shared<BinaryExpr>(op.lexeme, e, r, op.line);
        }
        return e;
    }

    // Pipe binds tighter than == so:  x |> f == y  =>  (x |> f) == y
    ExprPtr pipeOp() {
        ExprPtr e = comparison();
        while (match(Tok::PipeOp)) {
            int l = prevLine();
            ExprPtr r = comparison();
            e = std::make_shared<PipeExpr>(e, r, l);
        }
        return e;
    }

    ExprPtr comparison() {
        ExprPtr e = isLevel();
        while (check(Tok::Lt) || check(Tok::LtEq) || check(Tok::Gt) || check(Tok::GtEq)) {
            Token op = advance();
            ExprPtr r = isLevel();
            e = std::make_shared<BinaryExpr>(op.lexeme, e, r, op.line);
        }
        return e;
    }

    ExprPtr isLevel() {
        ExprPtr e = shift();
        while (match(Tok::Is)) {
            std::string tn;
            int ln = peek().line;
            if (check(Tok::Ident)) tn = advance().lexeme;
            else if (check(Tok::Nil)) { advance(); tn = "nil"; }
            else if (check(Tok::True) || check(Tok::False)) {
                throw ParseError(peek().line, "expected type name after 'is'");
            } else {
                throw ParseError(peek().line, "expected type name after 'is'");
            }
            e = std::make_shared<IsExpr>(e, tn, ln);
        }
        return e;
    }

    ExprPtr shift() {
        ExprPtr e = additive();
        while (check(Tok::Shl) || check(Tok::Shr)) {
            Token op = advance();
            ExprPtr r = additive();
            e = std::make_shared<BinaryExpr>(op.lexeme, e, r, op.line);
        }
        return e;
    }

    ExprPtr additive() {
        ExprPtr e = multiplicative();
        while (check(Tok::Plus) || check(Tok::Minus)) {
            Token op = advance();
            ExprPtr r = multiplicative();
            e = std::make_shared<BinaryExpr>(op.lexeme, e, r, op.line);
        }
        return e;
    }

    ExprPtr multiplicative() {
        ExprPtr e = unary();
        while (check(Tok::Star) || check(Tok::Slash) || check(Tok::Percent)) {
            Token op = advance();
            ExprPtr r = unary();
            e = std::make_shared<BinaryExpr>(op.lexeme, e, r, op.line);
        }
        return e;
    }

    ExprPtr unary() {
        if (check(Tok::Minus) || check(Tok::Bang) || check(Tok::Tilde)) {
            Token op = advance();
            ExprPtr e = unary();
            return std::make_shared<UnaryExpr>(op.lexeme, e, op.line);
        }
        return power();
    }

    ExprPtr power() {
        ExprPtr e = postfix();
        if (match(Tok::StarStar)) {
            int l = prevLine();
            ExprPtr r = unary();
            return std::make_shared<BinaryExpr>("**", e, r, l);
        }
        return e;
    }

    ExprPtr postfix() {
        ExprPtr e = primary();
        while (true) {
            if (check(Tok::LBracket)) {
                int l = advance().line;
                if (check(Tok::Colon)) {
                    // [:end] or [:end:step] or [::step]
                    advance();
                    ExprPtr endE = nullptr;
                    ExprPtr stepE = nullptr;
                    if (!check(Tok::RBracket) && !check(Tok::Colon)) endE = expression();
                    if (match(Tok::Colon)) {
                        if (!check(Tok::RBracket)) stepE = expression();
                    }
                    expect(Tok::RBracket, "expected ']' after slice");
                    e = std::make_shared<SliceExpr>(e, nullptr, endE, stepE, l);
                } else {
                    ExprPtr idx = expression();
                    if (match(Tok::Colon)) {
                        ExprPtr endE = nullptr;
                        ExprPtr stepE = nullptr;
                        if (!check(Tok::RBracket) && !check(Tok::Colon)) endE = expression();
                        if (match(Tok::Colon)) {
                            if (!check(Tok::RBracket)) stepE = expression();
                        }
                        expect(Tok::RBracket, "expected ']' after slice");
                        e = std::make_shared<SliceExpr>(e, idx, endE, stepE, l);
                    } else {
                        expect(Tok::RBracket, "expected ']' after index");
                        e = std::make_shared<IndexExpr>(e, idx, l);
                    }
                }
            } else if (check(Tok::Dot)) {
                int l = advance().line;
                Token name = expect(Tok::Ident, "expected field name after '.'");
                e = std::make_shared<FieldExpr>(e, name.lexeme, false, l);
            } else if (check(Tok::OptDot)) {
                int l = advance().line;
                Token name = expect(Tok::Ident, "expected field name after '?.'");
                e = std::make_shared<FieldExpr>(e, name.lexeme, true, l);
            } else if (check(Tok::LParen)) {
                int l = advance().line;
                std::vector<CallArg> args;
                if (!check(Tok::RParen)) {
                    do {
                        CallArg arg;
                        // named: ident := expr
                        if (check(Tok::Ident) && pos + 1 < toks.size() && toks[pos + 1].type == Tok::Define) {
                            arg.name = advance().lexeme;
                            advance(); // :=
                            arg.expr = expression();
                        } else {
                            arg.expr = expression();
                        }
                        args.push_back(std::move(arg));
                    } while (match(Tok::Comma));
                }
                expect(Tok::RParen, "expected ')' after arguments");
                e = std::make_shared<CallExpr>(e, std::move(args), l);
            } else {
                break;
            }
        }
        return e;
    }

    ExprPtr primary() {
        if (check(Tok::Number)) {
            Token t = advance();
            return std::make_shared<LiteralExpr>(Value::Num(t.num), t.line);
        }
        if (check(Tok::String)) {
            Token t = advance();
            if (t.str.find("${") == std::string::npos) {
                return std::make_shared<LiteralExpr>(Value::Str(t.str), t.line);
            }
            // String interpolation: "text ${expr} more"
            std::vector<InterpPart> parts;
            std::string current;
            size_t i = 0;
            while (i < t.str.size()) {
                if (t.str[i] == '$' && i + 1 < t.str.size() && t.str[i + 1] == '{') {
                    size_t j = i + 2;
                    int depth = 1;
                    bool inStr = false, esc = false;
                    while (j < t.str.size()) {
                        char c = t.str[j];
                        if (inStr) {
                            if (esc) esc = false;
                            else if (c == '\\') esc = true;
                            else if (c == '"') inStr = false;
                        } else {
                            if (c == '"') inStr = true;
                            else if (c == '{') depth++;
                            else if (c == '}') { depth--; if (depth == 0) break; }
                        }
                        j++;
                    }
                    if (j >= t.str.size()) throw ParseError(t.line, "unterminated interpolation");
                    if (!current.empty()) {
                        parts.push_back({true, current, nullptr});
                        current.clear();
                    }
                    std::string inner = t.str.substr(i + 2, j - i - 2);
                    Lexer innerLex(inner);
                    auto innerToks = innerLex.scan();
                    Parser innerParser(innerToks);
                    ExprPtr expr = innerParser.parseExpression();
                    parts.push_back({false, "", expr});
                    i = j + 1;
                } else {
                    current += t.str[i];
                    i++;
                }
            }
            if (!current.empty()) parts.push_back({true, current, nullptr});
            return std::make_shared<StringInterpExpr>(std::move(parts), t.line);
        }
        if (check(Tok::True)) {
            int l = advance().line;
            return std::make_shared<LiteralExpr>(Value::Bool(true), l);
        }
        if (check(Tok::False)) {
            int l = advance().line;
            return std::make_shared<LiteralExpr>(Value::Bool(false), l);
        }
        if (check(Tok::Nil)) {
            int l = advance().line;
            return std::make_shared<LiteralExpr>(Value::Nil(), l);
        }
        if (check(Tok::Match)) { advance(); return matchExpr(); }
        if (check(Tok::New)) {
            int l = advance().line;
            Token tname = expect(Tok::Ident, "expected type name after 'new'");
            expect(Tok::LBrace, "expected '{' in record construction");
            std::vector<std::pair<std::string, ExprPtr>> fields;
            while (!check(Tok::RBrace) && !check(Tok::Eof)) {
                Token fname = expect(Tok::Ident, "expected field name");
                expect(Tok::Colon, "expected ':' after field name");
                ExprPtr v = expression();
                fields.push_back({fname.lexeme, v});
                if (!match(Tok::Comma)) break;
            }
            expect(Tok::RBrace, "expected '}' after record fields");
            return std::make_shared<RecordConstructExpr>(tname.lexeme, std::move(fields), l);
        }
        if (check(Tok::Ident)) {
            Token t = advance();
            return std::make_shared<VarExpr>(t.lexeme, t.line);
        }
        if (check(Tok::LParen)) return parenOrLambda();
        if (check(Tok::LBracket)) return arrayLiteral();
        if (check(Tok::MapOpen)) return mapLiteral();
        if (check(Tok::SetOpen)) return setLiteral();
        throw ParseError(peek().line, "expected expression");
    }

    ExprPtr setLiteral() {
        int l = peek().line;
        expect(Tok::SetOpen, "expected '#{'");
        std::vector<ArrayExpr::Elem> elems;
        if (!check(Tok::RBrace)) {
            do {
                elems.push_back({expression(), false});
            } while (match(Tok::Comma) && !check(Tok::RBrace));
        }
        expect(Tok::RBrace, "expected '}'");
        // Build as: set(...elems) via a synthetic call would need runtime —
        // instead return an array and convert in a thin wrapper: use MapExpr-like
        // Construct a map literal with __set at eval time via ArrayExpr + native is heavy.
        // Simplest: emit a CallExpr to set(...) 
        auto callee = std::make_shared<VarExpr>("set", l);
        std::vector<ExprPtr> args;
        for (auto& e : elems) args.push_back(e.expr);
        // CallExpr currently takes vector of ExprPtr - check signature
        return std::make_shared<CallExpr>(callee, std::move(args), l);
    }

    ExprPtr parenOrLambda() {
        size_t save = pos;
        int l = peek().line;

        if (check(Tok::LParen)) {
            advance();
            std::vector<Param> ps;
            bool ok = true;
            if (!check(Tok::RParen)) {
                while (true) {
                    if (!check(Tok::Ident)) { ok = false; break; }
                    Token name = advance();
                    Param p;
                    p.name = name.lexeme;
                    if (check(Tok::Colon)) {
                        advance();
                        if (!check(Tok::Ident)) { ok = false; break; }
                        p.type = advance().lexeme;
                    }
                    if (check(Tok::Define)) {
                        advance();
                        p.defaultValue = expression();
                    }
                    ps.push_back(std::move(p));
                    if (check(Tok::Comma)) { advance(); continue; }
                    break;
                }
            }
            if (ok && check(Tok::RParen)) {
                advance();
                if (check(Tok::DefineFunc)) {
                    advance();
                    if (check(Tok::LBrace)) {
                        auto blk = block();
                        return std::make_shared<LambdaExpr>(std::move(ps), blk, "any", l);
                    }
                    ExprPtr exprBody = expression();
                    std::vector<StmtPtr> stmts;
                    stmts.push_back(std::make_shared<ReturnStmt>(exprBody, l));
                    auto blk = std::make_shared<BlockStmt>(std::move(stmts), l);
                    return std::make_shared<LambdaExpr>(std::move(ps), blk, "any", l);
                }
            }
        }

        pos = save;
        advance(); // (
        if (check(Tok::RParen)) {
            advance();
            // empty tuple ()
            return std::make_shared<ArrayExpr>(std::vector<ArrayExpr::Elem>{}, l);
        }
        ExprPtr e = expression();
        if (match(Tok::Comma)) {
            // tuple: (a, b, c)
            std::vector<ArrayExpr::Elem> elems;
            elems.push_back({e, false});
            if (!check(Tok::RParen)) {
                do {
                    elems.push_back({expression(), false});
                } while (match(Tok::Comma) && !check(Tok::RParen));
            }
            expect(Tok::RParen, "expected ')' after tuple");
            return std::make_shared<ArrayExpr>(std::move(elems), l);
        }
        expect(Tok::RParen, "expected ')'");
        return e;
    }

    ExprPtr arrayLiteral() {
        int l = peek().line;
        expect(Tok::LBracket, "expected '['");
        std::vector<ArrayExpr::Elem> elems;
        if (!check(Tok::RBracket)) {
            do {
                bool spread = false;
                if (check(Tok::DotDotDot)) { advance(); spread = true; }
                elems.push_back({expression(), spread});
            } while (match(Tok::Comma) && !check(Tok::RBracket));
        }
        expect(Tok::RBracket, "expected ']'");
        return std::make_shared<ArrayExpr>(std::move(elems), l);
    }

    ExprPtr mapLiteral() {
        int l = peek().line;
        expect(Tok::MapOpen, "expected '%{'");
        std::vector<std::pair<ExprPtr, ExprPtr>> pairs;
        if (!check(Tok::RBrace)) {
            do {
                ExprPtr key;
                // Shorthand: bare ident followed by ':' becomes string key
                if (check(Tok::Ident) && pos + 1 < toks.size() && toks[pos + 1].type == Tok::Colon) {
                    Token id = advance();
                    key = std::make_shared<LiteralExpr>(Value::Str(id.lexeme), id.line);
                } else {
                    key = expression();
                }
                expect(Tok::Colon, "expected ':' in map literal");
                ExprPtr val = expression();
                pairs.push_back({key, val});
            } while (match(Tok::Comma) && !check(Tok::RBrace));
        }
        expect(Tok::RBrace, "expected '}'");
        return std::make_shared<MapExpr>(std::move(pairs), l);
    }

    ExprPtr matchExpr() {
        int l = prevLine();
        ExprPtr subject;
        bool condStyle = false;
        // Subjectless match { cond => ..., _ => ... }  (like Lisp cond)
        if (check(Tok::LBrace)) {
            condStyle = true;
            subject = std::make_shared<LiteralExpr>(Value::Bool(true), l);
        } else {
            subject = expression();
        }
        expect(Tok::LBrace, "expected '{' after match subject");
        std::vector<MatchArm> arms;
        while (!check(Tok::RBrace) && !check(Tok::Eof)) {
            MatchArm arm;
            arm.line = peek().line;
            if (condStyle) {
                // arm is: _ => body   OR   expr => body
                if (check(Tok::Ident) && peek().lexeme == "_" &&
                    pos + 1 < toks.size() && toks[pos + 1].type == Tok::FatArrow) {
                    advance(); // _
                    arm.pat.kind = Pattern::PKind::Wildcard;
                } else {
                    arm.pat.kind = Pattern::PKind::Wildcard;
                    arm.guard = expression();
                }
            } else {
                arm.pat = parsePattern();
                if (match(Tok::When)) arm.guard = expression();
            }
            expect(Tok::FatArrow, "expected '=>' in match arm");
            if (check(Tok::LBrace)) {
                arm.blockBody = block();
            } else {
                arm.body = expression();
            }
            arms.push_back(std::move(arm));
            if (!match(Tok::Comma)) break;
        }
        expect(Tok::RBrace, "expected '}' after match arms");
        return std::make_shared<MatchExpr>(subject, std::move(arms), l);
    }

    Pattern parsePattern() {
        Pattern p;
        p.line = peek().line;
        if (check(Tok::Number)) {
            Token t = advance();
            p.kind = Pattern::PKind::Literal;
            p.literal = Value::Num(t.num);
            return p;
        }
        if (check(Tok::String)) {
            Token t = advance();
            p.kind = Pattern::PKind::Literal;
            p.literal = Value::Str(t.str);
            return p;
        }
        if (check(Tok::True))  { advance(); p.kind = Pattern::PKind::Literal; p.literal = Value::Bool(true); return p; }
        if (check(Tok::False)) { advance(); p.kind = Pattern::PKind::Literal; p.literal = Value::Bool(false); return p; }
        if (check(Tok::Nil))   { advance(); p.kind = Pattern::PKind::Literal; p.literal = Value::Nil(); return p; }
        if (check(Tok::Ident)) {
            Token t = advance();
            // Enum variant: Color.Red
            if (check(Tok::Dot)) {
                advance();
                Token var = expect(Tok::Ident, "expected variant after '.'");
                p.kind = Pattern::PKind::Literal;
                p.literal = Value::Str(t.lexeme + "." + var.lexeme);
                return p;
            }
            if (t.lexeme == "_") { p.kind = Pattern::PKind::Wildcard; return p; }
            p.kind = Pattern::PKind::Binding;
            p.bind = t.lexeme;
            return p;
        }
        if (check(Tok::LBracket)) {
            advance();
            p.kind = Pattern::PKind::Array;
            while (!check(Tok::RBracket) && !check(Tok::Eof)) {
                if (check(Tok::DotDotDot)) {
                    advance();
                    Token name = expect(Tok::Ident, "expected identifier after '...'");
                    p.rest = name.lexeme;
                    match(Tok::Comma);
                    break;
                }
                p.elems.push_back(parsePattern());
                if (!match(Tok::Comma)) break;
            }
            expect(Tok::RBracket, "expected ']' in array pattern");
            return p;
        }
        if (check(Tok::MapOpen)) {
            advance();
            p.kind = Pattern::PKind::Map;
            while (!check(Tok::RBrace) && !check(Tok::Eof)) {
                std::string key;
                if (check(Tok::String)) key = advance().str;
                else if (check(Tok::Ident)) key = advance().lexeme;
                else throw ParseError(peek().line, "map pattern key must be string or ident");
                expect(Tok::Colon, "expected ':' in map pattern");
                p.mapEntries.push_back({key, parsePattern()});
                if (!match(Tok::Comma)) break;
            }
            expect(Tok::RBrace, "expected '}' in map pattern");
            return p;
        }
        throw ParseError(peek().line, "expected pattern");
    }
};

// ---------------------------------------------------------------------------
// Module loading (after Parser is defined)
// ---------------------------------------------------------------------------
EnvPtr Interpreter::loadModule(const std::string& path, int line) {
    std::string full = path;
    if (!dirStack.empty() && !path.empty() && path[0] != '/') {
        full = dirStack.back() + "/" + path;
    }
    auto it = moduleCache.find(full);
    if (it != moduleCache.end()) return it->second;

    std::ifstream in(full, std::ios::binary);
    if (!in) throw RuntimeError(line, "cannot open module '" + path + "'");
    std::stringstream ss; ss << in.rdbuf();
    std::string src = ss.str();

    std::string dir = ".";
    size_t slash = full.find_last_of('/');
    if (slash != std::string::npos) dir = full.substr(0, slash);

    EnvPtr modEnv = std::make_shared<Env>(global);
    dirStack.push_back(dir);
    try {
        Lexer lex(src);
        Parser parser(lex.scan());
        auto prog = parser.parseProgram();
        for (auto& s : prog) s->exec(modEnv, *this);
    } catch (...) {
        dirStack.pop_back();
        throw;
    }
    dirStack.pop_back();
    moduleCache[full] = modEnv;
    return modEnv;
}

// ---------------------------------------------------------------------------
// CLI
// ---------------------------------------------------------------------------
static std::vector<std::string> g_sourceLines;

static void reportError(const std::string& kind, int line, const std::string& msg) {
    std::cerr << kind;
    if (line > 0) std::cerr << " [line " << line << "]";
    std::cerr << ": " << msg << '\n';
    if (line > 0 && line <= static_cast<int>(g_sourceLines.size())) {
        std::cerr << "  " << g_sourceLines[static_cast<size_t>(line - 1)] << '\n';
        std::cerr << "  ^\n";
    }
}

static void setSource(const std::string& src) {
    g_sourceLines.clear();
    std::string cur;
    for (char c : src) {
        if (c == '\n') { g_sourceLines.push_back(cur); cur.clear(); }
        else cur += c;
    }
    if (!cur.empty() || src.empty() || src.back() == '\n') g_sourceLines.push_back(cur);
}

static int runSource(const std::string& src, bool replMode, const std::string& baseDir = ".") {
    setSource(src);
    try {
        Lexer lex(src);
        std::vector<Token> toks = lex.scan();
        Parser parser(std::move(toks));
        std::vector<StmtPtr> prog = parser.parseProgram();

        Interpreter I;
        I.dirStack.push_back(baseDir);
        if (replMode) {
            for (const auto& s : prog) {
                Value v = s->exec(I.global, I);
                if (!v.isNil()) std::cout << v.toString(false) << '\n';
            }
        } else {
            I.run(prog);
        }
        return 0;
    } catch (const ParseError& e) {
        reportError("parse error", e.line, e.what());
    } catch (const RuntimeError& e) {
        reportError("runtime error", e.line, e.what());
    } catch (const std::exception& e) {
        reportError("error", 0, e.what());
    }
    return 1;
}

static int runSourceOn(Interpreter& I, const std::string& src) {
    setSource(src);
    try {
        Lexer lex(src);
        Parser parser(lex.scan());
        auto prog = parser.parseProgram();
        for (const auto& s : prog) {
            Value v = s->exec(I.global, I);
            if (!v.isNil()) std::cout << v.toString(false) << '\n';
        }
        return 0;
    } catch (const ParseError& e) {
        reportError("parse error", e.line, e.what());
    } catch (const RuntimeError& e) {
        reportError("runtime error", e.line, e.what());
    } catch (const std::exception& e) {
        reportError("error", 0, e.what());
    }
    return 1;
}

static void repl() {
    std::cout << "Vela REPL. Type .help for commands, 'exit' to leave.\n";
    Interpreter I;
    I.dirStack.push_back(".");
    std::string pending;
    std::string line;
    while (true) {
        std::cout << (pending.empty() ? "vela> " : "...> ") << std::flush;
        if (!std::getline(std::cin, line)) break;
        if (pending.empty()) {
            if (line == "exit" || line == "quit") break;
            if (line == ".help") {
                std::cout << "  .help     this help\n"
                          << "  .vars     list user bindings\n"
                          << "  .clear    reset interpreter state\n"
                          << "  exit      leave REPL\n";
                continue;
            }
            if (line == ".vars") {
                for (const auto& kv : I.global->vars) {
                    if (kv.first.size() >= 2 && kv.first[0] == '_' && kv.first[1] == '_') continue;
                    // skip native bulk noise: show non-fn or named fn
                    if (kv.second.kind == Value::Kind::Native) continue;
                    std::cout << "  " << kv.first << " = " << kv.second.toString(true) << '\n';
                }
                continue;
            }
            if (line == ".clear") {
                I = Interpreter();
                I.dirStack.push_back(".");
                std::cout << "state cleared\n";
                continue;
            }
        }

        pending += line;
        pending += '\n';

        int braces = 0;
        bool inStr = false;
        bool esc = false;
        for (char c : pending) {
            if (inStr) {
                if (esc) esc = false;
                else if (c == '\\') esc = true;
                else if (c == '"') inStr = false;
            } else {
                if (c == '"') inStr = true;
                else if (c == '{') braces++;
                else if (c == '}') braces--;
            }
        }
        if (braces > 0 || inStr) continue;

        runSourceOn(I, pending);
        pending.clear();
    }
    std::cout << '\n';
}

static std::string dirOf(const std::string& path) {
    size_t p = path.find_last_of('/');
    return p == std::string::npos ? "." : path.substr(0, p);
}

int main(int argc, char** argv) {
    std::string path;
    bool dumpTokens = false;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--tokens") dumpTokens = true;
        else if (a == "--version" || a == "-V") {
            std::cout << "Vela 1.0.0\n";
            return 0;
        } else if (a == "--help" || a == "-h") {
            std::cout << "Vela language\n"
                      << "  vela [options] [file.vel]\n"
                      << "  vela                  interactive REPL\n"
                      << "  --tokens              dump lexer tokens and exit\n"
                      << "  --version             print version and exit\n"
                      << "  --help                show this help\n";
            return 0;
        } else if (!a.empty() && a[0] != '-') {
            path = a;
        }
    }
    if (!path.empty()) {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            std::cerr << "error: cannot open file '" << path << "'\n";
            return 1;
        }
        std::stringstream ss;
        ss << in.rdbuf();
        std::string src = ss.str();
        if (dumpTokens) {
            try {
                Lexer lex(src);
                auto toks = lex.scan();
                for (const auto& t : toks) {
                    std::cout << t.line << "\t" << t.lexeme;
                    if (t.type == Tok::Number) std::cout << "  (" << t.num << ")";
                    if (t.type == Tok::String) std::cout << "  (str)";
                    std::cout << '\n';
                }
            } catch (const ParseError& e) {
                reportError("parse error", e.line, e.what());
                return 1;
            }
            return 0;
        }
        return runSource(src, false, dirOf(path));
    }
    if (dumpTokens) {
        std::cerr << "error: --tokens requires a file\n";
        return 1;
    }
    repl();
    return 0;
}
