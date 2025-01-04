@ stdcall CopyContext(ptr long ptr) kernelex.CopyContext
@ stdcall -ret64 -arch=i386,x86_64 GetEnabledXStateFeatures() kernelex.GetEnabledXStateFeatures
@ stdcall -arch=i386,x86_64 GetXStateFeaturesMask(ptr ptr) kernelex.GetXStateFeaturesMask
@ stdcall -arch=i386,x86_64 InitializeContext(ptr long ptr ptr) kernelex.InitializeContext
@ stdcall -arch=i386,x86_64 LocateXStateFeature(ptr long ptr) kernelex.LocateXStateFeature
@ stdcall -arch=i386,x86_64 SetXStateFeaturesMask(ptr int64) kernelex.SetXStateFeaturesMask
