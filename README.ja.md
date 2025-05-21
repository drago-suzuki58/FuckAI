# FuckAI

このツールは、[TheFuck](https://github.com/nvbn/thefuck)にインスパイアされて作られた、AIによるコマンド補助ツールです。

Other Language README
[English](README.md)

## 特徴

- FuckAIするだけでの簡単で詳細なコマンドサジェストで、利用者がどのコマンドを利用すればいいのかの自動判断
- インストールされているコマンドを自動的に検出し、利用者が必要とするコマンドを提案
- OpenAI APIを利用した高度なLLMによる自然言語処理

## ビルド

```sh
git clone https://github.com/drago-suzuki58/FuckAI.git
cd FuckAI
xmake
```

xmakeの詳細については、[xmake.io](https://xmake.io/#/)を参照してください。


## 使用方法

### 基本

```sh
FuckAI "<やりたいことを日本語や英語で入力>"
```

例:

```sh
FuckAI "プロセス一覧取得"
```

### オプション

- `--all`  
  インストール済み全コマンドをAIに渡し、網羅的な提案(システムコマンドを全て網羅するため、トークン消費が大きくなります)

- `--mini`(デフォルト)  
  トークン消費を抑えた高速・省コストモード(システムコマンドを網羅できません)

- `--help`, `-h`  
  ヘルプ表示

- `--version`, `-v`  
  バージョン表示

- `--config`, `-c`  
  OpenAI APIキーやモデルなどの設定を行います。  
  設定できるキーは以下の通りです。

  - `OPENAI_API_KEY` : OpenAIのAPIキー（例: sk-xxxxxxx）
  - `OPENAI_API_BASE_URL` : OpenAI APIのエンドポイントURL（省略時は公式のエンドポイントを使用）
  - `OPENAI_API_MODEL` : 使用するモデル名（例: gpt-4.1-mini-2025-04-14）

  設定内容は、カレントディレクトリの `config.txt` ファイルに保存されます。  
  このファイルを直接編集することも可能です。

- `--show-config`, `-s`  
  現在の設定内容を表示

## 設定

初回利用時はOpenAI APIキーやモデルを設定してください。

```sh
FuckAI --config OPENAI_API_KEY sk-xxxxxxx
FuckAI --config OPENAI_API_MODEL gpt-4.1-mini-2025-04-14
```

---

## 出力例

```sh
$ FuckAI "プロセス一覧取得"
Info: System Architecture: x86_64
Info: OS Version: Ubuntu 22.04.5 LTS
Info: Installed Commands: 1804
=== Commands ===
- ps
  psコマンドは現在実行中のプロセスの一覧を表示する標準的なコマンドです。オプションをつけることで詳細情報も確認可能で、シンプルかつ広く使われています。

- top
  topコマンドはリアルタイムでプロセスの状態を監視できるコマンドです。動的にプロセスを監視したい場合に適しています。

- htop
  htopはtopのインタラクティブな代替ツールで、カラー表示や操作性が向上しています。より見やすくプロセス一覧を確認したい場合に便利です。
```

## 注意事項

現在Windows環境では動作確認が取れていません。Linux環境での利用を推奨します。
