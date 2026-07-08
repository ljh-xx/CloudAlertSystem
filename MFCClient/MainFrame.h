#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <afxwin.h>
#include <afxcmn.h>

#include "TcpClient.h"
#include "StyledButton.h"
#ifdef STATUS_PENDING
#undef STATUS_PENDING
#endif
#include "Protocol.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

class CMainFrame : public CFrameWnd {
public:
    CMainFrame();
    ~CMainFrame() override;

    bool ShowLoginDialog();
    bool LoginUser(const CString& user, const CString& password, std::string& error);
    bool RegisterUser(const CString& user, const CString& password, const CString& email, std::string& error);
    bool ChangeEmail(const CString& password, const CString& email, std::string& error);
    void BeginPostLoginRefresh();

protected:
    CFont m_titleFont;
    CStatic m_title;
    CStatic m_userInfo;
    CStatic m_emailInfo;
    CStatic m_connectionInfo;
    CStatic m_listTitle;
    CStyledButton m_setEmailBtn;
    CStyledButton m_logoutBtn;
    CListCtrl m_list;
    CStatic m_panelTitle;
    CStatic m_addGroupTitle;
    CStatic m_editGroupTitle;
    CStatic m_idLabel;
    CStatic m_typeLabel;
    CStatic m_contractLabel;
    CStatic m_priceLabel;
    CStatic m_condLabel;
    CStatic m_timeLabel;
    CEdit m_idEdit;
    CComboBox m_typeCombo;
    CEdit m_contractEdit;
    CStyledButton m_contractPickBtn;
    CEdit m_priceEdit;
    CComboBox m_condCombo;
    CEdit m_timeEdit;
    CStyledButton m_addPriceBtn;
    CStyledButton m_addTimeBtn;
    CStyledButton m_modPriceBtn;
    CStyledButton m_modTimeBtn;
    CStyledButton m_deleteBtn;
    CComboBox m_statusCombo;
    CStyledButton m_deletedViewBtn;
    CStyledButton m_refreshBtn;
    CStatic m_statusText;
    CStatic m_toastText;

    TcpClient m_client;
    bool m_connected;
    bool m_closing;
    bool m_editMode;
    bool m_showDeletedOnly;
    int m_editAlertType;
    CString m_username;
    CString m_email;
    HANDLE m_recvThread;

    std::queue<std::string> m_responseQ;
    std::mutex m_responseMtx;
    std::condition_variable m_responseCV;

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnSetEmail();
    afx_msg void OnLogout();
    afx_msg void OnAddPrice();
    afx_msg void OnAddTime();
    afx_msg void OnModifyPrice();
    afx_msg void OnModifyTime();
    afx_msg void OnDeleteAlert();
    afx_msg void OnRefresh();
    afx_msg void OnViewDeleted();
    afx_msg void OnStatusFilterChanged();
    afx_msg void OnPickContract();
    afx_msg void OnAlertTypeChanged();
    afx_msg void OnListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnPostLogin(WPARAM, LPARAM);
    afx_msg LRESULT OnTriggered(WPARAM, LPARAM lParam);
    afx_msg LRESULT OnDisconnected(WPARAM, LPARAM);
    afx_msg void OnClose();
    afx_msg void OnTimer(UINT_PTR idEvent);

    static void MoveLabeled(CStatic& label, CEdit& edit, int x, int y, int w);
    static void MoveLabeled(CStatic& label, CWnd& ctrl, int x, int y, int w);
    static DWORD WINAPI RecvThreadProc(LPVOID p);

    void EnterAddMode();
    void EnterEditMode(int item);
    void UpdateEditorMode();
    void UpdateEditorVisibility();
    void HideEditorControl(CWnd& ctrl);
    void RedrawEditorArea();
    int GetSelectedAlertType();
    void RefreshAlerts();
    void RefreshEmail();
    void RefreshCurrentPrices();
    void AddAlertLine(const std::string& line);
    std::string StatusName(const std::string& s) const;
    std::string FormatPriceText(const std::string& raw) const;
    bool ValidateContract(const CString& contract);
    bool ValidatePrice(const CString& price);
    bool ValidateAlertId(const CString& id);
    bool ValidateTimeText(const CString& timeText);
    void RunSimpleCommand(const CString& cmd, const char* successText);
    bool SendCommand(const std::string& cmd, bool expectEnd, std::vector<std::string>& lines);
    void ClearPendingResponses();
    bool ReadResponseLine(std::string& line, DWORD timeoutMs);
    static bool IsOk(const std::vector<std::string>& lines);
    void ShowCommandError(const std::vector<std::string>& lines, const char* fallback);
    bool EnsureLoggedIn();
    bool EnsureConnected();
    bool StartReceiver();
    void UpdateButtonState();
    void SetStatus(const std::string& text);
    void ShowToast(const std::string& text);
    void StopConnection();

    DECLARE_MESSAGE_MAP()
};
