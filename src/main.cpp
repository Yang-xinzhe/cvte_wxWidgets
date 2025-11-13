#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <wx/timer.h>
#include <vector>
#include <map>
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;

namespace Theme {
    const wxColour Background = wxColour(3, 54, 75);       
    const wxColour CardNormal = wxColour(30, 30, 35);     
    const wxColour CardHover = wxColour(45, 45, 50);     
    const wxColour Primary1 = wxColour(0, 150, 200);      
    const wxColour Primary2 = wxColour(0, 100, 180);      
    const wxColour TextNormal = wxColour(200, 200, 200); 
    const wxColour TextSelected = wxColour(255, 255, 255);   
    const wxColour CheckMark = wxColour(0, 200, 100);        
}

enum class Language {
    English,
    Chinese,
    // Spanish,
    // Japanese,
    // etc.
};

class LanguageManager {
public:
    static LanguageManager& Instance() {
        static LanguageManager instance;
        return instance;
    }
    
    void SetLanguage(Language lang) {
        m_currentLanguage = lang;
    }
    
    Language GetLanguage() const {
        return m_currentLanguage;
    }
    
    wxString GetText(const wxString& key) const {
        auto it = m_translations.find(key);
        if (it != m_translations.end()) {
            switch (m_currentLanguage) {
                case Language::Chinese:
                    return it->second.chinese;
                case Language::English:
                default:
                    return it->second.english;
            }
        }
        return key;
    }
    
private:
    LanguageManager() : m_currentLanguage(Language::English) {
        InitializeTranslations();
    }
    
    struct Translation {
        wxString english;
        wxString chinese;
        // wxString spanish;
        // wxString japanese;
    };
    
    void InitializeTranslations() {
        // Tab 标签
        m_translations["tab_source"] = {"Source", wxString::FromUTF8("信号源")};
        m_translations["tab_picture"] = {"Picture", wxString::FromUTF8("图像")};
        m_translations["tab_sound"] = {"Sound", wxString::FromUTF8("声音")};
        m_translations["tab_channel"] = {"Channel", wxString::FromUTF8("频道")};
        m_translations["tab_common"] = {"Common", wxString::FromUTF8("通用")};
        
        // Source 页面
        m_translations["source_dtv"] = {"DTV", wxString::FromUTF8("数字电视")};
        m_translations["source_atv"] = {"ATV", wxString::FromUTF8("模拟电视")};
        m_translations["source_av"] = {"AV", wxString::FromUTF8("AV输入")};
        m_translations["source_hdmi1"] = {"HDMI1", "HDMI1"};
        m_translations["source_hdmi2"] = {"HDMI2", "HDMI2"};
        
        // Picture 页面
        m_translations["picture_standard"] = {"Standard", wxString::FromUTF8("标准")};
        m_translations["picture_dynamic"] = {"Dynamic", wxString::FromUTF8("动态")};
        m_translations["picture_movie"] = {"Movie", wxString::FromUTF8("电影")};
        m_translations["picture_game"] = {"Game", wxString::FromUTF8("游戏")};
        
        // Sound 页面
        m_translations["sound_standard"] = {"Standard", wxString::FromUTF8("标准")};
        m_translations["sound_music"] = {"Music", wxString::FromUTF8("音乐")};
        m_translations["sound_movie"] = {"Movie", wxString::FromUTF8("电影")};
        m_translations["sound_sports"] = {"Sports", wxString::FromUTF8("体育")};
        
        // Channel 页面
        m_translations["channel_auto"] = {"Auto Scan", wxString::FromUTF8("自动搜台")};
        m_translations["channel_manual"] = {"Manual Scan", wxString::FromUTF8("手动搜台")};
        m_translations["channel_list"] = {"Channel List", wxString::FromUTF8("频道列表")};
        
        // Common 页面
        m_translations["common_language_english"] = {"English", wxString::FromUTF8("英语")};
        m_translations["common_language_chinese"] = {"Chinese", wxString::FromUTF8("中文")};
        
        // 窗口标题
        m_translations["window_title"] = {"TV Menu Demo", wxString::FromUTF8("电视菜单演示")};
        m_translations["popup_switch_success"] = {"Switched to %s page.", wxString::FromUTF8("已切换到%s页面。")};
    }
    
    Language m_currentLanguage;
    std::map<wxString, Translation> m_translations;
};

#define TR(key) LanguageManager::Instance().GetText(key)

wxDECLARE_EVENT(wxEVT_TILE_CLICKED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_TILE_CLICKED, wxCommandEvent);

wxDECLARE_EVENT(wxEVT_TAB_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_TAB_CHANGED, wxCommandEvent);

wxDECLARE_EVENT(wxEVT_SOCKET_CMD, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_SOCKET_CMD, wxCommandEvent);


class TileButton : public wxPanel
{
public:
    TileButton(wxWindow* parent, wxWindowID id, const wxString& textKey)
        : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
        , m_textKey(textKey)
        , m_highlighted(false)
        , m_checked(false)
        , m_hover(false)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT); 
        // SetMinSize(wxSize(150, 60));
        // SetMaxSize(wxSize(150, 90));
        
        // 绑定事件
        Bind(wxEVT_PAINT, &TileButton::OnPaint, this);
        Bind(wxEVT_LEFT_DOWN, &TileButton::OnClick, this);
        Bind(wxEVT_ENTER_WINDOW, &TileButton::OnMouseEnter, this);
        Bind(wxEVT_LEAVE_WINDOW, &TileButton::OnMouseLeave, this);
    }
    
    void SetIconSvg(const wxBitmapBundle& bundle) {
        m_iconSvg = bundle;
        Refresh();
    }

    void SetHighlighted(bool highlighted) 
    { 
        if (m_highlighted != highlighted) {
            m_highlighted = highlighted;
            Refresh();
        }
    }

    void SetChecked(bool checked)
    {
        if (m_checked != checked) {
            m_checked = checked;
            Refresh();
        }
    }
    
    void UpdateLanguage() {
        Refresh();  // 重绘以显示新语言
    }
    
    bool IsHighlighted() const { return m_highlighted; }
    bool IsChecked() const { return m_checked; }
    wxString GetTextKey() const { return m_textKey; }

private:
    wxString m_textKey;  // 存储文本键而不是直接文本
    wxString m_icon;
    wxBitmapBundle m_iconSvg;
    bool m_highlighted;
    bool m_checked;
    bool m_hover;
    
    void OnPaint(wxPaintEvent& evt)
    {
        wxAutoBufferedPaintDC dc(this);
        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (!gc) return;
        
        wxSize size = GetClientSize();
        
        gc->SetBrush(wxBrush(Theme::Background));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRectangle(0, 0, size.x, size.y);
        
        double margin = 8;
        double x = margin;
        double y = margin;
        double w = size.x - 2 * margin;
        double h = size.y - 2 * margin;
        double radius = 8;
        
        if (m_highlighted) {
            wxGraphicsGradientStops stops(Theme::Primary1, Theme::Primary2);
            wxGraphicsBrush brush = gc->CreateLinearGradientBrush(
                x, y, x, y + h, stops);
            gc->SetBrush(brush);
        } else if (m_hover) {
            gc->SetBrush(wxBrush(Theme::CardHover));
        } else {
            gc->SetBrush(wxBrush(Theme::CardNormal));
        }
        
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRoundedRectangle(x, y, w, h, radius);
        
        if (m_checked) {
            double checkSize = 20;
            double checkX = x + w - checkSize - 8;
            double checkY = y + 8;
            
            gc->SetBrush(wxBrush(Theme::CheckMark));
            gc->DrawEllipse(checkX, checkY, checkSize, checkSize);
            
            gc->SetPen(wxPen(*wxWHITE, 2));
            wxGraphicsPath path = gc->CreatePath();
            path.MoveToPoint(checkX + 5, checkY + checkSize/2);
            path.AddLineToPoint(checkX + checkSize/2.5, checkY + checkSize - 6);
            path.AddLineToPoint(checkX + checkSize - 4, checkY + 4);
            gc->StrokePath(path);
        }

        const bool hasIcon = m_iconSvg.IsOk() || !m_icon.IsEmpty();
        double textScale = hasIcon ? 1.0 : 1.3;
        wxFont textFont = GetFont().Bold().Scale(textScale);
        wxString displayText = TR(m_textKey);
        gc->SetFont(textFont, m_highlighted ? Theme::TextSelected : Theme::TextNormal);
        double tw, th;
        gc->GetTextExtent(displayText, &tw, &th);

        int topPad     = hasIcon ? 8  : 12;
        int bottomPad  = hasIcon ? 10 : 15;
        int minSpacing = hasIcon ? 6  : 10;
        
        // 3. 计算文本位置（固定贴底）
        int textY = static_cast<int>(y + h - th - bottomPad);
        
        // 4. 计算图标可用空间和实际高度
        int iconTopY = static_cast<int>(y + topPad);
        int availableSpace = textY - iconTopY - minSpacing;  // 减去最小间距
        const double iconFraction = hasIcon ? 0.82 : 0.0;
        int desiredIconH = static_cast<int>(h * iconFraction);
        int maxIconH = wxMax(0, wxMin(desiredIconH, availableSpace));
        int minIconH = 18;
        int iconH = (maxIconH >= minIconH) ? maxIconH : wxMax(0, maxIconH);
        int iconW = iconH;
        int iconX = static_cast<int>(x + (w - iconW) / 2);
        
        // 5. 绘制图标（如果有足够空间）
        if (hasIcon && iconH > 0) {
            if (m_iconSvg.IsOk()) {
                wxBitmap bmp = m_iconSvg.GetBitmap(wxSize(iconW, iconH));
                if (bmp.IsOk()) gc->DrawBitmap(bmp, iconX, iconTopY, iconW, iconH);
            } else {
                wxFont iconFont = GetFont().Bold().Scale(2.0);
                gc->SetFont(iconFont, Theme::TextSelected);
                double itw, ith;
                gc->GetTextExtent(m_icon, &itw, &ith);
                gc->DrawText(m_icon, x + (w - itw) / 2, iconTopY);
            }
        }
        
        // 6. 绘制文本
        gc->SetFont(textFont, m_highlighted ? Theme::TextSelected : Theme::TextNormal);
        gc->DrawText(displayText, x + (w - tw) / 2, textY);
    }
    
    void OnClick(wxMouseEvent& evt)
    {
        wxCommandEvent event(wxEVT_TILE_CLICKED, GetId());
        event.SetEventObject(this);
        ProcessWindowEvent(event);
        evt.Skip();
    }
    
    void OnMouseEnter(wxMouseEvent& evt)
    {
        m_hover = true;
        Refresh();
        evt.Skip();
    }
    
    void OnMouseLeave(wxMouseEvent& evt)
    {
        m_hover = false;
        Refresh();
        evt.Skip();
    }
};

class TabBar : public wxPanel
{
public:
    TabBar(wxWindow* parent) 
        : wxPanel(parent, wxID_ANY)
        , m_selectedIndex(0)
    {
        SetBackgroundColour(Theme::Background);
        
        wxGridSizer* sizer = new wxGridSizer(1, 5, 0, 10);

        // 存储标签键
        m_tabKeys = {"tab_source", "tab_picture", "tab_sound", "tab_channel", "tab_common"};
        
        for (int i = 0; i < 5; i++) {
            wxButton* btn = new wxButton(this, 1000 + i, TR(m_tabKeys[i]), 
                wxDefaultPosition, wxSize(120, 40));
            btn->SetBackgroundColour(Theme::Background);
            btn->SetForegroundColour(Theme::TextNormal);
            btn->SetFont(GetFont().Bold().Scale(1.2));
            
            btn->Bind(wxEVT_BUTTON, [this, i](wxCommandEvent& evt) {
                SelectTab(i);
            });
            
            sizer->Add(btn, 0, wxALL, 5);
            m_tabs.push_back(btn);
        }
        
        SetSizer(sizer);
        UpdateTabStyles();
    }
    
    void SelectTab(int index, bool fireEvent = true)
    {
        if (index >= 0 && index < (int)m_tabs.size() && index != m_selectedIndex) {
            m_selectedIndex = index;
            UpdateTabStyles();
            
            if (fireEvent) {
                wxCommandEvent evt(wxEVT_TAB_CHANGED, GetId());
                evt.SetInt(index);
                ProcessWindowEvent(evt);
            }
        } else if (index >= 0 && index < (int)m_tabs.size() && !fireEvent) {
            // 即使索引未变，当仅更新样式时也确保刷新
            UpdateTabStyles();
        }
    }
    
    void UpdateLanguage() {
        // 更新所有标签的文本
        for (size_t i = 0; i < m_tabs.size(); i++) {
            m_tabs[i]->SetLabel(TR(m_tabKeys[i]));
        }
        Layout();
    }
    
    int GetSelectedIndex() const { return m_selectedIndex; }
    
    int GetTabCount() const { return static_cast<int>(m_tabs.size()); }
    
    wxString GetTabKey(int index) const
    {
        if (index >= 0 && index < static_cast<int>(m_tabKeys.size())) {
            return m_tabKeys[index];
        }
        return wxString();
    }

private:
    std::vector<wxButton*> m_tabs;
    std::vector<wxString> m_tabKeys;  // 存储文本键
    int m_selectedIndex;
    
    void UpdateTabStyles()
    {
        for (size_t i = 0; i < m_tabs.size(); i++) {
            if ((int)i == m_selectedIndex) {
                m_tabs[i]->SetForegroundColour(Theme::TextSelected);
                m_tabs[i]->SetBackgroundColour(Theme::Primary1);
            } else {
                m_tabs[i]->SetForegroundColour(Theme::TextNormal);
                m_tabs[i]->SetBackgroundColour(Theme::Background);
            }
            m_tabs[i]->Refresh();
        }
    }
};


class ContentPage : public wxPanel
{
public:
    ContentPage(wxWindow* parent, const std::vector<wxString>& itemKeys, const std::vector<wxString>& iconSvgPaths = {}, const wxSize& tileSize = wxSize(150, 60))
        : wxPanel(parent, wxID_ANY)
    {
        SetBackgroundColour(Theme::Background);
        
        // 使用 GridSizer，1行N列，和 TabBar 一样的对齐方式
        int numItems = itemKeys.size();
        wxGridSizer* sizer = new wxGridSizer(1, numItems, 0, 10);
        
        for (size_t i = 0; i < itemKeys.size(); i++) {
            TileButton* tile = new TileButton(this, 2000 + i, itemKeys[i]);
            
            tile->SetMinSize(tileSize);
            tile->SetMaxSize(tileSize);
            tile->SetInitialSize(tileSize);

            if(!iconSvgPaths.empty() && i < iconSvgPaths.size()) {
                const wxString& svg = iconSvgPaths[i];
                if(!svg.IsEmpty()) {
                    tile->SetIconSvg(wxBitmapBundle::FromSVGFile(svg, wxSize(1024, 1024)));
                }
            }

            tile->Bind(wxEVT_TILE_CLICKED, [this, tile](wxCommandEvent& evt) {
                OnTileClicked(tile);
                evt.Skip();  // 让事件继续传播到 MyFrame
            });
            
            sizer->Add(tile, 0, wxALL, 5);
            m_tiles.push_back(tile);
        }
        
        SetSizer(sizer);
        
    }
    
    void UpdateLanguage() {
        // 更新所有 TileButton 的语言
        for (auto tile : m_tiles) {
            tile->UpdateLanguage();
        }
    }
    
    const std::vector<TileButton*>& GetTiles() const { return m_tiles; }

private:
    std::vector<TileButton*> m_tiles;
    
    void OnTileClicked(TileButton* clicked)
    {
        for (auto tile : m_tiles) {
            tile->SetHighlighted(tile == clicked);
        }
    }
};

// Socket Server 线程 - 接收遥控器命令
class RemoteServerThread : public wxThread
{
public:
    RemoteServerThread(wxEvtHandler* handler) 
        : wxThread(wxTHREAD_DETACHED)
        , m_handler(handler)
        , m_serverSocket(INVALID_SOCKET)
    {
    }

protected:
    virtual ExitCode Entry() override
    {
        // 初始化 Winsock (Windows)
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            return (ExitCode)0;
        }
#endif

        // 创建 socket
        m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_serverSocket == INVALID_SOCKET) {
#ifdef _WIN32
            WSACleanup();
#endif
            return (ExitCode)0;
        }

        // 设置地址重用
        int opt = 1;
        setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        // 绑定端口
        sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(5050);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(m_serverSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            closesocket(m_serverSocket);
#ifdef _WIN32
            WSACleanup();
#endif
            return (ExitCode)0;
        }

        // 监听
        if (listen(m_serverSocket, 1) == SOCKET_ERROR) {
            closesocket(m_serverSocket);
#ifdef _WIN32
            WSACleanup();
#endif
            return (ExitCode)0;
        }

        // wxLogMessage(wxString::FromUTF8("遥控器服务已启动，监听端口 5050..."));

        // 主循环：接受连接并处理
        while (!TestDestroy()) {
            // 使用 select 等待连接（带超时，避免阻塞太久）
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(m_serverSocket, &readfds);
            
            timeval timeout;
            timeout.tv_sec = 1;  // 1秒超时，定期检查 TestDestroy()
            timeout.tv_usec = 0;
            
            int sel = select(0, &readfds, nullptr, nullptr, &timeout);
            
            // 超时或出错，继续循环
            if (sel <= 0) {
                continue;
            }
            
            // 有新连接
            sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            SOCKET clientSocket = accept(m_serverSocket, (sockaddr*)&clientAddr, &clientLen);
            
            if (clientSocket == INVALID_SOCKET) {
                continue;  // 接受失败，继续等待下一个
            }

            // wxLogMessage(wxString::FromUTF8("遥控器已连接！"));

            // 接收命令循环
            char buffer[256];
            while (!TestDestroy()) {
                // 同样使用 select 检查是否有数据可读（带超时）
                FD_ZERO(&readfds);
                FD_SET(clientSocket, &readfds);
                
                timeout.tv_sec = 1;  // 1秒超时
                timeout.tv_usec = 0;
                
                sel = select(0, &readfds, nullptr, nullptr, &timeout);
                
                if (sel < 0) {
                    // 出错，断开
                    wxLogMessage(wxString::FromUTF8("Socket 错误，断开连接"));
                    break;
                }
                
                if (sel == 0) {
                    // 超时，继续循环（这样可以检查 TestDestroy()）
                    continue;
                }
                
                // 有数据可读
                int n = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                if (n <= 0) {
                    // 连接关闭或出错
                    if (n == 0) {
                        wxLogMessage(wxString::FromUTF8("遥控器正常断开连接"));
                    } else {
                        wxLogMessage(wxString::FromUTF8("遥控器异常断开连接"));
                    }
                    break;
                }
                
                buffer[n] = 0;
                wxString cmd(buffer, wxConvUTF8);
                cmd.Trim();

                // 发送事件到主线程
                wxCommandEvent* event = new wxCommandEvent(wxEVT_SOCKET_CMD, wxID_ANY);
                event->SetString(cmd);
                wxQueueEvent(m_handler, event);
            }

            closesocket(clientSocket);
            wxLogMessage(wxString::FromUTF8("等待新的遥控器连接..."));
        }

        // 清理
        if (m_serverSocket != INVALID_SOCKET) {
            closesocket(m_serverSocket);
        }
#ifdef _WIN32
        WSACleanup();
#endif
        return (ExitCode)0;
    }

private:
    wxEvtHandler* m_handler;
    SOCKET m_serverSocket;
};

class MyFrame : public wxFrame
{
public:
    MyFrame() 
        : wxFrame(nullptr, wxID_ANY, TR("window_title"), 
                  wxDefaultPosition, wxSize(750, 205))
        , m_menuVisible(true)
        , m_currentPageIndex(0)
        , m_pendingTabIndex(0)
        , m_currentTileIndex(0)
        , m_inTabSelectionMode(true)
    {
        SetBackgroundColour(Theme::Background);
        
        // 主布局
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
        
        // 1. 顶部 TabBar
        m_tabBar = new TabBar(this);
        mainSizer->Add(m_tabBar, 0, wxEXPAND | wxALL, 10);
        
        // 2. 内容区域
        m_contentPanel = new wxPanel(this, wxID_ANY);
        m_contentPanel->SetBackgroundColour(Theme::Background);
        m_contentSizer = new wxBoxSizer(wxVERTICAL);
        m_contentPanel->SetSizer(m_contentSizer);
        mainSizer->Add(m_contentPanel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
        
        // 创建各个页面
        CreatePages();
        
        // 显示第一个页面
        ShowPage(0, false);
        
        // 绑定 Tab 切换事件
        m_tabBar->Bind(wxEVT_TAB_CHANGED, &MyFrame::OnTabChanged, this);
        
        // 绑定所有页面的 Tile 点击事件
        BindTileClickEvents();
        
        // 绑定键盘事件
        Bind(wxEVT_CHAR_HOOK, &MyFrame::OnKeyDown, this);
        
        // 绑定 socket 命令事件
        Bind(wxEVT_SOCKET_CMD, &MyFrame::OnSocketCommand, this);
        
        // 启动 socket server 线程
        m_serverThread = new RemoteServerThread(this);
        if (m_serverThread->Run() != wxTHREAD_NO_ERROR) {
            wxLogError(wxString::FromUTF8("无法启动遥控器服务线程"));
            delete m_serverThread;
            m_serverThread = nullptr;
        }
        
        SetSizer(mainSizer);
        Centre();
    }
    
    ~MyFrame()
    {
        // 停止 socket server 线程
        if (m_serverThread) {
            m_serverThread->Delete();
        }
    }

private:
    TabBar* m_tabBar;
    wxPanel* m_contentPanel;
    wxBoxSizer* m_contentSizer;
    std::vector<ContentPage*> m_pages;
    
    bool m_menuVisible;
    int m_currentPageIndex;
    int m_pendingTabIndex;
    int m_currentTileIndex;
    bool m_inTabSelectionMode;
    
    RemoteServerThread* m_serverThread;
    
    void CreatePages()
    {
        // Source 页 - 使用文本键
        std::vector<wxString> sourceKeys = {
            "source_dtv", "source_atv", "source_av", "source_hdmi1", "source_hdmi2"
        };

        std::vector<wxString> sourceIcons = {
            "C:\\wxwidgets-vscode\\icon\\tv.svg",
            "C:\\wxwidgets-vscode\\icon\\tv.svg",
            "C:\\wxwidgets-vscode\\icon\\channel.svg",
            "C:\\wxwidgets-vscode\\icon\\hdmi.svg",
            "C:\\wxwidgets-vscode\\icon\\hdmi.svg"
        };

        m_pages.push_back(new ContentPage(m_contentPanel, sourceKeys, sourceIcons, wxSize(150, 90)));
        
        // Picture 页
        std::vector<wxString> pictureKeys = {
            "picture_standard", "picture_dynamic", "picture_movie", "picture_game"
        };
        m_pages.push_back(new ContentPage(m_contentPanel, pictureKeys));
        
        // Sound 页
        std::vector<wxString> soundKeys = {
            "sound_standard", "sound_music", "sound_movie", "sound_sports"
        };
        m_pages.push_back(new ContentPage(m_contentPanel, soundKeys));
        
        // Channel 页
        std::vector<wxString> channelKeys = {
            "channel_auto", "channel_manual", "channel_list"
        };
        m_pages.push_back(new ContentPage(m_contentPanel, channelKeys));
        
        // Common 页
        std::vector<wxString> commonKeys = {
            "common_language_english", "common_language_chinese"
        };
        m_pages.push_back(new ContentPage(m_contentPanel, commonKeys));
        
        // 初始隐藏所有页面
        for (auto page : m_pages) {
            page->Hide();
        }
    }
    
    void BindTileClickEvents() {
        // 为所有页面的所有 TileButton 绑定点击事件到主窗口
        for (auto page : m_pages) {
            const auto& tiles = page->GetTiles();
            for (auto tile : tiles) {
                tile->Bind(wxEVT_TILE_CLICKED, &MyFrame::OnTileButtonClicked, this);
            }
        }
    }
    
    void OnTileButtonClicked(wxCommandEvent& evt) {
        // 获取被点击的按钮
        TileButton* clickedTile = dynamic_cast<TileButton*>(evt.GetEventObject());
        if (!clickedTile) return;
        
        bool found = false;
        bool toggledOn = false;
        for (size_t pageIdx = 0; pageIdx < m_pages.size() && !found; ++pageIdx) {
            const auto& tiles = m_pages[pageIdx]->GetTiles();
            for (size_t tileIdx = 0; tileIdx < tiles.size(); ++tileIdx) {
                if (tiles[tileIdx] == clickedTile) {
                    m_currentPageIndex = static_cast<int>(pageIdx);
                    m_currentTileIndex = static_cast<int>(tileIdx);
                    m_pendingTabIndex = m_currentPageIndex;
                    m_inTabSelectionMode = false;
                    UpdateTileSelection();
                    
                    toggledOn = ToggleTileChecked(m_currentPageIndex, m_currentTileIndex);
                    found = true;
                    break;
                }
            }
        }
        
        // 检查是否是 Language 按钮
        if (toggledOn) {
            if (clickedTile->GetTextKey() == "common_language_english") {
                LanguageManager::Instance().SetLanguage(Language::English);
                UpdateAllLanguages();
            } else if (clickedTile->GetTextKey() == "common_language_chinese") {
                LanguageManager::Instance().SetLanguage(Language::Chinese);
                UpdateAllLanguages();
            }
        }
        
        // 其他按钮的处理可以在这里添加
    }
    
    void UpdateAllLanguages() {
        // 更新窗口标题
        SetTitle(TR("window_title"));
        
        // 更新 TabBar
        m_tabBar->UpdateLanguage();
        
        // 更新所有页面
        for (auto page : m_pages) {
            page->UpdateLanguage();
        }
        
        Layout();
        Refresh();
    }
    
    void ShowPage(int index, bool showPopup)
    {
        if (index < 0 || index >= (int)m_pages.size())
            return;
        
        m_currentPageIndex = index;
        m_currentTileIndex = 0;
        
        // 隐藏所有页面
        for (auto page : m_pages) {
            page->Hide();
        }
        
        // 显示选中的页面
        m_contentSizer->Clear();
        m_contentSizer->Add(m_pages[index], 1, wxEXPAND);
        m_pages[index]->Show();
        
        m_contentPanel->Layout();
        
        UpdateTileSelection();
        
        if (showPopup) {
            ShowTabSwitchPopup(index);
        }
    }
    
    void UpdateTileSelection()
    {
        if (m_currentPageIndex < 0 || m_currentPageIndex >= (int)m_pages.size())
            return;
        
        const auto& tiles = m_pages[m_currentPageIndex]->GetTiles();
        if (tiles.empty())
            return;
        
        if (m_currentTileIndex < 0) {
            m_currentTileIndex = 0;
        } else if (m_currentTileIndex >= static_cast<int>(tiles.size())) {
            m_currentTileIndex = static_cast<int>(tiles.size()) - 1;
        }
        
        bool highlightTiles = !m_inTabSelectionMode;
        for (size_t i = 0; i < tiles.size(); ++i) {
            bool shouldSelect = highlightTiles && static_cast<int>(i) == m_currentTileIndex;
            tiles[i]->SetHighlighted(shouldSelect);
        }
    }

    bool ToggleTileChecked(int pageIndex, int tileIndex)
    {
        if (pageIndex < 0 || pageIndex >= static_cast<int>(m_pages.size()))
            return false;

        const auto& tiles = m_pages[pageIndex]->GetTiles();
        if (tileIndex < 0 || tileIndex >= static_cast<int>(tiles.size()))
            return false;

        TileButton* target = tiles[tileIndex];
        if (!target)
            return false;

        if (target->IsChecked()) {
            target->SetChecked(false);
            return false;
        }

        for (size_t i = 0; i < tiles.size(); ++i) {
            tiles[i]->SetChecked(static_cast<int>(i) == tileIndex);
        }
        return true;
    }
    
    void ShowTabSwitchPopup(int index)
    {
        wxString tabKey = m_tabBar->GetTabKey(index);
        wxString tabLabel = tabKey.IsEmpty() ? wxString::Format("%d", index + 1) : TR(tabKey);
        wxString message = wxString::Format(TR("popup_switch_success"), tabLabel);
        
        // 创建自动关闭的对话框
        wxDialog* popup = new wxDialog(this, wxID_ANY, TR("window_title"), 
                                       wxDefaultPosition, wxSize(300, 100),
                                       wxCAPTION | wxSTAY_ON_TOP);
        popup->SetBackgroundColour(Theme::Background);
        
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        
        wxStaticText* text = new wxStaticText(popup, wxID_ANY, message);
        text->SetForegroundColour(Theme::TextSelected);
        text->SetFont(popup->GetFont().Bold().Scale(1.2));
        
        sizer->Add(text, 1, wxALL | wxALIGN_CENTER, 20);
        popup->SetSizer(sizer);
        popup->Centre();
        
        // 创建定时器，3秒后关闭
        wxTimer* timer = new wxTimer(popup, wxID_ANY);
        popup->Bind(wxEVT_TIMER, [popup](wxTimerEvent&) {
            popup->Close();
        });
        timer->StartOnce(3000); // 3000 毫秒 = 3 秒
        
        popup->Show();
    }
    
    // 键盘事件处理
    void OnKeyDown(wxKeyEvent& event)
    {
        int keyCode = event.GetKeyCode();
        
        switch(keyCode)
        {
            case 'M':
            case WXK_ESCAPE:
                ToggleMenu();
                break;
                
            case WXK_LEFT:
                if (m_menuVisible) {
                    HandleHorizontalNavigation(-1);
                }
                break;
                
            case WXK_RIGHT:
                if (m_menuVisible) {
                    HandleHorizontalNavigation(1);
                }
                break;
                
            case WXK_UP:
                if (m_menuVisible) {
                    HandleHorizontalNavigation(-1);
                }
                break;
                
            case WXK_DOWN:
                if (m_menuVisible) {
                    HandleHorizontalNavigation(1);
                }
                break;
                
            case WXK_RETURN:
            case WXK_NUMPAD_ENTER:
                if (m_menuVisible) {
                    OnConfirmKey();
                }
                break;
                
            case WXK_BACK:
                if (m_menuVisible) {
                    OnBackKey();
                }
                break;
                
            default:
                event.Skip();
                break;
        }
    }
    
    void HandleHorizontalNavigation(int direction)
    {
        if (direction == 0)
            return;
        
        if (m_inTabSelectionMode) {
            NavigateTab(direction);
        } else {
            NavigateTile(direction);
        }
    }
    
    // 切换菜单显示/隐藏
    void ToggleMenu()
    {
        m_menuVisible = !m_menuVisible;
        
        if (m_menuVisible) {
            m_inTabSelectionMode = true;
            m_pendingTabIndex = m_currentPageIndex;
            m_tabBar->SelectTab(m_pendingTabIndex, false);
            UpdateTileSelection();
            Show(true);
            Raise();
        } else {
            Show(false);
        }
    }
    
    void EnterContentMode()
    {
        if (m_currentPageIndex < 0 || m_currentPageIndex >= (int)m_pages.size())
            return;
        
        const auto& tiles = m_pages[m_currentPageIndex]->GetTiles();
        if (tiles.empty())
            return;
        
        m_currentTileIndex = 0;
        
        m_inTabSelectionMode = false;
        UpdateTileSelection();
    }
    
    void ActivateCurrentTile()
    {
        if (m_inTabSelectionMode)
            return;
        
        if (m_currentPageIndex < 0 || m_currentPageIndex >= (int)m_pages.size())
            return;
        
        const auto& tiles = m_pages[m_currentPageIndex]->GetTiles();
        if (tiles.empty())
            return;
        
        if (m_currentTileIndex < 0 || m_currentTileIndex >= (int)tiles.size())
            return;
        
        TileButton* selectedTile = tiles[m_currentTileIndex];
        wxCommandEvent clickEvent(wxEVT_TILE_CLICKED, selectedTile->GetId());
        clickEvent.SetEventObject(selectedTile);
        selectedTile->GetEventHandler()->ProcessEvent(clickEvent);
    }
    
    // 在 Tile 之间导航
    void NavigateTile(int direction)
    {
        if (m_inTabSelectionMode)
            return;
        
        if (m_currentPageIndex < 0 || m_currentPageIndex >= (int)m_pages.size())
            return;
        
        const auto& tiles = m_pages[m_currentPageIndex]->GetTiles();
        if (tiles.empty())
            return;
        
        m_currentTileIndex += direction;
        if (m_currentTileIndex < 0) {
            m_currentTileIndex = static_cast<int>(tiles.size()) - 1;
        } else if (m_currentTileIndex >= static_cast<int>(tiles.size())) {
            m_currentTileIndex = 0;
        }
        
        UpdateTileSelection();
    }
    
    // 在 Tab 之间导航
    void NavigateTab(int direction)
    {
        if (m_pages.empty())
            return;
        
        int tabCount = m_tabBar->GetTabCount();
        if (tabCount == 0)
            return;
        
        int newIndex = m_pendingTabIndex + direction;
        
        if (newIndex < 0) {
            newIndex = tabCount - 1;
        } else if (newIndex >= tabCount) {
            newIndex = 0;
        }
        
        if (newIndex != m_pendingTabIndex) {
            m_pendingTabIndex = newIndex;
            m_tabBar->SelectTab(newIndex, false);
        }
    }
    
    // 确认键处理
    void OnConfirmKey()
    {
        if (m_inTabSelectionMode) {
            if (m_pendingTabIndex != m_currentPageIndex) {
                ShowPage(m_pendingTabIndex, true);
            }
            m_currentPageIndex = m_pendingTabIndex;
            m_tabBar->SelectTab(m_currentPageIndex, false);
            m_pendingTabIndex = m_currentPageIndex;
            EnterContentMode();
        } else {
            ActivateCurrentTile();
        }
    }
    
    // 返回键处理
    void OnBackKey()
    {
        if (!m_inTabSelectionMode) {
            m_inTabSelectionMode = true;
            m_pendingTabIndex = m_currentPageIndex;
            m_tabBar->SelectTab(m_pendingTabIndex, false);
            UpdateTileSelection();
            return;
        } else {
            if (m_pendingTabIndex != 0) {
                m_pendingTabIndex = 0;
                m_tabBar->SelectTab(m_pendingTabIndex, false);
                return;
            }
            ToggleMenu();
        }
    }
    
    void OnTabChanged(wxCommandEvent& evt)
    {
        int index = evt.GetInt();
        m_inTabSelectionMode = true;
        m_pendingTabIndex = index;
        ShowPage(index, true);
        m_pendingTabIndex = m_currentPageIndex;
    }
    
    // Socket 命令处理
    void OnSocketCommand(wxCommandEvent& event)
    {
        wxString cmd = event.GetString();
        // wxLogMessage(wxString::Format(wxString::FromUTF8("收到遥控器命令: %s"), cmd));
        
        if (cmd == "KEY_MENU") {
            ToggleMenu();
        }
        else if (cmd == "KEY_LEFT") {
            if (m_menuVisible) {
                HandleHorizontalNavigation(-1);
            }
        }
        else if (cmd == "KEY_RIGHT") {
            if (m_menuVisible) {
                HandleHorizontalNavigation(1);
            }
        }
        else if (cmd == "KEY_UP") {
            if (m_menuVisible) {
                HandleHorizontalNavigation(-1);
            }
        }
        else if (cmd == "KEY_DOWN") {
            if (m_menuVisible) {
                HandleHorizontalNavigation(1);
            }
        }
        else if (cmd == "KEY_OK") {
            if (m_menuVisible) {
                OnConfirmKey();
            }
        }
        else if (cmd == "KEY_RETURN" || cmd == "KEY_BACK") {
            if (m_menuVisible) {
                OnBackKey();
            }
        }
    }
};

// ============================================
// 7. MyApp - 应用程序
// ============================================
class MyApp : public wxApp
{
public:
    virtual bool OnInit()
    {
        MyFrame* frame = new MyFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);
