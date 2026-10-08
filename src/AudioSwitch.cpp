#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>
#include <mmdeviceapi.h>
#include <strsafe.h>

static wchar_t ini[MAX_PATH],logpath[MAX_PATH];
static wchar_t ids[2][512], names[2][128];
static UINT key=VK_F13, mods=MOD_NOREPEAT, taskbarCreated;
static NOTIFYICONDATAW icon={sizeof(icon)};
static const wchar_t* cls=L"AudioSwitchNativeWindow";
static wchar_t exePath[MAX_PATH],iconPath[MAX_PATH];
static bool showTray=true,trayVisible=false,ownIcon=false;
static const wchar_t* startupKey=L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t* startupName=L"AudioSwitch-DellHeadphones";
static void StartupCommand(wchar_t* out,size_t length){StringCchPrintfW(out,length,L"\"%s\" --background",exePath);}
static bool StartupEnabled(){wchar_t actual[1024]={},expected[1024];DWORD size=sizeof(actual);StartupCommand(expected,1024);return RegGetValueW(HKEY_CURRENT_USER,startupKey,startupName,RRF_RT_REG_SZ,nullptr,actual,&size)==ERROR_SUCCESS&&!wcscmp(actual,expected);}
static bool SetStartup(bool enable){HKEY k;LONG result=RegCreateKeyExW(HKEY_CURRENT_USER,startupKey,0,nullptr,0,KEY_SET_VALUE,nullptr,&k,nullptr);if(result!=ERROR_SUCCESS)return false;if(enable){wchar_t value[1024];StartupCommand(value,1024);result=RegSetValueExW(k,startupName,0,REG_SZ,(BYTE*)value,(DWORD)((wcslen(value)+1)*sizeof(wchar_t)));}else{result=RegDeleteValueW(k,startupName);if(result==ERROR_FILE_NOT_FOUND)result=ERROR_SUCCESS;}RegCloseKey(k);return result==ERROR_SUCCESS;}
static void SaveState(){wchar_t path[MAX_PATH];StringCchCopyW(path,MAX_PATH,ini);wchar_t* slash=wcsrchr(path,L'\\');if(slash)StringCchCopyW(slash+1,MAX_PATH-(slash+1-path),L"ui-state.ini");WritePrivateProfileStringW(L"State",L"Running",L"1",path);WritePrivateProfileStringW(L"State",L"TrayVisible",trayVisible?L"1":L"0",path);WritePrivateProfileStringW(L"State",L"ShowTrayPreference",showTray?L"1":L"0",path);WritePrivateProfileStringW(L"State",L"AutoStart",StartupEnabled()?L"1":L"0",path);}
static void Log(const wchar_t* text){SYSTEMTIME t;GetLocalTime(&t);wchar_t line[2048];StringCchPrintfW(line,2048,L"%04u-%02u-%02u %02u:%02u:%02u %s\r\n",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,text);char utf8[8192];int n=WideCharToMultiByte(CP_UTF8,0,line,-1,utf8,sizeof(utf8),nullptr,nullptr);HANDLE f=CreateFileW(logpath,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(f!=INVALID_HANDLE_VALUE){DWORD done;WriteFile(f,utf8,n-1,&done,nullptr);CloseHandle(f);}}
static HRESULT Default(IMMDeviceEnumerator* e,ERole role,LPWSTR* id){IMMDevice* d=nullptr;HRESULT hr=e->GetDefaultAudioEndpoint(eRender,role,&d);if(SUCCEEDED(hr)){hr=d->GetId(id);d->Release();}return hr;}
static HRESULT Set(const wchar_t* id,ERole role){
 const CLSID c={0x870af99c,0x171d,0x4f9e,{0xaf,0x0d,0xe6,0x3d,0xf4,0x0c,0x2b,0xc9}};
 const IID iid={0xf8679f50,0x850a,0x41cf,{0x9c,0x72,0x43,0x0f,0x29,0x02,0x90,0xc8}};
 IUnknown* p=nullptr;HRESULT hr=CoCreateInstance(c,nullptr,CLSCTX_INPROC_SERVER,iid,(void**)&p);
 if(SUCCEEDED(hr)){using Fn=HRESULT(STDMETHODCALLTYPE*)(void*,LPCWSTR,ERole);auto fn=(Fn)(*(void***)p)[13];hr=fn(p,id,role);p->Release();}return hr;
}
static HRESULT Verify(IMMDeviceEnumerator* e,const wchar_t* id,ERole role){for(int i=0;i<20;i++){LPWSTR got=nullptr;HRESULT hr=Default(e,role,&got);bool match=SUCCEEDED(hr)&&!wcscmp(got,id);CoTaskMemFree(got);if(match)return S_OK;Sleep(50);}return E_FAIL;}
static HRESULT Restore(IMMDeviceEnumerator* e,LPWSTR* original){HRESULT result=S_OK;for(int i=0;i<2;i++){HRESULT hr=Set(original[i],(ERole)i);if(SUCCEEDED(hr))hr=Verify(e,original[i],(ERole)i);if(FAILED(hr))result=hr;}return result;}
static HRESULT Select(IMMDeviceEnumerator* e,const wchar_t* id){IMMDevice* d=nullptr;HRESULT hr=e->GetDevice(id,&d);if(FAILED(hr))return hr;DWORD state=0;hr=d->GetState(&state);d->Release();if(FAILED(hr)||state!=DEVICE_STATE_ACTIVE)return HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);
 LPWSTR old[2]={};hr=Default(e,eConsole,&old[0]);if(SUCCEEDED(hr))hr=Default(e,eMultimedia,&old[1]);if(SUCCEEDED(hr)){hr=Set(id,eConsole);if(SUCCEEDED(hr))hr=Set(id,eMultimedia);if(SUCCEEDED(hr))hr=Verify(e,id,eConsole);if(SUCCEEDED(hr))hr=Verify(e,id,eMultimedia);if(FAILED(hr)){HRESULT restore=Restore(e,old);if(FAILED(restore)){Log(L"ERROR rollback failed; select output in Windows settings");hr=restore;}}}for(auto p:old)CoTaskMemFree(p);return hr;
}
static HRESULT RunAudio(bool test,int* selected){IMMDeviceEnumerator* e=nullptr;HRESULT hr=CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&e));if(FAILED(hr))return hr;
 if(test){LPWSTR old[2]={};hr=Default(e,eConsole,&old[0]);if(SUCCEEDED(hr))hr=Default(e,eMultimedia,&old[1]);if(SUCCEEDED(hr)){for(int i=0;i<2;i++){WritePrivateProfileStringW(L"TestRestore",i?L"Multimedia":L"Console",old[i],ini);}for(int i=0;i<2;i++){hr=Select(e,ids[i]);if(FAILED(hr))break;Log(names[i]);Sleep(400);}HRESULT back=Restore(e,old);if(FAILED(back))hr=back;else Log(L"Original audio defaults restored");}for(auto p:old)CoTaskMemFree(p);}
 else {LPWSTR current=nullptr;hr=Default(e,eMultimedia,&current);if(SUCCEEDED(hr)){int target=!wcscmp(current,ids[0])?1:0;hr=Select(e,ids[target]);if(SUCCEEDED(hr)){*selected=target;Log(names[target]);}}CoTaskMemFree(current);}
 e->Release();return hr;
}
static void Notify(const wchar_t* text,bool error){if(!trayVisible)return;StringCchCopyW(icon.szInfoTitle,64,error?L"音频切换失败":L"声音输出");StringCchCopyW(icon.szInfo,256,text);icon.uFlags=NIF_INFO;icon.dwInfoFlags=error?NIIF_WARNING:NIIF_INFO;Shell_NotifyIconW(NIM_MODIFY,&icon);}
static void Toggle(){int selected=0;HRESULT hr=RunAudio(false,&selected);if(SUCCEEDED(hr))Notify(names[selected],false);else{wchar_t text[160];StringCchPrintfW(text,160,L"目标设备可能未连接（0x%08X）。",(unsigned)hr);Log(text);Notify(text,true);}}
static void AddTray(HWND h){if(!showTray||trayVisible)return;icon.hWnd=h;icon.uID=1;icon.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;icon.uCallbackMessage=WM_APP+1;if(!icon.hIcon){icon.hIcon=(HICON)LoadImageW(nullptr,iconPath,IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_LOADFROMFILE);ownIcon=icon.hIcon!=nullptr;if(!icon.hIcon)icon.hIcon=LoadIconW(nullptr,IDI_INFORMATION);}StringCchCopyW(icon.szTip,128,L"音频切换 · 右键设置");trayVisible=Shell_NotifyIconW(NIM_ADD,&icon)!=FALSE;}
static bool SetTray(HWND h,bool visible){if(!WritePrivateProfileStringW(L"UI",L"ShowTray",visible?L"1":L"0",ini))return false;showTray=visible;if(visible)AddTray(h);else{Shell_NotifyIconW(NIM_DELETE,&icon);trayVisible=false;}return true;}
static LRESULT CALLBACK WindowProc(HWND h,UINT msg,WPARAM w,LPARAM l){if(msg==taskbarCreated){trayVisible=false;AddTray(h);return 0;}switch(msg){
 case WM_APP+2:if(w==1)return SetTray(h,true);if(w==2)return SetTray(h,false);if(w==3){DestroyWindow(h);return 1;}if(w==4){SaveState();return 1;}if(w==5)return SetStartup(true);if(w==6)return SetStartup(false);return 0;
 case WM_HOTKEY:if(w==1)Toggle();return 0;
 case WM_APP+1:if(l==WM_LBUTTONDBLCLK)Toggle();else if(l==WM_RBUTTONUP||l==WM_CONTEXTMENU){POINT p;GetCursorPos(&p);HMENU menu=CreatePopupMenu();AppendMenuW(menu,MF_STRING,1,L"切换：Dell ↔ 耳机");AppendMenuW(menu,MF_STRING|(StartupEnabled()?MF_CHECKED:0),3,L"开机自启（当前用户登录时）");AppendMenuW(menu,MF_STRING,4,L"隐藏托盘图标（快捷键继续工作）");AppendMenuW(menu,MF_SEPARATOR,0,nullptr);AppendMenuW(menu,MF_STRING,2,L"退出程序（停止快捷键）");SetForegroundWindow(h);int cmd=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,0,h,nullptr);DestroyMenu(menu);PostMessageW(h,WM_NULL,0,0);if(cmd==1)Toggle();if(cmd==2)DestroyWindow(h);if(cmd==3&&!SetStartup(!StartupEnabled()))Notify(L"无法更新开机自启设置。",true);if(cmd==4&&!SetTray(h,false))Notify(L"无法保存托盘设置。",true);}return 0;
 case WM_CLOSE:DestroyWindow(h);return 0;
 case WM_DESTROY:UnregisterHotKey(h,1);Shell_NotifyIconW(NIM_DELETE,&icon);trayVisible=false;if(ownIcon)DestroyIcon(icon.hIcon);icon.hIcon=nullptr;PostQuitMessage(0);return 0;
 }return DefWindowProcW(h,msg,w,l);}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int){
 wchar_t base[MAX_PATH];GetModuleFileNameW(nullptr,exePath,MAX_PATH);StringCchCopyW(base,MAX_PATH,exePath);wchar_t* slash=wcsrchr(base,L'\\');if(!slash)return 1;slash[1]=0;StringCchPrintfW(ini,MAX_PATH,L"%saudio-switch.ini",base);StringCchPrintfW(logpath,MAX_PATH,L"%saudio-native.log",base);StringCchPrintfW(iconPath,MAX_PATH,L"%sAudioSwitch.ico",base);
 int argc=0;LPWSTR* argv=CommandLineToArgvW(GetCommandLineW(),&argc);if(!argv)return 1;const wchar_t* arg=argc==2?argv[1]:L"";bool background=!wcscmp(arg,L"--background"),test=!wcscmp(arg,L"--self-test"),single=!wcscmp(arg,L"--toggle");UINT command=!wcscmp(arg,L"--show-tray")?1:!wcscmp(arg,L"--hide-tray")?2:!wcscmp(arg,L"--exit")?3:!wcscmp(arg,L"--status")?4:!wcscmp(arg,L"--autostart-on")?5:!wcscmp(arg,L"--autostart-off")?6:0;LocalFree(argv);
 if(command){HWND running=FindWindowW(cls,nullptr);if(running){DWORD_PTR result=0;return SendMessageTimeoutW(running,WM_APP+2,command,0,SMTO_ABORTIFHUNG,3000,&result)&&result?0:1;}if(command==5||command==6)return SetStartup(command==5)?0:1;if(command==3)return 0;if(command==2)return WritePrivateProfileStringW(L"UI",L"ShowTray",L"0",ini)?0:1;if(command==4)return 1;WritePrivateProfileStringW(L"UI",L"ShowTray",L"1",ini);}
 showTray=GetPrivateProfileIntW(L"UI",L"ShowTray",1,ini)!=0;if(!background&&!test&&!single){showTray=true;WritePrivateProfileStringW(L"UI",L"ShowTray",L"1",ini);}
 GetPrivateProfileStringW(L"Devices",L"DellId",L"",ids[0],512,ini);GetPrivateProfileStringW(L"Devices",L"HeadphonesId",L"",ids[1],512,ini);StringCchCopyW(names[0],128,L"DELL S2725QS");StringCchCopyW(names[1],128,L"耳机（High Definition Audio Device）");
 key=GetPrivateProfileIntW(L"Hotkey",L"VirtualKey",VK_F13,ini);mods|=GetPrivateProfileIntW(L"Hotkey",L"Modifiers",0,ini);
 if(!ids[0][0]||!ids[1][0]||!wcscmp(ids[0],ids[1])||key<1||key>254||(mods&~(MOD_NOREPEAT|MOD_ALT|MOD_CONTROL|MOD_SHIFT))){MessageBoxW(nullptr,L"audio-switch.ini 配置无效",L"音频切换",MB_ICONERROR);return 1;}
 HRESULT hr=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);if(FAILED(hr))return 1;
 if(test||single){int selected=0;hr=RunAudio(test,&selected);Log(SUCCEEDED(hr)?L"PASS":L"FAIL");CoUninitialize();return FAILED(hr)?1:0;}
 HANDLE mutex=CreateMutexW(nullptr,TRUE,L"Local\\DellHeadphonesAudioSwitch");if(!mutex||GetLastError()==ERROR_ALREADY_EXISTS){if(!background){HWND running=FindWindowW(cls,nullptr);if(running){DWORD_PTR result;SendMessageTimeoutW(running,WM_APP+2,1,0,SMTO_ABORTIFHUNG,3000,&result);}}if(mutex)CloseHandle(mutex);CoUninitialize();return 0;}
 taskbarCreated=RegisterWindowMessageW(L"TaskbarCreated");WNDCLASSW wc={};wc.lpfnWndProc=WindowProc;wc.hInstance=instance;wc.lpszClassName=cls;RegisterClassW(&wc);HWND h=CreateWindowExW(WS_EX_TOOLWINDOW,cls,L"AudioSwitch",WS_POPUP,0,0,0,0,nullptr,nullptr,instance,nullptr);
 if(!h||!RegisterHotKey(h,1,mods,key)){MessageBoxW(nullptr,L"无法注册快捷键，可能被其他程序占用。",L"音频切换",MB_ICONERROR);if(h)DestroyWindow(h);CloseHandle(mutex);CoUninitialize();return 1;}
 AddTray(h);Log(L"Native listener ready");MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}CloseHandle(mutex);CoUninitialize();return 0;
}
