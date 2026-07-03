// ChangeEmailDlg.cpp - password-confirmed email update dialog.

#include "ChangeEmailDlg.h"
#include "MainFrame.h"

#include <cwchar>

enum ChangeEmailControlId {
    IDC_EMAIL_PASSWORD = 2401,
    IDC_EMAIL_NEW,
    IDC_EMAIL_SUBMIT,
    IDC_EMAIL_STATUS
};

CChangeEmailDlg::CChangeEmailDlg(CMainFrame* mainFrame)
    : CDialog(), m_mainFrame(mainFrame) {
    BuildTemplate();
    InitModalIndirect(&m_template.dlg);
}

void CChangeEmailDlg::BuildTemplate() {
    ZeroMemory(&m_template, sizeof(m_template));
    m_template.dlg.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME;
    m_template.dlg.cdit = 0;
    m_template.dlg.cx = 245;
    m_template.dlg.cy = 130;
    wcscpy_s(m_template.title, L"ChangeEmail");
}

BOOL CChangeEmailDlg::OnInitDialog() {
    CDialog::OnInitDialog();
    SetWindowText("Change Email");

    m_titleFont.CreatePointFont(155, "Segoe UI");
    m_title.Create("Change Email", WS_CHILD | WS_VISIBLE, CRect(24, 18, 330, 52), this);
    m_title.SetFont(&m_titleFont);

    m_passwordLabel.Create("Password", WS_CHILD | WS_VISIBLE, CRect(24, 72, 120, 96), this);
    m_emailLabel.Create("New email", WS_CHILD | WS_VISIBLE, CRect(24, 108, 120, 132), this);
    m_passwordEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD,
                          CRect(126, 70, 360, 96), this, IDC_EMAIL_PASSWORD);
    m_emailEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                       CRect(126, 106, 360, 132), this, IDC_EMAIL_NEW);

    m_submitBtn.Create("Confirm", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_OWNERDRAW,
                       CRect(126, 154, 220, 188), this, IDC_EMAIL_SUBMIT);
    m_submitBtn.SetColors(RGB(32, 113, 214), RGB(255, 255, 255), RGB(22, 88, 170));
    m_cancelBtn.Create("Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                       CRect(232, 154, 310, 188), this, IDCANCEL);
    m_cancelBtn.SetColors(RGB(246, 247, 249), RGB(60, 66, 76), RGB(172, 178, 188));

    m_statusText.Create("Enter your password to confirm this change.",
                        WS_CHILD | WS_VISIBLE, CRect(24, 206, 360, 230), this, IDC_EMAIL_STATUS);

    SetWindowPos(nullptr, 0, 0, 410, 285, SWP_NOMOVE | SWP_NOZORDER);
    CenterWindow();
    return TRUE;
}

void CChangeEmailDlg::SetStatusText(const std::string& text) {
    if (::IsWindow(m_statusText.GetSafeHwnd())) m_statusText.SetWindowText(text.c_str());
}

void CChangeEmailDlg::OnSubmit() {
    CString password, email;
    m_passwordEdit.GetWindowText(password);
    m_emailEdit.GetWindowText(email);
    password.Trim();
    email.Trim();

    if (password.IsEmpty() || email.IsEmpty()) {
        SetStatusText("Password and new email are required.");
        return;
    }
    if (email.Find("@") < 0) {
        SetStatusText("Please enter a valid email address.");
        return;
    }

    std::string error;
    bool ok = m_mainFrame->ChangeEmail(password, email, error);
    m_passwordEdit.SetWindowText("");
    if (ok) {
        EndDialog(IDOK);
    } else {
        SetStatusText(error);
    }
}

BEGIN_MESSAGE_MAP(CChangeEmailDlg, CDialog)
    ON_BN_CLICKED(IDC_EMAIL_SUBMIT, &CChangeEmailDlg::OnSubmit)
END_MESSAGE_MAP()
