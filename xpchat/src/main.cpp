#include "central.h"
#include "winsock2.h"
#include <windows.h>
#include <commctrl.h>
#include <iostream>
#include <vector>
#include <xpchat/server.h>
#include <chrono>
#include <iomanip>
#include <xpchat/chat_protocol.h>
#include "chat.h"
#include <richedit.h>
#include <thread>

#define IDC_SCANBTN 1
#define IDC_SERVER_LIST 2
#define WM_SHOW_CHAT (WM_USER + 1)

HWND window;
HWND connectBtn;
HWND statusText;
HWND availableServersLabel;
HWND serverListWindow;
HWND chatBox;
HWND inputBox;
std::vector<Server> servers;
std::string chatBoxText;

void AddMenus(HWND hwnd)
{
    connectBtn = CreateWindowExW(WS_EX_CLIENTEDGE, L"BUTTON", L"Connect to central server", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 0, 200, 50, hwnd, (HMENU)IDC_SCANBTN, GetModuleHandleW(NULL), NULL);
}

void ShowChat()
{
    PostMessage(window, WM_SHOW_CHAT, 0, 0);
}

void SocketThread()
{
    std::cout << "Start listening for msgs" << std::endl;
    int fd;
    while ((fd = Chat::getInstance().getSocket()) >= 0)
    {
        std::string cmd;
        if (!ChatProtocol::readString(fd, cmd))
        {
            std::cout << "Failed to read cmd" << std::endl;
            return;
        }

        std::cout << "RECEIVED CMD" << cmd << std::endl;

        if (cmd == "MSG")
        {
            std::string msg;
            if (!ChatProtocol::readString(fd, msg))
            {
                std::cout << "Failed to read message" << std::endl;
                return;
            }

            chatBoxText += msg + "\n";
            SetWindowTextW(chatBox, std::wstring(chatBoxText.begin(), chatBoxText.end()).c_str());
        }
        else if (cmd == "RUSR")
        {
            int result;
            if (!ChatProtocol::readInt(fd, result))
            {
                std::cout << "Failed to read register result" << std::endl;
                continue;
            }

            std::cout << "RUSR RESULT: " << result << std::endl;

            if (result == 0)
            {
                std::string reason;
                if (!ChatProtocol::readString(fd, reason))
                {
                    reason = "Unknown reason";
                }

                Chat::getInstance().disconnect();
                MessageBoxW(window, std::wstring(reason.begin(), reason.end()).c_str(), L"Registration failed", MB_OK);
            }
            else
            {
                ShowChat();
                MessageBoxW(window, L"You are now registered with the server", L"Logged in", MB_OK);
            }
        }
    }
    std::cout << "SOCKET THREAD ENDED" << std::endl;
}

void SendMessageString(std::string msg)
{
    int fd = Chat::getInstance().getSocket();
    if (fd == -1)
    {
        std::cout << "No server to send message to" << std::endl;
        return;
    }

    std::cout << "SEND MSG:" << msg << std::endl;
    ChatProtocol::writeString(fd, "MSG");
    ChatProtocol::writeString(fd, msg);
    ChatProtocol::flush(fd);
    std::cout << "POST SEND MSG:" << msg << std::endl;
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

        statusText = CreateWindowExW(WS_EX_CLIENTEDGE, L"Static", L"Status: Connected to central", WS_CHILD | WS_VISIBLE, 0, 0, 500, 25, window, NULL, GetModuleHandleW(NULL), 0);
        availableServersLabel = CreateWindowExW(WS_EX_CLIENTEDGE, L"Static", L"Available chat servers:", WS_CHILD | WS_VISIBLE, 0, 25, 180, 25, window, NULL, GetModuleHandleW(NULL), 0);

        // List servers
        try
        {
            if (!central.listServers(servers))
            {
                MessageBoxW(window, L"Failed to list servers", L"Error", MB_OK);
                return;
            }
        }
        catch (std::runtime_error &err)
        {
            MessageBoxW(window, L"Failed to list servers", L"Error", MB_OK);
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
    else
    {
        MessageBoxW(window, L"An error occurred while connecting to the central server", L"Error", MB_OK);
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
                std::cout << "pre-connect" << std::endl;
                if (Chat::getInstance().connect(server))
                {
                    std::cout << "try start thread!" << std::endl;
                    std::thread(SocketThread).detach();
                    Central::getInstance().sendServerJoin(server.socket);
                }
                else
                {
                    std::cout << "Connect failed?" << std::endl;
                    MessageBoxW(window, L"Failed to connect to server", L"Connection failed", MB_OK);
                }
            }
        }
        else if (nmhdr->hwndFrom == inputBox)
        {
            std::cout << "txt changed" << std::endl;
            if (nmhdr->code == EN_MSGFILTER)
            {
                MSGFILTER *pMsgFilter = (MSGFILTER *)lParam;
                if (pMsgFilter->msg == WM_KEYDOWN && pMsgFilter->wParam == VK_RETURN)
                {
                    wchar_t in[1000];
                    GetWindowTextW(inputBox, in, 1000);
                    std::wstring wStr(in);
                    std::string str(wStr.begin(), wStr.end());
                    SendMessageString(str);

                    std::cout << "PRESSED ENTER" << std::endl;
                    SetWindowTextW(inputBox, L"");
                }
            }
        }
    }
    else if (msg == WM_SHOW_CHAT)
    {
        const Server *activeServer = Chat::getInstance().getActiveServer();
        if (activeServer == nullptr)
            return DefWindowProcW(hwnd, msg, wParam, lParam);

        std::wstring status = std::wstring(L"Status: Connected to ") + std::wstring(activeServer->ip.begin(), activeServer->ip.end());
        if (statusText != NULL)
        {
            SetWindowTextW(statusText, status.c_str());
        }

        if (availableServersLabel != NULL)
        {
            DestroyWindow(availableServersLabel);
            availableServersLabel = NULL;
        }

        if (serverListWindow != NULL)
        {
            DestroyWindow(serverListWindow);
            serverListWindow = 0;
        }

        chatBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"RICHEDIT50W", L"", WS_CHILD | WS_VISIBLE | ES_READONLY | ES_MULTILINE, 0, 25, 500, 415, window, NULL, GetModuleHandleW(NULL), 0);
        inputBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"RICHEDIT50W", L"", WS_CHILD | WS_VISIBLE, 0, 440, 500, 25, window, NULL, GetModuleHandleW(NULL), 0);
        SendMessage(inputBox, EM_SETEVENTMASK, 0, ENM_KEYEVENTS);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR lpCmdLine, int nCmdShow)
{
    std::cout << "XPChatter 1.0" << std::endl;

    // Load rich edit
    LoadLibraryA("Msftedit.dll");

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
    if (!(hwnd = CreateWindowW(newWindow.lpszClassName, L"XPChatter", WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 100, 100, 500, 500, NULL, NULL, hInstance, NULL)))
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