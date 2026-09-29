# easysearch — 文件快速搜索工具

在当前目录下按文件名关键词搜索文件，支持打开、回收站删除，以及生成文件索引。  
原 Python + Tkinter 版本（`ck.pyw`）的 **C++17 / Qt6 重构版**。

---

## 截图预览

```
┌──────────────────────────────────────────┐
│  文件查询器                           ☐☐☐ │
├──────────────────────────────────────────┤
│  请输入查询的字符:                        │
│  ┌──────────────────────────────────┐    │
│  │ report                           │    │
│  └──────────────────────────────────┘    │
│  [ 查询文件 ]                             │
│  ┌──────────────────────────────────┐    │
│  │ 1、report_2024.docx              │    │
│  │ 2、weekly_report.pdf             │    │
│  │ 3、image_report.png              │    │
│  └──────────────────────────────────┘    │
│  [ 打开 ] [ 删除 ] [ 生成 tree.tree ]    │
├──────────────────────────────────────────┤
│ tree.tree 已生成 (42 个文件/文件夹)       │
└──────────────────────────────────────────┘
```

---

## 功能

| 功能 | 操作 | 说明 |
|---|---|---|
| **搜索** | 输入关键词 → 回车 或 点"查询文件" | **优先从 `tree.tree` 索引检索**，不存在则扫描目录。不区分大小写 |
| **打开** | 选中条目 → 点"打开选中文件" | 调用系统默认程序（可多选） |
| **删除** | 选中条目 → 点"删除选中文件" | **移至回收站**，不是永久删除（可多选） |
| **生成 tree.tree** | 点"生成 tree.tree" 或 命令行 `-g` | 递归遍历所有文件，按文件名排序写入索引 |

---

## tree.tree 索引

### 检索优先级

```
查询时 ── tree.tree 存在？ ──→ 是 ──→ 从 tree.tree 过滤（内存操作，快）
                                └─ 否 ──→ 遍历目录（entryList，慢）
```

状态栏显示数据来源：`[tree.tree]` 或 `[目录扫描]`。

### 文件格式

```
=== FILE TABLE ===              ← 明文头部
.gitignore|.gitignore           ← 文件名|相对路径
CMakeLists.txt|CMakeLists.txt
main.cpp|main.cpp
build/easysearch.exe|build/easysearch.exe
...                             ← 按文件名排序
--- END OF FILE TABLE ---       ← 列表终止符
```

| 部分 | 说明 |
|---|---|
| `=== FILE TABLE ===` | 文件表起始标记 |
| `.gitignore` | 文件名 |
| `.gitignore` | 相对于当前目录的路径 |
| `--- END OF FILE TABLE ---` | **列表终止符**，标记文件表结束 |

### 设计要点

- **明文存储**：文件表完全明文，可直接阅读和编辑
- **列表终止符**：`--- END OF FILE TABLE ---` 标记边界，解析器精确停止
- **检索加速**：`tree.tree` 存在时只需在内存中做字符串匹配，无需遍历磁盘
- **命令行生成**：`easysearch -g` 或 `easysearch --generate-tree` 无需启动 GUI

---

## 构建

### 环境要求

| 依赖 | 版本 |
|---|---|
| 编译器 | C++17（GCC 8+ / MSVC 2019+ / Clang 10+） |
| CMake | ≥ 3.16 |
| Qt | Qt 6.x（推荐）或 Qt 5.15+（仅需 `Widgets` 模块） |

### 编译步骤

```bash
cd easysearch
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

> 若使用 MSVC：`cmake -B build -G "Visual Studio 17 2022"`

---

## 运行

### GUI 模式

```bash
run.bat
```

### 命令行模式（生成 tree.tree 后退出）

```bash
build\easysearch -g
build\easysearch --generate-tree
```

---

## 项目结构

```
easysearch/
├── CMakeLists.txt       # CMake 构建配置（Qt5/Qt6 双兼容）
├── main.cpp             # 全部源码（~280 行）
├── ck.pyw               # 原 Python 版（保留参考）
├── run.bat              # Windows 启动脚本
├── build/               # 编译输出（gitignore）
├── .gitignore
└── README.md
```

---

## 与原 Python 版（ck.pyw）的对比

| 方面 | Python + Tkinter | C++ + Qt6 |
|---|---|---|
| **搜索** | `os.listdir()` 每次都遍历磁盘 | **优先 `tree.tree` 索引**，回退 `QDir::entryList()` |
| **文件路径** | 解析带编号的字符串 `"1、name"` → 拼接路径 | `QListWidgetItem::setData(Qt::UserRole, fullPath)` |
| **打开文件** | `os.startfile()` | `QDesktopServices::openUrl()` |
| **删除文件** | `send2trash.send2trash()`（需第三方库） | `QFile::moveToTrash()`（Qt6 内置） |
| **多选** | 仅单选 | `ExtendedSelection` 多选模式 |
| **文件索引** | 无 | **`tree.tree`**：递归遍历 + 明文文件表 + 列表终止符 |
| **命令行模式** | 无 | `-g` / `--generate-tree` |
| **状态提示** | 无 | `QStatusBar` 显示来源 `[tree.tree]` / `[目录扫描]` |
| **跨平台** | 依赖 `send2trash` | 开箱即用（Qt 跨平台封装） |
| **依赖数量** | Python + tkinter + send2trash | Qt6::Widgets 仅一个模块 |

---

## 常见问题

### Q: 双击 exe 报"无法定位程序输入点"

`windeployqt` 部署的 Qt DLL 与系统上实际安装的 **libharfbuzz** 等库版本不匹配。

**解决**：始终用 `run.bat` 启动，或在 PATH 中将 MSYS2 的 `ucrt64\bin` 放在 `build\` 前面。

### Q: 能搜索子目录吗？

当前搜索通过 `tree.tree` 索引支持所有递归子目录（生成时已递归）。若没有 `tree.tree`，则只搜索当前目录一级。

### Q: Qt5 兼容吗？

兼容。`QFile::moveToTrash()` 是 Qt6 新增 API，Qt5 下自动回退到 `QFile::remove()`（永久删除，不经过回收站）。
