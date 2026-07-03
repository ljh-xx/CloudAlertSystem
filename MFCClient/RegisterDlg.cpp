// RegisterDlg.cpp - separate account registration dialog.

#include "RegisterDlg.h"
#include "MainFrame.h"

#include <cwchar>

enum RegisterControlId {
    IDC_REGISTER_USER = 2201,
    IDC_REGISTER_PASSWORD,
    IDC_REGISTER_CONFIRM,
    IDC_REGISTER_EMAIL,
    IDC_REGISTER_SUBMIT,
    IDC_REGISTER_STATUS
};

CRegisterDlg::CRegisterDlg(CMainFrame* mainFrame)
    : CDialog(), m_mainFrame(mainFrame) {
    BuildTemplate();
    InitModalIndirect(&m_template.dlg);
}

void CRegisterDlg::BuildTemplate() {
    ZeroMemory(&m_template, sizeof(m_template));
    m_template.dlg.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME;
    m_template.dlg.cdit = 0;
    m_template.dlg.cx = 245;
    m_template.dlg.cy = 190;
    wcscpy_s(m_template.title, L"Register");
}

BOOL CRegisterDlg::OnInitDialog() {
    CDialog::OnInitDialog();
    SetWindowText("Cloud Alert Register");

    m_titleFont.CreatePointFont(160, "Segoe UI");
    m_title.Create("Create Account", WS_CHILD | WS_VISIBLE, CRect(24, 18, 350, 52), this);
    m_title.SetFont(&m_titleFont);

    m_userLabel.Create("Username", WS_CHILD | WS_VISIBLE, CRect(24, 68, 120, 92), this);
    m_passwordLabel.Create("Password", WS_CHILD | WS_VISIBLE, CRect(24, 104, 120, 128), this);
    m_confirmLabel.Create("Confirm", WS_CHILD | WS_VISIBLE, CRect(24, 140, 120, 164), this);
    m_emailLabel.Create("Email", WS_CHILD | WS_VISIBLE, CRect(24, 176, 120, 200), this);

    m_userEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                      CRect(126, 66, 360, 92), this, IDC_REGISTER_USER);
    m_passwordEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD,
                          CRect(126, 102, 360, 128), this, IDC_REGISTER_PASSWORD);
    m_confirmEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD,
                         CRect(126, 138, 360, 164), this, IDC_REGISTER_CONFIRM);
    m_emailEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                       CRect(126, 174, 360, 200), this, IDC_REGISTER_EMAIL);

    m_submitBtn.Create("Register", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_OWNERDRAW,
                       CRect(126, 218, 220, 252), this, IDC_REGISTER_SUBMIT);
    m_submitBtn.SetColors(RGB(32, 113, 214), RGB(255, 255, 255), RGB(22, 88, 170));
    m_cancelBtn.Create("Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                       CRect(232, 218, 310, 252), this, IDCANCEL);
    m_cancelBtn.SetColors(RGB(246, 247, 249), RGB(60, 66, 76), RGB(172, 178, 188));

    m_statusText.Create("Email will be bound to this account.",
                        WS_CHILD | WS_VISIBLE, CRect(24, 268, 360, 292), this, IDC_REGISTER_STATUS);

    SetWindowPos(nullptr, 0, 0, 410, 350, SWP_NOMOVE | SWP_NOZORDER);
    CenterWindow();
    return TRUE;
}

void CRegisterDlg::SetStatusText(const std::string& text) {
    if (::IsWindow(m_statusText.GetSafeHwnd())) m_statusText.SetWindowText(text.c_str());
}

void CRegisterDlg::OnSubmit() {
    CString user, password, confirm, email;
    m_userEdit.GetWindowText(user);
    m_passwordEdit.GetWindowText(password);
    m_confirmEdit.GetWindowText(confirm);
    m_emailEdit.GetWindowText(email);
    user.Trim();
    password.Trim();
    confirm.Trim();
    email.Trim();

    if (user.IsEmpty() || password.IsEmpty() || confirm.IsEmpty() || email.IsEmpty()) {
        SetStatusText("All fields are required.");
        return;
    }
    if (password != confirm) {
        m_passwordEdit.SetWindowText("");
        m_confirmEdit.SetWindowText("");
        SetStatusText("Passwords do not match.");
        return;
    }
    if (email.Find("@") < 0) {
        SetStatusText("Please enter a valid email address.");
        return;
    }

    std::string error;
    bool ok = m_mainFrame->RegisterUser(user, password, email, error);
    m_passwordEdit.SetWindowText("");
    m_confirmEdit.SetWindowText("");
    if (ok) {
        AfxMessageBox("Registration succeeded. Please login.", MB_ICONINFORMATION);
        EndDialog(IDOK);
    } else {
        SetStatusText(error);
    }
}

BEGIN_MESSAGE_MAP(CRegisterDlg, CDialog)
    ON_BN_CLICKED(IDC_REGISTER_SUBMIT, &CRegisterDlg::OnSubmit)
END_MESSAGE_MAP()
