#define UNICODE
#define _UNICODE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <mmdeviceapi.h>
#include <propsys.h>
#include <functiondiscoverykeys_devpkey.h>
#include <strsafe.h>
#include <vector>
#include <string>
#include <algorithm>
#include "ChordState.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

struct AudioDeviceInfo {
    wchar_t id[512];
    wchar_t friendlyName[256];
    wchar_t desc[128];
    wchar_t interfaceName[128];
};

struct TargetDeviceConfig {
    wchar_t friendlyName[256];
    wchar_t desc[128];
    wchar_t interfaceName[128];
    wchar_t fallbackId[512];
};

enum SettingsControlId {
    IDC_LBL_BANNER = 101,
    IDC_LBL_DEV1 = 102,
    IDC_CB_DEV1 = 103,
    IDC_BTN_TEST1 = 104,
    IDC_INFO_DEV1 = 105,
    IDC_LBL_DEV2 = 106,
    IDC_CB_DEV2 = 107,
    IDC_BTN_TEST2 = 108,
    IDC_INFO_DEV2 = 109,
    IDC_LBL_HOTKEY = 110,
    IDC_LBL_STATUS = 111,
    IDC_BTN_REFRESH = 112,
    IDC_BTN_SAVE = 113,
    IDC_BTN_CANCEL = 114
};

static wchar_t ini[MAX_PATH], logpath[MAX_PATH], iconPath[MAX_PATH], exePath[MAX_PATH];
static TargetDeviceConfig targets[2];
static AudioDeviceInfo currentActive[2];
static bool targetResolved[2] = {false, false};

static UINT key = VK_F13, mods = MOD_NOREPEAT, chordKey = 0, taskbarCreated = 0;
static HHOOK keyboardHook = nullptr;
static HWND listenerWindow = nullptr;
static HWND settingsWindow = nullptr;
static NOTIFYICONDATAW icon = {sizeof(icon)};
static const wchar_t* cls = L"AudioSwitchNativeWindow";
static const wchar_t* settingsCls = L"AudioSwitchSettingsWindow";
static bool showTray = true, trayVisible = false, ownIcon = false;
static ChordState chord;

static const wchar_t* startupKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t* startupName = L"AudioSwitch-DellHeadphones";

static std::vector<AudioDeviceInfo> s_dlgDevices;
static HFONT s_dlgFont = nullptr;

static bool EqualsNoCase(const wchar_t* a, const wchar_t* b) {
    if (!a || !b) return false;
    return _wcsicmp(a, b) == 0;
}

static bool ContainsNoCase(const wchar_t* haystack, const wchar_t* needle) {
    if (!haystack || !needle || !needle[0]) return false;
    std::wstring h(haystack), n(needle);
    std::transform(h.begin(), h.end(), h.begin(), ::towlower);
    std::transform(n.begin(), n.end(), n.begin(), ::towlower);
    return h.find(n) != std::wstring::npos;
}

static void StartupCommand(wchar_t* out, size_t length) {
    StringCchPrintfW(out, length, L"\"%s\" --background", exePath);
}

static bool StartupEnabled() {
    wchar_t actual[1024] = {}, expected[1024];
    DWORD size = sizeof(actual);
    StartupCommand(expected, 1024);
    return RegGetValueW(HKEY_CURRENT_USER, startupKey, startupName, RRF_RT_REG_SZ, nullptr, actual, &size) == ERROR_SUCCESS && !wcscmp(actual, expected);
}

static bool SetStartup(bool enable) {
    HKEY k;
    LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, startupKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &k, nullptr);
    if (result != ERROR_SUCCESS) return false;
    if (enable) {
        wchar_t value[1024];
        StartupCommand(value, 1024);
        result = RegSetValueExW(k, startupName, 0, REG_SZ, (BYTE*)value, (DWORD)((wcslen(value) + 1) * sizeof(wchar_t)));
    } else {
        result = RegDeleteValueW(k, startupName);
        if (result == ERROR_FILE_NOT_FOUND) result = ERROR_SUCCESS;
    }
    RegCloseKey(k);
    return result == ERROR_SUCCESS;
}

static void SaveState() {
    wchar_t path[MAX_PATH];
    StringCchCopyW(path, MAX_PATH, ini);
    wchar_t* slash = wcsrchr(path, L'\\');
    if (slash) StringCchCopyW(slash + 1, MAX_PATH - (slash + 1 - path), L"ui-state.ini");
    WritePrivateProfileStringW(L"State", L"Running", L"1", path);
    WritePrivateProfileStringW(L"State", L"TrayVisible", trayVisible ? L"1" : L"0", path);
    WritePrivateProfileStringW(L"State", L"ShowTrayPreference", showTray ? L"1" : L"0", path);
    WritePrivateProfileStringW(L"State", L"AutoStart", StartupEnabled() ? L"1" : L"0", path);
}

static void Log(const wchar_t* text) {
    SYSTEMTIME t;
    GetLocalTime(&t);
    wchar_t line[2048];
    StringCchPrintfW(line, 2048, L"%04u-%02u-%02u %02u:%02u:%02u %s\r\n", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, text);
    char utf8[8192];
    int n = WideCharToMultiByte(CP_UTF8, 0, line, -1, utf8, sizeof(utf8), nullptr, nullptr);
    HANDLE f = CreateFileW(logpath, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f != INVALID_HANDLE_VALUE) {
        DWORD done;
        WriteFile(f, utf8, n - 1, &done, nullptr);
        CloseHandle(f);
    }
}

static std::vector<AudioDeviceInfo> EnumActiveEndpoints(IMMDeviceEnumerator* e) {
    std::vector<AudioDeviceInfo> list;
    if (!e) return list;
    IMMDeviceCollection* col = nullptr;
    if (FAILED(e->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &col))) return list;

    UINT count = 0;
    col->GetCount(&count);
    for (UINT i = 0; i < count; i++) {
        IMMDevice* dev = nullptr;
        if (SUCCEEDED(col->Item(i, &dev))) {
            AudioDeviceInfo info = {};
            LPWSTR devId = nullptr;
            if (SUCCEEDED(dev->GetId(&devId))) {
                StringCchCopyW(info.id, ARRAYSIZE(info.id), devId);
                CoTaskMemFree(devId);
            }

            IPropertyStore* props = nullptr;
            if (SUCCEEDED(dev->OpenPropertyStore(STGM_READ, &props))) {
                PROPVARIANT pv;
                PropVariantInit(&pv);

                if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &pv)) && pv.vt == VT_LPWSTR && pv.pwszVal) {
                    StringCchCopyW(info.friendlyName, ARRAYSIZE(info.friendlyName), pv.pwszVal);
                }
                PropVariantClear(&pv);

                if (SUCCEEDED(props->GetValue(PKEY_Device_DeviceDesc, &pv)) && pv.vt == VT_LPWSTR && pv.pwszVal) {
                    StringCchCopyW(info.desc, ARRAYSIZE(info.desc), pv.pwszVal);
                }
                PropVariantClear(&pv);

                if (SUCCEEDED(props->GetValue(PKEY_DeviceInterface_FriendlyName, &pv)) && pv.vt == VT_LPWSTR && pv.pwszVal) {
                    StringCchCopyW(info.interfaceName, ARRAYSIZE(info.interfaceName), pv.pwszVal);
                }
                PropVariantClear(&pv);

                props->Release();
            }
            dev->Release();
            list.push_back(info);
        }
    }
    col->Release();
    return list;
}

static int CalculateMatchScore(const AudioDeviceInfo& dev, const TargetDeviceConfig& target) {
    int score = 0;
    if (target.friendlyName[0] && EqualsNoCase(dev.friendlyName, target.friendlyName)) {
        score += 100;
    }
    if (target.desc[0] && EqualsNoCase(dev.desc, target.desc)) {
        score += 50;
    }
    if (target.interfaceName[0] && EqualsNoCase(dev.interfaceName, target.interfaceName)) {
        score += 30;
    }
    if (target.friendlyName[0] && (ContainsNoCase(dev.friendlyName, target.friendlyName) || ContainsNoCase(target.friendlyName, dev.friendlyName))) {
        score += 25;
    }
    if (target.desc[0] && (ContainsNoCase(dev.desc, target.desc) || ContainsNoCase(target.desc, dev.desc))) {
        score += 15;
    }
    if (target.fallbackId[0] && EqualsNoCase(dev.id, target.fallbackId)) {
        score += 10;
    }
    return score;
}

static bool ResolveTarget(const TargetDeviceConfig& target, const std::vector<AudioDeviceInfo>& activeDevs, AudioDeviceInfo* outResolved) {
    int bestScore = 0;
    const AudioDeviceInfo* bestDev = nullptr;
    for (const auto& dev : activeDevs) {
        int score = CalculateMatchScore(dev, target);
        if (score > bestScore) {
            bestScore = score;
            bestDev = &dev;
        }
    }
    if (bestScore > 0 && bestDev) {
        if (outResolved) *outResolved = *bestDev;
        return true;
    }
    return false;
}

static bool ResolveAllTargets(IMMDeviceEnumerator* e) {
    auto devs = EnumActiveEndpoints(e);
    targetResolved[0] = ResolveTarget(targets[0], devs, &currentActive[0]);
    targetResolved[1] = ResolveTarget(targets[1], devs, &currentActive[1]);
    return targetResolved[0] || targetResolved[1];
}

static void EnsureUnicodeIni(const wchar_t* path) {
    HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        HANDLE hNew = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hNew != INVALID_HANDLE_VALUE) {
            BYTE bom[2] = {0xFF, 0xFE};
            DWORD written = 0;
            WriteFile(hNew, bom, 2, &written, nullptr);
            CloseHandle(hNew);
        }
        return;
    }

    LARGE_INTEGER sz = {};
    GetFileSizeEx(hFile, &sz);
    if (sz.QuadPart == 0) {
        CloseHandle(hFile);
        HANDLE hNew = CreateFileW(path, GENERIC_WRITE, 0, nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hNew != INVALID_HANDLE_VALUE) {
            BYTE bom[2] = {0xFF, 0xFE};
            DWORD written = 0;
            WriteFile(hNew, bom, 2, &written, nullptr);
            CloseHandle(hNew);
        }
        return;
    }

    if (sz.QuadPart > 1024 * 1024) {
        CloseHandle(hFile);
        return;
    }

    std::vector<BYTE> buf((size_t)sz.QuadPart);
    DWORD readBytes = 0;
    ReadFile(hFile, buf.data(), (DWORD)buf.size(), &readBytes, nullptr);
    CloseHandle(hFile);

    if (readBytes >= 2 && buf[0] == 0xFF && buf[1] == 0xFE) {
        return;
    }

    const char* pData = (const char*)buf.data();
    int dataLen = (int)readBytes;
    if (readBytes >= 3 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF) {
        pData += 3;
        dataLen -= 3;
    }

    int wideLen = MultiByteToWideChar(CP_UTF8, 0, pData, dataLen, nullptr, 0);
    if (wideLen > 0) {
        std::vector<wchar_t> wideBuf(wideLen);
        MultiByteToWideChar(CP_UTF8, 0, pData, dataLen, wideBuf.data(), wideLen);

        HANDLE hWrite = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hWrite != INVALID_HANDLE_VALUE) {
            BYTE bom[2] = {0xFF, 0xFE};
            DWORD written = 0;
            WriteFile(hWrite, bom, 2, &written, nullptr);
            WriteFile(hWrite, wideBuf.data(), (DWORD)(wideLen * sizeof(wchar_t)), &written, nullptr);
            CloseHandle(hWrite);
        }
    }
}

static void LoadConfig() {
    EnsureUnicodeIni(ini);
    GetPrivateProfileStringW(L"Devices", L"Device1Friendly", L"", targets[0].friendlyName, ARRAYSIZE(targets[0].friendlyName), ini);
    GetPrivateProfileStringW(L"Devices", L"Device1Desc", L"", targets[0].desc, ARRAYSIZE(targets[0].desc), ini);
    GetPrivateProfileStringW(L"Devices", L"Device1Interface", L"", targets[0].interfaceName, ARRAYSIZE(targets[0].interfaceName), ini);
    GetPrivateProfileStringW(L"Devices", L"DellId", L"", targets[0].fallbackId, ARRAYSIZE(targets[0].fallbackId), ini);

    GetPrivateProfileStringW(L"Devices", L"Device2Friendly", L"", targets[1].friendlyName, ARRAYSIZE(targets[1].friendlyName), ini);
    GetPrivateProfileStringW(L"Devices", L"Device2Desc", L"", targets[1].desc, ARRAYSIZE(targets[1].desc), ini);
    GetPrivateProfileStringW(L"Devices", L"Device2Interface", L"", targets[1].interfaceName, ARRAYSIZE(targets[1].interfaceName), ini);
    GetPrivateProfileStringW(L"Devices", L"HeadphonesId", L"", targets[1].fallbackId, ARRAYSIZE(targets[1].fallbackId), ini);

    if (!targets[0].friendlyName[0] && !targets[0].desc[0] && targets[0].fallbackId[0]) {
        if (targets[0].fallbackId[0] != L'{') {
            StringCchCopyW(targets[0].desc, ARRAYSIZE(targets[0].desc), targets[0].fallbackId);
        }
    }
    if (!targets[1].friendlyName[0] && !targets[1].desc[0] && targets[1].fallbackId[0]) {
        if (targets[1].fallbackId[0] != L'{') {
            StringCchCopyW(targets[1].desc, ARRAYSIZE(targets[1].desc), targets[1].fallbackId);
        }
    }

    key = GetPrivateProfileIntW(L"Hotkey", L"VirtualKey", VK_F13, ini);
    mods = MOD_NOREPEAT | GetPrivateProfileIntW(L"Hotkey", L"Modifiers", 0, ini);
    chordKey = GetPrivateProfileIntW(L"Hotkey", L"ChordKey", 0, ini);
    showTray = GetPrivateProfileIntW(L"UI", L"ShowTray", 1, ini) != 0;
}

static void SaveConfig() {
    EnsureUnicodeIni(ini);
    WritePrivateProfileStringW(L"Devices", L"Device1Friendly", targets[0].friendlyName, ini);
    WritePrivateProfileStringW(L"Devices", L"Device1Desc", targets[0].desc, ini);
    WritePrivateProfileStringW(L"Devices", L"Device1Interface", targets[0].interfaceName, ini);
    if (currentActive[0].id[0]) WritePrivateProfileStringW(L"Devices", L"DellId", currentActive[0].id, ini);

    WritePrivateProfileStringW(L"Devices", L"Device2Friendly", targets[1].friendlyName, ini);
    WritePrivateProfileStringW(L"Devices", L"Device2Desc", targets[1].desc, ini);
    WritePrivateProfileStringW(L"Devices", L"Device2Interface", targets[1].interfaceName, ini);
    if (currentActive[1].id[0]) WritePrivateProfileStringW(L"Devices", L"HeadphonesId", currentActive[1].id, ini);
}

static HRESULT Default(IMMDeviceEnumerator* e, ERole role, LPWSTR* id) {
    IMMDevice* d = nullptr;
    HRESULT hr = e->GetDefaultAudioEndpoint(eRender, role, &d);
    if (SUCCEEDED(hr)) {
        hr = d->GetId(id);
        d->Release();
    }
    return hr;
}

static HRESULT Set(const wchar_t* id, ERole role) {
    const CLSID c = {0x870af99c, 0x171d, 0x4f9e, {0xaf, 0x0d, 0xe6, 0x3d, 0xf4, 0x0c, 0x2b, 0xc9}};
    const IID iid = {0xf8679f50, 0x850a, 0x41cf, {0x9c, 0x72, 0x43, 0x0f, 0x29, 0x02, 0x90, 0xc8}};
    IUnknown* p = nullptr;
    HRESULT hr = CoCreateInstance(c, nullptr, CLSCTX_INPROC_SERVER, iid, (void**)&p);
    if (SUCCEEDED(hr)) {
        using Fn = HRESULT(STDMETHODCALLTYPE*)(void*, LPCWSTR, ERole);
        auto fn = (Fn)(*(void***)p)[13];
        hr = fn(p, id, role);
        p->Release();
    }
    return hr;
}

static HRESULT Verify(IMMDeviceEnumerator* e, const wchar_t* id, ERole role) {
    for (int i = 0; i < 20; i++) {
        LPWSTR got = nullptr;
        HRESULT hr = Default(e, role, &got);
        bool match = SUCCEEDED(hr) && !wcscmp(got, id);
        CoTaskMemFree(got);
        if (match) return S_OK;
        Sleep(50);
    }
    return E_FAIL;
}

static HRESULT Restore(IMMDeviceEnumerator* e, LPWSTR* original) {
    HRESULT result = S_OK;
    for (int i = 0; i < 2; i++) {
        HRESULT hr = Set(original[i], (ERole)i);
        if (SUCCEEDED(hr)) hr = Verify(e, original[i], (ERole)i);
        if (FAILED(hr)) result = hr;
    }
    return result;
}

static HRESULT Select(IMMDeviceEnumerator* e, const wchar_t* id) {
    if (!id || !id[0]) return E_INVALIDARG;
    IMMDevice* d = nullptr;
    HRESULT hr = e->GetDevice(id, &d);
    if (FAILED(hr)) return hr;
    DWORD state = 0;
    hr = d->GetState(&state);
    d->Release();
    if (FAILED(hr) || state != DEVICE_STATE_ACTIVE) return HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);

    LPWSTR old[2] = {};
    hr = Default(e, eConsole, &old[0]);
    if (SUCCEEDED(hr)) hr = Default(e, eMultimedia, &old[1]);
    if (SUCCEEDED(hr)) {
        hr = Set(id, eConsole);
        if (SUCCEEDED(hr)) hr = Set(id, eMultimedia);
        if (SUCCEEDED(hr)) hr = Verify(e, id, eConsole);
        if (SUCCEEDED(hr)) hr = Verify(e, id, eMultimedia);
        if (FAILED(hr)) {
            HRESULT restore = Restore(e, old);
            if (FAILED(restore)) {
                Log(L"ERROR rollback failed; select output in Windows settings");
                hr = restore;
            }
        }
    }
    for (auto p : old) if (p) CoTaskMemFree(p);
    return hr;
}

static HRESULT RunAudio(bool test, int* selected, int forceTarget = -1) {
    IMMDeviceEnumerator* e = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&e));
    if (FAILED(hr)) return hr;

    ResolveAllTargets(e);

    if (test) {
        LPWSTR old[2] = {};
        hr = Default(e, eConsole, &old[0]);
        if (SUCCEEDED(hr)) hr = Default(e, eMultimedia, &old[1]);
        if (SUCCEEDED(hr)) {
            for (int i = 0; i < 2; i++) {
                WritePrivateProfileStringW(L"TestRestore", i ? L"Multimedia" : L"Console", old[i], ini);
            }
            for (int i = 0; i < 2; i++) {
                if (!targetResolved[i]) {
                    hr = HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);
                    break;
                }
                hr = Select(e, currentActive[i].id);
                if (FAILED(hr)) break;
                Log(currentActive[i].friendlyName);
                Sleep(400);
            }
            HRESULT back = Restore(e, old);
            if (FAILED(back)) hr = back;
            else Log(L"Original audio defaults restored");
        }
        for (auto p : old) if (p) CoTaskMemFree(p);
    } else if (forceTarget >= 0 && forceTarget < 2) {
        if (!targetResolved[forceTarget]) {
            hr = HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);
        } else {
            hr = Select(e, currentActive[forceTarget].id);
            if (SUCCEEDED(hr)) {
                if (selected) *selected = forceTarget;
                Log(currentActive[forceTarget].friendlyName);
            }
        }
    } else {
        LPWSTR current = nullptr;
        hr = Default(e, eMultimedia, &current);
        if (SUCCEEDED(hr)) {
            int target = -1;
            if (targetResolved[0] && !wcscmp(current, currentActive[0].id)) {
                target = 1;
            } else {
                target = 0;
            }

            if (!targetResolved[target] && targetResolved[1 - target]) {
                target = 1 - target;
            }

            if (target >= 0 && targetResolved[target]) {
                hr = Select(e, currentActive[target].id);
                if (SUCCEEDED(hr)) {
                    if (selected) *selected = target;
                    Log(currentActive[target].friendlyName);
                }
            } else {
                hr = HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);
            }
        }
        if (current) CoTaskMemFree(current);
    }
    e->Release();
    return hr;
}

static void Notify(const wchar_t* text, bool error) {
    if (!trayVisible) return;
    StringCchCopyW(icon.szInfoTitle, 64, error ? L"音频切换失败" : L"声音输出已切换");
    StringCchCopyW(icon.szInfo, 256, text);
    icon.uFlags = NIF_INFO;
    icon.dwInfoFlags = error ? NIIF_WARNING : NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &icon);
}

static void Toggle() {
    int selected = 0;
    HRESULT hr = RunAudio(false, &selected);
    if (SUCCEEDED(hr)) {
        const wchar_t* name = currentActive[selected].friendlyName[0] ? currentActive[selected].friendlyName :
                              (currentActive[selected].desc[0] ? currentActive[selected].desc : L"目标设备");
        Notify(name, false);
    } else {
        wchar_t text[256];
        if (hr == HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED)) {
            StringCchPrintfW(text, ARRAYSIZE(text), L"目标设备未连接或未就绪。请右键托盘打开【设备配置】检查。");
        } else {
            StringCchPrintfW(text, ARRAYSIZE(text), L"切换失败（错误码 0x%08X）。", (unsigned)hr);
        }
        Log(text);
        Notify(text, true);
    }
}

static void AddTray(HWND h) {
    if (!showTray || trayVisible) return;
    icon.hWnd = h;
    icon.uID = 1;
    icon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    icon.uCallbackMessage = WM_APP + 1;
    if (!icon.hIcon) {
        icon.hIcon = (HICON)LoadImageW(nullptr, iconPath, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_LOADFROMFILE);
        ownIcon = icon.hIcon != nullptr;
        if (!icon.hIcon) icon.hIcon = LoadIconW(nullptr, IDI_INFORMATION);
    }
    StringCchCopyW(icon.szTip, 128, L"音频切换 · 右键设置");
    trayVisible = Shell_NotifyIconW(NIM_ADD, &icon) != FALSE;
}

static bool SetTray(HWND h, bool visible) {
    if (!WritePrivateProfileStringW(L"UI", L"ShowTray", visible ? L"1" : L"0", ini)) return false;
    showTray = visible;
    if (visible) AddTray(h);
    else {
        Shell_NotifyIconW(NIM_DELETE, &icon);
        trayVisible = false;
    }
    return true;
}

static LRESULT CALLBACK KeyboardProc(int code, WPARAM w, LPARAM l) {
    if (code == HC_ACTION) {
        auto event = (KBDLLHOOKSTRUCT*)l;
        bool down = w == WM_KEYDOWN || w == WM_SYSKEYDOWN;
        bool up = w == WM_KEYUP || w == WM_SYSKEYUP;
        if (down || up) {
            auto result = chord.Process(event->vkCode == chordKey, event->vkCode == key, down);
            if (result.trigger) PostMessageW(listenerWindow, WM_APP + 3, 0, 0);
            if (result.suppress) return 1;
        }
    }
    return CallNextHookEx(keyboardHook, code, w, l);
}

static void UpdateComboDetails(HWND hwndDlg, int comboId, int infoId) {
    HWND hCombo = GetDlgItem(hwndDlg, comboId);
    HWND hInfo = GetDlgItem(hwndDlg, infoId);
    if (!hCombo || !hInfo) return;
    int idx = (int)SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
    if (idx >= 0 && idx < (int)s_dlgDevices.size()) {
        const auto& d = s_dlgDevices[idx];
        wchar_t buf[512];
        StringCchPrintfW(buf, ARRAYSIZE(buf), L"硬件描述: %s  |  驱动接口: %s",
            d.desc[0] ? d.desc : L"未知",
            d.interfaceName[0] ? d.interfaceName : L"默认接口");
        SetWindowTextW(hInfo, buf);
    } else {
        SetWindowTextW(hInfo, L"未选择设备");
    }
}

static void PopulateDeviceCombos(HWND hwndDlg) {
    IMMDeviceEnumerator* e = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&e)))) {
        return;
    }
    s_dlgDevices = EnumActiveEndpoints(e);
    e->Release();

    HWND hCb1 = GetDlgItem(hwndDlg, IDC_CB_DEV1);
    HWND hCb2 = GetDlgItem(hwndDlg, IDC_CB_DEV2);
    SendMessageW(hCb1, CB_RESETCONTENT, 0, 0);
    SendMessageW(hCb2, CB_RESETCONTENT, 0, 0);

    for (const auto& dev : s_dlgDevices) {
        const wchar_t* title = dev.friendlyName[0] ? dev.friendlyName : (dev.desc[0] ? dev.desc : dev.id);
        SendMessageW(hCb1, CB_ADDSTRING, 0, (LPARAM)title);
        SendMessageW(hCb2, CB_ADDSTRING, 0, (LPARAM)title);
    }

    int sel1 = 0, sel2 = 0;
    int bestScore1 = 0, bestScore2 = 0;
    for (size_t i = 0; i < s_dlgDevices.size(); i++) {
        int s1 = CalculateMatchScore(s_dlgDevices[i], targets[0]);
        if (s1 > bestScore1) { bestScore1 = s1; sel1 = (int)i; }
        int s2 = CalculateMatchScore(s_dlgDevices[i], targets[1]);
        if (s2 > bestScore2) { bestScore2 = s2; sel2 = (int)i; }
    }

    if (sel1 == sel2 && s_dlgDevices.size() > 1 && bestScore2 == 0) {
        sel2 = 1;
    }

    SendMessageW(hCb1, CB_SETCURSEL, sel1, 0);
    SendMessageW(hCb2, CB_SETCURSEL, sel2, 0);

    UpdateComboDetails(hwndDlg, IDC_CB_DEV1, IDC_INFO_DEV1);
    UpdateComboDetails(hwndDlg, IDC_CB_DEV2, IDC_INFO_DEV2);

    HWND hStatus = GetDlgItem(hwndDlg, IDC_LBL_STATUS);
    wchar_t st[128];
    StringCchPrintfW(st, ARRAYSIZE(st), L"已检测到 %u 个活跃音频播放设备。", (unsigned)s_dlgDevices.size());
    SetWindowTextW(hStatus, st);
}

static LRESULT CALLBACK SettingsWndProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_CREATE:
        return 0;

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)w;
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }

    case WM_COMMAND: {
        int wmId = LOWORD(w);
        int wmEvent = HIWORD(w);

        if (wmId == IDC_CB_DEV1 && wmEvent == CBN_SELCHANGE) {
            UpdateComboDetails(h, IDC_CB_DEV1, IDC_INFO_DEV1);
            return 0;
        }
        if (wmId == IDC_CB_DEV2 && wmEvent == CBN_SELCHANGE) {
            UpdateComboDetails(h, IDC_CB_DEV2, IDC_INFO_DEV2);
            return 0;
        }
        if (wmId == IDC_BTN_REFRESH) {
            PopulateDeviceCombos(h);
            return 0;
        }
        if (wmId == IDC_BTN_TEST1 || wmId == IDC_BTN_TEST2) {
            int comboId = (wmId == IDC_BTN_TEST1) ? IDC_CB_DEV1 : IDC_CB_DEV2;
            int idx = (int)SendDlgItemMessageW(h, comboId, CB_GETCURSEL, 0, 0);
            if (idx >= 0 && idx < (int)s_dlgDevices.size()) {
                IMMDeviceEnumerator* e = nullptr;
                HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&e));
                if (SUCCEEDED(hr)) {
                    hr = Select(e, s_dlgDevices[idx].id);
                    e->Release();
                }
                HWND hStatus = GetDlgItem(h, IDC_LBL_STATUS);
                wchar_t res[256];
                if (SUCCEEDED(hr)) {
                    StringCchPrintfW(res, ARRAYSIZE(res), L"✅ 已成功切换到：%s", s_dlgDevices[idx].friendlyName);
                } else {
                    StringCchPrintfW(res, ARRAYSIZE(res), L"❌ 切换失败（0x%08X）。", (unsigned)hr);
                }
                SetWindowTextW(hStatus, res);
            }
            return 0;
        }
        if (wmId == IDC_BTN_SAVE) {
            int idx1 = (int)SendDlgItemMessageW(h, IDC_CB_DEV1, CB_GETCURSEL, 0, 0);
            int idx2 = (int)SendDlgItemMessageW(h, IDC_CB_DEV2, CB_GETCURSEL, 0, 0);
            if (idx1 < 0 || idx2 < 0 || idx1 >= (int)s_dlgDevices.size() || idx2 >= (int)s_dlgDevices.size()) {
                MessageBoxW(h, L"请先在下拉列表中分别选择两个设备。", L"提示", MB_ICONWARNING);
                return 0;
            }
            if (idx1 == idx2) {
                MessageBoxW(h, L"两个切换目标不能是同一个设备，请分别选择不同的设备。", L"提示", MB_ICONWARNING);
                return 0;
            }

            const auto& d1 = s_dlgDevices[idx1];
            StringCchCopyW(targets[0].friendlyName, ARRAYSIZE(targets[0].friendlyName), d1.friendlyName);
            StringCchCopyW(targets[0].desc, ARRAYSIZE(targets[0].desc), d1.desc);
            StringCchCopyW(targets[0].interfaceName, ARRAYSIZE(targets[0].interfaceName), d1.interfaceName);
            StringCchCopyW(targets[0].fallbackId, ARRAYSIZE(targets[0].fallbackId), d1.id);
            currentActive[0] = d1;
            targetResolved[0] = true;

            const auto& d2 = s_dlgDevices[idx2];
            StringCchCopyW(targets[1].friendlyName, ARRAYSIZE(targets[1].friendlyName), d2.friendlyName);
            StringCchCopyW(targets[1].desc, ARRAYSIZE(targets[1].desc), d2.desc);
            StringCchCopyW(targets[1].interfaceName, ARRAYSIZE(targets[1].interfaceName), d2.interfaceName);
            StringCchCopyW(targets[1].fallbackId, ARRAYSIZE(targets[1].fallbackId), d2.id);
            currentActive[1] = d2;
            targetResolved[1] = true;

            SaveConfig();

            MessageBoxW(h, L"配置已保存！\n程序将在每次切换时根据友好名称与硬件描述动态匹配活跃端点，开关机后自动识别，无需手动修改 ID。", L"AudioSwitch", MB_ICONINFORMATION);
            DestroyWindow(h);
            return 0;
        }
        if (wmId == IDC_BTN_CANCEL || wmId == IDCANCEL) {
            DestroyWindow(h);
            return 0;
        }
        break;
    }

    case WM_CLOSE:
        DestroyWindow(h);
        return 0;

    case WM_DESTROY:
        if (s_dlgFont) {
            DeleteObject(s_dlgFont);
            s_dlgFont = nullptr;
        }
        s_dlgDevices.clear();
        settingsWindow = nullptr;
        return 0;
    }
    return DefWindowProcW(h, msg, w, l);
}

static void ShowSettingsDialog(HWND parent) {
    if (settingsWindow && IsWindow(settingsWindow)) {
        ShowWindow(settingsWindow, SW_RESTORE);
        SetForegroundWindow(settingsWindow);
        return;
    }

    WNDCLASSW swc = {};
    swc.lpfnWndProc = SettingsWndProc;
    swc.hInstance = GetModuleHandleW(nullptr);
    swc.lpszClassName = settingsCls;
    swc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    swc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassW(&swc);

    int dpi = 96;
    HDC screenDc = GetDC(nullptr);
    if (screenDc) {
        dpi = GetDeviceCaps(screenDc, LOGPIXELSY);
        ReleaseDC(nullptr, screenDc);
    }
    auto Scale = [dpi](int v) -> int { return MulDiv(v, dpi, 96); };

    int dlgW = Scale(560);
    int dlgH = Scale(430);
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - dlgW) / 2;
    int posY = (screenH - dlgH) / 2;

    settingsWindow = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        settingsCls,
        L"AudioSwitch - 音频设备与切换设置",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, dlgW, dlgH,
        parent, nullptr, GetModuleHandleW(nullptr), nullptr
    );

    if (!settingsWindow) return;

    if (icon.hIcon) {
        SendMessageW(settingsWindow, WM_SETICON, ICON_BIG, (LPARAM)icon.hIcon);
        SendMessageW(settingsWindow, WM_SETICON, ICON_SMALL, (LPARAM)icon.hIcon);
    }

    NONCLIENTMETRICSW ncm = { sizeof(ncm) };
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    s_dlgFont = CreateFontIndirectW(&ncm.lfMessageFont);

    CreateWindowExW(0, L"STATIC",
        L"请选择要双向切换的两个音频输出设备。\n程序将在后台按【友好名称】与【硬件描述】动态解析端点 ID，电脑重启、休眠或插拔后自动跟踪识别，无需手动修改 INI。",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        Scale(20), Scale(12), Scale(505), Scale(42),
        settingsWindow, (HMENU)IDC_LBL_BANNER, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", L"目标设备 1（例如显示器扬声器）：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        Scale(20), Scale(64), Scale(380), Scale(18),
        settingsWindow, (HMENU)IDC_LBL_DEV1, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        Scale(20), Scale(85), Scale(395), Scale(200),
        settingsWindow, (HMENU)IDC_CB_DEV1, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"BUTTON", L"试切此设备",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        Scale(425), Scale(84), Scale(100), Scale(26),
        settingsWindow, (HMENU)IDC_BTN_TEST1, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
        Scale(20), Scale(114), Scale(505), Scale(18),
        settingsWindow, (HMENU)IDC_INFO_DEV1, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", L"目标设备 2（例如有线耳机）：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        Scale(20), Scale(140), Scale(380), Scale(18),
        settingsWindow, (HMENU)IDC_LBL_DEV2, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        Scale(20), Scale(161), Scale(395), Scale(200),
        settingsWindow, (HMENU)IDC_CB_DEV2, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"BUTTON", L"试切此设备",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        Scale(425), Scale(160), Scale(100), Scale(26),
        settingsWindow, (HMENU)IDC_BTN_TEST2, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
        Scale(20), Scale(190), Scale(505), Scale(18),
        settingsWindow, (HMENU)IDC_INFO_DEV2, GetModuleHandleW(nullptr), nullptr);

    wchar_t hkText[256];
    if (chordKey) {
        StringCchPrintfW(hkText, ARRAYSIZE(hkText), L"当前切换快捷键：双键组合 (前导键 VK:%u + 主键 VK:%u)", chordKey, key);
    } else {
        StringCchPrintfW(hkText, ARRAYSIZE(hkText), L"当前切换快捷键：VK 键码 %u (默认 F13=124)", key);
    }
    CreateWindowExW(0, L"STATIC", hkText,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        Scale(20), Scale(222), Scale(505), Scale(20),
        settingsWindow, (HMENU)IDC_LBL_HOTKEY, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", L"就绪",
        WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
        Scale(20), Scale(250), Scale(505), Scale(20),
        settingsWindow, (HMENU)IDC_LBL_STATUS, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"BUTTON", L"🔄 刷新设备列表",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        Scale(20), Scale(295), Scale(130), Scale(32),
        settingsWindow, (HMENU)IDC_BTN_REFRESH, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"BUTTON", L"💾 保存并生效",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        Scale(285), Scale(295), Scale(120), Scale(32),
        settingsWindow, (HMENU)IDC_BTN_SAVE, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"BUTTON", L"取消",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        Scale(415), Scale(295), Scale(110), Scale(32),
        settingsWindow, (HMENU)IDC_BTN_CANCEL, GetModuleHandleW(nullptr), nullptr);

    if (s_dlgFont) {
        EnumChildWindows(settingsWindow, [](HWND hChild, LPARAM lParam) -> BOOL {
            SendMessageW(hChild, WM_SETFONT, lParam, TRUE);
            return TRUE;
        }, (LPARAM)s_dlgFont);
    }

    PopulateDeviceCombos(settingsWindow);
    ShowWindow(settingsWindow, SW_SHOW);
    UpdateWindow(settingsWindow);
}

static LRESULT CALLBACK WindowProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    if (msg == taskbarCreated) {
        trayVisible = false;
        AddTray(h);
        return 0;
    }
    switch (msg) {
    case WM_APP + 2:
        if (w == 1) return SetTray(h, true);
        if (w == 2) return SetTray(h, false);
        if (w == 3) { DestroyWindow(h); return 1; }
        if (w == 4) { SaveState(); return 1; }
        if (w == 5) return SetStartup(true);
        if (w == 6) return SetStartup(false);
        if (w == 7) { ShowSettingsDialog(h); return 1; }
        return 0;

    case WM_HOTKEY:
        if (w == 1) Toggle();
        return 0;

    case WM_APP + 3:
        Toggle();
        return 0;

    case WM_APP + 1:
        if (l == WM_LBUTTONDBLCLK) {
            Toggle();
        } else if (l == WM_RBUTTONUP || l == WM_CONTEXTMENU) {
            POINT p;
            GetCursorPos(&p);
            HMENU menu = CreatePopupMenu();

            wchar_t toggleText[128];
            const wchar_t* name0 = targets[0].desc[0] ? targets[0].desc : (targets[0].friendlyName[0] ? targets[0].friendlyName : L"设备1");
            const wchar_t* name1 = targets[1].desc[0] ? targets[1].desc : (targets[1].friendlyName[0] ? targets[1].friendlyName : L"设备2");
            StringCchPrintfW(toggleText, ARRAYSIZE(toggleText), L"切换：%s ↔ %s", name0, name1);

            AppendMenuW(menu, MF_STRING, 1, toggleText);
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(menu, MF_STRING, 10, L"音频设备配置(&S)...");
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(menu, MF_STRING | (StartupEnabled() ? MF_CHECKED : 0), 3, L"开机自启（当前用户登录时）");
            AppendMenuW(menu, MF_STRING, 4, L"隐藏托盘图标（快捷键继续工作）");
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(menu, MF_STRING, 2, L"退出程序（停止快捷键）");

            SetForegroundWindow(h);
            int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, p.x, p.y, 0, h, nullptr);
            DestroyMenu(menu);
            PostMessageW(h, WM_NULL, 0, 0);

            if (cmd == 1) Toggle();
            else if (cmd == 10) ShowSettingsDialog(h);
            else if (cmd == 2) DestroyWindow(h);
            else if (cmd == 3 && !SetStartup(!StartupEnabled())) Notify(L"无法更新开机自启设置。", true);
            else if (cmd == 4 && !SetTray(h, false)) Notify(L"无法保存托盘设置。", true);
        }
        return 0;

    case WM_CLOSE:
        DestroyWindow(h);
        return 0;

    case WM_DESTROY:
        if (keyboardHook) {
            UnhookWindowsHookEx(keyboardHook);
            keyboardHook = nullptr;
        }
        UnregisterHotKey(h, 1);
        Shell_NotifyIconW(NIM_DELETE, &icon);
        trayVisible = false;
        if (ownIcon && icon.hIcon) DestroyIcon(icon.hIcon);
        icon.hIcon = nullptr;
        if (settingsWindow && IsWindow(settingsWindow)) {
            DestroyWindow(settingsWindow);
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, msg, w, l);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int) {
    wchar_t base[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    StringCchCopyW(base, MAX_PATH, exePath);
    wchar_t* slash = wcsrchr(base, L'\\');
    if (!slash) return 1;
    slash[1] = 0;
    StringCchPrintfW(ini, MAX_PATH, L"%saudio-switch.ini", base);
    StringCchPrintfW(logpath, MAX_PATH, L"%saudio-native.log", base);
    StringCchPrintfW(iconPath, MAX_PATH, L"%sAudioSwitch.ico", base);

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return 1;
    const wchar_t* arg = argc >= 2 ? argv[1] : L"";
    bool background = !wcscmp(arg, L"--background");
    bool test = !wcscmp(arg, L"--self-test");
    bool single = !wcscmp(arg, L"--toggle");
    UINT command = !wcscmp(arg, L"--show-tray") ? 1 :
                   !wcscmp(arg, L"--hide-tray") ? 2 :
                   !wcscmp(arg, L"--exit") ? 3 :
                   !wcscmp(arg, L"--status") ? 4 :
                   !wcscmp(arg, L"--autostart-on") ? 5 :
                   !wcscmp(arg, L"--autostart-off") ? 6 :
                   (!wcscmp(arg, L"--settings") || !wcscmp(arg, L"--config")) ? 7 : 0;
    LocalFree(argv);

    if (command) {
        HWND running = FindWindowW(cls, nullptr);
        if (running) {
            DWORD_PTR result = 0;
            return SendMessageTimeoutW(running, WM_APP + 2, command, 0, SMTO_ABORTIFHUNG, 3000, &result) && result ? 0 : 1;
        }
        if (command == 5 || command == 6) return SetStartup(command == 5) ? 0 : 1;
        if (command == 3) return 0;
        if (command == 2) return WritePrivateProfileStringW(L"UI", L"ShowTray", L"0", ini) ? 0 : 1;
        if (command == 4) return 1;
        if (command == 7) {
            // 没有后台实例时直接打开设置界面
        } else {
            WritePrivateProfileStringW(L"UI", L"ShowTray", L"1", ini);
        }
    }

    LoadConfig();

    if (!background && !test && !single && command != 7) {
        showTray = true;
        WritePrivateProfileStringW(L"UI", L"ShowTray", L"1", ini);
    }

    if (chordKey && (chordKey > 254 || chordKey == key || mods != MOD_NOREPEAT)) {
        MessageBoxW(nullptr, L"双键组合配置无效", L"音频切换", MB_ICONERROR);
        return 1;
    }
    if (key < 1 || key > 254 || (mods & ~(MOD_NOREPEAT | MOD_ALT | MOD_CONTROL | MOD_SHIFT))) {
        MessageBoxW(nullptr, L"快捷键配置无效", L"音频切换", MB_ICONERROR);
        return 1;
    }

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return 1;

    // 尝试解析或自动发现配置
    IMMDeviceEnumerator* eProbe = nullptr;
    if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&eProbe)))) {
        auto devs = EnumActiveEndpoints(eProbe);
        // 如果未配置任何目标，且当前存在 Dell 或 耳机，自动匹配默认目标
        if (!targets[0].friendlyName[0] && !targets[0].desc[0] && !targets[0].fallbackId[0]) {
            for (const auto& d : devs) {
                if (ContainsNoCase(d.friendlyName, L"DELL") || ContainsNoCase(d.desc, L"DELL")) {
                    StringCchCopyW(targets[0].friendlyName, ARRAYSIZE(targets[0].friendlyName), d.friendlyName);
                    StringCchCopyW(targets[0].desc, ARRAYSIZE(targets[0].desc), d.desc);
                    StringCchCopyW(targets[0].interfaceName, ARRAYSIZE(targets[0].interfaceName), d.interfaceName);
                    break;
                }
            }
        }
        if (!targets[1].friendlyName[0] && !targets[1].desc[0] && !targets[1].fallbackId[0]) {
            for (const auto& d : devs) {
                if (ContainsNoCase(d.friendlyName, L"耳机") || ContainsNoCase(d.desc, L"耳机") || ContainsNoCase(d.friendlyName, L"Headphone")) {
                    StringCchCopyW(targets[1].friendlyName, ARRAYSIZE(targets[1].friendlyName), d.friendlyName);
                    StringCchCopyW(targets[1].desc, ARRAYSIZE(targets[1].desc), d.desc);
                    StringCchCopyW(targets[1].interfaceName, ARRAYSIZE(targets[1].interfaceName), d.interfaceName);
                    break;
                }
            }
        }
        ResolveAllTargets(eProbe);
        eProbe->Release();
    }

    if (test || single) {
        int selected = 0;
        hr = RunAudio(test, &selected);
        Log(SUCCEEDED(hr) ? L"PASS" : L"FAIL");
        CoUninitialize();
        return FAILED(hr) ? 1 : 0;
    }

    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"Local\\DellHeadphonesAudioSwitch");
    if (!mutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (!background) {
            HWND running = FindWindowW(cls, nullptr);
            if (running) {
                DWORD_PTR result;
                if (command == 7) {
                    SendMessageTimeoutW(running, WM_APP + 2, 7, 0, SMTO_ABORTIFHUNG, 3000, &result);
                } else {
                    SendMessageTimeoutW(running, WM_APP + 2, 1, 0, SMTO_ABORTIFHUNG, 3000, &result);
                }
            }
        }
        if (mutex) CloseHandle(mutex);
        CoUninitialize();
        return 0;
    }

    taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = cls;
    RegisterClassW(&wc);
    HWND h = CreateWindowExW(WS_EX_TOOLWINDOW, cls, L"AudioSwitch", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, instance, nullptr);

    listenerWindow = h;
    bool registered = false;
    if (h) {
        if (chordKey) {
            keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, instance, 0);
            registered = keyboardHook != nullptr;
        } else {
            registered = RegisterHotKey(h, 1, mods, key) != FALSE;
        }
    }

    if (!registered) {
        MessageBoxW(nullptr, L"无法注册快捷键，可能被其他程序占用。", L"音频切换", MB_ICONERROR);
        if (h) DestroyWindow(h);
        CloseHandle(mutex);
        CoUninitialize();
        return 1;
    }

    AddTray(h);
    Log(L"Native listener ready");

    if (command == 7 || (!targets[0].friendlyName[0] && !targets[0].desc[0] && !targets[0].fallbackId[0])) {
        ShowSettingsDialog(h);
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (settingsWindow && IsDialogMessageW(settingsWindow, &msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CloseHandle(mutex);
    CoUninitialize();
    return 0;
}
