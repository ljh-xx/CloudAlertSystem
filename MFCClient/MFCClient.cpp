// MFCClient.cpp - graphical client for Cloud Alert System.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <afxwin.h>
#include <afxcmn.h>
#include <afxext.h>

#include "TcpClient.h"
#ifdef STATUS_PENDING
#undef STATUS_PENDING
#endif
#include "Protocol.h"

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 8888

enum ControlId {
    IDC_EDIT_USER = 1001,
    IDC_EDIT_EMAIL,
    IDC_BTN_LOGIN,
    IDC_BTN_SET_EMAIL,
    IDC_LIST_ALERTS,
    IDC_EDIT_ID,
    IDC_EDIT_CONTRACT,
    IDC_EDIT_PRICE,
    IDC_COMBO_COND,
    IDC_EDIT_TIME,
    IDC_BTN_ADD_PRICE,
    IDC_BTN_ADD_TIME,
    IDC_BTN_MOD_PRICE,
    IDC_BTN_MOD_TIME,
    IDC_BTN_DELETE,
    IDC_COMBO_STATUS,
    IDC_BTN_REFRESH,
    IDC_STATUS_TEXT
};

static const UINT WM_APP_TRIGGERED = WM_APP + 1;
static const UINT WM_APP_DISCONNECTED = WM_APP + 2;

static std::vector<std::string> Split(const std::string& text) {
    std::istringstream iss(text);
    std::vector<std::string> out;
    std::string tok;
    while (iss >> tok) out.push_back(tok);
    return out;
}

class CMainFrame : public CFrameWnd {
public:
    CMainFrame() : m_connected(false), m_closing(false), m_recvThread(nullptr) {
        Create(nullptr, "Cloud Alert MFC Client",
               WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
               CRect(100, 80, 1100, 720));
    }

    ~CMainFrame() override {
        StopConnection();
    }

protected:
    CFont m_titleFont;
    CStatic m_title;
    CStatic m_userLabel;
    CStatic m_emailLabel;
    CEdit m_userEdit;
    CEdit m_emailEdit;
    CButton m_loginBtn;
    CButton m_setEmailBtn;
    CListCtrl m_list;
    CStatic m_panelTitle;
    CStatic m_idLabel;
    CStatic m_contractLabel;
    CStatic m_priceLabel;
    CStatic m_condLabel;
    CStatic m_timeLabel;
    CEdit m_idEdit;
    CEdit m_contractEdit;
    CEdit m_priceEdit;
    CComboBox m_condCombo;
    CEdit m_timeEdit;
    CButton m_addPriceBtn;
    CButton m_addTimeBtn;
    CButton m_modPriceBtn;
    CButton m_modTimeBtn;
    CButton m_deleteBtn;
    CComboBox m_statusCombo;
    CButton m_refreshBtn;
    CStatic m_statusText;

    TcpClient m_client;
    bool m_connected;
    bool m_closing;
    CString m_username;
    HANDLE m_recvThread;

    std::queue<std::string> m_responseQ;
    std::mutex m_responseMtx;
    std::condition_variable m_responseCV;

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct) {
        if (CFrameWnd::OnCreate(lpCreateStruct) == -1) return -1;

        m_titleFont.CreatePointFont(180, "Segoe UI");
        m_title.Create("Cloud Alert System", WS_CHILD | WS_VISIBLE, CRect(), this);
        m_title.SetFont(&m_titleFont);

        m_userLabel.Create("Username", WS_CHILD | WS_VISIBLE, CRect(), this);
        m_emailLabel.Create("Email", WS_CHILD | WS_VISIBLE, CRect(), this);
        m_userEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, IDC_EDIT_USER);
        m_emailEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, IDC_EDIT_EMAIL);
        m_emailEdit.SetWindowText("demo@example.com");
        m_loginBtn.Create("Connect / Login", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, IDC_BTN_LOGIN);
        m_setEmailBtn.Create("Set Email", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, IDC_BTN_SET_EMAIL);

        m_list.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
                      CRect(), this, IDC_LIST_ALERTS);
        m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
        m_list.InsertColumn(0, "ID", LVCFMT_LEFT, 55);
        m_list.InsertColumn(1, "Type", LVCFMT_LEFT, 70);
        m_list.InsertColumn(2, "Contract", LVCFMT_LEFT, 95);
        m_list.InsertColumn(3, "Price", LVCFMT_RIGHT, 90);
        m_list.InsertColumn(4, "Cond", LVCFMT_LEFT, 70);
        m_list.InsertColumn(5, "Time", LVCFMT_LEFT, 90);
        m_list.InsertColumn(6, "Status", LVCFMT_LEFT, 90);
        m_list.InsertColumn(7, "Created", LVCFMT_LEFT, 155);

        m_panelTitle.Create("Alert Editor", WS_CHILD | WS_VISIBLE, CRect(), this);
        m_idLabel.Create("Alert ID", WS_CHILD | WS_VISIBLE, CRect(), this);
        m_contractLabel.Create("Contract", WS_CHILD | WS_VISIBLE, CRect(), this);
        m_priceLabel.Create("Price", WS_CHILD | WS_VISIBLE, CRect(), this);
        m_condLabel.Create("Condition", WS_CHILD | WS_VISIBLE, CRect(), this);
        m_timeLabel.Create("Time", WS_CHILD | WS_VISIBLE, CRect(), this);
        m_idEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, IDC_EDIT_ID);
        m_contractEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, IDC_EDIT_CONTRACT);
        m_priceEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, IDC_EDIT_PRICE);
        m_timeEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, IDC_EDIT_TIME);
        m_contractEdit.SetWindowText("rb2609");
        m_timeEdit.SetWindowText("09:30:00");

        m_condCombo.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST, CRect(), this, IDC_COMBO_COND);
        m_condCombo.AddString(">= target");
        m_condCombo.AddString("<= target");
        m_condCombo.SetCurSel(0);

        m_addPriceBtn.Create("Add Price", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, IDC_BTN_ADD_PRICE);
        m_addTimeBtn.Create("Add Time", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, IDC_BTN_ADD_TIME);
        m_modPriceBtn.Create("Modify Price", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, IDC_BTN_MOD_PRICE);
        m_modTimeBtn.Create("Modify Time", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, IDC_BTN_MOD_TIME);
        m_deleteBtn.Create("Delete", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, IDC_BTN_DELETE);

        m_statusCombo.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST, CRect(), this, IDC_COMBO_STATUS);
        m_statusCombo.AddString("All");
        m_statusCombo.AddString("Pending");
        m_statusCombo.AddString("Triggered");
        m_statusCombo.AddString("Deleted");
        m_statusCombo.SetCurSel(0);
        m_refreshBtn.Create("Refresh", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, IDC_BTN_REFRESH);
        m_statusText.Create("Disconnected", WS_CHILD | WS_VISIBLE, CRect(), this, IDC_STATUS_TEXT);

        UpdateButtonState();
        return 0;
    }

    afx_msg void OnSize(UINT nType, int cx, int cy) {
        CFrameWnd::OnSize(nType, cx, cy);
        if (!::IsWindow(m_title.GetSafeHwnd())) return;

        const int pad = 18;
        const int top = 16;
        const int row = 28;
        const int panelW = 280;
        const int gap = 14;
        const int leftW = std::max(360, cx - panelW - pad * 2 - gap);
        const int panelX = pad + leftW + gap;

        m_title.MoveWindow(pad, top, 280, 34);
        m_userLabel.MoveWindow(pad, 64, 72, row);
        m_userEdit.MoveWindow(pad + 75, 62, 145, row);
        m_emailLabel.MoveWindow(pad + 235, 64, 45, row);
        m_emailEdit.MoveWindow(pad + 280, 62, 220, row);
        m_loginBtn.MoveWindow(pad + 515, 62, 125, row);
        m_setEmailBtn.MoveWindow(pad + 650, 62, 95, row);

        m_list.MoveWindow(pad, 108, leftW, std::max(220, cy - 162));
        m_statusText.MoveWindow(pad, cy - 38, leftW + panelW + gap, 24);

        int y = 108;
        m_panelTitle.MoveWindow(panelX, y, panelW, 25);
        y += 36;
        MoveLabeled(m_idLabel, m_idEdit, panelX, y, panelW); y += 42;
        MoveLabeled(m_contractLabel, m_contractEdit, panelX, y, panelW); y += 42;
        MoveLabeled(m_priceLabel, m_priceEdit, panelX, y, panelW); y += 42;
        m_condLabel.MoveWindow(panelX, y + 5, 78, row);
        m_condCombo.MoveWindow(panelX + 88, y, panelW - 88, 200); y += 42;
        MoveLabeled(m_timeLabel, m_timeEdit, panelX, y, panelW); y += 50;

        int half = (panelW - 10) / 2;
        m_addPriceBtn.MoveWindow(panelX, y, half, 32);
        m_addTimeBtn.MoveWindow(panelX + half + 10, y, half, 32); y += 40;
        m_modPriceBtn.MoveWindow(panelX, y, half, 32);
        m_modTimeBtn.MoveWindow(panelX + half + 10, y, half, 32); y += 40;
        m_deleteBtn.MoveWindow(panelX, y, panelW, 32); y += 52;
        m_statusCombo.MoveWindow(panelX, y, 145, 200);
        m_refreshBtn.MoveWindow(panelX + 155, y, panelW - 155, 30);
    }

    static void MoveLabeled(CStatic& label, CEdit& edit, int x, int y, int w) {
        label.MoveWindow(x, y + 5, 78, 26);
        edit.MoveWindow(x + 88, y, w - 88, 28);
    }

    afx_msg void OnLogin() {
        CString user;
        CString email;
        m_userEdit.GetWindowText(user);
        m_emailEdit.GetWindowText(email);
        user.Trim();
        email.Trim();
        if (user.IsEmpty() || email.IsEmpty()) {
            AfxMessageBox("Please enter username and email.");
            return;
        }

        if (!m_connected) {
            SetStatus("Connecting to server...");
            if (!m_client.Connect(SERVER_HOST, SERVER_PORT)) {
                SetStatus("Connect failed. Please start Server.exe first.");
                AfxMessageBox("Cannot connect to server. Is Server.exe running?");
                return;
            }
            m_connected = true;
            m_closing = false;
            m_recvThread = CreateThread(nullptr, 0, RecvThreadProc, this, 0, nullptr);
        }

        CString cmd;
        cmd.Format("LOGIN %s %s", (LPCSTR)user, (LPCSTR)email);
        std::vector<std::string> lines;
        if (SendCommand((LPCSTR)cmd, false, lines) && IsOk(lines)) {
            m_username = user;
            SetStatus("Logged in as " + std::string((LPCSTR)m_username));
            UpdateButtonState();
            RefreshAlerts();
        } else {
            ShowCommandError(lines, "Login failed.");
        }
    }

    afx_msg void OnSetEmail() {
        CString email;
        m_emailEdit.GetWindowText(email);
        email.Trim();
        CString cmd;
        cmd.Format("SET_EMAIL %s %s", (LPCSTR)m_username, (LPCSTR)email);
        RunSimpleCommand(cmd, "Email updated.");
    }

    afx_msg void OnAddPrice() {
        CString contract, price;
        m_contractEdit.GetWindowText(contract);
        m_priceEdit.GetWindowText(price);
        contract.Trim();
        price.Trim();
        int cond = std::max(0, m_condCombo.GetCurSel());
        CString cmd;
        cmd.Format("ADD_PRICE %s %s %s %d", (LPCSTR)m_username, (LPCSTR)contract, (LPCSTR)price, cond);
        RunSimpleCommand(cmd, "Price alert added.");
        RefreshAlerts();
    }

    afx_msg void OnAddTime() {
        CString contract, timeText;
        m_contractEdit.GetWindowText(contract);
        m_timeEdit.GetWindowText(timeText);
        contract.Trim();
        timeText.Trim();
        CString cmd;
        cmd.Format("ADD_TIME %s %s %s", (LPCSTR)m_username, (LPCSTR)contract, (LPCSTR)timeText);
        RunSimpleCommand(cmd, "Time alert added.");
        RefreshAlerts();
    }

    afx_msg void OnModifyPrice() {
        CString id, contract, price;
        m_idEdit.GetWindowText(id);
        m_contractEdit.GetWindowText(contract);
        m_priceEdit.GetWindowText(price);
        id.Trim();
        contract.Trim();
        price.Trim();
        int cond = std::max(0, m_condCombo.GetCurSel());
        CString cmd;
        cmd.Format("MODIFY_PRICE %s %s %s %d", (LPCSTR)id, (LPCSTR)contract, (LPCSTR)price, cond);
        RunSimpleCommand(cmd, "Price alert modified.");
        RefreshAlerts();
    }

    afx_msg void OnModifyTime() {
        CString id, contract, timeText;
        m_idEdit.GetWindowText(id);
        m_contractEdit.GetWindowText(contract);
        m_timeEdit.GetWindowText(timeText);
        id.Trim();
        contract.Trim();
        timeText.Trim();
        CString cmd;
        cmd.Format("MODIFY_TIME %s %s %s", (LPCSTR)id, (LPCSTR)contract, (LPCSTR)timeText);
        RunSimpleCommand(cmd, "Time alert modified.");
        RefreshAlerts();
    }

    afx_msg void OnDeleteAlert() {
        CString id;
        m_idEdit.GetWindowText(id);
        id.Trim();
        if (id.IsEmpty()) {
            AfxMessageBox("Please select or enter an alert ID.");
            return;
        }
        CString msg;
        msg.Format("Delete alert %s?", (LPCSTR)id);
        if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) != IDYES) return;
        CString cmd;
        cmd.Format("DELETE %s", (LPCSTR)id);
        RunSimpleCommand(cmd, "Alert deleted.");
        RefreshAlerts();
    }

    afx_msg void OnRefresh() {
        RefreshAlerts();
    }

    afx_msg void OnListItemChanged(NMHDR* pNMHDR, LRESULT* pResult) {
        NM_LISTVIEW* p = reinterpret_cast<NM_LISTVIEW*>(pNMHDR);
        if ((p->uChanged & LVIF_STATE) && (p->uNewState & LVIS_SELECTED)) {
            int i = p->iItem;
            m_idEdit.SetWindowText(m_list.GetItemText(i, 0));
            m_contractEdit.SetWindowText(m_list.GetItemText(i, 2));
            m_priceEdit.SetWindowText(m_list.GetItemText(i, 3));
            CString cond = m_list.GetItemText(i, 4);
            m_condCombo.SetCurSel(cond.Find("<=") >= 0 ? 1 : 0);
            m_timeEdit.SetWindowText(m_list.GetItemText(i, 5));
        }
        *pResult = 0;
    }

    afx_msg LRESULT OnTriggered(WPARAM, LPARAM lParam) {
        CString* line = reinterpret_cast<CString*>(lParam);
        CString msg = "Alert triggered:\n\n" + *line;
        SetStatus(std::string((LPCSTR)*line));
        AfxMessageBox(msg, MB_ICONINFORMATION);
        delete line;
        RefreshAlerts();
        return 0;
    }

    afx_msg LRESULT OnDisconnected(WPARAM, LPARAM) {
        if (!m_closing) SetStatus("Disconnected from server.");
        m_connected = false;
        UpdateButtonState();
        return 0;
    }

    afx_msg void OnClose() {
        StopConnection();
        CFrameWnd::OnClose();
    }

    void RefreshAlerts() {
        if (!EnsureLoggedIn()) return;
        int status = -1;
        int sel = m_statusCombo.GetCurSel();
        if (sel == 1) status = STATUS_PENDING;
        else if (sel == 2) status = STATUS_TRIGGERED;
        else if (sel == 3) status = STATUS_DELETED;

        CString cmd;
        cmd.Format("QUERY %s %d", (LPCSTR)m_username, status);
        std::vector<std::string> lines;
        if (!SendCommand((LPCSTR)cmd, true, lines)) {
            SetStatus("Query timeout or connection error.");
            return;
        }

        m_list.DeleteAllItems();
        for (const auto& line : lines) {
            if (line.rfind("ALERT ", 0) == 0) AddAlertLine(line);
        }
        CString s;
        s.Format("Loaded %d alert(s).", m_list.GetItemCount());
        SetStatus((LPCSTR)s);
    }

    void AddAlertLine(const std::string& line) {
        auto t = Split(line);
        if (t.size() < 9) return;
        int row = m_list.InsertItem(m_list.GetItemCount(), t[1].c_str());
        m_list.SetItemText(row, 1, t[2] == "0" ? "Price" : "Time");
        m_list.SetItemText(row, 2, t[3].c_str());
        m_list.SetItemText(row, 3, t[4].c_str());
        m_list.SetItemText(row, 4, t[5] == "1" ? "<=" : ">=");
        m_list.SetItemText(row, 5, t[6].c_str());
        m_list.SetItemText(row, 6, StatusName(t[7]).c_str());
        std::string created = t[8];
        if (t.size() > 9) created += " " + t[9];
        m_list.SetItemText(row, 7, created.c_str());
    }

    std::string StatusName(const std::string& s) const {
        if (s == "0") return "Pending";
        if (s == "1") return "Triggered";
        if (s == "2") return "Deleted";
        return s;
    }

    void RunSimpleCommand(const CString& cmd, const char* successText) {
        if (!EnsureLoggedIn()) return;
        std::vector<std::string> lines;
        if (SendCommand((LPCSTR)cmd, false, lines) && IsOk(lines)) {
            SetStatus(successText);
        } else {
            ShowCommandError(lines, "Command failed.");
        }
    }

    bool SendCommand(const std::string& cmd, bool expectEnd, std::vector<std::string>& lines) {
        ClearPendingResponses();
        if (!m_connected || !m_client.SendLine(cmd)) return false;

        DWORD deadline = GetTickCount() + 6000;
        while (true) {
            DWORD now = GetTickCount();
            if (now >= deadline) return false;
            std::string line;
            if (!ReadResponseLine(line, deadline - now)) return false;
            lines.push_back(line);

            if (line == RESP_END) return true;
            if (line.rfind(RESP_ERR, 0) == 0) return true;
            if (!expectEnd && (line.rfind(RESP_OK, 0) == 0 || line.rfind(RESP_ERR, 0) == 0)) {
                return true;
            }
        }
    }

    void ClearPendingResponses() {
        std::lock_guard<std::mutex> lk(m_responseMtx);
        while (!m_responseQ.empty()) m_responseQ.pop();
    }

    bool ReadResponseLine(std::string& line, DWORD timeoutMs) {
        std::unique_lock<std::mutex> lk(m_responseMtx);
        bool ok = m_responseCV.wait_for(
            lk, std::chrono::milliseconds(timeoutMs),
            [this] { return !m_responseQ.empty(); });
        if (!ok) return false;
        line = m_responseQ.front();
        m_responseQ.pop();
        return true;
    }

    static bool IsOk(const std::vector<std::string>& lines) {
        return !lines.empty() && lines.front().rfind(RESP_OK, 0) == 0;
    }

    void ShowCommandError(const std::vector<std::string>& lines, const char* fallback) {
        CString msg = fallback;
        if (!lines.empty()) msg = lines.front().c_str();
        SetStatus((LPCSTR)msg);
        AfxMessageBox(msg, MB_ICONWARNING);
    }

    bool EnsureLoggedIn() {
        if (!m_connected || m_username.IsEmpty()) {
            AfxMessageBox("Please connect and login first.");
            return false;
        }
        return true;
    }

    void UpdateButtonState() {
        BOOL enabled = m_connected && !m_username.IsEmpty();
        m_setEmailBtn.EnableWindow(enabled);
        m_addPriceBtn.EnableWindow(enabled);
        m_addTimeBtn.EnableWindow(enabled);
        m_modPriceBtn.EnableWindow(enabled);
        m_modTimeBtn.EnableWindow(enabled);
        m_deleteBtn.EnableWindow(enabled);
        m_refreshBtn.EnableWindow(enabled);
    }

    void SetStatus(const std::string& text) {
        if (::IsWindow(m_statusText.GetSafeHwnd())) m_statusText.SetWindowText(text.c_str());
    }

    void StopConnection() {
        m_closing = true;
        if (m_connected) m_client.Close();
        if (m_recvThread) {
            WaitForSingleObject(m_recvThread, 1500);
            CloseHandle(m_recvThread);
            m_recvThread = nullptr;
        }
        m_connected = false;
    }

    static DWORD WINAPI RecvThreadProc(LPVOID p) {
        CMainFrame* self = reinterpret_cast<CMainFrame*>(p);
        std::string line;
        while (!self->m_closing && self->m_client.IsConnected() && self->m_client.RecvLine(line)) {
            if (line.rfind(RESP_TRIGGERED, 0) == 0) {
                CString* msg = new CString(line.c_str());
                self->PostMessage(WM_APP_TRIGGERED, 0, reinterpret_cast<LPARAM>(msg));
            } else {
                {
                    std::lock_guard<std::mutex> lk(self->m_responseMtx);
                    self->m_responseQ.push(line);
                }
                self->m_responseCV.notify_one();
            }
        }
        self->PostMessage(WM_APP_DISCONNECTED, 0, 0);
        return 0;
    }

    DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_BTN_LOGIN, &CMainFrame::OnLogin)
    ON_BN_CLICKED(IDC_BTN_SET_EMAIL, &CMainFrame::OnSetEmail)
    ON_BN_CLICKED(IDC_BTN_ADD_PRICE, &CMainFrame::OnAddPrice)
    ON_BN_CLICKED(IDC_BTN_ADD_TIME, &CMainFrame::OnAddTime)
    ON_BN_CLICKED(IDC_BTN_MOD_PRICE, &CMainFrame::OnModifyPrice)
    ON_BN_CLICKED(IDC_BTN_MOD_TIME, &CMainFrame::OnModifyTime)
    ON_BN_CLICKED(IDC_BTN_DELETE, &CMainFrame::OnDeleteAlert)
    ON_BN_CLICKED(IDC_BTN_REFRESH, &CMainFrame::OnRefresh)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_ALERTS, &CMainFrame::OnListItemChanged)
    ON_MESSAGE(WM_APP_TRIGGERED, &CMainFrame::OnTriggered)
    ON_MESSAGE(WM_APP_DISCONNECTED, &CMainFrame::OnDisconnected)
END_MESSAGE_MAP()

class CMFCClientApp : public CWinApp {
public:
    BOOL InitInstance() override {
        CWinApp::InitInstance();
        INITCOMMONCONTROLSEX icc = {};
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
        InitCommonControlsEx(&icc);

        CMainFrame* frame = new CMainFrame();
        m_pMainWnd = frame;
        frame->ShowWindow(SW_SHOW);
        frame->UpdateWindow();
        return TRUE;
    }
};

CMFCClientApp theApp;
