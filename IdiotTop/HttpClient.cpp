#include "stdafx.h"
#include "HttpClient.h"
#include "Code.h"
#include "Func.h"
#include <exception>
#define MAXSIZE 1024000
#define USERAGENT TEXT("Mozilla/5.0 AppleWebKit/537.36 Chrome/45.0.2454.101 Safari/537.36")

HttpClient::HttpClient()//�����Ͷ˿�
{
	hOpen = InternetOpen(USERAGENT, INTERNET_OPEN_TYPE_DIRECT, NULL, 0, 0);
	
	if (hOpen == NULL)
	{
		throw 0;
	}
	
}
HttpClient::~HttpClient()
{
	InternetCloseHandle(hOpen);
}

CString HttpClient::Post(CString url, CString PostData)
{
	return Visit(url, PostData, TEXT("POST"));
}
CString HttpClient::Get(CString url)
{
	return Visit(url,TEXT(""), TEXT("GET"));
}
CString HttpClient::Visit(CString url, CString PostData, CString Verb)
{
	CString strServerName;	//����
	INTERNET_PORT dwPort;	//�˿�
	CString strObject;	//ҳ���ַ

	/*��ȡ��վ��Ϣ*/
	strServerName = GetServerNameFromUrl(url, FALSE);	//��ȡ����
	dwPort = GetPortFromUrl(url);				//��ȡ�˿�
	strObject = GetPageNameFromUrl(url);			//��ȡҳ���ַ

	/*������*/
	if (!(hConnect = InternetConnect(hOpen, strServerName, dwPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0)))
	{
		return TEXT("");
	}

	/*����*/
	DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_AUTO_REDIRECT;
	if (dwPort==443)//https
	{
		dwFlags = dwFlags | INTERNET_FLAG_SECURE;
	}
	else
	{
		dwFlags = dwFlags | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS;
	}

	if (!(hRequest = HttpOpenRequest(hConnect, Verb, strObject,NULL, NULL, NULL, dwFlags, 0)))
	{
		return TEXT("");
	}

	/*��ʼ���챨��*/
	//strHeader = strHeader + TEXT("Host:") + strServerName;

	//if (strHeader.Find(TEXT("User-Agent:")) == -1)
	//	strHeader = strHeader + TEXT("\r\n") + TEXT("User-Agent: ") + USERAGENT;
	if (strHeader.Find(TEXT("Accept:")) == -1)
		strHeader = strHeader + TEXT("\r\n") + TEXT("Accept: */*");
	/*if (strHeader.Find(TEXT("Accept-Encoding:")) == -1)
		strHeader = strHeader + TEXT("\r\n") + TEXT("Accept-Encoding: gzip,deflate");*/
	if (strHeader.Find(TEXT("Accept-Language:")) == -1)
		strHeader = strHeader + TEXT("\r\n") + TEXT("Accept-Language: zh-cn");
	/*if (strHeader.Find(TEXT("Connection:")) == -1)
		strHeader = strHeader + TEXT("\r\n") + TEXT("Connection: keep-alive");*/
	if (strHeader.Find(TEXT("Content-Type:")) == -1)
		strHeader = strHeader + TEXT("\r\n") + TEXT("Content-Type: application/x-www-form-urlencoded");
	if (strHeader.Find(TEXT("Referer:")) == -1)
		strHeader = strHeader + TEXT("\r\n") + TEXT("Referer: ") + url;
	if (!strCookie.IsEmpty())
		strHeader = strHeader + TEXT("\r\n") + TEXT("Cookie: ") + strCookie;
	if (strHeader.Find(TEXT("Content-Length:")) == -1&&Verb.CompareNoCase(TEXT("POST")) == 0)
	{
		CString tmp;
		tmp.Format(TEXT("%d"), PostData.GetLength());
		strHeader = strHeader + TEXT("\r\n") + TEXT("Content-Length: ") + tmp;
	}
	strHeader = strHeader + TEXT("\r\n\r\n");//���Ľ�����ʶ��

	/*���ͱ���*/
	if (Verb.CompareNoCase(TEXT("POST")) == 0)
	{
		char *pszData = NULL;
		//������ֻ�ܹ�����ANSI�ַ���
		pszData = UnicodeToANSI(PostData);
		if (!HttpSendRequest(hRequest, strHeader, strHeader.GetLength(), pszData, strlen(pszData)))
		{
			delete[]pszData;
			return TEXT("");
		}
		delete[]pszData;
	}
	else
	{
		if (!HttpSendRequest(hRequest, strHeader, strHeader.GetLength(), TEXT(""), 0))
		{
			return TEXT("");
		}
	}

	/*��������*/
	char buffer[1024];
	CString ReadBuffer;
	char *sReadBuffer = new char[MAXSIZE];//һ����ҳ1000K���˰�
	char *temp;
	DWORD dwSize = 0;//ÿ�ζ���ĳ���
	DWORD dwReadSize = 0;//�Ѿ�������ܳ���
	DWORD dwMax =MAXSIZE;//�ܹ��洢��󳤶�
	while (1)
	{
		InternetReadFile(hRequest, buffer, 1024, &dwSize);
		if (dwSize == 0)break;
		if (dwSize + dwReadSize > dwMax)//�ڴ�װ������
		{
			temp = sReadBuffer;
			sReadBuffer = new char[MAXSIZE + dwMax];//׷���ڴ�
			memcpy(sReadBuffer,temp, dwReadSize);
			delete []temp;
			dwMax = MAXSIZE + dwMax;
		}
		memcpy(&sReadBuffer[dwReadSize], buffer, dwSize);
		dwReadSize += dwSize;
	}
	//��char��ת����CString
	ReadBuffer = sReadBuffer;
	ReadBuffer = ReadBuffer.Left(dwReadSize);

	//�����ҳ��utf-8���룬��ת����ת����Ansi����Unicode
	int iPos = ReadBuffer.Find(TEXT("utf-8"));
	if (iPos != -1 && iPos < 500 || strHeader.Find(TEXT("utf-8")) != -1)
	{
		char *p= UTF8ToANSI(sReadBuffer);
		ReadBuffer = p;	
		delete[]p;
		ReadBuffer = ReadBuffer.Left(dwReadSize);
	}
	delete []sReadBuffer;

	/*�ӷ��ص�����ͷ����ȡCookie����ԭ����Cookie�ϲ�e*/
	HttpQueryInfo(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, NULL, &dwSize, NULL);
	TCHAR* AnsiHeaders = new TCHAR[dwSize + 1];
	if (!HttpQueryInfo(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, (LPVOID)AnsiHeaders, &dwSize, NULL))
	{
		delete[]AnsiHeaders;
		return ReadBuffer;
	}
	strRecvHeader = AnsiHeaders;
	delete[]AnsiHeaders;
	strCookie = UpdateCookie(GetCookieFromHeader(strRecvHeader), strCookie);//����Cookie

	InternetCloseHandle(hRequest);
	InternetCloseHandle(hConnect);
	strHeader.Empty();	//ÿ������֮��Ҫ�ѷ��͵�Э��ͷ���
	return ReadBuffer;
}
BOOL HttpClient::AddCookie(CString CookieName,CString Value)
{
	if (!strCookie.IsEmpty()) {
		strCookie = strCookie + TEXT(";");
	}
	strCookie = strCookie + CookieName + TEXT("=") + Value;
	return TRUE;
}
BOOL HttpClient::AddSendHeader(CString HeaderName,CString Value) {
	if (strHeader.Find(HeaderName+TEXT(":")) == -1) {
		strHeader = HeaderName + TEXT(":") + Value + TEXT("\r\n") + strHeader;
	}
	else {
		return FALSE;
	}
	return TRUE;
}
BOOL HttpClient::AddSendHeader(CString Header) {
	if (strHeader.Find(SubString(Header,TEXT(""),TEXT(":"))+TEXT(":")) == -1) {
		strHeader = Header + TEXT("\r\n") + strHeader;
	}
	else {
		return FALSE;
	}
	return TRUE;
}
CString HttpClient::GetRecvHeader(CString HeaderName) {
	DWORD Pos_start = 0, Pos_end = 0;
	Pos_start = strRecvHeader.Find(HeaderName + TEXT(":"));
	if (Pos_start == -1)return TEXT("");
	Pos_start += HeaderName.GetLength() + 1;//��ð��һ��ӽ���
	Pos_end = strRecvHeader.Find(TEXT("\r\n"), Pos_start);
	if (Pos_end == -1) {
		//��Header����ĩβ
		return strRecvHeader.Mid(Pos_start, strRecvHeader.GetLength() - Pos_start);
	}
	else {
		//��Header���м�
		return strRecvHeader.Mid(Pos_start, Pos_end - Pos_start);
	}

}

CString HttpClient::GetCookie(CString CookieName) {
	DWORD Pos_start = 0, Pos_end = 0;
	Pos_start = strCookie.Find(CookieName + TEXT("="));
	if (Pos_start == -1)return TEXT("");
	Pos_start += CookieName.GetLength() + 1;//�ѵȺ�һ��ӽ���
	Pos_end = strCookie.Find(TEXT(";"), Pos_start);
	if (Pos_end == -1) {
		//��Cookie����ĩβ
		return strCookie.Mid(Pos_start, strCookie.GetLength() - Pos_start);
	}
	else {
		//��Cookie���м�
		return strCookie.Mid(Pos_start, Pos_end - Pos_start);
	}

}

CString HttpClient::GetCookieFromHeader(CString Headers)
{
	CString Row;
	CString Cookie;
	DWORD pos = 0;
	DWORD tmp;

	Headers = Headers + TEXT("\r\n");
	while (1)
	{
		if (pos != 0)pos += 2;
		tmp = pos;
		pos = Headers.Find(TEXT("\r\n"), pos);
		if (pos == -1 || pos == 0 || pos == Headers.GetLength() - 2)//�մ�����Ѱ�ҵ����һ�����з�
		{
			break;
		}
		else
		{
			Row = Headers.Mid(tmp, pos - tmp);
		}
		if (Row.Find(TEXT("Set-Cookie:")) != -1)
		{
			Cookie = Cookie + SubString(Row, TEXT("Set-Cookie:"), TEXT(";")) + TEXT("; ");
		}
	}
	Cookie = Cookie.Left(Cookie.GetLength() - 2);
	return Cookie;
}
CString HttpClient::UpdateCookie(CString New,CString Old)
{
	int pos = 0;
	int tmp;
	CString Row;
	CString Row1;
	New.Remove(TEXT(' '));
	Old.Remove(TEXT(' '));
	if (New.IsEmpty())return Old;
	if (Old.IsEmpty())return New;
	while (1)
	{
		if (pos == -1)break;
		if (pos != 0)pos ++;
		tmp = pos;
		pos = Old.Find(TEXT(";"), pos);
		if (pos == -1)
		{
			Row = Old.Right(Old.GetLength() - tmp);
		}
		else
		{
			Row = Old.Mid(tmp, pos - tmp);
		}

		int pos1 = 0;
		while (1)//�ж��ϵ�cookie���µ�cookie���Ƿ���ڣ�����������ӽ�ȥ�������������ϵ�
		{
			if (pos1 == -1)
			{
				New = New + TEXT("; ") + Row;
				break;//���µ�����û�оɵģ��ͰѾɵ���ӵ��µ�����
			}
			if (pos1 != 0)pos1++;
			tmp = pos1;
			pos1 = New.Find(TEXT(";"), pos1);
			if (pos1 == -1)
			{
				Row1 = New.Right(New.GetLength() - tmp);
			}
			else
			{
				Row1 = New.Mid(tmp, pos1 - tmp);
			}
			if (New.Right(8) == TEXT("=deleted"))break;
			int i = Row1.Find(TEXT("="));
			int j = Row.Find(TEXT("="));
			if (Row1.Left(i) == Row.Left(j))break;//���ھ�����
			
		}
	}
	return New;
}
DWORD HttpClient::GetPortFromUrl(CString url)
{
	CString port;
	int iPos;
	port = GetServerNameFromUrl(url,true);
	iPos = port.Find(TEXT(":"));
	if (iPos != -1)
	{
		port = port.Right(port.GetLength() - iPos - 1);
		return _ttoi(port);//ת��������

	}
	
	if (url.Left(5) == TEXT("https"))return 443;
	return 80;
}
CString HttpClient::GetServerNameFromUrl(CString url,bool isPort)
{
	CString ServerName = url.MakeLower();
	if (ServerName.Right(1) != TEXT("/"))ServerName = ServerName + TEXT("/");
	if (ServerName.Left(8) == TEXT("https://"))
	{
		ServerName = SubString(ServerName, TEXT("https://"),TEXT("/"));
	}
	else 
	{
		if (ServerName.Find(TEXT("http://")) == -1)ServerName = TEXT("http://") + ServerName;
		ServerName = SubString(ServerName, TEXT("http://"), TEXT("/"));
	}
	if (isPort == true)return ServerName;//�Ƿ����˿�
	int iPos = ServerName.Find(TEXT(":"));
	if (iPos == -1)return ServerName;
	return ServerName.Left(iPos);
}
CString HttpClient::GetPageNameFromUrl(CString url)
{
	CString PageName;
	CString ServerName = GetServerNameFromUrl(url,NULL);
	int iPos = url.Find(ServerName);
	iPos = url.Find(TEXT("/"),iPos);
	if (iPos > 0)
	{
		PageName = url.Right(url.GetLength() - iPos);
	}
	else
	{
		PageName = TEXT("/");
	}
	return PageName;
}

