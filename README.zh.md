# 扩展 PL/0 编译器系统

这是一个基于 C++17 构建的扩展 PL/0 语言编译器和解释器项目。该项目包含功能强大的命令行界面 (CLI) 和可选的基于 Qt5 的图形用户界面 (GUI)，并支持数组、循环语句和堆内存分配等现代 PL/0 扩展特性。

## 功能特性

- **核心编译器**:
  - **词法分析器 (Lexer)**: 高效的分词处理，支持详细的源码位置追踪。
  - **语法分析器 (Parser)**: 带有完善错误恢复机制的递归下降分析器。
  - **语义分析**: 完整的符号表管理和作用域处理。
  - **代码生成**: 生成用于虚拟机执行的 P-Code 指令。
  - **解释器**: 基于栈的虚拟机，用于执行生成的 P-Code。
  - **优化器**: 多遍代码优化（常量折叠、死代码消除、强度削减）。

- **语言扩展**:
  - **数据结构**: 支持一维和多维数组。
  - **控制流**: 支持 `if-then-else`、`while-do`、`repeat-until` 和 `for-loop` 结构。
  - **内存管理**: 支持显式的堆内存分配与释放 (`new`/`free`)。
  - **过程调用**: 支持嵌套过程定义和递归调用。

- **工具链**:
  - **智能 CLI**: 智能文件路径解析和彩色诊断输出。
  - **调试模式**: 支持查看 Token 序列、AST（抽象语法树）、符号表和 P-Code。
  - **追踪模式**: 支持 P-Code 的逐条指令执行追踪。
  - **测试运行器**: 内置批量测试框架。
  - **GUI**: 基于 Qt5 的集成开发环境，提供语法高亮和编译过程可视化。

## EBNF 文法

### PL/0 语言的 BNF 描述（扩充的巴克斯范式表示法）

```bnf
<prog> → program <id>; <block>
<block> → [<condecl>][<vardecl>][<proc>]<body>
<condecl> → const <const>{, <const>};
<const> → <id> := <integer>
<vardecl> → var <vardef>{, <vardef>};
<vardef> → <id> [ : (integer | pointer) | [ <integer> ] ]
<proc> → procedure <id>([<id>{, <id>}]); <block>; { <proc> }
<body> → begin <statement>{; <statement>} end
<statement> → <id> [ [ <exp> ] ] := <exp>
            | * <exp> := <exp>
            | if <lexp> then <statement> [else <statement>]
            | while <lexp> do <statement>
            | for <id> := <exp> (to | downto) <exp> do <statement>
            | call <id>([<exp>{, <exp>}])
            | <body>
            | read (<id>{, <id>})
            | write (<exp>{, <exp>})
            | new (<id>, <exp>)
            | delete (<id>)
<lexp> → <exp> <lop> <exp> | odd <exp>
<exp> → [+|-]<term>{<aop><term>}
<term> → <factor>{<mop><factor>}
<factor> → <id> [ [ <exp> ] ] | <integer> | (<exp>) | & <id> [ [ <exp> ] ] | * <factor>
<lop> → = | <> | < | <= | > | >=
<aop> → + | -
<mop> → * | / | %
<id> → l{l|d}   （注：l表示字母）
<integer> → d{d}

注释：
<prog>：程序 ；<block>：块、程序体 ；<condecl>：常量说明 ；<const>：常量；
<vardecl>：变量说明 ；<proc>：分程序 ； <body>：复合语句 ；<statement>：语句；
<exp>：表达式 ；<lexp>：条件 ；<term>：项 ； <factor>：因子 ；
<aop>：加法运算符；<mop>：乘法运算符； <lop>：关系运算符。
```

## 环境要求

- **C++ 编译器**: 支持 C++17 标准 (GCC, Clang, 或 MSVC)
- **CMake**: 3.16 或更高版本
- **Qt5** (可选): 仅编译 GUI (`pl0gui`) 时需要。
  - 组件: `Qt5Core`, `Qt5Widgets`

## 编译指南

克隆仓库并使用 CMake 进行构建：

```bash
mkdir build
cd build
cmake ..
make
```

### 构建选项

- `-DCMAKE_BUILD_TYPE=Debug`: 构建包含调试信息的版本。
- `-DCMAKE_BUILD_TYPE=Release`: 构建优化后的发布版本 (默认)。

如果有检测到 Qt5 环境，系统将自动构建 GUI 应用程序 `pl0gui`。如果未找到，则仅构建命令行工具 `pl0c`。

## 使用说明

### 命令行接口 (CLI)

`pl0c` 是编译器的主要入口程序。

**基本编译与运行:**

```bash
./pl0c source.pl0
```

**查看生成的 P-Code:**

```bash
./pl0c source.pl0 --code
```

**查看完整调试输出:**

```bash
./pl0c source.pl0 --all
```

**可用选项:**

| 选项 | 说明 |
|------|------|
| `-h, --help` | 显示帮助信息 |
| `-v, --version` | 显示版本信息 |
| `--tokens` | 打印词法分析 Token 序列 |
| `--ast` | 打印抽象语法树 (AST) |
| `--sym` | 打印符号表内容 |
| `--code` | 打印生成的 P-Code 指令 |
| `--all` | 启用所有调试输出 |
| `--trace` | 逐步追踪 P-Code 执行过程 |
| `--no-run` | 仅编译，不执行 |
| `--no-color` | 禁用彩色输出 |
| `--test [dir]` | 运行批量测试 (默认: `test/`) |
| `-O, --optimize` | 开启实验性代码优化 |
| `-d, --debug` | 进入交互式调试模式 (断点、单步、运行) |

### 图形用户界面 (GUI)

构建完成后，可运行 GUI 程序：

```bash
./pl0gui
```

GUI 提供以下功能：
- 带语法高亮的源代码编辑器。
- Token 列表、符号表和 P-Code 的实时可视化展示。
- 包含执行结果的控制台输出面板。

## 测试

项目在 `test/` 目录下包含全面的测试套件，按组件（词法、语法等）和预期结果（正确、错误）分类。

**运行所有测试:**

```bash
./pl0c --test
```

**运行特定分类的测试:**

```bash
./pl0c --test test/parser
```

**测试目录结构:**
- `correct/`: 符合语法的正确源文件。
- `error/`: 包含特定错误以测试编译器报错/恢复能力的源文件。

## 文档

详细文档位于 `doc/` 目录：
- 系统设计与架构
- 用户使用指南
- 测试报告
- `Extension_Plan.md`: 未来扩展计划详解。
- `GUI_Debugger_Integration_Guide.md`: GUI调试器集成指南。
- `Test_Report.md`: 综合测试报告。

## 许可证

本项目采用 MIT 许可证。
Copyright (c) 2025.
