# ![](Docs/KakaoTalkAdGuard.png) KakaoTalk AdGuard

Ad removal tool for Windows KakaoTalk

## Features

- No framework libraries required
- Support Windows and Windows Server including all language versions
- Support Autorun on system startup (installation version only)
- Support Installer and Uninstaller

## Download

- Before or after installation, Specify the installation directory path (default: `%appdata%\KakaoTalkAdGuard`) as an exception path in your antivirus software settings. Because this binary file is not well known, antivirus software may detect it as malware.
- [Go to Release page to download](https://github.com/loopback-kr/KakaoTalkAdGuard/releases)

## Known issues

- On Windows with HiDPI, the trayicon menu is displayed in an abnormal position.

## Preview

![Highlights](https://github.com/loopback-kr/KakaoTalkAdGuard/assets/28856527/493bea2b-87c9-4792-9cfd-c534aec02b14)

## Release notes

### 1.0.0.11

<sup>Dec. 19, 2024</sup>

- Fixed an issue with ADs appearing in version 4.3.0.4263

### 1.0.0.10

<sup>May 9, 2024</sup>

- Fixed an issue where ads were not blocked when opening a KakaoTalk window and message notification windows were open at the same time

### 1.0.0.9

<sup>May 4, 2024</sup>

- Fixed an issue that updated profiles disappeared

### 1.0.0.8

<sup>Apr. 27, 2024</sup>

- Hotfix for the lockdown setting window not appearing issue

### 1.0.0.7

<sup>Apr. 27, 2024</sup>

- Support for multilingual versions of KakaoTalk
- Changed banner ad blocking criteria
- Removed white box at lockdown mode
- Code refactoring

### 1.0.0.6

<sup>Apr. 22, 2024</sup>

- Hotfix for the profile window not appearing issue

### 1.0.0.5

<sup>Apr. 22, 2024</sup>

- Support KakaoTalk version 4

### 1.0.0.4

<sup>Apr. 20, 2024</sup>

- Hotfix for KakaoTalk version 4 (experimental)

### 1.0.0.3

<sup>Jun. 23, 2023</sup>

- Support 32-bit architecture
- Support portable version trayicon context menu
- Changed restoring trayicon mechanism

### 1.0.0.2

<sup>Jun. 1, 2023</sup>

- Improved stability of trayicon
- Improved stability for popup ad removal mechanism
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
