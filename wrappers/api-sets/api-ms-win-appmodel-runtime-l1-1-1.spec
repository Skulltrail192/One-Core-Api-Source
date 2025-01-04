@ stub ClosePackageInfo
@ stdcall FindPackagesByPackageFamily(wstr long ptr ptr ptr ptr ptr) kernelex.FindPackagesByPackageFamily
@ stub FormatApplicationUserModelId
@ stdcall GetApplicationUserModelId(long ptr wstr) kernelex.GetApplicationUserModelId
@ stdcall GetCurrentApplicationUserModelId(ptr wstr) kernelex.GetCurrentApplicationUserModelId
@ stdcall GetCurrentPackageFamilyName(ptr ptr) kernel32.GetCurrentPackageFamilyName
@ stdcall GetCurrentPackageFullName(ptr ptr) kernel32.GetCurrentPackageFullName
@ stdcall GetCurrentPackageId(ptr ptr) kernel32.GetCurrentPackageId
@ stdcall GetCurrentPackageInfo(long ptr ptr ptr) kernelex.GetCurrentPackageInfo
@ stdcall GetCurrentPackagePath(ptr ptr) kernelex.GetCurrentPackagePath
@ stdcall GetPackageApplicationIds (ptr ptr ptr ptr) kernelex.GetPackageApplicationIds
@ stdcall GetPackageFamilyName(long ptr ptr) kernelex.GetPackageFamilyName
@ stdcall GetPackageFullName(long ptr ptr) kernel32.GetPackageFullName
@ stub GetPackageId
@ stub GetPackageInfo
@ stub GetPackagePath
@ stdcall GetPackagePathByFullName(wstr ptr wstr) kernelex.GetPackagePathByFullName
@ stdcall GetPackagesByPackageFamily(wstr ptr ptr ptr ptr) kernelex.GetPackagesByPackageFamily
@ stub GetStagedPackageOrigin
@ stub GetStagedPackagePathByFullName
@ stdcall OpenPackageInfoByFullName (wstr long ptr) kernelex.OpenPackageInfoByFullName
@ stdcall -stub PackageFamilyNameFromFullName(wstr ptr wstr)
@ stub PackageFamilyNameFromId
@ stub PackageFullNameFromId
@ stub PackageIdFromFullName
@ stub PackageNameAndPublisherIdFromFamilyName
@ stub ParseApplicationUserModelId
