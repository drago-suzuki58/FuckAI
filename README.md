# FuckAI

This tool is an AI-powered command assistant inspired by [TheFuck](https://github.com/nvbn/thefuck).

Other Language README
[Japanese](README.ja.md)

## Features

- Simple and detailed command suggestions: Just use FuckAI and it will automatically determine which command you should use
- Automatically detects installed commands and suggests the ones you need
- Advanced natural language processing using the OpenAI API (LLM)

## Build

```sh
git clone https://github.com/drago-suzuki58/FuckAI.git
cd FuckAI
xmake
```

For more details about xmake, see [xmake.io](https://xmake.io/#/).

## Usage

### Basic

```sh
FuckAI "<Describe what you want to do in Japanese or English>"
```

Example:

```sh
FuckAI "Show process list"
```

### Options

- `--all`  
  Sends all installed commands to the AI for comprehensive suggestions (covers all system commands, but uses more tokens)

- `--mini` (default)  
  Fast and cost-effective mode with reduced token usage (may not cover all system commands)

- `--help`, `-h`  
  Show help

- `--version`, `-v`  
  Show version information

- `--config`, `-c`  
  Set OpenAI API key, model, etc.  
  The following keys can be configured:

  - `OPENAI_API_KEY`: Your OpenAI API key (e.g., sk-xxxxxxx)
  - `OPENAI_API_BASE_URL`: The endpoint URL for the OpenAI API (defaults to the official endpoint if omitted)
  - `OPENAI_API_MODEL`: The model name to use (e.g., gpt-4.1-mini-2025-04-14)

  The configuration is saved in a `config.txt` file in the current directory.  
  You can also edit this file directly if needed.

- `--show-config`, `-s`  
  Show current configuration

## Configuration

Set your OpenAI API key and model before first use.

```sh
FuckAI --config OPENAI_API_KEY sk-xxxxxxx
FuckAI --config OPENAI_API_MODEL gpt-4.1-mini-2025-04-14
```

---

## Example Output

```sh
$ FuckAI "Show process list"
Info: System Architecture: x86_64
Info: OS Version: Ubuntu 22.04.5 LTS
Info: Installed Commands: 1804
=== Commands ===
- ps
  The 'ps' command displays a list of currently running processes. By adding options, you can view more detailed information. It is simple and widely used.

- top
  The 'top' command allows you to monitor the status of processes in real time. It is suitable for dynamic process monitoring.

- htop
  'htop' is an interactive alternative to 'top', offering color display and improved usability. It is convenient for checking process lists in a more user-friendly way.
```

## Notes

Currently, Windows is not supported. Please use on Linux.
