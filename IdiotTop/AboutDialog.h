#pragma once

#include "IdiotTop.h"

class CAboutDialog : public CDialogEx {
	DECLARE_DYNAMIC(CAboutDialog)

public:
	CAboutDialog(CWnd* pParent = NULL);
	virtual ~CAboutDialog();
	enum { IDD = IDD_BOX_ABOUT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
};
