// Library-level named-pipe debug server.
//
// Lifts demo/app.cpp's dispatchCommand + pipe thread into core-ui so any
// application can opt in with one ui_debug_server_start() call. See
// docs/debug-simulation.md for the full command catalogue.

#include "ui_debug_server.h"

#include "ui_context.h"
#include "ui_window.h"
#include "controls.h"
#include "../../include/ui_core.h"

#include <atomic>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <windows.h>

namespace ui::debug {

namespace {

// ============================================================================
// JSON / parsing helpers (ported verbatim from demo/app.cpp)
// ============================================================================

std::string okJson() { return "{\"ok\":true}"; }

std::string okFmt(const char* fmt, ...) {
    char buf[512];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    return buf;
}

std::string errJson(const std::string& msg) {
    std::string out = "{\"error\":\"";
    for (char c : msg) {
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    out += "\"}";
    return out;
}

std::string trimLead(const std::string& s) {
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) i++;
    return s.substr(i);
}

std::vector<std::string> splitWs(const std::string& s, int maxParts = 8) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i < s.size() && (int)out.size() < maxParts) {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) i++;
        if (i >= s.size()) break;
        if ((int)out.size() == maxParts - 1) {
            out.push_back(s.substr(i));
            return out;
        }
        size_t j = i;
        while (j < s.size() && s[j] != ' ' && s[j] != '\t') j++;
        out.push_back(s.substr(i, j - i));
        i = j;
    }
    return out;
}

std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}

int parseVk(const std::string& s) {
    std::string lo = s;
    for (auto& c : lo) c = (char)tolower((unsigned char)c);
    if (lo == "enter" || lo == "return") return VK_RETURN;
    if (lo == "esc"   || lo == "escape") return VK_ESCAPE;
    if (lo == "tab")       return VK_TAB;
    if (lo == "space")     return VK_SPACE;
    if (lo == "back" || lo == "backspace") return VK_BACK;
    if (lo == "del"  || lo == "delete")    return VK_DELETE;
    if (lo == "left")  return VK_LEFT;
    if (lo == "right") return VK_RIGHT;
    if (lo == "up")    return VK_UP;
    if (lo == "down")  return VK_DOWN;
    if (lo == "home")  return VK_HOME;
    if (lo == "end")   return VK_END;
    return std::atoi(s.c_str());
}

int parseBoolArg(const std::string& s, int deflt) {
    if (s.empty()) return deflt;
    std::string lo = s;
    for (auto& c : lo) c = (char)tolower((unsigned char)c);
    if (lo == "1" || lo == "on"  || lo == "true"  || lo == "show") return 1;
    if (lo == "0" || lo == "off" || lo == "false" || lo == "hide") return 0;
    if (lo == "toggle") return -1;
    return deflt;
}

// Translate a string id to a UiWidget handle by walking the active window's
// root. Inserts the root into the handle table if it's not already there
// (needed because v-if / media-query rebuilds may swap the actual Widget*
// behind the same logical id).
UiWidget WidgetByIdHandle(UiWindow win, const std::string& id) {
    auto& ctx = ui::GetContext();
    auto* wi = ctx.GetWindow(win);
    if (!wi) return 0;
    auto root = wi->Root();
    if (!root) return 0;
    uint64_t rh = ctx.handles.FindHandle(root.get());
    if (!rh) rh = ctx.handles.Insert(root);
    return ui_widget_find_by_id(rh, id.c_str());
}

bool parseMenuPath(const std::string& s, std::vector<int>& out) {
    out.clear();
    if (s.empty()) return false;
    size_t i = 0;
    while (i < s.size()) {
        size_t j = s.find('/', i);
        std::string tok = s.substr(i, (j == std::string::npos) ? std::string::npos : j - i);
        if (tok.empty()) return false;
        out.push_back(std::atoi(tok.c_str()));
        if (j == std::string::npos) break;
        i = j + 1;
    }
    return !out.empty();
}

}  // anonymous namespace

// ============================================================================
// Built-in dispatcher (ported from demo/app.cpp:dispatchCommand)
// ============================================================================

std::string BuiltinDispatch(UiWindow win, const std::string& cmd,
                            const std::string& rest) {
    auto args = splitWs(rest, 6);
    auto arg = [&](int i) -> std::string {
        return (i < (int)args.size()) ? args[i] : std::string();
    };

    // --- inspector ---
    if (cmd == "tree") {
        char* json = ui_debug_dump_tree(win);
        if (!json) return "{}";
        std::string r = json; ui_debug_free(json);
        return r;
    }
    if (cmd == "widget") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("not found: " + arg(0));
        char* json = ui_debug_dump_widget(w);
        if (!json) return "{}";
        std::string r = json; ui_debug_free(json);
        return r;
    }
    if (cmd == "highlight") {
        ui_debug_highlight(win, arg(0).empty() ? nullptr : arg(0).c_str());
        ui_window_invalidate(win);
        return okJson();
    }
    if (cmd == "screenshot") {
        if (arg(0).empty()) return errJson("usage: screenshot <path>");
        std::wstring wp = utf8ToWide(arg(0));
        int r = ui_debug_screenshot(win, wp.c_str());
        return r == 0 ? okJson() : errJson("screenshot failed");
    }
    if (cmd == "screenshot_menu") {
        if (arg(0).empty()) return errJson("usage: screenshot_menu <path>");
        std::wstring wp = utf8ToWide(arg(0));
        int r = ui_debug_screenshot_menu(win, wp.c_str());
        return r == 0 ? okJson() : errJson("screenshot_menu failed (no popup open?)");
    }
    if (cmd == "screenshot_widget") {
        if (arg(1).empty()) return errJson("usage: screenshot_widget <id> <path>");
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found: " + arg(0));
        std::wstring wp = utf8ToWide(arg(1));
        int r = ui_debug_screenshot_widget(win, w, wp.c_str());
        return r == 0 ? okJson() : errJson("screenshot_widget failed (widget not laid out?)");
    }
    if (cmd == "invalidate") {
        ui_window_invalidate(win);
        return okJson();
    }
    if (cmd == "pump") {
        int n = ui_debug_pump();
        return okFmt("{\"ok\":true,\"processed\":%d}", n);
    }

    // --- mouse / keyboard ---
    if (cmd == "click") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found: " + arg(0));
        return ui_debug_click(win, w) == 0 ? okJson() : errJson("click failed");
    }
    if (cmd == "click_at") {
        if (arg(1).empty()) return errJson("usage: click_at <x> <y>");
        float x = (float)std::atof(arg(0).c_str());
        float y = (float)std::atof(arg(1).c_str());
        return ui_debug_click_at(win, x, y) == 0 ? okJson() : errJson("click_at failed");
    }
    if (cmd == "dbl_click") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        return ui_debug_double_click(win, w) == 0 ? okJson() : errJson("dbl_click failed");
    }
    if (cmd == "rclick") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        return ui_debug_right_click(win, w) == 0 ? okJson() : errJson("rclick failed");
    }
    if (cmd == "rclick_at") {
        if (arg(1).empty()) return errJson("usage: rclick_at <x> <y>");
        float x = (float)std::atof(arg(0).c_str());
        float y = (float)std::atof(arg(1).c_str());
        int r = ui_debug_right_click_at(win, x, y);
        if (r != 0) return errJson("rclick_at failed");
        int open = ui_debug_menu_is_open(win);
        return okFmt("{\"ok\":true,\"menuOpen\":%s}", open ? "true" : "false");
    }
    if (cmd == "hover") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        return ui_debug_hover(win, w) == 0 ? okJson() : errJson("hover failed");
    }
    if (cmd == "move") {
        if (arg(1).empty()) return errJson("usage: move <x> <y>");
        float x = (float)std::atof(arg(0).c_str());
        float y = (float)std::atof(arg(1).c_str());
        return ui_debug_mouse_move(win, x, y) == 0 ? okJson() : errJson("move failed");
    }
    if (cmd == "drag") {
        if (arg(2).empty()) return errJson("usage: drag <id> <dx> <dy>");
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        float dx = (float)std::atof(arg(1).c_str());
        float dy = (float)std::atof(arg(2).c_str());
        return ui_debug_drag(win, w, dx, dy) == 0 ? okJson() : errJson("drag failed");
    }
    if (cmd == "drag_to") {
        if (arg(3).empty()) return errJson("usage: drag_to <x1> <y1> <x2> <y2>");
        float x1 = (float)std::atof(arg(0).c_str());
        float y1 = (float)std::atof(arg(1).c_str());
        float x2 = (float)std::atof(arg(2).c_str());
        float y2 = (float)std::atof(arg(3).c_str());
        return ui_debug_drag_to(win, x1, y1, x2, y2) == 0 ? okJson() : errJson("drag_to failed");
    }
    if (cmd == "wheel") {
        if (arg(1).empty()) return errJson("usage: wheel <id> <delta>");
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        float d = (float)std::atof(arg(1).c_str());
        return ui_debug_wheel(win, w, d) == 0 ? okJson() : errJson("wheel failed");
    }
    if (cmd == "wheel_at") {
        if (arg(2).empty()) return errJson("usage: wheel_at <x> <y> <delta>");
        float x = (float)std::atof(arg(0).c_str());
        float y = (float)std::atof(arg(1).c_str());
        float d = (float)std::atof(arg(2).c_str());
        return ui_debug_wheel_at(win, x, y, d) == 0 ? okJson() : errJson("wheel_at failed");
    }
    if (cmd == "focus") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        return ui_debug_focus(win, w) == 0 ? okJson() : errJson("focus failed");
    }
    if (cmd == "blur") {
        return ui_debug_blur(win) == 0 ? okJson() : errJson("blur failed");
    }
    if (cmd == "key") {
        if (arg(0).empty()) return errJson("usage: key <vk-or-name>");
        return ui_debug_key(win, parseVk(arg(0))) == 0 ? okJson() : errJson("key failed");
    }
    if (cmd == "type") {
        if (rest.empty()) return errJson("usage: type <text>");
        std::wstring w = utf8ToWide(trimLead(rest));
        return ui_debug_type_text(win, w.c_str()) == 0 ? okJson() : errJson("type failed");
    }

    // --- high-level controls ---
    if (cmd == "check") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        int v = parseBoolArg(arg(1), -1);
        int r = (v == -1) ? ui_debug_checkbox_toggle(win, w)
                          : ui_debug_checkbox_set(win, w, v);
        return r == 0 ? okJson() : errJson("check failed");
    }
    if (cmd == "toggle") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        int v = parseBoolArg(arg(1), -1);
        int r;
        if (v == -1) {
            auto* tg = dynamic_cast<ui::ToggleWidget*>(ui::GetContext().handles.LookupRaw(w));
            if (!tg) return errJson("not a Toggle");
            r = ui_debug_toggle_set(win, w, tg->On() ? 0 : 1);
        } else {
            r = ui_debug_toggle_set(win, w, v);
        }
        return r == 0 ? okJson() : errJson("toggle failed");
    }
    if (cmd == "radio") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        return ui_debug_radio_select(win, w) == 0 ? okJson() : errJson("radio failed");
    }
    if (cmd == "combo") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        if (arg(1).empty()) return errJson("usage: combo <id> <index>");
        return ui_debug_combo_select(win, w, std::atoi(arg(1).c_str())) == 0
                   ? okJson() : errJson("combo failed");
    }
    if (cmd == "combo_open") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        return ui_debug_combo_open(w) == 0 ? okJson() : errJson("combo_open failed");
    }
    if (cmd == "slider") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        if (arg(1).empty()) return errJson("usage: slider <id> <value>");
        return ui_debug_slider_set(win, w, (float)std::atof(arg(1).c_str())) == 0
                   ? okJson() : errJson("slider failed");
    }
    if (cmd == "number") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        if (arg(1).empty()) return errJson("usage: number <id> <value>");
        return ui_debug_number_set(win, w, (float)std::atof(arg(1).c_str())) == 0
                   ? okJson() : errJson("number failed");
    }
    if (cmd == "tab") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        if (arg(1).empty()) return errJson("usage: tab <id> <index>");
        int r = ui_debug_tab_set(w, std::atoi(arg(1).c_str()));
        if (r == 0) ui_window_invalidate(win);
        return r == 0 ? okJson() : errJson("tab failed");
    }
    if (cmd == "expander") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        int v = parseBoolArg(arg(1), -1);
        int want;
        if (v == -1) {
            auto* ex = dynamic_cast<ui::ExpanderWidget*>(ui::GetContext().handles.LookupRaw(w));
            if (!ex) return errJson("not an Expander");
            want = ex->IsExpanded() ? 0 : 1;
        } else want = v;
        int r = ui_debug_expander_set(w, want);
        if (r == 0) ui_window_invalidate(win);
        return r == 0 ? okJson() : errJson("expander failed");
    }
    if (cmd == "splitview") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        int v = parseBoolArg(arg(1), -1);
        int want;
        if (v == -1) {
            auto* sv = dynamic_cast<ui::SplitViewWidget*>(ui::GetContext().handles.LookupRaw(w));
            if (!sv) return errJson("not a SplitView");
            want = sv->IsPaneOpen() ? 0 : 1;
        } else want = v;
        int r = ui_debug_splitview_set(w, want);
        if (r == 0) ui_window_invalidate(win);
        return r == 0 ? okJson() : errJson("splitview failed");
    }
    if (cmd == "input" || cmd == "textarea" || cmd == "set_text") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        std::string txt;
        size_t sp = rest.find(' ');
        if (sp != std::string::npos) txt = rest.substr(sp + 1);
        std::wstring wt = utf8ToWide(txt);
        int r = ui_debug_text_set(w, wt.c_str());
        if (r == 0) ui_window_invalidate(win);
        return r == 0 ? okJson() : errJson("text set failed");
    }
    if (cmd == "scroll") {
        if (arg(0).empty() || arg(1).empty())
            return errJson("usage: scroll <id> <y>");
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        int r = ui_debug_scroll_set(w, (float)std::atof(arg(1).c_str()));
        if (r == 0) ui_window_invalidate(win);
        return r == 0 ? okJson() : errJson("scroll failed (not a ScrollView?)");
    }

    // --- ImageView ---
    if (cmd == "zoom") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        if (arg(1).empty()) return errJson("usage: zoom <id> <value>");
        ui_image_set_zoom(w, (float)std::atof(arg(1).c_str()));
        ui_window_invalidate(win);
        return okJson();
    }
    if (cmd == "pan") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        if (arg(2).empty()) return errJson("usage: pan <id> <x> <y>");
        ui_image_set_pan(w, (float)std::atof(arg(1).c_str()),
                            (float)std::atof(arg(2).c_str()));
        ui_window_invalidate(win);
        return okJson();
    }
    if (cmd == "rotate") {
        UiWidget w = WidgetByIdHandle(win, arg(0));
        if (!w) return errJson("widget not found");
        if (arg(1).empty()) return errJson("usage: rotate <id> <angle>");
        ui_image_set_rotation(w, std::atoi(arg(1).c_str()));
        ui_window_invalidate(win);
        return okJson();
    }

    // --- context menu (operates on currently active menu) ---
    if (cmd == "menu_is_open") {
        return okFmt("{\"ok\":true,\"open\":%s}",
                     ui_debug_menu_is_open(win) ? "true" : "false");
    }
    if (cmd == "menu_count") {
        int n = ui_debug_menu_item_count(win);
        if (n < 0) return errJson("no menu open");
        return okFmt("{\"ok\":true,\"count\":%d}", n);
    }
    if (cmd == "menu_click") {
        if (arg(0).empty()) return errJson("usage: menu_click <index>");
        int r = ui_debug_menu_click_index(win, std::atoi(arg(0).c_str()));
        if (r == 0) ui_debug_pump();
        return r == 0 ? okJson() : errJson("menu_click failed");
    }
    if (cmd == "menu_click_id") {
        if (arg(0).empty()) return errJson("usage: menu_click_id <item_id>");
        int r = ui_debug_menu_click_id(win, std::atoi(arg(0).c_str()));
        if (r == 0) ui_debug_pump();
        return r == 0 ? okJson() : errJson("menu_click_id failed");
    }
    if (cmd == "menu_close") {
        return ui_debug_menu_close(win) == 0 ? okJson() : errJson("menu_close failed");
    }
    if (cmd == "menu_click_path") {
        std::vector<int> path;
        if (!parseMenuPath(arg(0), path)) return errJson("usage: menu_click_path i0/i1/...");
        int r = ui_debug_menu_click_path(win, path.data(), (int)path.size());
        if (r == 0) ui_debug_pump();
        return r == 0 ? okJson() : errJson("menu_click_path failed");
    }
    if (cmd == "menu_count_at") {
        std::vector<int> path;
        if (!arg(0).empty() && !parseMenuPath(arg(0), path))
            return errJson("usage: menu_count_at [i0/i1/...]");
        int n = ui_debug_menu_item_count_at(win, path.empty() ? nullptr : path.data(),
                                                 (int)path.size());
        if (n < 0) return errJson("invalid path or no menu");
        return okFmt("{\"ok\":true,\"count\":%d}", n);
    }
    if (cmd == "menu_has_sub") {
        std::vector<int> path;
        if (!parseMenuPath(arg(0), path)) return errJson("usage: menu_has_sub i0/i1/...");
        int v = ui_debug_menu_has_submenu_at(win, path.data(), (int)path.size());
        return okFmt("{\"ok\":true,\"hasSubmenu\":%s}", v ? "true" : "false");
    }
    if (cmd == "menu_id_at") {
        std::vector<int> path;
        if (!parseMenuPath(arg(0), path)) return errJson("usage: menu_id_at i0/i1/...");
        int id = ui_debug_menu_item_id_at(win, path.data(), (int)path.size());
        if (id < 0) return errJson("invalid path / separator");
        return okFmt("{\"ok\":true,\"itemId\":%d}", id);
    }
    if (cmd == "menu_autoclose") {
        int v = parseBoolArg(arg(0), -1);
        if (v < 0) return errJson("usage: menu_autoclose 0|1");
        ui_debug_set_menu_autoclose(v);
        return okJson();
    }

    // --- dialog ---
    if (cmd == "dialog_confirm") {
        return ui_debug_dialog_confirm(win) == 0 ? okJson() : errJson("no active dialog");
    }
    if (cmd == "dialog_cancel") {
        return ui_debug_dialog_cancel(win) == 0 ? okJson() : errJson("no active dialog");
    }

    // --- HWND channel (PostMessage) ---
    if (cmd == "post_click") {
        if (arg(1).empty()) return errJson("usage: post_click <x> <y>");
        return ui_debug_post_click(win, (float)std::atof(arg(0).c_str()),
                                        (float)std::atof(arg(1).c_str())) == 0
                   ? okJson() : errJson("post_click failed");
    }
    if (cmd == "post_rclick") {
        if (arg(1).empty()) return errJson("usage: post_rclick <x> <y>");
        return ui_debug_post_right_click(win, (float)std::atof(arg(0).c_str()),
                                              (float)std::atof(arg(1).c_str())) == 0
                   ? okJson() : errJson("post_rclick failed");
    }
    if (cmd == "post_key") {
        if (arg(0).empty()) return errJson("usage: post_key <vk-or-name>");
        return ui_debug_post_key(win, parseVk(arg(0))) == 0
                   ? okJson() : errJson("post_key failed");
    }
    if (cmd == "post_char") {
        if (arg(0).empty()) return errJson("usage: post_char <codepoint>");
        return ui_debug_post_char(win, (unsigned int)std::atoi(arg(0).c_str())) == 0
                   ? okJson() : errJson("post_char failed");
    }

    if (cmd == "help" || cmd == "?") {
        return
        "{\"commands\":["
        "\"tree\",\"widget <id>\",\"highlight <id>\",\"screenshot <path>\","
        "\"screenshot_widget <id> <path>\",\"invalidate\",\"pump\","
        "\"click <id>\",\"click_at <x> <y>\",\"dbl_click <id>\","
        "\"rclick <id>\",\"rclick_at <x> <y>\",\"hover <id>\",\"move <x> <y>\","
        "\"drag <id> <dx> <dy>\",\"drag_to <x1> <y1> <x2> <y2>\","
        "\"wheel <id> <delta>\",\"wheel_at <x> <y> <delta>\","
        "\"focus <id>\",\"blur\",\"key <vk|name>\",\"type <text>\","
        "\"check <id> [0|1|toggle]\",\"toggle <id> [0|1|toggle]\",\"radio <id>\","
        "\"combo <id> <idx>\",\"combo_open <id>\",\"slider <id> <v>\","
        "\"number <id> <v>\",\"tab <id> <idx>\",\"expander <id> [0|1|toggle]\","
        "\"splitview <id> [0|1|toggle]\",\"scroll <id> <y>\","
        "\"input <id> <text>\",\"textarea <id> <text>\",\"set_text <id> <text>\","
        "\"zoom <id> <v>\",\"pan <id> <x> <y>\",\"rotate <id> <deg>\","
        "\"menu_is_open\",\"menu_count\",\"menu_click <idx>\","
        "\"menu_click_id <id>\",\"menu_close\",\"menu_click_path i0/i1/...\","
        "\"menu_count_at [path]\",\"menu_has_sub path\",\"menu_id_at path\","
        "\"menu_autoclose 0|1\","
        "\"dialog_confirm\",\"dialog_cancel\","
        "\"post_click <x> <y>\",\"post_rclick <x> <y>\","
        "\"post_key <vk|name>\",\"post_char <cp>\""
        "]}";
    }

    return std::string();  // empty → unrecognized; let user handler / pipe
                           // server respond with errJson.
}

// ============================================================================
// Pipe server thread + global state
// ============================================================================

struct ServerState {
    UiWindow win = 0;
    std::string pipeName;       // without "\\.\pipe\" prefix
    std::thread thread;
    std::atomic<bool> shutdown{false};
    HANDLE pipe = INVALID_HANDLE_VALUE;
    HANDLE event = nullptr;     // overlapped ConnectNamedPipe event
    UiDebugCommandHandler userHandler = nullptr;
    void* userHandlerData = nullptr;
    std::mutex handlerMu;       // guards userHandler swap during dispatch
};

static ServerState g_state;     // single global server

static std::string DispatchOnce(UiWindow win, const std::string& head,
                                const std::string& rest) {
    // 1. user handler first (lets app override builtins)
    UiDebugCommandHandler cb = nullptr;
    void* cbData = nullptr;
    {
        std::lock_guard<std::mutex> lk(g_state.handlerMu);
        cb = g_state.userHandler;
        cbData = g_state.userHandlerData;
    }
    if (cb) {
        char buf[8192];
        int n = cb(head.c_str(), rest.c_str(), buf, (int)sizeof(buf), cbData);
        if (n > 0) {
            return std::string(buf, std::min(n, (int)sizeof(buf)));
        }
    }
    // 2. builtin
    std::string r = BuiltinDispatch(win, head, rest);
    if (!r.empty()) return r;
    // 3. unknown
    return errJson("unknown command: " + head + "  (send 'help' for list)");
}

static void PipeWorker(UiWindow win, std::string pipeName) {
    std::string fullName = "\\\\.\\pipe\\" + pipeName;
    while (!g_state.shutdown.load()) {
        HANDLE pipe = CreateNamedPipeA(
            fullName.c_str(),
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, 65536, 65536, 0, NULL);
        if (pipe == INVALID_HANDLE_VALUE) break;
        g_state.pipe = pipe;

        OVERLAPPED ov = {}; ov.hEvent = g_state.event;
        ResetEvent(g_state.event);
        BOOL connected = ConnectNamedPipe(pipe, &ov);
        if (!connected) {
            DWORD err = GetLastError();
            if (err == ERROR_IO_PENDING) {
                WaitForSingleObject(g_state.event, INFINITE);
                if (g_state.shutdown.load()) {
                    CancelIo(pipe); CloseHandle(pipe); break;
                }
            } else if (err != ERROR_PIPE_CONNECTED) {
                CloseHandle(pipe); continue;
            }
        }
        if (g_state.shutdown.load()) { CloseHandle(pipe); break; }

        char buf[4096] = {};
        DWORD bytesRead = 0;
        if (ReadFile(pipe, buf, sizeof(buf) - 1, &bytesRead, NULL) && bytesRead > 0) {
            if (g_state.shutdown.load()) { CloseHandle(pipe); break; }
            buf[bytesRead] = 0;
            while (bytesRead > 0 && (buf[bytesRead - 1] == '\n' || buf[bytesRead - 1] == '\r')) {
                buf[--bytesRead] = 0;
            }
            std::string line(buf);
            size_t sp = line.find_first_of(" \t");
            std::string head = (sp == std::string::npos) ? line : line.substr(0, sp);
            std::string rest = (sp == std::string::npos) ? "" : line.substr(sp + 1);

            // Marshal dispatch onto UI thread — every Sim*/widget mutation
            // must run on the thread that owns the window's HWND.
            struct DispatchReq {
                UiWindow win;
                const std::string* head;
                const std::string* rest;
                std::string* resp;
            };
            std::string response;
            DispatchReq req{win, &head, &rest, &response};
            ui_window_invoke_sync(win, [](void* ud) {
                auto* r = static_cast<DispatchReq*>(ud);
                *r->resp = DispatchOnce(r->win, *r->head, *r->rest);
            }, &req);

            DWORD written;
            WriteFile(pipe, response.c_str(), (DWORD)response.size(), &written, NULL);
            FlushFileBuffers(pipe);
        }
        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }
    g_state.pipe = INVALID_HANDLE_VALUE;
}

}  // namespace ui::debug

// ============================================================================
// Public C API
// ============================================================================

extern "C" {

UI_API int ui_debug_server_start(UiWindow win, const char* pipe_name) {
    using namespace ui::debug;
    if (g_state.thread.joinable()) return -2;  // already running
    if (!ui::GetContext().GetWindow(win)) return -1;

    g_state.win = win;
    g_state.pipeName = (pipe_name && *pipe_name) ? pipe_name : "ui_core_debug";
    g_state.shutdown.store(false);
    g_state.event = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (!g_state.event) return -3;

    g_state.thread = std::thread(ui::debug::PipeWorker, win, g_state.pipeName);
    return 0;
}

UI_API void ui_debug_server_stop(void) {
    using namespace ui::debug;
    if (!g_state.thread.joinable()) return;
    g_state.shutdown.store(true);
    if (g_state.event) SetEvent(g_state.event);
    if (g_state.pipe != INVALID_HANDLE_VALUE) CancelIoEx(g_state.pipe, NULL);
    g_state.thread.join();
    if (g_state.event) { CloseHandle(g_state.event); g_state.event = nullptr; }
    g_state.pipeName.clear();
    g_state.win = 0;
    {
        std::lock_guard<std::mutex> lk(g_state.handlerMu);
        g_state.userHandler = nullptr;
        g_state.userHandlerData = nullptr;
    }
}

UI_API void ui_debug_server_set_handler(UiDebugCommandHandler cb, void* userdata) {
    using namespace ui::debug;
    std::lock_guard<std::mutex> lk(g_state.handlerMu);
    g_state.userHandler = cb;
    g_state.userHandlerData = userdata;
}

}  // extern "C"
