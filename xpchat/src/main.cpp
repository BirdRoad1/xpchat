#include "central.h"
#include <windows.h>
#include <iostream>

#define IDC_SCANBTN 1
HWND window;
HWND connectBtn;
HWND statusText;

void AddMenus(HWND hwnd)
{
    connectBtn = CreateWindowExW(WS_EX_CLIENTEDGE, L"BUTTON", L"Connect to central server", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 0, 200, 50, hwnd, (HMENU)IDC_SCANBTN, GetModuleHandleW(NULL), NULL);
}

void ConnectChatServer()
{
    // Now start scanning
    std::cout << "Connect to chat server was clicked!" << std::endl;
    Central& central = Central::getInstance();
    bool result = central.connect();
    if (result)
    {
        if (connectBtn)
        {
            DestroyWindow(connectBtn);
        }

        statusText = CreateWindowExW(WS_EX_CLIENTEDGE, L"Static", L"Status: Connected to central", WS_CHILD | WS_VISIBLE, 0, 0, 200, 25, window, NULL, GetModuleHandleW(NULL), 0);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"Static", L"Available chat servers:", WS_CHILD | WS_VISIBLE, 0, 25, 180, 25, window, NULL, GetModuleHandleW(NULL), 0);

        // List servers
        central.listServers();
    }
}

LRESULT WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    if (msg == WM_CREATE)
    {
        AddMenus(hwnd);
        return 0;
    }
    else if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }
    else if (msg == WM_COMMAND)
    {

        WORD notificationCode = HIWORD(wParam);
        WORD controlId = LOWORD(wParam);

        if (notificationCode == BN_CLICKED && controlId == IDC_SCANBTN)
        {
            ConnectChatServer();
            return 0;
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR lpCmdLine, int nCmdShow)
{
    std::cout << "XPChatter 1.0" << std::endl;

    // socket startup
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData))
    {
        std::cout << "Failed to startup winsock" << std::endl;
    }

    WNDCLASSW newWindow = {};
    newWindow.hbrBackground = (HBRUSH)COLOR_WINDOW;
    newWindow.hCursor = LoadCursor(NULL, IDC_ARROW);
    newWindow.hInstance = hInstance;
    newWindow.lpszClassName = L"xpchatter";
    newWindow.lpfnWndProc = WndProc;

    if (!RegisterClassW(&newWindow))
    {
        std::cout << "Failed to register class" << std::endl;
        return -1;
    }

    HWND hwnd;
    if (!(hwnd = CreateWindowW(newWindow.lpszClassName, L"XPChatter", WS_VISIBLE | WS_OVERLAPPEDWINDOW, 100, 100, 500, 500, NULL, NULL, hInstance, NULL)))
    {
        std::cout << "Failed to create window" << std::endl;
        return -1;
    }

    window = hwnd;

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}