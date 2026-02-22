# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 作業ルール

- **プログラムの編集は禁止**。コードの変更・追加・削除を行わないこと。
- ただし、コードのテストのために、入力となる文字列を書き換え実行することは特別に認める。
- 許可される作業: 質問への回答、設計アドバイス、Markdownファイルの生成・整理のみ。
- です。ます調で説明。文書はである調。
- 上の階層（`mikanOS/`直下など）のスクリプトを参照する必要はない。このフォルダ内で完結する。

## プロジェクト概要

**スタックベース仮想マシン（VM）**と、そのVM上で動作する**高水準言語（Jack系）のコンパイラ**を開発している。Nand2Tetris のアーキテクチャに近い設計。
標準ライブラリ（`<string>`, `<cstring>` 等）を使わず、文字列操作を自前で実装している。

## ビルド

```
g++ main.cpp string_utils.cpp virtual_machine.cpp -o main && main
```

## ファイル構成

| ファイル | 役割 |
|---|---|
| `main.cpp` | トークナイザ・パーサー（AST生成）・コード生成・デバッグ出力 |
| `string_utils.cpp` / `.hpp` | 標準ライブラリ不使用の自前文字列操作ユーティリティ |
| `virtual_machine.cpp` / `.hpp` | スタックベースVMの実装 |
| `VM_ARCHITECTURE.md` | VMの命令セット・メモリモデル・コールフレーム等の仕様書 |
| `JACK_ARCHITECTURE.md` | 高水準言語（Jack系）の予約語・コード例 |
| `COMPILE_REFERENCE.md` | Jack言語→VM言語の変換ルール・パターン集 |

## アーキテクチャ

- VM仕様: [VM_ARCHITECTURE.md](VM_ARCHITECTURE.md)
- 高水準言語仕様: [JACK_ARCHITECTURE.md](JACK_ARCHITECTURE.md)
- コンパイルリファレンス: [COMPILE_REFERENCE.md](COMPILE_REFERENCE.md)
