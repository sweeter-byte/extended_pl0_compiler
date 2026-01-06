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

```ebnf
program    = "program" ident ";" block EOF 
block      = [ "const" ident ":=" number { "," ident ":=" number } ";" ]
             [ "var" var_decl { "," var_decl } ";" ]
             { "procedure" ident "(" [ params ] ")" ";" block ";" }
             body 
body       = "begin" statement { ";" statement } "end" 
var_decl   = ident [ ":" ( "integer" | "pointer" ) | "[" number "]" ]
params     = ident { "," ident }
statement  = [ ident [ "[" expression "]" ] ":=" expression
             | "call" ident "(" [ args ] ")"
             | "begin" statement { ";" statement } "end"
             | "if" condition "then" statement [ "else" statement ]
             | "while" condition "do" statement
             | "for" ident ":=" expression ( "to" | "downto" ) expression "do" statement
             | "read" "(" ident { "," ident } ")"
             | "write" "(" expression { "," expression } ")"
             | "new" "(" ident "," expression ")"
             | "delete" "(" ident ")"
             | "*" expression ":=" expression
             ] 
condition  = "odd" expression
           | expression ( "=" | "#" | "<" | "<=" | ">" | ">=" ) expression 
expression = [ "+" | "-" ] term { ( "+" | "-" ) term } 
term       = factor { ( "*" | "/" | "%" ) factor } 
factor     = ident [ "[" expression "]" ]
           | number
           | "(" expression ")"
           | "*" factor
           | "&" ident [ "[" expression "]" ] 
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
