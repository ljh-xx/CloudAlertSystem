// MainFrame.cpp - main alert management window for Cloud Alert System.

#include "MainFrame.h"
#include "ChangeEmailDlg.h"
#include "LoginDlg.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <set>
#include <sstream>

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 8888

enum ControlId {
    IDC_EDIT_EMAIL = 1001,
    IDC_BTN_SET_EMAIL,
    IDC_BTN_LOGOUT,
    IDC_LIST_ALERTS,
    IDC_EDIT_ID,
    IDC_COMBO_ALERT_TYPE,
    IDC_EDIT_CONTRACT,
    IDC_BTN_PICK_CONTRACT,
    IDC_EDIT_PRICE,
    IDC_COMBO_COND,
    IDC_EDIT_TIME,
    IDC_BTN_ADD_PRICE,
    IDC_BTN_ADD_TIME,
    IDC_BTN_MOD_PRICE,
    IDC_BTN_MOD_TIME,
    IDC_BTN_DELETE,
    IDC_COMBO_STATUS,
    IDC_BTN_VIEW_DELETED,
    IDC_BTN_REFRESH,
    IDC_BTN_CANCEL_EDIT,
    IDC_USER_INFO,
    IDC_EMAIL_INFO,
    IDC_CONNECTION_INFO,
    IDC_LIST_TITLE,
    IDC_STATUS_TEXT,
    IDC_TOAST_TEXT
};

static const UINT WM_APP_TRIGGERED = WM_APP + 1;
static const UINT WM_APP_DISCONNECTED = WM_APP + 2;
static const UINT WM_APP_POST_LOGIN = WM_APP + 3;
static const UINT_PTR TIMER_TOAST = 3001;
static const UINT_PTR TIMER_PRICE_REFRESH = 3002;

static std::vector<std::string> Split(const std::string& text) {
    std::istringstream iss(text);
    std::vector<std::string> out;
    std::string tok;
    while (iss >> tok) out.push_back(tok);
    return out;
}

CMainFrame::CMainFrame()
    : m_connected(false), m_closing(false), m_editMode(false),
      m_showDeletedOnly(false), m_editAlertType(ALERT_TYPE_PRICE), m_recvThread(nullptr) {
    Create(nullptr, "Cloud Alert MFC Client",
           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
           CRect(80, 60, 1260, 820));
}

CMainFrame::~CMainFrame() {
    StopConnection();
}

bool CMainFrame::ShowLoginDialog() {
    CLoginDlg dlg(this);
    return dlg.DoModal() == IDOK;
}

bool CMainFrame::LoginUser(const CString& user, const CString& password, std::string& error) {
    if (!EnsureConnected()) {
        error = "Cannot connect to server. Is Server.exe running?";
        return false;
    }

    CString cmd;
    cmd.Format("LOGIN %s %s", (LPCSTR)user, (LPCSTR)password);
    std::vector<std::string> lines;
    if (SendCommand((LPCSTR)cmd, false, lines) && IsOk(lines)) {
        m_username = user;
        m_showDeletedOnly = false;
        if (!StartReceiver()) {
            m_username.Empty();
            error = "Login succeeded, but receiver thread failed to start.";
            UpdateButtonState();
            return false;
        }
        SetStatus("Logged in. Click Refresh to load alerts.");
        m_connectionInfo.SetWindowText("Server: 127.0.0.1:8888 | Online");
        ShowToast("Login successful");
        UpdateButtonState();
        return true;
    }

    error = lines.empty() ? "Login failed." : lines.front();
    return false;
}

void CMainFrame::BeginPostLoginRefresh() {
    PostMessage(WM_APP_POST_LOGIN, 0, 0);
}

bool CMainFrame::RegisterUser(const CString& user, const CString& password, const CString& email, std::string& error) {
    if (!EnsureConnected()) {
        error = "Cannot connect to server. Is Server.exe running?";
        return false;
    }

    CString cmd;
    cmd.Format("REGISTER %s %s %s", (LPCSTR)user, (LPCSTR)password, (LPCSTR)email);
    std::vector<std::string> lines;
    if (SendCommand((LPCSTR)cmd, false, lines) && IsOk(lines)) {
        SetStatus("Registration successful. You can login now.");
        return true;
    }

    error = lines.empty() ? "Registration failed." : lines.front();
    return false;
}

bool CMainFrame::ChangeEmail(const CString& password, const CString& email, std::string& error) {
    if (!EnsureConnected() || m_username.IsEmpty()) {
        error = "Please login first.";
        return false;
    }

    CString cmd;
    cmd.Format("SET_EMAIL %s %s %s", (LPCSTR)m_username, (LPCSTR)password, (LPCSTR)email);
    std::vector<std::string> lines;
    if (SendCommand((LPCSTR)cmd, false, lines) && IsOk(lines)) {
        SetStatus("Email updated.");
        m_email = email;
        ShowToast("Email updated");
        UpdateButtonState();
        return true;
    }

    error = lines.empty() ? "Failed to update email." : lines.front();
    return false;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) {
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1) return -1;

    m_titleFont.CreatePointFont(180, "Segoe UI");
    m_title.Create("Cloud Alert System", WS_CHILD | WS_VISIBLE, CRect(), this);
    m_title.SetFont(&m_titleFont);

    m_setEmailBtn.Create("Change Email", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW, CRect(), this, IDC_BTN_SET_EMAIL);
    m_setEmailBtn.SetColors(RGB(235, 245, 255), RGB(20, 88, 160), RGB(132, 181, 232));
    m_logoutBtn.Create("Logout", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW, CRect(), this, IDC_BTN_LOGOUT);
    m_logoutBtn.SetColors(RGB(246, 247, 249), RGB(72, 76, 84), RGB(176, 182, 192));
    m_userInfo.Create("Not logged in", WS_CHILD | WS_VISIBLE, CRect(), this, IDC_USER_INFO);
    m_emailInfo.Create("Email: -", WS_CHILD | WS_VISIBLE, CRect(), this, IDC_EMAIL_INFO);
    m_connectionInfo.Create("Server: 127.0.0.1:8888 | Offline", WS_CHILD | WS_VISIBLE, CRect(), this, IDC_CONNECTION_INFO);
    m_listTitle.Create("Alert List", WS_CHILD | WS_VISIBLE, CRect(), this, IDC_LIST_TITLE);

    m_list.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
                  CRect(), this, IDC_LIST_ALERTS);
    m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    m_list.InsertColumn(0, "ID", LVCFMT_LEFT, 55);
    m_list.InsertColumn(1, "Type", LVCFMT_LEFT, 70);
    m_list.InsertColumn(2, "Contract", LVCFMT_LEFT, 95);
    m_list.InsertColumn(3, "Trigger", LVCFMT_RIGHT, 90);
    m_list.InsertColumn(4, "Current", LVCFMT_RIGHT, 90);
    m_list.InsertColumn(5, "Cond", LVCFMT_LEFT, 70);
    m_list.InsertColumn(6, "Time", LVCFMT_LEFT, 90);
    m_list.InsertColumn(7, "Status", LVCFMT_LEFT, 90);
    m_list.InsertColumn(8, "Created", LVCFMT_LEFT, 165);

    m_panelTitle.Create("New Alert", WS_CHILD | WS_VISIBLE, CRect(), this);
    m_addGroupTitle.Create("", WS_CHILD | WS_VISIBLE, CRect(), this);
    m_editGroupTitle.Create("", WS_CHILD | WS_VISIBLE, CRect(), this);
    m_idLabel.Create("Alert ID", WS_CHILD | WS_VISIBLE, CRect(), this);
    m_typeLabel.Create("Type", WS_CHILD | WS_VISIBLE, CRect(), this);
    m_contractLabel.Create("Contract", WS_CHILD | WS_VISIBLE, CRect(), this);
    m_priceLabel.Create("Price", WS_CHILD | WS_VISIBLE, CRect(), this);
    m_condLabel.Create("Condition", WS_CHILD | WS_VISIBLE, CRect(), this);
    m_timeLabel.Create("Time", WS_CHILD | WS_VISIBLE, CRect(), this);
    m_idEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, IDC_EDIT_ID);
    m_typeCombo.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST, CRect(), this, IDC_COMBO_ALERT_TYPE);
    m_typeCombo.AddString("Price Alert");
    m_typeCombo.AddString("Time Alert");
    m_typeCombo.SetCurSel(0);
    m_contractEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, IDC_EDIT_CONTRACT);
    m_contractPickBtn.Create("Contracts", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW, CRect(), this, IDC_BTN_PICK_CONTRACT);
    m_contractPickBtn.SetColors(RGB(238, 242, 247), RGB(28, 34, 44), RGB(166, 176, 190));
    m_priceEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, IDC_EDIT_PRICE);
    m_timeEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, IDC_EDIT_TIME);
    m_contractEdit.SetWindowText("rb2609");
    m_timeEdit.SetWindowText("09:30:00");

    m_condCombo.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST, CRect(), this, IDC_COMBO_COND);
    m_condCombo.AddString(">= target");
    m_condCombo.AddString("<= target");
    m_condCombo.SetCurSel(0);

    m_addPriceBtn.Create("Add Alert", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW, CRect(), this, IDC_BTN_ADD_PRICE);
    m_addPriceBtn.SetColors(RGB(32, 113, 214), RGB(255, 255, 255), RGB(22, 88, 170));
    m_addTimeBtn.Create("Add Time", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW, CRect(), this, IDC_BTN_ADD_TIME);
    m_addTimeBtn.SetColors(RGB(32, 113, 214), RGB(255, 255, 255), RGB(22, 88, 170));
    m_modPriceBtn.Create("Save Changes", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW, CRect(), this, IDC_BTN_MOD_PRICE);
    m_modPriceBtn.SetColors(RGB(255, 247, 226), RGB(120, 79, 0), RGB(222, 178, 87));
    m_modTimeBtn.Create("Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW, CRect(), this, IDC_BTN_CANCEL_EDIT);
    m_modTimeBtn.SetColors(RGB(246, 247, 249), RGB(72, 76, 84), RGB(176, 182, 192));
    m_deleteBtn.Create("Delete Alert", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW, CRect(), this, IDC_BTN_DELETE);
    m_deleteBtn.SetColors(RGB(218, 48, 49), RGB(255, 255, 255), RGB(170, 32, 35));

    m_statusCombo.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST, CRect(), this, IDC_COMBO_STATUS);
    m_statusCombo.AddString("All");
    m_statusCombo.AddString("Pending");
    m_statusCombo.AddString("Triggered");
    m_statusCombo.SetCurSel(0);
    m_deletedViewBtn.Create("Deleted Only", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW, CRect(), this, IDC_BTN_VIEW_DELETED);
    m_deletedViewBtn.SetColors(RGB(255, 241, 241), RGB(145, 38, 38), RGB(230, 160, 160));
    m_refreshBtn.Create("Refresh", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW, CRect(), this, IDC_BTN_REFRESH);
    m_refreshBtn.SetColors(RGB(238, 242, 247), RGB(28, 34, 44), RGB(166, 176, 190));
    m_statusText.Create("Disconnected", WS_CHILD | WS_VISIBLE, CRect(), this, IDC_STATUS_TEXT);
    m_toastText.Create("", WS_CHILD | WS_BORDER | SS_CENTER, CRect(), this, IDC_TOAST_TEXT);
    m_toastText.ShowWindow(SW_HIDE);

    EnterAddMode();
    UpdateButtonState();
    return 0;
}

void CMainFrame::OnSize(UINT nType, int cx, int cy) {
    CFrameWnd::OnSize(nType, cx, cy);
    if (!::IsWindow(m_title.GetSafeHwnd()) || !::IsWindow(m_typeCombo.GetSafeHwnd())) return;

    const int pad = 22;
    const int top = 16;
    const int row = 28;
    const int panelW = 330;
    const int gap = 18;
    const int leftW = std::max(520, cx - panelW - pad * 2 - gap);
    const int panelX = pad + leftW + gap;

    m_title.MoveWindow(pad, top, 330, 38);
    m_connectionInfo.MoveWindow(std::max(pad + 420, cx - 350), 24, 330, 24);
    m_userInfo.MoveWindow(pad, 66, 260, 24);
    m_emailInfo.MoveWindow(pad, 92, 360, 24);
    m_setEmailBtn.MoveWindow(pad + 380, 72, 132, 32);
    m_logoutBtn.MoveWindow(pad + 524, 72, 104, 32);

    m_listTitle.MoveWindow(pad, 118, 180, 24);
    m_statusCombo.MoveWindow(pad + leftW - 372, 114, 128, 200);
    m_deletedViewBtn.MoveWindow(pad + leftW - 234, 114, 124, 32);
    m_refreshBtn.MoveWindow(pad + leftW - 100, 114, 100, 32);
    m_list.MoveWindow(pad, 154, leftW, std::max(260, cy - 214));
    m_statusText.MoveWindow(pad, cy - 38, leftW + panelW + gap, 24);
    m_toastText.MoveWindow(panelX, std::max(154, cy - 76), panelW, 30);

    int y = 154;
    m_panelTitle.MoveWindow(panelX, y, panelW, 28);
    y += 36;
    if (m_editMode) {
        MoveLabeled(m_idLabel, m_idEdit, panelX, y, panelW);
        y += 44;
    }
    MoveLabeled(m_typeLabel, m_typeCombo, panelX, y, panelW); y += 44;
    m_contractLabel.MoveWindow(panelX, y + 5, 86, row);
    m_contractEdit.MoveWindow(panelX + 95, y, panelW - 205, 30);
    m_contractPickBtn.MoveWindow(panelX + panelW - 102, y, 102, 30);
    y += 44;
    if (GetSelectedAlertType() == ALERT_TYPE_PRICE) {
        MoveLabeled(m_priceLabel, m_priceEdit, panelX, y, panelW); y += 44;
        m_condLabel.MoveWindow(panelX, y + 5, 78, row);
        m_condCombo.MoveWindow(panelX + 95, y, panelW - 95, 200); y += 54;
    } else {
        MoveLabeled(m_timeLabel, m_timeEdit, panelX, y, panelW); y += 54;
    }

    int half = (panelW - 12) / 2;
    if (m_editMode) {
        m_modPriceBtn.MoveWindow(panelX, y, half, 34);
        m_modTimeBtn.MoveWindow(panelX + half + 12, y, half, 34);
        y += 46;
        m_deleteBtn.MoveWindow(panelX, y, panelW, 36);
    } else {
        m_addPriceBtn.MoveWindow(panelX, y, panelW, 36);
    }
}

void CMainFrame::MoveLabeled(CStatic& label, CEdit& edit, int x, int y, int w) {
    label.MoveWindow(x, y + 5, 86, 26);
    edit.MoveWindow(x + 95, y, w - 95, 30);
}

void CMainFrame::MoveLabeled(CStatic& label, CWnd& ctrl, int x, int y, int w) {
    label.MoveWindow(x, y + 5, 86, 26);
    ctrl.MoveWindow(x + 95, y, w - 95, 160);
}

void CMainFrame::OnSetEmail() {
    if (!EnsureLoggedIn()) return;
    CChangeEmailDlg dlg(this);
    if (dlg.DoModal() == IDOK) {
        RefreshAlerts();
    }
}

void CMainFrame::OnLogout() {
    KillTimer(TIMER_PRICE_REFRESH);
    StopConnection();
    m_username.Empty();
    m_email.Empty();
    m_showDeletedOnly = false;
    m_list.DeleteAllItems();
    EnterAddMode();
    SetStatus("Logged out.");
    UpdateButtonState();
    ShowWindow(SW_HIDE);
    if (ShowLoginDialog()) {
        ShowWindow(SW_SHOW);
        UpdateWindow();
        BeginPostLoginRefresh();
    } else {
        PostMessage(WM_CLOSE);
    }
}

void CMainFrame::OnAddPrice() {
    CString contract, price, timeText;
    m_contractEdit.GetWindowText(contract);
    m_priceEdit.GetWindowText(price);
    m_timeEdit.GetWindowText(timeText);
    contract.Trim();
    price.Trim();
    timeText.Trim();
    if (!ValidateContract(contract)) return;
    CString cmd;
    if (GetSelectedAlertType() == ALERT_TYPE_PRICE) {
        if (!ValidatePrice(price)) return;
        int cond = std::max(0, m_condCombo.GetCurSel());
        cmd.Format("ADD_PRICE %s %s %s %d", (LPCSTR)m_username, (LPCSTR)contract, (LPCSTR)price, cond);
        RunSimpleCommand(cmd, "Price alert added.");
    } else {
        if (!ValidateTimeText(timeText)) return;
        cmd.Format("ADD_TIME %s %s %s", (LPCSTR)m_username, (LPCSTR)contract, (LPCSTR)timeText);
        RunSimpleCommand(cmd, "Time alert added.");
    }
    RefreshAlerts();
    EnterAddMode();
}

void CMainFrame::OnAddTime() {
    OnAddPrice();
}

void CMainFrame::OnModifyPrice() {
    CString id, contract, price, timeText;
    m_idEdit.GetWindowText(id);
    m_contractEdit.GetWindowText(contract);
    m_priceEdit.GetWindowText(price);
    m_timeEdit.GetWindowText(timeText);
    id.Trim();
    contract.Trim();
    price.Trim();
    timeText.Trim();
    if (!ValidateAlertId(id) || !ValidateContract(contract)) return;
    CString cmd;
    if (m_editAlertType == ALERT_TYPE_PRICE) {
        if (!ValidatePrice(price)) return;
        int cond = std::max(0, m_condCombo.GetCurSel());
        cmd.Format("MODIFY_PRICE %s %s %s %d", (LPCSTR)id, (LPCSTR)contract, (LPCSTR)price, cond);
        RunSimpleCommand(cmd, "Price alert modified.");
    } else {
        if (!ValidateTimeText(timeText)) return;
        cmd.Format("MODIFY_TIME %s %s %s", (LPCSTR)id, (LPCSTR)contract, (LPCSTR)timeText);
        RunSimpleCommand(cmd, "Time alert modified.");
    }
    RefreshAlerts();
    EnterAddMode();
}

void CMainFrame::OnModifyTime() {
    EnterAddMode();
}

void CMainFrame::OnDeleteAlert() {
    CString id;
    m_idEdit.GetWindowText(id);
    id.Trim();
    if (!ValidateAlertId(id)) return;
    CString msg;
    msg.Format("Delete alert %s?", (LPCSTR)id);
    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    CString cmd;
    cmd.Format("DELETE %s", (LPCSTR)id);
    RunSimpleCommand(cmd, "Alert deleted.");
    RefreshAlerts();
    EnterAddMode();
}

void CMainFrame::OnRefresh() {
    RefreshEmail();
    RefreshAlerts();
}

void CMainFrame::OnViewDeleted() {
    if (!m_connected || m_username.IsEmpty()) return;
    m_showDeletedOnly = !m_showDeletedOnly;
    m_statusCombo.SetCurSel(0);
    UpdateButtonState();
    RefreshAlerts();
    EnterAddMode();
}

void CMainFrame::OnStatusFilterChanged() {
    if (!m_connected || m_username.IsEmpty()) return;
    m_showDeletedOnly = false;
    UpdateButtonState();
    RefreshAlerts();
}

void CMainFrame::OnPickContract() {
    static const struct {
        const char* label;
        const char* code;
    } contracts[] = {
        { "Rebar - rb2609", "rb2609" },
        { "Rebar - rb2607", "rb2607" },
        { "Hot Coil - hc2609", "hc2609" },
        { "Iron Ore - i2609", "i2609" },
        { "Methanol - MA609", "MA609" },
        { "PTA - TA609", "TA609" },
        { "Egg - jd2609", "jd2609" },
        { "Soymeal - m2609", "m2609" },
        { "Soy Oil - y2609", "y2609" },
        { "Palm Oil - p2609", "p2609" },
        { "Corn - c2609", "c2609" },
        { "Cotton - CF609", "CF609" },
        { "Sugar - SR609", "SR609" },
        { "Gold - au2612", "au2612" },
        { "Silver - ag2612", "ag2612" },
    };

    CMenu menu;
    if (!menu.CreatePopupMenu()) return;

    const UINT baseId = 7000;
    for (int i = 0; i < static_cast<int>(sizeof(contracts) / sizeof(contracts[0])); ++i) {
        menu.AppendMenu(MF_STRING | MF_ENABLED, baseId + i, contracts[i].label);
    }

    CRect rc;
    m_contractPickBtn.GetWindowRect(&rc);
    UINT cmd = menu.TrackPopupMenu(
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
        rc.left,
        rc.bottom + 2,
        this);
    if (cmd >= baseId && cmd < baseId + sizeof(contracts) / sizeof(contracts[0])) {
        int index = static_cast<int>(cmd - baseId);
        m_contractEdit.SetWindowText(contracts[index].code);
        m_contractEdit.SetFocus();
        ShowToast(std::string("Contract selected: ") + contracts[index].code);
    }
}

void CMainFrame::OnAlertTypeChanged() {
    if (!m_editMode) {
        UpdateEditorMode();
    }
}

void CMainFrame::OnListItemChanged(NMHDR* pNMHDR, LRESULT* pResult) {
    NM_LISTVIEW* p = reinterpret_cast<NM_LISTVIEW*>(pNMHDR);
    if ((p->uChanged & LVIF_STATE) && (p->uNewState & LVIS_SELECTED)) {
        EnterEditMode(p->iItem);
    }
    *pResult = 0;
}

LRESULT CMainFrame::OnTriggered(WPARAM, LPARAM lParam) {
    CString* line = reinterpret_cast<CString*>(lParam);
    CString msg = "Alert triggered:\n\n" + *line;
    SetStatus(std::string((LPCSTR)*line));
    ShowToast("Alert triggered");
    AfxMessageBox(msg, MB_ICONINFORMATION);
    delete line;
    RefreshAlerts();
    return 0;
}

LRESULT CMainFrame::OnPostLogin(WPARAM, LPARAM) {
    if (!m_connected || m_username.IsEmpty()) return 0;
    SetStatus("Loading account data...");
    RefreshEmail();
    RefreshAlerts();
    SetTimer(TIMER_PRICE_REFRESH, 30000, nullptr);
    UpdateButtonState();
    return 0;
}

LRESULT CMainFrame::OnDisconnected(WPARAM, LPARAM) {
    if (m_closing) return 0;
    KillTimer(TIMER_PRICE_REFRESH);
    SetStatus("Disconnected from server. Please check whether Server.exe is still running.");
    m_connected = false;
    m_username.Empty();
    m_email.Empty();
    m_showDeletedOnly = false;
    m_list.DeleteAllItems();
    EnterAddMode();
    UpdateButtonState();
    return 0;
}

void CMainFrame::OnClose() {
    KillTimer(TIMER_PRICE_REFRESH);
    StopConnection();
    CFrameWnd::OnClose();
}

void CMainFrame::OnTimer(UINT_PTR idEvent) {
    if (idEvent == TIMER_TOAST) {
        KillTimer(TIMER_TOAST);
        m_toastText.ShowWindow(SW_HIDE);
        return;
    }
    if (idEvent == TIMER_PRICE_REFRESH) {
        if (m_connected && !m_username.IsEmpty()) {
            RefreshCurrentPrices();
        }
        return;
    }
    CFrameWnd::OnTimer(idEvent);
}

void CMainFrame::EnterAddMode() {
    m_toastText.ShowWindow(SW_HIDE);
    m_editMode = false;
    m_editAlertType = ALERT_TYPE_PRICE;
    m_idEdit.SetWindowText("");
    m_typeCombo.SetCurSel(0);
    m_contractEdit.SetWindowText("rb2609");
    m_priceEdit.SetWindowText("");
    m_condCombo.SetCurSel(0);
    m_timeEdit.SetWindowText("09:30:00");

    for (int i = 0; i < m_list.GetItemCount(); ++i) {
        m_list.SetItemState(i, 0, LVIS_SELECTED);
    }
    UpdateEditorMode();
}

void CMainFrame::EnterEditMode(int item) {
    m_toastText.ShowWindow(SW_HIDE);
    if (item < 0 || item >= m_list.GetItemCount()) return;

    m_editMode = true;
    CString type = m_list.GetItemText(item, 1);
    m_editAlertType = (type == "Time") ? ALERT_TYPE_TIME : ALERT_TYPE_PRICE;

    m_idEdit.SetWindowText(m_list.GetItemText(item, 0));
    m_typeCombo.SetCurSel(m_editAlertType == ALERT_TYPE_TIME ? 1 : 0);
    m_contractEdit.SetWindowText(m_list.GetItemText(item, 2));
    if (m_editAlertType == ALERT_TYPE_PRICE) {
        m_priceEdit.SetWindowText(m_list.GetItemText(item, 3));
        CString cond = m_list.GetItemText(item, 5);
        m_condCombo.SetCurSel(cond.Find("<=") >= 0 ? 1 : 0);
        m_timeEdit.SetWindowText("");
    } else {
        m_priceEdit.SetWindowText("");
        m_condCombo.SetCurSel(0);
        m_timeEdit.SetWindowText(m_list.GetItemText(item, 6));
    }
    UpdateEditorMode();
}

void CMainFrame::UpdateEditorMode() {
    if (!::IsWindow(m_panelTitle.GetSafeHwnd())) return;

    if (m_editMode) {
        CString id;
        m_idEdit.GetWindowText(id);
        CString title;
        title.Format("Edit Alert (ID: %s)", (LPCSTR)id);
        m_panelTitle.SetWindowText(title);
        m_modPriceBtn.SetWindowText("Save Changes");
        m_modTimeBtn.SetWindowText("Cancel");
    } else {
        m_panelTitle.SetWindowText("New Alert");
        m_addPriceBtn.SetWindowText("Add Alert");
    }

    m_idEdit.EnableWindow(FALSE);
    m_typeCombo.EnableWindow(!m_editMode);
    UpdateEditorVisibility();
    UpdateButtonState();

    CRect rc;
    GetClientRect(&rc);
    OnSize(SIZE_RESTORED, rc.Width(), rc.Height());
    RedrawEditorArea();
}

void CMainFrame::UpdateEditorVisibility() {
    const BOOL editing = m_editMode ? TRUE : FALSE;
    const BOOL priceMode = (GetSelectedAlertType() == ALERT_TYPE_PRICE) ? TRUE : FALSE;

    HideEditorControl(m_addGroupTitle);
    HideEditorControl(m_editGroupTitle);
    m_idLabel.ShowWindow(editing ? SW_SHOW : SW_HIDE);
    m_idEdit.ShowWindow(editing ? SW_SHOW : SW_HIDE);
    m_typeLabel.ShowWindow(SW_SHOW);
    m_typeCombo.ShowWindow(SW_SHOW);
    m_contractLabel.ShowWindow(SW_SHOW);
    m_contractEdit.ShowWindow(SW_SHOW);
    m_contractPickBtn.ShowWindow(SW_SHOW);

    if (priceMode) {
        m_priceLabel.ShowWindow(SW_SHOW);
        m_priceEdit.ShowWindow(SW_SHOW);
        m_condLabel.ShowWindow(SW_SHOW);
        m_condCombo.ShowWindow(SW_SHOW);
        HideEditorControl(m_timeLabel);
        HideEditorControl(m_timeEdit);
    } else {
        HideEditorControl(m_priceLabel);
        HideEditorControl(m_priceEdit);
        HideEditorControl(m_condLabel);
        HideEditorControl(m_condCombo);
        m_timeLabel.ShowWindow(SW_SHOW);
        m_timeEdit.ShowWindow(SW_SHOW);
    }

    if (editing) {
        HideEditorControl(m_addPriceBtn);
        HideEditorControl(m_addTimeBtn);
        m_modPriceBtn.ShowWindow(SW_SHOW);
        m_modTimeBtn.ShowWindow(SW_SHOW);
        m_deleteBtn.ShowWindow(SW_SHOW);
    } else {
        m_addPriceBtn.ShowWindow(SW_SHOW);
        HideEditorControl(m_addTimeBtn);
        HideEditorControl(m_modPriceBtn);
        HideEditorControl(m_modTimeBtn);
        HideEditorControl(m_deleteBtn);
    }
    RedrawEditorArea();
}

void CMainFrame::HideEditorControl(CWnd& ctrl) {
    if (!::IsWindow(ctrl.GetSafeHwnd()) || !ctrl.IsWindowVisible()) return;

    CRect rc;
    ctrl.GetWindowRect(&rc);
    ScreenToClient(&rc);
    ctrl.ShowWindow(SW_HIDE);
    InvalidateRect(&rc, TRUE);
}

void CMainFrame::RedrawEditorArea() {
    if (!::IsWindow(GetSafeHwnd()) || !::IsWindow(m_panelTitle.GetSafeHwnd())) return;

    CRect panel;
    m_panelTitle.GetWindowRect(&panel);
    ScreenToClient(&panel);
    panel.left -= 4;
    panel.top -= 4;
    CRect client;
    GetClientRect(&client);
    panel.right = client.right - 18;
    panel.bottom = client.bottom - 48;
    RedrawWindow(&panel, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

int CMainFrame::GetSelectedAlertType() {
    return m_typeCombo.GetCurSel() == 1 ? ALERT_TYPE_TIME : ALERT_TYPE_PRICE;
}

void CMainFrame::RefreshAlerts() {
    if (!EnsureLoggedIn()) return;
    m_listTitle.SetWindowText(m_showDeletedOnly ? "Deleted Alerts" : "Alert List");
    int status = -1;
    int sel = m_statusCombo.GetCurSel();
    if (m_showDeletedOnly) status = STATUS_DELETED;
    else if (sel == 1) status = STATUS_PENDING;
    else if (sel == 2) status = STATUS_TRIGGERED;

    CString cmd;
    cmd.Format("QUERY %s %d", (LPCSTR)m_username, status);
    std::vector<std::string> lines;
    if (!SendCommand((LPCSTR)cmd, true, lines)) {
        SetStatus("Query timeout or connection error.");
        return;
    }

    m_list.DeleteAllItems();
    for (const auto& line : lines) {
        if (line.rfind("ALERT ", 0) != 0) continue;
        auto fields = Split(line);
        if (!m_showDeletedOnly && fields.size() > 7 && fields[7] == "2") continue;
        AddAlertLine(line);
    }
    RefreshCurrentPrices();
    CString s;
    s.Format(m_showDeletedOnly ? "Loaded %d deleted alert(s)." : "Loaded %d alert(s).", m_list.GetItemCount());
    SetStatus((LPCSTR)s);
}

void CMainFrame::RefreshCurrentPrices() {
    if (m_username.IsEmpty() || !m_connected || m_list.GetItemCount() == 0) return;

    std::set<std::string> contracts;
    for (int i = 0; i < m_list.GetItemCount(); ++i) {
        CString contract = m_list.GetItemText(i, 2);
        contract.Trim();
        if (!contract.IsEmpty()) contracts.insert((LPCSTR)contract);
    }
    if (contracts.empty()) return;

    CString cmd = "GET_PRICES";
    for (const auto& contract : contracts) {
        cmd += " ";
        cmd += contract.c_str();
    }

    std::vector<std::string> lines;
    if (!SendCommand((LPCSTR)cmd, true, lines)) {
        SetStatus("Current prices unavailable: price query timed out.");
        return;
    }
    if (!lines.empty() && lines.front().rfind(RESP_ERR, 0) == 0) {
        SetStatus("Current prices unavailable: please restart the updated Server.exe.");
        return;
    }

    int updated = 0;
    for (const auto& line : lines) {
        if (line.rfind("PRICE ", 0) != 0) continue;
        auto fields = Split(line);
        if (fields.size() < 3) continue;
        CString contract = fields[1].c_str();
        CString price = FormatPriceText(fields[2]).c_str();
        for (int i = 0; i < m_list.GetItemCount(); ++i) {
            if (m_list.GetItemText(i, 2) == contract) {
                m_list.SetItemText(i, 4, price);
                if (price != "-") ++updated;
            }
        }
    }
    if (updated == 0) {
        SetStatus("Current prices waiting for CTP market data ticks.");
    }
}

void CMainFrame::RefreshEmail() {
    if (m_username.IsEmpty() || !m_connected) return;
    CString cmd;
    cmd.Format("GET_EMAIL %s", (LPCSTR)m_username);
    std::vector<std::string> lines;
    if (!SendCommand((LPCSTR)cmd, false, lines) || lines.empty() || !IsOk(lines)) {
        m_email.Empty();
        UpdateButtonState();
        return;
    }

    std::string line = lines.front();
    if (line.size() > 3) {
        m_email = line.substr(3).c_str();
    } else {
        m_email.Empty();
    }
    UpdateButtonState();
}

void CMainFrame::AddAlertLine(const std::string& line) {
    auto t = Split(line);
    if (t.size() < 9) return;

    int row = m_list.InsertItem(m_list.GetItemCount(), t[1].c_str());
    m_list.SetItemText(row, 1, t[2] == "0" ? "Price" : "Time");
    m_list.SetItemText(row, 2, t[3].c_str());
    m_list.SetItemText(row, 3, FormatPriceText(t[4]).c_str());
    m_list.SetItemText(row, 4, "-");
    std::string condText = (t[5] == "1") ? "<=" : ">=";
    m_list.SetItemText(row, 5, condText.c_str());
    m_list.SetItemText(row, 6, t[6] == "-" ? "" : t[6].c_str());
    m_list.SetItemText(row, 7, StatusName(t[7]).c_str());
    std::string created = t[8];
    if (t.size() > 9) created += " " + t[9];
    m_list.SetItemText(row, 8, created.c_str());
}

std::string CMainFrame::StatusName(const std::string& s) const {
    if (s == "0") return "Pending";
    if (s == "1") return "Triggered";
    if (s == "2") return "Deleted";
    return s;
}

std::string CMainFrame::FormatPriceText(const std::string& raw) const {
    if (raw.empty() || raw == "-") return "-";
    char* end = nullptr;
    double value = std::strtod(raw.c_str(), &end);
    if (end == raw.c_str() || *end != '\0') return raw;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << value;
    return oss.str();
}

bool CMainFrame::ValidateContract(const CString& contract) {
    if (contract.IsEmpty()) {
        AfxMessageBox("Please enter a contract code.");
        return false;
    }
    for (int i = 0; i < contract.GetLength(); ++i) {
        unsigned char ch = static_cast<unsigned char>(contract[i]);
        if (!std::isalnum(ch) && ch != '_' && ch != '-') {
            AfxMessageBox("Contract code can only contain letters, numbers, '_' or '-'.");
            return false;
        }
    }
    return true;
}

bool CMainFrame::ValidatePrice(const CString& price) {
    if (price.IsEmpty()) {
        AfxMessageBox("Please enter a trigger price.");
        return false;
    }
    char* end = nullptr;
    double value = std::strtod((LPCSTR)price, &end);
    if (end == (LPCSTR)price || *end != '\0' || value <= 0.0) {
        AfxMessageBox("Price must be a positive number.");
        return false;
    }
    return true;
}

bool CMainFrame::ValidateAlertId(const CString& id) {
    if (id.IsEmpty()) {
        AfxMessageBox("Please select or enter an alert ID.");
        return false;
    }
    for (int i = 0; i < id.GetLength(); ++i) {
        unsigned char ch = static_cast<unsigned char>(id[i]);
        if (!std::isdigit(ch)) {
            AfxMessageBox("Alert ID must be a number.");
            return false;
        }
    }
    return true;
}

bool CMainFrame::ValidateTimeText(const CString& timeText) {
    if (timeText.GetLength() != 8 || timeText[2] != ':' || timeText[5] != ':') {
        AfxMessageBox("Time must use HH:MM:SS format, for example 09:30:00.");
        return false;
    }
    for (int i = 0; i < timeText.GetLength(); ++i) {
        if (i == 2 || i == 5) continue;
        unsigned char ch = static_cast<unsigned char>(timeText[i]);
        if (!std::isdigit(ch)) {
            AfxMessageBox("Time must use HH:MM:SS format, for example 09:30:00.");
            return false;
        }
    }
    int hour = atoi(std::string((LPCSTR)timeText).substr(0, 2).c_str());
    int min = atoi(std::string((LPCSTR)timeText).substr(3, 2).c_str());
    int sec = atoi(std::string((LPCSTR)timeText).substr(6, 2).c_str());
    if (hour > 23 || min > 59 || sec > 59) {
        AfxMessageBox("Time must be in the valid range 00:00:00 to 23:59:59.");
        return false;
    }
    return true;
}

void CMainFrame::RunSimpleCommand(const CString& cmd, const char* successText) {
    if (!EnsureLoggedIn()) return;
    std::vector<std::string> lines;
    if (SendCommand((LPCSTR)cmd, false, lines) && IsOk(lines)) {
        SetStatus(successText);
        ShowToast(successText);
    } else {
        ShowCommandError(lines, "Command failed.");
    }
}

bool CMainFrame::SendCommand(const std::string& cmd, bool expectEnd, std::vector<std::string>& lines) {
    ClearPendingResponses();
    if (!m_connected || !m_client.SendLine(cmd)) return false;

    DWORD deadline = GetTickCount() + 6000;
    while (true) {
        DWORD now = GetTickCount();
        if (now >= deadline) return false;
        std::string line;
        if (m_recvThread) {
            if (!ReadResponseLine(line, deadline - now)) return false;
        } else {
            if (!m_client.RecvLine(line)) return false;
        }
        lines.push_back(line);

        if (line == RESP_END) return true;
        if (line.rfind(RESP_ERR, 0) == 0) return true;
        if (!expectEnd && (line.rfind(RESP_OK, 0) == 0 || line.rfind(RESP_ERR, 0) == 0)) {
            return true;
        }
    }
}

void CMainFrame::ClearPendingResponses() {
    std::lock_guard<std::mutex> lk(m_responseMtx);
    while (!m_responseQ.empty()) m_responseQ.pop();
}

bool CMainFrame::ReadResponseLine(std::string& line, DWORD timeoutMs) {
    std::unique_lock<std::mutex> lk(m_responseMtx);
    bool ok = m_responseCV.wait_for(
        lk, std::chrono::milliseconds(timeoutMs),
        [this] { return !m_responseQ.empty(); });
    if (!ok) return false;
    line = m_responseQ.front();
    m_responseQ.pop();
    return true;
}

bool CMainFrame::IsOk(const std::vector<std::string>& lines) {
    return !lines.empty() && lines.front().rfind(RESP_OK, 0) == 0;
}

void CMainFrame::ShowCommandError(const std::vector<std::string>& lines, const char* fallback) {
    CString msg = fallback;
    if (!lines.empty()) msg = lines.front().c_str();
    SetStatus((LPCSTR)msg);
    AfxMessageBox(msg, MB_ICONWARNING);
}

bool CMainFrame::EnsureLoggedIn() {
    if (!m_connected || m_username.IsEmpty()) {
        AfxMessageBox("Please login first.");
        return false;
    }
    return true;
}

bool CMainFrame::EnsureConnected() {
    if (m_connected) return true;

    SetStatus("Connecting to server...");
    if (!m_client.Connect(SERVER_HOST, SERVER_PORT)) {
        SetStatus("Connect failed. Please start Server.exe first.");
        return false;
    }
    m_connected = true;
    m_closing = false;
    m_connectionInfo.SetWindowText("Server: 127.0.0.1:8888 | Connected");
    UpdateButtonState();
    return true;
}

bool CMainFrame::StartReceiver() {
    if (m_recvThread) return true;
    if (!m_connected) return false;

    m_closing = false;
    m_recvThread = CreateThread(nullptr, 0, RecvThreadProc, this, 0, nullptr);
    return m_recvThread != nullptr;
}

void CMainFrame::UpdateButtonState() {
    BOOL loggedIn = m_connected && !m_username.IsEmpty();

    m_setEmailBtn.EnableWindow(loggedIn);
    m_addPriceBtn.EnableWindow(loggedIn && !m_editMode);
    m_addTimeBtn.EnableWindow(FALSE);
    m_modPriceBtn.EnableWindow(loggedIn && m_editMode);
    m_modTimeBtn.EnableWindow(loggedIn && m_editMode);
    m_deleteBtn.EnableWindow(loggedIn && m_editMode);
    m_contractPickBtn.EnableWindow(loggedIn);
    m_deletedViewBtn.EnableWindow(loggedIn);
    m_deletedViewBtn.SetWindowText(m_showDeletedOnly ? "Active List" : "Deleted Only");
    m_refreshBtn.EnableWindow(loggedIn);
    m_logoutBtn.EnableWindow(loggedIn);

    if (loggedIn) {
        CString userInfo;
        userInfo.Format("Current user: %s", (LPCSTR)m_username);
        m_userInfo.SetWindowText(userInfo);
        CString emailInfo;
        emailInfo.Format("Email: %s", m_email.IsEmpty() ? "-" : (LPCSTR)m_email);
        m_emailInfo.SetWindowText(emailInfo);
        m_connectionInfo.SetWindowText("Server: 127.0.0.1:8888 | Online");
    } else {
        m_userInfo.SetWindowText("Not logged in");
        m_emailInfo.SetWindowText("Email: -");
        m_connectionInfo.SetWindowText(m_connected
            ? "Server: 127.0.0.1:8888 | Connected"
            : "Server: 127.0.0.1:8888 | Offline");
    }
}

void CMainFrame::SetStatus(const std::string& text) {
    if (::IsWindow(m_statusText.GetSafeHwnd())) m_statusText.SetWindowText(text.c_str());
}

void CMainFrame::ShowToast(const std::string& text) {
    if (!::IsWindow(m_toastText.GetSafeHwnd())) return;
    m_toastText.SetWindowText(text.c_str());
    m_toastText.ShowWindow(SW_SHOW);
    m_toastText.BringWindowToTop();
    KillTimer(TIMER_TOAST);
    SetTimer(TIMER_TOAST, 2500, nullptr);
}

void CMainFrame::StopConnection() {
    m_closing = true;
    if (m_connected) m_client.Close();
    if (m_recvThread) {
        WaitForSingleObject(m_recvThread, 1500);
        CloseHandle(m_recvThread);
        m_recvThread = nullptr;
    }
    m_connected = false;
}

DWORD WINAPI CMainFrame::RecvThreadProc(LPVOID p) {
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
    if (!self->m_closing) {
        self->PostMessage(WM_APP_DISCONNECTED, 0, 0);
    }
    return 0;
}

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_CLOSE()
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_BTN_SET_EMAIL, &CMainFrame::OnSetEmail)
    ON_BN_CLICKED(IDC_BTN_LOGOUT, &CMainFrame::OnLogout)
    ON_BN_CLICKED(IDC_BTN_ADD_PRICE, &CMainFrame::OnAddPrice)
    ON_BN_CLICKED(IDC_BTN_ADD_TIME, &CMainFrame::OnAddTime)
    ON_BN_CLICKED(IDC_BTN_MOD_PRICE, &CMainFrame::OnModifyPrice)
    ON_BN_CLICKED(IDC_BTN_CANCEL_EDIT, &CMainFrame::OnModifyTime)
    ON_BN_CLICKED(IDC_BTN_DELETE, &CMainFrame::OnDeleteAlert)
    ON_BN_CLICKED(IDC_BTN_REFRESH, &CMainFrame::OnRefresh)
    ON_BN_CLICKED(IDC_BTN_VIEW_DELETED, &CMainFrame::OnViewDeleted)
    ON_BN_CLICKED(IDC_BTN_PICK_CONTRACT, &CMainFrame::OnPickContract)
    ON_CBN_SELCHANGE(IDC_COMBO_STATUS, &CMainFrame::OnStatusFilterChanged)
    ON_CBN_SELCHANGE(IDC_COMBO_ALERT_TYPE, &CMainFrame::OnAlertTypeChanged)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_ALERTS, &CMainFrame::OnListItemChanged)
    ON_MESSAGE(WM_APP_POST_LOGIN, &CMainFrame::OnPostLogin)
    ON_MESSAGE(WM_APP_TRIGGERED, &CMainFrame::OnTriggered)
    ON_MESSAGE(WM_APP_DISCONNECTED, &CMainFrame::OnDisconnected)
END_MESSAGE_MAP()
