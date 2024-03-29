/*
 *
 *      Copyright 1997  Marcus Meissner
 *      Copyright 1998  Juergen Schmied
 *      Copyright 2005  Mike McCormack
 *      Copyright 2009  Andrew Hill
 *      Copyright 2013  Dominik Hornung
 *      Copyright 2017  Hermes Belusca-Maito
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES
 *   Nearly complete information about the binary formats
 *   of .lnk files available at http://www.wotsit.org
 *
 *  You can use winedump to examine the contents of a link file:
 *   winedump lnk sc.lnk
 *
 *  MSI advertised shortcuts are totally undocumented.  They provide an
 *   icon for a program that is not yet installed, and invoke MSI to
 *   install the program when the shortcut is clicked on.  They are
 *   created by passing a special string to SetPath, and the information
 *   in that string is parsed an stored.
 */
/*
 * In the following is listed more documentation about the Shell Link file format,
 * as well as its interface.
 *
 * General introduction about "Shell Links" (MSDN):
 *   https://msdn.microsoft.com/en-us/library/windows/desktop/bb776891(v=vs.85).aspx
 *
 *
 * Details of the file format:
 *
 * - Official MSDN documentation "[MS-SHLLINK]: Shell Link (.LNK) Binary File Format":
 *   https://msdn.microsoft.com/en-us/library/dd871305.aspx
 *
 * - Forensics:
 *   http://forensicswiki.org/wiki/LNK
 *   http://computerforensics.parsonage.co.uk/downloads/TheMeaningofLIFE.pdf
 *   https://ithreats.files.wordpress.com/2009/05/lnk_the_windows_shortcut_file_format.pdf
 *   https://github.com/libyal/liblnk/blob/master/documentation/Windows%20Shortcut%20File%20(LNK)%20format.asciidoc
 *
 * - List of possible shell link header flags (SHELL_LINK_DATA_FLAGS enumeration):
 *   https://msdn.microsoft.com/en-us/library/windows/desktop/bb762540(v=vs.85).aspx
 *   https://msdn.microsoft.com/en-us/library/dd891314.aspx
 *
 *
 * In addition to storing its target by using a PIDL, a shell link file also
 * stores metadata to make the shell able to track the link target, in situations
 * where the link target is moved amongst local or network directories, or moved
 * to different volumes. For this, two structures are used:
 *
 * - The first and oldest one (from NewShell/WinNT4) is the "LinkInfo" structure,
 *   stored in a serialized manner at the beginning of the shell link file:
 *   https://msdn.microsoft.com/en-us/library/dd871404.aspx
 *   The official API for manipulating this is located in LINKINFO.DLL .
 *
 * - The second, more recent one, is an extra binary block appended to the
 *   extra-data list of the shell link file: this is the "TrackerDataBlock":
 *   https://msdn.microsoft.com/en-us/library/dd891376.aspx
 *   Its purpose is for link tracking, and works in coordination with the
 *   "Distributed Link Tracking" service ('TrkWks' client, 'TrkSvr' server).
 *   See a detailed explanation at:
 *   http://www.serverwatch.com/tutorials/article.php/1476701/Searching-for-the-Missing-Link-Distributed-Link-Tracking.htm
 *
 *
 * MSI installations most of the time create so-called "advertised shortcuts".
 * They provide an icon for a program that may not be installed yet, and invoke
 * MSI to install the program when the shortcut is opened (resolved).
 * The philosophy of this approach is explained in detail inside the MSDN article
 * "Application Resiliency: Unlock the Hidden Features of Windows Installer"
 * (by Michael Sanford), here:
 *   https://msdn.microsoft.com/en-us/library/aa302344.aspx
 *
 * This functionality is implemented by adding a binary "Darwin" data block
 * of type "EXP_DARWIN_LINK", signature EXP_DARWIN_ID_SIG == 0xA0000006,
 * to the shell link file:
 *   https://msdn.microsoft.com/en-us/library/dd871369.aspx
 * or, this could be done more simply by specifying a special link target path
 * with the IShellLink::SetPath() function. Defining the following GUID:
 *   SHELL32_AdvtShortcutComponent = "::{9db1186e-40df-11d1-aa8c-00c04fb67863}:"
 * setting a target of the form:
 *   "::{SHELL32_AdvtShortcutComponent}:<MSI_App_ID>"
 * would automatically create the necessary binary block.
 *
 * With that, the target of the shortcut now becomes the MSI data. The latter
 * is parsed from MSI and retrieved by the shell that then can run the program.
 *
 * This MSI functionality, dubbed "link blessing", actually originates from an
 * older technology introduced in Internet Explorer 3 (and now obsolete since
 * Internet Explorer 7), called "MS Internet Component Download (MSICD)", see
 * this MSDN introductory article:
 *   https://msdn.microsoft.com/en-us/library/aa741198(v=vs.85).aspx
 * and leveraged in Internet Explorer 4 with "Software Update Channels", see:
 *   https://msdn.microsoft.com/en-us/library/aa740931(v=vs.85).aspx
 * Applications supporting this technology could present shell links having
 * a special target, see subsection "Modifying the Shortcut" in the article:
 *   https://msdn.microsoft.com/en-us/library/aa741201(v=vs.85).aspx#pub_shor
 *
 * Similarly as for the MSI shortcuts, these MSICD shortcuts are created by
 * specifying a special link target path with the IShellLink::SetPath() function,
 * defining the following GUID:
 *   SHELL32_AdvtShortcutProduct = "::{9db1186f-40df-11d1-aa8c-00c04fb67863}:"
 * and setting a target of the form:
 *   "::{SHELL32_AdvtShortcutProduct}:<AppName>::<Path>" .
 * A tool, called "blesslnk.exe", was also provided for automatizing the process;
 * its ReadMe can be found in the (now outdated) MS "Internet Client SDK" (INetSDK,
 * for MS Windows 95 and NT), whose contents can be read at:
 *   http://www.msfn.org/board/topic/145352-new-windows-lnk-vulnerability/?page=4#comment-944223
 * The MS INetSDK can be found at:
 *   https://web.archive.org/web/20100924000013/http://support.microsoft.com/kb/177877
 *
 * Internally the shell link target of these MSICD shortcuts is converted into
 * a binary data block of a type similar to Darwin / "EXP_DARWIN_LINK", but with
 * a different signature EXP_LOGO3_ID_SIG == 0xA0000007 . Such shell links are
 * called "Logo3" shortcuts. They were evoked in this user comment in "The Old
 * New Thing" blog:
 *   https://blogs.msdn.microsoft.com/oldnewthing/20121210-00/?p=5883#comment-1025083
 *
 * The shell exports the API 'SoftwareUpdateMessageBox' (in shdocvw.dll) that
 * displays a message when an update for an application supporting this
 * technology is available.
 *
 */

#include "precomp.h"

#include <appmgmt.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define SHLINK_LOCAL  0
#define SHLINK_REMOTE 1
#define MAX_PROPERTY_SHEET_PAGE 32

/* link file formats */

#include "pshpack1.h"

struct LOCATION_INFO
{
    DWORD        dwTotalSize;
    DWORD        dwHeaderSize;
    DWORD        dwFlags;
    DWORD        dwVolTableOfs;
    DWORD        dwLocalPathOfs;
    DWORD        dwNetworkVolTableOfs;
    DWORD        dwFinalPathOfs;
};

struct LOCAL_VOLUME_INFO
{
    DWORD        dwSize;
    DWORD        dwType;
    DWORD        dwVolSerial;
    DWORD        dwVolLabelOfs;
};

struct volume_info
{
    DWORD        type;
    DWORD        serial;
    WCHAR        label[12];  /* assume 8.3 */
};

#include "poppack.h"

/* IShellLink Implementation */

static HRESULT ShellLink_UpdatePath(LPCWSTR sPathRel, LPCWSTR path, LPCWSTR sWorkDir, LPWSTR* psPath);

/* strdup on the process heap */
static LPWSTR __inline HEAP_strdupAtoW(HANDLE heap, DWORD flags, LPCSTR str)
{
    INT len;
    LPWSTR p;

    assert(str);

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    p = (LPWSTR)HeapAlloc(heap, flags, len * sizeof(WCHAR));
    if (!p)
        return p;
    MultiByteToWideChar(CP_ACP, 0, str, -1, p, len);
    return p;
}

static LPWSTR __inline strdupW(LPCWSTR src)
{
    LPWSTR dest;
    if (!src) return NULL;
    dest = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wcslen(src) + 1) * sizeof(WCHAR));
    if (dest)
        wcscpy(dest, src);
    return dest;
}

// TODO: Use it for constructor & destructor too
VOID CShellLink::Reset()
{
    ILFree(m_pPidl);
    m_pPidl = NULL;

    HeapFree(GetProcessHeap(), 0, m_sPath);
    m_sPath = NULL;
    ZeroMemory(&volume, sizeof(volume));

    HeapFree(GetProcessHeap(), 0, m_sDescription);
    m_sDescription = NULL;
    HeapFree(GetProcessHeap(), 0, m_sPathRel);
    m_sPathRel = NULL;
    HeapFree(GetProcessHeap(), 0, m_sWorkDir);
    m_sWorkDir = NULL;
    HeapFree(GetProcessHeap(), 0, m_sArgs);
    m_sArgs = NULL;
    HeapFree(GetProcessHeap(), 0, m_sIcoPath);
    m_sIcoPath = NULL;

    m_bRunAs = FALSE;
    m_bDirty = FALSE;

    if (m_pDBList)
        SHFreeDataBlockList(m_pDBList);
    m_pDBList = NULL;

    /**/sProduct = sComponent = NULL;/**/
}

CShellLink::CShellLink()
{
    m_Header.dwSize = sizeof(m_Header);
    m_Header.clsid = CLSID_ShellLink;
    m_Header.dwFlags = 0;

    m_Header.dwFileAttributes = 0;
    ZeroMemory(&m_Header.ftCreationTime, sizeof(m_Header.ftCreationTime));
    ZeroMemory(&m_Header.ftLastAccessTime, sizeof(m_Header.ftLastAccessTime));
    ZeroMemory(&m_Header.ftLastWriteTime, sizeof(m_Header.ftLastWriteTime));
    m_Header.nFileSizeLow = 0;

    m_Header.nIconIndex = 0;
    m_Header.nShowCommand = SW_SHOWNORMAL;
    m_Header.wHotKey = 0;

    m_pPidl = NULL;

    m_sPath = NULL;
    ZeroMemory(&volume, sizeof(volume));

    m_sDescription = NULL;
    m_sPathRel = NULL;
    m_sWorkDir = NULL;
    m_sArgs = NULL;
    m_sIcoPath = NULL;
    m_bRunAs = FALSE;
    m_bDirty = FALSE;
    m_pDBList = NULL;

    m_sLinkPath = NULL;
    m_iIdOpen = -1;

    /**/sProduct = sComponent = NULL;/**/
}

CShellLink::~CShellLink()
{
    TRACE("-- destroying IShellLink(%p)\n", this);

    ILFree(m_pPidl);

    HeapFree(GetProcessHeap(), 0, m_sPath);

    HeapFree(GetProcessHeap(), 0, m_sDescription);
    HeapFree(GetProcessHeap(), 0, m_sPathRel);
    HeapFree(GetProcessHeap(), 0, m_sWorkDir);
    HeapFree(GetProcessHeap(), 0, m_sArgs);
    HeapFree(GetProcessHeap(), 0, m_sIcoPath);
    HeapFree(GetProcessHeap(), 0, m_sLinkPath);
    SHFreeDataBlockList(m_pDBList);
}

HRESULT STDMETHODCALLTYPE CShellLink::GetClassID(CLSID *pclsid)
{
    TRACE("%p %p\n", this, pclsid);

    if (pclsid == NULL)
        return E_POINTER;
    *pclsid = CLSID_ShellLink;
    return S_OK;
}

/************************************************************************
 * IPersistStream_IsDirty (IPersistStream)
 */
HRESULT STDMETHODCALLTYPE CShellLink::IsDirty()
{
    TRACE("(%p)\n", this);
    return (m_bDirty ? S_OK : S_FALSE);
}

HRESULT STDMETHODCALLTYPE CShellLink::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    TRACE("(%p, %s, %x)\n", this, debugstr_w(pszFileName), dwMode);

    if (dwMode == 0)
        dwMode = STGM_READ | STGM_SHARE_DENY_WRITE;

    CComPtr<IStream> stm;
    HRESULT hr = SHCreateStreamOnFileW(pszFileName, dwMode, &stm);
    if (SUCCEEDED(hr))
    {
        HeapFree(GetProcessHeap(), 0, m_sLinkPath);
        m_sLinkPath = strdupW(pszFileName);
        hr = Load(stm);
        ShellLink_UpdatePath(m_sPathRel, pszFileName, m_sWorkDir, &m_sPath);
        m_bDirty = FALSE;
    }
    TRACE("-- returning hr %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CShellLink::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    TRACE("(%p)->(%s)\n", this, debugstr_w(pszFileName));

    if (!pszFileName)
        return E_FAIL;

    CComPtr<IStream> stm;
    HRESULT hr = SHCreateStreamOnFileW(pszFileName, STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE, &stm);
    if (SUCCEEDED(hr))
    {
        hr = Save(stm, FALSE);

        if (SUCCEEDED(hr))
        {
            if (m_sLinkPath)
                HeapFree(GetProcessHeap(), 0, m_sLinkPath);

            m_sLinkPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wcslen(pszFileName) + 1) * sizeof(WCHAR));
            if (m_sLinkPath)
                wcscpy(m_sLinkPath, pszFileName);

            m_bDirty = FALSE;
        }
        else
        {
            DeleteFileW(pszFileName);
            WARN("Failed to create shortcut %s\n", debugstr_w(pszFileName));
        }
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CShellLink::SaveCompleted(LPCOLESTR pszFileName)
{
    FIXME("(%p)->(%s)\n", this, debugstr_w(pszFileName));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::GetCurFile(LPOLESTR *ppszFileName)
{
    *ppszFileName = NULL;

    if (!m_sLinkPath)
    {
        /* IPersistFile::GetCurFile called before IPersistFile::Save */
        return S_FALSE;
    }

    *ppszFileName = (LPOLESTR)CoTaskMemAlloc((wcslen(m_sLinkPath) + 1) * sizeof(WCHAR));
    if (!*ppszFileName)
    {
        /* out of memory */
        return E_OUTOFMEMORY;
    }

    /* copy last saved filename */
    wcscpy(*ppszFileName, m_sLinkPath);

    return S_OK;
}

static HRESULT Stream_LoadString(IStream* stm, BOOL unicode, LPWSTR *pstr)
{
    TRACE("%p\n", stm);

    USHORT len;
    DWORD count = 0;
    HRESULT hr = stm->Read(&len, sizeof(len), &count);
    if (FAILED(hr) || count != sizeof(len))
        return E_FAIL;

    if (unicode)
        len *= sizeof(WCHAR);

    TRACE("reading %d\n", len);
    LPSTR temp = (LPSTR)HeapAlloc(GetProcessHeap(), 0, len + sizeof(WCHAR));
    if (!temp)
        return E_OUTOFMEMORY;
    count = 0;
    hr = stm->Read(temp, len, &count);
    if (FAILED(hr) || count != len)
    {
        HeapFree(GetProcessHeap(), 0, temp);
        return E_FAIL;
    }

    TRACE("read %s\n", debugstr_an(temp, len));

    /* convert to unicode if necessary */
    LPWSTR str;
    if (!unicode)
    {
        count = MultiByteToWideChar(CP_ACP, 0, temp, len, NULL, 0);
        str = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (count + 1) * sizeof(WCHAR));
        if (!str)
        {
            HeapFree(GetProcessHeap(), 0, temp);
            return E_OUTOFMEMORY;
        }
        MultiByteToWideChar(CP_ACP, 0, temp, len, str, count);
        HeapFree(GetProcessHeap(), 0, temp);
    }
    else
    {
        count /= sizeof(WCHAR);
        str = (LPWSTR)temp;
    }
    str[count] = 0;

    *pstr = str;

    return S_OK;
}


/*
 * NOTE: The following 5 functions are part of LINKINFO.DLL
 */
static BOOL ShellLink_GetVolumeInfo(LPCWSTR path, CShellLink::volume_info *volume)
{
    WCHAR drive[4] = { path[0], ':', '\\', 0 };

    volume->type = GetDriveTypeW(drive);
    BOOL bRet = GetVolumeInformationW(drive, volume->label, _countof(volume->label), &volume->serial, NULL, NULL, NULL, 0);
    TRACE("ret = %d type %d serial %08x name %s\n", bRet,
          volume->type, volume->serial, debugstr_w(volume->label));
    return bRet;
}

static HRESULT Stream_ReadChunk(IStream* stm, LPVOID *data)
{
    struct sized_chunk
    {
        DWORD size;
        unsigned char data[1];
    } *chunk;

    TRACE("%p\n", stm);

    DWORD size;
    ULONG count;
    HRESULT hr = stm->Read(&size, sizeof(size), &count);
    if (FAILED(hr) || count != sizeof(size))
        return E_FAIL;

    chunk = static_cast<sized_chunk *>(HeapAlloc(GetProcessHeap(), 0, size));
    if (!chunk)
        return E_OUTOFMEMORY;

    chunk->size = size;
    hr = stm->Read(chunk->data, size - sizeof(size), &count);
    if (FAILED(hr) || count != (size - sizeof(size)))
    {
        HeapFree(GetProcessHeap(), 0, chunk);
        return E_FAIL;
    }

    TRACE("Read %d bytes\n", chunk->size);

    *data = chunk;

    return S_OK;
}

static BOOL Stream_LoadVolume(LOCAL_VOLUME_INFO *vol, CShellLink::volume_info *volume)
{
    volume->serial = vol->dwVolSerial;
    volume->type = vol->dwType;

    if (!vol->dwVolLabelOfs)
        return FALSE;
    if (vol->dwSize <= vol->dwVolLabelOfs)
        return FALSE;
    INT len = vol->dwSize - vol->dwVolLabelOfs;

    LPSTR label = (LPSTR)vol;
    label += vol->dwVolLabelOfs;
    MultiByteToWideChar(CP_ACP, 0, label, len, volume->label, _countof(volume->label));

    return TRUE;
}

static LPWSTR Stream_LoadPath(LPCSTR p, DWORD maxlen)
{
    UINT len = 0;

    while (p[len] && len < maxlen)
        len++;

    UINT wlen = MultiByteToWideChar(CP_ACP, 0, p, len, NULL, 0);
    LPWSTR path = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wlen + 1) * sizeof(WCHAR));
    if (!path)
        return NULL;
    MultiByteToWideChar(CP_ACP, 0, p, len, path, wlen);
    path[wlen] = 0;

    return path;
}

static HRESULT Stream_LoadLocation(IStream *stm,
                                   CShellLink::volume_info *volume, LPWSTR *path)
{
    char *p = NULL;
    HRESULT hr = Stream_ReadChunk(stm, (LPVOID*) &p);
    if (FAILED(hr))
        return hr;

    LOCATION_INFO *loc = reinterpret_cast<LOCATION_INFO *>(p);
    if (loc->dwTotalSize < sizeof(LOCATION_INFO))
    {
        HeapFree(GetProcessHeap(), 0, p);
        return E_FAIL;
    }

    /* if there's valid local volume information, load it */
    if (loc->dwVolTableOfs &&
        ((loc->dwVolTableOfs + sizeof(LOCAL_VOLUME_INFO)) <= loc->dwTotalSize))
    {
        LOCAL_VOLUME_INFO *volume_info;

        volume_info = (LOCAL_VOLUME_INFO*) &p[loc->dwVolTableOfs];
        Stream_LoadVolume(volume_info, volume);
    }

    /* if there's a local path, load it */
    DWORD n = loc->dwLocalPathOfs;
    if (n && n < loc->dwTotalSize)
        *path = Stream_LoadPath(&p[n], loc->dwTotalSize - n);

    TRACE("type %d serial %08x name %s path %s\n", volume->type,
          volume->serial, debugstr_w(volume->label), debugstr_w(*path));

    HeapFree(GetProcessHeap(), 0, p);
    return S_OK;
}


/*
 * The format of the advertised shortcut info is:
 *
 *  Offset     Description
 *  ------     -----------
 *    0          Length of the block (4 bytes, usually 0x314)
 *    4          tag (dword)
 *    8          string data in ASCII
 *    8+0x104    string data in UNICODE
 *
 * In the original Win32 implementation the buffers are not initialized
 * to zero, so data trailing the string is random garbage.
 */
HRESULT CShellLink::GetAdvertiseInfo(LPWSTR *str, DWORD dwSig)
{
    LPEXP_DARWIN_LINK pInfo;

    *str = NULL;

    pInfo = (LPEXP_DARWIN_LINK)SHFindDataBlock(m_pDBList, dwSig);
    if (!pInfo)
        return E_FAIL;

    /* Make sure that the size of the structure is valid */
    if (pInfo->dbh.cbSize != sizeof(*pInfo))
    {
        ERR("Ooops. This structure is not as expected...\n");
        return E_FAIL;
    }

    TRACE("dwSig %08x  string = '%s'\n", pInfo->dbh.dwSignature, debugstr_w(pInfo->szwDarwinID));

    *str = pInfo->szwDarwinID;
    return S_OK;
}

/************************************************************************
 * IPersistStream_Load (IPersistStream)
 */
HRESULT STDMETHODCALLTYPE CShellLink::Load(IStream *stm)
{
    TRACE("%p %p\n", this, stm);

    if (!stm)
        return STG_E_INVALIDPOINTER;

    /* Free all the old stuff */
    Reset();

    ULONG dwBytesRead = 0;
    HRESULT hr = stm->Read(&m_Header, sizeof(m_Header), &dwBytesRead);
    if (FAILED(hr))
        return hr;

    if (dwBytesRead != sizeof(m_Header))
        return E_FAIL;
    if (m_Header.dwSize != sizeof(m_Header))
        return E_FAIL;
    if (!IsEqualIID(m_Header.clsid, CLSID_ShellLink))
        return E_FAIL;

    /* Load the new data in order */

    if (TRACE_ON(shell))
    {
        SYSTEMTIME stCreationTime;
        SYSTEMTIME stLastAccessTime;
        SYSTEMTIME stLastWriteTime;
        WCHAR sTemp[MAX_PATH];

        FileTimeToSystemTime(&m_Header.ftCreationTime, &stCreationTime);
        FileTimeToSystemTime(&m_Header.ftLastAccessTime, &stLastAccessTime);
        FileTimeToSystemTime(&m_Header.ftLastWriteTime, &stLastWriteTime);

        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &stCreationTime,
                       NULL, sTemp, _countof(sTemp));
        TRACE("-- stCreationTime: %s\n", debugstr_w(sTemp));
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &stLastAccessTime,
                       NULL, sTemp, _countof(sTemp));
        TRACE("-- stLastAccessTime: %s\n", debugstr_w(sTemp));
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &stLastWriteTime,
                       NULL, sTemp, _countof(sTemp));
        TRACE("-- stLastWriteTime: %s\n", debugstr_w(sTemp));
    }

    /* load all the new stuff */
    if (m_Header.dwFlags & SLDF_HAS_ID_LIST)
    {
        hr = ILLoadFromStream(stm, &m_pPidl);
        if (FAILED(hr))
            return hr;
    }
    pdump(m_pPidl);

    /* Load the location information... */
    if (m_Header.dwFlags & SLDF_HAS_LINK_INFO)
    {
        hr = Stream_LoadLocation(stm, &volume, &m_sPath);
        if (FAILED(hr))
            return hr;
    }
    /* ... but if it is required not to use it, clear it */
    if (m_Header.dwFlags & SLDF_FORCE_NO_LINKINFO)
    {
        HeapFree(GetProcessHeap(), 0, m_sPath);
        m_sPath = NULL;
        ZeroMemory(&volume, sizeof(volume));
    }

    BOOL unicode = !!(m_Header.dwFlags & SLDF_UNICODE);

    if (m_Header.dwFlags & SLDF_HAS_NAME)
    {
        hr = Stream_LoadString(stm, unicode, &m_sDescription);
        if (FAILED(hr))
            return hr;
        TRACE("Description  -> %s\n", debugstr_w(m_sDescription));
    }

    if (m_Header.dwFlags & SLDF_HAS_RELPATH)
    {
        hr = Stream_LoadString(stm, unicode, &m_sPathRel);
        if (FAILED(hr))
            return hr;
        TRACE("Relative Path-> %s\n", debugstr_w(m_sPathRel));
    }

    if (m_Header.dwFlags & SLDF_HAS_WORKINGDIR)
    {
        hr = Stream_LoadString(stm, unicode, &m_sWorkDir);
        if (FAILED(hr))
            return hr;
        PathRemoveBackslash(m_sWorkDir);
        TRACE("Working Dir  -> %s\n", debugstr_w(m_sWorkDir));
    }

    if (m_Header.dwFlags & SLDF_HAS_ARGS)
    {
        hr = Stream_LoadString(stm, unicode, &m_sArgs);
        if (FAILED(hr))
            return hr;
        TRACE("Arguments    -> %s\n", debugstr_w(m_sArgs));
    }

    if (m_Header.dwFlags & SLDF_HAS_ICONLOCATION)
    {
        hr = Stream_LoadString(stm, unicode, &m_sIcoPath);
        if (FAILED(hr))
            return hr;
        TRACE("Icon file    -> %s\n", debugstr_w(m_sIcoPath));
    }

    /* Now load the optional data block list */
    hr = SHReadDataBlockList(stm, &m_pDBList);
    if (FAILED(hr)) // FIXME: Should we fail?
        return hr;

    if (TRACE_ON(shell))
    {
#if (NTDDI_VERSION < NTDDI_LONGHORN)
        if (m_Header.dwFlags & SLDF_HAS_LOGO3ID)
        {
            hr = GetAdvertiseInfo(&sProduct, EXP_LOGO3_ID_SIG);
            if (SUCCEEDED(hr))
                TRACE("Product      -> %s\n", debugstr_w(sProduct));
        }
#endif
        if (m_Header.dwFlags & SLDF_HAS_DARWINID)
        {
            hr = GetAdvertiseInfo(&sComponent, EXP_DARWIN_ID_SIG);
            if (SUCCEEDED(hr))
                TRACE("Component    -> %s\n", debugstr_w(sComponent));
        }
    }

    if (m_Header.dwFlags & SLDF_RUNAS_USER)
        m_bRunAs = TRUE;
    else
        m_bRunAs = FALSE;

    TRACE("OK\n");

    pdump(m_pPidl);

    return S_OK;
}

/************************************************************************
 * Stream_WriteString
 *
 * Helper function for IPersistStream_Save. Writes a unicode string
 *  with terminating nul byte to a stream, preceded by the its length.
 */
static HRESULT Stream_WriteString(IStream* stm, LPCWSTR str)
{
    USHORT len = wcslen(str) + 1; // FIXME: Possible overflows?
    DWORD count;

    HRESULT hr = stm->Write(&len, sizeof(len), &count);
    if (FAILED(hr))
        return hr;

    len *= sizeof(WCHAR);

    hr = stm->Write(str, len, &count);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

/************************************************************************
 * Stream_WriteLocationInfo
 *
 * Writes the location info to a stream
 *
 * FIXME: One day we might want to write the network volume information
 *        and the final path.
 *        Figure out how Windows deals with unicode paths here.
 */
static HRESULT Stream_WriteLocationInfo(IStream* stm, LPCWSTR path,
        CShellLink::volume_info *volume)
{
    LOCAL_VOLUME_INFO *vol;
    LOCATION_INFO *loc;

    TRACE("%p %s %p\n", stm, debugstr_w(path), volume);

    /* figure out the size of everything */
    DWORD label_size = WideCharToMultiByte(CP_ACP, 0, volume->label, -1,
                                      NULL, 0, NULL, NULL);
    DWORD path_size = WideCharToMultiByte(CP_ACP, 0, path, -1,
                                     NULL, 0, NULL, NULL);
    DWORD volume_info_size = sizeof(*vol) + label_size;
    DWORD final_path_size = 1;
    DWORD total_size = sizeof(*loc) + volume_info_size + path_size + final_path_size;

    /* create pointers to everything */
    loc = static_cast<LOCATION_INFO *>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, total_size));
    vol = (LOCAL_VOLUME_INFO*) &loc[1];
    LPSTR szLabel = (LPSTR) &vol[1];
    LPSTR szPath = &szLabel[label_size];
    LPSTR szFinalPath = &szPath[path_size];

    /* fill in the location information header */
    loc->dwTotalSize = total_size;
    loc->dwHeaderSize = sizeof(*loc);
    loc->dwFlags = 1;
    loc->dwVolTableOfs = sizeof(*loc);
    loc->dwLocalPathOfs = sizeof(*loc) + volume_info_size;
    loc->dwNetworkVolTableOfs = 0;
    loc->dwFinalPathOfs = sizeof(*loc) + volume_info_size + path_size;

    /* fill in the volume information */
    vol->dwSize = volume_info_size;
    vol->dwType = volume->type;
    vol->dwVolSerial = volume->serial;
    vol->dwVolLabelOfs = sizeof(*vol);

    /* copy in the strings */
    WideCharToMultiByte(CP_ACP, 0, volume->label, -1,
                         szLabel, label_size, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, path, -1,
                         szPath, path_size, NULL, NULL);
    *szFinalPath = 0;

    ULONG count = 0;
    HRESULT hr = stm->Write(loc, total_size, &count);
    HeapFree(GetProcessHeap(), 0, loc);

    return hr;
}

/************************************************************************
 * IPersistStream_Save (IPersistStream)
 *
 * FIXME: makes assumptions about byte order
 */
HRESULT STDMETHODCALLTYPE CShellLink::Save(IStream *stm, BOOL fClearDirty)
{
    TRACE("%p %p %x\n", this, stm, fClearDirty);

    m_Header.dwSize = sizeof(m_Header);
    m_Header.clsid = CLSID_ShellLink;

    /*
     * Reset the flags: keep only the flags related to data blocks as they were
     * already set in accordance by the different mutator member functions.
     * The other flags will be determined now by the presence or absence of data.
     */
    m_Header.dwFlags &= (SLDF_RUN_WITH_SHIMLAYER | SLDF_RUNAS_USER |
                         SLDF_RUN_IN_SEPARATE | SLDF_HAS_DARWINID |
#if (NTDDI_VERSION < NTDDI_LONGHORN)
                         SLDF_HAS_LOGO3ID |
#endif
                         SLDF_HAS_EXP_ICON_SZ | SLDF_HAS_EXP_SZ);
    // TODO: When we will support Vista+ functionality, add other flags to this list.

    /* The stored strings are in UNICODE */
    m_Header.dwFlags |= SLDF_UNICODE;

    if (m_pPidl)
        m_Header.dwFlags |= SLDF_HAS_ID_LIST;
    if (m_sPath)
        m_Header.dwFlags |= SLDF_HAS_LINK_INFO;
    if (m_sDescription && *m_sDescription)
        m_Header.dwFlags |= SLDF_HAS_NAME;
    if (m_sPathRel && *m_sPathRel)
        m_Header.dwFlags |= SLDF_HAS_RELPATH;
    if (m_sWorkDir && *m_sWorkDir)
        m_Header.dwFlags |= SLDF_HAS_WORKINGDIR;
    if (m_sArgs && *m_sArgs)
        m_Header.dwFlags |= SLDF_HAS_ARGS;
    if (m_sIcoPath && *m_sIcoPath)
        m_Header.dwFlags |= SLDF_HAS_ICONLOCATION;
    if (m_bRunAs)
        m_Header.dwFlags |= SLDF_RUNAS_USER;

    /* Write the shortcut header */
    ULONG count;
    HRESULT hr = stm->Write(&m_Header, sizeof(m_Header), &count);
    if (FAILED(hr))
    {
        ERR("Write failed\n");
        return hr;
    }

    /* Save the data in order */

    if (m_pPidl)
    {
        hr = ILSaveToStream(stm, m_pPidl);
        if (FAILED(hr))
        {
            ERR("Failed to write PIDL\n");
            return hr;
        }
    }

    if (m_sPath)
    {
        hr = Stream_WriteLocationInfo(stm, m_sPath, &volume);
        if (FAILED(hr))
            return hr;
    }

    if (m_Header.dwFlags & SLDF_HAS_NAME)
    {
        hr = Stream_WriteString(stm, m_sDescription);
        if (FAILED(hr))
            return hr;
    }

    if (m_Header.dwFlags & SLDF_HAS_RELPATH)
    {
        hr = Stream_WriteString(stm, m_sPathRel);
        if (FAILED(hr))
            return hr;
    }

    if (m_Header.dwFlags & SLDF_HAS_WORKINGDIR)
    {
        hr = Stream_WriteString(stm, m_sWorkDir);
        if (FAILED(hr))
            return hr;
    }

    if (m_Header.dwFlags & SLDF_HAS_ARGS)
    {
        hr = Stream_WriteString(stm, m_sArgs);
        if (FAILED(hr))
            return hr;
    }

    if (m_Header.dwFlags & SLDF_HAS_ICONLOCATION)
    {
        hr = Stream_WriteString(stm, m_sIcoPath);
        if (FAILED(hr))
            return hr;
    }

    /*
     * Now save the data block list.
     *
     * NOTE that both advertised Product and Component are already saved
     * inside Logo3 and Darwin data blocks in the m_pDBList list, and the
     * m_Header.dwFlags is suitably initialized.
     */
    hr = SHWriteDataBlockList(stm, m_pDBList);
    if (FAILED(hr))
        return hr;

    /* Clear the dirty bit if requested */
    if (fClearDirty)
        m_bDirty = FALSE;

    return hr;
}

/************************************************************************
 * IPersistStream_GetSizeMax (IPersistStream)
 */
HRESULT STDMETHODCALLTYPE CShellLink::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    TRACE("(%p)\n", this);
    return E_NOTIMPL;
}

static BOOL SHELL_ExistsFileW(LPCWSTR path)
{
    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(path))
        return FALSE;

    return TRUE;
}

/**************************************************************************
 *  ShellLink_UpdatePath
 *    update absolute path in sPath using relative path in sPathRel
 */
static HRESULT ShellLink_UpdatePath(LPCWSTR sPathRel, LPCWSTR path, LPCWSTR sWorkDir, LPWSTR* psPath)
{
    if (!path || !psPath)
        return E_INVALIDARG;

    if (!*psPath && sPathRel)
    {
        WCHAR buffer[2*MAX_PATH], abs_path[2*MAX_PATH];
        LPWSTR final = NULL;

        /* first try if [directory of link file] + [relative path] finds an existing file */

        GetFullPathNameW(path, MAX_PATH * 2, buffer, &final);
        if (!final)
            final = buffer;
        wcscpy(final, sPathRel);

        *abs_path = '\0';

        if (SHELL_ExistsFileW(buffer))
        {
            if (!GetFullPathNameW(buffer, MAX_PATH, abs_path, &final))
                wcscpy(abs_path, buffer);
        }
        else
        {
            /* try if [working directory] + [relative path] finds an existing file */
            if (sWorkDir)
            {
                wcscpy(buffer, sWorkDir);
                wcscpy(PathAddBackslashW(buffer), sPathRel);

                if (SHELL_ExistsFileW(buffer))
                    if (!GetFullPathNameW(buffer, MAX_PATH, abs_path, &final))
                        wcscpy(abs_path, buffer);
            }
        }

        /* FIXME: This is even not enough - not all shell links can be resolved using this algorithm. */
        if (!*abs_path)
            wcscpy(abs_path, sPathRel);

        *psPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wcslen(abs_path) + 1) * sizeof(WCHAR));
        if (!*psPath)
            return E_OUTOFMEMORY;

        wcscpy(*psPath, abs_path);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::GetPath(LPSTR pszFile, INT cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD fFlags)
{
    HRESULT hr;
    LPWSTR pszFileW;
    WIN32_FIND_DATAW wfd;

    TRACE("(%p)->(pfile=%p len=%u find_data=%p flags=%u)(%s)\n",
          this, pszFile, cchMaxPath, pfd, fFlags, debugstr_w(m_sPath));

    /* Allocate a temporary UNICODE buffer */
    pszFileW = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, cchMaxPath * sizeof(WCHAR));
    if (!pszFileW)
        return E_OUTOFMEMORY;

    /* Call the UNICODE function */
    hr = GetPath(pszFileW, cchMaxPath, &wfd, fFlags);

    /* Convert the file path back to ANSI */
    WideCharToMultiByte(CP_ACP, 0, pszFileW, -1,
                        pszFile, cchMaxPath, NULL, NULL);

    /* Free the temporary buffer */
    HeapFree(GetProcessHeap(), 0, pszFileW);

    if (pfd)
    {
        ZeroMemory(pfd, sizeof(*pfd));

        /* Copy the file data if a file path was returned */
        if (*pszFile)
        {
            DWORD len;

            /* Copy the fixed part */
            CopyMemory(pfd, &wfd, FIELD_OFFSET(WIN32_FIND_DATAA, cFileName));

            /* Convert the file names to ANSI */
            len = lstrlenW(wfd.cFileName);
            WideCharToMultiByte(CP_ACP, 0, wfd.cFileName, len + 1,
                                pfd->cFileName, sizeof(pfd->cFileName), NULL, NULL);
            len = lstrlenW(wfd.cAlternateFileName);
            WideCharToMultiByte(CP_ACP, 0, wfd.cAlternateFileName, len + 1,
                                pfd->cAlternateFileName, sizeof(pfd->cAlternateFileName), NULL, NULL);
        }
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CShellLink::GetIDList(LPITEMIDLIST *ppidl)
{
    TRACE("(%p)->(ppidl=%p)\n", this, ppidl);

    if (!m_pPidl)
    {
        *ppidl = NULL;
        return S_FALSE;
    }

    *ppidl = ILClone(m_pPidl);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::SetIDList(LPCITEMIDLIST pidl)
{
    TRACE("(%p)->(pidl=%p)\n", this, pidl);
    return SetTargetFromPIDLOrPath(pidl, NULL);
}

HRESULT STDMETHODCALLTYPE CShellLink::GetDescription(LPSTR pszName, INT cchMaxName)
{
    TRACE("(%p)->(%p len=%u)\n", this, pszName, cchMaxName);

    if (cchMaxName)
        *pszName = 0;

    if (m_sDescription)
        WideCharToMultiByte(CP_ACP, 0, m_sDescription, -1,
                             pszName, cchMaxName, NULL, NULL);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::SetDescription(LPCSTR pszName)
{
    TRACE("(%p)->(pName=%s)\n", this, pszName);

    HeapFree(GetProcessHeap(), 0, m_sDescription);
    m_sDescription = NULL;

    if (pszName)
    {
        m_sDescription = HEAP_strdupAtoW(GetProcessHeap(), 0, pszName);
        if (!m_sDescription)
            return E_OUTOFMEMORY;
    }
    m_bDirty = TRUE;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::GetWorkingDirectory(LPSTR pszDir, INT cchMaxPath)
{
    TRACE("(%p)->(%p len=%u)\n", this, pszDir, cchMaxPath);

    if (cchMaxPath)
        *pszDir = 0;

    if (m_sWorkDir)
        WideCharToMultiByte(CP_ACP, 0, m_sWorkDir, -1,
                             pszDir, cchMaxPath, NULL, NULL);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::SetWorkingDirectory(LPCSTR pszDir)
{
    TRACE("(%p)->(dir=%s)\n", this, pszDir);

    HeapFree(GetProcessHeap(), 0, m_sWorkDir);
    m_sWorkDir = NULL;

    if (pszDir)
    {
        m_sWorkDir = HEAP_strdupAtoW(GetProcessHeap(), 0, pszDir);
        if (!m_sWorkDir)
            return E_OUTOFMEMORY;
    }
    m_bDirty = TRUE;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::GetArguments(LPSTR pszArgs, INT cchMaxPath)
{
    TRACE("(%p)->(%p len=%u)\n", this, pszArgs, cchMaxPath);

    if (cchMaxPath)
        *pszArgs = 0;

    if (m_sArgs)
        WideCharToMultiByte(CP_ACP, 0, m_sArgs, -1,
                             pszArgs, cchMaxPath, NULL, NULL);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::SetArguments(LPCSTR pszArgs)
{
    TRACE("(%p)->(args=%s)\n", this, pszArgs);

    HeapFree(GetProcessHeap(), 0, m_sArgs);
    m_sArgs = NULL;

    if (pszArgs)
    {
        m_sArgs = HEAP_strdupAtoW(GetProcessHeap(), 0, pszArgs);
        if (!m_sArgs)
            return E_OUTOFMEMORY;
    }

    m_bDirty = TRUE;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::GetHotkey(WORD *pwHotkey)
{
    TRACE("(%p)->(%p)(0x%08x)\n", this, pwHotkey, m_Header.wHotKey);
    *pwHotkey = m_Header.wHotKey;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::SetHotkey(WORD wHotkey)
{
    TRACE("(%p)->(hotkey=%x)\n", this, wHotkey);

    m_Header.wHotKey = wHotkey;
    m_bDirty = TRUE;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::GetShowCmd(INT *piShowCmd)
{
    TRACE("(%p)->(%p) %d\n", this, piShowCmd, m_Header.nShowCommand);
    *piShowCmd = m_Header.nShowCommand;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::SetShowCmd(INT iShowCmd)
{
    TRACE("(%p) %d\n", this, iShowCmd);

    m_Header.nShowCommand = iShowCmd;
    m_bDirty = TRUE;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::GetIconLocation(LPSTR pszIconPath, INT cchIconPath, INT *piIcon)
{
    HRESULT hr;
    LPWSTR pszIconPathW;

    TRACE("(%p)->(%p len=%u iicon=%p)\n", this, pszIconPath, cchIconPath, piIcon);

    /* Allocate a temporary UNICODE buffer */
    pszIconPathW = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, cchIconPath * sizeof(WCHAR));
    if (!pszIconPathW)
        return E_OUTOFMEMORY;

    /* Call the UNICODE function */
    hr = GetIconLocation(pszIconPathW, cchIconPath, piIcon);

    /* Convert the file path back to ANSI */
    WideCharToMultiByte(CP_ACP, 0, pszIconPathW, -1,
                        pszIconPath, cchIconPath, NULL, NULL);

    /* Free the temporary buffer */
    HeapFree(GetProcessHeap(), 0, pszIconPathW);

    return hr;
}

HRESULT STDMETHODCALLTYPE CShellLink::GetIconLocation(UINT uFlags, PSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
    HRESULT hr;
    LPWSTR pszIconFileW;

    TRACE("(%p)->(%u %p len=%u piIndex=%p pwFlags=%p)\n", this, uFlags, pszIconFile, cchMax, piIndex, pwFlags);

    /* Allocate a temporary UNICODE buffer */
    pszIconFileW = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, cchMax * sizeof(WCHAR));
    if (!pszIconFileW)
        return E_OUTOFMEMORY;

    /* Call the UNICODE function */
    hr = GetIconLocation(uFlags, pszIconFileW, cchMax, piIndex, pwFlags);

    /* Convert the file path back to ANSI */
    WideCharToMultiByte(CP_ACP, 0, pszIconFileW, -1,
                        pszIconFile, cchMax, NULL, NULL);

    /* Free the temporary buffer */
    HeapFree(GetProcessHeap(), 0, pszIconFileW);

    return hr;
}

HRESULT STDMETHODCALLTYPE CShellLink::Extract(PCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
    TRACE("(%p)->(path=%s iicon=%u)\n", this, pszFile, nIconIndex);

    LPWSTR str = NULL;
    if (pszFile)
    {
        str = HEAP_strdupAtoW(GetProcessHeap(), 0, pszFile);
        if (!str)
            return E_OUTOFMEMORY;
    }

    HRESULT hr = Extract(str, nIconIndex, phiconLarge, phiconSmall, nIconSize);

    if (str)
        HeapFree(GetProcessHeap(), 0, str);

    return hr;
}

HRESULT STDMETHODCALLTYPE CShellLink::SetIconLocation(LPCSTR pszIconPath, INT iIcon)
{
    TRACE("(%p)->(path=%s iicon=%u)\n", this, pszIconPath, iIcon);

    LPWSTR str = NULL;
    if (pszIconPath)
    {
        str = HEAP_strdupAtoW(GetProcessHeap(), 0, pszIconPath);
        if (!str)
            return E_OUTOFMEMORY;
    }

    HRESULT hr = SetIconLocation(str, iIcon);

    if (str)
        HeapFree(GetProcessHeap(), 0, str);

    return hr;
}

HRESULT STDMETHODCALLTYPE CShellLink::SetRelativePath(LPCSTR pszPathRel, DWORD dwReserved)
{
    TRACE("(%p)->(path=%s %x)\n", this, pszPathRel, dwReserved);

    HeapFree(GetProcessHeap(), 0, m_sPathRel);
    m_sPathRel = NULL;

    if (pszPathRel)
    {
        m_sPathRel = HEAP_strdupAtoW(GetProcessHeap(), 0, pszPathRel);
        m_bDirty = TRUE;
    }

    return ShellLink_UpdatePath(m_sPathRel, m_sPath, m_sWorkDir, &m_sPath);
}

static LPWSTR
shelllink_get_msi_component_path(LPWSTR component)
{
    DWORD Result, sz = 0;

    Result = CommandLineFromMsiDescriptor(component, NULL, &sz);
    if (Result != ERROR_SUCCESS)
        return NULL;

    sz++;
    LPWSTR path = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, sz * sizeof(WCHAR));
    Result = CommandLineFromMsiDescriptor(component, path, &sz);
    if (Result != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, path);
        path = NULL;
    }

    TRACE("returning %s\n", debugstr_w(path));

    return path;
}

HRESULT STDMETHODCALLTYPE CShellLink::Resolve(HWND hwnd, DWORD fFlags)
{
    HRESULT hr = S_OK;
    BOOL bSuccess;

    TRACE("(%p)->(hwnd=%p flags=%x)\n", this, hwnd, fFlags);

    /* FIXME: use IResolveShellLink interface? */

    // FIXME: See InvokeCommand().

#if (NTDDI_VERSION < NTDDI_LONGHORN)
    // NOTE: For Logo3 (EXP_LOGO3_ID_SIG), check also for SHRestricted(REST_NOLOGO3CHANNELNOTIFY)
    if (m_Header.dwFlags & SLDF_HAS_LOGO3ID)
    {
        FIXME("Logo3 links are not supported yet!\n");
        return E_FAIL;
    }
#endif

    /* Resolve Darwin (MSI) target */
    if (m_Header.dwFlags & SLDF_HAS_DARWINID)
    {
        LPWSTR component = NULL;
        hr = GetAdvertiseInfo(&component, EXP_DARWIN_ID_SIG);
        if (FAILED(hr))
            return E_FAIL;

        /* Clear the cached path */
        HeapFree(GetProcessHeap(), 0, m_sPath);
        m_sPath = NULL;
        m_sPath = shelllink_get_msi_component_path(component);
        if (!m_sPath)
            return E_FAIL;
    }

    if (!m_sPath && m_pPidl)
    {
        WCHAR buffer[MAX_PATH];

        bSuccess = SHGetPathFromIDListW(m_pPidl, buffer);
        if (bSuccess && *buffer)
        {
            m_sPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wcslen(buffer) + 1) * sizeof(WCHAR));
            if (!m_sPath)
                return E_OUTOFMEMORY;

            wcscpy(m_sPath, buffer);

            m_bDirty = TRUE;
        }
        else
        {
            hr = S_OK;    /* don't report an error occurred while just caching information */
        }
    }

    // FIXME: Strange to do that here...
    if (!m_sIcoPath && m_sPath)
    {
        m_sIcoPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wcslen(m_sPath) + 1) * sizeof(WCHAR));
        if (!m_sIcoPath)
            return E_OUTOFMEMORY;

        wcscpy(m_sIcoPath, m_sPath);
        m_Header.nIconIndex = 0;

        m_bDirty = TRUE;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CShellLink::SetPath(LPCSTR pszFile)
{
    TRACE("(%p)->(path=%s)\n", this, pszFile);

    if (!pszFile)
        return E_INVALIDARG;

    LPWSTR str = HEAP_strdupAtoW(GetProcessHeap(), 0, pszFile);
    if (!str)
        return E_OUTOFMEMORY;

    HRESULT hr = SetPath(str);
    HeapFree(GetProcessHeap(), 0, str);

    return hr;
}

HRESULT STDMETHODCALLTYPE CShellLink::GetPath(LPWSTR pszFile, INT cchMaxPath, WIN32_FIND_DATAW *pfd, DWORD fFlags)
{
    WCHAR buffer[MAX_PATH];

    TRACE("(%p)->(pfile=%p len=%u find_data=%p flags=%u)(%s)\n",
          this, pszFile, cchMaxPath, pfd, fFlags, debugstr_w(m_sPath));

    if (cchMaxPath)
        *pszFile = 0;
    // FIXME: What if cchMaxPath == 0 , or pszFile == NULL ??

    // FIXME: What about Darwin??

    /*
     * Retrieve the path to the target from the PIDL (if we have one).
     * NOTE: Do NOT use the cached path (m_sPath from link info).
     */
    if (m_pPidl && SHGetPathFromIDListW(m_pPidl, buffer))
    {
        if (fFlags & SLGP_SHORTPATH)
            GetShortPathNameW(buffer, buffer, _countof(buffer));
        // FIXME: Add support for SLGP_UNCPRIORITY
    }
    else
    {
        *buffer = 0;
    }

    /* If we have a FindData structure, initialize it */
    if (pfd)
    {
        ZeroMemory(pfd, sizeof(*pfd));

        /* Copy the file data if the target is a file path */
        if (*buffer)
        {
            pfd->dwFileAttributes = m_Header.dwFileAttributes;
            pfd->ftCreationTime   = m_Header.ftCreationTime;
            pfd->ftLastAccessTime = m_Header.ftLastAccessTime;
            pfd->ftLastWriteTime  = m_Header.ftLastWriteTime;
            pfd->nFileSizeHigh    = 0;
            pfd->nFileSizeLow     = m_Header.nFileSizeLow;

            /*
             * Build temporarily a short path in pfd->cFileName (of size MAX_PATH),
             * then extract and store the short file name in pfd->cAlternateFileName.
             */
            GetShortPathNameW(buffer, pfd->cFileName, _countof(pfd->cFileName));
            lstrcpynW(pfd->cAlternateFileName,
                      PathFindFileNameW(pfd->cFileName),
                      _countof(pfd->cAlternateFileName));

            /* Now extract and store the long file name in pfd->cFileName */
            lstrcpynW(pfd->cFileName,
                      PathFindFileNameW(buffer),
                      _countof(pfd->cFileName));
        }
    }

    /* Finally check if we have a raw path the user actually wants to retrieve */
    if ((fFlags & SLGP_RAWPATH) && (m_Header.dwFlags & SLDF_HAS_EXP_SZ))
    {
        /* Search for a target environment block */
        LPEXP_SZ_LINK pInfo;
        pInfo = (LPEXP_SZ_LINK)SHFindDataBlock(m_pDBList, EXP_SZ_LINK_SIG);
        if (pInfo && (pInfo->cbSize == sizeof(*pInfo)))
            lstrcpynW(buffer, pInfo->szwTarget, cchMaxPath);
    }

    /* For diagnostics purposes only... */
    // NOTE: SLGP_UNCPRIORITY is unsupported
    fFlags &= ~(SLGP_RAWPATH | SLGP_SHORTPATH);
    if (fFlags) FIXME("(%p): Unsupported flags %lu\n", this, fFlags);

    /* Copy the data back to the user */
    if (*buffer)
        lstrcpynW(pszFile, buffer, cchMaxPath);

    return (*buffer ? S_OK : S_FALSE);
}

HRESULT STDMETHODCALLTYPE CShellLink::GetDescription(LPWSTR pszName, INT cchMaxName)
{
    TRACE("(%p)->(%p len=%u)\n", this, pszName, cchMaxName);

    *pszName = 0;
    if (m_sDescription)
        lstrcpynW(pszName, m_sDescription, cchMaxName);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::SetDescription(LPCWSTR pszName)
{
    TRACE("(%p)->(desc=%s)\n", this, debugstr_w(pszName));

    HeapFree(GetProcessHeap(), 0, m_sDescription);
    if (pszName)
    {
        m_sDescription = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                                         (wcslen(pszName) + 1) * sizeof(WCHAR));
        if (!m_sDescription)
            return E_OUTOFMEMORY;

        wcscpy(m_sDescription, pszName);
    }
    else
        m_sDescription = NULL;

    m_bDirty = TRUE;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::GetWorkingDirectory(LPWSTR pszDir, INT cchMaxPath)
{
    TRACE("(%p)->(%p len %u)\n", this, pszDir, cchMaxPath);

    if (cchMaxPath)
        *pszDir = 0;

    if (m_sWorkDir)
        lstrcpynW(pszDir, m_sWorkDir, cchMaxPath);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::SetWorkingDirectory(LPCWSTR pszDir)
{
    TRACE("(%p)->(dir=%s)\n", this, debugstr_w(pszDir));

    HeapFree(GetProcessHeap(), 0, m_sWorkDir);
    if (pszDir)
    {
        m_sWorkDir = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                                     (wcslen(pszDir) + 1) * sizeof(WCHAR));
        if (!m_sWorkDir)
            return E_OUTOFMEMORY;
        wcscpy(m_sWorkDir, pszDir);
    }
    else
        m_sWorkDir = NULL;

    m_bDirty = TRUE;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::GetArguments(LPWSTR pszArgs, INT cchMaxPath)
{
    TRACE("(%p)->(%p len=%u)\n", this, pszArgs, cchMaxPath);

    if (cchMaxPath)
        *pszArgs = 0;

    if (m_sArgs)
        lstrcpynW(pszArgs, m_sArgs, cchMaxPath);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::SetArguments(LPCWSTR pszArgs)
{
    TRACE("(%p)->(args=%s)\n", this, debugstr_w(pszArgs));

    HeapFree(GetProcessHeap(), 0, m_sArgs);
    if (pszArgs)
    {
        m_sArgs = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                                  (wcslen(pszArgs) + 1) * sizeof(WCHAR));
        if (!m_sArgs)
            return E_OUTOFMEMORY;

        wcscpy(m_sArgs, pszArgs);
    }
    else
        m_sArgs = NULL;

    m_bDirty = TRUE;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::GetIconLocation(LPWSTR pszIconPath, INT cchIconPath, INT *piIcon)
{
    TRACE("(%p)->(%p len=%u iicon=%p)\n", this, pszIconPath, cchIconPath, piIcon);

    if (cchIconPath)
        *pszIconPath = 0;

    *piIcon = 0;

    /* Update the original icon path location */
    if (m_Header.dwFlags & SLDF_HAS_EXP_ICON_SZ)
    {
        WCHAR szPath[MAX_PATH];

        /* Search for an icon environment block */
        LPEXP_SZ_LINK pInfo;
        pInfo = (LPEXP_SZ_LINK)SHFindDataBlock(m_pDBList, EXP_SZ_ICON_SIG);
        if (pInfo && (pInfo->cbSize == sizeof(*pInfo)))
        {
            m_Header.dwFlags &= ~SLDF_HAS_ICONLOCATION;
            HeapFree(GetProcessHeap(), 0, m_sIcoPath);
            m_sIcoPath = NULL;

            SHExpandEnvironmentStringsW(pInfo->szwTarget, szPath, _countof(szPath));

            m_sIcoPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                                           (wcslen(szPath) + 1) * sizeof(WCHAR));
            if (!m_sIcoPath)
                return E_OUTOFMEMORY;

            wcscpy(m_sIcoPath, szPath);
            m_Header.dwFlags |= SLDF_HAS_ICONLOCATION;

            m_bDirty = TRUE;
        }
    }

    *piIcon = m_Header.nIconIndex;

    if (m_sIcoPath)
        lstrcpynW(pszIconPath, m_sIcoPath, cchIconPath);

    return S_OK;
}

static HRESULT SHELL_PidlGetIconLocationW(IShellFolder* psf, LPCITEMIDLIST pidl,
        UINT uFlags, PWSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
    LPCITEMIDLIST pidlLast;

    HRESULT hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast);
    if (SUCCEEDED(hr))
    {
        CComPtr<IExtractIconW> pei;

        hr = psf->GetUIObjectOf(0, 1, &pidlLast, IID_NULL_PPV_ARG(IExtractIconW, &pei));
        if (SUCCEEDED(hr))
            hr = pei->GetIconLocation(uFlags, pszIconFile, cchMax, piIndex, pwFlags);

        psf->Release();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CShellLink::GetIconLocation(UINT uFlags, PWSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
    /*
     * It is possible for a shell link to point to another shell link,
     * and in particular there is the possibility to point to itself.
     * Now, suppose we ask such a link to retrieve its associated icon.
     * This function would be called, and due to COM would be called again
     * recursively. To solve this issue, we forbid calling GetIconLocation()
     * with GIL_FORSHORTCUT set in uFlags, as done by Windows (shown by tests).
     */
    if (uFlags & GIL_FORSHORTCUT)
        return E_INVALIDARG;

    /*
     * Now, we set GIL_FORSHORTCUT so that: i) we allow the icon extractor
     * of the target to give us a suited icon, and ii) we protect ourselves
     * against recursive call.
     */
    uFlags |= GIL_FORSHORTCUT;

    if (m_pPidl || m_sPath)
    {
        CComPtr<IShellFolder> pdsk;

        HRESULT hr = SHGetDesktopFolder(&pdsk);
        if (SUCCEEDED(hr))
        {
            /* first look for an icon using the PIDL (if present) */
            if (m_pPidl)
                hr = SHELL_PidlGetIconLocationW(pdsk, m_pPidl, uFlags, pszIconFile, cchMax, piIndex, pwFlags);
            else
                hr = E_FAIL;

#if 0 // FIXME: Analyse further whether this is needed...
            /* if we couldn't find an icon yet, look for it using the file system path */
            if (FAILED(hr) && m_sPath)
            {
                LPITEMIDLIST pidl;

                /* LPITEMIDLIST pidl = ILCreateFromPathW(sPath); */
                hr = pdsk->ParseDisplayName(0, NULL, m_sPath, NULL, &pidl, NULL);
                if (SUCCEEDED(hr))
                {
                    hr = SHELL_PidlGetIconLocationW(pdsk, pidl, uFlags, pszIconFile, cchMax, piIndex, pwFlags);
                    SHFree(pidl);
                }
            }
#endif
        }
        return hr;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::Extract(PCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
    UNIMPLEMENTED;
    return E_FAIL;
}

#if 0
/* Extends the functionality of PathUnExpandEnvStringsW */
BOOL PathFullyUnExpandEnvStringsW(
    _In_  LPCWSTR pszPath,
    _Out_ LPWSTR  pszBuf,
    _In_  UINT    cchBuf)
{
    BOOL Ret = FALSE; // Set to TRUE as soon as PathUnExpandEnvStrings starts unexpanding.
    BOOL res;
    LPCWSTR p;

    // *pszBuf = L'\0';
    while (*pszPath && cchBuf > 0)
    {
        /* Attempt unexpanding the path */
        res = PathUnExpandEnvStringsW(pszPath, pszBuf, cchBuf);
        if (!res)
        {
            /* The unexpansion failed. Try to find a path delimiter. */
            p = wcspbrk(pszPath, L" /\\:*?\"<>|%");
            if (!p) /* None found, we will copy the remaining path */
                p = pszPath + wcslen(pszPath);
            else    /* Found one, we will copy the delimiter and skip it */
                ++p;
            /* If we overflow, we cannot unexpand more, so return FALSE */
            if (p - pszPath >= cchBuf)
                return FALSE; // *pszBuf = L'\0';

            /* Copy the untouched portion of path up to the delimiter, included */
            wcsncpy(pszBuf, pszPath, p - pszPath);
            pszBuf[p - pszPath] = L'\0'; // NULL-terminate

            /* Advance the pointers and decrease the remaining buffer size */
            cchBuf -= (p - pszPath);
            pszBuf += (p - pszPath);
            pszPath += (p - pszPath);
        }
        else
        {
            /*
             * The unexpansion succeeded. Skip the unexpanded part by trying
             * to find where the original path and the unexpanded string
             * become different.
             * NOTE: An alternative(?) would be to stop also at the last
             * path delimiter encountered in the loop (i.e. would be the
             * first path delimiter in the strings).
             */
            LPWSTR q;

            /*
             * The algorithm starts at the end of the strings and loops back
             * while the characters are equal, until it finds a discrepancy.
             */
            p = pszPath + wcslen(pszPath);
            q = pszBuf + wcslen(pszBuf); // This wcslen should be < cchBuf
            while ((*p == *q) && (p > pszPath) && (q > pszBuf))
            {
                --p; --q;
            }
            /* Skip discrepancy */
            ++p; ++q;

            /* Advance the pointers and decrease the remaining buffer size */
            cchBuf -= (q - pszBuf);
            pszBuf = q;
            pszPath = p;

            Ret = TRUE;
        }
    }

    return Ret;
}
#endif

HRESULT STDMETHODCALLTYPE CShellLink::SetIconLocation(LPCWSTR pszIconPath, INT iIcon)
{
    HRESULT hr = E_FAIL;
    WCHAR szUnExpIconPath[MAX_PATH];

    TRACE("(%p)->(path=%s iicon=%u)\n", this, debugstr_w(pszIconPath), iIcon);

    if (pszIconPath)
    {
        /* Try to fully unexpand the icon path */

        /*
         * Check whether the user-given file path contains unexpanded
         * environment variables. If so, create a target environment block.
         * Note that in this block we will store the user-given path.
         * It will contain the unexpanded environment variables, but
         * it can also contain already expanded path that the user does
         * not want to see them unexpanded (e.g. so that they always
         * refer to the same place even if the would-be corresponding
         * environment variable could change).
         */
        // FIXME: http://stackoverflow.com/questions/2976489/ishelllinkseticonlocation-translates-my-icon-path-into-program-files-which-i
        // if (PathFullyUnExpandEnvStringsW(pszIconPath, szUnExpIconPath, _countof(szUnExpIconPath)))
        SHExpandEnvironmentStringsW(pszIconPath, szUnExpIconPath, _countof(szUnExpIconPath));
        if (wcscmp(pszIconPath, szUnExpIconPath) != 0)
        {
            /* Unexpansion succeeded, so we need an icon environment block */
            EXP_SZ_LINK buffer;
            LPEXP_SZ_LINK pInfo;

            pInfo = (LPEXP_SZ_LINK)SHFindDataBlock(m_pDBList, EXP_SZ_ICON_SIG);
            if (pInfo)
            {
                /* Make sure that the size of the structure is valid */
                if (pInfo->cbSize != sizeof(*pInfo))
                {
                    ERR("Ooops. This structure is not as expected...\n");

                    /* Invalid structure, remove it altogether */
                    m_Header.dwFlags &= ~SLDF_HAS_EXP_ICON_SZ;
                    RemoveDataBlock(EXP_SZ_ICON_SIG);

                    /* Reset the pointer and go use the static buffer */
                    pInfo = NULL;
                }
            }
            if (!pInfo)
            {
                /* Use the static buffer */
                pInfo = &buffer;
                buffer.cbSize = sizeof(buffer);
                buffer.dwSignature = EXP_SZ_ICON_SIG;
            }

            lstrcpynW(pInfo->szwTarget, szUnExpIconPath, _countof(pInfo->szwTarget));
            WideCharToMultiByte(CP_ACP, 0, szUnExpIconPath, -1,
                                pInfo->szTarget, _countof(pInfo->szTarget), NULL, NULL);

            hr = S_OK;
            if (pInfo == &buffer)
                hr = AddDataBlock(pInfo);
            if (hr == S_OK)
                m_Header.dwFlags |= SLDF_HAS_EXP_ICON_SZ;
        }
        else
        {
            /* Unexpansion failed, so we need to remove any icon environment block */
            m_Header.dwFlags &= ~SLDF_HAS_EXP_ICON_SZ;
            RemoveDataBlock(EXP_SZ_ICON_SIG);
        }
    }

    /* Store the original icon path location (this one may contain unexpanded environment strings) */
    if (pszIconPath)
    {
        m_Header.dwFlags &= ~SLDF_HAS_ICONLOCATION;
        HeapFree(GetProcessHeap(), 0, m_sIcoPath);
        m_sIcoPath = NULL;

        m_sIcoPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                                     (wcslen(pszIconPath) + 1) * sizeof(WCHAR));
        if (!m_sIcoPath)
            return E_OUTOFMEMORY;

        wcscpy(m_sIcoPath, pszIconPath);
        m_Header.dwFlags |= SLDF_HAS_ICONLOCATION;
    }

    hr = S_OK;

    m_Header.nIconIndex = iIcon;
    m_bDirty = TRUE;

    return hr;
}

HRESULT STDMETHODCALLTYPE CShellLink::SetRelativePath(LPCWSTR pszPathRel, DWORD dwReserved)
{
    TRACE("(%p)->(path=%s %x)\n", this, debugstr_w(pszPathRel), dwReserved);

    HeapFree(GetProcessHeap(), 0, m_sPathRel);
    if (pszPathRel)
    {
        m_sPathRel = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                                      (wcslen(pszPathRel) + 1) * sizeof(WCHAR));
        if (!m_sPathRel)
            return E_OUTOFMEMORY;
        wcscpy(m_sPathRel, pszPathRel);
    }
    else
        m_sPathRel = NULL;

    m_bDirty = TRUE;

    return ShellLink_UpdatePath(m_sPathRel, m_sPath, m_sWorkDir, &m_sPath);
}

static LPWSTR GetAdvertisedArg(LPCWSTR str)
{
    if (!str)
        return NULL;

    LPCWSTR p = wcschr(str, L':');
    if (!p)
        return NULL;

    DWORD len = p - str;
    LPWSTR ret = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * (len + 1));
    if (!ret)
        return ret;

    memcpy(ret, str, sizeof(WCHAR)*len);
    ret[len] = 0;
    return ret;
}

HRESULT CShellLink::WriteAdvertiseInfo(LPCWSTR string, DWORD dwSig)
{
    EXP_DARWIN_LINK buffer;
    LPEXP_DARWIN_LINK pInfo;

    if (   (dwSig != EXP_DARWIN_ID_SIG)
#if (NTDDI_VERSION < NTDDI_LONGHORN)
        && (dwSig != EXP_LOGO3_ID_SIG)
#endif
        )
    {
        return E_INVALIDARG;
    }

    if (!string)
        return S_FALSE;

    pInfo = (LPEXP_DARWIN_LINK)SHFindDataBlock(m_pDBList, dwSig);
    if (pInfo)
    {
        /* Make sure that the size of the structure is valid */
        if (pInfo->dbh.cbSize != sizeof(*pInfo))
        {
            ERR("Ooops. This structure is not as expected...\n");

            /* Invalid structure, remove it altogether */
            if (dwSig == EXP_DARWIN_ID_SIG)
                m_Header.dwFlags &= ~SLDF_HAS_DARWINID;
#if (NTDDI_VERSION < NTDDI_LONGHORN)
            else if (dwSig == EXP_LOGO3_ID_SIG)
                m_Header.dwFlags &= ~SLDF_HAS_LOGO3ID;
#endif
            RemoveDataBlock(dwSig);

            /* Reset the pointer and go use the static buffer */
            pInfo = NULL;
        }
    }
    if (!pInfo)
    {
        /* Use the static buffer */
        pInfo = &buffer;
        buffer.dbh.cbSize = sizeof(buffer);
        buffer.dbh.dwSignature = dwSig;
    }

    lstrcpynW(pInfo->szwDarwinID, string, _countof(pInfo->szwDarwinID));
    WideCharToMultiByte(CP_ACP, 0, string, -1,
                        pInfo->szDarwinID, _countof(pInfo->szDarwinID), NULL, NULL);

    HRESULT hr = S_OK;
    if (pInfo == &buffer)
        hr = AddDataBlock(pInfo);
    if (hr == S_OK)
    {
        if (dwSig == EXP_DARWIN_ID_SIG)
            m_Header.dwFlags |= SLDF_HAS_DARWINID;
#if (NTDDI_VERSION < NTDDI_LONGHORN)
        else if (dwSig == EXP_LOGO3_ID_SIG)
            m_Header.dwFlags |= SLDF_HAS_LOGO3ID;
#endif
    }

    return hr;
}

HRESULT CShellLink::SetAdvertiseInfo(LPCWSTR str)
{
    HRESULT hr;
    LPCWSTR szComponent = NULL, szProduct = NULL, p;
    INT len;
    GUID guid;
    WCHAR szGuid[38+1];

    /**/sProduct = sComponent = NULL;/**/

    while (str[0])
    {
        /* each segment must start with two colons */
        if (str[0] != ':' || str[1] != ':')
            return E_FAIL;

        /* the last segment is just two colons */
        if (!str[2])
            break;
        str += 2;

        /* there must be a colon straight after a guid */
        p = wcschr(str, L':');
        if (!p)
            return E_FAIL;
        len = p - str;
        if (len != 38)
            return E_FAIL;

        /* get the guid, and check if it's validly formatted */
        memcpy(szGuid, str, sizeof(WCHAR)*len);
        szGuid[len] = 0;

        hr = CLSIDFromString(szGuid, &guid);
        if (hr != S_OK)
            return hr;
        str = p + 1;

        /* match it up to a guid that we care about */
        if (IsEqualGUID(guid, SHELL32_AdvtShortcutComponent) && !szComponent)
            szComponent = str; /* Darwin */
        else if (IsEqualGUID(guid, SHELL32_AdvtShortcutProduct) && !szProduct)
            szProduct = str;   /* Logo3  */
        else
            return E_FAIL;

        /* skip to the next field */
        str = wcschr(str, L':');
        if (!str)
            return E_FAIL;
    }

    /* we have to have a component for an advertised shortcut */
    if (!szComponent)
        return E_FAIL;

    szComponent = GetAdvertisedArg(szComponent);
    szProduct = GetAdvertisedArg(szProduct);

    hr = WriteAdvertiseInfo(szComponent, EXP_DARWIN_ID_SIG);
    // if (FAILED(hr))
        // return hr;
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    hr = WriteAdvertiseInfo(szProduct, EXP_LOGO3_ID_SIG);
    // if (FAILED(hr))
        // return hr;
#endif

    HeapFree(GetProcessHeap(), 0, (PVOID)szComponent);
    HeapFree(GetProcessHeap(), 0, (PVOID)szProduct);

    if (TRACE_ON(shell))
    {
        GetAdvertiseInfo(&sComponent, EXP_DARWIN_ID_SIG);
        TRACE("Component = %s\n", debugstr_w(sComponent));
#if (NTDDI_VERSION < NTDDI_LONGHORN)
        GetAdvertiseInfo(&sProduct, EXP_LOGO3_ID_SIG);
        TRACE("Product = %s\n", debugstr_w(sProduct));
#endif
    }

    return S_OK;
}

/*
 * Since the real PathResolve (from Wine) is unimplemented at the moment,
 * we use this local implementation, until a better one is written (using
 * code parts of the SHELL_xxx helpers in Wine's shellpath.c).
 */
static BOOL HACKISH_PathResolve(
    IN OUT PWSTR pszPath,
    IN PZPCWSTR dirs OPTIONAL,
    IN UINT fFlags)
{
    // FIXME: This is unimplemented!!!
#if 0
    return PathResolve(pszPath, dirs, fFlags);
#else
    BOOL Success = FALSE;
    USHORT i;
    LPWSTR fname = NULL;
    WCHAR szPath[MAX_PATH];

    /* First, search for a valid existing path */

    // NOTE: See also: SHELL_FindExecutable()

    /*
     * List of extensions searched for, by PathResolve with the flag
     * PRF_TRYPROGRAMEXTENSIONS == PRF_EXECUTABLE | PRF_VERIFYEXISTS set,
     * according to MSDN: https://msdn.microsoft.com/en-us/library/windows/desktop/bb776478(v=vs.85).aspx
     */
    static PCWSTR Extensions[] = {L".pif", L".com", L".bat", L".cmd", L".lnk", L".exe", NULL};
    #define LNK_EXT_INDEX   4   // ".lnk" has index 4 in the array above

    /*
     * Start at the beginning of the list if PRF_EXECUTABLE is set, otherwise
     * just use the last element 'NULL' (no extension checking).
     */
    i = ((fFlags & PRF_EXECUTABLE) ? 0 : _countof(Extensions) - 1);
    for (; i < _countof(Extensions); ++i)
    {
        /* Ignore shell links ".lnk" if needed */
        if ((fFlags & PRF_DONTFINDLNK) && (i == LNK_EXT_INDEX))
            continue;

        Success = (SearchPathW(NULL, pszPath, Extensions[i],
                               _countof(szPath), szPath, NULL) != 0);
        if (!Success)
        {
            ERR("SearchPathW(pszPath = '%S') failed\n", pszPath);
        }
        else
        {
            ERR("SearchPathW(pszPath = '%S', szPath = '%S') succeeded\n", pszPath, szPath);
            break;
        }
    }

    if (!Success)
    {
        ERR("SearchPathW(pszPath = '%S') failed\n", pszPath);

        /* We failed, try with PathFindOnPath, as explained by MSDN */
        // Success = PathFindOnPathW(pszPath, dirs);
        StringCchCopyW(szPath, _countof(szPath), pszPath);
        Success = PathFindOnPathW(szPath, dirs);
        if (!Success)
        {
            ERR("PathFindOnPathW(pszPath = '%S') failed\n", pszPath);

            /* We failed again, fall back to building a possible non-existing path */
            if (!GetFullPathNameW(pszPath, _countof(szPath), szPath, &fname))
            {
                ERR("GetFullPathNameW(pszPath = '%S') failed\n", pszPath);
                return FALSE;
            }

            Success = PathFileExistsW(szPath);
            if (!Success)
                ERR("PathFileExistsW(szPath = '%S') failed\n", szPath);

            /******************************************************/
            /* Question: Why this line is needed only for files?? */
            if (fname && (_wcsicmp(pszPath, fname) == 0))
                *szPath = L'\0';
            /******************************************************/
        }
        else
        {
            ERR("PathFindOnPathW(pszPath = '%S' ==> '%S') succeeded\n", pszPath, szPath);
        }
    }

    /* Copy back the results to the caller */
    StringCchCopyW(pszPath, MAX_PATH, szPath);

    /*
     * Since the called functions always checked whether the file path existed,
     * we do not need to redo a final check: we can use instead the cached
     * result in 'Success'.
     */
    return ((fFlags & PRF_VERIFYEXISTS) ? Success : TRUE);
#endif
}

HRESULT CShellLink::SetTargetFromPIDLOrPath(LPCITEMIDLIST pidl, LPCWSTR pszFile)
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidlNew = NULL;
    WCHAR szPath[MAX_PATH];

    /*
     * Not both 'pidl' and 'pszFile' should be set.
     * But either one or both can be NULL.
     */
    if (pidl && pszFile)
        return E_FAIL;

    if (pidl)
    {
        /* Clone the PIDL */
        pidlNew = ILClone(pidl);
        if (!pidlNew)
            return E_FAIL;
    }
    else if (pszFile)
    {
        /* Build a PIDL for this path target */
        hr = SHILCreateFromPathW(pszFile, &pidlNew, NULL);
        if (FAILED(hr))
        {
            /* This failed, try to resolve the path, then create a simple PIDL */

            StringCchCopyW(szPath, _countof(szPath), pszFile);
            // FIXME: Because PathResolve is unimplemented, we use our hackish implementation!
            HACKISH_PathResolve(szPath, NULL, PRF_TRYPROGRAMEXTENSIONS);

            pidlNew = SHSimpleIDListFromPathW(szPath);
            /******************************************************/
            /* Question: Why this line is needed only for files?? */
            hr = (*szPath ? S_OK : E_INVALIDARG); // S_FALSE
            /******************************************************/
        }
    }
    // else if (!pidl && !pszFile) { pidlNew = NULL; hr = S_OK; }

    ILFree(m_pPidl);
    m_pPidl = pidlNew;

    if (!pszFile)
    {
        if (SHGetPathFromIDListW(pidlNew, szPath))
            pszFile = szPath;
    }

    // TODO: Fully update link info, tracker, file attribs...

    // if (pszFile)
    if (!pszFile)
    {
        *szPath = L'\0';
        pszFile = szPath;
    }

    /* Update the cached path (for link info) */
    ShellLink_GetVolumeInfo(pszFile, &volume);
    m_sPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                              (wcslen(pszFile) + 1) * sizeof(WCHAR));
    if (!m_sPath)
        return E_OUTOFMEMORY;
    wcscpy(m_sPath, pszFile);

    m_bDirty = TRUE;
    return hr;
}

HRESULT STDMETHODCALLTYPE CShellLink::SetPath(LPCWSTR pszFile)
{
    LPWSTR unquoted = NULL;
    HRESULT hr = S_OK;

    TRACE("(%p)->(path=%s)\n", this, debugstr_w(pszFile));

    if (!pszFile)
        return E_INVALIDARG;

    /*
     * Allow upgrading Logo3 shortcuts (m_Header.dwFlags & SLDF_HAS_LOGO3ID),
     * but forbid upgrading Darwin ones.
     */
    if (m_Header.dwFlags & SLDF_HAS_DARWINID)
        return S_FALSE;

    /* quotes at the ends of the string are stripped */
    SIZE_T len = wcslen(pszFile);
    if (pszFile[0] == L'"' && pszFile[len-1] == L'"')
    {
        unquoted = strdupW(pszFile);
        PathUnquoteSpacesW(unquoted);
        pszFile = unquoted;
    }

    /* any other quote marks are invalid */
    if (wcschr(pszFile, L'"'))
    {
        hr = S_FALSE;
        goto end;
    }

    /* Clear the cached path */
    HeapFree(GetProcessHeap(), 0, m_sPath);
    m_sPath = NULL;

    /* Check for an advertised target (Logo3 or Darwin) */
    if (SetAdvertiseInfo(pszFile) != S_OK)
    {
        /* This is not an advertised target, but a regular path */
        WCHAR szPath[MAX_PATH];

        /*
         * Check whether the user-given file path contains unexpanded
         * environment variables. If so, create a target environment block.
         * Note that in this block we will store the user-given path.
         * It will contain the unexpanded environment variables, but
         * it can also contain already expanded path that the user does
         * not want to see them unexpanded (e.g. so that they always
         * refer to the same place even if the would-be corresponding
         * environment variable could change).
         */
        if (*pszFile)
            SHExpandEnvironmentStringsW(pszFile, szPath, _countof(szPath));
        else
            *szPath = L'\0';

        if (*pszFile && (wcscmp(pszFile, szPath) != 0))
        {
            /*
             * The user-given file path contains unexpanded environment
             * variables, so we need a target environment block.
             */
            EXP_SZ_LINK buffer;
            LPEXP_SZ_LINK pInfo;

            pInfo = (LPEXP_SZ_LINK)SHFindDataBlock(m_pDBList, EXP_SZ_LINK_SIG);
            if (pInfo)
            {
                /* Make sure that the size of the structure is valid */
                if (pInfo->cbSize != sizeof(*pInfo))
                {
                    ERR("Ooops. This structure is not as expected...\n");

                    /* Invalid structure, remove it altogether */
                    m_Header.dwFlags &= ~SLDF_HAS_EXP_SZ;
                    RemoveDataBlock(EXP_SZ_LINK_SIG);

                    /* Reset the pointer and go use the static buffer */
                    pInfo = NULL;
                }
            }
            if (!pInfo)
            {
                /* Use the static buffer */
                pInfo = &buffer;
                buffer.cbSize = sizeof(buffer);
                buffer.dwSignature = EXP_SZ_LINK_SIG;
            }

            lstrcpynW(pInfo->szwTarget, pszFile, _countof(pInfo->szwTarget));
            WideCharToMultiByte(CP_ACP, 0, pszFile, -1,
                                pInfo->szTarget, _countof(pInfo->szTarget), NULL, NULL);

            hr = S_OK;
            if (pInfo == &buffer)
                hr = AddDataBlock(pInfo);
            if (hr == S_OK)
                m_Header.dwFlags |= SLDF_HAS_EXP_SZ;

            /* Now, make pszFile point to the expanded buffer */
            pszFile = szPath;
        }
        else
        {
            /*
             * The user-given file path does not contain unexpanded environment
             * variables, so we need to remove any target environment block.
             */
            m_Header.dwFlags &= ~SLDF_HAS_EXP_SZ;
            RemoveDataBlock(EXP_SZ_LINK_SIG);

            /* pszFile points to the user path */
        }

        /* Set the target */
        hr = SetTargetFromPIDLOrPath(NULL, pszFile);
    }

    m_bDirty = TRUE;

end:
    HeapFree(GetProcessHeap(), 0, unquoted);
    return hr;
}

HRESULT STDMETHODCALLTYPE CShellLink::AddDataBlock(void* pDataBlock)
{
    if (SHAddDataBlock(&m_pDBList, (DATABLOCK_HEADER*)pDataBlock))
    {
        m_bDirty = TRUE;
        return S_OK;
    }
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CShellLink::CopyDataBlock(DWORD dwSig, void** ppDataBlock)
{
    DATABLOCK_HEADER* pBlock;
    PVOID pDataBlock;

    TRACE("%p %08x %p\n", this, dwSig, ppDataBlock);

    *ppDataBlock = NULL;

    pBlock = SHFindDataBlock(m_pDBList, dwSig);
    if (!pBlock)
    {
        ERR("unknown datablock %08x (not found)\n", dwSig);
        return E_FAIL;
    }

    pDataBlock = LocalAlloc(LMEM_ZEROINIT, pBlock->cbSize);
    if (!pDataBlock)
        return E_OUTOFMEMORY;

    CopyMemory(pDataBlock, pBlock, pBlock->cbSize);

    *ppDataBlock = pDataBlock;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::RemoveDataBlock(DWORD dwSig)
{
    if (SHRemoveDataBlock(&m_pDBList, dwSig))
    {
        m_bDirty = TRUE;
        return S_OK;
    }
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CShellLink::GetFlags(DWORD *pdwFlags)
{
    TRACE("%p %p\n", this, pdwFlags);
    *pdwFlags = m_Header.dwFlags;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::SetFlags(DWORD dwFlags)
{
#if 0 // FIXME!
    m_Header.dwFlags = dwFlags;
    m_bDirty = TRUE;
    return S_OK;
#else
    FIXME("\n");
    return E_NOTIMPL;
#endif
}

/**************************************************************************
 * CShellLink implementation of IShellExtInit::Initialize()
 *
 * Loads the shelllink from the dataobject the shell is pointing to.
 */
HRESULT STDMETHODCALLTYPE CShellLink::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    TRACE("%p %p %p %p\n", this, pidlFolder, pdtobj, hkeyProgID);

    if (!pdtobj)
        return E_FAIL;

    FORMATETC format;
    format.cfFormat = CF_HDROP;
    format.ptd = NULL;
    format.dwAspect = DVASPECT_CONTENT;
    format.lindex = -1;
    format.tymed = TYMED_HGLOBAL;

    STGMEDIUM stgm;
    HRESULT hr = pdtobj->GetData(&format, &stgm);
    if (FAILED(hr))
        return hr;

    UINT count = DragQueryFileW((HDROP)stgm.hGlobal, -1, NULL, 0);
    if (count == 1)
    {
        count = DragQueryFileW((HDROP)stgm.hGlobal, 0, NULL, 0);
        count++;
        LPWSTR path = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR));
        if (path)
        {
            count = DragQueryFileW((HDROP)stgm.hGlobal, 0, path, count);
            hr = Load(path, 0);
            HeapFree(GetProcessHeap(), 0, path);
        }
    }
    ReleaseStgMedium(&stgm);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    int id = 1;

    TRACE("%p %p %u %u %u %u\n", this,
          hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    if (!hMenu)
        return E_INVALIDARG;

    WCHAR wszOpen[20];
    if (!LoadStringW(shell32_hInstance, IDS_OPEN_VERB, wszOpen, _countof(wszOpen)))
        *wszOpen = L'\0';

    MENUITEMINFOW mii;
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
    mii.dwTypeData = wszOpen;
    mii.cch = wcslen(mii.dwTypeData);
    mii.wID = idCmdFirst + id++;
    mii.fState = MFS_DEFAULT | MFS_ENABLED;
    mii.fType = MFT_STRING;
    if (!InsertMenuItemW(hMenu, indexMenu, TRUE, &mii))
        return E_FAIL;
    m_iIdOpen = 1;

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, id);
}

HRESULT STDMETHODCALLTYPE CShellLink::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    LPWSTR args = NULL;
    LPWSTR path = NULL;

    TRACE("%p %p\n", this, lpici);

    if (lpici->cbSize < sizeof(CMINVOKECOMMANDINFO))
        return E_INVALIDARG;

    // NOTE: We could use lpici->hwnd (certainly in case lpici->fMask doesn't contain CMIC_MASK_FLAG_NO_UI)
    // as the parent window handle... ?
    /* FIXME: get using interface set from IObjectWithSite?? */
    // NOTE: We might need an extended version of Resolve that provides us with paths...
    HRESULT hr = Resolve(lpici->hwnd, 0);
    if (FAILED(hr))
    {
        TRACE("failed to resolve component with error 0x%08x", hr);
        return hr;
    }

    path = strdupW(m_sPath);

    if ( lpici->cbSize == sizeof(CMINVOKECOMMANDINFOEX) &&
        (lpici->fMask & CMIC_MASK_UNICODE) )
    {
        LPCMINVOKECOMMANDINFOEX iciex = (LPCMINVOKECOMMANDINFOEX)lpici;
        SIZE_T len = 2;

        if (m_sArgs)
            len += wcslen(m_sArgs);
        if (iciex->lpParametersW)
            len += wcslen(iciex->lpParametersW);

        args = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        *args = 0;
        if (m_sArgs)
            wcscat(args, m_sArgs);
        if (iciex->lpParametersW)
        {
            wcscat(args, L" ");
            wcscat(args, iciex->lpParametersW);
        }
    }
    else if (m_sArgs != NULL)
    {
        args = strdupW(m_sArgs);
    }

    SHELLEXECUTEINFOW sei;
    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_HASLINKNAME | SEE_MASK_UNICODE |
               (lpici->fMask & (SEE_MASK_NOASYNC | SEE_MASK_ASYNCOK | SEE_MASK_FLAG_NO_UI));
    sei.lpFile = path;
    sei.lpClass = m_sLinkPath;
    sei.nShow = m_Header.nShowCommand;
    sei.lpDirectory = m_sWorkDir;
    sei.lpParameters = args;
    sei.lpVerb = L"open";

    // HACK for ShellExecuteExW
    if (m_sPath && wcsstr(m_sPath, L".cpl"))
        sei.lpVerb = L"cplopen";

    if (ShellExecuteExW(&sei))
        hr = S_OK;
    else
        hr = E_FAIL;

    HeapFree(GetProcessHeap(), 0, args);
    HeapFree(GetProcessHeap(), 0, path);

    return hr;
}

HRESULT STDMETHODCALLTYPE CShellLink::GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pwReserved, LPSTR pszName, UINT cchMax)
{
    FIXME("%p %lu %u %p %p %u\n", this, idCmd, uType, pwReserved, pszName, cchMax);
    return E_NOTIMPL;
}

INT_PTR CALLBACK ExtendedShortcutProc(HWND hwndDlg, UINT uMsg,
                                      WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            if (lParam)
            {
                HWND hDlgCtrl = GetDlgItem(hwndDlg, 14000);
                SendMessage(hDlgCtrl, BM_SETCHECK, BST_CHECKED, 0);
            }
            return TRUE;
        case WM_COMMAND:
        {
            HWND hDlgCtrl = GetDlgItem(hwndDlg, 14000);
            if (LOWORD(wParam) == IDOK)
            {
                if (SendMessage(hDlgCtrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
                    EndDialog(hwndDlg, 1);
                else
                    EndDialog(hwndDlg, 0);
            }
            else if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hwndDlg, -1);
            }
            else if (LOWORD(wParam) == 14000)
            {
                if (SendMessage(hDlgCtrl, BM_GETCHECK, 0, 0) == BST_CHECKED)
                    SendMessage(hDlgCtrl, BM_SETCHECK, BST_UNCHECKED, 0);
                else
                    SendMessage(hDlgCtrl, BM_SETCHECK, BST_CHECKED, 0);
            }
        }
    }
    return FALSE;
}

HRESULT
WINAPI
SHOpenFolderAndSelectItems(LPITEMIDLIST pidlFolder,
                           UINT cidl,
                           PCUITEMID_CHILD_ARRAY apidl,
                           DWORD dwFlags);

/**************************************************************************
* SH_GetTargetTypeByPath
*
* Function to get target type by passing full path to it
*/
LPWSTR SH_GetTargetTypeByPath(LPCWSTR lpcwFullPath)
{
    LPCWSTR pwszExt;
    static WCHAR wszBuf[MAX_PATH];

    /* Get file information */
    SHFILEINFOW fi;
    if (!SHGetFileInfoW(lpcwFullPath, 0, &fi, sizeof(fi), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES))
    {
        ERR("SHGetFileInfoW failed for %ls (%lu)\n", lpcwFullPath, GetLastError());
        fi.szTypeName[0] = L'\0';
        fi.hIcon = NULL;
    }

    pwszExt = PathFindExtensionW(lpcwFullPath);
    if (pwszExt[0])
    {
        if (!fi.szTypeName[0])
        {
            /* The file type is unknown, so default to string "FileExtension File" */
            size_t cchRemaining = 0;
            LPWSTR pwszEnd = NULL;

            StringCchPrintfExW(wszBuf, _countof(wszBuf), &pwszEnd, &cchRemaining, 0, L"%s ", pwszExt + 1);
        }
        else
        {
            /* Update file type */
            StringCbPrintfW(wszBuf, sizeof(wszBuf), L"%s (%s)", fi.szTypeName, pwszExt);
        }
    }

    return wszBuf;
}

/**************************************************************************
 * SH_ShellLinkDlgProc
 *
 * dialog proc of the shortcut property dialog
 */

INT_PTR CALLBACK CShellLink::SH_ShellLinkDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CShellLink *pThis = reinterpret_cast<CShellLink *>(GetWindowLongPtr(hwndDlg, DWLP_USER));

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            LPPROPSHEETPAGEW ppsp = (LPPROPSHEETPAGEW)lParam;
            if (ppsp == NULL)
                break;

            TRACE("ShellLink_DlgProc (WM_INITDIALOG hwnd %p lParam %p ppsplParam %x)\n", hwndDlg, lParam, ppsp->lParam);

            pThis = reinterpret_cast<CShellLink *>(ppsp->lParam);
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pThis);

            TRACE("m_sArgs: %S sComponent: %S m_sDescription: %S m_sIcoPath: %S m_sPath: %S m_sPathRel: %S sProduct: %S m_sWorkDir: %S\n", pThis->m_sArgs, pThis->sComponent, pThis->m_sDescription,
                  pThis->m_sIcoPath, pThis->m_sPath, pThis->m_sPathRel, pThis->sProduct, pThis->m_sWorkDir);

            /* Get file information */
            // FIXME! FIXME! Shouldn't we use pThis->m_sIcoPath, pThis->m_Header.nIconIndex instead???
            SHFILEINFOW fi;
            if (!SHGetFileInfoW(pThis->m_sLinkPath, 0, &fi, sizeof(fi), SHGFI_TYPENAME | SHGFI_ICON))
            {
                ERR("SHGetFileInfoW failed for %ls (%lu)\n", pThis->m_sLinkPath, GetLastError());
                fi.szTypeName[0] = L'\0';
                fi.hIcon = NULL;
            }

            if (fi.hIcon) // TODO: destroy icon
                SendDlgItemMessageW(hwndDlg, 14000, STM_SETICON, (WPARAM)fi.hIcon, 0);
            else
                ERR("ExtractIconW failed %ls %u\n", pThis->m_sIcoPath, pThis->m_Header.nIconIndex);

            /* Target type */
            if (pThis->m_sPath)
                SetDlgItemTextW(hwndDlg, 14005, SH_GetTargetTypeByPath(pThis->m_sPath));

            /* Target location */
            if (pThis->m_sPath)
            {
                WCHAR target[MAX_PATH];
                StringCchCopyW(target, _countof(target), pThis->m_sPath);
                PathRemoveFileSpecW(target);
                SetDlgItemTextW(hwndDlg, 14007, PathFindFileNameW(target));
            }

            /* Target path */
            if (pThis->m_sPath)
            {
                WCHAR newpath[2*MAX_PATH] = L"\0";
                if (wcschr(pThis->m_sPath, ' '))
                    StringCchPrintfExW(newpath, _countof(newpath), NULL, NULL, 0, L"\"%ls\"", pThis->m_sPath);
                else
                    StringCchCopyExW(newpath, _countof(newpath), pThis->m_sPath, NULL, NULL, 0);

                if (pThis->m_sArgs && pThis->m_sArgs[0])
                {
                    StringCchCatW(newpath, _countof(newpath), L" ");
                    StringCchCatW(newpath, _countof(newpath), pThis->m_sArgs);
                }
                SetDlgItemTextW(hwndDlg, 14009, newpath);
            }

            /* Working dir */
            if (pThis->m_sWorkDir)
                SetDlgItemTextW(hwndDlg, 14011, pThis->m_sWorkDir);

            /* Description */
            if (pThis->m_sDescription)
                SetDlgItemTextW(hwndDlg, 14019, pThis->m_sDescription);

            return TRUE;
        }

        case WM_NOTIFY:
        {
            LPPSHNOTIFY lppsn = (LPPSHNOTIFY)lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
                WCHAR wszBuf[MAX_PATH];
                /* set working directory */
                GetDlgItemTextW(hwndDlg, 14011, wszBuf, _countof(wszBuf));
                pThis->SetWorkingDirectory(wszBuf);
                /* set link destination */
                GetDlgItemTextW(hwndDlg, 14009, wszBuf, _countof(wszBuf));
                LPWSTR lpszArgs = NULL;
                LPWSTR unquoted = strdupW(wszBuf);
                StrTrimW(unquoted, L" ");
                if (!PathFileExistsW(unquoted))
                {
                    lpszArgs = PathGetArgsW(unquoted);
                    PathRemoveArgsW(unquoted);
                    StrTrimW(lpszArgs, L" ");
                }
                if (unquoted[0] == '"' && unquoted[wcslen(unquoted)-1] == '"')
                    PathUnquoteSpacesW(unquoted);


                WCHAR *pwszExt = PathFindExtensionW(unquoted);
                if (!wcsicmp(pwszExt, L".lnk"))
                {
                    // FIXME load localized error msg
                    MessageBoxW(hwndDlg, L"You cannot create a link to a shortcut", L"Error", MB_ICONERROR);
                    SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                    return TRUE;
                }

                if (!PathFileExistsW(unquoted))
                {
                    // FIXME load localized error msg
                    MessageBoxW(hwndDlg, L"The specified file name in the target box is invalid", L"Error", MB_ICONERROR);
                    SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                    return TRUE;
                }

                pThis->SetPath(unquoted);
                if (lpszArgs)
                    pThis->SetArguments(lpszArgs);
                else
                    pThis->SetArguments(L"\0");

                HeapFree(GetProcessHeap(), 0, unquoted);

                TRACE("This %p m_sLinkPath %S\n", pThis, pThis->m_sLinkPath);
                pThis->Save(pThis->m_sLinkPath, TRUE);
                SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
                return TRUE;
            }
            break;
        }

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case 14020:
                    SHOpenFolderAndSelectItems(pThis->m_pPidl, 0, NULL, 0);
                    ///
                    /// FIXME
                    /// open target directory
                    ///
                    return TRUE;

                case 14021:
                {
                    WCHAR wszPath[MAX_PATH] = L"";

                    if (pThis->m_sIcoPath)
                        wcscpy(wszPath, pThis->m_sIcoPath);
                    INT IconIndex = pThis->m_Header.nIconIndex;
                    if (PickIconDlg(hwndDlg, wszPath, _countof(wszPath), &IconIndex))
                    {
                        pThis->SetIconLocation(wszPath, IconIndex);
                        ///
                        /// FIXME redraw icon
                        ///
                    }
                    return TRUE;
                }

                case 14022:
                {
                    INT_PTR result = DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(IDD_SHORTCUT_EXTENDED_PROPERTIES), hwndDlg, ExtendedShortcutProc, (LPARAM)pThis->m_bRunAs);
                    if (result == 1 || result == 0)
                    {
                        if (pThis->m_bRunAs != result)
                        {
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        }

                        pThis->m_bRunAs = result;
                    }
                    return TRUE;
                }
            }
            if (HIWORD(wParam) == EN_CHANGE)
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            break;

        default:
            break;
    }
    return FALSE;
}

/**************************************************************************
 * ShellLink_IShellPropSheetExt interface
 */

HRESULT STDMETHODCALLTYPE CShellLink::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    HPROPSHEETPAGE hPage = SH_CreatePropertySheetPage(IDD_SHORTCUT_PROPERTIES, SH_ShellLinkDlgProc, (LPARAM)this, NULL);
    if (hPage == NULL)
    {
        ERR("failed to create property sheet page\n");
        return E_FAIL;
    }

    if (!pfnAddPage(hPage, lParam))
        return E_FAIL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplacePage, LPARAM lParam)
{
    TRACE("(%p) (uPageID %u, pfnReplacePage %p lParam %p\n", this, uPageID, pfnReplacePage, lParam);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CShellLink::SetSite(IUnknown *punk)
{
    TRACE("%p %p\n", this, punk);

    m_site = punk;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::GetSite(REFIID iid, void ** ppvSite)
{
    TRACE("%p %s %p\n", this, debugstr_guid(&iid), ppvSite);

    if (m_site == NULL)
        return E_FAIL;

    return m_site->QueryInterface(iid, ppvSite);
}

HRESULT STDMETHODCALLTYPE CShellLink::DragEnter(IDataObject *pDataObject,
    DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p)->(DataObject=%p)\n", this, pDataObject);
    LPCITEMIDLIST pidlLast;
    CComPtr<IShellFolder> psf;

    HRESULT hr = SHBindToParent(m_pPidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast);

    if (SUCCEEDED(hr))
    {
        hr = psf->GetUIObjectOf(0, 1, &pidlLast, IID_NULL_PPV_ARG(IDropTarget, &m_DropTarget));

        if (SUCCEEDED(hr))
            hr = m_DropTarget->DragEnter(pDataObject, dwKeyState, pt, pdwEffect);
        else
            *pdwEffect = DROPEFFECT_NONE;
    }
    else
        *pdwEffect = DROPEFFECT_NONE;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CShellLink::DragOver(DWORD dwKeyState, POINTL pt,
    DWORD *pdwEffect)
{
    TRACE("(%p)\n", this);
    HRESULT hr = S_OK;
    if (m_DropTarget)
        hr = m_DropTarget->DragOver(dwKeyState, pt, pdwEffect);
    return hr;
}

HRESULT STDMETHODCALLTYPE CShellLink::DragLeave()
{
    TRACE("(%p)\n", this);
    HRESULT hr = S_OK;
    if (m_DropTarget)
    {
        hr = m_DropTarget->DragLeave();
        m_DropTarget.Release();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CShellLink::Drop(IDataObject *pDataObject,
    DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p)\n", this);
    HRESULT hr = S_OK;
    if (m_DropTarget)
        hr = m_DropTarget->Drop(pDataObject, dwKeyState, pt, pdwEffect);

    return hr;
}

/**************************************************************************
 *      IShellLink_ConstructFromFile
 */
HRESULT WINAPI IShellLink_ConstructFromPath(WCHAR *path, REFIID riid, LPVOID *ppv)
{
    CComPtr<IPersistFile> ppf;
    HRESULT hr = CShellLink::_CreatorClass::CreateInstance(NULL, IID_PPV_ARG(IPersistFile, &ppf));
    if (FAILED(hr))
        return hr;

    hr = ppf->Load(path, 0);
    if (FAILED(hr))
        return hr;

    return ppf->QueryInterface(riid, ppv);
}

HRESULT WINAPI IShellLink_ConstructFromFile(IShellFolder * psf, LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppv)
{
    WCHAR path[MAX_PATH];
    if (!ILGetDisplayNameExW(psf, pidl, path, 0))
        return E_FAIL;

    return IShellLink_ConstructFromPath(path, riid, ppv);
}
