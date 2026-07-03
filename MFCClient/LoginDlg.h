#pragma once

#include <afxwin.h>
#include <string>

#include "StyledButton.h"

class CMainFrame;

struct LoginDialogTemplate {
    DLGTEMPLATE dlg;
    WORD menu;
    WORD windowClass;
    WCHAR title[6];
};

class CLoginDlg : public CDialog {
public:
    explicit CLoginDlg(CMainFrame* mainFrame);

protected:
    CMainFrame* m_mainFrame;
    LoginDialogTemplate m_template;
    CFont m_titleFont;
    CStatic m_title;
    CStatic m_userLabel;
    CStatic m_passwordLabel;
    CStatic m_statusText;
    CEdit m_userEdit;
    CEdit m_passwordEdit;
    CStyledButton m_loginBtn;
    CStyledButton m_registerBtn;
    CStyledButton m_cancelBtn;

    BOOL OnInitDialog() override;
    void BuildTemplate();
    void SetStatusText(const std::string& text);

    afx_msg void OnLogin();
    afx_msg void OnRegister();

    DECLARE_MESSAGE_MAP()
};
