#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <winreg.h>

// IDs dos controles
#define IDC_COMBOBOX 101
#define IDC_APPLY_BUTTON 102
#define IDC_DELETE_BUTTON 103
#define IDC_CANCEL_BUTTON 104

// Opções do dropdown
const TCHAR* versions[] = {
    _T("Windows 2000"),
    _T("Windows XP SP3"),
    _T("Windows Server 2003 SP2"),
    _T("Windows Vista SP2"),
    _T("Windows 7 SP1"),
    _T("Windows 8.1"),
    _T("Windows 10 1511"),
    _T("Windows 10 1607"),
    _T("Windows 10 1809"),
    _T("Windows 10 22H2"),
    _T("Windows 11 24H2")
};


// Valores associados às opções
const TCHAR* versionValues[] = {
    _T("5.0.2195"),
    _T("5.1.2600"),
    _T("5.2.3790"),
    _T("6.0.6002"),
    _T("6.1.7601"),
    _T("6.3.9600"),
    _T("10.0.10586"),
    _T("10.0.14393"),
    _T("10.0.17763"),
    _T("10.0.19045"),
    _T("10.0.22600")
};

// Prototipo das funções
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void WriteToRegistry(const TCHAR*);
void DeleteRegistryKey();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
	HWND hwnd;
	MSG msg = {0};
	
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = _T("DropdownApp");
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc)) return -1;

    hwnd = CreateWindow(
        _T("DropdownApp"), 
        _T("Windows Compatibility Tool"), 
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
        CW_USEDEFAULT, CW_USEDEFAULT, 
        400, 200, 
        NULL, NULL, hInstance, NULL
    );
    
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hComboBox, hApplyButton, hDeleteButton, hCancelButton;
    static HFONT hFont;
	int i;

    switch (msg) {
        case WM_CREATE: {
	        // Criar a fonte Arial
	        hFont = CreateFont(
	            20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
	            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
	            DEFAULT_PITCH | FF_SWISS, TEXT("Arial")
	        );        	
        	
            // Dropdown List
            hComboBox = CreateWindow(
                WC_COMBOBOXW, NULL,
                CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
                15, 20, 360, 200,
                hwnd, (HMENU)IDC_COMBOBOX,
                GetModuleHandle(NULL), NULL
            );

            // Adiciona as opções ao ComboBox
            for (i = 0; i < sizeof(versions) / sizeof(versions[0]); ++i) {
                SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)versions[i]);
            }
            
            // Definir o índice padrão (por exemplo, 1 - "Item 2")
            SendMessage(hComboBox, CB_SETCURSEL, 0, 0);
            // Botão Aplicar
            hApplyButton = CreateWindow(
                _T("BUTTON"), _T("Apply"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                35, 80, 100, 30,
                hwnd, (HMENU)IDC_APPLY_BUTTON,
                GetModuleHandle(NULL), NULL
            );

            // Botão Deletar
            hDeleteButton = CreateWindow(
                _T("BUTTON"), _T("Delete"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD,
                145, 80, 100, 30,
                hwnd, (HMENU)IDC_DELETE_BUTTON,
                GetModuleHandle(NULL), NULL
            );

            // Botão Cancelar
            hCancelButton = CreateWindow(
                _T("BUTTON"), _T("Cancel"),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD,
                255, 80, 100, 30,
                hwnd, (HMENU)IDC_CANCEL_BUTTON,
                GetModuleHandle(NULL), NULL
            );
            
	        // Aplicar a fonte Arial a todos os controles
	        SendMessage(hComboBox, WM_SETFONT, (WPARAM)hFont, TRUE);
	        SendMessage(hApplyButton, WM_SETFONT, (WPARAM)hFont, TRUE);
	        SendMessage(hDeleteButton, WM_SETFONT, (WPARAM)hFont, TRUE);
	        SendMessage(hCancelButton, WM_SETFONT, (WPARAM)hFont, TRUE);            
            break;
        }

        case WM_COMMAND: {
            if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_COMBOBOX) {
                // Seleção mudou no ComboBox
            }

            switch (LOWORD(wParam)) {
                case IDC_APPLY_BUTTON: {
                    // Obtém a seleção do ComboBox
				    int idx = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
				    if (idx != CB_ERR) {
				        const TCHAR* selectedValue = versionValues[idx];
				        WriteToRegistry(selectedValue);
				    } else {
				        MessageBox(hwnd, _T("Please, select a version."), _T("Error"), MB_OK | MB_ICONERROR);
				    }
				    break;
                }

                case IDC_DELETE_BUTTON: {
                    SendMessage(hComboBox, CB_SETCURSEL, -1, 0);
                    DeleteRegistryKey();
                    //MessageBox(hwnd, _T("Seleção deletada e chave de registro removida!"), _T("Deletar"), MB_OK | MB_ICONINFORMATION);
                    break;
                }

                case IDC_CANCEL_BUTTON: {
                    PostQuitMessage(0);
                    break;
                }
            }
            break;
        }

        case WM_DESTROY: {
            PostQuitMessage(0);
            break;
        }

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void WriteToRegistry(const TCHAR* value) {
    HKEY hKey;
    int msgboxID;
    LONG result;
#ifdef _M_AMD64      
    LONG resultWow64;
    HKEY Wow64hKey;
#endif	    
    
    result = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
        0, KEY_SET_VALUE, &hKey
    );
    
#ifdef _M_AMD64     
    resultWow64 = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        _T("SOFTWARE\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion"),
        0, KEY_SET_VALUE, &Wow64hKey
    );  
#endif	  

    if (result == ERROR_SUCCESS) {
        RegSetValueEx(hKey, _T("EmulatedVersion"), 0, REG_SZ, (const BYTE*)value, (_tcslen(value) + 1) * sizeof(TCHAR));
#ifdef _M_AMD64 
        if(resultWow64 == ERROR_SUCCESS){
            RegSetValueEx(Wow64hKey, _T("EmulatedVersion"), 0, REG_SZ, (const BYTE*)value, (_tcslen(value) + 1) * sizeof(TCHAR));	
		}
#endif       
        RegCloseKey(hKey);
        msgboxID = MessageBox(NULL, _T("Value saved with success on registry!"), _T("Success"), MB_OK | MB_ICONINFORMATION);
        if(msgboxID == IDOK){
        	PostQuitMessage(0);
		}        
    } else {
        MessageBox(NULL, _T("Error while trying access the registry key."), _T("Error"), MB_OK | MB_ICONERROR);
    }
}

void DeleteRegistryKey() {
    HKEY hKey;
	LONG result;
	int msgboxID;
#ifdef _M_AMD64      
    LONG resultWow64;
    HKEY Wow64hKey;
#endif	

    result = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
        0, KEY_SET_VALUE, &hKey
    );
	
#ifdef _M_AMD64     
    resultWow64 = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        _T("SOFTWARE\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion"),
        0, KEY_SET_VALUE, &Wow64hKey
    );  
#endif	

    if (result == ERROR_SUCCESS) {
        result = RegDeleteValue(hKey, _T("EmulatedVersion"));
#ifdef _M_AMD64 
        if(resultWow64 == ERROR_SUCCESS){
            resultWow64 = RegDeleteValue(Wow64hKey, _T("EmulatedVersion"));	
		}
#endif 		
        RegCloseKey(hKey);
#ifdef _M_AMD64 
        RegCloseKey(Wow64hKey);
#endif		

        if (result == ERROR_SUCCESS) {
            msgboxID = MessageBox(NULL, _T("Registry Key removed with success!"), _T("Success"), MB_OK | MB_ICONINFORMATION);
			if(msgboxID == IDOK){
				PostQuitMessage(0);
			} 			
        } else {
            MessageBox(NULL, _T("Error while trying remove the registry key value."), _T("Error"), MB_OK | MB_ICONERROR);
        }
    } else {
        MessageBox(NULL, _T("Error trying open the registry key."), _T("Erro"), MB_OK | MB_ICONERROR);
    }
}

