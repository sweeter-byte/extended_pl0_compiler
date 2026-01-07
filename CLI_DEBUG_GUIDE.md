# Extended PL/0 编译器 CLI 调试指南

本文档介绍了 Extended PL/0 编译器（`pl0c`）提供的各种调试功能，包括交互式调试模式和静态诊断输出。

## 1. 交互式调试模式 (`-d`, `--debug`)

通过在运行 `pl0c` 时添加 `-d` 或 `--debug` 选项，可以进入交互式调试模式。

```bash
./pl0c source.pl0 -d
```

### 调试命令

进入调试模式后，程序会暂停在第一条指令前。你可以使用以下命令控制程序执行：

| 命令 | 全称 | 说明 |
| :--- | :--- | :--- |
| `b <line>` | **Breakpoint** | 在指定行设置断点 |
| `s` | **Step** | 单步执行（进入过程调用） |
| `n` | **Next** | 单步执行（跳过过程调用） |
| `r` / `c` | **Run / Continue** | 继续运行直到遇到断点或程序结束 |
| `p <var>` | **Print** | 打印指定变量的值 |
| `q` | **Quit** | 退出调试器并终止程序 |

### 示例

```text
(debug L1)> b 10
Breakpoint set at line 10
(debug L1)> r
(debug L10)> p x
x = 42
(debug L10)> s
(debug L11)> q
```

---

## 2. 诊断输出选项

除了交互式调试，`pl0c` 还提供了多个标志位用于查看编译器中间状态：

| 选项 | 说明 |
| :--- | :--- |
| `--tokens` | 打印词法分析器产生的 Token 序列 |
| `--ast` | 打印抽象语法树（AST） |
| `--sym` | 打印符号表（Symbol Table）内容 |
| `--code` | 打印生成的 P-Code 指令集 |
| `--all` | 启用以上所有静态调试输出 |
| `--trace` | 在执行时逐行追踪 P-Code 的变化 |

### 使用示例

查看程序的 P-Code 指令：
```bash
./pl0c test/integration/correct/cor_01_bubblesort.pl0 --code
```

查看完整的编译器分析过程（Token, AST, Sym, Code）：
```bash
./pl0c my_program.pl0 --all
```

---

## 3. 其他相关选项

- `--no-run`: 仅编译代码，不执行生成的 P-Code。
- `--no-color`: 禁用终端颜色输出。
- `-O`, `--optimize`: 启用代码优化（常量折叠、死代码消除等），查看优化后的中间代码时非常有用。
