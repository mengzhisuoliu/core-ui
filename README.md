<p align="center">
  <img src="./logo.svg" alt="CORE UI" height="64">
</p>

<p align="center">
  <a href="./README-en.md">English</a> · <b>中文</b>
  &nbsp;·&nbsp;
  <a href="https://ghboke.github.io/core-ui/"><b>📖 在线文档</b></a>
</p>

**Core UI** 是一个现代化的 Windows 桌面 UI 框架，从零重新设计以匹配 Microsoft **Fluent 2** 视觉语言，同时保持**原生级的性能**与**极小的分发体积**。底层基于 **Direct2D / Direct3D 11** 硬件加速渲染，把从按钮、文本框到 Flyout、Dialog、TitleBar 的 25+ 个内置控件统一在一套 **纯 C API** 之下（共 250+ 个导出函数）——Rust、Go、Python、C#、Delphi 乃至 Lua 都能直接调用，不需要写 C++ 绑定层。

推荐用 **`.uix` 单文件组件**（Vue 3 SFC 风格：`<window>` + `<script>` + `<style>` + `<template>`）描述界面：响应式数据绑定、computed / methods、`v-if` / `v-for` / `v-model` / `@click`、CSS 子集和 CSS 变量主题——脚本由内置的 **QuickJS-NG** 在原生进程内求值，没有 DOM、没有 Webview。项目还专门为 **AI 协作**而设计：一份自包含的速查文档（[`docs/uix-ai-guide.md`](./docs/uix-ai-guide.md)）让 LLM 一次读完就能生成完整的可运行应用，真正做到"描述需求 → 得到界面"的极速原型闭环。

> **8.4 MB 一个 DLL，就能写出跟 Office / VSCode 同一设计语言的 Windows 桌面应用。**
> 不要 Chromium、不要 .NET、不要 Qt 几十兆的 moc/uic——一个 C 头文件，一份 `.uix` 单文件组件，搞定。

![version](https://img.shields.io/badge/version-1.5.0.42-blue)
![license](https://img.shields.io/badge/license-MIT-green)
![platform](https://img.shields.io/badge/platform-Windows%2010%2B-lightgrey)
![size](https://img.shields.io/badge/dll-8.4MB-brightgreen)
![api](https://img.shields.io/badge/C%20API-250%2B-orange)
![script](https://img.shields.io/badge/script-QuickJS--NG-purple)

## 🎯 为什么选 Core UI

| 对比维度 | Electron | WPF / WinUI 3 | Qt | **Core UI** |
|---------|----------|---------------|-----|-------------|
| **分发体积** | 100+ MB | 需要 .NET 运行时 | 40+ MB Qt DLLs | **8.4 MB 单 DLL** |
| **启动时间** | 1–3 秒 | 0.5–1 秒 | 0.5–1 秒 | **< 200 ms** |
| **内存占用** | 150+ MB | 80+ MB | 60+ MB | **< 30 MB** |
| **语言绑定** | 只能 JS | 只能 .NET | 只能 C++ | **C ABI，任意语言** |
| **设计规范** | 自己画 | Fluent（受限） | 类原生 | **Fluent 2 原生级** |
| **声明式 / 响应式 UI** | JSX + 虚拟 DOM | XAML + Binding | QML | **`.uix` Vue 3 SFC（QuickJS-NG）** |
| **学习曲线** | 大前端生态 | XAML + C# | C++ + Meta 对象 | **Vue 模板 + C 即上手** |

**一句话总结**：想要 Electron 的开发体验 + 原生的性能 + Fluent 2 的颜值 + Vue 的响应式心智模型——Core UI 是目前唯一同时打满几格的方案。

## 🤖 AI 时代的快速 UI 开发方案

**Core UI 是为 LLM 和 AI Agent 从零设计的 UI 框架。**

现在大家都让 AI 写代码。你试过让 Claude / GPT / Cursor 写一个 Qt 应用吗？它会告诉你去看 `moc`、帮你 include 一堆用不上的头、还经常搞错信号槽语法。写 WPF？它会把 XAML、C# code-behind、x:Bind 表达式混着编。原因很简单：**这些框架的心智负担不是为 token 预算设计的**。

Core UI 的架构天然让 AI 做对：

| 设计决策 | AI 收益 |
|---------|---------|
| **`.uix` 单文件组件 = Vue 3 SFC** | LLM 训练语料里 Vue / React / HTML 的占比远超任何桌面 UI，模板 + `data/computed/methods` 一次成形 |
| **纯 C ABI，全部 `uint64_t` 句柄** | 没有 C++ 模板、没有继承、没有虚函数——LLM 极难幻觉出类型错误 |
| **CSS 风格样式 + Flexbox** | 直接套用前端排版直觉，不用学新的 grid / 锚点 / dock 体系 |
| **全部 API 遵循 `ui_<名词>_<动词>`** | 可被精确预测：想加载 `.uix`？`ui_page_load_file` 几乎一次猜对 |
| **一份 `docs/uix-ai-guide.md` 自包含速查** | Agent 只需 fetch 这一个文件，就能写出完整应用，不必遍历仓库 |
| **250+ 导出函数全部进 DLL 符号表** | Agent 可以动态检查 `objdump -p core-ui.dll` 知道当前版本真实 API |
| **所有文本走 `$t('key')`** | Agent 生成的 UI 天然多语言、数据驱动，不写死 |
| **`ui_debug_*` 事件注入** | Agent 自己点击 / 输入 / 截图，不需要真鼠标键盘——实现 emit → build → click → verify 闭环 |

典型工作流：

```
用户："帮我做一个 Windows 桌面下载器 UI，要 Fluent 外观"
  ↓
AI fetch  docs/uix-ai-guide.md   （一个文件，全部知识）
  ↓
AI 生成   app.uix + main.cpp     （单文件 SFC + 10 行 C）
  ↓
build-clang-cl.ps1 -Target ...   （编译器兜底，LLM 不用自己验证）
  ↓
ui_debug_*  自动点击 + 截图       （AI 自己跑回归，闭环）
```

### 📖 AI 文档入口

| 入口 | 给谁用 | 说明 |
|------|-------|------|
| **[`llms.txt`](./llms.txt)** | AI Agent 爬虫 | [llmstxt.org](https://llmstxt.org) 标准索引，Agent 第一步 fetch 这个 |
| **[`docs/uix-ai-guide.md`](./docs/uix-ai-guide.md)** | LLM 编程 | **自包含速查表**：`.uix` 文件结构 + 模板 + 脚本 + CSS 子集 + widget 列表 + 例子 |
| **[`docs/uix-guide.md`](./docs/uix-guide.md)** | 深入参考 | Vue 3 SFC 完整指南、cookbook、限制 |
| **[`docs/debug-simulation.md`](./docs/debug-simulation.md)** | 自动化 | `ui_debug_*` 事件注入 + Named Pipe IPC，AI 自验证闭环 |
| **[`UI_CORE_API.md`](./UI_CORE_API.md)** | API 全量 | 250+ 导出函数完整清单，按模块分组 |

**Cursor / Claude Code / Cline / Continue 等用户推荐把 `docs/uix-ai-guide.md` 加到项目规则里**（Cursor 的 `.cursorrules`、Claude Code 的 `CLAUDE.md`），一次上下文全覆盖。

提示词模板（复制即用）：

```
本项目使用 Core UI 框架。生成代码前先阅读 docs/uix-ai-guide.md。
约束：
  1. UI 用 .uix 单文件组件描述，C++ 只写 ui_init / ui_page_load_file / ui_run 这层胶水。
  2. <script> 必须是 Vue 3 SFC：export default { data(){...}, computed:{}, methods:{} }。
  3. 颜色用 var(--bg) / var(--fg) / var(--accent) 等 CSS 变量，不要硬编码 hex。
  4. 文本用 {{ $t('key') }} 引用 .lang 文件，不要写死字符串。
  5. 模板事件：@click / @change / @input / @focus / @blur / @mousedown / @wheel ...
```

## ✨ 核心特性

### 🚀 轻到离谱，快到失真

- **8.4 MB 全量 DLL**，静态编译后 demo exe 仅 **1 MB**，可装进 U 盘跑
- **Direct2D + Direct3D 11** 全硬件加速，Per-Monitor DPI V2 一次画对
- **冷启动 < 200ms**，对 `.uix` 文件路径点一下就能看到窗口

### 🎨 颜值即正义

- 严格对齐 **Microsoft Fluent 2 Design Token**：色彩、圆角、阴影、动画无一例外
- 深色 / 浅色主题**一行切换**，所有内置控件自动响应；CSS 变量随之 cascade
- **自定义无边框窗口**自带 `<TitleBar>` 控件，系统级拖拽 / 贴靠 / 动画
- 25+ 控件颗粒度对标 WinUI 3：`button` / `input` / `toggle` / `combobox` / `progressbar` / `menu` / `Dialog` / `Toast`...

### 🧩 `.uix` 单文件组件 — 像写 Vue 一样写桌面 UI

```vue
<window title="Hello" width="400" height="300" centered="true" theme="light"/>

<script>
export default {
  data()    { return { count: 0 }; },
  computed: { doubled() { return this.count * 2; } },
  methods:  { inc() { this.count++; } }
}
</script>

<style>
  .root  { padding: 24px; gap: 12px; background: var(--bg); }
  .h1    { font-size: 22px; color: var(--fg); font-weight: 600; }
  button { background: var(--accent); color: #fff;
           padding: 6px 14px; border-radius: 4px; cursor: pointer; }
</style>

<template>
  <div class="root">
    <label class="h1">Hello, Core UI!</label>
    <label>count = {{ count }} · doubled = {{ doubled }}</label>
    <button @click="inc">+1</button>
  </div>
</template>
```

- **Vue 3 Options API**：`data()` / `computed` / `methods`，QuickJS-NG (ES2020+) 求值
- **响应式系统**：Proxy + WatchEffect，模板里 `{{ expr }}` / `:attr` / `v-if` / `v-for` / `v-model` / `@click` 自动收集依赖、增量重渲染
- **CSS 子集**：类 / 标签 / 后代选择器、伪类（`:hover` / `:disabled`）、Flexbox、`var(--*)` CSS 变量主题
- **`<window>` 标签**：声明式窗口配置（标题 / 尺寸 / 无边框 / 主题），无需写一行 C
- **i18n 内置**：`{{ $t('welcome') }}` 自动查 `.lang` 文件，运行时 `ui_page_set_locale` 切语言
- **嵌套 `v-for > v-if > v-for` 安全**，keyed diff 跨 reorder 复用控件
- **声明式右键菜单**：`<menu trigger="#id" event="rclick">` + `<menuitem>` / `<separator>`

### 🌐 纯 C API，所有语言都能调

```c
#include <ui_core.h>

ui_init_with_theme(UI_THEME_LIGHT);

UiPage page = ui_page_load_file(L"app.uix");
ui_page_load_language_file(page, "zh", L"lang/zh.lang");
ui_page_set_locale(page, "zh");

UiWindow win = ui_page_open_window(page, NULL);

/* 双向交换 reactive 状态 */
ui_page_set_int (page, "count", 42);
ui_page_set_text(page, "name",  L"Alice");
ui_page_set_json(page, "items", "[{\"id\":1,\"label\":\"a\"}]");
char* j = ui_page_get_json(page, "items");
ui_page_free(j);

ui_run();
ui_page_destroy(page);
```

- **250+ 导出函数**，句柄全部 `uint64_t`，没有一个 C++ 类型泄漏
- Rust / Go / Python / C# / Delphi / Pascal / Lua 全部能直接绑定
- Page API（`ui_page_*`）为 `.uix` 而生；底层 widget 工厂（`ui_vbox`、`ui_button` ...）仍可过程式构造控件树

### 🔍 自动化 / 调试：控件可编程驱动

从 1.1.0 起新增一整套 **`ui_debug_*` 事件注入 API** —— 无需真实鼠标键盘即可
驱动任意控件，为端到端测试、AI Agent 操作 UI、脚本化回归设计：

```c
ui_debug_click(win, btn);                    // 完整 mouse down/up，触发 onClick
ui_debug_combo_select(win, combo, 2);        // 选中第 3 项 + 触发 onChanged
ui_debug_right_click_at(win, 300, 200);      // 右键弹出注册的 context menu
int path[] = {2, 0};
ui_debug_menu_click_path(win, path, 2);      // 点 submenu 里的叶子
ui_debug_type_text(win, L"hello");           // 逐字符键盘输入
```

- **60+ 个 `ui_debug_*` 函数**：click / hover / drag / wheel / key / focus / 各控件
  高层操作 / 子菜单 path 点击 / HWND `PostMessage` 通道 …
- 内置 **Named Pipe IPC**（`\\.\pipe\ui_core_debug`）45+ 条命令，PowerShell / Python
  一行能驱动；`ui_debug_server_start(win, NULL)` 启动
- **线程安全工具** `ui_window_invoke_sync` 把工作线程的调用 marshal 到 UI 线程
- 完整参考见 **[`docs/debug-simulation.md`](./docs/debug-simulation.md)**

## 📊 真实数据

| 指标 | 数值 |
|------|------|
| `core-ui.dll` 体积（全功能） | **8.4 MB** |
| `ui-demo-uix.exe` 体积（静态链接） | **~1 MB** |
| 空窗口内存占用 | **< 30 MB** |
| 冷启动到首帧 | **< 200 ms** |
| 60 fps 动画 CPU 占用 | **< 3%** |
| 导出 C 函数数量 | **250+** |
| 内置控件 | **25+** |
| 脚本运行时 | **QuickJS-NG v0.14.0**（ES2020+） |
| 支持的图片格式（ImageView） | PNG / JPG / BMP / GIF / WebP / SVG |

## 🚀 快速开始

### 环境要求

- Windows 10 (1709+)
- CMake 3.20+
- **MSVC 2019+ 或 clang-cl**（C++17）— 不支持 MinGW

### 构建

从 **Visual Studio Developer Command Prompt**（或先 source `vcvars64.bat`）开一个终端，然后：

```bash
git clone https://github.com/ghboke/core-ui.git
cd core-ui
cmake -B build -G Ninja
cmake --build build --target core-ui
```

常用 target：

| Target | 产物 |
|---|---|
| `core-ui` | `core-ui.dll` + `core-ui.lib` 导入库（默认） |
| `core-ui-static` | `core-ui-static.lib` 自包含静态归档（含 QuickJS） |
| `ui-demo-uix` | `ui-demo-uix.exe` 单文件 demo（资源烤进 exe） |

不指定 `--target` 就编全部。Visual Studio 17 2022 generator 也能用：

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

单 exe 分发（无 DLL 依赖，所有代码 + QuickJS 静态链进 exe）：

```bash
cmake -B build -G Ninja -DUI_CORE_STATIC=ON
cmake --build build
```

### Hello World

**hello.uix**

```vue
<window title="Hello" width="400" height="300" centered="true" theme="light"/>

<script>
export default {
  data()    { return { count: 0 }; },
  methods:  { inc() { this.count++; } }
}
</script>

<style>
  .root  { padding: 24px; gap: 12px; background: var(--bg); }
  button { background: var(--accent); color: #fff;
           padding: 6px 14px; border-radius: 4px; }
</style>

<template>
  <div class="root">
    <label>count = {{ count }}</label>
    <button @click="inc">+1</button>
  </div>
</template>
```

**main.cpp**

```cpp
#include <ui_core.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    ui_init_with_theme(UI_THEME_LIGHT);

    UiPage page = ui_page_load_file(L"hello.uix");
    if (!page) return 1;

    UiWindow win = ui_page_open_window(page, NULL);
    if (!win) { ui_page_destroy(page); return 2; }

    int ret = ui_run();
    ui_page_destroy(page);
    return ret;
}
```

就这么多。**没有 .vcxproj、没有 moc 预处理器、没有 XAML 编译器、没有 IDL。**

### 单 exe 打包（资源烤进可执行文件）

```cmake
include(cmake/UiCoreHelpers.cmake)

add_executable(my-app WIN32 main.cpp)
target_link_libraries(my-app PRIVATE core-ui)

ui_core_embed_text(my-app FILE app.uix      OUT app.embed.h     VAR k_app)
ui_core_embed_text(my-app FILE lang/zh.lang OUT lang_zh.embed.h VAR k_lang_zh)
```

```cpp
#include "app.embed.h"
#include "lang_zh.embed.h"

UiPage page = ui_page_load_string(k_app);
ui_page_load_language_string(page, "zh", k_lang_zh);
```

`demo/ui_demo_uix.cpp` 就是这种用法的最小完整例子（57 行胶水 + 单 `.uix` 文件 12 页 demo）。

### 🛠️ 故障排查

#### 1. ninja link 阶段卡死（Windows SDK rc.exe bug）

**症状**：build 看似在 link / 资源编译阶段卡死、CPU 不动；查 `build/CMakeFiles/<target>.dir/CMakeFiles/<target>-version.rc.res` 是 **0 字节**，同目录有 9 KB 左右的 `RCa*` 临时文件。`tasklist` 看到 `cmake.exe` + `rc.exe` 进程在跑但内存恒定。

**原因**：某些 Windows SDK 版本的 `rc.exe` 在 ninja 子进程上下文里有死锁 bug。

**修复**：让 cmake 用 LLVM 的 `llvm-rc.exe` 替代系统 `rc.exe`：

```bash
# 找到 llvm-rc.exe 路径（通常随 LLVM / clang-cl 一起装）
where llvm-rc

cmake -B build -G Ninja -DCMAKE_RC_COMPILER=C:/path/to/llvm-rc.exe
```

#### 2. `cmake -B build` 报 `core-ui only supports MSVC / clang-cl`

**原因**：cmake 没识别到 MSVC 工具链——多半是终端没初始化 vcvars。

**修复**：

- 用 **Visual Studio Developer Command Prompt** 开终端
- 或在普通 cmd 里先运行：`"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"`（路径按你装的 VS 版本调整）

#### 3. 静态链接缺 `JS_*` 符号

**症状**：用 `core-ui-static.lib` 静态链接时报 `unresolved external __imp_JS_*` ~50 个。

**原因**：`core-ui-static.lib` 不含 QuickJS 实现。**用 install target 生成的 `lib/static/core-ui.lib`** 才是合并了 QuickJS 的胖归档：

```bash
cmake --build build --target release-package
# 产物在 release/core-ui-vX.Y.Z/lib/static/core-ui.lib
```

## 🧩 内置控件 / 标签

`.uix` 模板里的标签直接映射到原生 widget：

| 类别 | 标签 |
|---|---|
| **容器** | `div`（Flexbox：`flex-direction` / `flex` / `gap` / `padding`）|
| **文本** | `label`（多行、自动换行）|
| **按钮** | `button`、`IconButton` |
| **输入** | `input`（type=`text` / `password` / `checkbox` / `radio` / `range` / `number`）、`textarea` |
| **选择** | `toggle`、`combobox` |
| **状态** | `progressbar`，`badge` 类（CSS）|
| **弹出** | `menu` / `menuitem` / `separator`、`Flyout`、`Dialog`、`Toast` |
| **图像** | `img`、`svg`（内联），底层 `ImageView` 支持缩放 / 平移 / 裁剪 |
| **窗口** | `TitleBar`（仅 `frameless="true"` 时使用） |

需要程序化构造时，C API 也提供 `ui_vbox` / `ui_hbox` / `ui_label` / `ui_button` / `ui_text_input` / `ui_combobox` / `ui_slider` / `ui_progress_bar` / `ui_image_view` / `ui_dialog` / `ui_tab_control` / `ui_scroll_view` 等工厂函数。

## 🎨 主题

内置 Fluent 2 深色 / 浅色主题，运行时一行切换：

```c
ui_theme_set_mode(UI_THEME_DARK);
ui_theme_set_mode(UI_THEME_LIGHT);
```

`.uix` 的 `<style>` 用 CSS 变量引用主题色，切主题时由库重新 cascade，所有控件自动响应：

```css
var(--bg) / var(--fg) / var(--fg-2) / var(--fg-3)
var(--accent) / var(--card-bg) / var(--sidebar-bg)
var(--sidebar-hover) / var(--border-subtle)
```

## 🔢 版本号

版本号格式：`MAJOR.MINOR.PATCH.BUILD`（当前 `1.5.0.42`）

- 编译期宏：`UI_CORE_VERSION_MAJOR/MINOR/PATCH/BUILD` + `UI_CORE_VERSION_STRING`
- 运行时 API：

```c
int major, minor, patch;
ui_core_version(&major, &minor, &patch);   // 1, 5, 0
int build = ui_core_version_build();        // 42
const char* v = ui_core_version_string();   // "1.5.0.42"
```

- Windows DLL 属性页（右键 `core-ui.dll` → 属性 → 详细信息）会显示 `FileVersion 1.5.0.42`
- 完整发布历史见 GitHub [Releases](https://github.com/ghboke/core-ui/releases) 页

## 📁 项目结构

```
core-ui/
├── include/
│   ├── ui_core.h         # 公共 C API（250+ 函数）
│   └── plugin_api.h
├── src/ui/
│   ├── renderer.*        # Direct2D 渲染引擎
│   ├── widget.*          # 基础控件 + 布局
│   ├── controls.*        # 所有内置控件
│   ├── ui_window.*       # 窗口管理 + 事件分发
│   ├── ui_api.cpp        # 核心 C API 实现
│   ├── ui_debug_server.* # Named Pipe 调试 IPC
│   ├── animation.*       # 动画系统
│   ├── context_menu.*    # 右键菜单
│   ├── image_*           # 图片解码 / GDI / SVG / GIF
│   ├── uix/              # .uix SFC 解析 + QuickJS 脚本运行时
│   ├── markup/           # .ui XML markup（兼容旧路径）
│   ├── reactive/         # Proxy + WatchEffect 响应式绑定
│   ├── css/              # CSS 解析 / 选择器 / cascade
│   ├── flex/             # Flexbox 布局
│   ├── page/             # Page API（widget_factory / compiler / state）
│   └── expression/       # JSON 互操作
├── demo/
│   ├── ui_demo.uix       # 12 页 SFC demo
│   ├── ui_demo_uix.cpp   # 60 行胶水 + locale poll
│   └── lang/             # 中文 / 英文语言包
├── docs/                 # 文档
├── cmake/                # UiCoreHelpers.cmake (embed_text)
├── website/              # 文档站（Vite + React）发布到 GitHub Pages
├── .github/workflows/    # GitHub Actions（Pages 部署）
├── CMakeLists.txt
├── LICENSE
├── README.md / README-en.md
├── UI_CORE_API.md
├── VERSION / version.json
├── llms.txt
└── logo.svg
```

## 📚 文档

| 文档 | 内容 |
|------|------|
| [快速上手](docs/getting-started.md) | 集成指南（CMake + MSVC / clang-cl）|
| [.uix AI 速查](docs/uix-ai-guide.md) | **给 AI 喂的 1 页 prompt**，写 `.uix` 单文件组件 |
| [.uix 详细指南](docs/uix-guide.md) | Vue 3 SFC 完整参考（脚本 / CSS / widget / cookbook / 限制）|
| [调试 & 自动化](docs/debug-simulation.md) | Named Pipe 事件注入 API，AI 自验证闭环 |
| [C API 参考](docs/c-api.md) | 每个函数的参数级说明 |
| [控件](docs/controls.md) | 控件详细说明 |
| [布局](docs/layout.md) | Flexbox / 百分比 / 绝对定位 |
| [设计系统](docs/design-system.md) | Fluent 2 设计规范 |
| [国际化](docs/i18n.md) | `.lang` 文件 + `$t()` |
| [.ui markup 速查](docs/ai-guide.md) | 旧 `.ui` XML markup 速查（兼容路径）|
| [.ui markup 详细](docs/markup.md) | `.ui` 文件语法 |
| [API 总览](UI_CORE_API.md) | 全部导出函数清单 |

## 🤝 适用场景

- ✅ **Windows 工具类桌面应用**（下载器、图片查看器、配置管理器、资料库工具）
- ✅ **需要 Fluent 外观但不愿被 .NET / WinUI 绑架**的原生项目
- ✅ **Rust / Go / Python 想要 Fluent 界面**但找不到合适绑定
- ✅ **嵌入到现有 C++ 项目**作为 UI 层（无第三方运行时）
- ✅ **离线分发**，体积敏感，不能带 Electron 全家桶
- ✅ **AI 驱动的 UI 生成**：把 `.uix` 当目标格式，闭环 emit → build → click → screenshot

## 📝 许可证

[MIT License](./LICENSE) © core-ui contributors

— 如果这个项目让你少写了一个 Electron 应用，**请点一个 Star ⭐**。
