
@ stdcall LogonIdFromWinStationNameA(ptr ptr ptr)
@ stdcall LogonIdFromWinStationNameW(ptr ptr ptr)
@ stdcall RemoteAssistancePrepareSystemRestore(ptr)
@ stdcall ServerGetInternetConnectorStatus(ptr ptr ptr)
@ stdcall ServerLicensingClose(ptr)
@ stdcall ServerLicensingDeactivateCurrentPolicy(ptr)
@ stdcall ServerLicensingFreePolicyInformation(ptr)
@ stdcall ServerLicensingGetAvailablePolicyIds(ptr ptr ptr)
@ stdcall ServerLicensingGetPolicy(ptr ptr)
@ stdcall ServerLicensingGetPolicyInformationA(ptr ptr ptr ptr)
@ stdcall ServerLicensingGetPolicyInformationW(ptr ptr ptr ptr)
@ stdcall ServerLicensingLoadPolicy(ptr ptr)
@ stdcall ServerLicensingOpenA(ptr)
@ stdcall ServerLicensingOpenW(ptr)
@ stdcall ServerLicensingSetPolicy(ptr ptr ptr)
@ stdcall ServerLicensingUnloadPolicy(ptr ptr)
@ stdcall ServerQueryInetConnectorInformationA(ptr ptr ptr ptr)
@ stdcall ServerQueryInetConnectorInformationW(ptr ptr ptr ptr)
@ stdcall ServerSetInternetConnectorStatus(ptr ptr ptr)
@ stdcall WinStationActivateLicense(ptr ptr ptr ptr)
@ stdcall WinStationAutoReconnect(ptr)
@ stdcall WinStationBroadcastSystemMessage(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall WinStationCheckAccess(ptr ptr ptr) winstabase.WinStationCheckAccess
@ stdcall WinStationCheckLoopBack(ptr ptr ptr ptr)
@ stdcall WinStationCloseServer(ptr ptr ptr ptr)
@ stdcall WinStationConnectA(ptr)
@ stdcall WinStationConnectCallback(ptr ptr ptr ptr ptr)
@ stdcall WinStationConnectW(ptr ptr ptr ptr ptr)
@ stdcall WinStationDisconnect(ptr ptr ptr)
@ stdcall WinStationEnumerateA(ptr ptr ptr)
@ stdcall WinStationEnumerateLicenses(ptr ptr ptr)
@ stdcall WinStationEnumerateProcesses(ptr ptr)
@ stdcall WinStationEnumerateW(ptr ptr ptr)
@ stdcall WinStationEnumerate_IndexedA(ptr ptr ptr ptr ptr)
@ stdcall WinStationEnumerate_IndexedW(ptr ptr ptr ptr ptr)
@ stdcall WinStationFreeGAPMemory(ptr ptr ptr)
@ stdcall WinStationFreeMemory(ptr)
@ stdcall WinStationGenerateLicense(ptr ptr ptr ptr)
@ stdcall WinStationGetAllProcesses(ptr ptr ptr ptr)
@ stdcall WinStationGetLanAdapterNameA(ptr ptr ptr ptr ptr ptr)
@ stdcall WinStationGetLanAdapterNameW(ptr ptr ptr ptr ptr ptr)
@ stdcall WinStationGetMachinePolicy(ptr ptr)
@ stdcall WinStationGetProcessSid(ptr ptr ptr ptr ptr ptr)
@ stdcall WinStationGetTermSrvCountersValue(ptr ptr ptr)
@ stdcall WinStationInstallLicense(ptr ptr ptr)
@ stdcall WinStationIsHelpAssistantSession(ptr ptr)
@ stdcall WinStationNameFromLogonIdA(ptr ptr ptr)
@ stdcall WinStationNameFromLogonIdW(ptr ptr ptr)
@ stdcall WinStationNtsdDebug(ptr ptr ptr ptr ptr)
@ stdcall WinStationOpenServerA(ptr)
@ stdcall WinStationOpenServerW(ptr)
@ stdcall WinStationQueryInformationA(ptr ptr ptr ptr ptr ptr)
@ stdcall WinStationQueryInformationW(ptr ptr ptr ptr ptr ptr)
@ stdcall WinStationQueryLicense(ptr ptr ptr)
@ stdcall WinStationQueryLogonCredentialsW(ptr)
@ stdcall WinStationQueryUpdateRequired(ptr ptr)
@ stdcall WinStationRegisterConsoleNotification(ptr ptr ptr)
@ stdcall WinStationRegisterConsoleNotificationEx(ptr ptr ptr ptr) winstabase.WinStationRegisterConsoleNotificationEx
@ stdcall WinStationRemoveLicense(ptr ptr ptr)
@ stdcall WinStationRenameA(ptr ptr ptr)
@ stdcall WinStationRenameW(ptr ptr ptr)
@ stdcall WinStationReset(ptr ptr ptr)
@ stdcall WinStationSendMessageA(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall WinStationSendMessageW(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall WinStationSendWindowMessage(ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall WinStationServerPing(ptr)
@ stdcall WinStationSetInformationA(ptr ptr ptr ptr ptr)
@ stdcall WinStationSetInformationW(ptr ptr ptr ptr ptr)
@ stdcall WinStationSetPoolCount(ptr ptr ptr)
@ stdcall WinStationShadow(ptr ptr ptr ptr ptr)
@ stdcall WinStationShadowStop(ptr ptr ptr)
@ stdcall WinStationShutdownSystem(ptr ptr)
@ stdcall WinStationTerminateProcess(ptr ptr ptr)
@ stdcall WinStationUnRegisterConsoleNotification(ptr ptr)
@ stdcall WinStationVirtualOpen(ptr ptr ptr)
@ stdcall WinStationWaitSystemEvent(ptr ptr ptr)
@ stdcall _NWLogonQueryAdmin(ptr ptr ptr)
@ stdcall _NWLogonSetAdmin(ptr ptr ptr)
@ stdcall _WinStationAnnoyancePopup(ptr ptr)
@ stdcall _WinStationBeepOpen(ptr ptr ptr)
@ stdcall _WinStationBreakPoint(ptr ptr ptr)
@ stdcall _WinStationCallback(ptr ptr ptr)
@ stdcall _WinStationCheckForApplicationName(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall _WinStationFUSCanRemoteUserDisconnect(ptr ptr ptr)
@ stdcall _WinStationGetApplicationInfo(ptr ptr ptr ptr)
@ stdcall _WinStationNotifyDisconnectPipe()
@ stdcall _WinStationNotifyLogoff()
@ stdcall _WinStationNotifyLogon(ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall _WinStationNotifyNewSession(ptr ptr)
@ stdcall _WinStationReInitializeSecurity(ptr)
@ stdcall _WinStationReadRegistry(ptr)
@ stdcall _WinStationSessionInitialized() winstabase._WinStationSessionInitialized
@ stdcall _WinStationShadowTarget(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall _WinStationShadowTargetSetup(ptr ptr)
@ stdcall _WinStationUpdateClientCachedCredentials(ptr ptr ptr ptr ptr ptr ptr)
@ stdcall _WinStationUpdateSettings(ptr ptr ptr)
@ stdcall _WinStationUpdateUserConfig(ptr)
@ stdcall _WinStationWaitForConnect()

#missing functions
@ stdcall WinStationCanLogonProceed(long wstr wstr) winstabase.WinStationCanLogonProceed
@ stdcall _WinStationOpenSessionDirectory(long long) winstabase._WinStationOpenSessionDirectory 
@ stdcall WinStationRedirectErrorMessage(ptr ptr) winstabase.WinStationRedirectErrorMessage
@ stdcall WinStationRequestSessionsList(ptr ptr ptr) winstabase.WinStationRequestSessionsList ;for Windows XP

#missing on Windows XP
@ stdcall WinStationRegisterNotificationEvent(ptr ptr ptr ptr)
@ stdcall WinStationUnRegisterNotificationEvent(ptr)

#Vista Functions
@ stdcall WinStationConnectEx(ptr ptr)
@ stdcall WinStationEnumerateExW(ptr ptr ptr)
@ stdcall WinStationFreePropertyValue(ptr)
@ stdcall WinStationFreeUserCertificates(ptr) 
@ stdcall WinStationFreeUserCredentials(ptr) 
@ stdcall WinStationGetConnectionProperty(ptr ptr ptr) 
@ stdcall WinStationGetDeviceId(ptr ptr ptr ptr)
@ stdcall WinStationGetInitialApplication(ptr ptr ptr ptr ptr) 
@ stdcall WinStationGetLoggedOnCount(ptr ptr) 
@ stdcall WinStationGetSessionIds(ptr ptr ptr)
@ stdcall WinStationGetUserCertificates(ptr) 
@ stdcall WinStationGetUserCredentials(ptr) 
@ stdcall WinStationGetUserProfile(ptr ptr ptr ptr) 
@ stdcall WinStationIsSessionPermitted() 
@ stdcall WinStationNegotiateSession(ptr ptr ptr ptr ptr ptr ptr)
@ stdcall WinStationQueryAllowConcurrentConnections() 
@ stdcall WinStationQueryEnforcementCore(long ptr ptr ptr long ptr) 
@ stdcall WinStationReportUIResult(ptr ptr ptr)
@ stdcall WinStationRevertFromServicesSession()
@ stdcall WinStationSetAutologonPassword(ptr ptr)
@ stdcall WinStationSetConnectionProperty(ptr ptr ptr)
@ stdcall WinStationSwitchToServicesSession() 
@ stdcall WinStationSystemShutdownStarted() 
@ stdcall WinStationSystemShutdownWait(long ptr) 
@ stdcall WinStationUserLoginAccessCheck(ptr ptr ptr ptr) 
@ stdcall WinStationVerify(ptr ptr ptr ptr ptr ptr)
@ stdcall WinStationVirtualOpenEx(ptr ptr ptr long) 