#pragma once

#include <afxwin.h>
#include <string>

#include "StyledButton.h"

class CMainFrame;

struct RegisterDialogTemplate {
    DLGTEMPLATE dlg;
    WORD menu;
    WORD windowClass;
    WCHAR title[9];
};

class CRegisterDlg : public CDialog {
public:
    explicit CRegisterDlg(CMainFrame* mainFrame);

protected:
    CMainFrame* m_mainFrame;
    RegisterDialogTemplate m_template;
    CFont m_titleFont;
    CStatic m_title;
    CStatic m_userLabel;
    CStatic m_passwordLabel;
    CStatic m_confirmLabel;
    CStatic m_emailLabel;
    CStatic m_statusText;
    CEdit m_userEdit;
    CEdit m_passwordEdit;
    CEdit m_confirmEdit;
    CEdit m_emailEdit;
    CStyledButton m_submitBtn;
    CStyledButton m_cancelBtn;

    BOOL OnInitDialog() override;
    void BuildTemplate();
    void SetStatusText(const std::string& text);

    afx_msg void OnSubmit();

    DECLARE_MESSAGE_MAP()
};
