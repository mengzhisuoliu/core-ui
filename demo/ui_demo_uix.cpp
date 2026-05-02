// ui-demo-uix — single-file Vue 3 SFC demo.
// Default locale is zh; the i18n / settings pages let the user switch.
//
// Methods{} can't call the C ui_page_set_locale directly, so the .uix
// writes its desired locale to state.requestedLocale and a 200ms
// SetTimer here forwards changes to set_locale. The reactive bindings
// then auto-refire and every $t() call returns the new translation.

#include <ui_core.h>
#include <windows.h>
#include <cstring>

#include "app_uix.embed.h"
#include "lang_zh.embed.h"
#include "lang_en.embed.h"

static UiPage g_page = 0;
static char   g_curLocale[16] = "zh";
static int    g_curDark = 0;

static VOID CALLBACK OnPoll(HWND, UINT, UINT_PTR, DWORD) {
    if (!g_page) return;

    // Locale: settings page writes requestedLocale via setLang(); forward
    // it to ui_page_set_locale so $t bindings refire.
    if (char* req = ui_page_get_json(g_page, "requestedLocale")) {
        const char* want = nullptr;
        if      (std::strstr(req, "\"zh\"")) want = "zh";
        else if (std::strstr(req, "\"en\"")) want = "en";
        if (want && std::strcmp(g_curLocale, want) != 0) {
            ui_page_set_locale(g_page, want);
            std::strncpy(g_curLocale, want, sizeof(g_curLocale) - 1);
            g_curLocale[sizeof(g_curLocale) - 1] = 0;
        }
        ui_page_free(req);
    }

    // Dark mode: <toggle v-model="dark"/> writes state.dark; forward to
    // ui_theme_set_mode so native widgets (toggle/checkbox/button/input)
    // repaint with dark colors. Demo CSS responds via the .dark class
    // bound on the shell <div>.
    if (char* j = ui_page_get_json(g_page, "dark")) {
        int wantDark = (std::strstr(j, "true") != nullptr) ? 1 : 0;
        if (wantDark != g_curDark) {
            ui_theme_set_mode(wantDark ? UI_THEME_DARK : UI_THEME_LIGHT);
            g_curDark = wantDark;
        }
        ui_page_free(j);
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    ui_init_with_theme(UI_THEME_LIGHT);

    UiPage page = ui_page_load_string(k_app_uix);
    if (!page) return 1;
    g_page = page;

    ui_page_load_language_string(page, "zh", k_lang_zh);
    ui_page_load_language_string(page, "en", k_lang_en);
    ui_page_set_locale(page, "zh");
    std::strcpy(g_curLocale, "zh");

    UiWindow win = ui_page_open_window(page, NULL);
    if (!win) { ui_page_destroy(page); return 2; }

    // 200ms poll: the i18n / settings pages write requestedLocale into
    // state via the @click="setLang('zh'|'en')" methods; we forward the
    // change to ui_page_set_locale so $t bindings refire.
    HWND hwnd = (HWND)ui_window_hwnd(win);
    SetTimer(hwnd, 1, 200, OnPoll);

    ui_debug_server_start(win, NULL);
    ui_run();
    ui_page_destroy(page);
    return 0;
}
