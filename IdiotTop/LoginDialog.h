#pragma once


class CLoginDialog : public CDialogEx, public Callback<CourseHelper::LOGIN_RESULT> {
	DECLARE_DYNAMIC(CLoginDialog)

public:
	CLoginDialog(Callback<CourseHelper::OTHER_RESULT> *callback, CWnd* pParent = NULL);
	virtual ~CLoginDialog();
	afx_msg void OnBnClickedLogin();
	enum { IDD = IDD_LOGIN_DIALOG };

	virtual void onResult(CourseHelper::LOGIN_RESULT result);

protected:
	virtual void OnCancel();
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
private:
	CEdit usernameEdit;
	CEdit passwordEdit;
	CButton loginButton;
	BOOL cancelable;
	Callback<CourseHelper::OTHER_RESULT> *callback;
};
