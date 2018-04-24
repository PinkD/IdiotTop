#pragma once

#include "afxwin.h"
#include "Callback.h"
#include "IdiotTop.h"
#include "LoginDialog.h"
#include "AboutDialog.h"
#include "CourseHelper.h"

#define IDM_TRAY WM_USER + 1

class CMainDialog : public CDialogEx, public Callback<CourseHelper::OTHER_RESULT> , public Callback<CString> {
	DECLARE_DYNAMIC(CMainDialog)

public:
	CMainDialog(CWnd* pParent = NULL);
	virtual ~CMainDialog();
	virtual BOOL DestroyWindow();
	afx_msg void OnBnClickedLogin();
	afx_msg void OnBnClickedStart();
	afx_msg void OnBnClickedGetAnswers();
	afx_msg void OnBnClickedAboutAuthor();
	afx_msg void OnNMClickCourseList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnTrayMessage(WPARAM wParam, LPARAM lParam);
	virtual void OnCancel();
	inline void printMessage(CString content);
	inline void appendMessage(CString content);
	virtual void onResult(CourseHelper::OTHER_RESULT result);
	virtual void onResult(CString result);
	enum { IDD = IDD_MAIN_DIALOG};

protected:

	CStatic textArea;
	CButton loginButton;
	CButton startButton;
	CButton getAnswerLibButton;
	CButton aboutAuthorButton;
	CListCtrl courseListCtrl;
	HICON hIcon;
	CLoginDialog *loginDialog;
	CAboutDialog aboutDialog;

	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()

private:
	void addToTray();
	inline void updateTrayTip();
	int itemId;

	NOTIFYICONDATA nid;
	inline void showWindow();
	inline void hideWindow();
};
