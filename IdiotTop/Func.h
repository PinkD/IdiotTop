
#ifndef FUNC_H
#define FUNC_H

#include <afx.h>
#include<time.h>
#include<sys/timeb.h>

CString SubString(CString str, CString start, CString end);
CString SubString(CString str, CString start, CString end, int *offset);
int GetRandNumberBetween(int m, int n);
CString GetRandNumberStringBetween(int m, int n);
double GetRandZeroToOne();
CString GetString(DWORD id);
CString GetRandLetterString(int n);
long long getCurrentTimestamp();
CString readAnswerFromFile(CString path);
BOOL writeAnswerToFile(CString path, CString Answer);
BOOL backupFile(CString path);
BOOL commitFile(CString path);
BOOL restoreFile(CString path);
BOOL clearBackupFile(CString path);
BOOL createDirIfNotExists(CString path);

#endif