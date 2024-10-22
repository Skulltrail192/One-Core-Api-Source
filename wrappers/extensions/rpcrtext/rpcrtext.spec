@ stdcall CreateProxyFromTypeInfo(ptr ptr ptr ptr ptr)
@ stdcall CreateStubFromTypeInfo(ptr ptr ptr ptr)
#@ stdcall I_RpcServerTurnOnOffKeepalives() rpcrt4.I_RpcServerTurnOnOffKeepalives
@ stdcall CStdStubBuffer_AddRef(ptr) 
@ stdcall CStdStubBuffer_Connect(ptr ptr) 
@ stdcall CStdStubBuffer_CountRefs(ptr) 
@ stdcall CStdStubBuffer_DebugServerQueryInterface(ptr ptr)
@ stdcall CStdStubBuffer_DebugServerRelease(ptr ptr)
@ stdcall CStdStubBuffer_Disconnect(ptr)
@ stdcall CStdStubBuffer_Invoke(ptr ptr ptr)
@ stdcall CStdStubBuffer_IsIIDSupported(ptr ptr)
@ stdcall CStdStubBuffer_QueryInterface(ptr ptr ptr)
@ stdcall DceErrorInqTextA (long ptr)
@ stdcall DceErrorInqTextW (long ptr)
#@ stdcall DllGetClassObject() rpcrt4.DllGetClassObject
#@ stdcall DllInstall() rpcrt4.DllInstall
#@ stdcall DllRegisterServer() rpcrt4.DllRegisterServer
@ stdcall GlobalMutexClearExternal() rpcrt4.GlobalMutexClearExternal
@ stdcall GlobalMutexRequestExternal() rpcrt4.GlobalMutexRequestExternal
@ stdcall IUnknown_AddRef_Proxy(ptr)
@ stdcall IUnknown_QueryInterface_Proxy(ptr ptr ptr)
@ stdcall IUnknown_Release_Proxy(ptr)
@ stdcall I_RpcAbortAsyncCall(ptr long) I_RpcAsyncAbortCall
@ stdcall I_RpcAllocate(long)
@ stdcall I_RpcAsyncAbortCall(ptr long)
@ stdcall I_RpcAsyncSetHandle(ptr ptr)
@ stdcall I_RpcBCacheAllocate() rpcrt4.I_RpcBCacheAllocate
@ stdcall I_RpcBCacheFree() rpcrt4.I_RpcBCacheFree
@ stdcall I_RpcBindingCopy() rpcrt4.I_RpcBindingCopy
#@ stdcall I_RpcBindingHandleToAsyncHandle(ptr ptr) rpcrt4.I_RpcBindingHandleToAsyncHandle
@ stdcall I_RpcBindingInqConnId() rpcrt4.I_RpcBindingInqConnId
#@ stdcall I_RpcBindingInqDynamicEndpoint(ptr str)
#@ stdcall I_RpcBindingInqDynamicEndpointA(ptr str) rpcrt4.I_RpcBindingInqDynamicEndpointA
#@ stdcall I_RpcBindingInqDynamicEndpointW(ptr wstr) rpcrt4.I_RpcBindingInqDynamicEndpointW
@ stdcall I_RpcBindingInqLocalClientPID() rpcrt4.I_RpcBindingInqLocalClientPID 
#@ stdcall I_RpcBindingInqMarshalledTargetInfo() rpcrt4.I_RpcBindingInqMarshalledTargetInfo
@ stdcall I_RpcBindingInqSecurityContext() rpcrt4.I_RpcBindingInqSecurityContext
@ stdcall I_RpcBindingInqTransportType(ptr ptr)
@ stdcall I_RpcBindingInqWireIdForSnego() rpcrt4.I_RpcBindingInqWireIdForSnego
@ stdcall I_RpcBindingIsClientLocal() rpcrt4.I_RpcBindingIsClientLocal
@ stdcall I_RpcBindingToStaticStringBindingW() rpcrt4.I_RpcBindingToStaticStringBindingW
@ stdcall I_RpcClearMutex() rpcrt4.I_RpcClearMutex
@ stdcall I_RpcConnectionInqSockBuffSize() rpcrt4.I_RpcConnectionInqSockBuffSize
@ stdcall I_RpcConnectionSetSockBuffSize() rpcrt4.I_RpcConnectionSetSockBuffSize
@ stdcall I_RpcDeleteMutex() rpcrt4.I_RpcDeleteMutex
@ stdcall I_RpcEnableWmiTrace() rpcrt4.I_RpcEnableWmiTrace 
@ stdcall I_RpcExceptionFilter(long)
@ stdcall I_RpcFree(ptr)
@ stdcall I_RpcFreeBuffer(ptr)
@ stdcall I_RpcFreePipeBuffer() rpcrt4.I_RpcFreePipeBuffer
@ stdcall I_RpcGetBuffer(ptr)
@ stdcall I_RpcGetBufferWithObject() rpcrt4.I_RpcGetBufferWithObject
@ stdcall I_RpcGetCurrentCallHandle()
@ stdcall I_RpcGetExtendedError() rpcrt4.I_RpcGetExtendedError
@ stdcall I_RpcIfInqTransferSyntaxes() rpcrt4.I_RpcIfInqTransferSyntaxes
@ stdcall I_RpcLogEvent() rpcrt4.I_RpcLogEvent
@ stdcall I_RpcMapWin32Status(long)
#@ stdcall I_RpcNDRCGetWireRepresentation() rpcrt4.I_RpcNDRCGetWireRepresentation
#@ stdcall I_RpcNDRSContextEmergencyCleanup() rpcrt4.I_RpcNDRSContextEmergencyCleanup
@ stdcall I_RpcNegotiateTransferSyntax(ptr)
@ stdcall I_RpcNsBindingSetEntryName() rpcrt4.I_RpcNsBindingSetEntryName
@ stdcall I_RpcNsBindingSetEntryNameA() rpcrt4.I_RpcNsBindingSetEntryNameA
@ stdcall I_RpcNsBindingSetEntryNameW() rpcrt4.I_RpcNsBindingSetEntryNameW
@ stdcall I_RpcNsInterfaceExported() rpcrt4.I_RpcNsInterfaceExported
@ stdcall I_RpcNsInterfaceUnexported() rpcrt4.I_RpcNsInterfaceUnexported
@ stdcall I_RpcParseSecurity() rpcrt4.I_RpcParseSecurity
@ stdcall I_RpcPauseExecution() rpcrt4.I_RpcPauseExecution
@ stdcall I_RpcProxyNewConnection() rpcrt4.I_RpcProxyNewConnection 
@ stdcall I_RpcReallocPipeBuffer() rpcrt4.I_RpcReallocPipeBuffer
@ stdcall I_RpcReceive(ptr)
#@ stdcall I_RpcRecordCalloutFailure() rpcrt4.I_RpcRecordCalloutFailure
#@ stdcall I_RpcReplyToClientWithStatus() rpcrt4.I_RpcReplyToClientWithStatus
@ stdcall I_RpcRequestMutex() rpcrt4.I_RpcRequestMutex
@ stdcall I_RpcSend(ptr)
@ stdcall I_RpcSendReceive(ptr)
@ stdcall I_RpcServerAllocateIpPort() rpcrt4.I_RpcServerAllocateIpPort
#@ stdcall I_RpcServerCheckClientRestriction() rpcrt4.I_RpcServerCheckClientRestriction
@ stdcall I_RpcServerInqAddressChangeFn() rpcrt4.I_RpcServerInqAddressChangeFn
@ stdcall I_RpcServerInqLocalConnAddress() rpcrt4.I_RpcServerInqLocalConnAddress 
@ stdcall I_RpcServerInqTransportType() rpcrt4.I_RpcServerInqTransportType
#@ stdcall I_RpcServerIsClientDisconnected() rpcrt4.I_RpcServerIsClientDisconnected
@ stdcall I_RpcServerRegisterForwardFunction() rpcrt4.I_RpcServerRegisterForwardFunction
@ stdcall I_RpcServerSetAddressChangeFn() rpcrt4.I_RpcServerSetAddressChangeFn
@ stdcall I_RpcServerUseProtseq2A() rpcrt4.I_RpcServerUseProtseq2A
@ stdcall I_RpcServerUseProtseq2W() rpcrt4.I_RpcServerUseProtseq2W
@ stdcall I_RpcServerUseProtseqEp2A() rpcrt4.I_RpcServerUseProtseqEp2A
@ stdcall I_RpcServerUseProtseqEp2W() rpcrt4.I_RpcServerUseProtseqEp2W
#@ stdcall I_RpcSessionStrictContextHandle() rpcrt4.I_RpcSessionStrictContextHandle
@ stdcall I_RpcSetAsyncHandle() rpcrt4.I_RpcSetAsyncHandle
@ stdcall I_RpcSsDontSerializeContext() rpcrt4.I_RpcSsDontSerializeContext
@ stdcall I_RpcSystemFunction001() rpcrt4.I_RpcSystemFunction001  
@ stdcall I_RpcTransConnectionAllocatePacket() rpcrt4.I_RpcTransConnectionAllocatePacket
@ stdcall I_RpcTransConnectionFreePacket() rpcrt4.I_RpcTransConnectionFreePacket
@ stdcall I_RpcTransConnectionReallocPacket() rpcrt4.I_RpcTransConnectionReallocPacket
@ stdcall I_RpcTransDatagramAllocate2() rpcrt4.I_RpcTransDatagramAllocate2
@ stdcall I_RpcTransDatagramAllocate() rpcrt4.I_RpcTransDatagramAllocate
@ stdcall I_RpcTransDatagramFree() rpcrt4.I_RpcTransDatagramFree
@ stdcall I_RpcTransGetThreadEvent() rpcrt4.I_RpcTransGetThreadEvent
@ stdcall I_RpcTransIoCancelled() rpcrt4.I_RpcTransIoCancelled
@ stdcall I_RpcTransServerNewConnection() rpcrt4.I_RpcTransServerNewConnection
@ stdcall I_RpcTurnOnEEInfoPropagation() rpcrt4.I_RpcTurnOnEEInfoPropagation 
@ stdcall I_UuidCreate() rpcrt4.I_UuidCreate
@ stdcall MIDL_wchar_strcpy() rpcrt4.MIDL_wchar_strcpy
@ stdcall MIDL_wchar_strlen() rpcrt4.MIDL_wchar_strlen
@ stdcall MesBufferHandleReset() rpcrt4.MesBufferHandleReset
@ stdcall MesDecodeBufferHandleCreate(ptr long ptr)
@ stdcall MesDecodeIncrementalHandleCreate(ptr ptr ptr)
@ stdcall MesEncodeDynBufferHandleCreate(ptr ptr ptr)
@ stdcall MesEncodeFixedBufferHandleCreate(ptr long ptr ptr)
@ stdcall MesEncodeIncrementalHandleCreate(ptr ptr ptr ptr)
@ stdcall MesHandleFree(ptr)
@ stdcall MesIncrementalHandleReset(ptr ptr ptr ptr ptr long)
@ stdcall MesInqProcEncodingId() rpcrt4.MesInqProcEncodingId
@ stdcall -arch=x86_64 Ndr64AsyncClientCall() rpcrt4.Ndr64AsyncClientCall
@ stdcall -arch=x86_64 Ndr64AsyncServerCall64() rpcrt4.Ndr64AsyncServerCall64
@ stdcall -arch=x86_64 Ndr64AsyncServerCallAll() rpcrt4.Ndr64AsyncServerCallAll
@ stdcall -arch=x86_64 Ndr64DcomAsyncClientCall() rpcrt4.Ndr64DcomAsyncClientCall
@ stdcall -arch=x86_64 Ndr64DcomAsyncStubCall() rpcrt4.Ndr64DcomAsyncStubCall
@ stdcall NDRCContextBinding(ptr)
@ stdcall NDRCContextMarshall(ptr ptr)
@ stdcall NDRCContextUnmarshall(ptr ptr ptr long)
@ stdcall NDRSContextMarshall2(ptr ptr ptr ptr ptr long)
@ stdcall NDRSContextMarshall(ptr ptr ptr)
@ stdcall NDRSContextMarshallEx(ptr ptr ptr ptr)
@ stdcall NDRSContextUnmarshall2(ptr ptr ptr ptr long)
@ stdcall NDRSContextUnmarshall(ptr ptr)
@ stdcall NDRSContextUnmarshallEx(ptr ptr ptr)
@ stdcall NDRcopy() rpcrt4.NDRcopy
@ stdcall NdrAllocate(ptr long)
@ varargs NdrAsyncClientCall(ptr ptr)
@ stdcall NdrAsyncServerCall() rpcrt4.NdrAsyncServerCall
@ stdcall NdrByteCountPointerBufferSize(ptr ptr ptr)
@ stdcall NdrByteCountPointerFree(ptr ptr ptr)
@ stdcall NdrByteCountPointerMarshall(ptr ptr ptr)
@ stdcall NdrByteCountPointerUnmarshall(ptr ptr ptr long)
@ stdcall NdrCStdStubBuffer2_Release(ptr ptr)
@ stdcall NdrCStdStubBuffer_Release(ptr ptr)
@ stdcall NdrClearOutParameters(ptr ptr ptr)
@ varargs -arch=i386 NdrClientCall(ptr ptr) NdrClientCall2
@ varargs NdrClientCall2(ptr ptr)
@ varargs -arch=x86_64 NdrClientCall3(ptr ptr)
@ stdcall NdrClientContextMarshall(ptr ptr long)
@ stdcall NdrClientContextUnmarshall(ptr ptr ptr)
@ stdcall NdrClientInitialize() rpcrt4.NdrClientInitialize
@ stdcall NdrClientInitializeNew(ptr ptr ptr long)
@ stdcall NdrComplexArrayBufferSize(ptr ptr ptr)
@ stdcall NdrComplexArrayFree(ptr ptr ptr)
@ stdcall NdrComplexArrayMarshall(ptr ptr ptr)
@ stdcall NdrComplexArrayMemorySize(ptr ptr)
@ stdcall NdrComplexArrayUnmarshall(ptr ptr ptr long)
@ stdcall NdrComplexStructBufferSize(ptr ptr ptr)
@ stdcall NdrComplexStructFree(ptr ptr ptr)
@ stdcall NdrComplexStructMarshall(ptr ptr ptr)
@ stdcall NdrComplexStructMemorySize(ptr ptr)
@ stdcall NdrComplexStructUnmarshall(ptr ptr ptr long)
@ stdcall NdrConformantArrayBufferSize(ptr ptr ptr)
@ stdcall NdrConformantArrayFree(ptr ptr ptr)
@ stdcall NdrConformantArrayMarshall(ptr ptr ptr)
@ stdcall NdrConformantArrayMemorySize(ptr ptr)
@ stdcall NdrConformantArrayUnmarshall(ptr ptr ptr long)
@ stdcall NdrConformantStringBufferSize(ptr ptr ptr)
@ stdcall NdrConformantStringMarshall(ptr ptr ptr)
@ stdcall NdrConformantStringMemorySize(ptr ptr)
@ stdcall NdrConformantStringUnmarshall(ptr ptr ptr long)
@ stdcall NdrConformantStructBufferSize(ptr ptr ptr)
@ stdcall NdrConformantStructFree(ptr ptr ptr)
@ stdcall NdrConformantStructMarshall(ptr ptr ptr)
@ stdcall NdrConformantStructMemorySize(ptr ptr)
@ stdcall NdrConformantStructUnmarshall(ptr ptr ptr long)
@ stdcall NdrConformantVaryingArrayBufferSize(ptr ptr ptr)
@ stdcall NdrConformantVaryingArrayFree(ptr ptr ptr)
@ stdcall NdrConformantVaryingArrayMarshall(ptr ptr ptr)
@ stdcall NdrConformantVaryingArrayMemorySize(ptr ptr)
@ stdcall NdrConformantVaryingArrayUnmarshall(ptr ptr ptr long)
@ stdcall NdrConformantVaryingStructBufferSize(ptr ptr ptr)
@ stdcall NdrConformantVaryingStructFree(ptr ptr ptr)
@ stdcall NdrConformantVaryingStructMarshall(ptr ptr ptr)
@ stdcall NdrConformantVaryingStructMemorySize(ptr ptr)
@ stdcall NdrConformantVaryingStructUnmarshall(ptr ptr ptr long)
@ stdcall NdrContextHandleInitialize(ptr ptr)
@ stdcall NdrContextHandleSize(ptr ptr ptr)
@ stdcall NdrConvert2(ptr ptr long)
@ stdcall NdrConvert(ptr ptr)
@ stdcall NdrCorrelationFree(ptr)
@ stdcall NdrCorrelationInitialize(ptr ptr long long)
@ stdcall NdrCorrelationPass(ptr)
#@ stdcall NdrCreateServerInterfaceFromStub() rpcrt4.NdrCreateServerInterfaceFromStub
@ stdcall NdrDcomAsyncClientCall() rpcrt4.NdrDcomAsyncClientCall
@ stdcall NdrDcomAsyncStubCall() rpcrt4.NdrDcomAsyncStubCall
@ stdcall NdrDllCanUnloadNow(ptr)
@ stdcall NdrDllGetClassObject(ptr ptr ptr ptr ptr ptr)
@ stdcall NdrDllRegisterProxy(long ptr ptr)
@ stdcall NdrDllUnregisterProxy(long ptr ptr)
@ stdcall NdrEncapsulatedUnionBufferSize(ptr ptr ptr)
@ stdcall NdrEncapsulatedUnionFree(ptr ptr ptr)
@ stdcall NdrEncapsulatedUnionMarshall(ptr ptr ptr)
@ stdcall NdrEncapsulatedUnionMemorySize(ptr ptr)
@ stdcall NdrEncapsulatedUnionUnmarshall(ptr ptr ptr long)
@ stdcall NdrFixedArrayBufferSize(ptr ptr ptr)
@ stdcall NdrFixedArrayFree(ptr ptr ptr)
@ stdcall NdrFixedArrayMarshall(ptr ptr ptr)
@ stdcall NdrFixedArrayMemorySize(ptr ptr)
@ stdcall NdrFixedArrayUnmarshall(ptr ptr ptr long)
@ stdcall NdrFreeBuffer(ptr)
@ stdcall NdrFullPointerFree(ptr ptr)
@ stdcall NdrFullPointerInsertRefId(ptr long ptr)
@ stdcall NdrFullPointerQueryPointer(ptr ptr long ptr)
@ stdcall NdrFullPointerQueryRefId(ptr long long ptr)
@ stdcall NdrFullPointerXlatFree(ptr)
@ stdcall NdrFullPointerXlatInit(long long) 
@ stdcall NdrGetBuffer(ptr long ptr)
@ stdcall NdrGetDcomProtocolVersion() rpcrt4.NdrGetDcomProtocolVersion
@ stdcall NdrGetSimpleTypeBufferAlignment() rpcrt4.NdrGetSimpleTypeBufferAlignment 
@ stdcall NdrGetSimpleTypeBufferSize() rpcrt4.NdrGetSimpleTypeBufferSize 
@ stdcall NdrGetSimpleTypeMemorySize() rpcrt4.NdrGetSimpleTypeMemorySize 
@ stdcall NdrGetTypeFlags() rpcrt4.NdrGetTypeFlags 
@ stdcall NdrGetUserMarshalInfo(ptr long ptr)
@ stdcall NdrInterfacePointerBufferSize(ptr ptr ptr)
@ stdcall NdrInterfacePointerFree(ptr ptr ptr)
@ stdcall NdrInterfacePointerMarshall(ptr ptr ptr)
@ stdcall NdrInterfacePointerMemorySize(ptr ptr)
@ stdcall NdrInterfacePointerUnmarshall(ptr ptr ptr long)
@ stdcall NdrMapCommAndFaultStatus(ptr ptr ptr long)
@ varargs NdrMesProcEncodeDecode(ptr ptr ptr)
@ stdcall NdrMesProcEncodeDecode2() rpcrt4.NdrMesProcEncodeDecode2
@ stdcall -arch=x86_64 NdrMesProcEncodeDecode3() rpcrt4.NdrMesProcEncodeDecode3
@ stdcall NdrMesSimpleTypeAlignSize() rpcrt4.NdrMesSimpleTypeAlignSize
@ stdcall -arch=x86_64 NdrMesSimpleTypeAlignSizeAll() rpcrt4.NdrMesSimpleTypeAlignSizeAll
@ stdcall NdrMesSimpleTypeDecode() rpcrt4.NdrMesSimpleTypeDecode
@ stdcall -arch=x86_64 NdrMesSimpleTypeDecodeAll() rpcrt4.NdrMesSimpleTypeDecodeAll
@ stdcall NdrMesSimpleTypeEncode() rpcrt4.NdrMesSimpleTypeEncode
@ stdcall -arch=x86_64 NdrMesSimpleTypeEncodeAll() rpcrt4.NdrMesSimpleTypeEncodeAll
@ stdcall NdrMesTypeAlignSize2() rpcrt4.NdrMesTypeAlignSize2
@ stdcall -arch=x86_64 NdrMesTypeAlignSize3() rpcrt4.NdrMesTypeAlignSize3
@ stdcall NdrMesTypeAlignSize() rpcrt4.NdrMesTypeAlignSize
@ stdcall NdrMesTypeDecode2(ptr ptr ptr ptr ptr)
@ stdcall -arch=x86_64 NdrMesTypeDecode3(ptr ptr ptr ptr ptr) rpcrt4.NdrMesTypeDecode3
@ stdcall NdrMesTypeDecode() rpcrt4.NdrMesTypeDecode
@ stdcall NdrMesTypeEncode2(ptr ptr ptr ptr ptr)
@ stdcall -arch=x86_64 NdrMesTypeEncode3(ptr ptr ptr ptr ptr) rpcrt4.NdrMesTypeEncode3
@ stdcall NdrMesTypeEncode() rpcrt4.NdrMesTypeEncode
@ stdcall NdrMesTypeFree2(ptr ptr ptr ptr ptr)
@ stdcall -arch=x86_64 NdrMesTypeFree3(ptr ptr ptr ptr ptr) rpcrt4.NdrMesTypeFree3
@ stdcall NdrNonConformantStringBufferSize(ptr ptr ptr)
@ stdcall NdrNonConformantStringMarshall(ptr ptr ptr)
@ stdcall NdrNonConformantStringMemorySize(ptr ptr)
@ stdcall NdrNonConformantStringUnmarshall(ptr ptr ptr long)
@ stdcall NdrNonEncapsulatedUnionBufferSize(ptr ptr ptr)
@ stdcall NdrNonEncapsulatedUnionFree(ptr ptr ptr)
@ stdcall NdrNonEncapsulatedUnionMarshall(ptr ptr ptr)
@ stdcall NdrNonEncapsulatedUnionMemorySize(ptr ptr)
@ stdcall NdrNonEncapsulatedUnionUnmarshall(ptr ptr ptr long)
@ stdcall NdrNsGetBuffer() rpcrt4.NdrNsGetBuffer
@ stdcall NdrNsSendReceive() rpcrt4.NdrNsSendReceive
@ stdcall NdrOleAllocate(long)
@ stdcall NdrOleFree(ptr)
@ stdcall NdrOutInit() rpcrt4.NdrOutInit
@ stdcall NdrPartialIgnoreClientBufferSize() rpcrt4.NdrPartialIgnoreClientBufferSize
@ stdcall NdrPartialIgnoreClientMarshall() rpcrt4.NdrPartialIgnoreClientMarshall 
@ stdcall NdrPartialIgnoreServerInitialize() rpcrt4.NdrPartialIgnoreServerInitialize 
@ stdcall NdrPartialIgnoreServerUnmarshall() rpcrt4.NdrPartialIgnoreServerUnmarshall 
@ stdcall NdrPointerBufferSize(ptr ptr ptr)
@ stdcall NdrPointerFree(ptr ptr ptr)
@ stdcall NdrPointerMarshall(ptr ptr ptr)
@ stdcall NdrPointerMemorySize(ptr ptr)
@ stdcall NdrPointerUnmarshall(ptr ptr ptr long)
@ stdcall NdrProxyErrorHandler(long)
@ stdcall NdrProxyFreeBuffer(ptr ptr)
@ stdcall NdrProxyGetBuffer(ptr ptr)
@ stdcall NdrProxyInitialize(ptr ptr ptr ptr long)
@ stdcall NdrProxySendReceive(ptr ptr)
@ stdcall NdrRangeUnmarshall(ptr ptr ptr long)
@ stdcall NdrRpcSmClientAllocate() rpcrt4.NdrRpcSmClientAllocate
@ stdcall NdrRpcSmClientFree() rpcrt4.NdrRpcSmClientFree
@ stdcall NdrRpcSmSetClientToOsf(ptr)
@ stdcall NdrRpcSsDefaultAllocate() rpcrt4.NdrRpcSsDefaultAllocate
@ stdcall NdrRpcSsDefaultFree() rpcrt4.NdrRpcSsDefaultFree
@ stdcall NdrRpcSsDisableAllocate() rpcrt4.NdrRpcSsDisableAllocate
@ stdcall NdrRpcSsEnableAllocate() rpcrt4.NdrRpcSsEnableAllocate
@ stdcall NdrSendReceive(ptr ptr)
@ stdcall NdrServerCall2(ptr)
@ stdcall -arch=x86_64 NdrServerCallAll() rpcrt4.NdrServerCallAll
@ stdcall -arch=x86_64 NdrServerCallNdr64() rpcrt4.NdrServerCallNdr64
@ stdcall -arch=i386 NdrServerCall(ptr)
@ stdcall NdrServerContextMarshall(ptr ptr long)
@ stdcall NdrServerContextNewMarshall(ptr ptr ptr ptr) 
@ stdcall NdrServerContextNewUnmarshall(ptr ptr) 
@ stdcall NdrServerContextUnmarshall(ptr)
@ stdcall NdrServerInitialize() rpcrt4.NdrServerInitialize
@ stdcall NdrServerInitializeMarshall() rpcrt4.NdrServerInitializeMarshall
@ stdcall NdrServerInitializeNew(ptr ptr ptr)
@ stdcall NdrServerInitializePartial() rpcrt4.NdrServerInitializePartial 
@ stdcall NdrServerInitializeUnmarshall() rpcrt4.NdrServerInitializeUnmarshall
@ stdcall NdrServerMarshall() rpcrt4.NdrServerMarshall
@ stdcall NdrServerUnmarshall() rpcrt4.NdrServerUnmarshall
@ stdcall NdrSimpleStructBufferSize(ptr ptr ptr)
@ stdcall NdrSimpleStructFree(ptr ptr ptr)
@ stdcall NdrSimpleStructMarshall(ptr ptr ptr)
@ stdcall NdrSimpleStructMemorySize(ptr ptr)
@ stdcall NdrSimpleStructUnmarshall(ptr ptr ptr long)
@ stdcall NdrSimpleTypeMarshall(ptr ptr long)
@ stdcall NdrSimpleTypeUnmarshall(ptr ptr long)
@ stdcall NdrStubCall2(ptr ptr ptr ptr)
@ stdcall -arch=x86_64 NdrStubCall3(ptr ptr ptr ptr)
@ stdcall -arch=i386 NdrStubCall(ptr ptr ptr ptr)
@ stdcall NdrStubForwardingFunction(ptr ptr ptr ptr)
@ stdcall NdrStubGetBuffer(ptr ptr ptr)
@ stdcall NdrStubInitialize(ptr ptr ptr ptr)
@ stdcall NdrStubInitializeMarshall() rpcrt4.NdrStubInitializeMarshall
@ stdcall NdrTypeFlags() rpcrt4.NdrTypeFlags 
@ stdcall NdrTypeFree() rpcrt4.NdrTypeFree 
@ stdcall NdrTypeMarshall() rpcrt4.NdrTypeMarshall 
@ stdcall NdrTypeSize() rpcrt4.NdrTypeSize 
@ stdcall NdrTypeUnmarshall() rpcrt4.NdrTypeUnmarshall 
@ stdcall NdrUnmarshallBasetypeInline() rpcrt4.NdrUnmarshallBasetypeInline 
@ stdcall NdrUserMarshalBufferSize(ptr ptr ptr)
@ stdcall NdrUserMarshalFree(ptr ptr ptr)
@ stdcall NdrUserMarshalMarshall(ptr ptr ptr)
@ stdcall NdrUserMarshalMemorySize(ptr ptr)
@ stdcall NdrUserMarshalSimpleTypeConvert() rpcrt4.NdrUserMarshalSimpleTypeConvert
@ stdcall NdrUserMarshalUnmarshall(ptr ptr ptr long)
@ stdcall NdrVaryingArrayBufferSize(ptr ptr ptr)
@ stdcall NdrVaryingArrayFree(ptr ptr ptr)
@ stdcall NdrVaryingArrayMarshall(ptr ptr ptr)
@ stdcall NdrVaryingArrayMemorySize(ptr ptr)
@ stdcall NdrVaryingArrayUnmarshall(ptr ptr ptr long)
@ stdcall NdrXmitOrRepAsBufferSize(ptr ptr ptr)
@ stdcall NdrXmitOrRepAsFree(ptr ptr ptr)
@ stdcall NdrXmitOrRepAsMarshall(ptr ptr ptr)
@ stdcall NdrXmitOrRepAsMemorySize(ptr ptr)
@ stdcall NdrXmitOrRepAsUnmarshall(ptr ptr ptr long)
@ stdcall NdrpCreateProxy() rpcrt4.NdrpCreateProxy 
@ stdcall NdrpCreateStub() rpcrt4.NdrpCreateStub 
@ stdcall NdrpGetProcFormatString() rpcrt4.NdrpGetProcFormatString 
@ stdcall NdrpGetTypeFormatString() rpcrt4.NdrpGetTypeFormatString 
@ stdcall NdrpGetTypeGenCookie() rpcrt4.NdrpGetTypeGenCookie 
@ stdcall NdrpMemoryIncrement() rpcrt4.NdrpMemoryIncrement 
@ stdcall NdrpReleaseTypeFormatString() rpcrt4.NdrpReleaseTypeFormatString 
@ stdcall NdrpReleaseTypeGenCookie () rpcrt4.NdrpReleaseTypeGenCookie
@ stdcall NdrpSetRpcSsDefaults() rpcrt4.NdrpSetRpcSsDefaults
@ stdcall NdrpVarVtOfTypeDesc() rpcrt4.NdrpVarVtOfTypeDesc 
@ stdcall RpcAbortAsyncCall(ptr long) RpcAsyncAbortCall
@ stdcall RpcAsyncAbortCall(ptr long)
@ stdcall RpcAsyncCancelCall(ptr long)
@ stdcall RpcAsyncCompleteCall(ptr ptr)
@ stdcall RpcAsyncGetCallStatus(ptr)
@ stdcall RpcAsyncInitializeHandle(ptr long)
@ stdcall RpcAsyncRegisterInfo() rpcrt4.RpcAsyncRegisterInfo
@ stdcall RpcBindingCopy(ptr ptr)
@ stdcall RpcBindingFree(ptr)
@ stdcall RpcBindingFromStringBindingA(str  ptr)
@ stdcall RpcBindingFromStringBindingW(wstr ptr)
@ stdcall RpcBindingInqAuthClientA(ptr ptr ptr ptr ptr ptr)
@ stdcall RpcBindingInqAuthClientExA(ptr ptr ptr ptr ptr ptr long)
@ stdcall RpcBindingInqAuthClientExW(ptr ptr ptr ptr ptr ptr long)
@ stdcall RpcBindingInqAuthClientW(ptr ptr ptr ptr ptr ptr)
@ stdcall RpcBindingInqAuthInfoA(ptr ptr ptr ptr ptr ptr)
@ stdcall RpcBindingInqAuthInfoExA(ptr ptr ptr ptr ptr ptr long ptr)
@ stdcall RpcBindingInqAuthInfoExW(ptr ptr ptr ptr ptr ptr long ptr)
@ stdcall RpcBindingInqAuthInfoW(ptr ptr ptr ptr ptr ptr)
@ stdcall RpcBindingInqObject(ptr ptr)
@ stdcall RpcBindingInqOption() rpcrt4.RpcBindingInqOption
@ stdcall RpcBindingReset(ptr)
@ stdcall RpcBindingServerFromClient() rpcrt4.RpcBindingServerFromClient
@ stdcall RpcBindingSetAuthInfoA(ptr str long long ptr long)
@ stdcall RpcBindingSetAuthInfoExA(ptr str long long ptr long ptr)
@ stdcall RpcBindingSetAuthInfoExW(ptr wstr long long ptr long ptr)
@ stdcall RpcBindingSetAuthInfoW(ptr wstr long long ptr long)
@ stdcall RpcBindingSetObject(ptr ptr)
@ stdcall RpcBindingSetOption(ptr long long)
@ stdcall RpcBindingToStringBindingA(ptr ptr)
@ stdcall RpcBindingToStringBindingW(ptr ptr)
@ stdcall RpcBindingVectorFree(ptr)
@ stdcall RpcCancelAsyncCall(ptr long) RpcAsyncCancelCall
@ stdcall RpcCancelThread(ptr)
@ stdcall RpcCancelThreadEx(ptr long)
@ stdcall RpcCertGeneratePrincipalNameA() rpcrt4.RpcCertGeneratePrincipalNameA
@ stdcall RpcCertGeneratePrincipalNameW() rpcrt4.RpcCertGeneratePrincipalNameW
@ stdcall RpcCompleteAsyncCall(ptr ptr) RpcAsyncCompleteCall
@ stdcall RpcEpRegisterA(ptr ptr ptr str)
@ stdcall RpcEpRegisterNoReplaceA(ptr ptr ptr str)
@ stdcall RpcEpRegisterNoReplaceW(ptr ptr ptr wstr)
@ stdcall RpcEpRegisterW(ptr ptr ptr wstr)
@ stdcall RpcEpResolveBinding(ptr ptr)
@ stdcall RpcEpUnregister(ptr ptr ptr)
@ stdcall RpcErrorAddRecord() rpcrt4.RpcErrorAddRecord 
@ stdcall RpcErrorClearInformation() rpcrt4.RpcErrorClearInformation 
@ stdcall RpcErrorEndEnumeration(ptr)
@ stdcall RpcErrorGetNextRecord(ptr long ptr)
#@ stdcall RpcErrorGetNumberOfRecords() rpcrt4.RpcErrorGetNumberOfRecords
@ stdcall RpcErrorLoadErrorInfo(ptr long ptr)
@ stdcall RpcErrorResetEnumeration () rpcrt4.RpcErrorResetEnumeration
@ stdcall RpcErrorSaveErrorInfo(ptr ptr ptr)
@ stdcall RpcErrorStartEnumeration(ptr)
@ stdcall RpcFreeAuthorizationContext() rpcrt4.RpcFreeAuthorizationContext 
@ stdcall RpcGetAsyncCallStatus(ptr) RpcAsyncGetCallStatus
@ stdcall RpcGetAuthorizationContextForClient() rpcrt4.RpcGetAuthorizationContextForClient
@ stdcall RpcIfIdVectorFree() rpcrt4.RpcIfIdVectorFree
@ stdcall RpcIfInqId() rpcrt4.RpcIfInqId
@ stdcall RpcImpersonateClient(ptr)
@ stdcall RpcInitializeAsyncHandle(ptr long) RpcAsyncInitializeHandle
@ stdcall RpcMgmtEnableIdleCleanup()
@ stdcall RpcMgmtEpEltInqBegin(ptr long ptr long ptr ptr)
@ stdcall RpcMgmtEpEltInqDone() rpcrt4.RpcMgmtEpEltInqDone
@ stdcall RpcMgmtEpEltInqNextA() rpcrt4.RpcMgmtEpEltInqNextA
@ stdcall RpcMgmtEpEltInqNextW() rpcrt4.RpcMgmtEpEltInqNextW
@ stdcall RpcMgmtEpUnregister() rpcrt4.RpcMgmtEpUnregister
@ stdcall RpcMgmtInqComTimeout() rpcrt4.RpcMgmtInqComTimeout
@ stdcall RpcMgmtInqDefaultProtectLevel() rpcrt4.RpcMgmtInqDefaultProtectLevel
@ stdcall RpcMgmtInqIfIds(ptr ptr)
@ stdcall RpcMgmtInqServerPrincNameA() rpcrt4.RpcMgmtInqServerPrincNameA
@ stdcall RpcMgmtInqServerPrincNameW() rpcrt4.RpcMgmtInqServerPrincNameW
@ stdcall RpcMgmtInqStats(ptr ptr)
@ stdcall RpcMgmtIsServerListening(ptr)
@ stdcall RpcMgmtSetAuthorizationFn(ptr)
@ stdcall RpcMgmtSetCancelTimeout(long)
@ stdcall RpcMgmtSetComTimeout(ptr long)
@ stdcall RpcMgmtSetServerStackSize(long)
@ stdcall RpcMgmtStatsVectorFree(ptr)
@ stdcall RpcMgmtStopServerListening(ptr)
@ stdcall RpcMgmtWaitServerListen()
@ stdcall RpcNetworkInqProtseqsA(ptr)
@ stdcall RpcNetworkInqProtseqsW(ptr)
@ stdcall RpcNetworkIsProtseqValidA(ptr)
@ stdcall RpcNetworkIsProtseqValidW(ptr)
@ stdcall RpcNsBindingInqEntryNameA() rpcrt4.RpcNsBindingInqEntryNameA
@ stdcall RpcNsBindingInqEntryNameW() rpcrt4.RpcNsBindingInqEntryNameW
@ stdcall RpcObjectInqType() rpcrt4.RpcObjectInqType
@ stdcall RpcObjectSetInqFn() rpcrt4.RpcObjectSetInqFn
@ stdcall RpcObjectSetType(ptr ptr)
@ stdcall RpcProtseqVectorFreeA(ptr)
@ stdcall RpcProtseqVectorFreeW(ptr)
@ stdcall RpcRaiseException(long)
@ stdcall RpcRegisterAsyncInfo() rpcrt4.RpcRegisterAsyncInfo
@ stdcall RpcRevertToSelf()
@ stdcall RpcRevertToSelfEx(ptr)
@ stdcall RpcServerInqBindings(ptr)
@ stdcall RpcServerInqCallAttributesA() rpcrt4.RpcServerInqCallAttributesA 
@ stdcall RpcServerInqCallAttributesW() rpcrt4.RpcServerInqCallAttributesW 
@ stdcall RpcServerInqDefaultPrincNameA(long ptr)
@ stdcall RpcServerInqDefaultPrincNameW(long ptr)
@ stdcall RpcServerInqIf() rpcrt4.RpcServerInqIf
@ stdcall RpcServerListen(long long long)
@ stdcall RpcServerRegisterAuthInfoA(str  long ptr ptr)
@ stdcall RpcServerRegisterAuthInfoW(wstr long ptr ptr)
@ stdcall RpcServerRegisterIf2(ptr ptr ptr long long long ptr)
@ stdcall RpcServerRegisterIf(ptr ptr ptr)
@ stdcall RpcServerRegisterIfEx(ptr ptr ptr long long ptr)
@ stdcall RpcServerTestCancel() rpcrt4.RpcServerTestCancel
@ stdcall RpcServerUnregisterIf(ptr ptr long)
@ stdcall RpcServerUnregisterIfEx(ptr ptr long)
@ stdcall RpcServerUseAllProtseqs() rpcrt4.RpcServerUseAllProtseqs
@ stdcall RpcServerUseAllProtseqsEx() rpcrt4.RpcServerUseAllProtseqsEx
@ stdcall RpcServerUseAllProtseqsIf() rpcrt4.RpcServerUseAllProtseqsIf
@ stdcall RpcServerUseAllProtseqsIfEx() rpcrt4.RpcServerUseAllProtseqsIfEx
@ stdcall RpcServerUseProtseqA(str long ptr)
@ stdcall RpcServerUseProtseqEpA(str  long str  ptr)
@ stdcall RpcServerUseProtseqEpExA(str  long str  ptr ptr)
@ stdcall RpcServerUseProtseqEpExW(wstr long wstr ptr ptr)
@ stdcall RpcServerUseProtseqEpW(wstr long wstr ptr) 
@ stdcall RpcServerUseProtseqExA() rpcrt4.RpcServerUseProtseqExA
@ stdcall RpcServerUseProtseqExW() rpcrt4.RpcServerUseProtseqExW
@ stdcall RpcServerUseProtseqIfA() rpcrt4.RpcServerUseProtseqIfA
@ stdcall RpcServerUseProtseqIfExA() rpcrt4.RpcServerUseProtseqIfExA
@ stdcall RpcServerUseProtseqIfExW() rpcrt4.RpcServerUseProtseqIfExW
@ stdcall RpcServerUseProtseqIfW() rpcrt4.RpcServerUseProtseqIfW
@ stdcall RpcServerUseProtseqW(wstr long ptr)
@ stdcall RpcServerYield() rpcrt4.RpcServerYield
@ stdcall RpcSmAllocate() rpcrt4.RpcSmAllocate
@ stdcall RpcSmClientFree() rpcrt4.RpcSmClientFree
@ stdcall RpcSmDestroyClientContext(ptr)
@ stdcall RpcSmDisableAllocate() rpcrt4.RpcSmDisableAllocate
@ stdcall RpcSmEnableAllocate() rpcrt4.RpcSmEnableAllocate
@ stdcall RpcSmFree() rpcrt4.RpcSmFree
@ stdcall RpcSmGetThreadHandle() rpcrt4.RpcSmGetThreadHandle
@ stdcall RpcSmSetClientAllocFree() rpcrt4.RpcSmSetClientAllocFree
@ stdcall RpcSmSetThreadHandle() rpcrt4.RpcSmSetThreadHandle
@ stdcall RpcSmSwapClientAllocFree() rpcrt4.RpcSmSwapClientAllocFree
@ stdcall RpcSsAllocate() rpcrt4.RpcSsAllocate
@ stdcall RpcSsContextLockExclusive() rpcrt4.RpcSsContextLockExclusive 
@ stdcall RpcSsContextLockShared() rpcrt4.RpcSsContextLockShared 
@ stdcall RpcSsDestroyClientContext(ptr)
@ stdcall RpcSsDisableAllocate() rpcrt4.RpcSsDisableAllocate
@ stdcall RpcSsDontSerializeContext() rpcrt4.RpcSsDontSerializeContext
@ stdcall RpcSsEnableAllocate() rpcrt4.RpcSsEnableAllocate
@ stdcall RpcSsFree() rpcrt4.RpcSsFree
@ stdcall RpcSsGetContextBinding() rpcrt4.RpcSsGetContextBinding
@ stdcall RpcSsGetThreadHandle() rpcrt4.RpcSsGetThreadHandle
@ stdcall RpcSsSetClientAllocFree() rpcrt4.RpcSsSetClientAllocFree
@ stdcall RpcSsSetThreadHandle() rpcrt4.RpcSsSetThreadHandle
@ stdcall RpcSsSwapClientAllocFree() rpcrt4.RpcSsSwapClientAllocFree
@ stdcall RpcStringBindingComposeA(str  str  str  str  str  ptr)
@ stdcall RpcStringBindingComposeW(wstr wstr wstr wstr wstr ptr)
@ stdcall RpcStringBindingParseA(str  ptr ptr ptr ptr ptr)
@ stdcall RpcStringBindingParseW(wstr ptr ptr ptr ptr ptr)
@ stdcall RpcStringFreeA(ptr)
@ stdcall RpcStringFreeW(ptr)
@ stdcall RpcTestCancel() rpcrt4.RpcTestCancel
@ stdcall RpcUserFree() rpcrt4.RpcUserFree 
@ stdcall SimpleTypeAlignment() rpcrt4.SimpleTypeAlignment 
@ stdcall SimpleTypeBufferSize() rpcrt4.SimpleTypeBufferSize 
@ stdcall SimpleTypeMemorySize() rpcrt4.SimpleTypeMemorySize 
@ stdcall TowerConstruct(ptr ptr ptr ptr ptr ptr)
@ stdcall TowerExplode(ptr ptr ptr ptr ptr ptr)
@ stdcall UuidCompare(ptr ptr ptr)
@ stdcall UuidCreate(ptr)
@ stdcall UuidCreateNil(ptr)
@ stdcall UuidCreateSequential(ptr) 
@ stdcall UuidEqual(ptr ptr ptr)
@ stdcall UuidFromStringA(str ptr)
@ stdcall UuidFromStringW(wstr ptr)
@ stdcall UuidHash(ptr ptr)
@ stdcall UuidIsNil(ptr ptr)
@ stdcall UuidToStringA(ptr ptr)
@ stdcall UuidToStringW(ptr ptr)
@ stdcall char_array_from_ndr() rpcrt4.char_array_from_ndr
@ stdcall char_from_ndr() rpcrt4.char_from_ndr
#@ stdcall CheckVerificationTrailer(ptr ptr ptr ptr long) rpcrt4.CheckVerificationTrailer
@ stdcall data_from_ndr() rpcrt4.data_from_ndr
@ stdcall data_into_ndr() rpcrt4.data_into_ndr
@ stdcall data_size_ndr() rpcrt4.data_size_ndr
@ stdcall double_array_from_ndr() rpcrt4.double_array_from_ndr
@ stdcall double_from_ndr() rpcrt4.double_from_ndr
@ stdcall enum_from_ndr() rpcrt4.enum_from_ndr
@ stdcall float_array_from_ndr() rpcrt4.float_array_from_ndr
@ stdcall float_from_ndr() rpcrt4.float_from_ndr
@ stdcall long_array_from_ndr() rpcrt4.long_array_from_ndr
@ stdcall long_from_ndr() rpcrt4.long_from_ndr
@ stdcall long_from_ndr_temp() rpcrt4.long_from_ndr_temp
@ stdcall pfnFreeRoutines() rpcrt4.pfnFreeRoutines 
@ stdcall pfnMarshallRoutines() rpcrt4.pfnMarshallRoutines 
@ stdcall pfnSizeRoutines() rpcrt4.pfnSizeRoutines 
@ stdcall pfnUnmarshallRoutines() rpcrt4.pfnUnmarshallRoutines 
@ stdcall short_array_from_ndr() rpcrt4.short_array_from_ndr
@ stdcall short_from_ndr() rpcrt4.short_from_ndr
@ stdcall short_from_ndr_temp() rpcrt4.short_from_ndr_temp
@ stdcall tree_into_ndr() rpcrt4.tree_into_ndr
@ stdcall tree_peek_ndr() rpcrt4.tree_peek_ndr
@ stdcall tree_size_ndr() rpcrt4.tree_size_ndr

#Missing on Win 2k3 SP1
@ stdcall -stub I_RpcSNCHOption(ptr ptr) ;rpcrt4.I_RpcSNCHOption

#Missing on Win 2k3 with Updates
@ stdcall -stub I_RpcBindingIsServerLocal(ptr ptr)

#From Longhorn/Vista
@ stdcall DllRegisterServer()
@ stdcall I_RpcBindingCreateNP(wstr wstr wstr ptr)
@ stub I_RpcCompleteAndFree
@ stdcall I_RpcGetPortAllocationData(ptr)
@ stdcall I_RpcInitHttpImports(ptr)
@ stdcall I_RpcServerStartService(wstr wstr ptr)
@ stdcall I_RpcVerifierCorruptionExpected()
@ stdcall NdrGetBaseInterfaceFromStub(ptr ptr ptr)
@ stdcall RpcBindingBind(ptr ptr ptr)
@ stdcall RpcBindingCreateA(ptr ptr ptr ptr)
@ stdcall RpcBindingCreateW(ptr ptr ptr ptr)
@ stdcall RpcBindingUnbind(ptr)
@ stdcall RpcExceptionFilter(long)
@ stdcall RpcServerCompleteSecurityCallback(ptr long)
@ stdcall RpcServerInqBindingHandle(ptr)
@ stdcall RpcServerSubscribeForNotification(ptr long long ptr)
@ stdcall RpcServerUnsubscribeForNotification(ptr long ptr)
@ stdcall I_RpcBindingInqSecurityContextKeyInfo(ptr ptr)
@ stub I_RpcFwThisIsTheManager
@ stdcall I_RpcServerInqRemoteConnAddress(ptr ptr ptr ptr)
@ stub RpcCertMatchPrincipalName	