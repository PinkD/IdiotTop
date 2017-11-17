/*
���ﶨ����һ��Web�࣬����get����post��ҳ
Visit����ʹ��wininet��ȡ��ҳ���ݣ����Զ�ת�룬����ר��
*/
#ifndef WEBCLIENT_H
#define WEBCLIENT_H

#include <afx.h>
#include <Wininet.h>  
#pragma comment(lib,"wininet.lib")

class HttpClient
{
public:
	HttpClient();
	~HttpClient();

	//ÿ�η���֮��Э��ͷ�����ÿ�
	CString Get(CString url);
	CString Post(CString url,CString PostData);
	
	//ÿ��ʹ��POST����GET֮��Header�����ÿգ�����ÿ��ʹ��POST����GET��Ҫ����Э��ͷ
	BOOL AddSendHeader(CString HeaderName, CString Value);
	BOOL AddSendHeader(CString Header);

	CString GetRecvHeader() { return strRecvHeader; }
	CString GetRecvHeader(CString HeaderName);

	BOOL AddCookie(CString CookieName, CString Value);

	CString GetCookie() { return strCookie; }
	CString GetCookie(CString CookieName);

private:
	HINTERNET hOpen;
	HINTERNET hConnect;
	HINTERNET hRequest;

	CString strHeader;	//�ύЭ��ͷ
	CString strRecvHeader;	//�յ���Э��ͷ
	CString strCookie;	//�ύCookies,���������ݱ���ʱ���Զ��ش����ص�Cookies

	//ɵ��ʽ�Զ�ת���ȡ��ҳ����
	CString Visit(CString url, CString PostData, CString Verb);
	//��url��ȡ����Ҫʹ�õĶ˿�
	static DWORD GetPortFromUrl(CString url);
	//��url��ȡ������,��ѡ���Ƿ����˿�
	static CString GetServerNameFromUrl(CString url, bool isPort);
	//��url��ȡ��ҳ���ַ��û��Ҳ�᷵��һ��/�����磺baidu.com/content/1.txt -> /content/1.txt 
	static CString GetPageNameFromUrl(CString url);
	//��A��B����ͬ��Cookie�ϲ�
	static CString UpdateCookie(CString New, CString Old);
	//��Э��ͷ��ȡ��Cookie
	static CString GetCookieFromHeader(CString Header);
};

#endif