#pragma once

#include <afxwin.h>
#include <string>

#include "StyledButton.h"

class CMainFrame;

struct ChangeEmailDialogTemplate {
    DLGTEMPLATE dlg;
    WORD menu;
    WORD windowClass;
    WCHAR title[13];
};

class CChangeEmailDlg : public CDialog {
public:
    explicit CChangeEmailDlg(CMainFrame* mainFrame);

protected:
    CMainFrame* m_mainFrame;
    ChangeEmailDialogTemplate m_template;
    CFont m_titleFont;
    CStatic m_title;
    CStatic m_passwordLabel;
    CStatic m_emailLabel;
    CStatic m_statusText;
    CEdit m_passwordEdit;
    CEdit m_emailEdit;
    CStyledButton m_submitBtn;
    CStyledButton m_cancelBtn;

    BOOL OnInitDialog() override;
    void BuildTemplate();
    void SetStatusText(const std::string& text);

    afx_msg void OnSubmit();

    DECLARE_MESSAGE_MAP()
};
