#include "stdafx.h"
#include "Func.h"
#include "Code.h"

CString GetRandLetterString(int n) {
	int i;
	CString str;
	char c;
	int t;
	for (i = 0; i < n; i++) {
		t = rand() % 3;
		if (t > 1) {
			t = 'a';
		}
		else {
			t = 'A';
		}
		c = t + rand() % 26;
		str = str + c;
	}
	return str;
}

int GetRandNumberBetween(int m, int n) {
	return rand() % (n - m + 1) + m;
}

double GetRandZeroToOne() {
	return  (rand() % 10) / (float)10;
}

CString GetRandNumberStringBetween(int m, int n) {
	CString str;
	str.Format(TEXT("%d"), GetRandNumberBetween(m, n));
	return str;
}

CString GetString(DWORD id) {//Load string from resource
	CString s;
	s.LoadString(id);
	return s;
}

CString SubString(CString str, CString start, CString end) {
	int iPos = str.Find(start);
	if (iPos == -1) {
		return TEXT("");
	}
	iPos += start.GetLength();
	int iPos1 = str.Find(end, iPos);
	if (iPos1 == -1) {
		return TEXT("");
	}
	return str.Mid(iPos, iPos1 - iPos);
}

CString SubString(CString str, CString start, CString end, int *offset) {
	int iPos = str.Find(start, *offset);
	if (iPos == -1) {
		return TEXT("");
	}
	iPos += start.GetLength();
	int iPos1 = str.Find(end, iPos);
	if (iPos1 == -1) {
		return TEXT("");
	}
	*offset = iPos1;
	return str.Mid(iPos, iPos1 - iPos);
}

long long getCurrentTimestamp() {
	long long time_last;
	time_last = time(NULL);     //总秒数  
	struct timeb t1;
	ftime(&t1);
	//CString strTime;
	//strTime.Format(_T("%lldms"), t1.time * 1000 + t1.millitm);  //总毫秒数  
	//OutputDebugString(strTime);
	return t1.time * 1000 + t1.millitm;
}


CString readAnswerFromFile(CString path) {
	CStdioFile File;
	CString result;
	if (File.Open(path, CFile::modeNoTruncate | CFile::typeBinary | CFile::modeRead | CFile::shareDenyNone)) {
		ULONGLONG count = File.GetLength();
		char *p = new char[count + 1];
		memset(p, 0, count + 1);
		File.Read(p, count);
		result = p;
		delete[] p;
		File.Close();
	}
	return result;
}

BOOL writeAnswerToFile(CString path, CString answer) {
	CStdioFile file;
	if (file.Open(path, CFile::modeNoTruncate | CFile::modeCreate | CFile::typeBinary | CFile::modeReadWrite | CFile::shareDenyNone)) {
		file.SeekToEnd();
		char *p = UnicodeToANSI(answer);
		file.Write(p, strlen(p));
		delete[] p;
		file.Close();
		return TRUE;
	}
	return FALSE;
}

BOOL backupFile(CString path) {//mv file file.bak
	return MoveFile(path, path + TEXT(".bak"));
}

BOOL commitFile(CString path) {//mv file.bak file.old
	return MoveFile(path + TEXT(".bak"), path + TEXT(".old"));
}

BOOL restoreFile(CString path) {//mv file.bak file
	return MoveFile(path + TEXT(".bak"), path);
}

BOOL clearBackupFile(CString path) {//rm ./*.bak 
	CFileFind fileFind;
	BOOL found = fileFind.FindFile(path + TEXT("*.bak"));
	while (found) {
		found = fileFind.FindNextFile();
		if (!DeleteFile(fileFind.GetFilePath())) {
			return FALSE;
		}
	}
	return TRUE;
}

BOOL createDirIfNotExists(CString path) {
	if (PathFileExists(path)) {
		return FALSE;
	} else {
		return CreateDirectory(path, NULL);
	}
}
