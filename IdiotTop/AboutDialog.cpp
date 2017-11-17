
#include "stdafx.h"
#include "afxdialogex.h"
#include "AboutDialog.h"


IMPLEMENT_DYNAMIC(CAboutDialog, CDialogEx)

CAboutDialog::CAboutDialog(CWnd* pParent /*=NULL*/) : CDialogEx(IDD_BOX_ABOUT, pParent) {

}

CAboutDialog::~CAboutDialog(){
}

void CAboutDialog::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CAboutDialog, CDialogEx)
END_MESSAGE_MAP()


