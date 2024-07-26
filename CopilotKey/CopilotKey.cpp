// CopilotKey.cpp : This file is the main file for the Copilot Key replacement for PCs that do not have a Copilot button.
//

#include <iostream>
#include <Windows.h>
#include <Shlobj.h>

using namespace std;

HOOKPROC address;
static HHOOK keyboard_hook;

int InstallProgram(HINSTANCE hInstance, LPCWSTR ExecutablePath, LPCWSTR InstallDestinationFolder) {
    wstring InstallDestinationPath1 = InstallDestinationFolder;
    wstring InstallDestinationPath = InstallDestinationPath1 + L"\\CopilotKey.exe";
    CopyFile(ExecutablePath, InstallDestinationPath.c_str(), FALSE);

    HKEY hKey;
    LONG result;
    const wchar_t* subKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    const wchar_t* valueName = L"CopilotKey";
    const wchar_t* valueData = InstallDestinationPath.c_str();

    // Open the registry key
    result = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,  // Root key
        subKey,             // Subkey path
        0,                  // Reserved, set to 0
        KEY_SET_VALUE,      // Access rights (write access)
        &hKey               // Handle to the opened key
    );

    if (result != ERROR_SUCCESS) {
        std::cerr << "Failed to open registry key: " << result << std::endl;
        return 1;
    }

    // Write the value to the registry
    result = RegSetValueEx(
        hKey,               // Handle to the opened key
        valueName,          // Value name
        0,                  // Reserved, set to 0
        REG_SZ,             // Value type (string)
        (const BYTE*)valueData, // Value data
        (wcslen(valueData) + 1) * sizeof(wchar_t) // Value data size (in bytes)
    );

    if (result != ERROR_SUCCESS) {
        return 1;
    }

    // Close the registry key
    RegCloseKey(hKey);
    return 0;
}

BOOL IsElevated() {
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if (hToken) {
        CloseHandle(hToken);
    }
    return fRet;
}

int CopilotButton()
{
    // This structure will be used to create the keyboard
// input event.
    INPUT ip[6] = {0};

    ip[0].type = INPUT_KEYBOARD;
    ip[0].ki.wScan = 0; // hardware scan code for key
    ip[0].ki.time = 0;
    ip[0].ki.dwExtraInfo = 0;
    ip[0].ki.wVk = VK_LSHIFT;
    ip[0].ki.dwFlags = 0; // 0 for key press

    // Press the Windows key
    ip[1].type = INPUT_KEYBOARD;
    ip[1].ki.wScan = 0; // hardware scan code for key
    ip[1].ki.time = 0;
    ip[1].ki.dwExtraInfo = 0;
    ip[1].ki.wVk = VK_LWIN;
    ip[1].ki.dwFlags = 0; // 0 for key press

    // Press the F23 key
    ip[2].type = INPUT_KEYBOARD;
    ip[2].ki.wScan = 0; // hardware scan code for key
    ip[2].ki.time = 0;
    ip[2].ki.dwExtraInfo = 0;
    ip[2].ki.wVk = VK_F23;
    ip[2].ki.dwFlags = 0; // 0 for key press

    ip[3] = ip[2];
    ip[3].ki.dwFlags |= KEYEVENTF_KEYUP;

    ip[4] = ip[1];
    ip[4].ki.dwFlags |= KEYEVENTF_KEYUP;

    ip[5] = ip[0];
    ip[5].ki.dwFlags |= KEYEVENTF_KEYUP;

    SendInput(6, ip, sizeof(INPUT));

    // Exit normally
    return 0;
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    BOOL fEatKeystroke = FALSE;

    if (nCode == HC_ACTION)
    {
        switch (wParam)
        {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
            PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
            if (fEatKeystroke = (p->vkCode == VK_APPS)) {
                CopilotButton();
                break;
            }
            break;
        }
    }
    return(fEatKeystroke ? 1 : CallNextHookEx(NULL, nCode, wParam, lParam));
}
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    LPWSTR* argv;
    int argc;

    LPWSTR commandLine = GetCommandLineW(); // Get the command line string
    argv = CommandLineToArgvW(commandLine, &argc); // Parse into arguments

    // Use argc and argv as before

    if ((argc == 2) && _wcsicmp(argv[1], L"/install") == 0)
    {
        TCHAR szExePath[MAX_PATH];
        GetModuleFileName(NULL, szExePath, MAX_PATH);

        if (IsElevated() == FALSE)
        {
            CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
            WCHAR cd[MAX_PATH];
            GetCurrentDirectory(MAX_PATH, cd);
            SHELLEXECUTEINFO info = {0};
            info.hInstApp = hInstance;
            info.lpVerb = L"runas";
            info.lpFile = szExePath;
            info.lpParameters = L"/install";
            info.lpDirectory = cd;
            info.cbSize = sizeof(info);
            info.fMask = NULL;
            ShellExecuteEx(&info);
            if (GetLastError() != 0) {
                WCHAR buf[256];
                FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, ERROR_OPERATION_ABORTED, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    buf, (sizeof(buf) / sizeof(WCHAR)), NULL);
                MessageBox(NULL, buf, L"Error", MB_ICONERROR + MB_OK);
            return ERROR_OPERATION_ABORTED;
            }
            return ERROR_SUCCESS;
        }
        else {
                    LPWSTR ProgramDataFolder[MAX_PATH];
        SHGetKnownFolderPath(FOLDERID_ProgramData, NULL, GetCurrentProcessToken(), ProgramDataFolder);
        InstallProgram(hInstance, szExePath, *ProgramDataFolder);

        ExitProcess(GetLastError());
        }

    }
    else {
    address = LowLevelKeyboardProc;
        keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, address, hInstance, 0);
        // Keep this app running until we're told to stop
        MSG msg;
        while (!GetMessage(&msg, NULL, NULL, NULL)) {    //this while loop keeps the hook
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        UnhookWindowsHookEx(keyboard_hook);
    return 0;
    }
    LocalFree(argv); // Free memory allocated by CommandLineToArgvW

}