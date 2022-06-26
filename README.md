## Welcome to One-Core-API!

This repository contains the source-code for the One-Core-API project.

One-Core-API is based on React OS, therefore the build system and many underlying components are the same.

One-Core-API's build system is compatible with Windows Server 2003 SP2, Windows XP X64 Edition SP2, and Windows XP SP3.

This repository contains all code from React OS, as well as some additional wrappers, compatible Windows Vista DLLs, backported APIs, ETC.

### List of known compatible software.

* Web Storm 2018
* Intelliji 2018 (other versions might work)
* Filezilla (latest)
* Visual Studio Code 1.18
* Chrome version 102
* Opera version 38
* Firefox version 53
* JDK 1.8
* Maxthon 5.1
* Python 3.6
* .Net Framework 4.8
* Geekbench 4.2
* Performance Test
* Adobe Reader DC (2021)
* Windows 7 games
* Vista Applications
* K-Lite Codec 15+

Several other applications have been tested.

### Installation order of packages:

1. Case 1 (Kernel Standalone first): Kernel Standalone -> App Compat Installer: you can't install Base installer and other packages side by side with Kernel Standalone installed;
2: App Compat (only once)
3: Case Base Installer first Base Installer -> New Installer (Additional Dlls) -> D3d Installer -> API-SET installer -> App Compat -> Kernel Update
4: AppCompat Base Installer first App Compat -> Base Installer -> New Installer (Additional Dlls) -> D3d Installer -> API-SET installer -> Kernel Update