#pragma once

#include "Resource.h" 
#include "CourseHelper.h"   
#include "Func.h"
#include "Callback.h"

class CIdiotTopApp : public CWinApp {
public:
	CIdiotTopApp();

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

};

extern CourseHelper courseHelper;

extern CIdiotTopApp theApp;
