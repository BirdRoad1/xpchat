#include "central.h"
#include <windows.h>
#include <commctrl.h>
#include <iostream>
#include <vector>
#include <xpchat/server.h>
#include <chrono>
#include <iomanip>
#include <xpchat/chat_protocol.h>

#define IDC_SCANBTN 1
#define IDC_SERVER_LIST 2
HWND window;
HWND connectBtn;
HWND statusText;
HWND serverListWindow;
std::vector<Server> servers;

void AddMenus(HWND hwnd)
{
    connectBtn = CreateWindowExW(WS_EX_CLIENTEDGE, L"BUTTON", L"Connect to central server", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 0, 200, 50, hwnd, (HMENU)IDC_SCANBTN, GetModuleHandleW(NULL), NULL);
}

void ConnectChatServer()
{
    // Now start scanning
    std::cout << "Connect to chat server was clicked!" << std::endl;
    Central &central = Central::getInstance();
    bool result = central.connect();
    std::cout << "Post connect" << std::endl;
    if (result)
    {
        if (connectBtn)
        {
            DestroyWindow(connectBtn);
        }

        statusText = CreateWindowExW(WS_EX_CLIENTEDGE, L"Static", L"Status: Connected to central", WS_CHILD | WS_VISIBLE, 0, 0, 200, 25, window, NULL, GetModuleHandleW(NULL), 0);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"Static", L"Available chat servers:", WS_CHILD | WS_VISIBLE, 0, 25, 180, 25, window, NULL, GetModuleHandleW(NULL), 0);

        // List servers
        try
        {
            if (!central.listServers(servers))
            {
                MessageBoxW(window, L"Error", L"Failed to list servers", MB_OK);
                return;
            }
        }
        catch (std::runtime_error &err)
        {
            MessageBoxW(window, L"Error", L"Failed to list servers", MB_OK);
            std::cout << "List servers exception: " << err.what() << std::endl;
            return;
        }

        std::cout << "Create window!" << std::endl;

        serverListWindow = CreateWindowExW(WS_EX_CLIENTEDGE, L"SysListView32", L"", WS_CHILD | WS_VISIBLE | LVS_REPORT, 0, 50, 450, 380, window, (HMENU)IDC_SERVER_LIST, GetModuleHandleW(NULL), 0);

        LVCOLUMN ipCol = {0};
        ipCol.mask = LVCF_WIDTH | LVCF_TEXT;
        ipCol.cx = 100;              // Set column width (e.g., 400 pixels)
        ipCol.pszText = "Server IP"; // Column header text
        ListView_InsertColumn(serverListWindow, 0, &ipCol);

        LVCOLUMN idCol = {0};
        idCol.mask = LVCF_WIDTH | LVCF_TEXT;
        idCol.cx = 150;                      // Set column width (e.g., 400 pixels)
        idCol.pszText = "Server Identifier"; // Column header text
        ListView_InsertColumn(serverListWindow, 1, &idCol);

        LVCOLUMN joinDateCol = {0};
        joinDateCol.mask = LVCF_WIDTH | LVCF_TEXT;
        joinDateCol.cx = 150;              // Set column width (e.g., 400 pixels)
        joinDateCol.pszText = "Join Date"; // Column header text
        ListView_InsertColumn(serverListWindow, 2, &joinDateCol);

        for (const Server &server : servers)
        {

            LVITEM lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = 0; // Insert at the top
            std::string str(server.ip);
            lvi.pszText = &str[0];
            ListView_InsertItem(serverListWindow, &lvi);

            LVITEM subItem = {0};
            subItem.mask = LVIF_TEXT;
            subItem.iItem = 0;
            subItem.iSubItem = 1; // Second column
            std::string str1(server.identifier);
            subItem.pszText = &str1[0];
            ListView_SetItem(serverListWindow, &subItem);

            LVITEM subItem2 = {0};
            subItem2.mask = LVIF_TEXT;
            subItem2.iItem = 0;
            subItem2.iSubItem = 2; // Second column
            std::stringstream ss;
            ss << std::put_time(std::gmtime(&server.connectedTimestamp), "%Y-%m-%d %I:%M:%S %p");
            std::string str2 = ss.str();
            subItem2.pszText = &str2[0];

            ListView_SetItem(serverListWindow, &subItem2);
        }
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
    else if (msg == WM_NOTIFY)
    {
        LPNMHDR nmhdr = (LPNMHDR)lParam;
        if (nmhdr->idFrom == IDC_SERVER_LIST && nmhdr->code == NM_DBLCLK)
        {
            LPNMITEMACTIVATE nmitem = (LPNMITEMACTIVATE)lParam;
            int row = nmitem->iItem;
            int col = nmitem->iSubItem;
            if (row >= 0)
            {
                const Server &server = servers[row];
                std::cout << "Double clicked row:" << server.ip << std::endl;
                Central::getInstance().sendServerJoin(server.socket);
            }
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