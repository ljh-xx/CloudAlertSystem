// LoginDlg.cpp - startup login/register dialog.

#include "LoginDlg.h"
#include "MainFrame.h"
#include "RegisterDlg.h"

#include <cwchar>

enum LoginControlId {
    IDC_LOGIN_USER = 2001,
    IDC_LOGIN_PASSWORD,
    IDC_LOGIN_BUTTON,
    IDC_REGISTER_BUTTON,
    IDC_LOGIN_STATUS
};

CLoginDlg::CLoginDlg(CMainFrame* mainFrame)
    : CDialog(), m_mainFrame(mainFrame) {
    BuildTemplate();
    InitModalIndirect(&m_template.dlg);
}

void CLoginDlg::BuildTemplate() {
    ZeroMemory(&m_template, sizeof(m_template));
    m_template.dlg.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME;
    m_template.dlg.dwExtendedStyle = 0;
    m_template.dlg.cdit = 0;
    m_template.dlg.x = 0;
    m_template.dlg.y = 0;
    m_template.dlg.cx = 230;
    m_template.dlg.cy = 120;
    wcscpy_s(m_template.title, L"Login");
}

BOOL CLoginDlg::OnInitDialog() {
    CDialog::OnInitDialog();
    SetWindowText("Cloud Alert Login");

    m_titleFont.CreatePointFont(170, "Segoe UI");
    m_title.Create("Cloud Alert System", WS_CHILD | WS_VISIBLE, CRect(24, 20, 340, 54), this);
    m_title.SetFont(&m_titleFont);

    m_userLabel.Create("Username", WS_CHILD | WS_VISIBLE, CRect(24, 72, 108, 96), this);
    m_passwordLabel.Create("Password", WS_CHILD | WS_VISIBLE, CRect(24, 108, 108, 132), this);

    m_userEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                      CRect(116, 70, 340, 96), this, IDC_LOGIN_USER);
    m_passwordEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD,
                          CRect(116, 106, 340, 132), this, IDC_LOGIN_PASSWORD);

    m_loginBtn.Create("Login", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_OWNERDRAW,
                      CRect(116, 154, 184, 186), this, IDC_LOGIN_BUTTON);
    m_loginBtn.SetColors(RGB(32, 113, 214), RGB(255, 255, 255), RGB(22, 88, 170));
    m_registerBtn.Create("Register", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                         CRect(194, 154, 274, 186), this, IDC_REGISTER_BUTTON);
    m_registerBtn.SetColors(RGB(232, 245, 238), RGB(20, 100, 64), RGB(116, 180, 144));
    m_cancelBtn.Create("Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                       CRect(284, 154, 340, 186), this, IDCANCEL);
    m_cancelBtn.SetColors(RGB(246, 247, 249), RGB(60, 66, 76), RGB(172, 178, 188));

    m_statusText.Create("Login with username and password.",
                        WS_CHILD | WS_VISIBLE, CRect(24, 208, 340, 232), this, IDC_LOGIN_STATUS);

    SetWindowPos(nullptr, 0, 0, 390, 285, SWP_NOMOVE | SWP_NOZORDER);
    CenterWindow();
    return TRUE;
}

void CLoginDlg::SetStatusText(const std::string& text) {
    if (::IsWindow(m_statusText.GetSafeHwnd())) m_statusText.SetWindowText(text.c_str());
}

void CLoginDlg::OnLogin() {
    CString user;
    CString password;
    m_userEdit.GetWindowText(user);
    m_passwordEdit.GetWindowText(password);
    user.Trim();
    password.Trim();

    if (user.IsEmpty() || password.IsEmpty()) {
        SetStatusText("Username and password are required.");
        return;
    }

    std::string error;
    bool ok = m_mainFrame->LoginUser(user, password, error);
    m_passwordEdit.SetWindowText("");
    if (ok) {
        EndDialog(IDOK);
    } else {
        SetStatusText(error);
    }
}

void CLoginDlg::OnRegister() {
    m_passwordEdit.SetWindowText("");
    CRegisterDlg dlg(m_mainFrame);
    if (dlg.DoModal() == IDOK) {
        SetStatusText("Registration succeeded. Please login.");
    }
}

BEGIN_MESSAGE_MAP(CLoginDlg, CDialog)
    ON_BN_CLICKED(IDC_LOGIN_BUTTON, &CLoginDlg::OnLogin)
    ON_BN_CLICKED(IDC_REGISTER_BUTTON, &CLoginDlg::OnRegister)
END_MESSAGE_MAP()
