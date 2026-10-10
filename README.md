# AudioSwitch

轻量 Windows 音频输出切换工具。用一个快捷键在 **Dell 显示器扬声器与有线耳机**之间切换，适合搭配洛斐小顺2等可编程键盘。

原生 C++ / Win32 实现，无 .NET 依赖，无键盘轮询、驱动或 Shell 注入。空闲时通过 Windows 消息循环等待快捷键。本机专用工作集曾测得约 1.1 MB，实际占用会随环境和操作变化。

支持 End+PgUp 双键组合：在 `[Hotkey]` 中设置 `VirtualKey=33`、`ChordKey=35`、`Modifiers=0`，重启程序。按住 End 再按 PgUp，每次独立按下 PgUp 切换一次，长按不连发。组合中的 PgUp 不再向当前应用发送翻页；End 保留原有按键行为（在编辑器中仍可能移动光标）。单独 PgUp 正常工作。

双键组合使用 Windows 低级键盘 Hook，仅检查按键状态，不记录输入文本；Hook 回调只投递切换消息，不在回调内执行音频操作。`ChordKey=0` 时仍使用原有 RegisterHotKey 方式。

## 功能

- 默认按 **F13** 切换两个配置的播放端点。
- **动态设备识别**：每次切换时基于设备的友好名称（FriendlyName）与硬件描述（DeviceDesc）动态解析系统最新的活动端点 ID。即使因电脑重启、休眠或插拔导致 Windows 重新分配端点 GUID，依然自动无缝识别。
- **图形配置界面**：托盘右键可一键打开“快捷键与设备设置”窗口，可视化下拉选择目标设备、查看硬件描述与驱动接口，并提供即时试听切换。
- **快捷键图形化配置**：
  - 支持单键/修饰键模式（可勾选 Ctrl/Alt/Shift，支持预设选择或按键直接录制，默认 F13）；
  - 支持双键组合模式（Chord，如 End+PgUp，可录制或选择前导键与触发键）；
  - 点击“🎯 录制”后直接敲击键盘即可一键捕获按键；
  - 保存后即时重新注册热键并生效，无需重启程序。
- 托盘双击切换；右键可隐藏图标、开关自启或退出。
- 隐藏图标后快捷键继续工作；再次双击 EXE 可恢复图标。
- 自启仅作用于当前用户登录，保留托盘隐藏偏好。
- 同目录 `AudioSwitch.ico` 可自定义托盘图标。
- 设备不可用时不会自动切换到第三个设备。

只修改 Console / Multimedia 默认播放角色，不修改麦克风或 Communications 角色。应用若固定指定了音频设备，需要在应用内改为“系统默认”。

## 构建

需要 Windows x64、Visual Studio 或 Build Tools 的“使用 C++ 的桌面开发”工作负载及 Windows SDK。

```bat
build.cmd
```

脚本通过 `vswhere` 查找编译器，生成 `build\AudioSwitch.exe`。采用静态 C/C++ 运行库链接。

## 配置设备与快捷键

**推荐方式：图形界面配置**
右键点击系统托盘图标，选择 **“快捷键与设备设置(&S)...”**（或执行 `AudioSwitch.exe --settings`）：
1. **音频设备**：下拉选择“设备 1”和“设备 2”，可查看详细硬件描述与驱动接口类型，可点击“试切此设备”试听效果；
2. **切换热键**：
   - 选择切换模式（“单键/修饰键” 或 “双键组合 Chord”）；
   - 单键模式可勾选 Ctrl/Alt/Shift，点击【🎯 录制主键】直接在键盘上敲键绑定（如 F13、F1~F24、字母键等），或从常用预设下拉选择；
3. **通知设置**：勾选或取消【切换音频时弹出系统通知气泡】，支持静默切换模式（游戏/观影免打扰）；
4. 点击“保存并生效”后即刻更新配置并热重载生效，配置持久化到 `audio-switch.ini`。

**手动配置文件配置（可选）**
将 `audio-switch.example.ini` 复制为 `audio-switch.ini`：
```ini
[Devices]
Device1Friendly=DELL S2725QS (NVIDIA High Definition Audio)
Device1Desc=DELL S2725QS
Device1Interface=NVIDIA High Definition Audio

Device2Friendly=耳机 (High Definition Audio Device)
Device2Desc=耳机
Device2Interface=High Definition Audio Device
```
程序在每次切换时都会动态遍历当前系统活跃设备并打分匹配，因此开关机、插拔显示器或显卡驱动重载后均无需重新配置。旧版本的 `DellId` 和 `HeadphonesId` 字段同样作为向后兼容 fallback 保留。

## 键盘与托盘

在键盘改键工具（例如适配该键盘的 VIA）中，将一个按键映射为 **F13**。也可使用自定义键码 `KC_F13`。不用录制宏。

双击 EXE 启动监听，不需要管理员权限。`VirtualKey=124` 表示 F13；修饰键 `Modifiers` 支持 Alt=1、Ctrl=2、Shift=4，组合时相加。配置修改后需退出并重启程序。

托盘菜单区分：

- **隐藏托盘图标**：继续监听快捷键，停止显示切换气泡。
- **退出程序**：停止快捷键监听。
- **开机自启**：当前用户登录时运行，默认关闭。

程序使用当前用户的 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` 中的 `AudioSwitch-DellHeadphones` 项，启动参数为 `--background`。不创建服务或计划任务。启用后若移动程序，应先关闭自启，再在新位置重新启用。

## ICO 图标

把标准 ICO 文件命名为 `AudioSwitch.ico`，放在 EXE 同目录，退出再启动。建议包含 16×16、32×32、48×48 等尺寸。缺失或加载失败时回退为系统 `IDI_INFORMATION` 图标。

这是托盘图标配置，不会改变 EXE 文件的图标；EXE 图标需在构建时嵌入资源。

## 命令行

| 参数 | 行为 |
|---|---|
| 无参数 | 启动监听；若已有实例，恢复显示其托盘 |
| `--background` | 启动监听，遵循隐藏偏好 |
| `--toggle` | 切换一次后退出 |
| `--show-tray` / `--hide-tray` | 显示 / 隐藏托盘 |
| `--settings` / `--config` | 打开音频设备与切换图形配置界面 |
| `--exit` | 退出运行实例 |
| `--autostart-on` / `--autostart-off` | 开启 / 关闭当前用户自启 |
| `--status` | 将运行实例状态写入 `ui-state.ini` |
| `--self-test` | 短暂切换两个端点，读回验证，再恢复原播放默认值 |

自测会临时改变实际音频输出。原值记录在 INI 的 `TestRestore` 节；正常返回时恢复。如强制终止进程，应在 Windows 声音面板中恢复输出。

日志为 EXE 同目录的 `audio-native.log`。程序目录需要当前用户可写。

## 验证和限制

已在 Windows 11 26H2 / 26300.9550 上验证双端点切换及恢复、快捷键注册、隐藏/恢复托盘、自启项增删和后台启动偏好。没有用重启 Windows 验证登录自启。

读取端点使用 Core Audio；设置默认端点使用 **未文档化的 IPolicyConfig 接口**，未来 Windows 更新可能需要适配。托盘使用 `Shell_NotifyIconW`，快捷键使用 `RegisterHotKey`。

卸载：先取消开机自启，再退出并删除程序目录。退出不会改变最后选择的音频输出。

本仓库不包含个人设备配置、日志或之前的显示器亮度实验。
