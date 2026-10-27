// O'zbek harflari для Windows: o' → ö, g' → ğ, sh → ş, ch → ç во всех программах.
//
// Низкоуровневый хук клавиатуры определяет набранный символ в текущей раскладке. Если символ
// завершает правило, нажатие гасится, а символ перед кареткой заменяется через SendInput
// (Backspace + символ Unicode). Сама логика замены — в engine.c.
//
// Потоки:
//   главный — значок в трее, меню, горячая клавиша, настройки и политики;
//   хук     — WH_KEYBOARD_LL / WH_MOUSE_LL. Ничего медленного: Windows молча снимает хуки,
//             которые не укладываются в LowLevelHooksTimeout;
//   a11y    — следит за фокусом через MSAA: в полях пароля замена не работает.
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <oleacc.h>
#include <stdio.h>
#include <wchar.h>

#include "engine.h"
#include "resource.h"
#include "version.h"

#define APP_TITLE L"O'zbek harflari"
#define SETTINGS_KEY L"Software\\UzbekHarflari"
#define POLICY_KEY L"Software\\Policies\\UzbekHarflari"
#define APPS_SUBKEY L"\\ExcludedApps"

// Метка наших SendInput, чтобы хук не обрабатывал их повторно.
#define INJECT_MARK ((ULONG_PTR)0x555A4842)

#define WM_TRAY (WM_APP + 1)
#define WM_HOOK_FAILED (WM_APP + 2)
#define TRAY_ID 1
#define TIMER_TRAY_RETRY 1
#define HOTKEY_TOGGLE 1
#define MAX_APPS 64
#define APP_LEN 64

enum { CMD_ENABLED = 1, CMD_APP, CMD_EXIT, CMD_RULE };

// Имена значений в реестре и в политиках для UZ_RULES — в том же порядке.
static const WCHAR *const RULE_VALUES[] = { L"RuleO", L"RuleG", L"RuleSh", L"RuleCh" };
_Static_assert(ARRAYSIZE(RULE_VALUES) == UZ_RULE_COUNT, "RULE_VALUES must match UZ_RULES");

// Окна оболочки: после клика по значку в трее активным становится панель задач, а пункт меню
// «Shu dasturda ishlaydi» должен относиться к программе, в которой пользователь печатал.
static const WCHAR *const SHELL_CLASSES[] = {
  L"Shell_TrayWnd", L"Shell_SecondaryTrayWnd", L"NotifyIconOverflowWindow",
  L"TopLevelWindowForOverflowXamlIsland", L"Windows.UI.Core.CoreWindow", L"Progman", L"WorkerW",
};

typedef struct {
  BOOL enabled, enabledLocked;
  BOOL rules[UZ_RULE_COUNT], rulesLocked[UZ_RULE_COUNT];
  WCHAR userApps[MAX_APPS][APP_LEN];   // имена exe в нижнем регистре
  WCHAR policyApps[MAX_APPS][APP_LEN];
  int userAppCount, policyAppCount;
} Settings;

// Общее для потоков. Настройки пишет главный поток, читают хук и меню.
static SRWLOCK g_lock = SRWLOCK_INIT;
static Settings g_settings;
static WCHAR g_lastApp[APP_LEN];
static volatile LONG g_settingsVersion;
static volatile LONG g_passwordFocus;

static HINSTANCE g_inst;
static HWND g_wnd;
static HICON g_iconOn, g_iconOff;
static UINT g_taskbarCreated;
static DWORD g_ownIntegrity;

// Только поток хука.
static UzEngine g_engine;
static HWND g_hookWindow;
static LONG g_hookVersion = -1;
static BOOL g_hookActive;      // замена разрешена в активном окне
static DWORD g_swallowKeyUp;   // клавиша, чьё нажатие погашено, — гасим и её отпускание
static BOOL g_capsLock, g_capsHeld;
static BOOL g_afterDeadKey;

/* ---------- Настройки ---------- */

static BOOL read_dword(HKEY root, const WCHAR *key, const WCHAR *name, DWORD *value) {
  DWORD size = sizeof *value;
  return RegGetValueW(root, key, name, RRF_RT_REG_DWORD, NULL, value, &size) == ERROR_SUCCESS;
}

// Политика (HKLM, затем HKCU) сильнее пользовательской настройки и блокирует её в меню.
static void load_flag(const WCHAR *name, BOOL *value, BOOL *locked) {
  DWORD v;
  *locked = read_dword(HKEY_LOCAL_MACHINE, POLICY_KEY, name, &v) ||
            read_dword(HKEY_CURRENT_USER, POLICY_KEY, name, &v);
  if (*locked || read_dword(HKEY_CURRENT_USER, SETTINGS_KEY, name, &v)) *value = v != 0;
}

// Список программ — строковые значения подраздела (так пишет его и GPO для элемента list в ADMX).
static int load_apps(HKEY root, const WCHAR *key, WCHAR (*apps)[APP_LEN], int count) {
  HKEY h;
  if (RegOpenKeyExW(root, key, 0, KEY_READ, &h) != ERROR_SUCCESS) return count;
  for (DWORD i = 0; count < MAX_APPS; i++) {
    WCHAR name[256], data[MAX_PATH];
    DWORD nameLen = ARRAYSIZE(name), size = sizeof data - sizeof(WCHAR), type;
    LONG r = RegEnumValueW(h, i, name, &nameLen, NULL, &type, (BYTE *)data, &size);
    if (r == ERROR_NO_MORE_ITEMS) break;
    if (r != ERROR_SUCCESS || type != REG_SZ) continue;
    data[size / sizeof(WCHAR)] = 0;
    // Администратор мог указать полный путь — сравниваем только имя файла.
    const WCHAR *exe = wcsrchr(data, L'\\');
    exe = exe ? exe + 1 : data;
    if (!*exe || wcslen(exe) >= APP_LEN) continue;
    wcscpy(apps[count], exe);
    CharLowerW(apps[count++]);
  }
  RegCloseKey(h);
  return count;
}

static void load_settings(void) {
  static Settings s;
  ZeroMemory(&s, sizeof s);
  s.enabled = TRUE;
  load_flag(L"Enabled", &s.enabled, &s.enabledLocked);
  for (int i = 0; i < UZ_RULE_COUNT; i++) {
    s.rules[i] = TRUE;
    load_flag(RULE_VALUES[i], &s.rules[i], &s.rulesLocked[i]);
  }
  s.userAppCount = load_apps(HKEY_CURRENT_USER, SETTINGS_KEY APPS_SUBKEY, s.userApps, 0);
  s.policyAppCount = load_apps(HKEY_LOCAL_MACHINE, POLICY_KEY APPS_SUBKEY, s.policyApps, 0);
  s.policyAppCount = load_apps(HKEY_CURRENT_USER, POLICY_KEY APPS_SUBKEY, s.policyApps, s.policyAppCount);

  AcquireSRWLockExclusive(&g_lock);
  g_settings = s;
  ReleaseSRWLockExclusive(&g_lock);
  InterlockedIncrement(&g_settingsVersion);
}

static void save_value(const WCHAR *key, const WCHAR *name, DWORD type, const void *data, DWORD size) {
  HKEY h;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, key, 0, NULL, 0, KEY_SET_VALUE, NULL, &h, NULL) != ERROR_SUCCESS) return;
  RegSetValueExW(h, name, 0, type, data, size);
  RegCloseKey(h);
}

static void save_flag(const WCHAR *name, BOOL value) {
  DWORD v = value != 0;
  save_value(SETTINGS_KEY, name, REG_DWORD, &v, sizeof v);
}

static void save_app_excluded(const WCHAR *exe, BOOL excluded) {
  if (excluded) {
    save_value(SETTINGS_KEY APPS_SUBKEY, exe, REG_SZ, exe, (DWORD)((wcslen(exe) + 1) * sizeof(WCHAR)));
  } else {
    RegDeleteKeyValueW(HKEY_CURRENT_USER, SETTINGS_KEY APPS_SUBKEY, exe);
  }
}

static BOOL in_list(WCHAR (*apps)[APP_LEN], int count, const WCHAR *exe) {
  for (int i = 0; i < count; i++) {
    if (wcscmp(apps[i], exe) == 0) return TRUE;
  }
  return FALSE;
}

/* ---------- Процессы ---------- */

static DWORD integrity_level(HANDLE process) {
  HANDLE token;
  DWORD level = MAXDWORD;
  union { TOKEN_MANDATORY_LABEL label; BYTE raw[64]; } info;
  DWORD size;
  if (!OpenProcessToken(process, TOKEN_QUERY, &token)) return level;
  if (GetTokenInformation(token, TokenIntegrityLevel, &info, sizeof info, &size)) {
    PSID sid = info.label.Label.Sid;
    level = *GetSidSubAuthority(sid, *GetSidSubAuthorityCount(sid) - 1);
  }
  CloseHandle(token);
  return level;
}

// Имя exe владельца окна в нижнем регистре. FALSE — наш процесс или нет доступа.
static BOOL window_app(HWND wnd, WCHAR exe[APP_LEN], DWORD *integrity) {
  DWORD pid = 0;
  GetWindowThreadProcessId(wnd, &pid);
  if (!pid || pid == GetCurrentProcessId()) return FALSE;
  HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (!process) return FALSE;

  WCHAR path[MAX_PATH];
  DWORD len = ARRAYSIZE(path);
  BOOL ok = QueryFullProcessImageNameW(process, 0, path, &len);
  if (ok) {
    const WCHAR *name = wcsrchr(path, L'\\');
    lstrcpynW(exe, name ? name + 1 : path, APP_LEN);
    CharLowerW(exe);
    if (integrity) *integrity = integrity_level(process);
  }
  CloseHandle(process);
  return ok;
}

/* ---------- Поток хука ---------- */

static BOOL is_down(int vk) {
  return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

static BOOL is_modifier(DWORD vk) {
  switch (vk) {
  case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
  case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
  case VK_MENU: case VK_LMENU: case VK_RMENU:
  case VK_LWIN: case VK_RWIN: case VK_NUMLOCK: case VK_SCROLL:
    return TRUE;
  }
  return FALSE;
}

// Пересчитывается при смене активного окна или настроек, а не на каждое нажатие.
static void refresh_target(HWND wnd) {
  WCHAR exe[APP_LEN];
  DWORD integrity = MAXDWORD;
  // В окна с более высоким уровнем целостности (запущенные от администратора) UIPI не пропустит
  // SendInput, и погашенная клавиша просто пропала бы.
  BOOL reachable = wnd && window_app(wnd, exe, &integrity) && integrity <= g_ownIntegrity;

  AcquireSRWLockShared(&g_lock);
  g_hookActive = reachable && g_settings.enabled &&
                 !in_list(g_settings.userApps, g_settings.userAppCount, exe) &&
                 !in_list(g_settings.policyApps, g_settings.policyAppCount, exe);
  for (int i = 0; i < UZ_RULE_COUNT; i++) g_engine.rules[i] = g_settings.rules[i] != 0;
  ReleaseSRWLockShared(&g_lock);
}

// Окно с фокусом ввода. У UWP-приложений и консолей оно живёт в другом потоке, чем активное
// окно, а раскладка у каждого потока своя.
static HWND input_focus(HWND wnd) {
  GUITHREADINFO info = { .cbSize = sizeof info };
  return GetGUIThreadInfo(GetWindowThreadProcessId(wnd, NULL), &info) && info.hwndFocus ? info.hwndFocus : wnd;
}

// Классические поля пароля (Win32, WinForms, RichEdit) видны по стилю сразу, без MSAA.
static BOOL is_password_edit(HWND focus) {
  WCHAR cls[64];
  if (!(GetWindowLongPtrW(focus, GWL_STYLE) & ES_PASSWORD)) return FALSE;
  if (!GetClassNameW(focus, cls, ARRAYSIZE(cls))) return FALSE;
  CharLowerW(cls);
  return wcsstr(cls, L"edit") != NULL;
}

// Символ клавиши в раскладке окна с фокусом: 1 — обычный, -1 — «мёртвая» клавиша, 0 — не символ.
static int key_char(const KBDLLHOOKSTRUCT *k, HWND focus, WCHAR *ch) {
  BYTE state[256] = { 0 };
  WCHAR buf[8] = { 0 };
  if (is_down(VK_SHIFT)) state[VK_SHIFT] = 0x80;
  if (g_capsLock) state[VK_CAPITAL] = 0x01;
  HKL layout = GetKeyboardLayout(GetWindowThreadProcessId(focus, NULL));
  // Китайский, японский и корейский IME сами собирают символы из латиницы — не вмешиваемся.
  switch (PRIMARYLANGID(LOWORD((UINT_PTR)layout))) {
  case LANG_CHINESE: case LANG_JAPANESE: case LANG_KOREAN:
    return 0;
  }
  // Флаг 4 — не менять состояние клавиатуры ядра, иначе ломаются «мёртвые» клавиши (Windows 10 1607+).
  int n = ToUnicodeEx(k->vkCode, k->scanCode, state, buf, ARRAYSIZE(buf), 4, layout);
  *ch = buf[0];
  if (n < 0) return -1;
  return n == 1 && buf[0] >= 0x20 && buf[0] != 0x7f;
}

// Стирает символ перед кареткой и печатает text — как replaceBeforeCaret в content.js.
static BOOL replace_before_caret(const WCHAR *text, DWORD vk) {
  INPUT in[8];
  UINT n = 0;
  ZeroMemory(in, sizeof in);
  for (int up = 0; up < 2; up++, n++) {
    in[n].type = INPUT_KEYBOARD;
    in[n].ki.wVk = VK_BACK;
    in[n].ki.wScan = (WORD)MapVirtualKeyW(VK_BACK, MAPVK_VK_TO_VSC);
    in[n].ki.dwFlags = up ? KEYEVENTF_KEYUP : 0;
    in[n].ki.dwExtraInfo = INJECT_MARK;
  }
  for (; *text && n + 2 <= ARRAYSIZE(in); text++) {
    for (int up = 0; up < 2; up++, n++) {
      in[n].type = INPUT_KEYBOARD;
      in[n].ki.wScan = *text;
      in[n].ki.dwFlags = KEYEVENTF_UNICODE | (up ? KEYEVENTF_KEYUP : 0);
      in[n].ki.dwExtraInfo = INJECT_MARK;
    }
  }
  // Вызов прямо из хука: вставленный ввод встаёт в очередь раньше следующих нажатий.
  if (SendInput(n, in, sizeof(INPUT)) != n) {
    uz_reset(&g_engine);
    return FALSE;
  }
  g_swallowKeyUp = vk;
  return TRUE;
}

// TRUE — нажатие погасить.
static BOOL handle_key(const KBDLLHOOKSTRUCT *k, BOOL down) {
  DWORD vk = k->vkCode;
  if (k->dwExtraInfo == INJECT_MARK) return FALSE;

  if (!down) {
    if (vk == VK_CAPITAL) g_capsHeld = FALSE;
    if (vk != g_swallowKeyUp) return FALSE;
    g_swallowKeyUp = 0;
    return TRUE;
  }
  // GetKeyState в потоке хука не успевает за вводом, поэтому Caps Lock отслеживаем сами.
  if (vk == VK_CAPITAL) {
    if (!g_capsHeld) g_capsLock = !g_capsLock;
    g_capsHeld = TRUE;
    return FALSE;
  }
  if (is_modifier(vk)) return FALSE;
  // Автоповтор дошёл до программы — значит, и отпускание должно дойти.
  if (vk == g_swallowKeyUp) g_swallowKeyUp = 0;

  HWND wnd = GetForegroundWindow();
  if (wnd != g_hookWindow) {
    g_hookWindow = wnd;
    g_hookVersion = -1;
    g_afterDeadKey = FALSE;
    uz_reset(&g_engine);
  }
  LONG version = g_settingsVersion;
  if (version != g_hookVersion) {
    g_hookVersion = version;
    refresh_target(wnd);
  }

  // Сочетания клавиш, чужой программный ввод (экранная клавиатура, автонабор паролей)
  // и поля пароля не трогаем.
  if (!g_hookActive || (k->flags & LLKHF_INJECTED) ||
      is_down(VK_CONTROL) || is_down(VK_MENU) || is_down(VK_LWIN) || is_down(VK_RWIN) || g_passwordFocus) {
    uz_reset(&g_engine);
    return FALSE;
  }
  HWND focus = input_focus(wnd);
  if (is_password_edit(focus)) {
    uz_reset(&g_engine);
    return FALSE;
  }

  WCHAR text[3];
  if (vk == VK_BACK) return uz_backspace(&g_engine, text) && replace_before_caret(text, vk);

  WCHAR ch;
  int kind = key_char(k, focus, &ch);
  if (kind == 0 || g_afterDeadKey) {
    // После «мёртвой» клавиши программа сама составит символ — какой, мы не знаем.
    g_afterDeadKey = FALSE;
    uz_reset(&g_engine);
    return FALSE;
  }
  // «Мёртвый» апостроф (раскладка US-International) тоже завершает o' и g'.
  if (uz_type(&g_engine, ch, text)) return replace_before_caret(text, vk);
  if (kind < 0) {
    g_afterDeadKey = TRUE;
    uz_reset(&g_engine);
  }
  return FALSE;
}

static LRESULT CALLBACK keyboard_proc(int code, WPARAM wp, LPARAM lp) {
  if (code == HC_ACTION && handle_key((const KBDLLHOOKSTRUCT *)lp, wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN)) {
    return 1;
  }
  return CallNextHookEx(NULL, code, wp, lp);
}

static LRESULT CALLBACK mouse_proc(int code, WPARAM wp, LPARAM lp) {
  // Клик может переставить каретку — история символов перед ней больше не верна.
  if (code == HC_ACTION && wp != WM_MOUSEMOVE && wp != WM_MOUSEWHEEL && wp != WM_MOUSEHWHEEL) {
    uz_reset(&g_engine);
  }
  return CallNextHookEx(NULL, code, wp, lp);
}

static DWORD WINAPI hook_thread(LPVOID param) {
  (void)param;
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
  HHOOK keyboard = SetWindowsHookExW(WH_KEYBOARD_LL, keyboard_proc, g_inst, 0);
  HHOOK mouse = SetWindowsHookExW(WH_MOUSE_LL, mouse_proc, g_inst, 0);
  if (!keyboard) {
    PostMessageW(g_wnd, WM_HOOK_FAILED, 0, 0);
  } else {
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {}
    UnhookWindowsHookEx(keyboard);
  }
  if (mouse) UnhookWindowsHookEx(mouse);
  return 0;
}

/* ---------- Поток a11y ---------- */

static BOOL has_protected_state(IAccessible *acc, VARIANT child) {
  VARIANT state;
  VariantInit(&state);
  BOOL result = SUCCEEDED(IAccessible_get_accState(acc, child, &state)) && state.vt == VT_I4 &&
                (state.lVal & STATE_SYSTEM_PROTECTED);
  VariantClear(&state);
  return result;
}

// STATE_SYSTEM_PROTECTED ставят поля пароля в Win32, WinForms, WPF, Chrome, Edge и Firefox.
static BOOL is_password_field(HWND wnd, LONG object, LONG child) {
  IAccessible *acc = NULL;
  VARIANT target;
  VariantInit(&target);
  if (FAILED(AccessibleObjectFromEvent(wnd, object, child, &acc, &target)) || !acc) return FALSE;

  BOOL result = has_protected_state(acc, target);
  if (!result) {
    // Chrome и Edge включают доступность только при первом запросе и не сообщают о поле,
    // которое уже было в фокусе, — спрашиваем фокус у документа.
    VARIANT focus;
    VariantInit(&focus);
    if (SUCCEEDED(IAccessible_get_accFocus(acc, &focus))) {
      if (focus.vt == VT_DISPATCH && focus.pdispVal) {
        IAccessible *focused = NULL;
        if (SUCCEEDED(IDispatch_QueryInterface(focus.pdispVal, &IID_IAccessible, (void **)&focused))) {
          VARIANT self;
          VariantInit(&self);
          self.vt = VT_I4;
          self.lVal = CHILDID_SELF;
          result = has_protected_state(focused, self);
          IAccessible_Release(focused);
        }
      } else if (focus.vt == VT_I4 && focus.lVal != CHILDID_SELF) {
        result = has_protected_state(acc, focus);
      }
      VariantClear(&focus);
    }
  }
  VariantClear(&target);
  IAccessible_Release(acc);
  return result;
}

static void remember_app(HWND wnd) {
  WCHAR cls[64], exe[APP_LEN];
  if (!wnd || !GetClassNameW(wnd, cls, ARRAYSIZE(cls))) return;
  for (size_t i = 0; i < ARRAYSIZE(SHELL_CLASSES); i++) {
    if (wcscmp(cls, SHELL_CLASSES[i]) == 0) return;
  }
  if (!window_app(wnd, exe, NULL)) return;
  AcquireSRWLockExclusive(&g_lock);
  wcscpy(g_lastApp, exe);
  ReleaseSRWLockExclusive(&g_lock);
}

static void CALLBACK win_event_proc(HWINEVENTHOOK hook, DWORD event, HWND wnd, LONG object, LONG child,
                                    DWORD thread, DWORD time) {
  (void)hook; (void)thread; (void)time;
  if (event == EVENT_SYSTEM_FOREGROUND) {
    InterlockedExchange(&g_passwordFocus, 0);
    remember_app(wnd);
  } else if (wnd) {
    InterlockedExchange(&g_passwordFocus, is_password_field(wnd, object, child));
  }
}

static DWORD WINAPI a11y_thread(LPVOID param) {
  (void)param;
  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  DWORD flags = WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS;
  HWINEVENTHOOK foreground = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL,
                                             win_event_proc, 0, 0, flags);
  HWINEVENTHOOK focus = SetWinEventHook(EVENT_OBJECT_FOCUS, EVENT_OBJECT_FOCUS, NULL, win_event_proc, 0, 0, flags);
  remember_app(GetForegroundWindow());

  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0) > 0) DispatchMessageW(&msg);

  if (foreground) UnhookWinEvent(foreground);
  if (focus) UnhookWinEvent(focus);
  CoUninitialize();
  return 0;
}

/* ---------- Трей и меню ---------- */

static BOOL update_tray(DWORD action) {
  AcquireSRWLockShared(&g_lock);
  BOOL enabled = g_settings.enabled;
  ReleaseSRWLockShared(&g_lock);

  NOTIFYICONDATAW nid = { .cbSize = sizeof nid, .hWnd = g_wnd, .uID = TRAY_ID };
  nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
  nid.uCallbackMessage = WM_TRAY;
  nid.hIcon = enabled ? g_iconOn : g_iconOff;
  swprintf(nid.szTip, ARRAYSIZE(nid.szTip), L"%ls — %ls", APP_TITLE, enabled ? L"yoqilgan" : L"o'chirilgan");
  if (!Shell_NotifyIconW(action, &nid)) return FALSE;
  if (action == NIM_ADD) {
    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nid);
  }
  return TRUE;
}

static void add_tray(void) {
  // При входе в систему панель задач может ещё не быть готова — пробуем снова.
  if (!update_tray(NIM_ADD)) SetTimer(g_wnd, TIMER_TRAY_RETRY, 2000, NULL);
}

static void show_balloon(const WCHAR *text, HICON icon) {
  NOTIFYICONDATAW nid = { .cbSize = sizeof nid, .hWnd = g_wnd, .uID = TRAY_ID };
  nid.uFlags = NIF_INFO | NIF_REALTIME;
  nid.dwInfoFlags = NIIF_USER | NIIF_NOSOUND;
  nid.hBalloonIcon = icon;
  lstrcpynW(nid.szInfoTitle, APP_TITLE, ARRAYSIZE(nid.szInfoTitle));
  lstrcpynW(nid.szInfo, text, ARRAYSIZE(nid.szInfo));
  Shell_NotifyIconW(NIM_MODIFY, &nid);
}

static void settings_changed(void) {
  load_settings();
  update_tray(NIM_MODIFY);
}

// Горячая клавиша Alt+Shift+U — быстро включить/выключить замену (как в расширении).
static void toggle_enabled(BOOL notify) {
  AcquireSRWLockShared(&g_lock);
  BOOL enabled = !g_settings.enabled, locked = g_settings.enabledLocked;
  ReleaseSRWLockShared(&g_lock);

  if (locked) {
    if (notify) show_balloon(L"Bu sozlamani administrator belgilagan", g_iconOn);
    return;
  }
  save_flag(L"Enabled", enabled);
  settings_changed();
  if (notify) {
    show_balloon(enabled ? L"Avtomatik almashtirish yoqildi" : L"Avtomatik almashtirish o'chirildi",
                 enabled ? g_iconOn : g_iconOff);
  }
}

static void show_menu(int x, int y) {
  static Settings s;
  WCHAR app[APP_LEN], label[128];
  AcquireSRWLockShared(&g_lock);
  s = g_settings;
  wcscpy(app, g_lastApp);
  ReleaseSRWLockShared(&g_lock);

  HMENU menu = CreatePopupMenu();
  AppendMenuW(menu, MF_STRING | MF_GRAYED, 0, APP_TITLE L" " TEXT(APP_VERSION));
  AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
  AppendMenuW(menu, MF_STRING | (s.enabled ? MF_CHECKED : 0) | (s.enabledLocked ? MF_GRAYED : 0), CMD_ENABLED,
              L"Avtomatik almashtirish\tAlt+Shift+U");
  AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
  for (int i = 0; i < UZ_RULE_COUNT; i++) {
    swprintf(label, ARRAYSIZE(label), L"%ls  →  %lc", UZ_RULES[i].from, UZ_RULES[i].lower);
    AppendMenuW(menu, MF_STRING | (s.rules[i] ? MF_CHECKED : 0) | (s.rulesLocked[i] ? MF_GRAYED : 0),
                CMD_RULE + i, label);
  }

  BOOL appByPolicy = FALSE, appByUser = FALSE;
  if (app[0]) {
    appByPolicy = in_list(s.policyApps, s.policyAppCount, app);
    appByUser = in_list(s.userApps, s.userAppCount, app);
    swprintf(label, ARRAYSIZE(label), L"Shu dasturda ishlaydi (%ls)", app);
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING | (appByPolicy || appByUser ? 0 : MF_CHECKED) | (appByPolicy ? MF_GRAYED : 0),
                CMD_APP, label);
  }
  AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
  AppendMenuW(menu, MF_STRING, CMD_EXIT, L"Chiqish");

  // Без SetForegroundWindow меню не закроется по клику мимо.
  SetForegroundWindow(g_wnd);
  UINT align = GetSystemMetrics(SM_MENUDROPALIGNMENT) ? TPM_RIGHTALIGN : TPM_LEFTALIGN;
  int cmd = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY | align, x, y, g_wnd, NULL);
  PostMessageW(g_wnd, WM_NULL, 0, 0);
  DestroyMenu(menu);

  if (cmd == CMD_ENABLED) {
    toggle_enabled(FALSE);
  } else if (cmd >= CMD_RULE && cmd < CMD_RULE + UZ_RULE_COUNT) {
    save_flag(RULE_VALUES[cmd - CMD_RULE], !s.rules[cmd - CMD_RULE]);
    settings_changed();
  } else if (cmd == CMD_APP) {
    save_app_excluded(app, !appByUser);
    settings_changed();
  } else if (cmd == CMD_EXIT) {
    DestroyWindow(g_wnd);
  }
}

static LRESULT CALLBACK wnd_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
  case WM_TRAY:
    switch (LOWORD(lp)) {
    case WM_CONTEXTMENU:
    case NIN_SELECT:
    case NIN_KEYSELECT:
      show_menu(GET_X_LPARAM(wp), GET_Y_LPARAM(wp));
    }
    return 0;
  case WM_HOTKEY:
    if (wp == HOTKEY_TOGGLE) toggle_enabled(TRUE);
    return 0;
  case WM_TIMER:
    if (wp == TIMER_TRAY_RETRY && update_tray(NIM_ADD)) KillTimer(wnd, TIMER_TRAY_RETRY);
    return 0;
  case WM_SETTINGCHANGE:
    // После обновления групповых политик Windows рассылает WM_SETTINGCHANGE с "Policy".
    if (lp && wcscmp((const WCHAR *)lp, L"Policy") == 0) settings_changed();
    return 0;
  case WM_HOOK_FAILED:
    MessageBoxW(NULL, L"Klaviatura bilan ishlashni boshlab bo'lmadi.", APP_TITLE, MB_ICONERROR);
    DestroyWindow(wnd);
    return 0;
  case WM_QUERYENDSESSION:
    return TRUE;
  case WM_ENDSESSION:
    // Выход из системы или Restart Manager (обновление через MSI) просит закрыться.
    if (wp) DestroyWindow(wnd);
    return 0;
  case WM_DESTROY: {
    NOTIFYICONDATAW nid = { .cbSize = sizeof nid, .hWnd = wnd, .uID = TRAY_ID };
    Shell_NotifyIconW(NIM_DELETE, &nid);
    PostQuitMessage(0);
    return 0;
  }
  }
  // Explorer перезапустился — значок в трее нужно добавить заново.
  if (msg == g_taskbarCreated && msg) {
    add_tray();
    return 0;
  }
  return DefWindowProcW(wnd, msg, wp, lp);
}

static HICON load_icon(int id) {
  int size = GetSystemMetrics(SM_CXSMICON);
  return (HICON)LoadImageW(g_inst, MAKEINTRESOURCEW(id), IMAGE_ICON, size, size, LR_DEFAULTCOLOR);
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, PWSTR cmdLine, int show) {
  (void)prev; (void)cmdLine; (void)show;

  // Один экземпляр на сеанс пользователя.
  CreateMutexW(NULL, FALSE, L"Local\\UzbekHarflari");
  if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;

  g_inst = inst;
  g_ownIntegrity = integrity_level(GetCurrentProcess());
  g_capsLock = (GetKeyState(VK_CAPITAL) & 1) != 0;
  uz_init(&g_engine);
  load_settings();
  g_iconOn = load_icon(IDI_APP);
  g_iconOff = load_icon(IDI_APP_OFF);

  WNDCLASSW wc = { .lpfnWndProc = wnd_proc, .hInstance = inst, .lpszClassName = L"UzbekHarflari" };
  wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(IDI_APP));
  RegisterClassW(&wc);
  g_taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
  // Обычное (не message-only) окно: Restart Manager при обновлении шлёт WM_ENDSESSION окнам верхнего уровня.
  g_wnd = CreateWindowExW(WS_EX_TOOLWINDOW, wc.lpszClassName, APP_TITLE, WS_POPUP, 0, 0, 0, 0, NULL, NULL, inst, NULL);
  if (!g_wnd) return 1;
  ChangeWindowMessageFilterEx(g_wnd, g_taskbarCreated, MSGFLT_ALLOW, NULL);

  add_tray();
  RegisterHotKey(g_wnd, HOTKEY_TOGGLE, MOD_ALT | MOD_SHIFT | MOD_NOREPEAT, 'U');

  DWORD hookId, a11yId;
  HANDLE threads[2] = {
    CreateThread(NULL, 0, hook_thread, NULL, 0, &hookId),
    CreateThread(NULL, 0, a11y_thread, NULL, 0, &a11yId),
  };

  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  PostThreadMessageW(hookId, WM_QUIT, 0, 0);
  PostThreadMessageW(a11yId, WM_QUIT, 0, 0);
  WaitForMultipleObjects(ARRAYSIZE(threads), threads, TRUE, 2000);
  return 0;
}
