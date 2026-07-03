// MFCClient.cpp - application entry for Cloud Alert System.

#include <afxwin.h>
#include <afxcmn.h>

#include "MainFrame.h"

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

        if (!frame->ShowLoginDialog()) {
            frame->DestroyWindow();
            return FALSE;
        }

        frame->ShowWindow(SW_SHOW);
        frame->UpdateWindow();
        return TRUE;
    }
};

CMFCClientApp theApp;
