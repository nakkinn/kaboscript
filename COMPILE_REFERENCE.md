# コンパイルリファレンス

Jack言語からVM言語への変換ルール。VMには `le`, `ge`, `ne` 命令は存在しない。`eq`, `gt`, `lt`, `not` の組み合わせで表現する。

## プログラム全体の構造

```
call main 0
pop temp 0
goto $end

[関数定義...]

label $end
```

## 演算子のコンパイル

### 算術

| Jack | VM |
|---|---|
| `a + b` | `[a] [b] add` |
| `a - b` | `[a] [b] sub` |
| `a * b` | `[a] [b] mul` |
| `-a` | `[a] neg` |
| `!a` | `[a] not` |

### 比較

| Jack | VM | 補足 |
|---|---|---|
| `a == b` | `[a] [b] eq` | |
| `a != b` | `[a] [b] eq not` | eq の反転 |
| `a > b` | `[a] [b] gt` | |
| `a < b` | `[a] [b] lt` | |
| `a >= b` | `[a] [b] lt not` | lt の反転 |
| `a <= b` | `[a] [b] gt not` | gt の反転 |

### 論理

| Jack | VM |
|---|---|
| `a && b` | `[a] [b] and` |
| `a \|\| b` | `[a] [b] or` |
| `!a` | `[a] not` |

## 変数アクセス

| 種類 | push | pop |
|---|---|---|
| ローカル変数（N番目） | `push local N` | `pop local N` |
| 引数（N番目） | `push argument N` | `pop argument N` |
| グローバル変数（N番目） | `push static N` | `pop static N` |
| 整数リテラル | `push const N` | ― |

## 変数宣言

### `var x = 10;`（ローカル、スロット N）

```
push const 10
pop local N
```

### `var x;`（初期化なし）

コード生成不要。`function` 命令が0初期化済みのスロットを確保する。

## 変数代入

### `x = expr;`

```
[expr]
pop local N
```

### `x++;`

```
push local N
push const 1
add
pop local N
```

### `x += expr;`

```
push local N
[expr]
add
pop local N
```

`x--`, `x -= expr`, `x *= expr` も同様のパターン。

## 関数定義

### `function add(a, b){ ... return expr; }`

```
function add 0          // 0 = ローカル変数数（max_local）
[本体]
[expr]                  // return 式
return
```

- 引数は `argument 0`, `argument 1`, ... でアクセス。
- ローカル変数数は関数内の max_local（ウォーターマーク値）。

## 関数呼び出し

### 式の中: `var x = add(1, 2);`

```
push const 1            // 引数0
push const 2            // 引数1
call add 2              // 2 = 引数の数
pop local N             // 戻り値を x に格納
```

### 文として: `add(1, 2);`（戻り値を捨てる）

```
push const 1
push const 2
call add 2
pop temp 0              // 戻り値を捨てる
```

## 組み込み関数

### `print(expr);`

```
[expr]
call print 1
pop temp 0
```

print の定義（VMコードとして出力する）:

```
function print 0
push argument 0
out
outsp
push const 0
return
```

### `newline();`

```
call newline 0
pop temp 0
```

newline の定義:

```
function newline 0
outnl
push const 0
return
```

## if文

### `if(cond){ A }`

```
[cond]
ifgo if_then_N
goto if_end_N
label if_then_N
[A]
label if_end_N
```

条件が真（非0）なら then へジャンプ。偽ならスキップ。

### `if(cond){ A }else{ B }`

```
[cond]
ifgo if_then_N
[B]                     // else が先
goto if_end_N
label if_then_N
[A]                     // then が後
label if_end_N
```

## while文

### `while(cond){ A }`

```
label loop_start_N
[cond]
not                     // 続行条件 → 脱出条件に反転
ifgo loop_end_N
[A]
goto loop_start_N
label loop_end_N
```

注: `[cond]` が既に比較演算の結果（0 or 1）なら、`not` で反転して脱出条件にする。

### 例: `while(i < 10){ ... }`

```
label loop_start_N
push local 0            // i
push const 10
lt                      // i < 10 (続行条件)
not                     // i >= 10 (脱出条件)
ifgo loop_end_N
[本体]
goto loop_start_N
label loop_end_N
```

## for文

### `for(var i = 0; i < 10; i++){ A }`

while に展開する:

```
// init: var i = 0
push const 0
pop local N

// while(i < 10)
label loop_start_N
push local N
push const 10
lt
not
ifgo loop_end_N

[A]

// update: i++
push local N
push const 1
add
pop local N

goto loop_start_N
label loop_end_N
```

## return文

### `return expr;`

```
[expr]
return
```

## ラベル番号

全てのラベルはグローバルなカウンタ `label_count` で一意にする。if, while, for が生成するたびにインクリメント。

```
if_then_0, if_end_0
loop_start_1, loop_end_1
if_then_2, if_end_2
...
```

## 変換例

### Jack

```
function factorial(n){
    if(n <= 1){
        return 1;
    }
    return n * factorial(n - 1);
}

function main(){
    print(factorial(6));
    newline();
    return 0;
}
```

### VM

```
call main 0
pop temp 0
goto $end

function factorial 0
push argument 0
push const 1
gt
not
ifgo if_then_0
goto if_end_0
label if_then_0
push const 1
return
label if_end_0
push argument 0
push argument 0
push const 1
sub
call factorial 1
mul
return

function main 0
push const 6
call factorial 1
call print 1
pop temp 0
call newline 0
pop temp 0
push const 0
return

function print 0
push argument 0
out
outsp
push const 0
return

function newline 0
outnl
push const 0
return

label $end
```
