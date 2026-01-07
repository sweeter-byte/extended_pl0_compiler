# Extended PL/0 Compiler

[![C++17](https://img.shields.io/badge/C++-17-blue.svg?style=flat-square&logo=cplusplus)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square)](https://opensource.org/licenses/MIT)
[![Build Status](https://img.shields.io/badge/Build-CMake-brightgreen.svg?style=flat-square)](https://cmake.org/)
[![GUI](https://img.shields.io/badge/GUI-Qt5-orange.svg?style=flat-square&logo=qt)](https://www.qt.io/)

本项目是基于 C++17 实现的高性能、多特性 PL/0 编译器与解释器系统。它包含一个高效的栈式虚拟机、支持错误恢复的递归下降分析器，以及一个功能完备、基于 Qt5 的可视化 IDE 调试环境。

[**核心特性**](#-功能特性) | [**快速开始**](#-快速开始) | [**语言文法**](#-ebnf-文法) | [**GUI 调试器**](#-gui-调试器) | [**系统架构**](#-系统架构)

---

## 功能特性

### 核心编译引擎
- **高效词法分析**: 支持高吞吐量的 Token 处理与精确的源码位置映射。
- **现代化语法分析**: 基于递归下降算法，内置智能错误恢复机制与彩色诊断信息。
- **完善的语义分析**: 支持嵌套作用域管理与多级符号表维护。
- **P-Code 指令生成**: 为专用栈式虚拟机生成紧凑、高效的中间指令。
- **高性能解释器**: 具备调试能力的高速 VM 执​​行引擎。
- **端到端优化器**: 支持常量折叠、死代码消除等多种优化算子。

### 语言扩展特性
- **复杂数据结构**: 支持一维和多维数组。
- **现代控制流**: 支持 `if-then-else`、`while-do`、`repeat-until` 和 `for-loop` (to/downto)。
- **手动内存管理**: 类似 C 语言的显式堆内存分配 (`new` 和 `delete`)。
- **一等公民过程**: 支持过程嵌套、递归调用及词法作用域。

### 开发者工具
- **智能 CLI**: 提供智能文件识别、彩色诊断显示及交互式 REPL 调试器。
- **全方位调试能力**:
  - `查看 Token`: 检查并分析词法分析输出。
  - `AST 可视化`: 直观探索程序抽象语法结构。
  - `P-Code 导出`: 深入分析底层指令生成。
  - `追踪模式`: 实时查看状态并逐步跟踪代码执行。
- **内置测试框架**: 集成批量自动化测试功能，确保回归测试可靠性。
- **Qt5 IDE**: 现代化的图形开发界面，集成了语法高亮和多维度可视化面板。

---

## GUI 调试器

本项目包含一个基于 Qt5 的 IDE，将乏味的编译过程转化为直观的可视化体验。

> [!TIP]
> 使用 GUI 调试器可以直观地观察程序的执行流、检查运行时栈的变化，并实时监控变量的具体数值。

- **交互式断点**: 通过点击行号区域即可快速开启或关闭执行暂停点。
- **状态可视化**: 实时显示符号表、生成的 P-Code 以及抽象语法树 (AST)。
- **内存深度监视**: 实时查看变量、数组元素以及指针指向的目标内容。
- **运行时栈图示**: 以图形化方式展示运行时栈帧和活动记录。

---

## 系统架构

编译器采用模块化架构设计，遵循经典的编译器设计模式。

![系统架构](config/Architecture.png)

---

## 快速开始

### 开发环境要求
- **编译器**: 支持 C++17 标准 (GCC 8+, Clang 7+, 或 MSVC 2019+)
- **构建工具**: CMake 3.16+
- **图形界面依赖**: Qt5 (Core & Widgets) - *可选*

### 编译步骤
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 基本用法
```bash
# 编译并运行 PL/0 程序
./pl0c examples/sample.pl0

# 进入交互式命令行调试模式
./pl0c examples/sample.pl0 --debug

# 启动图形界面 IDE
./pl0gui
```

---

## EBNF 文法

扩展 PL/0 语言支持丰富的语言结构，其 EBNF 定义如下：

```bnf
<prog>      ::= program <id> ";" <block>
<block>     ::= [<condecl>] [<vardecl>] [<proc>] <body>
<condecl>   ::= const <const> {"," <const>} ";"
<const>     ::= <id> ":=" <integer>
<vardecl>   ::= var <vardef> {"," <vardef>} ";"
<vardef>    ::= <id> [":" ("integer" | "pointer") | "[" <integer> "]"]
<proc>      ::= procedure <id> "(" [<id> {"," <id>}] ")" ";" <block> ";" {<proc>}
<body>      ::= begin <statement> {";" <statement>} end
<statement> ::= <id> ["[" <exp> "]"] ":=" <exp>
              | "*" <exp> ":=" <exp>
              | if <lexp> then <statement> [else <statement>]
              | while <lexp> do <statement>
              | for <id> ":=" <exp> ("to" | "downto") <exp> do <statement>
              | call <id> "(" [<exp> {"," <exp>}] ")"
              | <body>
              | read "(" <id> {"," <id>} ")"
              | write "(" <exp> {"," <exp>} ")"
              | new "(" <id> "," <exp> ")"
              | delete "(" <id> ")"
```
*(更多关于因子 (factor) 和表达式 (expression) 的规则，请参阅 [完整文法文档](doc/Grammar.md)。)*

---

## 相关文档

如需深入了解本项目，请参阅以下文档：

- [**语言文法规范**](doc/Grammar.md): 包含因子 (factor) 和表达式 (expression) 的详细 EBNF 规则。
- [**CLI 调试指南**](CLI_DEBUG_GUIDE.md): 命令行交互式调试器的使用手册。
- [**GUI 调试指南**](GUI_DEBUG_GUIDE.md): Qt5 调试器的详细使用说明。

---

## 测试运行

CLI 内置了全面的批量测试功能：

```bash
# 运行默认目录下的所有测试用例
./pl0c --test

# 运行指定分类下的测试
./pl0c --test test/correct
```

---

## 开源许可证

本项目基于 **MIT 许可证** 开源。
Copyright (c) 2025.
