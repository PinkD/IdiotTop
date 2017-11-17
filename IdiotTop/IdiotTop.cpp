
#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "IdiotTop.h"
#include "MainDialog.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CIdiotTopApp::CIdiotTopApp() {
	SetAppID(_T("IdiotTop.AppID.InitVersion"));
}

CourseHelper courseHelper;
CIdiotTopApp theApp;

BOOL CIdiotTopApp::InitInstance() {
	AfxEnableControlContainer();
	CWinApp::InitInstance();
	SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));
	CMainDialog mainDialog;
	m_pMainWnd = &mainDialog;
	mainDialog.DoModal();//ignore result
	return FALSE;
}

int CIdiotTopApp::ExitInstance() {
	clearBackupFile(TEXT("BinaryAnswerLibrary\\"));
	clearBackupFile(TEXT("TextAnswerLibrary\\"));
	return CWinApp::ExitInstance();
}
