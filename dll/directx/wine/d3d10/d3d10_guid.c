/*** Autogenerated by WIDL <undefined version> from F:/Develop/One-Core/One-Core-API-New/sdk/include/psdk/d3d10_1.idl - Do not edit ***/

#ifdef __REACTOS__
#define WIN32_NO_STATUS
#define WIN32_LEAN_AND_MEAN
#endif

#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
    DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
    const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif

#ifdef __cplusplus
extern "C" {
#endif

MIDL_DEFINE_GUID(IID, IID_ID3D10Effect, 0x51b0ca8b, 0xec0b, 0x4519, 0x87, 0x0d, 0x8e, 0xe1, 0xcb, 0x50, 0x17, 0xc7);
MIDL_DEFINE_GUID(IID, IID_ID3D10StateBlock, 0x0803425a, 0x57f5, 0x4dd6, 0x94, 0x65, 0xa8, 0x75, 0x70, 0x83, 0x4a, 0x08);
MIDL_DEFINE_GUID(IID, IID_ID3D10ShaderReflection, 0xd40e20b6, 0xf8f7, 0x42ad, 0xab, 0x20, 0x4b, 0xaf, 0x8f, 0x15, 0xdf, 0xaa);

#ifdef __cplusplus
}
#endif

#undef MIDL_DEFINE_GUID