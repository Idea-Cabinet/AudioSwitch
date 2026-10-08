# AudioSwitch

轻量 Windows 音频输出切换工具。用一个快捷键在 **Dell 显示器扬声器与有线耳机**之间切换，适合搭配洛斐小顺2等可编程键盘。

原生 C++ / Win32 实现，无 .NET 依赖，无键盘轮询、驱动或 Shell 注入。空闲时通过 Windows 消息循环等待快捷键。本机专用工作集曾测得约 1.1 MB，实际占用会随环境和操作变化。

支持 End+PgUp 双键组合：在 `[Hotkey]` 中设置 `VirtualKey=33`、`ChordKey=35`、`Modifiers=0`，重启程序。按住 End 再按 PgUp，每次独立按下 PgUp 切换一次，长按不连发。组合中的 PgUp 不再向当前应用发送翻页；End 保留原有按键行为（在编辑器中仍可能移动光标）。单独 PgUp 正常工作。

双键组合使用 Windows 低级键盘 Hook，仅检查按键状态，不记录输入文本；Hook 回调只投递切换消息，不在回调内执行音频操作。`ChordKey=0` 时仍使用原有 RegisterHotKey 方式。

## 功能

- 默认按 **F13** 切换两个配置的播放端点。
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

## 配置设备

将 `audio-switch.example.ini` 复制到 EXE 同目录，命名为 `audio-switch.ini`，填写两个真实播放端点 ID。示例中的占位符不可直接使用。

以下 PowerShell 命令只读列出注册表中处于活动状态的播放设备及 ID：

```powershell
Get-ChildItem 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render' |
    ForEach-Object {
        $device = Get-ItemProperty $_.PSPath
        $properties = Get-ItemProperty ($_.PSPath + '\Properties')
        if ($device.DeviceState -eq 1) {
            [pscustomobject]@{
                Name = $properties.'{a45c254e-df1c-4efd-8020-67d146a850e0},2'
                Interface = $properties.'{b3f8fa53-0004-438e-9003-51a46e139bfc},6'
                EndpointId = '{0.0.0.00000000}.' + $_.PSChildName
            }
        }
    } | Format-List
```

`DellId` 和 `HeadphonesId` 实际上是两个目标端点槽位。可填写其他播放设备，但当前托盘与提示文字仍按 Dell / 耳机命名。重装驱动或变更端口后，端点 ID 可能需要更新。

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
