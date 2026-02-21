# VMアーキテクチャ

自作OS（mikanOS）組み込み用スタックベース仮想マシンの仕様。Nand2Tetris の VM アーキテクチャに近い設計。

## メモリセグメント

| セグメント | サイズ | 用途 |
|---|---|---|
| `stack` | 4092 | メインスタック。引数・ローカル変数・コールフレームを含む |
| `heap` | 4092 | this/thatセグメント経由でアクセスするオブジェクト領域 |
| `statics` | 512 | 静的変数領域 |
| `temp` | 8 | 一時変数領域 |

## レジスタ（ポインタ）

| レジスタ | 用途 |
|---|---|
| `ip` | 命令ポインタ |
| `sp` | スタックポインタ |
| `local_address` | 現在の関数のローカル変数先頭 |
| `argument_address` | 現在の関数の引数先頭 |
| `this_address` | heap上のオブジェクトベース |
| `that_address` | heap上の配列/オブジェクトベース |

## 命令セット

### スタック操作

| 命令 | 形式 | 動作 |
|---|---|---|
| `push` | `push <segment> <index>` | 指定セグメントの値をスタックに積む |
| `pop` | `pop <segment> <index>` | スタックトップを指定セグメントに格納 |

segment: `argument`, `local`, `static`, `const`, `this`, `that`, `pointer`, `temp`

### 算術・論理

| 命令 | 動作 |
|---|---|
| `add` | `a + b` |
| `sub` | `a - b` |
| `mul` | `a * b` |
| `neg` | `-a` |
| `eq` | `a == b` → 0 or 1 |
| `gt` | `a > b` → 0 or 1 |
| `lt` | `a < b` → 0 or 1 |
| `and` | `a && b` → 0 or 1 |
| `or` | `a \|\| b` → 0 or 1 |
| `not` | `a == 0` → 0 or 1 |

二項演算は `a`(sp-2), `b`(sp-1) の順。結果は `a` の位置に置かれ sp が1減る。

### 出力

| 命令 | 形式 | 動作 |
|---|---|---|
| `out` | `out` | スタックトップを数値として出力バッファに追加（spを1減らす） |
| `outstr` | `outstr <文字列>` | 文字列を出力バッファに追加（スタック不変） |
| `outsp` | `outsp` | 空白を出力（スタック不変） |
| `outnl` | `outnl` | 改行を出力（スタック不変） |

### フロー制御

| 命令 | 形式 | 動作 |
|---|---|---|
| `label` | `label <名前>` | ジャンプ先ラベルを定義 |
| `goto` | `goto <名前>` | 無条件ジャンプ |
| `ifgo` | `ifgo <名前>` | スタックトップが非0ならジャンプ（spを1減らす） |

### 関数

| 命令 | 形式 | 動作 |
|---|---|---|
| `function` | `function <名前> <ローカル変数数>` | 関数を定義し、ローカル変数分spを進める |
| `call` | `call <名前> <引数数>` | 関数を呼び出す（コールフレームを積む） |
| `return` | `return` | 呼び出し元に戻る（戻り値はスタックトップ） |

## コールフレーム構造

`call`時にスタック上に5ワードのフレームを積む:

```
sp → [戻りアドレス]
      [保存された local_address]
      [保存された argument_address]
      [保存された this_address]
      [保存された that_address]
      ← 新しい local_address はここを指す
```

`return`時にフレームを復元し、戻り値を `argument[0]` の位置に格納する。

## スクリプト形式

- 1行1命令。最大3トークン（空白区切り）で `VMCode{segment0, segment1, segment2}` に格納される。
- `#` で始まる行はコメント。空行は無視される。
- ラベル名・関数名は実行前のパスで行番号に解決される。

## スクリプト例（階乗の計算）

```
#階乗の計算

push const 6
call factorial 1
out
return

function factorial 0
push argument 0
push const 1
eq
ifgo BASE
push argument 0
push argument 0
push const 1
sub
call factorial 1
mul
return

label BASE
push const 1
return
```
