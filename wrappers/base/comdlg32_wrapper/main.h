#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

LSTATUS
WINAPI
RegGetValueW(
  _In_ HKEY hkey,
  _In_opt_ LPCWSTR lpSubKey,
  _In_opt_ LPCWSTR lpValue,
  _In_ DWORD dwFlags,
  _Out_opt_ LPDWORD pdwType,
  _When_((dwFlags & 0x7F) == RRF_RT_REG_SZ || (dwFlags & 0x7F) == RRF_RT_REG_EXPAND_SZ ||
    (dwFlags & 0x7F) == (RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ) || *pdwType == REG_SZ ||
    *pdwType == REG_EXPAND_SZ, _Post_z_)
    _When_((dwFlags & 0x7F) == RRF_RT_REG_MULTI_SZ || *pdwType == REG_MULTI_SZ, _Post_ _NullNull_terminated_)
      _Out_writes_bytes_to_opt_(*pcbData,*pcbData) PVOID pvData,
  _Inout_opt_ LPDWORD pcbData);