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

struct KeyPreset {
    const wchar_t* name;
    UINT vk;
};

static const KeyPreset k_mainKeyPresets[] = {
    { L"F13 (键码 124 - 默认推荐)", VK_F13 },
    { L"F14 (键码 125)", VK_F14 },
    { L"F15 (键码 126)", VK_F15 },
    { L"F16 (键码 127)", VK_F16 },
    { L"F17 (键码 128)", VK_F17 },
    { L"F18 (键码 129)", VK_F18 },
    { L"F19 (键码 130)", VK_F19 },
    { L"F20 (键码 131)", VK_F20 },
    { L"F21 (键码 132)", VK_F21 },
    { L"F22 (键码 133)", VK_F22 },
    { L"F23 (键码 134)", VK_F23 },
    { L"F24 (键码 135)", VK_F24 },
    { L"Page Up (PgUp - 键码 33)", VK_PRIOR },
    { L"Page Down (PgDn - 键码 34)", VK_NEXT },
    { L"End (键码 35)", VK_END },
    { L"Home (键码 36)", VK_HOME },
    { L"Insert (键码 45)", VK_INSERT },
    { L"Delete (键码 46)", VK_DELETE },
    { L"Pause / Break (键码 19)", VK_PAUSE },
    { L"Scroll Lock (键码 145)", VK_SCROLL },
    { L"F1 (键码 112)", VK_F1 },
    { L"F2 (键码 113)", VK_F2 },
    { L"F3 (键码 114)", VK_F3 },
    { L"F4 (键码 115)", VK_F4 },
    { L"F5 (键码 116)", VK_F5 },
    { L"F6 (键码 117)", VK_F6 },
    { L"F7 (键码 118)", VK_F7 },
    { L"F8 (键码 119)", VK_F8 },
    { L"F9 (键码 120)", VK_F9 },
    { L"F10 (键码 121)", VK_F10 },
    { L"F11 (键码 122)", VK_F11 },
    { L"F12 (键码 123)", VK_F12 },
    { L"A", 'A' }, { L"B", 'B' }, { L"C", 'C' }, { L"D", 'D' },
    { L"E", 'E' }, { L"F", 'F' }, { L"G", 'G' }, { L"H", 'H' },
    { L"I", 'I' }, { L"J", 'J' }, { L"K", 'K' }, { L"L", 'L' },
    { L"M", 'M' }, { L"N", 'N' }, { L"O", 'O' }, { L"P", 'P' },
    { L"Q", 'Q' }, { L"R", 'R' }, { L"S", 'S' }, { L"T", 'T' },
    { L"U", 'U' }, { L"V", 'V' }, { L"W", 'W' }, { L"X", 'X' },
    { L"Y", 'Y' }, { L"Z", 'Z' },
    { L"1", '1' }, { L"2", '2' }, { L"3", '3' }, { L"4", '4' },
    { L"5", '5' }, { L"6", '6' }, { L"7", '7' }, { L"8", '8' },
    { L"9", '9' }, { L"0", '0' },
    { L"` (~)", VK_OEM_3 },
    { L"- (_)", VK_OEM_MINUS },
    { L"= (+)", VK_OEM_PLUS },
    { L"[", VK_OEM_4 }, { L"]", VK_OEM_6 }, { L"\\", VK_OEM_5 },
    { L";", VK_OEM_1 }, { L"'", VK_OEM_7 },
    { L",", VK_OEM_COMMA }, { L".", VK_OEM_PERIOD }, { L"/", VK_OEM_2 }
};

static const KeyPreset k_chordKeyPresets[] = {
    { L"End (键码 35 - 默认推荐)", VK_END },
    { L"Home (键码 36)", VK_HOME },
    { L"Page Up (PgUp - 键码 33)", VK_PRIOR },
    { L"Page Down (PgDn - 键码 34)", VK_NEXT },
    { L"Insert (键码 45)", VK_INSERT },
    { L"Delete (键码 46)", VK_DELETE },
    { L"Scroll Lock (键码 145)", VK_SCROLL },
    { L"Pause (键码 19)", VK_PAUSE },
    { L"F13 (键码 124)", VK_F13 },
    { L"F14 (键码 125)", VK_F14 },
    { L"F15 (键码 126)", VK_F15 }
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

    // Hotkey Controls
    IDC_LBL_HK_TITLE = 110,
    IDC_LBL_HK_MODE = 111,
    IDC_CB_HK_MODE = 112,
    IDC_LBL_MODIFIERS = 113,
    IDC_CHK_CTRL = 114,
    IDC_CHK_ALT = 115,
    IDC_CHK_SHIFT = 116,
    IDC_LBL_CHORD_KEY = 117,
    IDC_CB_CHORD_PRESET = 118,
    IDC_BTN_REC_CHORD = 119,
    IDC_LBL_CUR_CHORD = 120,
    IDC_LBL_MAIN_KEY = 121,
    IDC_CB_KEY_PRESET = 122,
    IDC_BTN_REC_KEY = 123,
    IDC_LBL_CUR_KEY = 124,
    IDC_LBL_HK_HINT = 125,

    // Bottom Controls
    IDC_LBL_STATUS = 126,
    IDC_BTN_REFRESH = 127,
    IDC_BTN_SAVE = 128,
    IDC_BTN_CANCEL = 129
};

enum RecordingTarget {
    REC_NONE = 0,
    REC_MAIN_KEY,
    REC_CHORD_KEY
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

static UINT s_dlgKey = VK_F13;
static UINT s_dlgMods = MOD_NOREPEAT;
static UINT s_dlgChordKey = 0;
static int s_dlgMode = 0;

static RecordingTarget s_recTarget = REC_NONE;
static HHOOK s_recHook = nullptr;
static HWND s_recDialog = nullptr;

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

static std::wstring FormatKeyDisplay(UINT vk) {
    if (vk == 0) return L"未设置";
    if (vk >= VK_F1 && vk <= VK_F24) {
        return L"F" + std::to_wstring(vk - VK_F1 + 1) + L" (" + std::to_wstring(vk) + L")";
    }
    switch (vk) {
    case VK_PRIOR: return L"PgUp (33)";
    case VK_NEXT:  return L"PgDn (34)";
    case VK_END:   return L"End (35)";
    case VK_HOME:  return L"Home (36)";
    case VK_INSERT:return L"Insert (45)";
    case VK_DELETE:return L"Delete (46)";
    case VK_PAUSE: return L"Pause (19)";
    case VK_SCROLL:return L"ScrollLock (145)";
    case VK_ESCAPE:return L"Esc (27)";
    case VK_SPACE: return L"Space (32)";
    case VK_TAB:   return L"Tab (9)";
    }
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) {
        return std::wstring(1, (wchar_t)vk) + L" (" + std::to_wstring(vk) + L")";
    }
    return L"VK: " + std::to_wstring(vk);
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

    wchar_t numBuf[32];
    StringCchPrintfW(numBuf, ARRAYSIZE(numBuf), L"%u", key);
    WritePrivateProfileStringW(L"Hotkey", L"VirtualKey", numBuf, ini);

    StringCchPrintfW(numBuf, ARRAYSIZE(numBuf), L"%u", (mods & ~MOD_NOREPEAT));
    WritePrivateProfileStringW(L"Hotkey", L"Modifiers", numBuf, ini);

    StringCchPrintfW(numBuf, ARRAYSIZE(numBuf), L"%u", chordKey);
    WritePrivateProfileStringW(L"Hotkey", L"ChordKey", numBuf, ini);
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
            StringCchPrintfW(text, ARRAYSIZE(text), L"目标设备未连接或未就绪。请右键托盘打开【快捷键与设备设置】检查。");
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

static bool ApplyHotkey(HWND hListener) {
    if (!hListener || !IsWindow(hListener)) return false;
    if (keyboardHook) {
        UnhookWindowsHookEx(keyboardHook);
        keyboardHook = nullptr;
    }
    UnregisterHotKey(hListener, 1);
    chord = ChordState{};

    if (chordKey) {
        keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandleW(nullptr), 0);
        return keyboardHook != nullptr;
    } else {
        return RegisterHotKey(hListener, 1, mods, key) != FALSE;
    }
}

static LRESULT CALLBACK RecordingHookProc(int code, WPARAM w, LPARAM l) {
    if (code == HC_ACTION && (w == WM_KEYDOWN || w == WM_SYSKEYDOWN)) {
        auto event = (KBDLLHOOKSTRUCT*)l;
        UINT vk = event->vkCode;

        if (vk == VK_ESCAPE) {
            PostMessageW(s_recDialog, WM_APP + 11, 0, 0);
            if (s_recHook) {
                UnhookWindowsHookEx(s_recHook);
                s_recHook = nullptr;
            }
            s_recTarget = REC_NONE;
            return 1;
        }

        if (s_recTarget != REC_NONE && s_recDialog && IsWindow(s_recDialog)) {
            if (vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_SHIFT) {
                if (s_recTarget == REC_MAIN_KEY) {
                    SendDlgItemMessageW(s_recDialog, IDC_CHK_SHIFT, BM_SETCHECK, BST_CHECKED, 0);
                }
                return 1;
            }
            if (vk == VK_LCONTROL || vk == VK_RCONTROL || vk == VK_CONTROL) {
                if (s_recTarget == REC_MAIN_KEY) {
                    SendDlgItemMessageW(s_recDialog, IDC_CHK_CTRL, BM_SETCHECK, BST_CHECKED, 0);
                }
                return 1;
            }
            if (vk == VK_LMENU || vk == VK_RMENU || vk == VK_MENU) {
                if (s_recTarget == REC_MAIN_KEY) {
                    SendDlgItemMessageW(s_recDialog, IDC_CHK_ALT, BM_SETCHECK, BST_CHECKED, 0);
                }
                return 1;
            }

            if (s_recTarget == REC_MAIN_KEY) {
                if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) {
                    SendDlgItemMessageW(s_recDialog, IDC_CHK_CTRL, BM_SETCHECK, BST_CHECKED, 0);
                }
                if ((GetKeyState(VK_MENU) & 0x8000) != 0) {
                    SendDlgItemMessageW(s_recDialog, IDC_CHK_ALT, BM_SETCHECK, BST_CHECKED, 0);
                }
                if ((GetKeyState(VK_SHIFT) & 0x8000) != 0) {
                    SendDlgItemMessageW(s_recDialog, IDC_CHK_SHIFT, BM_SETCHECK, BST_CHECKED, 0);
                }
            }

            PostMessageW(s_recDialog, WM_APP + 10, (WPARAM)s_recTarget, (LPARAM)vk);
            if (s_recHook) {
                UnhookWindowsHookEx(s_recHook);
                s_recHook = nullptr;
            }
            s_recTarget = REC_NONE;
            return 1;
        }
    }
    return CallNextHookEx(s_recHook, code, w, l);
}

static void StopRecording(HWND hDlg) {
    if (s_recHook) {
        UnhookWindowsHookEx(s_recHook);
        s_recHook = nullptr;
    }
    s_recTarget = REC_NONE;
    SetWindowTextW(GetDlgItem(hDlg, IDC_BTN_REC_KEY), (s_dlgMode == 1) ? L"🎯 录制触发键" : L"🎯 录制主键");
    SetWindowTextW(GetDlgItem(hDlg, IDC_BTN_REC_CHORD), L"🎯 录制前导键");
    SetWindowTextW(GetDlgItem(hDlg, IDC_LBL_STATUS), L"已取消录制。");
}

static void StartRecording(HWND hDlg, RecordingTarget target) {
    StopRecording(hDlg);
    s_recTarget = target;
    s_recDialog = hDlg;

    if (target == REC_MAIN_KEY) {
        SetWindowTextW(GetDlgItem(hDlg, IDC_BTN_REC_KEY), L"⏳ 请按键(Esc取消)");
    } else {
        SetWindowTextW(GetDlgItem(hDlg, IDC_BTN_REC_CHORD), L"⏳ 请按键(Esc取消)");
    }
    SetWindowTextW(GetDlgItem(hDlg, IDC_LBL_STATUS), L"正在录制按键：请在键盘上按下目标按键（按 Esc 取消）...");

    s_recHook = SetWindowsHookExW(WH_KEYBOARD_LL, RecordingHookProc, GetModuleHandleW(nullptr), 0);
}

static void UpdateHotkeyUIMode(HWND hDlg, int mode) {
    int showMod = (mode == 0) ? SW_SHOW : SW_HIDE;
    int showChord = (mode == 1) ? SW_SHOW : SW_HIDE;

    ShowWindow(GetDlgItem(hDlg, IDC_LBL_MODIFIERS), showMod);
    ShowWindow(GetDlgItem(hDlg, IDC_CHK_CTRL), showMod);
    ShowWindow(GetDlgItem(hDlg, IDC_CHK_ALT), showMod);
    ShowWindow(GetDlgItem(hDlg, IDC_CHK_SHIFT), showMod);

    ShowWindow(GetDlgItem(hDlg, IDC_LBL_CHORD_KEY), showChord);
    ShowWindow(GetDlgItem(hDlg, IDC_CB_CHORD_PRESET), showChord);
    ShowWindow(GetDlgItem(hDlg, IDC_BTN_REC_CHORD), showChord);
    ShowWindow(GetDlgItem(hDlg, IDC_LBL_CUR_CHORD), showChord);

    HWND hMainLbl = GetDlgItem(hDlg, IDC_LBL_MAIN_KEY);
    HWND hHintLbl = GetDlgItem(hDlg, IDC_LBL_HK_HINT);

    if (mode == 0) {
        SetWindowTextW(hMainLbl, L"主快捷键：");
        SetWindowTextW(GetDlgItem(hDlg, IDC_BTN_REC_KEY), L"🎯 录制主键");
        SetWindowTextW(hHintLbl, L"说明：可勾选 Ctrl/Alt/Shift 组合，客制化键盘推荐选 F13；点击【🎯 录制】可直接按键盘设置。");
    } else {
        SetWindowTextW(hMainLbl, L"触发键（按下）：");
        SetWindowTextW(GetDlgItem(hDlg, IDC_BTN_REC_KEY), L"🎯 录制触发键");
        SetWindowTextW(hHintLbl, L"说明：按住前导键再按触发键进行切换，触发键静默拦截；前导键保留原有光标移动/输入。");
    }
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

    case WM_APP + 10: {
        RecordingTarget target = (RecordingTarget)w;
        UINT vk = (UINT)l;
        if (target == REC_MAIN_KEY) {
            s_dlgKey = vk;
            SetWindowTextW(GetDlgItem(h, IDC_LBL_CUR_KEY), FormatKeyDisplay(s_dlgKey).c_str());
            SetWindowTextW(GetDlgItem(h, IDC_BTN_REC_KEY), (s_dlgMode == 1) ? L"🎯 录制触发键" : L"🎯 录制主键");
            for (int i = 0; i < ARRAYSIZE(k_mainKeyPresets); i++) {
                if (k_mainKeyPresets[i].vk == s_dlgKey) {
                    SendDlgItemMessageW(h, IDC_CB_KEY_PRESET, CB_SETCURSEL, i, 0);
                    break;
                }
            }
            SetWindowTextW(GetDlgItem(h, IDC_LBL_STATUS), L"按键录制成功！");
        } else if (target == REC_CHORD_KEY) {
            s_dlgChordKey = vk;
            SetWindowTextW(GetDlgItem(h, IDC_LBL_CUR_CHORD), FormatKeyDisplay(s_dlgChordKey).c_str());
            SetWindowTextW(GetDlgItem(h, IDC_BTN_REC_CHORD), L"🎯 录制前导键");
            for (int i = 0; i < ARRAYSIZE(k_chordKeyPresets); i++) {
                if (k_chordKeyPresets[i].vk == s_dlgChordKey) {
                    SendDlgItemMessageW(h, IDC_CB_CHORD_PRESET, CB_SETCURSEL, i, 0);
                    break;
                }
            }
            SetWindowTextW(GetDlgItem(h, IDC_LBL_STATUS), L"前导键录制成功！");
        }
        return 0;
    }

    case WM_APP + 11: {
        StopRecording(h);
        return 0;
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
        if (wmId == IDC_CB_HK_MODE && wmEvent == CBN_SELCHANGE) {
            int sel = (int)SendDlgItemMessageW(h, IDC_CB_HK_MODE, CB_GETCURSEL, 0, 0);
            if (sel >= 0) {
                s_dlgMode = sel;
                UpdateHotkeyUIMode(h, s_dlgMode);
            }
            return 0;
        }
        if (wmId == IDC_CB_KEY_PRESET && wmEvent == CBN_SELCHANGE) {
            int sel = (int)SendDlgItemMessageW(h, IDC_CB_KEY_PRESET, CB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < ARRAYSIZE(k_mainKeyPresets)) {
                s_dlgKey = k_mainKeyPresets[sel].vk;
                SetWindowTextW(GetDlgItem(h, IDC_LBL_CUR_KEY), FormatKeyDisplay(s_dlgKey).c_str());
            }
            return 0;
        }
        if (wmId == IDC_CB_CHORD_PRESET && wmEvent == CBN_SELCHANGE) {
            int sel = (int)SendDlgItemMessageW(h, IDC_CB_CHORD_PRESET, CB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < ARRAYSIZE(k_chordKeyPresets)) {
                s_dlgChordKey = k_chordKeyPresets[sel].vk;
                SetWindowTextW(GetDlgItem(h, IDC_LBL_CUR_CHORD), FormatKeyDisplay(s_dlgChordKey).c_str());
            }
            return 0;
        }
        if (wmId == IDC_BTN_REC_KEY) {
            if (s_recTarget == REC_MAIN_KEY) {
                StopRecording(h);
            } else {
                StartRecording(h, REC_MAIN_KEY);
            }
            return 0;
        }
        if (wmId == IDC_BTN_REC_CHORD) {
            if (s_recTarget == REC_CHORD_KEY) {
                StopRecording(h);
            } else {
                StartRecording(h, REC_CHORD_KEY);
            }
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

            UINT newKey = s_dlgKey;
            UINT newChordKey = (s_dlgMode == 1) ? s_dlgChordKey : 0;
            UINT newMods = MOD_NOREPEAT;

            if (s_dlgMode == 0) {
                if (IsDlgButtonChecked(h, IDC_CHK_CTRL) == BST_CHECKED) newMods |= MOD_CONTROL;
                if (IsDlgButtonChecked(h, IDC_CHK_ALT) == BST_CHECKED) newMods |= MOD_ALT;
                if (IsDlgButtonChecked(h, IDC_CHK_SHIFT) == BST_CHECKED) newMods |= MOD_SHIFT;
                if (newKey < 1 || newKey > 254) {
                    MessageBoxW(h, L"请选择或录制有效的主快捷键。", L"提示", MB_ICONWARNING);
                    return 0;
                }
            } else {
                if (newChordKey < 1 || newChordKey > 254 || newKey < 1 || newKey > 254) {
                    MessageBoxW(h, L"双键模式必须分别设置有效的前导键与触发键。", L"提示", MB_ICONWARNING);
                    return 0;
                }
                if (newChordKey == newKey) {
                    MessageBoxW(h, L"前导键与触发键不能相同，请选择不同的按键。", L"提示", MB_ICONWARNING);
                    return 0;
                }
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

            key = newKey;
            mods = newMods;
            chordKey = newChordKey;

            SaveConfig();

            bool hotkeyOk = ApplyHotkey(listenerWindow);
            if (!hotkeyOk) {
                MessageBoxW(h, L"设备配置已保存，但新快捷键注册失败！\n可能被其他软件占用，请尝试更换其他按键组合。", L"警告", MB_ICONWARNING);
            } else {
                MessageBoxW(h, L"配置已保存并即时生效！\n设备动态识别已更新，快捷键已就绪。", L"AudioSwitch", MB_ICONINFORMATION);
            }
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
        if (s_recHook) {
            UnhookWindowsHookEx(s_recHook);
            s_recHook = nullptr;
        }
        s_recTarget = REC_NONE;
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

    s_dlgKey = key;
    s_dlgMods = mods;
    s_dlgChordKey = chordKey;
    s_dlgMode = chordKey ? 1 : 0;

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

    int dlgW = Scale(580);
    int dlgH = Scale(460);
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - dlgW) / 2;
    int posY = (screenH - dlgH) / 2;

    settingsWindow = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        settingsCls,
        L"AudioSwitch - 音频设备与快捷键配置",
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
        L"选择要在快捷键下双向切换的两个音频输出设备及切换热键。\n系统按【友好名称】与【硬件描述】动态解析端点，开关机后自动识别，无需手动修改 INI。",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        Scale(20), Scale(10), Scale(535), Scale(36),
        settingsWindow, (HMENU)IDC_LBL_BANNER, GetModuleHandleW(nullptr), nullptr);

    // Device 1
    CreateWindowExW(0, L"STATIC", L"目标设备 1（例如显示器扬声器）：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        Scale(20), Scale(52), Scale(400), Scale(16),
        settingsWindow, (HMENU)IDC_LBL_DEV1, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        Scale(20), Scale(70), Scale(415), Scale(200),
        settingsWindow, (HMENU)IDC_CB_DEV1, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"BUTTON", L"试切此设备",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        Scale(445), Scale(69), Scale(110), Scale(25),
        settingsWindow, (HMENU)IDC_BTN_TEST1, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
        Scale(20), Scale(97), Scale(535), Scale(16),
        settingsWindow, (HMENU)IDC_INFO_DEV1, GetModuleHandleW(nullptr), nullptr);

    // Device 2
    CreateWindowExW(0, L"STATIC", L"目标设备 2（例如有线耳机）：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        Scale(20), Scale(118), Scale(400), Scale(16),
        settingsWindow, (HMENU)IDC_LBL_DEV2, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        Scale(20), Scale(136), Scale(415), Scale(200),
        settingsWindow, (HMENU)IDC_CB_DEV2, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"BUTTON", L"试切此设备",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        Scale(445), Scale(135), Scale(110), Scale(25),
        settingsWindow, (HMENU)IDC_BTN_TEST2, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
        Scale(20), Scale(163), Scale(535), Scale(16),
        settingsWindow, (HMENU)IDC_INFO_DEV2, GetModuleHandleW(nullptr), nullptr);

    // Hotkey Group Header
    CreateWindowExW(0, L"STATIC", L"── 快捷键设置 ──────────────────────────────────────────────────────────",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        Scale(20), Scale(186), Scale(535), Scale(16),
        settingsWindow, (HMENU)IDC_LBL_HK_TITLE, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", L"切换模式：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        Scale(20), Scale(208), Scale(75), Scale(18),
        settingsWindow, (HMENU)IDC_LBL_HK_MODE, GetModuleHandleW(nullptr), nullptr);

    HWND hCbMode = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        Scale(100), Scale(205), Scale(455), Scale(100),
        settingsWindow, (HMENU)IDC_CB_HK_MODE, GetModuleHandleW(nullptr), nullptr);
    SendMessageW(hCbMode, CB_ADDSTRING, 0, (LPARAM)L"单键 / 修饰键模式 (RegisterHotKey，推荐 F13 或 Ctrl/Alt 组合)");
    SendMessageW(hCbMode, CB_ADDSTRING, 0, (LPARAM)L"双键组合模式 Chord (低级键盘钩子，按住前导键再按触发键，如 End+PgUp)");
    SendMessageW(hCbMode, CB_SETCURSEL, s_dlgMode, 0);

    // Row 1: Mode 0 (Modifiers)
    CreateWindowExW(0, L"STATIC", L"修饰键：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        Scale(20), Scale(236), Scale(65), Scale(18),
        settingsWindow, (HMENU)IDC_LBL_MODIFIERS, GetModuleHandleW(nullptr), nullptr);

    HWND hChkCtrl = CreateWindowExW(0, L"BUTTON", L"Ctrl",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        Scale(90), Scale(234), Scale(65), Scale(20),
        settingsWindow, (HMENU)IDC_CHK_CTRL, GetModuleHandleW(nullptr), nullptr);
    if (s_dlgMods & MOD_CONTROL) SendMessageW(hChkCtrl, BM_SETCHECK, BST_CHECKED, 0);

    HWND hChkAlt = CreateWindowExW(0, L"BUTTON", L"Alt",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        Scale(160), Scale(234), Scale(65), Scale(20),
        settingsWindow, (HMENU)IDC_CHK_ALT, GetModuleHandleW(nullptr), nullptr);
    if (s_dlgMods & MOD_ALT) SendMessageW(hChkAlt, BM_SETCHECK, BST_CHECKED, 0);

    HWND hChkShift = CreateWindowExW(0, L"BUTTON", L"Shift",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        Scale(230), Scale(234), Scale(65), Scale(20),
        settingsWindow, (HMENU)IDC_CHK_SHIFT, GetModuleHandleW(nullptr), nullptr);
    if (s_dlgMods & MOD_SHIFT) SendMessageW(hChkShift, BM_SETCHECK, BST_CHECKED, 0);

    // Row 1: Mode 1 (Chord Key)
    CreateWindowExW(0, L"STATIC", L"前导键（按住）：",
        WS_CHILD | SS_LEFT,
        Scale(20), Scale(236), Scale(115), Scale(18),
        settingsWindow, (HMENU)IDC_LBL_CHORD_KEY, GetModuleHandleW(nullptr), nullptr);

    HWND hCbChord = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        Scale(140), Scale(233), Scale(175), Scale(200),
        settingsWindow, (HMENU)IDC_CB_CHORD_PRESET, GetModuleHandleW(nullptr), nullptr);
    for (const auto& item : k_chordKeyPresets) {
        SendMessageW(hCbChord, CB_ADDSTRING, 0, (LPARAM)item.name);
    }
    for (int i = 0; i < ARRAYSIZE(k_chordKeyPresets); i++) {
        if (k_chordKeyPresets[i].vk == s_dlgChordKey) {
            SendMessageW(hCbChord, CB_SETCURSEL, i, 0);
            break;
        }
    }

    CreateWindowExW(0, L"BUTTON", L"🎯 录制前导键",
        WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
        Scale(325), Scale(232), Scale(105), Scale(25),
        settingsWindow, (HMENU)IDC_BTN_REC_CHORD, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", FormatKeyDisplay(s_dlgChordKey).c_str(),
        WS_CHILD | SS_LEFTNOWORDWRAP,
        Scale(440), Scale(236), Scale(120), Scale(18),
        settingsWindow, (HMENU)IDC_LBL_CUR_CHORD, GetModuleHandleW(nullptr), nullptr);

    // Row 2: Main Key (for both modes)
    CreateWindowExW(0, L"STATIC", L"主快捷键：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        Scale(20), Scale(268), Scale(115), Scale(18),
        settingsWindow, (HMENU)IDC_LBL_MAIN_KEY, GetModuleHandleW(nullptr), nullptr);

    HWND hCbKey = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        Scale(140), Scale(265), Scale(175), Scale(200),
        settingsWindow, (HMENU)IDC_CB_KEY_PRESET, GetModuleHandleW(nullptr), nullptr);
    for (const auto& item : k_mainKeyPresets) {
        SendMessageW(hCbKey, CB_ADDSTRING, 0, (LPARAM)item.name);
    }
    for (int i = 0; i < ARRAYSIZE(k_mainKeyPresets); i++) {
        if (k_mainKeyPresets[i].vk == s_dlgKey) {
            SendMessageW(hCbKey, CB_SETCURSEL, i, 0);
            break;
        }
    }

    CreateWindowExW(0, L"BUTTON", L"🎯 录制按键",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        Scale(325), Scale(264), Scale(105), Scale(25),
        settingsWindow, (HMENU)IDC_BTN_REC_KEY, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", FormatKeyDisplay(s_dlgKey).c_str(),
        WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
        Scale(440), Scale(268), Scale(120), Scale(18),
        settingsWindow, (HMENU)IDC_LBL_CUR_KEY, GetModuleHandleW(nullptr), nullptr);

    // Hint text
    CreateWindowExW(0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        Scale(20), Scale(300), Scale(535), Scale(18),
        settingsWindow, (HMENU)IDC_LBL_HK_HINT, GetModuleHandleW(nullptr), nullptr);

    // Status text
    CreateWindowExW(0, L"STATIC", L"就绪",
        WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
        Scale(20), Scale(324), Scale(535), Scale(18),
        settingsWindow, (HMENU)IDC_LBL_STATUS, GetModuleHandleW(nullptr), nullptr);

    // Bottom buttons
    CreateWindowExW(0, L"BUTTON", L"🔄 刷新设备列表",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        Scale(20), Scale(355), Scale(130), Scale(30),
        settingsWindow, (HMENU)IDC_BTN_REFRESH, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"BUTTON", L"💾 保存并生效",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        Scale(305), Scale(355), Scale(125), Scale(30),
        settingsWindow, (HMENU)IDC_BTN_SAVE, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"BUTTON", L"取消",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        Scale(445), Scale(355), Scale(110), Scale(30),
        settingsWindow, (HMENU)IDC_BTN_CANCEL, GetModuleHandleW(nullptr), nullptr);

    if (s_dlgFont) {
        EnumChildWindows(settingsWindow, [](HWND hChild, LPARAM lParam) -> BOOL {
            SendMessageW(hChild, WM_SETFONT, lParam, TRUE);
            return TRUE;
        }, (LPARAM)s_dlgFont);
    }

    UpdateHotkeyUIMode(settingsWindow, s_dlgMode);
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
            AppendMenuW(menu, MF_STRING, 10, L"快捷键与设备设置(&S)...");
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
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(),&argc);
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

    IMMDeviceEnumerator* eProbe = nullptr;
    if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&eProbe)))) {
        auto devs = EnumActiveEndpoints(eProbe);
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
    bool registered = ApplyHotkey(h);

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
