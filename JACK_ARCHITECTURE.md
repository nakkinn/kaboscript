# 高水準言語仕様

VM上で動作する高水準言語の仕様。クラスベースのオブジェクト指向言語。

## プログラム構造

トップレベルには **`class` ブロックのみ**記述できる。エントリーポイントは `Main.main`。

```
class Main {
    function main() {
        Output.printi(42);
        Output.printc('\n');
        return 0;
    }
}
```

- `Main.main` が自動的に呼び出される。
- クラスの宣言順序は自由。後方で定義されたクラスも参照できる（コンパイラが2パスで解決する）。

---

## 型

| 型 | 説明 |
|---|---|
| `int` | 整数型（プリミティブ） |
| クラス名 | そのクラスのインスタンスへのヒープアドレス |

型情報はコンパイラが静的に管理し、`obj.method()` の呼び出し先クラスを決定するために使用する。

---

## クラス定義

```
class ClassName {
    // 変数宣言（field / static）
    // 関数定義（constructor / method / function）
}
```

---

## 変数の種類

| 種類 | 宣言場所 | VMセグメント | スコープ |
|---|---|---|---|
| 引数 | 関数の `()` 内 | `argument` | その関数内 |
| ローカル変数 | 関数内 | `local` | その関数内（ブロックスコープ） |
| フィールド変数 | クラス内（修飾子なし） | `this` | そのインスタンス |
| 静的変数 | クラス内（`static`） | `static` | そのクラス全体（全インスタンス共有） |

```
class Vector {
    int x;            // field → this 0
    int y;            // field → this 1
    static int count; // static → static N（全インスタンス共有）
}
```

### フィールド変数

クラス内でキーワードなしに型名から始まる宣言。初期値なし。

```
class Counter {
    int count;
    int id;
}
```

### 静的変数

`static` キーワードを付ける。初期値なし（ゼロ初期化）。プログラム起動時に1つだけ確保される。

```
class IDGen {
    static int next;
}
```

---

## 関数の種類

関数定義に**返り値の型は書かない**。

### `constructor`

インスタンスを生成して返す。`return this` はコンパイラが自動生成するため記述不要。

```
constructor new(int arg1, ...) {
    // フィールドの初期化
}
```

**VMへの変換：**

1. `push const フィールド数` → `call $alloc 1` でヒープ確保
2. `pop pointer 0` で `this` のベースアドレスをセット
3. フィールドを初期化
4. `push pointer 0` → `return`（自動生成）

```
// constructor new(int px, int py) のVMコード
function Vector.new 0
push const 2
call $alloc 1
pop pointer 0
push argument 0     // px
pop this 0          // x = px
push argument 1     // py
pop this 1          // y = py
push pointer 0      // return this（自動生成）
return
```

### `method`

インスタンスに属する操作。`this` を介してフィールドにアクセスできる。

```
method name(int arg1, ...) {
    // this を使える
}
```

**VMへの変換：**

- 呼び出し側がオブジェクトのアドレスを **`argument 0` として自動付与**して渡す
- メソッド先頭で `push argument 0` → `pop pointer 0` を実行し `this` を確定させる
- 実引数は `argument 1` から始まる

```
// v1.getX() の呼び出し側VMコード
push local 0        // v1 のアドレス（自動付与）
call Vector.getX 1  // 引数は this の1個

// method getX() のVMコード
function Vector.getX 0
push argument 0     // this
pop pointer 0       // this のベースアドレスをセット
push this 0         // x フィールドを読む
return
```

### `function`

クラスに属するが `this` を持たない静的な関数。インスタンス不要で呼び出せる。

```
function name(int arg1, ...) {
    // this は使えない
}
```

**VMへの変換：**

- 呼び出し側はオブジェクトのアドレスを追加しない
- メソッド先頭に `pop pointer 0` は生成されない

```
// ClassName.func(args) の呼び出し側VMコード
push const 1
push const 2
call IDGen.add 2    // this の付与なし
```

---

## 呼び出し規則まとめ

| 種類 | 呼び出し構文 | VM上の引数 | 先頭処理 |
|---|---|---|---|
| `constructor` | `ClassName.new(args)` | 実引数のみ | `$alloc` → `pop pointer 0` |
| `method` | `obj.method(args)` | obj + 実引数 | `push argument 0 / pop pointer 0` |
| `function` | `ClassName.func(args)` | 実引数のみ | なし |

---

## 変数宣言

### フィールド・静的変数（クラス内）

初期値なし。コンストラクタ・関数内で明示的に初期化する。

```
class Foo {
    int x;            // フィールド変数
    static int total; // 静的変数
}
```

### ローカル変数（関数内）

型名 + 変数名で宣言する。初期値を同時に指定することもできる。

```
function main() {
    int x;          // 初期化なし（0初期化）
    int y = 10;     // 初期化あり
    Vector v;       // クラス型
}
```

- ブロック内での宣言も可能。スコープはそのブロック内に限定される。
- 使用する前に宣言が必要。前方参照は不可。

---

## 変数代入

```
x = expr;
x++;
x--;
x += expr;
x -= expr;
x *= expr;
```

---

## メソッド・関数呼び出し

### インスタンスメソッド呼び出し

```
v1.setX(10);
obj.method(arg1, arg2);
```

### static関数・コンストラクタ呼び出し

```
Vector v = Vector.new(3, 4);
IDGen.reset();
```

### 呼び出しの判定ルール

`.` の左辺が型名テーブルに存在する → static呼び出し
`.` の左辺が変数テーブルに存在する → インスタンスメソッド呼び出し

---

## 制御構文

### if-else

```
if(x > 0) {
    // ...
} else {
    // ...
}
```

### while

```
while(x > 0) {
    x--;
}
```

### for

```
for(int i = 0; i < 10; i++) {
    // ...
}
```

---

## 演算子

### 優先順位（上が高い）

| 優先順位 | 演算子 | 結合性 |
|---|---|---|
| 1 | `()` `[]` `.` | 左 |
| 2 | `-`(単項) `+`(単項) `!` | 右 |
| 3 | `*` `/` | 左 |
| 4 | `+` `-` | 左 |
| 5 | `<` `>` `<=` `>=` | 左 |
| 6 | `==` `!=` | 左 |
| 7 | `&&` | 左 |
| 8 | `\|\|` | 左 |

---

## 配列

ヒープ上に確保される。変数にはアドレスのみ保持する。

```
int arr[10];        // alloc(10) → アドレスをローカルに格納
arr[3] = 100;       // 書き込み
int x = arr[3];     // 読み出し
```

---

## 組み込み関数・クラス

### Outputクラス

| 呼び出し | 引数 | 動作 |
|---|---|---|
| `Output.printi(expr)` | 整数 | 整数として出力 |
| `Output.printc(expr)` | 整数（ASCIIコード） | ASCIIコードに対応する文字として出力 |

### 文字リテラル

`'A'` のような文字リテラルは、**コンパイラがASCIIコード（整数）に変換**する糖衣構文。
内部では全て `int` として扱われる。

| 記法 | 変換後の整数 | 意味 |
|---|---|---|
| `'A'` | 65 | 英字・記号 |
| `' '` | 32 | 空白 |
| `'\n'` | 10 | 改行 |
| `'\\'` | 92 | バックスラッシュ |
| `'\''` | 39 | シングルクォート |

```
Output.printi(42);       // 42 を整数として出力
Output.printc('A');      // 65 として渡され、'A' として出力
Output.printc(65);       // 上と等価
Output.printc('\n');     // 10 として渡され、改行として出力
```

---

## オブジェクトのメモリレイアウト

インスタンスはヒープ上に確保されたフィールドの列であり、変数にはそのアドレスが格納される。

```
heap[addr + 0] = x   ← this 0
heap[addr + 1] = y   ← this 1

Vector v1;  // v1 にはヒープアドレスが入る
```

`static` フィールドはインスタンスのヒープ領域ではなく VM の `static` セグメントに格納される。
`static` インデックスはプログラム全体でグローバルに管理され、クラスをまたいで引き継がれる。

---

## コメント

```
// 行コメント

/* ブロックコメント
   複数行可 */
```

---

## コード例

### Vectorクラス

```
class Vector {
    int x;
    int y;
    static int count;

    constructor new(int px, int py) {
        x = px;
        y = py;
        count++;
    }

    method getX() { return x; }
    method getY() { return y; }

    method add(Vector other) {
        x += other.getX();
        y += other.getY();
        return 0;
    }

    method lengthSq() {
        return x * x + y * y;
    }

    function getCount() {
        return count;
    }
}

class Main {
    function main() {
        Vector v1;
        Vector v2;
        v1 = Vector.new(3, 4);
        v2 = Vector.new(1, 2);
        v1.add(v2);
        Output.printi(v1.lengthSq());
        Output.printi(Vector.getCount());
        return 0;
    }
}
```

### Counterクラス（static変数）

```
class Counter {
    static int total;
    int id;

    constructor new() {
        total++;
        id = total;
    }

    method getId()    { return id; }
    method getTotal() { return total; }
}

class Main {
    function main() {
        Counter a;
        Counter b;
        a = Counter.new();
        b = Counter.new();
        Output.printi(a.getId());
        Output.printi(b.getId());
        Output.printi(a.getTotal());
        return 0;
    }
}
```

期待出力: `1 2 2`
