/*������һЩ������뺯��*/
#ifndef CODE_H
#define CODE_H
#include <afx.h>

char * UnicodeToANSI(const wchar_t* str);
wchar_t * UTF8ToUnicode(const char* str);
char* UTF8ToANSI(const char* str);

CString HexStrToWChars(CString s);//���� \u515A\u8BFE\u6559\u7A0B -->> ���ν̳�
CString  UrlDecodeUTF8(CString str);
#endif