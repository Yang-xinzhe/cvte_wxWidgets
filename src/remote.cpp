// 电视遥控器程序 - socket client
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;

// 遥控器主题色
namespace RemoteTheme {
    const wxColour Background = wxColour(25, 25, 30);
    const wxColour ButtonNormal = wxColour(50, 50, 55);
    const wxColour ButtonHover = wxColour(70, 70, 75);
    const wxColour ButtonPress = wxColour(30, 30, 35);
    const wxColour Primary = wxColour(0, 150, 200);
    const wxColour TextNormal = wxColour(220, 220, 220);
    const wxColour TextBright = wxColour(255, 255, 255);
}

// 圆形按钮控件
class RemoteButton : public wxPanel
{
public:
    RemoteButton(wxWindow* parent, wxWindowID id, const wxString& label, const wxSize& size = wxSize(80, 80))
        : wxPanel(parent, id, wxDefaultPosition, size, wxBORDER_NONE)
        , m_label(label)
        , m_hover(false)
        , m_pressed(false)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetMinSize(size);
        SetMaxSize(size);
        
        Bind(wxEVT_PAINT, &RemoteButton::OnPaint, this);
        Bind(wxEVT_LEFT_DOWN, &RemoteButton::OnMouseDown, this);
        Bind(wxEVT_LEFT_UP, &RemoteButton::OnMouseUp, this);
        Bind(wxEVT_ENTER_WINDOW, &RemoteButton::OnMouseEnter, this);
        Bind(wxEVT_LEAVE_WINDOW, &RemoteButton::OnMouseLeave, this);
    }

private:
    wxString m_label;
    bool m_hover;
    bool m_pressed;

    void OnPaint(wxPaintEvent& evt)
    {
        wxAutoBufferedPaintDC dc(this);
        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (!gc) return;

        wxSize size = GetClientSize();
        
        // 背景
        gc->SetBrush(wxBrush(RemoteTheme::Background));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRectangle(0, 0, size.x, size.y);
        
        // 按钮圆形
        double radius = std::min(size.x, size.y) / 2.0 - 5;
        double cx = size.x / 2.0;
        double cy = size.y / 2.0;
        
        wxColour btnColor;
        if (m_pressed) {
            btnColor = RemoteTheme::ButtonPress;
        } else if (m_hover) {
            btnColor = RemoteTheme::ButtonHover;
        } else {
            btnColor = RemoteTheme::ButtonNormal;
        }
        
        gc->SetBrush(wxBrush(btnColor));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawEllipse(cx - radius, cy - radius, radius * 2, radius * 2);
        
        // 文字
        wxFont font = GetFont().Bold().Scale(1.5);
        gc->SetFont(font, m_pressed ? RemoteTheme::Primary : RemoteTheme::TextNormal);
        double tw, th;
        gc->GetTextExtent(m_label, &tw, &th);
        gc->DrawText(m_label, cx - tw / 2, cy - th / 2);
        
        delete gc;
    }

    void OnMouseDown(wxMouseEvent& evt)
    {
        m_pressed = true;
        Refresh();
        
        // 发送命令
        wxCommandEvent event(wxEVT_BUTTON, GetId());
        event.SetEventObject(this);
        GetParent()->GetEventHandler()->ProcessEvent(event);
    }

    void OnMouseUp(wxMouseEvent& evt)
    {
        m_pressed = false;
        Refresh();
    }

    void OnMouseEnter(wxMouseEvent& evt)
    {
        m_hover = true;
        Refresh();
    }

    void OnMouseLeave(wxMouseEvent& evt)
    {
        m_hover = false;
        m_pressed = false;
        Refresh();
    }
};

// 遥控器主窗口
class RemoteFrame : public wxFrame
{
public:
    RemoteFrame()
        : wxFrame(nullptr, wxID_ANY, wxString::FromUTF8("电视遥控器"), 
                  wxDefaultPosition, wxSize(350, 550))
        , m_socket(INVALID_SOCKET)
        , m_connected(false)
    {
        SetBackgroundColour(RemoteTheme::Background);
        
        // 主布局
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
        
        // 状态栏
        m_statusText = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("未连接"));
        m_statusText->SetForegroundColour(wxColour(255, 100, 100));
        wxFont statusFont = m_statusText->GetFont().Scale(1.1);
        m_statusText->SetFont(statusFont);
        mainSizer->Add(m_statusText, 0, wxALL | wxALIGN_CENTER, 10);
        
        // 连接按钮
        wxButton* btnConnect = new wxButton(this, ID_CONNECT, wxString::FromUTF8("连接到 TV Menu"));
        btnConnect->SetBackgroundColour(RemoteTheme::Primary);
        btnConnect->SetForegroundColour(*wxWHITE);
        mainSizer->Add(btnConnect, 0, wxALL | wxEXPAND, 10);
        
        mainSizer->AddSpacer(20);
        
        // MENU 按钮
        RemoteButton* btnMenu = new RemoteButton(this, ID_MENU, "MENU", wxSize(120, 50));
        mainSizer->Add(btnMenu, 0, wxALL | wxALIGN_CENTER, 5);
        
        mainSizer->AddSpacer(20);
        
        // 方向键区域（十字布局）
        wxGridSizer* navSizer = new wxGridSizer(3, 3, 5, 5);
        
        // 第一行
        navSizer->AddSpacer(0);
        RemoteButton* btnUp = new RemoteButton(this, ID_UP, wxString::FromUTF8("▲"));
        navSizer->Add(btnUp, 0, wxALIGN_CENTER);
        navSizer->AddSpacer(0);
        
        // 第二行
        RemoteButton* btnLeft = new RemoteButton(this, ID_LEFT, wxString::FromUTF8("◀"));
        navSizer->Add(btnLeft, 0, wxALIGN_CENTER);
        
        RemoteButton* btnOK = new RemoteButton(this, ID_OK, "OK", wxSize(80, 80));
        navSizer->Add(btnOK, 0, wxALIGN_CENTER);
        
        RemoteButton* btnRight = new RemoteButton(this, ID_RIGHT, wxString::FromUTF8("▶"));
        navSizer->Add(btnRight, 0, wxALIGN_CENTER);
        
        // 第三行
        navSizer->AddSpacer(0);
        RemoteButton* btnDown = new RemoteButton(this, ID_DOWN, wxString::FromUTF8("▼"));
        navSizer->Add(btnDown, 0, wxALIGN_CENTER);
        navSizer->AddSpacer(0);
        
        mainSizer->Add(navSizer, 0, wxALL | wxALIGN_CENTER, 10);
        
        mainSizer->AddSpacer(20);
        
        // RETURN 按钮
        RemoteButton* btnReturn = new RemoteButton(this, ID_RETURN, wxString::FromUTF8("返回"), wxSize(120, 50));
        mainSizer->Add(btnReturn, 0, wxALL | wxALIGN_CENTER, 5);
        
        SetSizer(mainSizer);
        Centre();
        
        // 绑定事件
        Bind(wxEVT_BUTTON, &RemoteFrame::OnConnect, this, ID_CONNECT);
        Bind(wxEVT_BUTTON, &RemoteFrame::OnMenuButton, this, ID_MENU);
        Bind(wxEVT_BUTTON, &RemoteFrame::OnUpButton, this, ID_UP);
        Bind(wxEVT_BUTTON, &RemoteFrame::OnDownButton, this, ID_DOWN);
        Bind(wxEVT_BUTTON, &RemoteFrame::OnLeftButton, this, ID_LEFT);
        Bind(wxEVT_BUTTON, &RemoteFrame::OnRightButton, this, ID_RIGHT);
        Bind(wxEVT_BUTTON, &RemoteFrame::OnOKButton, this, ID_OK);
        Bind(wxEVT_BUTTON, &RemoteFrame::OnReturnButton, this, ID_RETURN);
        Bind(wxEVT_CLOSE_WINDOW, &RemoteFrame::OnClose, this);
        
        // 初始化 Winsock
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }
    
    ~RemoteFrame()
    {
        Disconnect();
#ifdef _WIN32
        WSACleanup();
#endif
    }

private:
    enum {
        ID_CONNECT = 1000,
        ID_MENU,
        ID_UP,
        ID_DOWN,
        ID_LEFT,
        ID_RIGHT,
        ID_OK,
        ID_RETURN
    };
    
    SOCKET m_socket;
    bool m_connected;
    wxStaticText* m_statusText;
    
    void OnConnect(wxCommandEvent& evt)
    {
        if (m_connected) {
            Disconnect();
        } else {
            Connect();
        }
    }
    
    void Connect()
    {
        // 创建 socket
        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket == INVALID_SOCKET) {
            wxMessageBox(wxString::FromUTF8("创建 socket 失败"), wxString::FromUTF8("错误"), wxOK | wxICON_ERROR);
            return;
        }
        
        // 目标地址
        sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(5050);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        
        // 设置为非阻塞模式
        u_long nonblock = 1;
        ioctlsocket(m_socket, FIONBIO, &nonblock);
        
        // 尝试连接（非阻塞，立即返回）
        int rc = connect(m_socket, (sockaddr*)&addr, sizeof(addr));
        if (rc == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS && err != WSAEINVAL) {
                closesocket(m_socket);
                m_socket = INVALID_SOCKET;
                wxMessageBox(
                    wxString::FromUTF8("连接失败！请确保 TV Menu 程序正在运行。"),
                    wxString::FromUTF8("连接错误"), 
                    wxOK | wxICON_ERROR
                );
                return;
            }
            
            // 用 select 等待连接完成（设置 3 秒超时）
            fd_set wset, eset;
            FD_ZERO(&wset);
            FD_ZERO(&eset);
            FD_SET(m_socket, &wset);
            FD_SET(m_socket, &eset);
            
            timeval tv;
            tv.tv_sec = 3;   // 超时 3 秒
            tv.tv_usec = 0;
            
            int sel = select(0, nullptr, &wset, &eset, &tv);
            if (sel <= 0 || FD_ISSET(m_socket, &eset)) {
                // 超时或错误
                closesocket(m_socket);
                m_socket = INVALID_SOCKET;
                wxMessageBox(
                    wxString::FromUTF8("连接超时或被拒绝！\n请确认 TV Menu 已运行且端口 5050 开放。"),
                    wxString::FromUTF8("连接失败"), 
                    wxOK | wxICON_ERROR
                );
                return;
            }
            
            // 检查 SO_ERROR 确保真正连接成功
            int so_error = 0;
            socklen_t len = sizeof(so_error);
            getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (char*)&so_error, &len);
            if (so_error != 0) {
                closesocket(m_socket);
                m_socket = INVALID_SOCKET;
                wxMessageBox(
                    wxString::FromUTF8("连接失败（SO_ERROR）！\n请确保 TV Menu 程序正在运行。"),
                    wxString::FromUTF8("连接错误"), 
                    wxOK | wxICON_ERROR
                );
                return;
            }
        }
        
        // 连接成功，设回阻塞模式（便于后续 send）
        u_long block = 0;
        ioctlsocket(m_socket, FIONBIO, &block);
        
        m_connected = true;
        m_statusText->SetLabel(wxString::FromUTF8("已连接"));
        m_statusText->SetForegroundColour(wxColour(100, 255, 100));
        
        wxButton* btn = dynamic_cast<wxButton*>(FindWindow(ID_CONNECT));
        if (btn) {
            btn->SetLabel(wxString::FromUTF8("断开连接"));
        }
        
        wxLogMessage(wxString::FromUTF8("已连接到 TV Menu"));
    }
    
    void Disconnect()
    {
        if (m_socket != INVALID_SOCKET) {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
        }
        
        m_connected = false;
        m_statusText->SetLabel(wxString::FromUTF8("未连接"));
        m_statusText->SetForegroundColour(wxColour(255, 100, 100));
        
        wxButton* btn = dynamic_cast<wxButton*>(FindWindow(ID_CONNECT));
        if (btn) {
            btn->SetLabel(wxString::FromUTF8("连接到 TV Menu"));
        }
    }
    
    void SendCommand(const std::string& cmd)
    {
        if (!m_connected || m_socket == INVALID_SOCKET) {
            wxMessageBox(wxString::FromUTF8("请先连接到 TV Menu"), wxString::FromUTF8("未连接"), wxOK | wxICON_WARNING);
            return;
        }
        
        int result = send(m_socket, cmd.c_str(), cmd.size(), 0);
        if (result == SOCKET_ERROR) {
            wxMessageBox(wxString::FromUTF8("发送命令失败，连接可能已断开"), wxString::FromUTF8("错误"), wxOK | wxICON_ERROR);
            Disconnect();
        } else {
            // wxLogMessage(wxString::Format(wxString::FromUTF8("发送命令: %s"), cmd.c_str()));
        }
    }
    
    void OnMenuButton(wxCommandEvent& evt) { SendCommand("KEY_MENU"); }
    void OnUpButton(wxCommandEvent& evt) { SendCommand("KEY_UP"); }
    void OnDownButton(wxCommandEvent& evt) { SendCommand("KEY_DOWN"); }
    void OnLeftButton(wxCommandEvent& evt) { SendCommand("KEY_LEFT"); }
    void OnRightButton(wxCommandEvent& evt) { SendCommand("KEY_RIGHT"); }
    void OnOKButton(wxCommandEvent& evt) { SendCommand("KEY_OK"); }
    void OnReturnButton(wxCommandEvent& evt) { SendCommand("KEY_RETURN"); }
    
    void OnClose(wxCloseEvent& evt)
    {
        Disconnect();
        evt.Skip();
    }
};

// 应用程序
class RemoteApp : public wxApp
{
public:
    virtual bool OnInit()
    {
        RemoteFrame* frame = new RemoteFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(RemoteApp);
