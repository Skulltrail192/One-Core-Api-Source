Welcome to One-Core-API!

Here you can found source code of One-Core-API project. It is based on ReactOS source and use ReactOS's build environment and is compatible with Windows Server 2003 SP2, Windows XP SP3 and Windows XP x64 SP2. You can do amazing things installing binaries.

This is software that uses modified files from the respective systems, contains other files still in the testing or experimental phase and has only one developer. In other words, it is impossible to predict all possible scenarios on all possible types of computers or virtual machines. Between XP/2003 and Vista there was the biggest leap in new APIs, new technologies and modifications to existing APIs, so it is very difficult to have the same level of compatibility in NT5 as in NT6. Be calm, be prudent and before saying that this software is bad or "crap", report the defect in the issues and as soon as possible, it will be analyzed and I will try to correct the problem. Help me, complaining or defaming the software doesn't do anyone any good.

Now, we will describe folders present on this repository:

Contains all ReactOS code and addition of wrappers, new dlls present in Windows Vista, drivers and API-SETs. 

You can run several programs with One-Core-API, like:

    Web Storm 2018
    Intelliji 2018 (maybe other versions works)
    Filezilla (lastest)
    Visual Studio Code 1.81
    Chrome up to version 123
    Opera up to version 106
    Firefox up to version 122
    JDK 1.8
    Maxthon 5.1
    Python 3.6
    .Net Framework up to 4.8
    Geekbench 4.2
    Performance Test
    Adobe Reader DC (2021)
    Windows 7 games
    Vista Applications
    K-Lite Codec 15+
    Several other applications

All code related with One-Core-API is on "wrappers" folder. Only Visual Studio Solution is supported to compile this code. And, you must use ROSBE version 2.1.6. If try use 2.2.0+ version, you will get a error. 
You can download this ROSBE version using this link: 
https://sourceforge.net/projects/reactos/files/RosBE-Windows/i386/2.1.6/

Also, only Visual Studio 2010 and 2012 are supported.
For create a Visual Studio Solution, follow these steps:
- Create a folder for each arch to be output folder.
  - I always create "Output-MSVC-i386" for x86/i386 and "Output-MSVC-amd64" for am64/x64
- Open Visual Studio Command Prompt for your target architecture;
  - Commonly placed on start menu: "All Programs" -> "Microsoft Visual Studio 2012 (example)" -> "Visual Studio Tools" -> VS2012 x86 Native Tools Command Prompt (for x86) and  VS2012 x64 Cross Tools Command Prompt (for x64)
    ![Visual-Code-Tools-Shortcut](https://github.com/Skulltrail192/One-Core-Api/assets/5159776/30ae549b-fcb8-469e-af80-372a5e61aaea)
- On Visual Studio Command Prompt, navigate to Ouput folder;
  - Example: "cd **/D** D:\Output-MSVC-i386"
- After navigate to Output folder, copy path of your One-Core-API code folder and insert on command prompt with "configure VSSOlution" option;
  - Example: "D:\One-Core-API\configure VSSOlution";
- Wait configuration done if all did right;
  ![Configuration-done](https://github.com/Skulltrail192/One-Core-Api/assets/5159776/13dc5047-f411-4548-8912-e43c9a17591e)
- After done, open Explorer and navigate to Output Folder and open "Reactos.sln"
![Output-Folder](https://github.com/Skulltrail192/One-Core-Api/assets/5159776/3e1a3822-f501-44bb-896f-08ddae8bb62b)
- So, after load solution, will be presented this screen:
![Visual-Studio-Solution](https://github.com/Skulltrail192/One-Core-Api/assets/5159776/13542537-1c42-49ce-b280-1e8890f630b6)
- And to build, click with right button over component of you want compile and choose "Build"
![Build](https://github.com/Skulltrail192/One-Core-Api/assets/5159776/4018d2cf-94a1-4bca-853f-84a7806c3f93);
- x64 has a some bug on first build. Winnt.h from Windows Kits (installed by Visual Code) generate a error when compile. Comment line as on screenshot:
![Error-x64](https://github.com/Skulltrail192/One-Core-Api/assets/5159776/d1594f4c-2744-49fc-8c5c-98eb81c5244e)
- If this fix and on x86, if all build right, you see this screen:
![Build-Right](https://github.com/Skulltrail192/One-Core-Api/assets/5159776/fe73296f-5bda-4080-8818-23e48d40a0c9)
- So, to get the binarie, navigate to the folder corresponding to the component with explorer:
![Binarie](https://github.com/Skulltrail192/One-Core-Api/assets/5159776/87b82c5e-d311-4428-94d8-919c67a3e0cc)
- Done! 
