#pragma once
#include "../widget.h"
#include "../uix/template_ast.h"
#include "../css/css_ast.h"
#include <memory>
#include <string>
#include <vector>

namespace ui::page {

// A reactive binding: raw JS expression source (rewritten + compiled to a
// closure at attach time, then re-evaluated on dep change with the result
// dispatched to a widget property).
//
//   text interpolation `Hello, {{ name }}!` → `"Hello, " + (name) + "!"`
//   :class="active ? 'on' : 'off'"          → `active ? 'on' : 'off'`
//   v-show="visible"                         → `visible`
//   v-model on `userName`                    → `userName`  (read side)
struct CompiledBinding {
    Widget* target = nullptr;
    std::string property;
    std::string sourceJs;
};

// An event handler: raw JS source, compiled to a closure at attach time.
//   `inc`                — bare ident (resolves to a method, called with $event)
//   `onDel(item.id)`     — call expression
//   `count = count + 1`  — assignment statement
struct CompiledEvent {
    Widget* target = nullptr;
    std::string event;
    std::string sourceJs;
};

// v-model target: on user-input change, write the widget's new value back
// into the named state property (via the JS proxy's set-trap).
struct CompiledModelWrite {
    Widget* target = nullptr;
    std::string propertyName;
};

// v-if: truthy → template subtree mounts as parentWidget's child at
// insertIndex; falsy → it's unmounted. Mount/unmount, not v-show toggle.
struct CompiledConditional {
    Widget* parentWidget = nullptr;
    const ui::uix::Node* templateNode = nullptr;
    std::string condSourceJs;
    size_t insertIndex = 0;
};

// v-for: runtime watches listSourceJs; on any list change, tear down old
// iterations and rebuild from the new array. Per-iteration bindings get
// `loopVar` and (optional) `indexVar` as closure parameters.
struct CompiledLoop {
    Widget* parentWidget = nullptr;
    const ui::uix::Node* templateNode = nullptr;
    std::string loopVar;
    std::string indexVar;
    std::string listName;
    std::string listSourceJs;
    std::string keySourceJs;
    size_t insertIndex = 0;
};

// Static window configuration extracted from a top-level <window .../> tag.
// All fields default to "unset"; consumer merges onto a UiWindowConfig.
struct WindowHints {
    bool        present     = false;  // set if <window> tag existed
    std::wstring title;                // empty → unset
    int         width       = 0;      // 0 → unset (use caller default)
    int         height      = 0;
    int         minWidth    = 0;
    int         minHeight   = 0;
    int         resizable   = -1;     // -1=unset, 0=no, 1=yes
    int         frameless   = -1;     // -1=unset, 0=system frame, 1=frameless custom chrome
    int         centered    = -1;     // -1=unset, 0=use x,y, 1=center on screen
    int         theme       = -1;     // -1=unset, 0=dark, 1=light
};

// 编译期收集的 <menu>: 一棵 ContextMenu 结构 + 可选的"trigger 元素 @ event"
// 自动挂载. PageState::Attach 把它实例化成 ContextMenuPtr 并 wire callback.
struct CompiledMenu;
using CompiledMenuPtr = std::shared_ptr<CompiledMenu>;

struct CompiledMenuItem {
    int          itemId = 0;            // 数字 id, ContextMenu callback 用来识别
    std::string  text;                  // 显示文本（已 i18n desugar 之后的字面量）
    std::string  shortcut;              // 快捷键文本, 例如 "Ctrl+S" (仅显示)
    std::string  onClick;               // 触发时调用的 PageState method 名
    bool         separator = false;     // true → 这是 <separator/>, 其他字段忽略
    /* Phase A: 嵌套 <menu text="..."> 作为 submenu. submenu 非空时,
       当前 item 渲染成 "Recent ▸" 样式, hover/click 弹出 submenu. */
    CompiledMenuPtr submenu;
    /* Phase B: <menuitem>...<svg>...</svg>Save</menuitem> — 内联 SVG 当 icon.
       字符串形式（包含 `<svg ...>...</svg>` 整段），运行时 ParseSvgIcon. 已有
       Renderer::DrawSvgIcon 路径会按 menu 文字色/per-item override 着色, 所以
       SVG 内的 fill 通常用 currentColor. 跟 phase C 的 imgSrc 互斥（同时给的
       话 imgSrc 优先, svg 忽略）. */
    std::string  iconSvg;
    /* Phase C: <menuitem icon="logo.png"> 或 <menuitem><img src="logo.png"/>...</menuitem>.
       imgSrc 是 ui::asset 的 key (跟 <img src> 同一个解析器 chain). 运行时
       Resolve → bytes → Renderer::LoadImageFromBytes → ID2D1Bitmap → menu item.
       光栅图不跟随主题色 (跟 SVG 不同), 所以 dark mode 用 PNG 要自己换图源. */
    std::string  imgSrc;
    /* Phase D: <menuitem style="color: #d63a26"> per-item 颜色覆盖. ContextMenu
       渲染会用它代替默认 textColor; SVG icon 也跟着变色 (DrawSvgIcon 接 color
       参数). PNG icon 不变色. hasColor=false 时走默认主题色. */
    bool         hasColor = false;
    float        color_r = 0, color_g = 0, color_b = 0, color_a = 1;
};

struct CompiledMenu {
    std::string  id;                    // <menu id="X"> ; 可空
    std::string  triggerSelector;       // "#btnId" → 该元素 click/rclick 自动 show menu
    std::string  triggerEvent;          // "click" (默认) | "rclick"
    std::vector<CompiledMenuItem> items;
};

struct CompiledPage {
    WidgetPtr root;
    std::vector<CompiledBinding> bindings;
    std::vector<CompiledEvent> events;
    std::vector<CompiledModelWrite> modelWrites;
    std::vector<CompiledLoop> loops;
    std::vector<CompiledConditional> conditionals;
    std::vector<CompiledMenu> menus;
    // Raw <script> block content (Vue 3 SFC `export default {…}`); fed to
    // ScriptRuntime::EvalModule at attach time.
    std::string scriptSource;
    WindowHints windowHints;
    std::vector<std::string> errors;

    // The original HTML AST + CSS must live through the page's lifetime so v-for
    // iterations can re-compile their template subtree AND so widget recompute
    // lambdas hold stable pointers into the stylesheet (hence heap-allocated).
    std::unique_ptr<ui::uix::Node> ownedHtml;
    std::unique_ptr<ui::css::Stylesheet> ownedStylesheet;

    // CSS variables (`:root { --x: ... }` plus library-injected theme tokens
    // like `--bg / --fg / --accent`). Held via shared_ptr so all widget
    // recomputeStyle lambdas share ONE source of truth — `ui_theme_set_mode`
    // overwrites the contents in place and the next cascade pass on every
    // widget picks up new values without re-binding any lambda.
    std::shared_ptr<std::vector<std::pair<std::string, std::string>>> cssVars;
};

}  // namespace ui::page
