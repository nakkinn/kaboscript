# OOP アーキテクチャ仕様

クラスと型定義を導入した高水準言語の仕様。

---

## クラス定義

トップレベルには `class` ブロックのみ記述できる。

```
class ClassName {
    // field / static 変数宣言
    // constructor / method / function 定義
}
```

---

## 変数の種類

| 種類 | 宣言 | VMセグメント | スコープ |
|---|---|---|---|
| 引数 | 関数の `()` 内 | `argument` | その関数内 |
| ローカル変数 | 関数内の `var` | `local` | その関数内 |
| フィールド変数 | クラス内の `field var` | `this` | そのインスタンス |
| 静的変数 | クラス内の `static var` | `static` | そのクラス全体（全インスタンス共有） |

```
class Vector {
    int x;        // this 0
    int y;        // this 1
    static int count;   // static 0（全インスタンス共有）
}
```

---

## 型

| 型 | 説明 |
|---|---|
| `int` | 整数型（プリミティブ） |
| クラス名 | そのクラスのインスタンスへのヒープアドレス |

変数宣言には型を明示する。

```
int x;
Vector v;
```

型情報はコンパイラが静的に管理し、`obj.method()` の呼び出し先クラスを決定するために使用する。

---

## 関数の種類

### `constructor`

インスタンスを生成して返す。`return this` で終わる。

```
constructor ClassName new(int arg1, ...){
    // フィールドの初期化
    return this;
}
```

**裏側の挙動：**

1. `push const フィールド数` → `call $alloc 1` でヒープ領域を確保
2. `pop pointer 0` で `this` のベースアドレスをセット
3. フィールドを初期化（`pop this 0` など）
4. `push pointer 0` → `return` でインスタンスのアドレスを返す

```
// constructor Vector new(int px, int py) のVMコード
function Vector.new 0
push const 2        // フィールド数（x, y）
call $alloc 1       // ヒープ確保 → アドレスが返る
pop pointer 0       // this のベースアドレスに設定
push argument 0     // px
pop this 0          // x = px
push argument 1     // py
pop this 1          // y = py
push pointer 0      // this を返す
return
```

---

### `method`

インスタンスに属する操作。`this` を介してフィールドにアクセスできる。

```
method ReturnType name(int arg1, ...){
    // this を使える
}
```

**裏側の挙動：**

- 呼び出し側がオブジェクトのアドレスを **`argument 0` として自動的に追加**して渡す
- メソッド先頭で `push argument 0` → `pop pointer 0` を実行し `this` を確定させる
- `argument 0` は `this` なので、実引数は `argument 1` から始まる

```
// v1.getX() の呼び出し側VMコード
push local 0        // v1 のアドレス（自動付与）
call Vector.getX 1  // 引数は this の1個

// method int getX() のVMコード
function Vector.getX 0
push argument 0     // this（v1 のアドレス）
pop pointer 0       // this のベースアドレスをセット
push this 0         // x フィールドを読む
return
```

---

### `function`

クラスに属するが `this` を持たない静的な関数。インスタンス不要で呼び出せる。コンストラクタもこの仕組みで実現される。

```
function ReturnType name(int arg1, ...){
    // this は使えない
}
```

**裏側の挙動：**

- 呼び出し側はオブジェクトのアドレスを追加しない
- メソッド先頭に `pop pointer 0` は生成されない
- VM関数名は `ClassName.functionName`（命名による区別のみ）

```
// Vector.dot(v1, v2) の呼び出し側VMコード
push local 0        // v1（通常の引数）
push local 1        // v2（通常の引数）
call Vector.dot 2   // this の付与なし

// function int dot(Vector v1, Vector v2) のVMコード
function Vector.dot 0
// pop pointer 0 なし
push argument 0     // v1
call Vector.getX 1
push argument 1     // v2
call Vector.getX 1
mul
push argument 0
call Vector.getY 1
push argument 1
call Vector.getY 1
mul
add
return
```

---

## 呼び出し規則まとめ

| 種類 | 呼び出し構文 | VM上の引数 | 先頭処理 |
|---|---|---|---|
| `constructor` | `ClassName.new(args)` | 実引数のみ | `$alloc` → `pop pointer 0` |
| `method` | `obj.method(args)` | obj + 実引数 | `pop pointer 0` |
| `function` | `ClassName.func(args)` | 実引数のみ | なし |

---

## コード例：Vector クラス

```
class Vector {
    int x;
    int y;
    static int count;

    constructor Vector new(int px, int py){
        x = px;
        y = py;
        count++;
        return this;
    }

    method int getX()   { return x; }
    method int getY()   { return y; }

    method void add(Vector other){
        x += other.getX();
        y += other.getY();
        return;
    }

    method int lengthSq(){
        return x*x + y*y;
    }

    function int dot(Vector v1, Vector v2){
        return v1.getX() * v2.getX() + v1.getY() * v2.getY();
    }

    function int getCount(){
        return count;
    }
}

class Main {
    function void main(){
        Vector v1 = Vector.new(3, 4);
        Vector v2 = Vector.new(1, 2);
        v1.add(v2);
        print(v1.lengthSq());
        print(Vector.dot(v1, v2));
        print(Vector.getCount());
        return;
    }
}
```

---

## オブジェクトのメモリレイアウト

インスタンスはヒープ上に確保されたフィールドの列であり、変数にはそのアドレスが格納される。配列と同じ構造。

```
heap[addr + 0] = x   ← this 0
heap[addr + 1] = y   ← this 1

var Vector v1;  // v1 にはアドレスが入る
```

`static` フィールドはインスタンスのヒープ領域ではなく VM の `static` セグメントに格納される。

---

## コンパイラへの影響

| 追加要素 | 内容 |
|---|---|
| `VarEntry.segment` | `2`（this）を追加 |
| 変数テーブルの管理 | フィールドはクラスのコンパイル開始時に登録し、全メソッドのコンパイルが終わるまで保持 |
| 型テーブル | 変数名 → クラス名のマッピングを保持し、`obj.method()` の呼び出し先を決定する |
| 呼び出し生成 | `method` の場合は `push obj` を自動挿入、`function` は挿入しない |
