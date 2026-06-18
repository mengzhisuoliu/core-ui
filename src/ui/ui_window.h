#pragma once

#ifndef UI_API
  #if defined(UI_CORE_STATIC)
    #define UI_API
  #elif defined(UI_CORE_BUILDING)
    #define UI_API __declspec(dllexport)
  #else
    #define UI_API __declspec(dllimport)
  #endif
#endif

#include <windows.h>
#include <shellapi.h>
#include <cstdint>
#include <functional>
#include <string>

#include "renderer.h"
#include "widget.h"
#include "event.h"
#include "context_menu.h"

namespace ui {


class UI_API UiWindowImpl {
public:
    UiWindowImpl();
    ~UiWindowImpl();

    bool Create(const wchar_t* title, int width, int height,
                bool borderless, bool resizable, bool acceptFiles,
                int x = CW_USEDEFAULT, int y = CW_USEDEFAULT,
                bool toolWindow = false,
                HWND ownerHwnd = nullptr);  /* Build 65+ (L14) */
    void Show();
    void ShowImmediate();  /* 跳过开场动画 */
    /* PrepareRT 已完成 frame 确立 (premax 的 SW_SHOWMAXIMIZED 或普通分支的
     * SWP_FRAMECHANGED) — ShowImmediate 跳过重复的 SetWindowPos(FRAMECHANGED)
     * (~10ms DWM 事务)。未走 prepare 的独立调用方仍执行, 行为不变。 */
    bool framePrepared_ = false;
    void SetIconFromPixels(const uint8_t* rgba, int w, int h);
    void PrepareRT();      /* 预创建渲染目标 */
    void Hide();
    void Invalidate();
    void SetRoot(WidgetPtr root);
    void SetTitle(const std::wstring& title);
    HWND Handle() const { return hwnd_; }
    Renderer& GetRenderer() { return renderer_; }

    // Window ID in the context registry
    uint64_t windowId = 0;

    // Callbacks
    std::function<void()> onClose;
    std::function<void(int, int)> onResize;
    std::function<void(const std::wstring&)> onDrop;
    std::function<void(int)> onKey;
    std::function<void(float, float)> onRightClick;
    std::function<void(const MenuClickInfo*)> onMenuItemClick;  // 点击项 id+属性载荷

    // Focus management
    Widget* focusedWidget_ = nullptr;
    bool showFocusRing_ = false;   // only show focus ring when navigating via keyboard
    bool tabNavigationEnabled_ = false; // must be explicitly enabled (e.g. from .ui file)
    void FocusNext(bool reverse = false);
    void SetFocus(Widget* w);
    void ClearFocus();

    // Shortcut system
    struct Shortcut { int modifiers; int vk; std::function<void()> callback; };
    std::vector<Shortcut> shortcuts_;
    void RegisterShortcut(int modifiers, int vk, std::function<void()> cb);

    // Context menu
    void ShowMenu(ContextMenuPtr menu, float x, float y);
    void CloseMenu();

    // Toast notification
    // position: 0=top, 1=center, 2=bottom
    // icon: 0=none, 1=success(green✓), 2=error(red✕), 3=warning(yellow⚠)
    // anim: 0=slide(默认, 旧行为), 1=fade(纯渐入渐出, y 不变)
    void ShowToast(const std::wstring& text, int durationMs = 2000, int position = 0, int icon = 0, int anim = 0);
    /* Build 165+ (L172 follow-up): toast 改成独立 DirectComposition 透明叠加窗
     * (照 ContextMenu 弹窗). 淡入淡出只重渲这个小窗, 不碰主窗 D2D RT → 大图下
     * 丝滑. PaintToast 把绘制逻辑画到 toast 窗 (0,0) 原点; ToastWndProc 在 toast
     * 窗自己的 WM_TIMER 上推进 phase. 旧的主窗 DrawToast / 主窗 WM_TIMER 路径已删. */

    static bool RegisterWindowClass();

    // Dialog (public for C API access)
    void LayoutRoot();
    WidgetPtr Root() const { return root_; }
    bool skipOpenAnimation_ = false;
    /* Build 105+ (L25): config.start_maximized 状态. ui_window_create 拷,
     * Show / ShowImmediate 消费一次 (走 SW_SHOWMAXIMIZED 路径) 后清零, 不
     * 残留影响后续 ShowWindow(SW_HIDE)+Show() 循环. */
    bool startMaximizedPending_ = false;

    // Public so Context::UpdateAnimTimers can fire it after a binding
    // application flips a widget into animating_=true outside an event handler.
    void UpdateToggleAnimTimer();

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void OnPaint();
    void OnResize(UINT width, UINT height);
    void OnDpiChanged(UINT dpi, const RECT* suggested);
    void OnMouseMove(float x, float y);
    void OnMouseDown(float x, float y);
    void OnMouseUp(float x, float y);
    /* 鼠标 capture 被外部夺走 (WM_CAPTURECHANGED, 典型 DoDragDrop 起拖) 时调:
     * 复位 press 中的 widget (清 pressedWidget_ + OnMouseUp 取消其 drag 状态),
     * 但不 fire onClick. 否则 widget 收不到 WM_LBUTTONUP 卡在 drag 态, 后续
     * hover 仍被路由 OnMouseMove → 误拖/误 pan. */
    void CancelMouseCapture();
    void OnMouseDoubleClick(float x, float y);
    void OnMouseWheel(float x, float y, int delta);
    void OnDropFiles(HDROP hDrop);
    LRESULT OnNcCalcSize(WPARAM wParam, LPARAM lParam);
    LRESULT OnNcHitTest(int x, int y);
    void OnGetMinMaxInfo(MINMAXINFO* mmi);
    bool HasFocusedTextInput() const;
    void UpdateCaretBlinkTimer();
    void StartWindowOpenAnimation();
    void StartWindowCloseAnimation();

    HWND        hwnd_ = nullptr;
    HICON       hIcon_ = nullptr;
    bool        borderless_ = false;
    bool        resizable_ = true;
    bool        toolWindow_ = false;
    bool        maximized_ = false;
    int         configWidth_ = 0, configHeight_ = 0;
    /* 用户覆盖的最小窗口尺寸（0 = 用 theme::kMin* 默认值）。
     * 无边框画布模式下可以小于默认。 */
    int         minWOverride_ = 0, minHOverride_ = 0;
    /* 背景模式：0=主题色填充（默认），1=透明/不擦背景（widget 自己全覆盖画）。
     * 无边框画布模式下设 1，避免 SetWindowPos 扩窗口时的背景闪烁。 */
    int         bgMode_ = 0;
    /* L57: aspect ratio lock — 用户拖窗 resize 时按这个比例锁另一边. 0/0
     * = disable (默认). 看图器 borderless 模式 enter 时设 = (image_w, image_h),
     * exit 时清零. WM_SIZING 收到 user 拖出的 RECT 后, 按 ratio 算合法 size
     * 写回, Win32 把这个 RECT 当 user 实际拖的 size. */
    int         aspectLockW_ = 0, aspectLockH_ = 0;
    UINT        dpi_ = 96;
    float       dpiScale_ = 1.0f;
    std::wstring title_;

    Renderer    renderer_;
    WidgetPtr   root_;
    Widget*     hoveredWidget_ = nullptr;
    Widget*     pressedWidget_ = nullptr;
    ContextMenuPtr activeMenu_;

    // Toast
    std::wstring toastText_;
    float toastAlpha_ = 0.0f;
    float toastSlide_ = 0.0f;  // 0=隐藏位置, 1=完全显示
    int   toastPos_ = 0;       // 0=top, 1=center, 2=bottom
    int   toastPhase_ = 0;     // 0=idle, 1=slideIn, 2=hold, 3=slideOut
    int   toastIcon_ = 0;     // 0=none, 1=success, 2=error, 3=warning
    int   toastAnim_ = 0;     // 0=slide(旧行为), 1=fade(纯透明度过渡, y 不变)
    UINT_PTR toastTimerId_ = 0;
    bool  toastFading_ = false;
    int   holdDurationMs_ = 2000;
    int   holdElapsed_ = 0;        /* deprecated, kept for ABI 兼容; 新代码用 toastShownTick_ */
    /* Build 68+ (L18): toast 动画起点, time-based 推进, 防 WM_TIMER 合并 tick 时
     * 进度卡顿. ShowToast 时记一次 GetTickCount64, tick handler 用 now-shown 算
     * phase + slide. */
    uint64_t toastShownTick_ = 0;
    /* Build 165+ (L172 follow-up): toast 独立叠加窗. */
    HWND     toastHwnd_ = nullptr;        // 透明 DComp 叠加窗 (owner = hwnd_)
    Renderer toastRenderer_;              // 该窗专属 composition-mode RT
    bool     toastTimePeriodSet_ = false; // timeBeginPeriod(1) 已生效? (配对 timeEndPeriod)
    /* 这次 toast 的几何 (DIP) + 屏幕落位 (物理像素), ShowToast 算一次缓存,
     * PaintToast / ToastWndProc 复用 (避免 settle 期反复重算). */
    float    toastBoxW_ = 0.0f, toastBoxH_ = 0.0f;   // 框尺寸 (DIP)
    int      toastScreenX_ = 0;                       // 窗口左上 X (物理像素, 全程不变)
    int      toastScreenTargetY_ = 0;                 // 目标 Y (物理像素, FADE 钉死, SLIDE 终点)
    int      toastSlideRangePx_ = 0;                  // SLIDE: hideOffset 像素幅度 (带符号)
    void     PaintToast();                            // 画到 toast 窗 (0,0) 原点
    void     DestroyToast();                          // 销毁叠加窗 + KillTimer + timeEndPeriod 配对
    static bool s_toastClassRegistered_;
    static LRESULT CALLBACK ToastWndProc(HWND, UINT, WPARAM, LPARAM);

    bool        startupRevealPending_ = false;
    bool        startupRevealPosted_ = false;
    bool        caretBlinkTimerRunning_ = false;
    bool        toggleAnimTimerRunning_ = false;

    // Tooltip
    Widget*     tooltipWidget_ = nullptr;
    DWORD       hoverStartTick_ = 0;
    float       tooltipX_ = 0, tooltipY_ = 0;
    float       mouseX_ = 0, mouseY_ = 0;
    bool        tooltipVisible_ = false;
    UINT_PTR    tooltipTimerId_ = 0;
    static constexpr DWORD kTooltipDelayMs = 500;
    static constexpr UINT_PTR kTooltipTimerId = 9999;

    bool        windowAnimating_ = false;
    bool        windowClosing_ = false;
    bool        isMoving_ = false;   // 窗口正在移动/调整大小
    bool        isResizing_ = false; // 本次 sizemove 是 resize（非纯移动）

    float       windowAnimProgress_ = 0.0f;
    LARGE_INTEGER windowAnimStartTick_ = {};   // 动画起始时间（高精度）
    float       windowTargetWidth_ = 0.0f;
    float       windowTargetHeight_ = 0.0f;
    int         windowTargetX_ = 0;
    int         windowTargetY_ = 0;
    int         windowCloseStartX_ = 0;
    int         windowCloseStartY_ = 0;
    static constexpr float kWindowOpenAnimDurationMs = 180.0f;
    static constexpr float kWindowCloseAnimDurationMs = 180.0f;

    static bool classRegistered_;

public:
    /* Debug highlight */
    std::string debugHighlightId_;
    void SetDebugHighlight(const char* widgetId);
    int  Screenshot(const wchar_t* outPath);
    /* Screenshot a sub-region (in DIP). region.right/bottom are exclusive.
     * Empty / invalid region → falls back to full window. */
    int  ScreenshotRegion(D2D1_RECT_F region, const wchar_t* outPath);

    /* ---- Debug event simulation (DIP coordinates) ---- */
    /* 这些方法走和真实 Win32 消息一样的路径（命中测试、焦点、下拉、Flyout…）， */
    /* 用于自动化测试和 pipe 命令；内部会把 DIP 乘以 dpiScale_ 后调用私有 handler。 */
    void SimMouseMove(float dipX, float dipY);
    void SimMouseDown(float dipX, float dipY);
    void SimMouseUp(float dipX, float dipY);
    void SimMouseWheel(float dipX, float dipY, float delta);
    void SimRightClick(float dipX, float dipY);
    void SimKeyDown(int vk);
    void SimKeyChar(wchar_t ch);

    /* 共用的键盘分发（Tab / 快捷键 / Enter/Space 激活 / 方向键 Slider/Radio / Esc 关 ComboBox）
       返回 true 表示事件被消费。WM_KEYDOWN 和 SimKeyDown 都走这里。 */
    bool DispatchKeyDown(int vk);

    /* 在 UI 线程上同步执行 fn(ud)。跨线程调用时内部用 SendMessageW；
       已在 UI 线程时直接调用。返回前 fn 必已执行完成。 */
    void InvokeSync(void (*fn)(void* ud), void* ud);

    /* 无边框画布模式相关：覆盖最小尺寸 / 背景擦除行为。0 = 恢复默认。 */
    void SetMinSize(int wDip, int hDip) { minWOverride_ = wDip; minHOverride_ = hDip; }
    void SetBackgroundMode(int mode)    { bgMode_ = mode; }
    int  BackgroundMode() const         { return bgMode_; }
    /* L57: 锁定窗口 aspect 比例. 0/0 = disable. 看 aspectLockW_/H_ 注释. */
    void SetAspectLock(int ratioW, int ratioH) { aspectLockW_ = ratioW; aspectLockH_ = ratioH; }

    /* 窗口几何（DIP-native，内部乘 dpiScale_ 转物理像素）。
     * x/y 是屏幕物理坐标（Win32 惯例）；w/h 是 DIP（按当前 DPI 换算）。
     * 包含一次 SetWindowPos + InvalidateRect + UpdateWindow，确保 resize 后
     * 同帧内把 widget 内容画到新尺寸，减少扩大窗口时的背景闪烁。 */
    void SetWindowRect(int xScreen, int yScreen, int wDip, int hDip);
    void SetWindowSize(int wDip, int hDip);             /* 保持位置不变 */
    void SetWindowPosition(int xScreen, int yScreen);   /* 保持尺寸不变 */
    void GetWindowRectScreen(int* x, int* y, int* wDip, int* hDip) const;
    int  Dpi() const;                                   /* GetDpiForWindow 值, 96/120/144/... */
    /* 以 client(ax_dip, ay_dip) 点与屏幕(sx, sy) 对齐的方式 resize。 */
    void ResizeWithAnchor(int wDip, int hDip,
                          float anchorClientXDip, float anchorClientYDip,
                          int anchorScreenX, int anchorScreenY);
    /* 一键开/关无边框画布模式（见 ui_core.h 中的 ui_window_enable_canvas_mode）。 */
    void EnableCanvasMode(bool enable);

    /* 运行时切换窗口边框：
     *   frameless=true  → 无系统边框 / 标题栏（需 HTML / 代码里自己提供 TitleBar）
     *   frameless=false → 系统原生边框 + 标题栏 + 最小化 / 最大化 / 关闭
     * 内部用 SetWindowLongPtr 改 GWL_STYLE 再 SetWindowPos(SWP_FRAMECHANGED) 刷新。 */
    void SetFrameless(bool frameless);
    bool IsFrameless() const { return borderless_; }

    /* Active context menu popup (nullptr if none open) */
    ContextMenuPtr ActiveMenu() const { return activeMenu_; }

    /* Focused widget accessor for debug API */
    Widget* FocusedWidget() const { return focusedWidget_; }

    /* build 174: 编程式设焦点 + 亮焦点环 (键盘导航可见)。供 ui_window_focus_widget
     * C API (msgbox 初始焦点落在按钮上 + 方向键移焦点用)。w=null 清焦点。 */
    void FocusWidget(Widget* w);

    // Called from ui::Context::NotifyWidgetDestroyed when a widget dies
    // unexpectedly (v-for iter destroyed while cursor on it).
    void NotifyWidgetDestroyed(class Widget* w);

    /* DPI scale for coordinate conversion */
    float DpiScale() const { return dpiScale_; }
};

} // namespace ui
