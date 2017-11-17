// LoginDialog.cpp : 实现文件
//

#include "stdafx.h"
#include "IdiotTop.h"
#include "LoginDialog.h"
#include "afxdialogex.h"


IMPLEMENT_DYNAMIC(CLoginDialog, CDialogEx)

CLoginDialog::CLoginDialog(Callback<CourseHelper::OTHER_RESULT> *callback, CWnd* pParent /*=NULL*/) : CDialogEx(CLoginDialog::IDD, pParent) {
	this->callback = callback;
	cancelable = TRUE;
}

CLoginDialog::~CLoginDialog() {
}

void CLoginDialog::DoDataExchange(CDataExchange* pDX) {
	DDX_Control(pDX, IDC_EDIT_USERNAME, usernameEdit);
	DDX_Control(pDX, IDC_EDIT_PASSWORD, passwordEdit);
	DDX_Control(pDX, IDC_LOGIN_BUTTON, loginButton);
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CLoginDialog, CDialogEx)
	ON_BN_CLICKED(IDC_LOGIN_BUTTON, &CLoginDialog::OnBnClickedLogin)
END_MESSAGE_MAP()

void CLoginDialog::OnBnClickedLogin(){
	CString username;
	CString password;
	usernameEdit.GetWindowText(username);
	passwordEdit.GetWindowText(password);
	if (username.IsEmpty() || password.IsEmpty()) {
		MessageBox(GetString(IDS_CANNOT_EMPTY));
	} else {
		usernameEdit.EnableWindow(FALSE);
		passwordEdit.EnableWindow(FALSE);
		loginButton.EnableWindow(FALSE);
		loginButton.SetWindowText(GetString(IDS_LOGINING));
		courseHelper.login(username, password, this);
	}
	cancelable = FALSE;
}


void CLoginDialog::onResult(CourseHelper::LOGIN_RESULT result) {
	cancelable = TRUE;
	switch (result) {
	case CourseHelper::LOGIN_RESULT::RESULT_LOGIN_OK:
		courseHelper.refreshCourse(callback);
		//EndDialog(0);
		return;
	case CourseHelper::LOGIN_RESULT::RESULT_LOGIN_FAILT:
		MessageBox(GetString(IDS_LOGIN_FAIL));
		break;
	case CourseHelper::LOGIN_RESULT::RESULT_NETWORK_FAIL:
		MessageBox(GetString(IDS_LOGIN_NETWORK_ERROR));
		break;
	case CourseHelper::LOGIN_RESULT::RESULT_NO_RETRY:
		MessageBox(GetString(IDS_LOGIN_ERROR));
		break;
	default:
		break;
	}
	usernameEdit.EnableWindow(TRUE);
	passwordEdit.EnableWindow(TRUE);
	loginButton.EnableWindow(TRUE);
	loginButton.SetWindowText(GetString(IDS_LOGIN));
}

void CLoginDialog::OnCancel() {
	if (cancelable) {
		CDialog::OnCancel();
	}
}
