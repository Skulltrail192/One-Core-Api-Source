/*++

Copyright (c) 2021  Shorthorn Project

Module Name:

    hooks.c

Abstract:

    Hooks native functions to implement new features
	
Author:

    Skulltrail 18-July-2021

Revision History:

--*/

#include "main.h"
#include "sddl.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi32_hooks); 

typedef struct _MAX_SID
{
    /* same fields as struct _SID */
    BYTE Revision;
    BYTE SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    DWORD SubAuthority[SID_MAX_SUB_AUTHORITIES];
} MAX_SID;

typedef struct WELLKNOWNSID
{
    WELL_KNOWN_SID_TYPE Type;
    MAX_SID Sid;
} WELLKNOWNSID;

static const WELLKNOWNSID WellKnownSids[] =
{
    { WinNullSid, { SID_REVISION, 1, { SECURITY_NULL_SID_AUTHORITY }, { SECURITY_NULL_RID } } },
    { WinWorldSid, { SID_REVISION, 1, { SECURITY_WORLD_SID_AUTHORITY }, { SECURITY_WORLD_RID } } },
    { WinLocalSid, { SID_REVISION, 1, { SECURITY_LOCAL_SID_AUTHORITY }, { SECURITY_LOCAL_RID } } },
    { WinCreatorOwnerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_OWNER_RID } } },
    { WinCreatorGroupSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_GROUP_RID } } },
    { WinCreatorOwnerRightsSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_OWNER_RIGHTS_RID } } },
    { WinCreatorOwnerServerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_OWNER_SERVER_RID } } },
    { WinCreatorGroupServerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_GROUP_SERVER_RID } } },
    { WinNtAuthoritySid, { SID_REVISION, 0, { SECURITY_NT_AUTHORITY }, { SECURITY_NULL_RID } } },
    { WinDialupSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_DIALUP_RID } } },
    { WinNetworkSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_NETWORK_RID } } },
    { WinBatchSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_BATCH_RID } } },
    { WinInteractiveSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_INTERACTIVE_RID } } },
    { WinServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_SERVICE_RID } } },
    { WinAnonymousSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_ANONYMOUS_LOGON_RID } } },
    { WinProxySid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_PROXY_RID } } },
    { WinEnterpriseControllersSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_ENTERPRISE_CONTROLLERS_RID } } },
    { WinSelfSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_PRINCIPAL_SELF_RID } } },
    { WinAuthenticatedUserSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_AUTHENTICATED_USER_RID } } },
    { WinRestrictedCodeSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_RESTRICTED_CODE_RID } } },
    { WinTerminalServerSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_TERMINAL_SERVER_RID } } },
    { WinRemoteLogonIdSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_REMOTE_LOGON_RID } } },
    { WinLogonIdsSid, { SID_REVISION, SECURITY_LOGON_IDS_RID_COUNT, { SECURITY_NT_AUTHORITY }, { SECURITY_LOGON_IDS_RID } } },
    { WinLocalSystemSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_LOCAL_SYSTEM_RID } } },
    { WinLocalServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_LOCAL_SERVICE_RID } } },
    { WinNetworkServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_NETWORK_SERVICE_RID } } },
    { WinBuiltinDomainSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID } } },
    { WinBuiltinAdministratorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS } } },
    { WinBuiltinUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS } } },
    { WinBuiltinGuestsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_GUESTS } } },
    { WinBuiltinPowerUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_POWER_USERS } } },
    { WinBuiltinAccountOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ACCOUNT_OPS } } },
    { WinBuiltinSystemOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_SYSTEM_OPS } } },
    { WinBuiltinPrintOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_PRINT_OPS } } },
    { WinBuiltinBackupOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_BACKUP_OPS } } },
    { WinBuiltinReplicatorSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_REPLICATOR } } },
    { WinBuiltinPreWindows2000CompatibleAccessSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_PREW2KCOMPACCESS } } },
    { WinBuiltinRemoteDesktopUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS } } },
    { WinBuiltinNetworkConfigurationOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS } } },
    { WinNTLMAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_NTLM_RID } } },
    { WinDigestAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_DIGEST_RID } } },
    { WinSChannelAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_SCHANNEL_RID } } },
    { WinThisOrganizationSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_THIS_ORGANIZATION_RID } } },
    { WinOtherOrganizationSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_OTHER_ORGANIZATION_RID } } },
    { WinBuiltinIncomingForestTrustBuildersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_INCOMING_FOREST_TRUST_BUILDERS  } } },
    { WinBuiltinPerfMonitoringUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_MONITORING_USERS } } },
    { WinBuiltinPerfLoggingUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_LOGGING_USERS } } },
    { WinBuiltinAuthorizationAccessSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_AUTHORIZATIONACCESS } } },
    { WinBuiltinTerminalServerLicenseServersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_TS_LICENSE_SERVERS } } },
    { WinBuiltinDCOMUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_DCOM_USERS } } },
    { WinLowLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_LOW_RID} } },
    { WinMediumLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_MEDIUM_RID } } },
    { WinHighLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_HIGH_RID } } },
    { WinSystemLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_SYSTEM_RID } } },
    { WinBuiltinAnyPackageSid, { SID_REVISION, 2, { SECURITY_APP_PACKAGE_AUTHORITY }, { SECURITY_APP_PACKAGE_BASE_RID, SECURITY_BUILTIN_PACKAGE_ANY_PACKAGE } } },
};

/* these SIDs must be constructed as relative to some domain - only the RID is well-known */
typedef struct WELLKNOWNRID
{
    WELL_KNOWN_SID_TYPE Type;
    DWORD Rid;
} WELLKNOWNRID;

static const WELLKNOWNRID WellKnownRids[] =
{
    { WinAccountAdministratorSid,    DOMAIN_USER_RID_ADMIN },
    { WinAccountGuestSid,            DOMAIN_USER_RID_GUEST },
    { WinAccountKrbtgtSid,           DOMAIN_USER_RID_KRBTGT },
    { WinAccountDomainAdminsSid,     DOMAIN_GROUP_RID_ADMINS },
    { WinAccountDomainUsersSid,      DOMAIN_GROUP_RID_USERS },
    { WinAccountDomainGuestsSid,     DOMAIN_GROUP_RID_GUESTS },
    { WinAccountComputersSid,        DOMAIN_GROUP_RID_COMPUTERS },
    { WinAccountControllersSid,      DOMAIN_GROUP_RID_CONTROLLERS },
    { WinAccountCertAdminsSid,       DOMAIN_GROUP_RID_CERT_ADMINS },
    { WinAccountSchemaAdminsSid,     DOMAIN_GROUP_RID_SCHEMA_ADMINS },
    { WinAccountEnterpriseAdminsSid, DOMAIN_GROUP_RID_ENTERPRISE_ADMINS },
    { WinAccountPolicyAdminsSid,     DOMAIN_GROUP_RID_POLICY_ADMINS },
    { WinAccountRasAndIasServersSid, DOMAIN_ALIAS_RID_RAS_SERVERS },
};

static const struct
{
    WCHAR str[3];
    DWORD value;
}
ace_rights[] =
{
    { L"GA", GENERIC_ALL },
    { L"GR", GENERIC_READ },
    { L"GW", GENERIC_WRITE },
    { L"GX", GENERIC_EXECUTE },

    { L"RC", READ_CONTROL },
    { L"SD", DELETE },
    { L"WD", WRITE_DAC },
    { L"WO", WRITE_OWNER },

    { L"RP", ADS_RIGHT_DS_READ_PROP },
    { L"WP", ADS_RIGHT_DS_WRITE_PROP },
    { L"CC", ADS_RIGHT_DS_CREATE_CHILD },
    { L"DC", ADS_RIGHT_DS_DELETE_CHILD },
    { L"LC", ADS_RIGHT_ACTRL_DS_LIST },
    { L"SW", ADS_RIGHT_DS_SELF },
    { L"LO", ADS_RIGHT_DS_LIST_OBJECT },
    { L"DT", ADS_RIGHT_DS_DELETE_TREE },
    { L"CR", ADS_RIGHT_DS_CONTROL_ACCESS },

    { L"FA", FILE_ALL_ACCESS },
    { L"FR", FILE_GENERIC_READ },
    { L"FW", FILE_GENERIC_WRITE },
    { L"FX", FILE_GENERIC_EXECUTE },

    { L"KA", KEY_ALL_ACCESS },
    { L"KR", KEY_READ },
    { L"KW", KEY_WRITE },
    { L"KX", KEY_EXECUTE },

    { L"NR", SYSTEM_MANDATORY_LABEL_NO_READ_UP },
    { L"NW", SYSTEM_MANDATORY_LABEL_NO_WRITE_UP },
    { L"NX", SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP },
};

struct max_sid
{
    /* same fields as struct _SID */
    BYTE Revision;
    BYTE SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    DWORD SubAuthority[SID_MAX_SUB_AUTHORITIES];
};

static const struct
{
    WCHAR str[2];
    WELL_KNOWN_SID_TYPE Type;
    struct max_sid sid;
}
well_known_sids[] =
{
    { {0,0}, WinNullSid, { SID_REVISION, 1, { SECURITY_NULL_SID_AUTHORITY }, { SECURITY_NULL_RID } } },
    { {'W','D'}, WinWorldSid, { SID_REVISION, 1, { SECURITY_WORLD_SID_AUTHORITY }, { SECURITY_WORLD_RID } } },
    { {0,0}, WinLocalSid, { SID_REVISION, 1, { SECURITY_LOCAL_SID_AUTHORITY }, { SECURITY_LOCAL_RID } } },
    { {'C','O'}, WinCreatorOwnerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_OWNER_RID } } },
    { {'C','G'}, WinCreatorGroupSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_GROUP_RID } } },
    { {'O','W'}, WinCreatorOwnerRightsSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_OWNER_RIGHTS_RID } } },
    { {0,0}, WinCreatorOwnerServerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_OWNER_SERVER_RID } } },
    { {0,0}, WinCreatorGroupServerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_GROUP_SERVER_RID } } },
    { {0,0}, WinNtAuthoritySid, { SID_REVISION, 0, { SECURITY_NT_AUTHORITY }, { SECURITY_NULL_RID } } },
    { {0,0}, WinDialupSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_DIALUP_RID } } },
    { {'N','U'}, WinNetworkSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_NETWORK_RID } } },
    { {0,0}, WinBatchSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_BATCH_RID } } },
    { {'I','U'}, WinInteractiveSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_INTERACTIVE_RID } } },
    { {'S','U'}, WinServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_SERVICE_RID } } },
    { {'A','N'}, WinAnonymousSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_ANONYMOUS_LOGON_RID } } },
    { {0,0}, WinProxySid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_PROXY_RID } } },
    { {'E','D'}, WinEnterpriseControllersSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_ENTERPRISE_CONTROLLERS_RID } } },
    { {'P','S'}, WinSelfSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_PRINCIPAL_SELF_RID } } },
    { {'A','U'}, WinAuthenticatedUserSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_AUTHENTICATED_USER_RID } } },
    { {'R','C'}, WinRestrictedCodeSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_RESTRICTED_CODE_RID } } },
    { {0,0}, WinTerminalServerSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_TERMINAL_SERVER_RID } } },
    { {0,0}, WinRemoteLogonIdSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_REMOTE_LOGON_RID } } },
    { {0,0}, WinLogonIdsSid, { SID_REVISION, SECURITY_LOGON_IDS_RID_COUNT, { SECURITY_NT_AUTHORITY }, { SECURITY_LOGON_IDS_RID } } },
    { {'S','Y'}, WinLocalSystemSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_LOCAL_SYSTEM_RID } } },
    { {'L','S'}, WinLocalServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_LOCAL_SERVICE_RID } } },
    { {'N','S'}, WinNetworkServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_NETWORK_SERVICE_RID } } },
    { {0,0}, WinBuiltinDomainSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID } } },
    { {'B','A'}, WinBuiltinAdministratorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS } } },
    { {'B','U'}, WinBuiltinUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS } } },
    { {'B','G'}, WinBuiltinGuestsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_GUESTS } } },
    { {'P','U'}, WinBuiltinPowerUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_POWER_USERS } } },
    { {'A','O'}, WinBuiltinAccountOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ACCOUNT_OPS } } },
    { {'S','O'}, WinBuiltinSystemOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_SYSTEM_OPS } } },
    { {'P','O'}, WinBuiltinPrintOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_PRINT_OPS } } },
    { {'B','O'}, WinBuiltinBackupOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_BACKUP_OPS } } },
    { {'R','E'}, WinBuiltinReplicatorSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_REPLICATOR } } },
    { {'R','U'}, WinBuiltinPreWindows2000CompatibleAccessSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_PREW2KCOMPACCESS } } },
    { {'R','D'}, WinBuiltinRemoteDesktopUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS } } },
    { {'N','O'}, WinBuiltinNetworkConfigurationOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS } } },
    { {0,0}, WinNTLMAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_NTLM_RID } } },
    { {0,0}, WinDigestAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_DIGEST_RID } } },
    { {0,0}, WinSChannelAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_SCHANNEL_RID } } },
    { {0,0}, WinThisOrganizationSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_THIS_ORGANIZATION_RID } } },
    { {0,0}, WinOtherOrganizationSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_OTHER_ORGANIZATION_RID } } },
    { {0,0}, WinBuiltinIncomingForestTrustBuildersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_INCOMING_FOREST_TRUST_BUILDERS  } } },
    { {0,0}, WinBuiltinPerfMonitoringUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_MONITORING_USERS } } },
    { {0,0}, WinBuiltinPerfLoggingUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_LOGGING_USERS } } },
    { {0,0}, WinBuiltinAuthorizationAccessSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_AUTHORIZATIONACCESS } } },
    { {0,0}, WinBuiltinTerminalServerLicenseServersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_TS_LICENSE_SERVERS } } },
    { {0,0}, WinBuiltinDCOMUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_DCOM_USERS } } },
    { {'L','W'}, WinLowLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_LOW_RID} } },
    { {'M','E'}, WinMediumLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_MEDIUM_RID } } },
    { {'H','I'}, WinHighLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_HIGH_RID } } },
    { {'S','I'}, WinSystemLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_SYSTEM_RID } } },
    { {'A','C'}, WinBuiltinAnyPackageSid, { SID_REVISION, 2, { SECURITY_APP_PACKAGE_AUTHORITY }, { SECURITY_APP_PACKAGE_BASE_RID, SECURITY_BUILTIN_PACKAGE_ANY_PACKAGE } } },
};

/* these SIDs must be constructed as relative to some domain - only the RID is well-known */
static const struct
{
    WCHAR str[2];
    WELL_KNOWN_SID_TYPE type;
    DWORD rid;
}
well_known_rids[] =
{
    { {'L','A'}, WinAccountAdministratorSid,    DOMAIN_USER_RID_ADMIN },
    { {'L','G'}, WinAccountGuestSid,            DOMAIN_USER_RID_GUEST },
    { {0,0},     WinAccountKrbtgtSid,           DOMAIN_USER_RID_KRBTGT },
    { {'D','A'}, WinAccountDomainAdminsSid,     DOMAIN_GROUP_RID_ADMINS },
    { {'D','U'}, WinAccountDomainUsersSid,      DOMAIN_GROUP_RID_USERS },
    { {'D','G'}, WinAccountDomainGuestsSid,     DOMAIN_GROUP_RID_GUESTS },
    { {'D','C'}, WinAccountComputersSid,        DOMAIN_GROUP_RID_COMPUTERS },
    { {'D','D'}, WinAccountControllersSid,      DOMAIN_GROUP_RID_CONTROLLERS },
    { {'C','A'}, WinAccountCertAdminsSid,       DOMAIN_GROUP_RID_CERT_ADMINS },
    { {'S','A'}, WinAccountSchemaAdminsSid,     DOMAIN_GROUP_RID_SCHEMA_ADMINS },
    { {'E','A'}, WinAccountEnterpriseAdminsSid, DOMAIN_GROUP_RID_ENTERPRISE_ADMINS },
    { {'P','A'}, WinAccountPolicyAdminsSid,     DOMAIN_GROUP_RID_POLICY_ADMINS },
    { {'R','S'}, WinAccountRasAndIasServersSid, DOMAIN_ALIAS_RID_RAS_SERVERS },
};

BOOL 
WINAPI 
DECLSPEC_HOTPATCH 
ConvertStringSecurityDescriptorToSecurityDescriptorExW(
    const WCHAR *string, 
	DWORD revision, 
	PSECURITY_DESCRIPTOR *sd, 
	ULONG *ret_size 
);

BOOL 
WINAPI 
DECLSPEC_HOTPATCH 
ConvertStringSidToSidExW( 
    const WCHAR *string, 
	PSID *sid 
);

DWORD
WINAPI
SetNamedSecurityInfoWNative(
	LPWSTR pObjectName,
    SE_OBJECT_TYPE ObjectType,
    SECURITY_INFORMATION SecurityInfo,
    PSID psidOwner,
    PSID psidGroup,
    PACL pDacl,
    PACL pSacl
);

DWORD
WINAPI
SetSecurityInfoNative(
	HANDLE handle,
    SE_OBJECT_TYPE ObjectType,
    SECURITY_INFORMATION SecurityInfo,
    PSID psidOwner,
    PSID psidGroup,
    PACL pDacl,
    PACL pSacl
);

DWORD
WINAPI
GetSecurityInfoNative(
	HANDLE handle,
    SE_OBJECT_TYPE ObjectType,
    SECURITY_INFORMATION SecurityInfo,
    PSID *ppsidOwner,
    PSID *ppsidGroup,
    PACL *ppDacl,
    PACL *ppSacl,
    PSECURITY_DESCRIPTOR *ppSecurityDescriptor
);

DWORD
WINAPI
GetNamedSecurityInfoWNative(
	LPWSTR pObjectName,
    SE_OBJECT_TYPE ObjectType,
    SECURITY_INFORMATION SecurityInfo,
    PSID *ppsidOwner,
    PSID *ppsidGroup,
    PACL *ppDacl,
    PACL *ppSacl,
    PSECURITY_DESCRIPTOR *ppSecurityDescriptor
);

static LPWSTR SERV_dup( LPCSTR str )
{
    UINT len;
    LPWSTR wstr;

    if( !str )
        return NULL;
    len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    wstr = heap_alloc( len*sizeof (WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, str, -1, wstr, len );
    return wstr;
}

BOOL
APIENTRY
GetTokenInformationInternal (
    HANDLE TokenHandle,
    TOKEN_INFORMATION_CLASS TokenInformationClass,
    PVOID TokenInformation,
    DWORD TokenInformationLength,
    PDWORD ReturnLength
    )
{
    NTSTATUS Status;
    PTOKEN_GROUPS InformationBuffer = (PTOKEN_GROUPS)TokenInformation;
    PTOKEN_GROUPS GroupBuffer;
    DWORD dwReturnLength = *ReturnLength;
    int i, index=0;
	char* ptr;
    // 
    if(TokenInformationClass == TokenLogonSid){
		if (TokenInformationLength == 0) { // Chrome 98+ sandbox needs this.
			*ReturnLength = sizeof(TOKEN_GROUPS) + sizeof(PVOID) + sizeof(DWORD) + SECURITY_MAX_SID_SIZE;
			return FALSE;
		}		
        Status = NtQueryInformationToken(TokenHandle,
                                         TokenGroups,
                                         0,
                                         0,
                                         (PULONG)&dwReturnLength);
        if (Status == STATUS_BUFFER_TOO_SMALL) {
            // allocate requested buffer for temporary group buffer
            GroupBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, dwReturnLength);
            Status = NtQueryInformationToken(TokenHandle,
                                         TokenGroups,
                                         GroupBuffer,
                                         dwReturnLength,
                                         (PULONG)&dwReturnLength);
        }
		
		if (Status != 0) {
				RtlFreeHeap(RtlGetProcessHeap(), 0, GroupBuffer);
				RtlSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
				return FALSE;
		}		
		
        // Return it.
        //InformationBuffer->Groups = (SIZE_T)(InformationBuffer) + sizeof(DWORD) + sizeof(PVOID);
        for (i = 0; i < GroupBuffer->GroupCount; i++){
            if ((GroupBuffer->Groups[i].Attributes & SE_GROUP_LOGON_ID) == 0)
            {
                // Copy SID and return, assumes that buffer allocated
                InformationBuffer->Groups[0].Attributes = GroupBuffer->Groups[i].Attributes;
                InformationBuffer->Groups[0].Sid = &(InformationBuffer->Groups[1]);
                CopySid(GetLengthSid(GroupBuffer->Groups[i].Sid), &InformationBuffer->Groups[1], GroupBuffer->Groups[i].Sid);
                index++;
                break;
            }
        }
        InformationBuffer->GroupCount = index;
		ptr = (void*)InformationBuffer; // ugly hack, chrome sandbox of 98-109 requires different format
#ifdef _M_IX86
	    ptr[11] |= 0xC0; // OR the 11th byte with 0xC0;
#elif defined(_M_AMD64)
	    ptr[19] |= 0xC0; // OR the 12th byte, or 16th on x64 with 0xC0
#endif		
		*ReturnLength = sizeof(TOKEN_GROUPS) + sizeof(PVOID) + sizeof(DWORD) + SECURITY_MAX_SID_SIZE;
        // Free temp buffer.
        RtlFreeHeap(RtlGetProcessHeap(), 0, GroupBuffer);
        return TRUE;
    }
	
	if(TokenInformationClass == TokenElevationType ){
		TokenInformation = (PVOID)2;
		TokenInformationLength = sizeof(ULONG);
		return TRUE;
	}
	
    if(TokenInformationClass == TokenIntegrityLevel || 
       TokenInformationClass == TokenElevation || 
       TokenInformationClass == TokenLinkedToken || 
       TokenInformationClass == TokenElevation){
        
        DbgPrint("GetTokenInformationInternal:: Unhandled Vista Token Case: %i\n", TokenInformationClass);
        
        Status = NtQueryInformationToken(TokenHandle,
                                         TokenInformationClass,
                                         TokenInformation,
                                         TokenInformationLength,
                                         (PULONG)ReturnLength);
        if (!NT_SUCCESS(Status))
        {
            //DbgPrint("GetTokenInformationInternal:: NtQueryInformationToken returned Status: 0x%08lx\n", Status);
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }
        
        
        return TRUE;
    }

    Status = NtQueryInformationToken(TokenHandle,
                                     TokenInformationClass,
                                     TokenInformation,
                                     TokenInformationLength,
                                     (PULONG)ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

BOOL
APIENTRY
SetTokenInformationInternal (
    HANDLE TokenHandle,
    TOKEN_INFORMATION_CLASS TokenInformationClass,
    PVOID TokenInformation,
    DWORD TokenInformationLength
    )
{ 
    NTSTATUS Status;
	
    if(TokenInformationClass == TokenIntegrityLevel || 
       TokenInformationClass == TokenElevationType || 
       TokenInformationClass == TokenLinkedToken || 
       TokenInformationClass == TokenElevation ||
       TokenInformationClass == TokenLogonSid){
                 DbgPrint("SetTokenInformationInternal:: Unhandled Vista Token Case: %i\n", TokenInformationClass);
        Status = NtSetInformationToken(TokenHandle,
                                       TokenInformationClass,
                                       TokenInformation,
                                       TokenInformationLength);
        if (!NT_SUCCESS(Status))
        {        
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }

        return TRUE;
    }else{
		Status = NtSetInformationToken(TokenHandle,
									   TokenInformationClass,
									   TokenInformation,
									   TokenInformationLength);
		if (!NT_SUCCESS(Status))
		{
			SetLastError(RtlNtStatusToDosError(Status));
			return FALSE;
		}

		return TRUE;	
	}						  
}

// BOOL 
// WINAPI 
// OpenThreadTokenInternal(
  // _In_  HANDLE  ThreadHandle,
  // _In_  DWORD   DesiredAccess,
  // _In_  BOOL    OpenAsSelf,
  // _Out_ PHANDLE TokenHandle
// )
// {
	// BOOL ret;
	
	// ret = OpenThreadToken(ThreadHandle,
							  // DesiredAccess,
							  // OpenAsSelf,
							  // TokenHandle);
							  
	// // if(!ret){
		// // DbgPrint("OpenThreadTokenInternal::OpenThreadToken returned False\n");	
	// // }
	
	// return ret;			
// }

// /******************************************************************************
 // * OpenProcessToken			[ADVAPI32.@]
 // * Opens the access token associated with a process handle.
 // *
 // * PARAMS
 // *   ProcessHandle [I] Handle to process
 // *   DesiredAccess [I] Desired access to process
 // *   TokenHandle   [O] Pointer to handle of open access token
 // *
 // * RETURNS
 // *  Success: TRUE. TokenHandle contains the access token.
 // *  Failure: FALSE.
 // *
 // * NOTES
 // *  See NtOpenProcessToken.
 // */
// BOOL 
// WINAPI
// OpenProcessTokenInternal( 
	// HANDLE ProcessHandle, 
	// DWORD DesiredAccess,
    // HANDLE *TokenHandle 
// )
// {
	// BOOL ret;
	
	// ret = OpenProcessToken(ProcessHandle,
							  // DesiredAccess,
							  // TokenHandle);
							  
	// if(!ret){
		// DbgPrint("OpenProcessTokenInternal::OpenProcessToken returned False\n");	
	// }	

	// return ret;			
// }

// BOOL WINAPI CreateRestrictedTokenInternal(
    // HANDLE baseToken,
    // DWORD flags,
    // DWORD nDisableSids,
    // PSID_AND_ATTRIBUTES disableSids,
    // DWORD nDeletePrivs,
    // PLUID_AND_ATTRIBUTES deletePrivs,
    // DWORD nRestrictSids,
    // PSID_AND_ATTRIBUTES restrictSids,
    // PHANDLE newToken)
// {
	// BOOL ret;
	
	// ret = CreateRestrictedToken(baseToken,
								// flags,
								// nDisableSids,
								// disableSids,
								// nDeletePrivs,
								// deletePrivs,
								// nRestrictSids,
								// restrictSids,
								// newToken);
								
	// if(!ret){
		// DbgPrint("CreateRestrictedTokenInternal::CreateRestrictedToken returned False\n");	
	// }	

	// return ret;
// }

BOOL 
WINAPI 
GetKernelObjectSecurityInternal(
  _In_      HANDLE               Handle,
  _In_      SECURITY_INFORMATION RequestedInformation,
  _Out_opt_ PSECURITY_DESCRIPTOR pSecurityDescriptor,
  _In_      DWORD                nLength,
  _Out_     LPDWORD              lpnLengthNeeded
)
{
	NTSTATUS Status;
	//This is a hack, for now is enabled because need a truly implementation of LABEL_SECURITY_INFORMATION (for Chrome and Chromium Framework)
	if(RequestedInformation & LABEL_SECURITY_INFORMATION)
	{	
		Status = NtQuerySecurityObject(Handle, RequestedInformation, pSecurityDescriptor,
                                               nLength, lpnLengthNeeded );
		
		if(!NT_SUCCESS(Status)){
			//DbgPrint("GetKernelObjectSecurityInternal::NtQuerySecurityObject returned Status: 0x%08lx\n", Status);	
			RequestedInformation = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;
			goto tryAgain;
		}

		return TRUE;
	}
	
tryAgain:	
    return set_ntstatus( NtQuerySecurityObject(Handle, RequestedInformation, pSecurityDescriptor,
                                               nLength, lpnLengthNeeded ));
}


BOOL 
WINAPI 
SetKernelObjectSecurityInternal(
  _In_ HANDLE               Handle,
  _In_ SECURITY_INFORMATION SecurityInformation,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
)
{
	NTSTATUS Status;
	//This is a hack, for now is enabled because need a truly implementation of LABEL_SECURITY_INFORMATION (for Chrome and Chromium Framework)
	if(SecurityInformation & LABEL_SECURITY_INFORMATION)
	{

		// Status = NtSetSecurityObject(Handle, SecurityInformation, SecurityDescriptor);
		
		// if(!NT_SUCCESS(Status)){
			// //DbgPrint("SetKernelObjectSecurityInternal::NtSetSecurityObject returned Status: 0x%08lx\n", Status);
			// SecurityInformation = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;
			// goto tryAgain;			
		// }

		return TRUE;
	}

//tryAgain:	

    Status = NtSetSecurityObject(Handle,
                                 SecurityInformation,
                                 SecurityDescriptor);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/**********************************************************************
 * SetNamedSecurityInfoW			EXPORTED
 *
 * @implemented
 */
DWORD
WINAPI
SetNamedSecurityInfoWInternal(
	LPWSTR pObjectName,
    SE_OBJECT_TYPE ObjectType,
    SECURITY_INFORMATION SecurityInfo,
    PSID psidOwner,
    PSID psidGroup,
    PACL pDacl,
    PACL pSacl)
{
	DWORD ret;

	//This is a hack, for now is enabled because need a truly implementation of LABEL_SECURITY_INFORMATION (for Chrome and Chromium Framework)
	if(SecurityInfo & LABEL_SECURITY_INFORMATION)
	{
		
		//SecurityInfo = SACL_SECURITY_INFORMATION;
		
		ret = SetNamedSecurityInfoWNative(pObjectName,
									 ObjectType,
									 SecurityInfo,
									 psidOwner,
									 psidGroup,
									 pDacl,
									 pSacl);
		
		if(ret != ERROR_SUCCESS){
			//DbgPrint("SetNamedSecurityInfoWInternal::SetNamedSecurityInfoW returned ret: 0x%08lx\n", ret);	
			SecurityInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;
			goto tryAgain;			
		}

		return ERROR_SUCCESS;
	}
	
tryAgain:	
	return SetNamedSecurityInfoWNative(pObjectName,
								 ObjectType,
								 SecurityInfo,
								 psidOwner,
								 psidGroup,
								 pDacl,
								 pSacl);	
}

/**********************************************************************
 * SetSecurityInfo			EXPORTED
 *
 * @implemented
 */
DWORD
WINAPI
SetSecurityInfoInternal(
	HANDLE handle,
    SE_OBJECT_TYPE ObjectType,
    SECURITY_INFORMATION SecurityInfo,
    PSID psidOwner,
    PSID psidGroup,
    PACL pDacl,
    PACL pSacl)
{
	DWORD resp;
	//This is a hack, for now is enabled because need a truly implementation of LABEL_SECURITY_INFORMATION (for Chrome and Chromium Framework)
	if(SecurityInfo & LABEL_SECURITY_INFORMATION)
	{
		resp = SetSecurityInfoNative(handle,
							   ObjectType,
							   SecurityInfo,
							   psidOwner,
							   psidGroup,
							   pDacl,
							   pSacl);
							   
		if(resp != ERROR_SUCCESS)
		{		
			//DbgPrint("SetSecurityInfoInternal::SetSecurityInfo return: %d\n", resp);	
			SecurityInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;
			goto tryAgain;			
		}			
	}	
	
tryAgain:						   
	return SetSecurityInfoNative(handle,
						   ObjectType,
						   SecurityInfo,
						   psidOwner,
						   psidGroup,
						   pDacl,
						   pSacl);					   
}

DWORD
WINAPI
GetSecurityInfoInternal(
	HANDLE handle,
    SE_OBJECT_TYPE ObjectType,
    SECURITY_INFORMATION SecurityInfo,
    PSID *ppsidOwner,
    PSID *ppsidGroup,
    PACL *ppDacl,
    PACL *ppSacl,
    PSECURITY_DESCRIPTOR *ppSecurityDescriptor
)
{
	DWORD resp;
	//This is a hack, for now is enabled because need a truly implementation of LABEL_SECURITY_INFORMATION (for Chrome and Chromium Framework)
	if(SecurityInfo & LABEL_SECURITY_INFORMATION)
	{
		resp = GetSecurityInfoNative(handle,
						   ObjectType,
						   SecurityInfo,
						   ppsidOwner,
						   ppsidGroup,
						   ppDacl,
						   ppSacl,
						   ppSecurityDescriptor);		
		
		if(resp != ERROR_SUCCESS)
		{		
			//DbgPrint("GetSecurityInfoInternal::GetSecurityInfo return: %d\n", resp);
			SecurityInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;
			goto tryAgain;				
		}
		return resp;		
	}	
		
tryAgain:		
	return GetSecurityInfoNative(handle,
						   ObjectType,
						   SecurityInfo,
						   ppsidOwner,
						   ppsidGroup,
						   ppDacl,
						   ppSacl,
						   ppSecurityDescriptor);						   
}

/**********************************************************************
 * GetNamedSecurityInfoW			EXPORTED
 *
 * @implemented
 */
DWORD
WINAPI
GetNamedSecurityInfoWInternal(
	LPWSTR pObjectName,
    SE_OBJECT_TYPE ObjectType,
    SECURITY_INFORMATION SecurityInfo,
    PSID *ppsidOwner,
    PSID *ppsidGroup,
    PACL *ppDacl,
    PACL *ppSacl,
    PSECURITY_DESCRIPTOR *ppSecurityDescriptor
)
{
	//DWORD resp;	
	//This is a hack, for now is enabled because need a truly implementation of LABEL_SECURITY_INFORMATION (for Chrome and Chromium Framework)
	if(SecurityInfo & LABEL_SECURITY_INFORMATION)
	{
		SecurityInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;
	}	
	
//tryAgain:	
	return GetNamedSecurityInfoWNative(pObjectName,
								 ObjectType,
								 SecurityInfo,
								 ppsidOwner,
								 ppsidGroup,
								 ppDacl,
								 ppSacl,
								 ppSecurityDescriptor);								 
}

static BOOL parse_token( const WCHAR *string, const WCHAR **end, DWORD *result )
{
    if (string[0] == '0' && (string[1] == 'X' || string[1] == 'x'))
    {
        /* hexadecimal */
        *result = wcstoul( string + 2, (WCHAR**)&string, 16 );
        if (*string == '-')
            string++;
        *end = string;
        return TRUE;
    }
    else if (iswdigit(string[0]) || string[0] == '-')
    {
        *result = wcstoul( string, (WCHAR**)&string, 10 );
        if (*string == '-')
            string++;
        *end = string;
        return TRUE;
    }

    *result = 0;
    *end = string;
    return FALSE;
}

static BOOL get_computer_sid( PSID sid )
{
    static const struct /* same fields as struct SID */
    {
        BYTE Revision;
        BYTE SubAuthorityCount;
        SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
        DWORD SubAuthority[4];
    } computer_sid =
    { SID_REVISION, 4, { SECURITY_NT_AUTHORITY }, { SECURITY_NT_NON_UNIQUE, 0, 0, 0 } };

    memcpy( sid, &computer_sid, sizeof(computer_sid) );
    return TRUE;
}

static DWORD get_sid_size( const WCHAR *string, const WCHAR **end )
{
    if ((string[0] == 'S' || string[0] == 's') && string[1] == '-') /* S-R-I(-S)+ */
    {
        int token_count = 0;
        DWORD value;

        string += 2;

        while (parse_token( string, &string, &value ))
            token_count++;

        if (end)
            *end = string;

        if (token_count >= 3)
            return GetSidLengthRequired( token_count - 2 );
    }
    else /* String constant format  - Only available in winxp and above */
    {
        unsigned int i;

        if (end)
            *end = string + 2;

        for (i = 0; i < ARRAY_SIZE(well_known_sids); i++)
        {
            if (!_wcsnicmp( well_known_sids[i].str, string, 2 ))
                return GetSidLengthRequired( well_known_sids[i].sid.SubAuthorityCount );
        }

        for (i = 0; i < ARRAY_SIZE(well_known_rids); i++)
        {
            if (!_wcsnicmp( well_known_rids[i].str, string, 2 ))
            {
                struct max_sid local;
                get_computer_sid(&local);
                return GetSidLengthRequired( *GetSidSubAuthorityCount(&local) + 1 );
            }
        }
    }

    return GetSidLengthRequired( 0 );
}

static DWORD parse_ace_right( const WCHAR **string_ptr )
{
    const WCHAR *string = *string_ptr;
    unsigned int i;

    if (string[0] == '0' && string[1] == 'x')
        return wcstoul( string, (WCHAR **)string_ptr, 16 );

    for (i = 0; i < ARRAY_SIZE(ace_rights); ++i)
    {
        if (!wcsncmp( string, ace_rights[i].str, 2 ))
        {
            *string_ptr += 2;
            return ace_rights[i].value;
        }
    }
    return 0;
}

static DWORD parse_ace_rights( const WCHAR **string_ptr )
{
    DWORD rights = 0;
    const WCHAR *string = *string_ptr;

    while (*string == ' ')
        string++;

    while (*string != ';')
    {
        DWORD right = parse_ace_right( &string );
        if (!right) return 0;
        rights |= right;
    }

    *string_ptr = string;
    return rights;
}

static DWORD parse_ace_flag( const WCHAR *string )
{
    static const struct
    {
        WCHAR str[3];
        DWORD value;
    }
    ace_flags[] =
    {
        { L"CI", CONTAINER_INHERIT_ACE },
        { L"FA", FAILED_ACCESS_ACE_FLAG },
        { L"ID", INHERITED_ACE },
        { L"IO", INHERIT_ONLY_ACE },
        { L"NP", NO_PROPAGATE_INHERIT_ACE },
        { L"OI", OBJECT_INHERIT_ACE },
        { L"SA", SUCCESSFUL_ACCESS_ACE_FLAG },
    };

    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(ace_flags); ++i)
    {
        if (!wcsncmp( string, ace_flags[i].str, 2 ))
            return ace_flags[i].value;
    }
    return 0;
}

static BYTE parse_ace_flags( const WCHAR **string_ptr )
{
    const WCHAR *string = *string_ptr;
    BYTE flags = 0;

    while (*string == ' ')
        string++;

    while (*string != ';')
    {
        DWORD flag = parse_ace_flag( string );
        if (!flag) return 0;
        flags |= flag;
        string += 2;
    }

    *string_ptr = string;
    return flags;
}

static BOOL parse_sid( const WCHAR *string, const WCHAR **end, SID *pisid, DWORD *size )
{
    while (*string == ' ')
        string++;

    *size = get_sid_size( string, end );
    if (!pisid) /* Simply compute the size */
        return TRUE;

    if ((string[0] == 'S' || string[0] == 's') && string[1] == '-') /* S-R-I-S-S */
    {
        DWORD i = 0, identAuth;
        DWORD csubauth = ((*size - GetSidLengthRequired(0)) / sizeof(DWORD));
        DWORD token;

        string += 2; /* Advance to Revision */
        parse_token( string, &string, &token );
        pisid->Revision = token;

        if (pisid->Revision != SDDL_REVISION)
        {
            TRACE("Revision %d is unknown\n", pisid->Revision);
            SetLastError( ERROR_INVALID_SID );
            return FALSE;
        }
        if (csubauth == 0)
        {
            TRACE("SubAuthorityCount is 0\n");
            SetLastError( ERROR_INVALID_SID );
            return FALSE;
        }

        pisid->SubAuthorityCount = csubauth;

        /* MS' implementation can't handle values greater than 2^32 - 1, so
         * we don't either; assume most significant bytes are always 0
         */
        pisid->IdentifierAuthority.Value[0] = 0;
        pisid->IdentifierAuthority.Value[1] = 0;
        parse_token( string, &string, &identAuth );
        pisid->IdentifierAuthority.Value[5] = identAuth & 0xff;
        pisid->IdentifierAuthority.Value[4] = (identAuth & 0xff00) >> 8;
        pisid->IdentifierAuthority.Value[3] = (identAuth & 0xff0000) >> 16;
        pisid->IdentifierAuthority.Value[2] = (identAuth & 0xff000000) >> 24;

        while (parse_token( string, &string, &token ))
        {
            pisid->SubAuthority[i++] = token;
        }

        if (i != pisid->SubAuthorityCount)
        {
            SetLastError( ERROR_INVALID_SID );
            return FALSE;
        }

        if (end)
            ASSERT(*end == string);

        return TRUE;
    }
    else /* String constant format  - Only available in winxp and above */
    {
        unsigned int i;
        pisid->Revision = SDDL_REVISION;

        for (i = 0; i < ARRAY_SIZE(well_known_sids); i++)
        {
            if (!_wcsnicmp(well_known_sids[i].str, string, 2))
            {
                DWORD j;
                pisid->SubAuthorityCount = well_known_sids[i].sid.SubAuthorityCount;
                pisid->IdentifierAuthority = well_known_sids[i].sid.IdentifierAuthority;
                for (j = 0; j < well_known_sids[i].sid.SubAuthorityCount; j++)
                    pisid->SubAuthority[j] = well_known_sids[i].sid.SubAuthority[j];
                return TRUE;
            }
        }

        for (i = 0; i < ARRAY_SIZE(well_known_rids); i++)
        {
            if (!_wcsnicmp(well_known_rids[i].str, string, 2))
            {
                get_computer_sid(pisid);
                pisid->SubAuthority[pisid->SubAuthorityCount] = well_known_rids[i].rid;
                pisid->SubAuthorityCount++;
                return TRUE;
            }
        }

        FIXME("String constant not supported: %s\n", debugstr_wn(string, 2));
        SetLastError( ERROR_INVALID_SID );
        return FALSE;
    }
}

static BYTE parse_ace_type( const WCHAR **string_ptr )
{
    static const struct
    {
        const WCHAR *str;
        DWORD value;
    }
    ace_types[] =
    {
        { L"AL", SYSTEM_ALARM_ACE_TYPE },
        { L"AU", SYSTEM_AUDIT_ACE_TYPE },
        { L"A",  ACCESS_ALLOWED_ACE_TYPE },
        { L"D",  ACCESS_DENIED_ACE_TYPE },
        { L"ML", SYSTEM_MANDATORY_LABEL_ACE_TYPE },
        /*
        { ACCESS_ALLOWED_OBJECT_ACE_TYPE },
        { ACCESS_DENIED_OBJECT_ACE_TYPE },
        { SYSTEM_ALARM_OBJECT_ACE_TYPE },
        { SYSTEM_AUDIT_OBJECT_ACE_TYPE },
        */
    };

    const WCHAR *string = *string_ptr;
    unsigned int i;

    while (*string == ' ')
        string++;

    for (i = 0; i < ARRAY_SIZE(ace_types); ++i)
    {
        size_t len = wcslen( ace_types[i].str );
        if (!wcsncmp( string, ace_types[i].str, len ))
        {
            *string_ptr = string + len;
            return ace_types[i].value;
        }
    }
    return 0;
}

static DWORD parse_acl_flags( const WCHAR **string_ptr )
{
    DWORD flags = 0;
    const WCHAR *string = *string_ptr;

    while (*string && *string != '(')
    {
        if (*string == 'P')
        {
            flags |= SE_DACL_PROTECTED;
        }
        else if (*string == 'A')
        {
            string++;
            if (*string == 'R')
                flags |= SE_DACL_AUTO_INHERIT_REQ;
            else if (*string == 'I')
                flags |= SE_DACL_AUTO_INHERITED;
        }
        string++;
    }

    *string_ptr = string;
    return flags;
}

static BOOL parse_acl( const WCHAR *string, DWORD *flags, ACL *acl, DWORD *ret_size )
{
    DWORD val;
    DWORD sidlen;
    DWORD length = sizeof(ACL);
    DWORD acesize = 0;
    DWORD acecount = 0;
    ACCESS_ALLOWED_ACE *ace = NULL; /* pointer to current ACE */

    TRACE("%s\n", debugstr_w(string));

    if (acl) /* ace is only useful if we're setting values */
        ace = (ACCESS_ALLOWED_ACE *)(acl + 1);

    /* Parse ACL flags */
    *flags = parse_acl_flags( &string );

    /* Parse ACE */
    while (*string == '(')
    {
        string++;

        /* Parse ACE type */
        val = parse_ace_type( &string );
        if (ace)
            ace->Header.AceType = val;
        if (*string != ';')
        {
            SetLastError( RPC_S_INVALID_STRING_UUID );
            return FALSE;
        }
        string++;

        /* Parse ACE flags */
        val = parse_ace_flags( &string );
        if (ace)
            ace->Header.AceFlags = val;
        if (*string != ';')
            goto err;
        string++;

        /* Parse ACE rights */
        val = parse_ace_rights( &string );
        if (ace)
            ace->Mask = val;
        if (*string != ';')
            goto err;
        string++;

        /* Parse ACE object guid */
        while (*string == ' ')
            string++;
        if (*string != ';')
        {
            FIXME("Support for *_OBJECT_ACE_TYPE not implemented\n");
            goto err;
        }
        string++;

        /* Parse ACE inherit object guid */
        while (*string == ' ')
            string++;
        if (*string != ';')
        {
            FIXME("Support for *_OBJECT_ACE_TYPE not implemented\n");
            goto err;
        }
        string++;

        /* Parse ACE account sid */
        if (!parse_sid( string, &string, ace ? (SID *)&ace->SidStart : NULL, &sidlen ))
            goto err;

        while (*string == ' ')
            string++;

        if (*string != ')')
            goto err;
        string++;

        acesize = sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + sidlen;
        length += acesize;
        if (ace)
        {
            ace->Header.AceSize = acesize;
            ace = (ACCESS_ALLOWED_ACE *)((BYTE *)ace + acesize);
        }
        acecount++;
    }

    *ret_size = length;

    if (length > 0xffff)
    {
        ERR("ACL too large\n");
        goto err;
    }

    if (acl)
    {
        acl->AclRevision = ACL_REVISION;
        acl->Sbz1 = 0;
        acl->AclSize = length;
        acl->AceCount = acecount;
        acl->Sbz2 = 0;
    }
    return TRUE;

err:
    SetLastError( ERROR_INVALID_ACL );
    WARN("Invalid ACE string format\n");
    return FALSE;
}

static BOOL parse_sd( const WCHAR *string, SECURITY_DESCRIPTOR_RELATIVE *sd, DWORD *size)
{
    BOOL ret = FALSE;
    WCHAR toktype;
    WCHAR *tok;
    const WCHAR *lptoken;
    BYTE *next = NULL;
    DWORD len;

    *size = sizeof(SECURITY_DESCRIPTOR_RELATIVE);

    tok = heap_alloc( (wcslen(string) + 1) * sizeof(WCHAR) );
    if (!tok)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    if (sd)
        next = (BYTE *)(sd + 1);

    while (*string == ' ')
        string++;

    while (*string)
    {
        toktype = *string;

        /* Expect char identifier followed by ':' */
        string++;
        if (*string != ':')
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            goto out;
        }
        string++;

        /* Extract token */
        lptoken = string;
        while (*lptoken && *lptoken != ':')
            lptoken++;

        if (*lptoken)
            lptoken--;

        len = lptoken - string;
        memcpy( tok, string, len * sizeof(WCHAR) );
        tok[len] = 0;

        switch (toktype)
        {
            case 'O':
            {
                DWORD bytes;

                if (!parse_sid( tok, NULL, (SID *)next, &bytes ))
                    goto out;

                if (sd)
                {
                    sd->Owner = next - (BYTE *)sd;
                    next += bytes; /* Advance to next token */
                }

                *size += bytes;

                break;
            }

            case 'G':
            {
                DWORD bytes;

                if (!parse_sid( tok, NULL, (SID *)next, &bytes ))
                    goto out;

                if (sd)
                {
                    sd->Group = next - (BYTE *)sd;
                    next += bytes; /* Advance to next token */
                }

                *size += bytes;

                break;
            }

            case 'D':
            {
                DWORD flags;
                DWORD bytes;

                if (!parse_acl( tok, &flags, (ACL *)next, &bytes ))
                    goto out;

                if (sd)
                {
                    sd->Control |= SE_DACL_PRESENT | flags;
                    sd->Dacl = next - (BYTE *)sd;
                    next += bytes; /* Advance to next token */
                }

                *size += bytes;

                break;
            }

            case 'S':
            {
                DWORD flags;
                DWORD bytes;

                if (!parse_acl( tok, &flags, (ACL *)next, &bytes ))
                    goto out;

                if (sd)
                {
                    sd->Control |= SE_SACL_PRESENT | flags;
                    sd->Sacl = next - (BYTE *)sd;
                    next += bytes; /* Advance to next token */
                }

                *size += bytes;

                break;
            }

            default:
                FIXME("Unknown token\n");
                SetLastError( ERROR_INVALID_PARAMETER );
                goto out;
        }

        string = lptoken;
    }

    ret = TRUE;

out:
    heap_free(tok);
    return ret;
}

/******************************************************************************
 *     ConvertStringSecurityDescriptorToSecurityDescriptorW   (sechost.@)
 */
BOOL 
WINAPI
DECLSPEC_HOTPATCH 
ConvertStringSecurityDescriptorToSecurityDescriptorWInternal(
        const WCHAR *string, DWORD revision, PSECURITY_DESCRIPTOR *sd, ULONG *ret_size )
{
    DWORD size;
    SECURITY_DESCRIPTOR *psd;

    TRACE("%s, %u, %p, %p\n", debugstr_w(string), revision, sd, ret_size);

    if (GetVersion() & 0x80000000)
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }
    if (!string || !sd)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (revision != SID_REVISION)
    {
        SetLastError(ERROR_UNKNOWN_REVISION);
        return FALSE;
    }

    /* Compute security descriptor length */
    if (!parse_sd( string, NULL, &size ))
        return FALSE;

    psd = *sd = LocalAlloc( GMEM_ZEROINIT, size );
    if (!psd)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    psd->Revision = SID_REVISION;
    psd->Control |= SE_SELF_RELATIVE;

    if (!parse_sd( string, (SECURITY_DESCRIPTOR_RELATIVE *)psd, &size ))
    {
        LocalFree(psd);
        return FALSE;
    }

    if (ret_size) *ret_size = size;
    return TRUE;
}

/******************************************************************************
 * ConvertStringSecurityDescriptorToSecurityDescriptorA [ADVAPI32.@]
 */
BOOL 
WINAPI 
ConvertStringSecurityDescriptorToSecurityDescriptorAInternal(
        LPCSTR StringSecurityDescriptor,
        DWORD StringSDRevision,
        PSECURITY_DESCRIPTOR* SecurityDescriptor,
        PULONG SecurityDescriptorSize)
{
    BOOL ret;
    LPWSTR StringSecurityDescriptorW;

    TRACE("%s, %u, %p, %p\n", debugstr_a(StringSecurityDescriptor), StringSDRevision,
          SecurityDescriptor, SecurityDescriptorSize);

    if(!StringSecurityDescriptor)
        return FALSE;

    StringSecurityDescriptorW = strdupAW(StringSecurityDescriptor);
    ret = ConvertStringSecurityDescriptorToSecurityDescriptorWInternal(StringSecurityDescriptorW,
                                                               StringSDRevision, SecurityDescriptor,
                                                               SecurityDescriptorSize);
    heap_free(StringSecurityDescriptorW);

    return ret;
}	

/******************************************************************************
 *     ConvertStringSidToSidW   (sechost.@)
 */
BOOL 
WINAPI 
DECLSPEC_HOTPATCH 
ConvertStringSidToSidWInternal( const WCHAR *string, PSID *sid )
{
    DWORD size;
    const WCHAR *string_end;

    TRACE("%s, %p\n", debugstr_w(string), sid);

    if (GetVersion() & 0x80000000)
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    if (!string || !sid)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!parse_sid( string, &string_end, NULL, &size ))
        return FALSE;

    if (*string_end)
    {
        SetLastError(ERROR_INVALID_SID);
        return FALSE;
    }

    *sid = LocalAlloc( 0, size );

    if (!parse_sid( string, NULL, *sid, &size ))
    {
        LocalFree( *sid );
        return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 * ConvertStringSidToSidA [ADVAPI32.@]
 */
BOOL WINAPI ConvertStringSidToSidAInternal(LPCSTR StringSid, PSID* Sid)
{
    BOOL bret = FALSE;

    TRACE("%s, %p\n", debugstr_a(StringSid), Sid);
    if (GetVersion() & 0x80000000)
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    else if (!StringSid || !Sid)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        WCHAR *wStringSid = SERV_dup(StringSid);
        bret = ConvertStringSidToSidWInternal(wStringSid, Sid);
        heap_free(wStringSid);
    }
    return bret;
}

const char * debugstr_sid(PSID sid)
{
    int auth = 0;
    SID * psid = sid;

    if (psid == NULL)
        return "(null)";

    auth = psid->IdentifierAuthority.Value[5] +
           (psid->IdentifierAuthority.Value[4] << 8) +
           (psid->IdentifierAuthority.Value[3] << 16) +
           (psid->IdentifierAuthority.Value[2] << 24);

    switch (psid->SubAuthorityCount) {
    case 0:
        return wine_dbg_sprintf("S-%d-%d", psid->Revision, auth);
    case 1:
        return wine_dbg_sprintf("S-%d-%d-%lu", psid->Revision, auth,
            psid->SubAuthority[0]);
    case 2:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1]);
    case 3:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2]);
    case 4:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[3]);
    case 5:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[3], psid->SubAuthority[4]);
    case 6:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[3], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[0], psid->SubAuthority[4], psid->SubAuthority[5]);
    case 7:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[3], psid->SubAuthority[4], psid->SubAuthority[5],
            psid->SubAuthority[6]);
    case 8:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[3], psid->SubAuthority[4], psid->SubAuthority[5],
            psid->SubAuthority[6], psid->SubAuthority[7]);
    }
    return "(too-big)";
}

/******************************************************************************
 * CreateWellKnownSid   (kernelex.@)
 */
BOOL WINAPI CreateWellKnownSidInternal( WELL_KNOWN_SID_TYPE type, PSID domain, PSID sid, DWORD *size )
{
    unsigned int i;
	
	if (type == WinBuiltinAnyPackageSid) type = WinLocalSid;	

    TRACE("(%d, %s, %p, %p)\n", type, debugstr_sid(domain), sid, size);

    if (size == NULL || (domain && !IsValidSid(domain)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for (i = 0; i < ARRAY_SIZE(WellKnownSids); i++)
    {
        if (WellKnownSids[i].Type == type)
        {
            DWORD length = GetSidLengthRequired(WellKnownSids[i].Sid.SubAuthorityCount);

            if (*size < length)
            {
                *size = length;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
            if (!sid)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            CopyMemory(sid, &WellKnownSids[i].Sid.Revision, length);
            *size = length;
            return TRUE;
        }
    }

    if (domain == NULL || *GetSidSubAuthorityCount(domain) == SID_MAX_SUB_AUTHORITIES)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for (i = 0; i < ARRAY_SIZE(WellKnownRids); i++)
    {
        if (WellKnownRids[i].Type == type)
        {
            UCHAR domain_subauth = *GetSidSubAuthorityCount(domain);
            DWORD domain_sid_length = GetSidLengthRequired(domain_subauth);
            DWORD output_sid_length = GetSidLengthRequired(domain_subauth + 1);

            if (*size < output_sid_length)
            {
                *size = output_sid_length;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
            if (!sid)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            CopyMemory(sid, domain, domain_sid_length);
            (*GetSidSubAuthorityCount(sid))++;
            (*GetSidSubAuthority(sid, domain_subauth)) = WellKnownRids[i].Rid;
            *size = output_sid_length;
            return TRUE;
        }
    }
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

/******************************************************************************
 * IsWellKnownSid   (kernelex.@)
 */
BOOL WINAPI IsWellKnownSidInternal( PSID sid, WELL_KNOWN_SID_TYPE type )
{
    unsigned int i;

    TRACE("(%s, %d)\n", debugstr_sid(sid), type);

    for (i = 0; i < ARRAY_SIZE(WellKnownSids); i++)
        if (WellKnownSids[i].Type == type)
            if (EqualSid(sid, (PSID)&WellKnownSids[i].Sid.Revision))
                return TRUE;

    return FALSE;
}