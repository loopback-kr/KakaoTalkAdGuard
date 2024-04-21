# ![](Docs/KakaoTalkADGuard.png) KakaoTalk ADGuard

AD removal tool for Windows KakaoTalk

## Features

- No framework libraries required
- Support Windows and Windows Server including all language versions
- Support Autorun on system startup (installation version only)
- Support Installer and Uninstaller

## Download

- Before or after installation, Specify the installation directory path (default: `%appdata%\KakaoTalkADGuard`) as an exception path in your antivirus software settings. Because this binary file is not well known, antivirus software may detect it as malware.
- [Go to Release page to download](https://github.com/loopback-kr/KakaoTalkADGuard/releases)

## Known issues

- In KakaoTalk version 4, Ads can appear when user logs-in or resizes KakaoTalk window. In this case, the ADs will disappear when you move the KakaoTalk window.

- On Windows with HiDPI, the trayicon menu is displayed in an abnormal position.

- If a KakaoTalk process is running on a regular user account that has been promoted to administrator privileges due to a KakaoTalk update, it is not possible to block ADs unless KakaoTalk ADGuard is also running as administrator privileges.

## Preview

![Highlights](https://github.com/loopback-kr/KakaoTalkADGuard/assets/28856527/493bea2b-87c9-4792-9cfd-c534aec02b14)

## Release notes

### 1.0.0.4

<sup>Apr. 20, 2024</sup>

- Support KakaoTalk version 4 (experimental)

### 1.0.0.3

<sup>Jun. 23, 2023</sup>

- Support 32-bit architecture
- Support portable version trayicon context menu
- Changed restoring trayicon mechanism

### 1.0.0.2

<sup>Jun. 1, 2023</sup>

- Improved stability of trayicon
- Improved stability for popup AD removal mechanism
- Changed context menu items for trayicon
- Fixed an issue that a registry key was not deleted when uninstalling
- Update icon for uninstallation
- Minimized Windows UAC permissions
- Changed installation mechanism to close running processes
- Added version attributes and uninstall information

### 1.0.0.1

<sup>May 30, 2023</sup>

- Fixed issue where the send button was removed when sending files in chat room.

### 1.0.0.0

<sup>May 29, 2023</sup>

- Initial release
