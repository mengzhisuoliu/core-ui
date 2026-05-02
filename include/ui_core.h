/*
 * ui_core.h — Public C API for the Core UI framework
 *
 * Pure C header. No C++ types exposed.
 * Link against core-ui.dll (import library: core-ui.lib).
 */
#ifndef UI_CORE_H
#define UI_CORE_H

#include <stdint.h>
#include <wchar.h>

/* ------------------------------------------------------------------ */
/* Export / import                                                     */
/* ------------------------------------------------------------------ */
#if defined(UI_CORE_STATIC)
  #define UI_API
#elif defined(UI_CORE_BUILDING)
  #define UI_API __declspec(dllexport)
#else
  #define UI_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Version                                                            */
/* ------------------------------------------------------------------ */
/* 编译期版本宏（由 CMake 注入，若未注入则使用默认值）。                    */
#ifndef UI_CORE_VERSION_MAJOR
#define UI_CORE_VERSION_MAJOR 1
#endif
#ifndef UI_CORE_VERSION_MINOR
#define UI_CORE_VERSION_MINOR 0
#endif
#ifndef UI_CORE_VERSION_PATCH
#define UI_CORE_VERSION_PATCH 0
#endif
#ifndef UI_CORE_VERSION_BUILD
#define UI_CORE_VERSION_BUILD 1
#endif

#define UI_CORE_VERSION_STRINGIFY_(x) #x
#define UI_CORE_VERSION_STRINGIFY(x) UI_CORE_VERSION_STRINGIFY_(x)
#define UI_CORE_VERSION_STRING \
    UI_CORE_VERSION_STRINGIFY(UI_CORE_VERSION_MAJOR) "." \
    UI_CORE_VERSION_STRINGIFY(UI_CORE_VERSION_MINOR) "." \
    UI_CORE_VERSION_STRINGIFY(UI_CORE_VERSION_PATCH) "." \
    UI_CORE_VERSION_STRINGIFY(UI_CORE_VERSION_BUILD)

/* 运行时 API：无需 ui_init，任意时刻可调。                                  */
UI_API void        ui_core_version(int* major, int* minor, int* patch); /* 任一指针可为 NULL */
UI_API int         ui_core_version_build(void);                         /* 构建编号 */
UI_API const char* ui_core_version_string(void);                        /* "1.0.0.1" */

/* ------------------------------------------------------------------ */
/* Opaque handles                                                     */
/* ------------------------------------------------------------------ */
typedef uint64_t UiWidget;
typedef uint64_t UiWindow;

#define UI_INVALID 0

/* ------------------------------------------------------------------ */
/* Basic types                                                        */
/* ------------------------------------------------------------------ */
typedef struct UiColor {
    float r, g, b, a;
} UiColor;

typedef struct UiRect {
    float left, top, right, bottom;
} UiRect;

/* ------------------------------------------------------------------ */
/* Window configuration                                               */
/* ------------------------------------------------------------------ */
typedef struct UiWindowConfig {
    const wchar_t* title;
    int width;
    int height;
    int system_frame;   /* 0 (default) = borderless custom chrome, 1 = system title bar */
    int resizable;      /* 1 = WS_THICKFRAME */
    int accept_files;   /* 1 = WS_EX_ACCEPTFILES */
    int x;              /* 窗口初始 x 坐标，0 = 屏幕居中 */
    int y;              /* 窗口初始 y 坐标，0 = 屏幕居中 */
    int tool_window;    /* 1 = 工具窗口（不在任务栏显示图标） */
    int skip_animation; /* 1 = 跳过开场动画（文件关联打开时用，加速首次显示） */
    const void* icon_pixels; /* RGBA 像素数据（32bpp），NULL = 默认图标 */
    int icon_width;          /* 图标宽度 */
    int icon_height;         /* 图标高度 */
} UiWindowConfig;

/* ------------------------------------------------------------------ */
/* Theme                                                              */
/* ------------------------------------------------------------------ */
typedef enum UiThemeMode {
    UI_THEME_DARK  = 0,
    UI_THEME_LIGHT = 1
} UiThemeMode;

/* ------------------------------------------------------------------ */
/* Initialization / shutdown / message loop                           */
/* ------------------------------------------------------------------ */
UI_API int  ui_init(void);                        /* returns 0, default dark theme */
UI_API int  ui_init_with_theme(UiThemeMode mode); /* init with specified theme */
UI_API void ui_shutdown(void);
UI_API int  ui_run(void);              /* message loop, returns exit code */
UI_API void ui_quit(int exit_code);    /* posts WM_QUIT */

/* ------------------------------------------------------------------ */
/* Window management                                                  */
/* ------------------------------------------------------------------ */
UI_API UiWindow ui_window_create(const UiWindowConfig* config);
UI_API void     ui_window_destroy(UiWindow win);
UI_API void     ui_window_show(UiWindow win);
UI_API void     ui_window_show_immediate(UiWindow win);  /* 跳过开场动画 */
UI_API void     ui_window_prepare_rt(UiWindow win);     /* 预创建渲染目标（不显示窗口） */
UI_API void     ui_window_hide(UiWindow win);
UI_API void     ui_window_set_root(UiWindow win, UiWidget root);
UI_API void     ui_window_set_title(UiWindow win, const wchar_t* title);
UI_API void     ui_window_invalidate(UiWindow win);
/* 强制重新布局（set_visible 等改变影响布局的属性后调用） */
UI_API void     ui_window_relayout(UiWindow win);
UI_API void*    ui_window_hwnd(UiWindow win);   /* returns HWND */

/* ------------------------------------------------------------------ */
/* Window callbacks                                                   */
/* ------------------------------------------------------------------ */
typedef void (*UiWindowCloseCallback)(UiWindow win, void* userdata);
typedef void (*UiWindowResizeCallback)(UiWindow win, int w, int h, void* userdata);
typedef void (*UiWindowDropCallback)(UiWindow win, const wchar_t* path, void* userdata);
typedef void (*UiWindowKeyCallback)(UiWindow win, int vk_code, void* userdata);

/* ------------------------------------------------------------------ */
/* Context Menu                                                       */
/* ------------------------------------------------------------------ */
typedef uint64_t UiMenu;

UI_API UiMenu   ui_menu_create(void);
UI_API void     ui_menu_destroy(UiMenu menu);
UI_API void     ui_menu_add_item(UiMenu menu, int id, const wchar_t* text);
UI_API void     ui_menu_add_item_ex(UiMenu menu, int id, const wchar_t* text,
                                     const wchar_t* shortcut, const char* svg);
UI_API void     ui_menu_add_separator(UiMenu menu);
UI_API void     ui_menu_add_submenu(UiMenu menu, const wchar_t* text, UiMenu submenu);
UI_API void     ui_menu_set_enabled(UiMenu menu, int id, int enabled);
UI_API void     ui_menu_set_bg_color(UiMenu menu, UiColor color);
UI_API void     ui_menu_show(UiWindow win, UiMenu menu, float x, float y);
UI_API void     ui_menu_close(UiWindow win);

/* Toast notification (bottom-center, auto-fade) */
UI_API void     ui_toast(UiWindow win, const wchar_t* text, int duration_ms);
UI_API void     ui_toast_at(UiWindow win, const wchar_t* text, int duration_ms, int position); /* 0=top 1=center 2=bottom */
UI_API void     ui_toast_ex(UiWindow win, const wchar_t* text, int duration_ms, int position, int icon); /* icon: 0=none 1=success 2=error 3=warning */

/* ------------------------------------------------------------------ */
/* Dialog (modal confirm / alert)                                     */
/* ------------------------------------------------------------------ */
typedef void (*UiDialogCallback)(UiWidget dialog, int confirmed, void* userdata);

UI_API void ui_dialog_show(UiWidget dialog, UiWindow win,
                           const wchar_t* title, const wchar_t* message,
                           UiDialogCallback cb, void* userdata);
UI_API void ui_dialog_hide(UiWidget dialog, UiWindow win);
UI_API void ui_dialog_set_ok_text(UiWidget dialog, const wchar_t* text);
UI_API void ui_dialog_set_cancel_text(UiWidget dialog, const wchar_t* text);
UI_API void ui_dialog_set_show_cancel(UiWidget dialog, int show);
/* Per-dialog theme override. mode: 0 = follow global theme (default),
 * 1 = force light, 2 = force dark. */
UI_API void ui_dialog_set_theme_mode(UiWidget dialog, int mode);

typedef void (*UiMenuCallback)(UiWindow win, int item_id, void* userdata);
UI_API void ui_window_on_menu(UiWindow win, UiMenuCallback cb, void* userdata);

typedef void (*UiRightClickCallback)(UiWindow win, float x, float y, void* userdata);
UI_API void ui_window_on_right_click(UiWindow win, UiRightClickCallback cb, void* userdata);

/* ------------------------------------------------------------------ */
/* Window callbacks                                                   */
/* ------------------------------------------------------------------ */
UI_API void ui_window_on_close(UiWindow win, UiWindowCloseCallback cb, void* userdata);
UI_API void ui_window_on_resize(UiWindow win, UiWindowResizeCallback cb, void* userdata);
UI_API void ui_window_on_drop(UiWindow win, UiWindowDropCallback cb, void* userdata);
UI_API void ui_window_on_key(UiWindow win, UiWindowKeyCallback cb, void* userdata);

/* ------------------------------------------------------------------ */
/* Layout containers                                                  */
/* ------------------------------------------------------------------ */
UI_API UiWidget ui_vbox(void);
UI_API UiWidget ui_hbox(void);
UI_API UiWidget ui_spacer(float size);     /* 0 = expanding */
UI_API UiWidget ui_panel(UiColor bg);
UI_API UiWidget ui_panel_themed(int theme_color_id);  /* 0=sidebar_bg, 1=toolbar_bg, 2=content_bg */

/* ------------------------------------------------------------------ */
/* Controls                                                           */
/* ------------------------------------------------------------------ */
UI_API UiWidget ui_label(const wchar_t* text);
UI_API UiWidget ui_button(const wchar_t* text);
UI_API UiWidget ui_checkbox(const wchar_t* text);
UI_API UiWidget ui_slider(float min_val, float max_val, float value);
UI_API UiWidget ui_separator(void);
UI_API UiWidget ui_vseparator(void);
UI_API UiWidget ui_text_input(const wchar_t* placeholder);
UI_API UiWidget ui_text_area(const wchar_t* placeholder);
UI_API UiWidget ui_combobox(const wchar_t** items, int count);
UI_API UiWidget ui_radio_button(const wchar_t* text, const char* group);
UI_API UiWidget ui_toggle(const wchar_t* text);
UI_API UiWidget ui_progress_bar(float min_val, float max_val, float value);
UI_API UiWidget ui_tab_control(void);
UI_API UiWidget ui_scroll_view(void);
UI_API UiWidget ui_dialog(void);

/* ------------------------------------------------------------------ */
/* ImageView (zoomable, pannable image canvas)                       */
/* ------------------------------------------------------------------ */
UI_API UiWidget ui_image_view(void);
UI_API void     ui_image_load_file(UiWidget w, UiWindow win, const wchar_t* path);
UI_API void     ui_image_set_pixels(UiWidget w, UiWindow win,
                                     const void* pixels, int width, int height, int stride);
UI_API void     ui_image_clear(UiWidget w);
UI_API float    ui_image_get_zoom(UiWidget w);
UI_API void     ui_image_set_zoom(UiWidget w, float zoom);
UI_API void     ui_image_fit(UiWidget w);
UI_API void     ui_image_reset(UiWidget w);
UI_API void     ui_image_get_pan(UiWidget w, float* out_x, float* out_y);
UI_API void     ui_image_set_pan(UiWidget w, float x, float y);
UI_API int      ui_image_width(UiWidget w);
UI_API int      ui_image_height(UiWidget w);
/* 读回当前 bitmap 像素到 CPU（BGRA premultiplied, stride=w*4）。
 * 成功返回 1，*out_pixels 需用 ui_image_free_pixels 释放；失败返回 0（tile 模式/无图/回读失败）。*/
UI_API int      ui_image_get_pixels(UiWidget w, UiWindow win, void** out_pixels, int* out_w, int* out_h);
UI_API void     ui_image_free_pixels(void* pixels);
UI_API void     ui_image_set_checkerboard(UiWidget w, int on);
UI_API void     ui_image_set_antialias(UiWidget w, int on);
UI_API int      ui_image_get_antialias(UiWidget w);
UI_API void     ui_image_set_zoom_range(UiWidget w, float min_zoom, float max_zoom);

typedef void (*UiViewportCallback)(UiWidget widget, float zoom, float panX, float panY, void* userdata);
UI_API void     ui_image_on_viewport_changed(UiWidget w, UiViewportCallback cb, void* userdata);

/* ImageView mouse down/move 钩子：返回非 0 吞掉事件。
 * mouse_move 钩子吞事件时 core-ui 会顺带结束 pan 状态，
 * 用于 pan 中途切换到"拖出"等流程。 */
typedef int (*UiImageMouseDownCallback)(UiWidget widget, float x, float y, int btn, void* userdata);
typedef int (*UiImageMouseMoveCallback)(UiWidget widget, float x, float y, void* userdata);
UI_API void     ui_image_on_mouse_down(UiWidget w, UiImageMouseDownCallback cb, void* userdata);
UI_API void     ui_image_on_mouse_move(UiWidget w, UiImageMouseMoveCallback cb, void* userdata);

/* Rotation (0, 90, 180, 270 degrees) */
UI_API void     ui_image_set_rotation(UiWidget w, int angle);
UI_API int      ui_image_get_rotation(UiWidget w);

/* Loading spinner */
UI_API void     ui_image_set_loading(UiWidget w, int loading);
UI_API int      ui_image_get_loading(UiWidget w);

/* Tiled rendering (for very large images) */
UI_API void     ui_image_set_tiled(UiWidget w, UiWindow win, int full_width, int full_height, int tile_size);
UI_API void     ui_image_set_tile(UiWidget w, UiWindow win, int tile_x, int tile_y,
                                   const void* pixels, int width, int height, int stride);
UI_API void     ui_image_set_tile_preview(UiWidget w, UiWindow win,
                                           const void* pixels, int width, int height, int stride);
UI_API void     ui_image_clear_tiles(UiWidget w);
UI_API int      ui_image_is_tiled(UiWidget w);

/* Raw image dimensions (before rotation) */
UI_API int      ui_image_raw_width(UiWidget w);
UI_API int      ui_image_raw_height(UiWidget w);

/* ------------------------------------------------------------------ */
/* ImageViewPlus (万能图像查看：位图 / SVG 矢量 / GIF / 分块 / 未来扩展)
 *
 * 与 ui_image_view 的区别：
 *   - 对 SVG 使用 D2D1SvgDocument 原生矢量（Win10 1607+，自动降级到路径解析）
 *   - 动画统一 Tick 驱动，扩展新后端无需改 widget
 *   - 加载按扩展名自动分派，调用方统一接口
 */
/* ------------------------------------------------------------------ */
UI_API UiWidget ui_image_view_plus(void);
UI_API int      ui_image_view_plus_load(UiWidget w, UiWindow win, const wchar_t* path);
UI_API void     ui_image_view_plus_set_pixels(UiWidget w, UiWindow win,
                                               const void* pixels, int width, int height, int stride);
UI_API void     ui_image_view_plus_clear(UiWidget w);

UI_API float    ui_image_view_plus_get_zoom(UiWidget w);
UI_API void     ui_image_view_plus_set_zoom(UiWidget w, float zoom);
UI_API void     ui_image_view_plus_fit(UiWidget w);
UI_API void     ui_image_view_plus_reset(UiWidget w);
UI_API void     ui_image_view_plus_get_pan(UiWidget w, float* out_x, float* out_y);
UI_API void     ui_image_view_plus_set_pan(UiWidget w, float x, float y);
UI_API void     ui_image_view_plus_set_zoom_range(UiWidget w, float min_zoom, float max_zoom);

UI_API int      ui_image_view_plus_width(UiWidget w);        /* 考虑旋转 */
UI_API int      ui_image_view_plus_height(UiWidget w);
UI_API int      ui_image_view_plus_raw_width(UiWidget w);    /* 不含旋转 */
UI_API int      ui_image_view_plus_raw_height(UiWidget w);

UI_API void     ui_image_view_plus_set_rotation(UiWidget w, int angle);
UI_API int      ui_image_view_plus_get_rotation(UiWidget w);

UI_API void     ui_image_view_plus_set_checkerboard(UiWidget w, int on);
UI_API void     ui_image_view_plus_set_antialias(UiWidget w, int on);
UI_API int      ui_image_view_plus_get_antialias(UiWidget w);
/* 自由拖拽：on=1 任意尺寸都能拖到画布外；on=0（默认）小图强制居中，大图限边界内 */
UI_API void     ui_image_view_plus_set_free_pan(UiWidget w, int on);
UI_API int      ui_image_view_plus_get_free_pan(UiWidget w);

UI_API void     ui_image_view_plus_set_loading(UiWidget w, int loading);
UI_API int      ui_image_view_plus_get_loading(UiWidget w);

/* 动画 */
UI_API int      ui_image_view_plus_is_animated(UiWidget w);
UI_API int      ui_image_view_plus_frame_count(UiWidget w);
UI_API int      ui_image_view_plus_current_frame(UiWidget w);
UI_API void     ui_image_view_plus_start_animation(UiWidget w);
UI_API void     ui_image_view_plus_stop_animation(UiWidget w);

/* 源元信息 */
UI_API int      ui_image_view_plus_is_vector(UiWidget w);    /* SVG / 其它矢量后端 */
UI_API const char* ui_image_view_plus_source_type(UiWidget w);

/* Crop 模式 */
UI_API void     ui_image_view_plus_set_crop_mode(UiWidget w, int on);
UI_API int      ui_image_view_plus_is_crop_mode(UiWidget w);
UI_API void     ui_image_view_plus_set_crop_rect(UiWidget w, float x, float y, float width, float height);
UI_API void     ui_image_view_plus_get_crop_rect(UiWidget w, float* x, float* y, float* width, float* height);
UI_API void     ui_image_view_plus_set_crop_aspect(UiWidget w, float ratio);   /* 0=free */
UI_API void     ui_image_view_plus_reset_crop(UiWidget w);

typedef void (*UiCropCallback)(UiWidget widget, float x, float y, float w, float h, void* userdata);
UI_API void     ui_image_view_plus_on_crop_changed(UiWidget w, UiCropCallback cb, void* userdata);

typedef void (*UiLoadedCallback)(UiWidget widget, void* userdata);
typedef void (*UiLoadFailedCallback)(UiWidget widget, const wchar_t* path, void* userdata);
UI_API void     ui_image_view_plus_on_loaded(UiWidget w, UiLoadedCallback cb, void* userdata);
UI_API void     ui_image_view_plus_on_load_failed(UiWidget w, UiLoadFailedCallback cb, void* userdata);
UI_API void     ui_image_view_plus_on_viewport_changed(UiWidget w, UiViewportCallback cb, void* userdata);

/* 分块大图模式 */
UI_API void     ui_image_view_plus_begin_tiled(UiWidget w, UiWindow win,
                                                 int full_width, int full_height, int tile_size);
UI_API void     ui_image_view_plus_set_tile(UiWidget w, int tile_x, int tile_y,
                                             const void* pixels, int width, int height, int stride);
UI_API void     ui_image_view_plus_clear_tiles(UiWidget w);

/* ------------------------------------------------------------------ */
/* IconButton (SVG icon button)                                      */
/* ------------------------------------------------------------------ */
UI_API UiWidget ui_icon_button(const char* svg, int ghost);
UI_API void     ui_icon_button_set_svg(UiWidget w, const char* svg);
UI_API void     ui_icon_button_set_ghost(UiWidget w, int ghost);
UI_API void     ui_icon_button_set_icon_color(UiWidget w, UiColor color);
UI_API void     ui_icon_button_set_icon_padding(UiWidget w, float padding);

/* ------------------------------------------------------------------ */
/* TitleBar (borderless window title bar)                             */
/* ------------------------------------------------------------------ */
UI_API UiWidget ui_titlebar(const wchar_t* title);
UI_API void     ui_titlebar_set_title(UiWidget titlebar, const wchar_t* title);
UI_API void     ui_titlebar_show_buttons(UiWidget titlebar, int showMin, int showMax, int showClose);
/* show=0 完全不画图标，标题文字滑到最左；show=1 走"用户设的→EXE 嵌入资源
   ID=1→不画"三段查找。默认 show=1。 */
UI_API void     ui_titlebar_show_icon(UiWidget titlebar, int show);
/* 显式设标题栏图标，覆盖 EXE 嵌入图标的自动加载。pixels 为 RGBA8888，
   传 NULL 清掉，回到自动加载行为。 */
UI_API void     ui_titlebar_set_icon_pixels(UiWidget titlebar,
                                             const void* rgba, int width, int height);
UI_API void     ui_titlebar_set_bg_color(UiWidget titlebar, UiColor color);
UI_API void     ui_titlebar_add_widget(UiWidget titlebar, UiWidget custom_widget);
/* Title font weight. Default 400 (NORMAL). Typical values:
   400 NORMAL / 500 MEDIUM / 600 SEMI_BOLD / 700 BOLD. */
UI_API void     ui_titlebar_set_title_weight(UiWidget titlebar, int weight);

/* ------------------------------------------------------------------ */
/* Widget tree operations                                             */
/* ------------------------------------------------------------------ */
UI_API void     ui_widget_add_child(UiWidget parent, UiWidget child);
UI_API void     ui_widget_remove_child(UiWidget parent, UiWidget child);
UI_API void     ui_widget_destroy(UiWidget widget);
UI_API UiWidget ui_widget_find_by_id(UiWidget root, const char* id);

/* ------------------------------------------------------------------ */
/* Widget common properties                                           */
/* ------------------------------------------------------------------ */
UI_API void ui_widget_set_id(UiWidget w, const char* id);
UI_API void ui_widget_set_width(UiWidget w, float width);
UI_API void ui_widget_set_height(UiWidget w, float height);
UI_API void ui_widget_set_size(UiWidget w, float width, float height);
UI_API void ui_widget_set_expand(UiWidget w, int expand);
UI_API void ui_widget_set_padding(UiWidget w, float left, float top, float right, float bottom);
UI_API void ui_widget_set_padding_uniform(UiWidget w, float p);
UI_API void ui_widget_set_gap(UiWidget w, float gap);
UI_API void ui_widget_set_visible(UiWidget w, int visible);
UI_API void ui_widget_set_opacity(UiWidget w, float opacity);  /* 0.0 = 全透明 + 不响应 hit */
UI_API float ui_widget_get_opacity(UiWidget w);
UI_API void ui_widget_set_enabled(UiWidget w, int enabled);
UI_API void ui_widget_set_bg_color(UiWidget w, UiColor color);

UI_API int    ui_widget_get_visible(UiWidget w);
UI_API int    ui_widget_get_enabled(UiWidget w);
UI_API UiRect ui_widget_get_rect(UiWidget w);
UI_API void   ui_widget_set_rect(UiWidget w, UiRect rect);

/* ------------------------------------------------------------------ */
/* Label                                                              */
/* ------------------------------------------------------------------ */
UI_API void ui_label_set_text(UiWidget w, const wchar_t* text);
UI_API void ui_label_set_font_size(UiWidget w, float size);
UI_API void ui_label_set_bold(UiWidget w, int bold);
UI_API void ui_label_set_wrap(UiWidget w, int wrap);  /* 1 = 自动换行 */
UI_API void ui_label_set_max_lines(UiWidget w, int maxLines);  /* wrap 模式最大行数，0=不限 */
UI_API void ui_label_set_text_color(UiWidget w, UiColor color);
UI_API void ui_label_set_align(UiWidget w, int align);  /* 0=left, 1=right, 2=center */

/* ------------------------------------------------------------------ */
/* Button                                                             */
/* ------------------------------------------------------------------ */
UI_API void ui_button_set_font_size(UiWidget w, float size);
UI_API void ui_button_set_type(UiWidget w, int type);  /* 0=default, 1=primary(accent) */
UI_API void ui_button_set_text_color(UiWidget w, UiColor color);
UI_API void ui_button_set_bg_color(UiWidget w, UiColor color);  /* 自定义背景色，hover/press 自动加深 */

/* ------------------------------------------------------------------ */
/* CheckBox                                                           */
/* ------------------------------------------------------------------ */
UI_API int  ui_checkbox_get_checked(UiWidget w);
UI_API void ui_checkbox_set_checked(UiWidget w, int checked);

/* ------------------------------------------------------------------ */
/* Slider                                                             */
/* ------------------------------------------------------------------ */
UI_API float ui_slider_get_value(UiWidget w);
UI_API void  ui_slider_set_value(UiWidget w, float value);

/* ------------------------------------------------------------------ */
/* TextInput                                                          */
/* ------------------------------------------------------------------ */
/* Returned pointer is valid until the next call to this function on the same thread. */
UI_API const wchar_t* ui_text_input_get_text(UiWidget w);
UI_API void           ui_text_input_set_text(UiWidget w, const wchar_t* text);
UI_API void           ui_text_input_set_read_only(UiWidget w, int read_only);

/* ------------------------------------------------------------------ */
/* TextArea (multi-line text input)                                   */
/* ------------------------------------------------------------------ */
/* Returned pointer is valid until the next call to this function on the same thread. */
UI_API const wchar_t* ui_text_area_get_text(UiWidget w);
UI_API void           ui_text_area_set_text(UiWidget w, const wchar_t* text);
UI_API void           ui_text_area_set_read_only(UiWidget w, int read_only);

/* ------------------------------------------------------------------ */
/* ComboBox                                                           */
/* ------------------------------------------------------------------ */
UI_API int  ui_combobox_get_selected(UiWidget w);
UI_API void ui_combobox_set_selected(UiWidget w, int index);

/* ------------------------------------------------------------------ */
/* RadioButton                                                        */
/* ------------------------------------------------------------------ */
UI_API int ui_radio_get_selected(UiWidget w);
UI_API void ui_radio_set_selected(UiWidget w, int selected);

/* ------------------------------------------------------------------ */
/* Toggle                                                             */
/* ------------------------------------------------------------------ */
UI_API int  ui_toggle_get_on(UiWidget w);
UI_API void ui_toggle_set_on(UiWidget w, int on);
UI_API void ui_toggle_set_on_immediate(UiWidget w, int on);

/* ------------------------------------------------------------------ */
/* ProgressBar                                                        */
/* ------------------------------------------------------------------ */
UI_API float ui_progress_get_value(UiWidget w);
UI_API void  ui_progress_set_value(UiWidget w, float value);

/* ------------------------------------------------------------------ */
/* TabControl                                                         */
/* ------------------------------------------------------------------ */
UI_API void ui_tab_add(UiWidget tab_control, const wchar_t* title, UiWidget content);
UI_API int  ui_tab_get_active(UiWidget tab_control);
UI_API void ui_tab_set_active(UiWidget tab_control, int index);

/* ------------------------------------------------------------------ */
/* ScrollView                                                         */
/* ------------------------------------------------------------------ */
UI_API void ui_scroll_set_content(UiWidget scroll_view, UiWidget content);

/* ------------------------------------------------------------------ */
/* Widget callbacks                                                   */
/* ------------------------------------------------------------------ */
typedef void (*UiClickCallback)(UiWidget widget, void* userdata);
typedef void (*UiValueCallback)(UiWidget widget, int value, void* userdata);
typedef void (*UiFloatCallback)(UiWidget widget, float value, void* userdata);
typedef void (*UiSelectionCallback)(UiWidget widget, int index, void* userdata);

UI_API void ui_widget_set_tooltip(UiWidget w, const wchar_t* text);
UI_API void ui_widget_on_click(UiWidget w, UiClickCallback cb, void* userdata);
UI_API void ui_checkbox_on_changed(UiWidget w, UiValueCallback cb, void* userdata);
UI_API void ui_slider_on_changed(UiWidget w, UiFloatCallback cb, void* userdata);
UI_API void ui_toggle_on_changed(UiWidget w, UiValueCallback cb, void* userdata);
UI_API void ui_combobox_on_changed(UiWidget w, UiSelectionCallback cb, void* userdata);

UI_API void        ui_theme_set_mode(UiThemeMode mode);
UI_API UiThemeMode ui_theme_get_mode(void);

/* Get current theme colors (read-only, changes when theme switches) */
UI_API UiColor ui_theme_bg(void);             /* window background */
UI_API UiColor ui_theme_content_bg(void);     /* content area background */
UI_API UiColor ui_theme_sidebar_bg(void);     /* sidebar / panel bg */
UI_API UiColor ui_theme_toolbar_bg(void);     /* toolbar / panel bg */
UI_API UiColor ui_theme_accent(void);         /* accent color */
/* 设置全局品牌色 (覆盖默认 accent).
 * 调用后所有用 accent 的 widget (primary button / nav 选中态 / slider /
 * progress / focus 边等) 立即跟随. hover/press/selected 自动派生.
 * accentText (按钮上的文字色) 按背景亮度自动选黑白.
 * 跨 ui_theme_set_mode 切换深/浅色仍保留 (再调一次深浅切换后还是品牌色).
 * 传 alpha=0 取消覆盖, 回到当前 mode 默认 accent.
 * 调完会自动 InvalidateAllWindows 触发重绘. */
UI_API void    ui_theme_set_accent(UiColor color);
/* 字符串版本: 接受跟 CSS 同款写法 — "#RGB" / "#RRGGBB" / "#RRGGBBAA" /
 * "rgb(r,g,b)" / "rgba(r,g,b,a)" / 命名色 ("red", "transparent" 等).
 * 解析失败返回非 0 (调用方可以 fallback). 传 NULL / "" / "none" 视为
 * "取消覆盖" (等同 ui_theme_set_accent({0,0,0,0})).                    */
UI_API int     ui_theme_set_accent_hex(const char* color);
UI_API UiColor ui_theme_text(void);           /* button/normal text */
UI_API UiColor ui_theme_divider(void);        /* divider/border */

/* ------------------------------------------------------------------ */
/* CustomWidget (user-defined widget via C callbacks)                  */
/* ------------------------------------------------------------------ */
typedef void* UiDrawCtx;

typedef void (*UiCustomDrawCallback)(UiWidget w, UiDrawCtx ctx, UiRect rect, void* ud);
typedef int  (*UiCustomMouseCallback)(UiWidget w, float x, float y, int btn, void* ud);
typedef int  (*UiCustomWheelCallback)(UiWidget w, float x, float y, float delta, void* ud);
typedef int  (*UiCustomKeyCallback)(UiWidget w, int vk, void* ud);
typedef int  (*UiCustomCharCallback)(UiWidget w, int ch, void* ud);
typedef void (*UiCustomLayoutCallback)(UiWidget w, UiRect rect, void* ud);

UI_API UiWidget ui_custom(void);
UI_API void ui_custom_on_draw(UiWidget w, UiCustomDrawCallback cb, void* ud);
UI_API void ui_custom_on_mouse_down(UiWidget w, UiCustomMouseCallback cb, void* ud);
UI_API void ui_custom_on_mouse_move(UiWidget w, UiCustomMouseCallback cb, void* ud);
UI_API void ui_custom_on_mouse_up(UiWidget w, UiCustomMouseCallback cb, void* ud);
UI_API void ui_custom_on_mouse_wheel(UiWidget w, UiCustomWheelCallback cb, void* ud);
UI_API void ui_custom_on_key_down(UiWidget w, UiCustomKeyCallback cb, void* ud);
UI_API void ui_custom_on_char(UiWidget w, UiCustomCharCallback cb, void* ud);
UI_API void ui_custom_on_layout(UiWidget w, UiCustomLayoutCallback cb, void* ud);
UI_API void ui_custom_set_focused(UiWidget w, int focused);
UI_API int  ui_custom_get_focused(UiWidget w);

/* ------------------------------------------------------------------ */
/* Drawing API (use inside UiCustomDrawCallback)                      */
/* ------------------------------------------------------------------ */
UI_API void  ui_draw_fill_rect(UiDrawCtx ctx, UiRect rect, UiColor color);
UI_API void  ui_draw_rect(UiDrawCtx ctx, UiRect rect, UiColor color, float width);
UI_API void  ui_draw_fill_rounded_rect(UiDrawCtx ctx, UiRect rect, float rx, float ry, UiColor color);
UI_API void  ui_draw_rounded_rect(UiDrawCtx ctx, UiRect rect, float rx, float ry, UiColor color, float width);
UI_API void  ui_draw_line(UiDrawCtx ctx, float x1, float y1, float x2, float y2, UiColor color, float width);
UI_API void  ui_draw_text(UiDrawCtx ctx, const wchar_t* text, UiRect rect, UiColor color, float fontSize);
UI_API void  ui_draw_text_ex(UiDrawCtx ctx, const wchar_t* text, UiRect rect, UiColor color,
                              float fontSize, int align, int bold);
UI_API float ui_draw_measure_text(UiDrawCtx ctx, const wchar_t* text, float fontSize);
UI_API void  ui_draw_bitmap(UiDrawCtx ctx, const uint8_t* pixels,
                             int width, int height, int stride, UiRect dest);
UI_API void  ui_draw_push_clip(UiDrawCtx ctx, UiRect rect);
UI_API void  ui_draw_pop_clip(UiDrawCtx ctx);

/* ------------------------------------------------------------------ */
/* Debug / Inspector                                                  */
/* ------------------------------------------------------------------ */
/* Returns a JSON string describing the widget tree of the window.    */
/* Caller must free the returned pointer with ui_debug_free().        */
UI_API char*    ui_debug_dump_tree(UiWindow win);
UI_API char*    ui_debug_dump_widget(UiWidget w);
UI_API void     ui_debug_free(char* ptr);
UI_API void     ui_debug_highlight(UiWindow win, const char* widget_id);  /* 红框高亮指定控件，NULL 清除 */
UI_API int      ui_debug_screenshot(UiWindow win, const wchar_t* outPath);  /* 截图保存为 PNG，成功返回 0 */
/* 截图当前打开的 popup menu (右键菜单 / 下拉菜单 / submenu 链最深的那个).
 * 没菜单打开返回 -1. 用来验证 menu icon / 颜色 / 子菜单实际渲染. */
UI_API int      ui_debug_screenshot_menu(UiWindow win, const wchar_t* outPath);
/* 截单个 widget。widget 必须已布局（rect 非空），输出 PNG。 */
UI_API int      ui_debug_screenshot_widget(UiWindow win, UiWidget w, const wchar_t* outPath);

/* ------------------------------------------------------------------ */
/* Debug server — named-pipe IPC（库级，所有应用均可一行启用）          */
/* ------------------------------------------------------------------ */
/* 启动后 \\.\pipe\<pipe_name> 接收文本命令，返回 JSON 响应。命令集合见 */
/* docs/debug-simulation.md。pipe_name=NULL 用默认 "ui_core_debug"。   */
/* 同进程仅一个 server；连续 _start 会返回 -2。返回 0 = OK。            */
UI_API int   ui_debug_server_start(UiWindow win, const char* pipe_name);
UI_API void  ui_debug_server_stop(void);
/* 自定义命令处理器，可覆盖 builtin 或新增私有命令。                     */
/* 返回值：>0 = 写入 out_buf 的字节数（作为 JSON 响应），0 = 不处理（回退 */
/* 到 builtin），<0 = 错误（也回退）。out_buf 容量由 out_cap 给出。     */
typedef int (*UiDebugCommandHandler)(const char* cmd, const char* args,
                                     char* out_buf, int out_cap, void* userdata);
UI_API void  ui_debug_server_set_handler(UiDebugCommandHandler cb, void* userdata);

/* ------------------------------------------------------------------ */
/* Debug / Simulation — 自动化测试用的事件注入 API                     */
/* ------------------------------------------------------------------ */
/*  两类通道：                                                        */
/*    1) ui_debug_*   —— 直接走 widget 事件路径（同步、触发回调）      */
/*    2) ui_debug_post_* —— 通过 Win32 PostMessage 走真实消息循环      */
/*       （需调用 ui_debug_pump 或等待 ui_run 处理）                   */
/*  所有函数返回 0 成功，非 0 失败；坐标参数为 DIP（逻辑像素）。        */

/* ---- Widget 查询 ---- */
UI_API int   ui_debug_widget_center(UiWidget w, float* outX, float* outY);
UI_API int   ui_debug_widget_is_visible(UiWidget w);

/* ---- 鼠标：走内部事件通路（触发命中测试、焦点、回调…） ---- */
UI_API int   ui_debug_click(UiWindow win, UiWidget w);
UI_API int   ui_debug_click_at(UiWindow win, float x, float y);
UI_API int   ui_debug_double_click(UiWindow win, UiWidget w);
UI_API int   ui_debug_right_click(UiWindow win, UiWidget w);
UI_API int   ui_debug_right_click_at(UiWindow win, float x, float y);
UI_API int   ui_debug_hover(UiWindow win, UiWidget w);
UI_API int   ui_debug_mouse_move(UiWindow win, float x, float y);
UI_API int   ui_debug_mouse_down(UiWindow win, float x, float y);
UI_API int   ui_debug_mouse_up(UiWindow win, float x, float y);
UI_API int   ui_debug_drag(UiWindow win, UiWidget w, float dx, float dy);
UI_API int   ui_debug_drag_to(UiWindow win, float x1, float y1, float x2, float y2);
UI_API int   ui_debug_wheel(UiWindow win, UiWidget w, float delta);
UI_API int   ui_debug_wheel_at(UiWindow win, float x, float y, float delta);

/* ---- 焦点 / 键盘 ---- */
UI_API int   ui_debug_focus(UiWindow win, UiWidget w);
UI_API int   ui_debug_blur(UiWindow win);
UI_API int   ui_debug_key(UiWindow win, int vk);          /* 发给焦点控件 */
UI_API int   ui_debug_type_char(UiWindow win, unsigned int ch);
UI_API int   ui_debug_type_text(UiWindow win, const wchar_t* text);

/* ---- 控件高层操作（直接改状态 + 触发 on_changed 回调） ---- */
UI_API int   ui_debug_checkbox_toggle(UiWindow win, UiWidget w);
UI_API int   ui_debug_checkbox_set(UiWindow win, UiWidget w, int checked);
UI_API int   ui_debug_toggle_set(UiWindow win, UiWidget w, int on);
UI_API int   ui_debug_radio_select(UiWindow win, UiWidget w);
UI_API int   ui_debug_combo_select(UiWindow win, UiWidget w, int index);
UI_API int   ui_debug_combo_open(UiWidget w);
UI_API int   ui_debug_combo_close(UiWidget w);
UI_API int   ui_debug_slider_set(UiWindow win, UiWidget w, float value);
UI_API int   ui_debug_number_set(UiWindow win, UiWidget w, float value);
UI_API int   ui_debug_tab_set(UiWidget w, int index);
UI_API int   ui_debug_expander_set(UiWidget w, int expanded);
UI_API int   ui_debug_splitview_set(UiWidget w, int open);
UI_API int   ui_debug_flyout_show(UiWidget flyout, UiWidget anchor);
UI_API int   ui_debug_flyout_hide(UiWidget flyout);
UI_API int   ui_debug_text_set(UiWidget w, const wchar_t* text);
UI_API int   ui_debug_scroll_set(UiWidget scrollview, float y);

/* ---- Context menu（对当前已打开的 active menu 操作） ---- */
UI_API int   ui_debug_menu_is_open(UiWindow win);
UI_API int   ui_debug_menu_item_count(UiWindow win);
UI_API int   ui_debug_menu_click_index(UiWindow win, int index);
UI_API int   ui_debug_menu_click_id(UiWindow win, int item_id);
UI_API int   ui_debug_menu_close(UiWindow win);

/* 子菜单：path 是整数索引数组，如 {2,1} 即"顶层第 2 项的 submenu 中第 1 项"。
   对只操作顶层的情形，等同 ui_debug_menu_click_index（depth=1）。 */
UI_API int   ui_debug_menu_item_count_at(UiWindow win, const int* path, int depth);
UI_API int   ui_debug_menu_item_id_at(UiWindow win, const int* path, int depth);
UI_API int   ui_debug_menu_has_submenu_at(UiWindow win, const int* path, int depth);
UI_API int   ui_debug_menu_click_path(UiWindow win, const int* path, int depth);

/* 开关 context menu 的"前台变化即自动关闭"行为。
   自动化脚本（如 PowerShell 发 pipe 命令）持有前台窗口时，需要调用
   ui_debug_set_menu_autoclose(0) 关掉自动关闭；否则菜单打开后 50ms 内就被关掉。
   参数 enabled=0 表示关闭自动关闭，=1 恢复正常。 */
UI_API void  ui_debug_set_menu_autoclose(int enabled);

/* 在 UI 线程上同步执行 fn(ud)。跨线程调用时内部用 SendMessage 到窗口，
   已在 UI 线程时直接调用。返回前 fn 必已执行完成。主要给自动化脚本 / 调试 pipe
   用，保证 widget 访问不跨线程产生数据竞争。 */
UI_API void  ui_window_invoke_sync(UiWindow win, void (*fn)(void* ud), void* ud);

/* ------------------------------------------------------------------ */
/* Frameless canvas mode —— 无边框画布 (since 1.2.0)                  */
/* ------------------------------------------------------------------ */
/* 覆盖窗口最小尺寸（DIP）。传 0 恢复默认（theme::kMinWidth/kMinHeight，480×360）。
   用于无边框画布等场景：窗口随 widget 尺寸动态缩放，需要小于主题默认最小尺寸。 */
UI_API void  ui_window_set_min_size(UiWindow win, int w_dip, int h_dip);

/* 运行时切换窗口边框：
 *   frameless != 0 → 无系统边框 / 标题栏（需自己放 TitleBar）
 *   frameless == 0 → 系统原生标题栏 + 最小化 / 最大化 / 关闭按钮
 * 内部用 SetWindowLongPtr + SWP_FRAMECHANGED 热更新窗口样式。 */
UI_API void  ui_window_set_frameless(UiWindow win, int frameless);
UI_API int   ui_window_is_frameless(UiWindow win);

/* 窗口背景擦除模式：
     0 = 主题背景色填充（默认，普通窗口）
     1 = 透明 / 不擦背景（画布模式：widget 自己画满客户区，
         避免 SetWindowPos 扩大窗口时的背景色闪烁）
   mode=1 要求 widget 覆盖整个客户区，否则会看到前一帧残留像素。 */
UI_API void  ui_window_set_background_mode(UiWindow win, int mode);

/* 标记 widget 为"点击即拖动窗口"：命中该 widget 时 WM_NCHITTEST 返回 HTCAPTION，
   交给系统做窗口拖动。HitTest 返回的是最深层子节点，所以 Panel 里面的 Button
   等控件不会被这个标记波及（除非它们自己也标了 dragWindow）。
   典型用法：无边框画布模式给根 Panel 打上这个标，让整个画布都能拖窗口。 */
UI_API void  ui_widget_set_drag_window(UiWidget w, int enable);

/* ---- 窗口几何（DIP-native） ---- */
/* x / y 是屏幕物理像素（Win32 惯例）；w_dip / h_dip 是 DIP（按当前 DPI 换算）。
   每次调用都会触发一次同步重绘（InvalidateRect + UpdateWindow），配合
   ui_window_set_background_mode(win, 1) 能把扩大窗口时的背景闪减到最小。 */
UI_API void  ui_window_set_rect(UiWindow win, int x_screen, int y_screen,
                                 int w_dip, int h_dip);
UI_API void  ui_window_set_size(UiWindow win, int w_dip, int h_dip);
UI_API void  ui_window_set_position(UiWindow win, int x_screen, int y_screen);
UI_API void  ui_window_get_rect_screen(UiWindow win,
                                        int* out_x, int* out_y,
                                        int* out_w_dip, int* out_h_dip);

/* 滚轮缩放"光标不动"原语：resize 到 (w_dip, h_dip)，
   并把新客户区里的 (client_x_dip, client_y_dip) 对齐到屏幕 (screen_x, screen_y)。
   典型调用：滚轮回调中已知鼠标在屏幕 (sx, sy)、在旧客户区(cx, cy)，
   缩放比变为 z'/z 后，新客户区中同一图像点位于 (cx * z'/z, cy * z'/z)，
   传这个点 + 屏幕鼠标坐标，窗口会自动移动使该点保持在鼠标下。
   注意：仅无边框窗口（system_frame=0）结果准确，有系统边框时 client 区
   与窗口左上角之间还隔着边框 + 标题栏，会偏移。 */
UI_API void  ui_window_resize_with_anchor(UiWindow win,
                                           int w_dip, int h_dip,
                                           float client_x_dip, float client_y_dip,
                                           int screen_x, int screen_y);

/* 一键进入 / 退出"无边框画布"模式。开启时等同于：
     - ui_window_set_min_size(win, 32, 32)
     - ui_window_set_background_mode(win, 1)
     - 根 widget.dragWindow = true
     - 如果根树里有 TitleBar，visible=false
   关闭时反之。要求窗口在 ui_window_create 时就用 system_frame=0 创建无边框。 */
UI_API void  ui_window_enable_canvas_mode(UiWindow win, int enable);

/* ------------------------------------------------------------------ */
/* Font / Text rendering (since 1.3.0)                                 */
/* ------------------------------------------------------------------ */
/* 文字渲染档位。见 docs/c-api.md 对比图。 */
typedef enum UiTextRenderMode {
    UI_TEXT_RENDER_SMOOTH     = 0, /* GRAYSCALE + NATURAL_SYM（默认 / WinUI 风） */
    UI_TEXT_RENDER_CLEARTYPE  = 1, /* CLEARTYPE + NATURAL（Office/Chrome） */
    UI_TEXT_RENDER_SHARP      = 2, /* CLEARTYPE + GDI_CLASSIC（记事本最锐） */
    UI_TEXT_RENDER_GRAY_SHARP = 3, /* GRAYSCALE + GDI_CLASSIC（锐且无彩边） */
    UI_TEXT_RENDER_ALIASED    = 4, /* 无抗锯齿 / 像素字体 */
} UiTextRenderMode;

/* ---- 全局默认（进程级，新建窗口的初值；旧窗口已创建也会跟随，只要它自己没 override）---- */
/* 设置全局默认字体族。NULL = 恢复 "Segoe UI"。
   常用值: L"Microsoft YaHei UI"（中文应用推荐）/ L"Segoe UI" / L"SimSun"。 */
UI_API void             ui_theme_set_default_font(const wchar_t* family);
UI_API const wchar_t*   ui_theme_get_default_font(void);

/* 设置"中英分离"字体：ASCII/拉丁字符用 latin，CJK 字符用 cjk。
   任一参数为 NULL 表示不覆盖该范围（由 default_font 或系统 fallback 处理）。
   两者皆 NULL = 关闭中英分离。
   典型: ui_theme_set_cjk_font(L"Segoe UI", L"Microsoft YaHei UI"). */
UI_API void             ui_theme_set_cjk_font(const wchar_t* latin, const wchar_t* cjk);
UI_API const wchar_t*   ui_theme_get_cjk_latin_font(void);
UI_API const wchar_t*   ui_theme_get_cjk_cjk_font(void);

/* 设置全局默认文字渲染模式。 */
UI_API void             ui_theme_set_text_render_mode(UiTextRenderMode mode);
UI_API UiTextRenderMode ui_theme_get_text_render_mode(void);

/* ---- 窗口级覆盖（优先级高于全局） ---- */
/* NULL / UI_TEXT_RENDER_SMOOTH 不能表达"清除 override"，用 ui_window_clear_font_override 重置。 */
UI_API void             ui_window_set_default_font(UiWindow win, const wchar_t* family);
UI_API void             ui_window_set_cjk_font(UiWindow win, const wchar_t* latin, const wchar_t* cjk);
UI_API void             ui_window_set_text_render_mode(UiWindow win, UiTextRenderMode mode);
UI_API void             ui_window_clear_font_override(UiWindow win);   /* 清除所有 font/mode 覆盖 */

/* ---- Dialog / Toast ---- */
UI_API int   ui_debug_dialog_confirm(UiWindow win);
UI_API int   ui_debug_dialog_cancel(UiWindow win);

/* ---- HWND 通道：通过 Win32 消息循环派发（异步，需 pump） ---- */
UI_API int   ui_debug_post_click(UiWindow win, float x, float y);
UI_API int   ui_debug_post_right_click(UiWindow win, float x, float y);
UI_API int   ui_debug_post_mouse_move(UiWindow win, float x, float y);
UI_API int   ui_debug_post_key(UiWindow win, int vk);
UI_API int   ui_debug_post_char(UiWindow win, unsigned int ch);

/* 处理所有已排队的窗口消息（使 Post* 生效）。返回已处理的消息数。 */
UI_API int   ui_debug_pump(void);

/* ------------------------------------------------------------------ */
/* Asset resolver — 资源按"名字"加载（HTML <img src> / <link href>）  */
/* ------------------------------------------------------------------ */
/* HTML 里 <img src="logo.png"> / <link href="theme.css"> 这类按名字
   引用的资源，库不假设任何路径或文件系统结构，统一通过这里解析。
   注册的来源按注册顺序串成 chain，先注册先匹配。

   两种典型工作流：
   - dev：注册一个目录，边改边看 → ui_asset_register_dir("assets/")
   - ship：CMake ui_core_embed_binary() 烤进 exe，启动时注册 →
           ui_asset_register_blob("logo.png", k_logo_bytes, k_logo_size)
   两种工作流不需要改一行 HTML。 */

/* 自定义解析回调：name → bytes。返回非 0 表示找到。bytes 必须保活到
   下一次 ui_asset_reset() 或进程结束（库不复制内容）。 */
typedef int (*UiAssetResolver)(const char* name,
                                const void** out_bytes,
                                size_t*       out_size,
                                void*         userdata);

UI_API void ui_asset_register_dir(const char* dir_utf8);
UI_API void ui_asset_register_blob(const char* name, const void* bytes, size_t size);
UI_API void ui_asset_register_resolver(UiAssetResolver fn, void* userdata);
UI_API void ui_asset_reset(void);

/* ------------------------------------------------------------------ */
/* Page API — HTML + Vue-like declarative pages                       */
/* ------------------------------------------------------------------ */
typedef uint64_t UiPage;

/* Load a .uix page (SFC: <window> + <template> + <script> + <style>) from
 * file or string. Returns 0 on failure. */
UI_API UiPage   ui_page_load_file(const wchar_t* path);
UI_API UiPage   ui_page_load_string(const char* html_source);
UI_API void     ui_page_destroy(UiPage p);

/* Reload a previously-loaded page from disk (preserves reactive variable values by name). */
UI_API int      ui_page_reload(UiPage p);

/* Root widget handle (usable with ui_window_set_content etc.). Returns 0 on failure. */
UI_API UiWidget ui_page_root(UiPage p);

/* Open a window sized and titled per the page's <window ... /> tag.
 * Any fields not specified in the tag fall back to caller's optional override,
 * then to sensible defaults. Also switches theme if <window theme="..."> was set.
 * The page's root widget is automatically installed as the window content.
 * Returns the new UiWindow handle, or 0 on failure. */
UI_API UiWindow ui_page_open_window(UiPage p, const UiWindowConfig* override_defaults);

/* Set reactive variables. The page's template re-evaluates any dependent bindings. */
UI_API void     ui_page_set_bool(UiPage p, const char* name, int value);
UI_API void     ui_page_set_int(UiPage p, const char* name, int value);
UI_API void     ui_page_set_float(UiPage p, const char* name, double value);
UI_API void     ui_page_set_text(UiPage p, const char* name, const wchar_t* value);

/* Set a list of strings (for v-for). items is an array of `count` wide strings. */
UI_API void     ui_page_set_text_list(UiPage p, const char* name, const wchar_t** items, int count);

/* JSON I/O — push 任意嵌套对象 / 数组到 reactive state, 或反向读出. 比并行
 * 字符串数组好用得多, 适合 v-for 一条记录多字段的场景:
 *
 *   const char* json = "[{\"id\":1,\"name\":\"a\",\"on\":true}, ...]";
 *   ui_page_set_json(p, "items", json);   // 0=ok, -1=parse error
 *
 *   <div v-for="x in items" :key="x.id">{{ x.name }} {{ x.on }}</div>
 *
 * ui_page_get_json 返回 malloc 出的 UTF-8 JSON, 调用方用 ui_page_free 释放.
 * 不存在或 kind=Function 返回 NULL.                                           */
UI_API int      ui_page_set_json(UiPage p, const char* name, const char* utf8_json);
UI_API char*    ui_page_get_json(UiPage p, const char* name);
UI_API void     ui_page_free    (void* ptr);

/* (Removed in 1.5.0) ui_page_set_handler / ui_page_set_handler_ex —
 * Vue 3 SFC `methods: { onDel(id, on) { ... } }` replaces these. C-side
 * native methods can register on the JS state via the QuickJS API
 * directly when needed. Existing call sites should migrate to either
 * Vue 3 methods{} or expose a free-standing C function via the
 * embedding application. */

/* ---- Internationalization ------------------------------------------ */
/* Install a translation table for `locale`. `keys_values` is a flat array
 * of alternating UTF-8 key and value strings, with `count` being the number
 * of PAIRS (so the array length is count*2).
 * Example: const char* kv[] = { "save", "Save", "cancel", "Cancel" };
 *          ui_page_load_translations(p, "en", kv, 2);                    */
UI_API void     ui_page_load_translations(UiPage p, const char* locale,
                                          const char** keys_values, int count);

/* Load translations from `.lang` file format (UTF-8, one `key=value` per
 * line, # and ; line comments, BOM tolerated).
 *   ui_page_load_language_string — feed content as string (good for embed)
 *   ui_page_load_language_file   — read from disk (returns 0 on open failure)
 *
 * Replace-not-merge: calling either of these (or ui_page_load_translations)
 * twice for the same locale **replaces** the prior table for that locale —
 * keys not in the new content drop. Build the full table per call.
 *
 * Templates: write `<label>@app.title</label>` (compile-time desugar to
 * `$t('app.title')`) or explicit `{{ $t('app.title') }}`. Both auto-update
 * on ui_page_set_locale(). Missing keys fall back to the key itself. */
UI_API void     ui_page_load_language_string(UiPage p, const char* locale,
                                              const char* utf8_content);
UI_API int      ui_page_load_language_file  (UiPage p, const char* locale,
                                              const wchar_t* path);

/* Switch the active locale. Any binding that calls $t() in the template
 * re-evaluates, so labels refresh in place.                              */
UI_API void     ui_page_set_locale(UiPage p, const char* locale);

/* Retrieve a UiMenu handle for a `<menu id="name">` declared in the HTML.
 * Returned menu is owned by the page; do NOT call ui_menu_destroy on it.
 * Use with ui_menu_show + ui_window_on_menu for click-routing.
 *   <menu id="ctx">
 *     <menuitem id="1">Save</menuitem>
 *     <separator/>
 *     <menuitem id="2" shortcut="Ctrl+Q">Quit</menuitem>
 *   </menu>                                                              */
UI_API UiMenu   ui_page_menu(UiPage p, const char* name);

/* Retrieve last error (compile + runtime). Returned pointer is valid until next call. */
UI_API const char* ui_page_last_error(UiPage p);

/* ---- Lifecycle hooks (Vue parity for v-if / v-for) -------------------- */
/* Fires every time a widget with the given HTML id is mounted: initial
 * render, v-if becoming truthy, v-for iteration build. The widget handle
 * passed to the callback is FRESH (a previous handle for the same id will
 * be invalid after unmount). Use this instead of registering callbacks via
 * a one-shot ui_widget_find_by_id when the target lives inside v-if/v-for.
 *
 *   void on_mount(UiPage p, UiWidget w, void* ud) {
 *       ui_widget_on_click(w, on_click_cb, ud);
 *   }
 *   ui_page_on_widget_mount(page, "btn_x", on_mount, NULL);
 *
 * Pass cb=NULL to clear. Multiple registrations on the same id replace the
 * previous one (only one mount + one unmount handler per id).            */
typedef void (*UiWidgetLifecycleCallback)(UiPage page, UiWidget w, void* userdata);
UI_API void ui_page_on_widget_mount(UiPage p, const char* widget_id,
                                     UiWidgetLifecycleCallback cb, void* userdata);
UI_API void ui_page_on_widget_unmount(UiPage p, const char* widget_id,
                                       UiWidgetLifecycleCallback cb, void* userdata);

/* Process-wide feature flag — switches subsequent ui_page_load_* calls to the
 * QuickJS runtime (Vue 3 SFC `export default { … }` syntax + Proxy reactive).
 * Pass v=0 to use the legacy AST + Property<Value> path (default).
 *
 * Also resolved from env var UI_PAGE_QUICKJS=1 if this function is not called
 * before the first ui_page_load_*.
 *
 * Phase 3a.4: only `{{ expr }}` text bindings are wired on the QuickJS path;
 * @event / v-if / v-for / v-model / methods / computed are ignored (will be
 * filled in Phases 3a.5 — 3a.9).                                         */
UI_API void ui_page_set_quickjs_enabled(int v);

/* ------------------------------------------------------------------ */
/* Layout API — `.ui` 标记文件加载器（XAML/QML 风格的旧声明式系统）   */
/* HTML 那套 (UiPage) 是 Vue 风格响应式；这套是更简单的"一次性构建    */
/* widget 树 + 显式数据绑定 + 命名 handler"。ui-demo.exe 用的是这个。 */
/* ------------------------------------------------------------------ */
typedef uint64_t UiLayout;

/* 创建空 layout / 销毁 */
UI_API UiLayout       ui_layout_create(void);
UI_API void           ui_layout_destroy(UiLayout layout);

/* 加载 .ui 内容（文件或字符串）。返回 1=成功 / 0=失败，失败原因看 last_error。 */
UI_API int            ui_layout_load_file  (UiLayout layout, const wchar_t* path);
UI_API int            ui_layout_load_string(UiLayout layout, const char* utf8_source);
UI_API const char*    ui_layout_last_error (UiLayout layout);

/* 加载后取根 widget / 按 id 查找。返回 0/UI_INVALID 表示没有。 */
UI_API UiWidget       ui_layout_root        (UiLayout layout);
UI_API UiWidget       ui_layout_find_by_id  (UiLayout layout, const char* id);

/* 一步式：按 .ui 里的 <ui width="..." height="..." title="..."> 提示创建窗口，
   挂根 widget，调 show。等同于 demo/app.cpp 那一坨手写 boilerplate。
   override_defaults 字段非 0/NULL 的会覆盖 .ui 提示。 */
UI_API UiWindow       ui_layout_open_window(UiLayout layout,
                                             const UiWindowConfig* override_defaults);

/* 事件 handler — .ui 里 onClick="onSomething" 等字面量名称在此绑实际函数。
   按签名分 5 组。user 是任意用户上下文指针，会原样回传给 fn。 */
UI_API void           ui_layout_set_handler_void  (UiLayout, const char* name,
                                                    void (*fn)(void* user), void* user);
UI_API void           ui_layout_set_handler_bool  (UiLayout, const char* name,
                                                    void (*fn)(int v, void* user), void* user);
UI_API void           ui_layout_set_handler_int   (UiLayout, const char* name,
                                                    void (*fn)(int v, void* user), void* user);
UI_API void           ui_layout_set_handler_float (UiLayout, const char* name,
                                                    void (*fn)(double v, void* user), void* user);
UI_API void           ui_layout_set_handler_text  (UiLayout, const char* name,
                                                    void (*fn)(const wchar_t* v, void* user), void* user);

/* 数据绑定 push（按 .ui 里 :prop="bindingName" 对应的名称设值）。 */
UI_API void           ui_layout_set_bool (UiLayout, const char* name, int    value);
UI_API void           ui_layout_set_int  (UiLayout, const char* name, int    value);
UI_API void           ui_layout_set_float(UiLayout, const char* name, double value);
UI_API void           ui_layout_set_text (UiLayout, const char* name, const wchar_t* value);

/* i18n. lang 文件 / 字符串是 key=value 格式（UTF-8，每行一条）。
   Apply 把 .ui 里所有 @key 替换为当前语言的字符串。 */
UI_API int            ui_layout_load_language_string(UiLayout, const char* utf8_content);
UI_API int            ui_layout_load_language_file  (UiLayout, const wchar_t* path);
UI_API void           ui_layout_apply_language      (UiLayout);

#ifdef __cplusplus
}
#endif

#endif /* UI_CORE_H */
