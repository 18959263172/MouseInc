class Window :
    public ATL::CWindowImpl<Window>
{
public:
    DECLARE_WND_CLASS(app_name)

    BEGIN_MSG_MAP(Window)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_COMMAND(OnCommand)

        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_USER, OnContextMenu)
        MESSAGE_HANDLER(WM_REPEAT_RUN, OnRepeatRun)
        MESSAGE_HANDLER(WM_TASKBAR_CREATED, OnTaskbarCreated)
        MESSAGE_HANDLER(WM_HOTKEY, OnHotKey)
        MESSAGE_HANDLER(WM_USER + 1, Delegation)
    END_MSG_MAP()

private:
    LRESULT OnCreate(LPCREATESTRUCT lpCreateStruct)
    {
        // 注册消息
        WM_REPEAT_RUN = ::RegisterWindowMessage(repeat_run);
        WM_TASKBAR_CREATED = ::RegisterWindowMessage(L"TaskbarCreated");

        // 初始化gdiplus
        GdiplusStartupInput in;
        GdiplusStartup(&gdiplus_token, &in, NULL);

        // 初始化手势
        gesture_window.Create(NULL, CRect(0, 0, 0, 0), app_name, WS_POPUP, WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
        gesture_mgr.Init();

        natural_scroll.Init();

        // 安装鼠标钩子
        mouse_hook = ::SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, ::GetModuleHandle(NULL), 0);

        // 安装键盘钩子（监控音量变化）
        keyboard_hook = ::SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, ::GetModuleHandle(NULL), 0);

        about_window.Create(m_hWnd, CRect(0, 0, 500, 348), i18n::GetString(L"about").c_str(), WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION);

        // 控制图标显示
        show_icon = false;
        if(!Config::GetInt(L"Other", L"HideTrayIcon"))
        {
            ShowTrayIcon();
        }

        main_window = m_hWnd;

        lua_engine->Init();
        return 0;
    }

    // 显示图标
    void ShowTrayIcon()
    {
        if(!show_icon)
        {
            memset(&nid, 0, sizeof(nid));
            nid.cbSize = sizeof(NOTIFYICONDATA);
            nid.uID = 100;
            nid.hWnd = m_hWnd;
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            nid.uCallbackMessage = WM_USER;

            if(IsWindows8OrGreater())
            {
                nid.hIcon = ::LoadIcon(::GetModuleHandle(NULL), L"TRAY_WHITE");
            }
            else
            {
                // win7使用黑色图标
                nid.hIcon = ::LoadIcon(::GetModuleHandle(NULL), L"TRAY_BLACK");
            }
            wcscpy_s(nid.szTip, _countof(nid.szTip), i18n::GetString(L"description").c_str());
            ::Shell_NotifyIcon(NIM_ADD, &nid);

            show_icon = true;
        }
    }

    // 隐藏图标
    void HideTrayIcon()
    {
        if(show_icon)
        {
            ::Shell_NotifyIcon(NIM_DELETE, &nid);
            show_icon = false;
        }
    }

    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        HideTrayIcon();

        ::UnhookWindowsHookEx(keyboard_hook);
        ::UnhookWindowsHookEx(mouse_hook);

        natural_scroll.Exit();

        gesture_window.DestroyWindow();

        GdiplusShutdown(gdiplus_token);

        ::PostQuitMessage(0);
        return 0;
    }

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        switch (lParam)
        {
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            ShowContextMenu(m_hWnd);
            break;
        }
        return 0;
    }

    void OnCommand(UINT uNotifyCode, int nID, CWindow wndCtl)
    {
        switch(nID)
        {
            case MI_HIDE_ICON:
                {
                    int hide = Config::GetInt(L"Other", L"HideTrayIcon");
                    hide = !hide;

                    if(hide)
                    {
                        HideTrayIcon();
                    }
                    else
                    {
                        ShowTrayIcon();
                    }

                    Config::SetInt(L"Other", L"HideTrayIcon", hide);
                }
                break;
            case MI_ABOUT:
                {
                    about_window.ShowWindow(SW_SHOW);
                }
                break;
            case MI_AUTO_RUN:
                switch_autorun(m_hWnd);
                break;
            case MI_EXIT:
                DestroyWindow();
                break;
            case MI_TOGGLE_ICON:
                if(show_icon)
                {
                    HideTrayIcon();
                }
                else
                {
                    ShowTrayIcon();
                }
                break;
            default:
                MenuDefaultHandle(m_hWnd, nID);
                break;
        }
    }

    LRESULT OnRepeatRun(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        ShowTrayIcon();
        return 0;
    }

    LRESULT OnTaskbarCreated(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (show_icon)
        {
            show_icon = false;
            ShowTrayIcon();
        }
        return 0;
    }

    LRESULT OnHotKey(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        lua_engine->Callback((int)wParam);
        return 0;
    }

    LRESULT Delegation(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        lua_engine->Do(current_aciton.c_str());
        return 0;
    }
private:
    ULONG_PTR gdiplus_token;

    UINT WM_REPEAT_RUN;
    UINT WM_TASKBAR_CREATED;

    NOTIFYICONDATA nid;
    bool show_icon;
    AboutWindow about_window;
};