

#include "stdafx.h"
#include "afxdialogex.h"
#include "MainDialog.h"


IMPLEMENT_DYNAMIC(CMainDialog, CDialogEx)

CMainDialog::CMainDialog(CWnd* pParent /*=NULL*/) : CDialogEx(CMainDialog::IDD, pParent) {
	loginDialog = new CLoginDialog(this, this);
}

CMainDialog::~CMainDialog(){
	delete loginButton;
}

void CMainDialog::printMessage(CString content){
	textArea.SetWindowText(content);
}

void CMainDialog::appendMessage(CString content){
	int end = textArea.GetWindowTextLength();
	textArea.SetSel(end, end);
	textArea.ReplaceSel(content);;
}

void CMainDialog::onResult(CourseHelper::OTHER_RESULT result) {
	CArray<CourseInfo *> *courseList;
	auto style = courseListCtrl.GetExtendedStyle();
	switch (result) {
	case CourseHelper::OTHER_RESULT::RESULT_OK:
		courseListCtrl.SetExtendedStyle(style | LVS_EX_FULLROWSELECT);
		loginDialog->EndModalLoop(0);
		courseListCtrl.EnableWindow();
		getAnswerLibButton.EnableWindow();
		courseList = courseHelper.getList();
		if (courseList->IsEmpty()){
			courseListCtrl.InsertColumn(0, TEXT("Note"), LVCFMT_LEFT, 0xFF);
			courseListCtrl.InsertItem(0, TEXT("No course found"));
			break;
		}

		while (courseListCtrl.DeleteColumn(0));//clear table

		courseListCtrl.InsertColumn(0, NULL, LVCFMT_CENTER, 0);
		courseListCtrl.InsertColumn(1, TEXT("�γ̴���"), LVCFMT_CENTER, 80);
		courseListCtrl.InsertColumn(2, TEXT("�γ�����"), LVCFMT_CENTER, 270);
		courseListCtrl.InsertColumn(3, TEXT("�ɼ�"), LVCFMT_CENTER, 70);
		courseListCtrl.InsertColumn(4, TEXT("ѧϰ����"), LVCFMT_CENTER, 110);
		courseListCtrl.InsertColumn(5, TEXT("״̬"), LVCFMT_CENTER, 70);
		for (int i = 0; i < courseList->GetCount(); i++) {
			courseListCtrl.InsertItem(i, (*courseList)[i]->toString());
			courseListCtrl.SetItem(i, 0, LVIF_TEXT, NULL, NULL, NULL, NULL, NULL);
			courseListCtrl.SetItem(i, 1, LVIF_TEXT, (*courseList)[i]->getId(), NULL, NULL, NULL, NULL);
			courseListCtrl.SetItem(i, 2, LVIF_TEXT, (*courseList)[i]->getName(), NULL, NULL, NULL, NULL);
			courseListCtrl.SetItem(i, 3, LVIF_TEXT, (*courseList)[i]->getScore(), NULL, NULL, NULL, NULL);
			courseListCtrl.SetItem(i, 4, LVIF_TEXT, (*courseList)[i]->getDeadline(), NULL, NULL, NULL, NULL);
			courseListCtrl.SetItem(i, 5, LVIF_TEXT, (*courseList)[i]->getStatus(), NULL, NULL, NULL, NULL);
		}
		break;
	default:
		break;
	}
}

void CMainDialog::onResult(CString result) {
	if (result == GetString(IDS_DONE)) {
		textArea.SetWindowText(result);
		startButton.SetWindowText(GetString(IDS_START));
		getAnswerLibButton.SetWindowText(GetString(IDS_START_1));
		loginButton.EnableWindow();
		getAnswerLibButton.EnableWindow();
		startButton.EnableWindow();
	} else if (result.Left(2) == TEXT("\r\n")) {
		appendMessage(result);
	} else {
		printMessage(result);
	}
}

void CMainDialog::DoDataExchange(CDataExchange* pDX){
	CDialogEx::DoDataExchange(pDX);
	hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	DDX_Control(pDX, IDC_PROGRESS_TEXT, textArea);
	DDX_Control(pDX, IDC_LOGIN_BUTTON, loginButton);
	DDX_Control(pDX, IDC_START_BUTTON, startButton);
	DDX_Control(pDX, IDC_GET_ANSWERS_BUTTON, getAnswerLibButton);
	DDX_Control(pDX, IDC_ABOUT_AUTHOR_BUTTON, aboutAuthorButton);
	DDX_Control(pDX, IDC_COURSE_LIST, courseListCtrl);
	textArea.SetWindowText(GetString(IDS_TIPS));
	addToTray();
	updateTrayTip();
	courseListCtrl.EnableWindow(FALSE);
}

BEGIN_MESSAGE_MAP(CMainDialog, CDialogEx)
	ON_BN_CLICKED(IDC_LOGIN_BUTTON, &CMainDialog::OnBnClickedLogin)
	ON_BN_CLICKED(IDC_START_BUTTON, &CMainDialog::OnBnClickedStart)
	ON_BN_CLICKED(IDC_GET_ANSWERS_BUTTON, &CMainDialog::OnBnClickedGetAnswers)
	ON_BN_CLICKED(IDC_ABOUT_AUTHOR_BUTTON, &CMainDialog::OnBnClickedAboutAuthor)
	ON_MESSAGE(IDM_TRAY, OnTrayMessage)
	ON_NOTIFY(NM_CLICK, IDC_COURSE_LIST, &CMainDialog::OnNMClickCourseList)
END_MESSAGE_MAP()

void CMainDialog::OnBnClickedLogin() {
	loginDialog->DoModal();
}

void CMainDialog::OnBnClickedStart() {
	if (courseHelper.getStatus() != CourseHelper::STATUS::STATUS_ANSWER && courseHelper.getStatus() != CourseHelper::STATUS::STATUS_VIDEO) {
		loginButton.EnableWindow(FALSE);
		getAnswerLibButton.EnableWindow(FALSE);
		startButton.SetWindowText(GetString(IDS_STOP));
		courseHelper.answerCourse(this, itemId);
		textArea.SetWindowText(TEXT(""));
	} else {
		courseHelper.cancelAnswerCourse();
		textArea.SetWindowText(TEXT("��ֹͣˢ��"));
		startButton.SetWindowText(GetString(IDS_START));
		loginButton.EnableWindow();
		getAnswerLibButton.EnableWindow();
	}
}

void CMainDialog::OnBnClickedGetAnswers() {
	if (courseHelper.getStatus() != CourseHelper::STATUS::STATUS_LIBRARY) {
		loginButton.EnableWindow(FALSE);
		startButton.EnableWindow(FALSE);
		getAnswerLibButton.SetWindowText(GetString(IDS_STOP));
		courseHelper.getAllAnswerLib(this);
		textArea.SetWindowText(TEXT(""));
	} else {
		courseHelper.cancelGetAnswerLib();
		textArea.SetWindowText(TEXT("��ֹͣ��ȡ���"));
		getAnswerLibButton.SetWindowText(GetString(IDS_START_1));
		loginButton.EnableWindow();
		startButton.EnableWindow();
	}
}

void CMainDialog::OnBnClickedAboutAuthor() {
	aboutDialog.DoModal();
}

void CMainDialog::OnCancel(){
	hideWindow();
}

LRESULT CMainDialog::OnTrayMessage(WPARAM wParam, LPARAM lParam) {
	if (wParam != IDM_TRAY){
		return 1;
	}
	CMenu menu;
	CPoint p;
	int result;
	switch (lParam) {
	case WM_RBUTTONUP:
		GetCursorPos(&p);
		menu.CreatePopupMenu();
		menu.AppendMenu(MF_STRING, WM_SHOWWINDOW, GetString(IDS_OPEN));
		menu.AppendMenu(MF_STRING, WM_DESTROY, GetString(IDS_EXIT));
		SetForegroundWindow();//SetForegroundWindow before TrackPopupMenu makes menu can be cancel when clicking outside
		result = menu.TrackPopupMenu(TPM_RETURNCMD, p.x, p.y, this);
		switch (result) {
		case WM_SHOWWINDOW:
			showWindow();
			break;
		case WM_DESTROY:
			DestroyWindow();
			break;
		}
		menu.Detach();
		menu.DestroyMenu();
		break;
	case WM_LBUTTONDBLCLK:
		showWindow();
		break;
	case WM_MOUSEFIRST:
		updateTrayTip();
		break;
	}
	return 0;
}

void CMainDialog::showWindow() {
	ShowWindow(SW_SHOW);
}

void CMainDialog::hideWindow(){
	ShowWindow(SW_HIDE);
}

void CMainDialog::addToTray(){
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = m_hWnd;
	nid.uID = IDM_TRAY;
	nid.hIcon = hIcon;
	nid.uCallbackMessage = IDM_TRAY;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	Shell_NotifyIcon(NIM_ADD, &nid);
}

void CMainDialog::updateTrayTip() {
	if (courseHelper.isStatusChanged()){
		StrCpy(nid.szTip, GetString(courseHelper.getStatus()));
		Shell_NotifyIcon(NIM_MODIFY, &nid);
		courseHelper.resetStatusChanged();
	}
}

BOOL CMainDialog::DestroyWindow(){
	Shell_NotifyIcon(NIM_DELETE, &nid);
	return CDialogEx::DestroyWindow();
}



void CMainDialog::OnNMClickCourseList(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;
	if (pNMItemActivate->iItem == -1) {
		if (startButton.IsWindowEnabled()) {
			startButton.EnableWindow(FALSE);
		}
	} else {
		if (!startButton.IsWindowEnabled()) {
			startButton.EnableWindow();
		}
	}
	itemId = pNMItemActivate->iItem;
}
