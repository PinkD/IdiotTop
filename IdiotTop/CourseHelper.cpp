#include "stdafx.h"

#include "CourseHelper.h"


BOOL CourseHelper::stop = TRUE;

#define returnIfStop() \
			if(stop) {\
				return;\
			}


void CourseHelper::login(CString username, CString password, Callback<LOGIN_RESULT> *callback) {
	user.Init(username, password);
	loginCallback = callback;
	SubmitThreadpoolWork(CreateThreadpoolWork(loginAsync, this, NULL));
}

void CALLBACK CourseHelper::loginAsync(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work) {
	CourseHelper *courseHelper = (CourseHelper *)context;

#ifdef _DEBUG
	//For debug, skip login repeatly
	//courseHelper->setStatus(STATUS_LOGIN);
	//return courseHelper->loginCallback->onResult(RESULT_LOGIN_OK);
#endif

	CString response;

	//JSESSIONID
	response = courseHelper->httpClient.Get(GetString(IDS_BASE_URL));
	if (courseHelper->httpClient.GetCookie().IsEmpty()) {
		return courseHelper->loginCallback->onResult(RESULT_NETWORK_FAIL);
	} else {
#ifdef _DEBUG
		OutputDebugString(TEXT("���") + courseHelper->httpClient.GetCookie() + TEXT("\n"));
#endif
	}

	//DWRSESSIONID
	courseHelper->httpClient.AddSendHeader(TEXT("Referer"), GetString(IDS_BASE_URL) + GetString(IDS_INDEX_HTM));
	response = courseHelper->httpClient.Post(GetString(IDS_BASE_URL) + GetString(IDS_URL_GEN_ID), GetString(IDS_POST_DATA_GEN_ID));

	courseHelper->DWRSESSIONID = SubString(response, TEXT("r.handleCallback(\"0\",\"0\",\""), TEXT("\");")); 
#ifdef _DEBUG
	OutputDebugString(TEXT("��õ�DWRSESSIONID��") + courseHelper->DWRSESSIONID + TEXT("\n"));
#endif
	if (courseHelper->DWRSESSIONID.IsEmpty()) {
		return courseHelper->loginCallback->onResult(RESULT_NETWORK_FAIL);
	} else {
		courseHelper->httpClient.AddCookie(TEXT("DWRSESSIONID"), courseHelper->DWRSESSIONID);
	}
#ifdef _DEBUG
	OutputDebugString(TEXT("��õ�Cookies��") + courseHelper->httpClient.GetCookie() + TEXT("\n"));
#endif

	//code
	courseHelper->httpClient.Get(GetString(IDS_BASE_URL) + GetString(IDS_LOGIN_IMAGE));
	CString authorCode = courseHelper->httpClient.GetCookie(TEXT("rand"));


	//login
	courseHelper->httpClient.AddSendHeader(GetString(IDS_HEADER_REF_LOGIN));
	courseHelper->httpClient.AddCookie(TEXT("rand"), GetRandNumberStringBetween(1000, 9999));
	CString postData = GetString(IDS_POST_DATA_LOGIN) + courseHelper->getScriptSessionId();
	postData.Replace(TEXT("`username`"), courseHelper->user.getUsername());
	postData.Replace(TEXT("`password`"), courseHelper->user.getPassword());
	postData.Replace(TEXT("`authorCode`"), authorCode);
	response = courseHelper->httpClient.Post(GetString(IDS_BASE_URL) + GetString(IDS_URL_CORE), postData);

#ifdef _DEBUG
	OutputDebugString(TEXT("��½�󷵻ص����ݣ�\n") + response + TEXT("\n"));
#endif
	if (response.Find(TEXT("flag:-4")) != -1 || response.Find(TEXT("flag:20")) != -1) {
		return courseHelper->loginCallback->onResult(RESULT_NO_RETRY);
	}
	else if (response.Find(TEXT("flag:1")) == -1) {
		return courseHelper->loginCallback->onResult(RESULT_LOGIN_FAILT);
	}
	else {
		courseHelper->setStatus(STATUS_LOGIN);
		return courseHelper->loginCallback->onResult(RESULT_LOGIN_OK);
	}
}


void CourseHelper::refreshCourse(Callback<CourseHelper::OTHER_RESULT> *callback) {
	otherCallback = callback;
	SubmitThreadpoolWork(CreateThreadpoolWork(refreshCourseAsync, this, NULL));
}

void CALLBACK CourseHelper::refreshCourseAsync(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work) {
	CourseHelper *courseHelper = (CourseHelper *)context;
	//ˢ�����пγ̲������ڴ�����
	courseHelper->clearList();
	CString response;
	courseHelper->httpClient.AddSendHeader(GetString(IDS_HEADER_REF_STUDY));
	CString postData = GetString(IDS_POST_DATA_STUDY) + courseHelper->getScriptSessionId();
	response = courseHelper->httpClient.Post(GetString(IDS_BASE_URL) + GetString(IDS_URL_COMMON), postData);
	response = HexStrToWChars(response);
	if (response.Find(TEXT("flag:0")) == -1) {
		courseHelper->otherCallback->onResult(CourseHelper::OTHER_RESULT::RESULT_FAIL);
	}

	CourseInfo *courseInfo;
	CString code;
	CString name;
	CString status;
	CString score;
	CString deadline;


	CString TD;//һ��
	int TDpos = 0;
	int index = -1;

	CString TR;//һ��
	int TRpos = 0;

	while (TRUE) {
		TR = SubString(response, TEXT("<tr>\\r\\n                <td>"), TEXT("</tr>\\r\\n"), &TRpos);//ȡ��һ��
		if (TR.IsEmpty()) {
			break;
		}
		index++;
		code = SubString(TR, TEXT(""), TEXT("</td>\\r\\n"), &TDpos);
		name = SubString(TR, TEXT("target=\\\"_blank\\\">"), TEXT("</a></td>"), &TDpos);
		score = SubString(TR, TEXT("<td>"), TEXT("</td>\\r\\n"), &TDpos);
		deadline = SubString(TR, TEXT("<td>"), TEXT("</td>\\r\\n"), &TDpos);
		status = SubString(TR, TEXT("<td>"), TEXT("</td>\\r\\n"), &TDpos);
		TDpos = 0;
		courseInfo = new CourseInfo(code, name, status, score, deadline);
		courseHelper->courseList.InsertAt(0, courseInfo);
	}
	courseHelper->otherCallback->onResult(CourseHelper::OTHER_RESULT::RESULT_OK);
}

//-----------------------------------------------------------

void CourseHelper::cancelAnswerCourse() {
	setStatus(STATUS_LOGIN);
	textCallback = (Callback<CString> *)emptyTextCallback;
	stop = TRUE;
}

void CourseHelper::answerAllCourse(Callback<CString> *textCallback) {
	setStatus(STATUS_ANSWER);
	this->textCallback = textCallback;
	stop = FALSE;
	SubmitThreadpoolWork(CreateThreadpoolWork(answerAllCourseAsync, this, NULL));
}

void CourseHelper::answerAllCourseAsync(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work) {//ˢ�����߳�
	CourseHelper *courseHelper = (CourseHelper *)context;
	for (int i = 0; i < courseHelper->courseList.GetCount(); i++) {
		returnIfStop();
		courseHelper->answerCourse(*courseHelper->courseList[i]);
	}
	courseHelper->setStatus(STATUS_DONE);
}


void CourseHelper::answerCourse(CourseInfo course) {//ˢĳ�ڿ�
	CStringArray allSectionId;//ÿ���γ�����ÿ�ڵ�ID
	CStringArray allSectionName;//ÿ���γ�����ÿ�ڵ�����
	CStringArray allTimeSection;//ÿ���γ�����ûˢʱ��Ľڵ�����

	if (getSection(course.getId(), &allSectionId, &allSectionName)) {//��ȡ�����½�
		answerAllSection(course, allSectionId, allSectionName, &allTimeSection);
		returnIfStop();
		initPostTime(allTimeSection);//ˢʱ��
		returnIfStop();
	} else {
		textCallback->onResult(course.getId() + TEXT(" ") + course.getName() + TEXT("��ȡ�γ��½�ʧ�ܣ�"));
	}
}


void CourseHelper::answerAllSection(CourseInfo course, CStringArray &allSectionId, CStringArray &allSectionName, CStringArray *allTimeSection) {//ˢĳһ��
	if (course.getStatus() == "�ѽ�ҵ") {
		textCallback->onResult(TEXT("\r\n") + course.getName() + TEXT("�ѽ�ҵ"));
#ifdef _DEBUG
		OutputDebugString(course.getName() + TEXT("�ѽ�ҵ"));
#endif
		return;
	}
	for (int i = 0; i < allSectionId.GetCount(); i++) {
		returnIfStop();
		textCallback->onResult(TEXT("��ʼˢ��") + allSectionId[i] + TEXT(".") + allSectionName[i] + TEXT("\n"));
#ifdef _DEBUG
		OutputDebugString(TEXT("��ʼˢ��") + allSectionId[i] + TEXT(".") + allSectionName[i] + TEXT("\n"));
#endif
		answerSection(course, allSectionId[i], allSectionName[i], allTimeSection);
		refreshCourseGrade(course.getId());
	}
#ifdef _DEBUG
	OutputDebugString(TEXT("�Ѿ�ˢ��γ�") + course.getName() + TEXT("\n"));
#endif
}

void CourseHelper::answerSection(CourseInfo course, CString sectionId, CString sectionName, CStringArray *allTimeSection) {//��ĳ�µ���һ���ύ
	CString Exes;//һ����������
	CString ExesName;//������ͺ͵ڼ���
	CString question;//����ͱ�ѡ��
	CString answer;//���ύ�Ĵ�
	CString answerId;//���ID
	CString selection;//ĳ��ѡ��
	CString allInfo;
	CString tmp;
	CString response;
	int Pos = 0;
	int Pos1 = 0;
	CString courseAnswer;
	CString sectionStatus;
	BOOL answered = FALSE;
	BOOL replied = FALSE;

	courseAnswer = readAnswerFromFile(TEXT("BinaryAnswerLibrary\\") + course.getId() + TEXT(".") + course.getName() + TEXT(".txt"));//��ȡ�γ̴�
	sectionStatus = getSectionStatus(course.getId(), sectionId, 1);
	if (sectionStatus.Find(TEXT("OK</strong> \\r\\n                 <span class=\\\"explain_rate\\\"><a href=\\\"javascript:;\\\" onclick=\\\"atPage(\\\'ʱ��˵��")) == -1) {//ʱ��ûˢ�꣬�����ˢ����
		allTimeSection->Add(TEXT("|") + course.getId() + TEXT("|") + course.getName() + TEXT("|") + sectionId + TEXT("|") + sectionName + TEXT("|"));
	}
	if (sectionStatus.Find(TEXT("OK</strong> \\r\\n                 <span class=\\\"explain_rate\\\"><a href=\\\"javascript:;\\\" onclick=\\\"atPage(\\\'ϰ��˵��")) != -1) {//ϰ��ˢ����
		answered = TRUE;
	}
	if (sectionStatus.Find(TEXT("OK</strong> \\r\\n             <span class=\\\"explain_rate\\\"><a href=\\\"javascript:;\\\" onclick=\\\"atPage(\\\'ý��˵��")) != -1) {//ý��ˢ����
		replied = TRUE;
	}

	if (answered == TRUE && replied == TRUE) {
		textCallback->onResult(TEXT("\r\n��������ɣ�") + course.getName() + TEXT(" ") + sectionName);
		return;//ȫ��ˢ���˾ͷ���
	}
	//��ʼ��ȡ���ڵ��������ݣ�����ý�����ۺ�ϰ��
	CString header = GetString(IDS_HEADER_REF_LEARN);
	header.Replace(TEXT("`courseId`"), course.getId());
	header.Replace(TEXT("`sectionId"), sectionId);
	httpClient.AddSendHeader(header);
	CString postData = GetString(IDS_POST_DATA_LEARN_STATUS) + getScriptSessionId();
	postData.Replace(TEXT("`courseId`"), course.getId());
	postData.Replace(TEXT("`sectionId`"), sectionId);
	response = httpClient.Post(GetString(IDS_BASE_URL) + GetString(IDS_URL_COMMON), postData);

	if (replied != TRUE) {
		mediaReply(response, course.getName(), sectionName);//��ý������
	}

	response = SubString(response, TEXT("<span class=\\\"delNum"), TEXT("</dd>"));
	response = HexStrToWChars(response);

	//�ύ���µ����
	answer = SubString(courseAnswer, TEXT("<") + sectionId + TEXT(">"), TEXT("</") + sectionId + TEXT(">"));
	if (response.Find(TEXT("��ȷ�ʣ�100%")) != -1 || answered == TRUE) {
		textCallback->onResult(TEXT("\r\n�����������꣺") + course.getName() + TEXT(" ") + sectionName);
		return;
	}
	if (!answer.IsEmpty()) {//����⣬��δ����
		textCallback->onResult(TEXT("\r\n�ύ��⣺") + course.getName() + TEXT(" ") + sectionName);
		header = GetString(IDS_HEADER_REF_LEARN);
		header.Replace(TEXT("`courseId`"), course.getId());
		header.Replace(TEXT("`sectionId`"), sectionId);
		httpClient.AddSendHeader(header);
		postData = GetString(IDS_POST_DATA_ANSWER) + getScriptSessionId();
		CString s = UrlDecodeUTF8(answer);
		postData.Replace(TEXT("`answer`"), s);
		postData.Replace(TEXT("`courseId`"), course.getId());
		postData.Replace(TEXT("`sectionId`"), sectionId);
		response = httpClient.Post(GetString(IDS_BASE_URL) + GetString(IDS_URL_DO), postData);
#ifdef _DEBUG
		OutputDebugString(TEXT("���ⷵ��:") + response);
#endif
		textCallback->onResult(TEXT("\r\n�ȴ���һ�½�..."));
		Sleep(CourseHelper::DELAY_TIME);//��ʱһ��ʱ�䣬����һ��
	} else {
		textCallback->onResult(TEXT("\r\n����⣺") + course.getName() + TEXT(" ") + sectionName);
	}

}

void CourseHelper::mediaReply(CString allInfo, CString courseName, CString sectionName) {
	//�Կγ��е�ý���������
	CString mediaId;
	int pos = 0;
	CString s;

	textCallback->onResult(TEXT("\r\n�ύý�����ۣ�") + courseName + TEXT(" ") + sectionName);
	while (1) {
		returnIfStop();
		mediaId = SubString(allInfo, TEXT("parent.showMediaRight("), TEXT(")"), &pos);
		if (mediaId.IsEmpty()) {
#ifdef _DEBUG
			OutputDebugString(TEXT("����ý���Ѿ������꣺") + courseName + TEXT(" ") + sectionName + TEXT("\n"));
#endif
			break;
		}
#ifdef _DEBUG
		OutputDebugString(TEXT("�����ύý�����ۣ�") + courseName + TEXT(" ") + sectionName + TEXT(" ") + mediaId + TEXT(" "));
#endif
		CString header = GetString(IDS_HEADER_REF_MEDIA);
		header.Replace(TEXT("`mediaId`"), mediaId);
		httpClient.AddSendHeader(header);
		CString postData = GetString(IDS_POST_DATA_MEDIA) + getScriptSessionId();
		postData.Replace(TEXT("`mediaId`"), mediaId);
		s = httpClient.Post(GetString(IDS_BASE_URL) + GetString(IDS_URL_DO), postData);//�ύ
#ifdef _DEBUG
		OutputDebugString(TEXT("���ύ�����أ�") + s + "\n");
#endif
	}
}

void CourseHelper::initPostTime(CStringArray &allTimeSection) {
	//����ˢʱ����߳�, �ͻ���ÿ15�������������һ�����ݣ�ÿ�η��ͣ�batchId++�������͵��Ĵ�ʱ,�ͻ��˻��ٴη�����������ȡ��ǰ�γ̵�ʣ��ʱ��,����ʹ��N���߳���ˢʱ�䣬���Դ���ȼӿ�ˢʱ��

	setStatus(STATUS_VIDEO);
	textCallback->onResult(TEXT("��ʼˢʱ�䣺"));

	CString section;
	CString courseId;//��ǰ���еĿγ̴���
	CString courseName;//��ǰ���еĿγ�����
	CString sectionId;
	CString	sectionName;
	int iPos;

	int NumSection = allTimeSection.GetCount();
	for (int i = 0; i < NumSection; i++) {
		returnIfStop();
		//ȡ��һ��
		iPos = 0;
		section = allTimeSection[0];
		allTimeSection.RemoveAt(0);
		courseId = SubString(section, TEXT("|"), TEXT("|"), &iPos);
		courseName = SubString(section, TEXT("|"), TEXT("|"), &iPos);
		sectionId = SubString(section, TEXT("|"), TEXT("|"), &iPos);
		sectionName = SubString(section, TEXT("|"), TEXT("|"), &iPos);

		postTime(courseId, courseName, sectionId, sectionName);
		refreshCourseGrade(courseId);
	}
}

void CourseHelper::postTime(CString courseId, CString courseName, CString sectionId, CString sectionName) {
	CString response;
	int begin = 1;
	int batchId = begin;
	CString lastTime = TEXT("---");//�ϴ�ʣ��ʱ��

	CString fmt;
	while (1) {
		returnIfStop();
		//ÿˢһ��ʱ�䶼��ȡһ��ʣ��ʱ�䣬�������Զ��Ӧ�ó���ˢ��ʱ�����ˢ��ʱ��
		response = getSectionStatus(courseId, sectionId, batchId);
		if (response.Find(TEXT("OK</strong> \\r\\n                 <span class=\\\"explain_rate\\\"><a href=\\\"javascript:;\\\" onclick=\\\"atPage(\\\'ʱ��˵��")) != -1) {
			//ˢ����
			textCallback->onResult(TEXT("\r\n���½ڵ�ʱ��ˢ���ˣ�") + courseName + TEXT(" ") + sectionName);
#ifdef _DEBUG
			OutputDebugString(TEXT("���½ڵ�ʱ��ˢ���ˣ�") + courseName + TEXT(" ") + sectionName + TEXT("\n"));
#endif
			return;
		}
		else {
			CString remainTime = SubString(response, TEXT("��ѧʱ��\\\">"), TEXT("</span>"));
			remainTime = remainTime + TEXT("/") + SubString(response, TEXT("��ѧϰʱ��\\\">"), TEXT("</span>"));

			//if ((batchId - begin) % 4 == 0)//ÿ4��
			if (lastTime.CollateNoCase(remainTime) != 0) {//��ʣ��ʱ���иı�ʱˢ��
				lastTime = remainTime;
				textCallback->onResult(TEXT("\r\nʣ��ʱ�䣺") + courseName + TEXT(" ") + sectionName + TEXT(" ") + remainTime);
#ifdef _DEBUG
				OutputDebugString(TEXT("ʣ��ʱ�䣺") + courseName + TEXT(" ") + sectionName + TEXT(" ") + remainTime + TEXT("\n"));
#endif
				batchId++;
				begin = batchId;
			}
		}
		for (int j = 1; j <= 15; j++) {//��ʱ15��
			returnIfStop();
			Sleep(1000);
		}
		CString header = GetString(IDS_HEADER_REF_LEARN);
		header.Replace(TEXT("`courseId`"), courseId);
		header.Replace(TEXT("`sectionId`"), sectionId);
		httpClient.AddSendHeader(header);
		CString postData = GetString(IDS_POST_DATA_TIME) + getScriptSessionId();
		postData.Replace(TEXT("`courseId`"), courseId);
		postData.Replace(TEXT("`sectionId`"), sectionId);
		fmt.Format(TEXT("%d"), batchId);
		postData.Replace(TEXT("`batchId`"), fmt);
		response = httpClient.Post(GetString(IDS_BASE_URL) + GetString(IDS_URL_COMMON), postData);
		if (response.Find(TEXT("flag:1")) == -1) {
#ifdef _DEBUG
			OutputDebugString(TEXT("POSTʱ�����") + courseName + TEXT(" ") + sectionName + TEXT("\n"));
#endif
		} else {
			textCallback->onResult(TEXT("\r\n����15�룺") + courseName + TEXT(" ") + sectionName);
#ifdef _DEBUG
			OutputDebugString(TEXT("POST��һ��ʱ�䣺") + courseName + TEXT(" ") + sectionName);
#endif
		}
#ifdef _DEBUG
		OutputDebugString(TEXT("ˢʱ�䷵�ص����ݣ�\n") + response);
#endif
		batchId++;
	}
}


//-----------------------------------------------------------

void CourseHelper::cancelGetAnswerLib() {
	setStatus(STATUS_LOGIN);
	textCallback = (Callback<CString> *)emptyTextCallback;
	stop = TRUE;
}

void CourseHelper::getAllAnswerLib(Callback<CString> *textCallback) {
	stop = FALSE;
	setStatus(STATUS_LIBRARY);
	this->textCallback = textCallback;
	SubmitThreadpoolWork(CreateThreadpoolWork(getAllAnswerLibAsync, this, NULL));
}


void CourseHelper::getAllAnswerLibAsync(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work) {
	CourseHelper *courseHelper = (CourseHelper *)context;
	for (int i = 0; i < courseHelper->courseList.GetCount(); i++) {
		returnIfStop();
		courseHelper->backupCourse(courseHelper->courseList[i]);//mv xxx.txt -> xxx.txt.bak
		courseHelper->getCourseAnswerLib(*courseHelper->courseList[i]);
		if (stop) {//on cancel: mv xxx.txt.bak xxx.txt
			courseHelper->restoreCourse();
			return;
		}
		courseHelper->commitCourse(courseHelper->courseList[i]);//mv xxx.txt.bak xxx.txt.old
	}
	courseHelper->setStatus(STATUS_DONE);
}

void CourseHelper::getCourseAnswerLib(CourseInfo course) {
	CStringArray allSectionId;//ÿ���γ�����ÿ�ڵ�ID
	CStringArray allSectionName;//ÿ���γ�����ÿ�ڵ�����

	if (getSection(course.getId(), &allSectionId, &allSectionName)) {//��ȡ�����½�
		getAllSectionAnswerLib(course, allSectionId, allSectionName);
	} else {
		textCallback->onResult(course.getId() + TEXT(" ") + course.getName() + TEXT("\r\n��ȡ�γ��½�ʧ�ܣ�"));
	}
}

void CourseHelper::getAllSectionAnswerLib(CourseInfo course, CStringArray &allSectionId, CStringArray &alSctionName) {//��ȡĳһ��
	for (int i = 0; i < allSectionId.GetCount(); i++) {
		returnIfStop();
		textCallback->onResult(TEXT("��ʼ��ȡ�ýڴ𰸣�") + allSectionId[i] + TEXT(".") + alSctionName[i] + TEXT("\r\n"));
#ifdef _DEBUG
		OutputDebugString(TEXT("��ʼ��ȡ�ýڴ𰸣�") + allSectionId[i] + TEXT(".") + alSctionName[i] + TEXT("\r\n"));
#endif
		getSectionAnswerLib(course, allSectionId[i], alSctionName[i]);
	}
#ifdef _DEBUG
	OutputDebugString(TEXT("�Ѿ���ȡ��γ�") + course.getName() + TEXT("\n"));
#endif
}

void CourseHelper::getSectionAnswerLib(CourseInfo course, CString sectionId, CString sectionName) {

	CString exes;//һ����������
	CString exesName;//������ͺ͵ڼ���
	CString question;//����ͱ�ѡ��
	CString answer;//���ύ�Ĵ�
	CString answerId;//���ID
	CString selection;//ĳ��ѡ��
	CString allInfo;
	CString tmp;
	CString response;
	int Pos = 0;
	int Pos1 = 0;
	CString courseAnswer;

	//��ȡ����״̬
	CString sectionStatus;
	BOOL answered = FALSE;
	BOOL replied = FALSE;

	CString header = GetString(IDS_HEADER_REF_LEARN);
	header.Replace(TEXT("`courseId`"), course.getId());
	header.Replace(TEXT("`sectionId"), sectionId);
	httpClient.AddSendHeader(header);
	CString postData = GetString(IDS_POST_DATA_LEARN_STATUS) + getScriptSessionId();
	postData.Replace(TEXT("`courseId`"), course.getId());
	postData.Replace(TEXT("`sectionId`"), sectionId);
	response = httpClient.Post(GetString(IDS_BASE_URL) + GetString(IDS_URL_COMMON), postData);

	response = SubString(response, TEXT("<span class=\\\"delNum"), TEXT("</dd>"));
	response = HexStrToWChars(response);


	//�����µ���ȫ����ȡ���������ں�̨����ȡ
	while (1) {
		returnIfStop();
		exes.Empty(); exesName.Empty(); question.Empty(); answerId.Empty(); tmp.Empty(); selection.Empty();
		Pos1 = 0;
		exes = SubString(response, TEXT("<li name=\\\"xt\\\""), TEXT("\\r\\n                   \\r\\n                 </li>"), &Pos);//ÿһ����
		if (exes.IsEmpty()) {
			exes = SubString(response, TEXT("<li>"), TEXT("\\r\\n\\r\\n                  \\r\\n"), &Pos);//���ֿ��ܵĽ�β
		}
		if (exes.IsEmpty()) {//��һ�ڵ�������
			answer = answer.Left(answer.GetLength() - 3);//ȥ��ĩβ��&;&
			answer = TEXT("<") + sectionId + TEXT(">") + answer + TEXT("</") + sectionId + TEXT(">\r\n");
			writeAnswerToFile(TEXT("BinaryAnswerLibrary\\") + course.getId() + TEXT(".") + course.getName() + TEXT(".txt"), answer);
			textCallback->onResult(TEXT("\r\n�ýڵ����Ѿ���ȡ�꣬��һ�ڣ�"));
#ifdef _DEBUG
			OutputDebugString(TEXT("\n�ýڵ����Ѿ���ȡ�꣬��һ�ڣ�") + sectionName + TEXT("\r\n"));
#endif
			return;//����һ�ڣ�
		}
		//if (Exes.Find(TEXT("ed-ans")) == -1)continue;//�ж��Ƿ�ش���ȷ���������ȷ�Ͳ�Ҫ
		exesName = SubString(exes, TEXT("<h5>"), TEXT("\\r\\n"), &Pos1);//�ڼ���
		answerId = SubString(exes, TEXT("id=\\\""), TEXT("\\\""));
		answerId = answerId.Right(answerId.GetLength() - 3);//xt_4260 dx_2182 mx_2180 pd_2187ֻҪ���������
		answer = answer + answerId + TEXT("&=&");

		if (exesName.Find(TEXT("�����")) != -1) {
			int Last = 0;
			int Pos2 = 0;
			question = SubString(exes, TEXT("<p>\\r\\n\\t\\t\\t\\t\\t\\t"), TEXT("\\r\\n                   </p>"), &Pos1);
			while (1) {//ȡ���м�����
				Pos2 = question.Find(TEXT("\\r\\n                             "), Last);
				if (Pos2 == -1) {
					Pos2 = question.Find(TEXT("\\r\\n\\t\\t\\t\\t\\t "), Last);
				}
				if (Pos2 == -1) {
					tmp = tmp + question.Right(question.GetLength() - Last);
					break;
				}
				tmp = tmp + question.Mid(Last, Pos2 - Last) + TEXT("��   ��");
				Last = Pos2;
				answer = answer + SubString(question, TEXT("value=\\\""), TEXT("\\\" />��"), &Last) + TEXT("&,&");
				if (question.Find(TEXT("\\r\\n\\t\\t\\t\\t\\t\\t"), Last) == -1) {
					Last = Last + 6;//  \" />���ĳ���
				} else {
					Last = question.Find(TEXT("\\r\\n\\t\\t\\t\\t\\t\\t"), Last);
					Last = Last + 16;// \r\n\t\t\t\t\t\t�ĳ���
				}
			}
			if (answer.Right(3) = TEXT("&,&")) {
				answer = answer.Left(answer.GetLength() - 3);//ȥ��ĩβ��&,&
			}
			question = TEXT("\r\nQuestion��") + tmp + TEXT("\r\nAnswer��") + answer + TEXT("\r\n");
			//OutputDebugString(TEXT("\nQuestion��") + Question);
		} else if (exesName.Find(TEXT("��ѡ��")) != -1 || exesName.Find(TEXT("��ѡ��")) != -1 || exesName.Find(TEXT("�ж���")) != -1) {
			CString options;
			int n = 0;
			question = SubString(exes, TEXT("<p>"), TEXT("</p>"), &Pos1);
			//OutputDebugString(TEXT("\nQuestion��") + Question + TEXT("\nAnswer��\n"));
			question = TEXT("\r\nQuestion��") + question + TEXT("\r\nAnswer��\r\n");
			while (1) {
				selection = SubString(exes, TEXT("<li"), TEXT("\\r\\n"), &Pos1);
				if (selection.IsEmpty()) {
					break;
				}else if ((selection.Find(TEXT("checked=\\\"checked\\\"")) != -1)) {
					options = options + SubString(selection, TEXT("value=\\\""), TEXT("\\\"")) + TEXT(",");
					n++;
					//Answer = Answer + SubString(Selection, TEXT("value=\\\""), TEXT("\\\"")) + TEXT(",");
					question = question + TEXT("��");
					//OutputDebugString(TEXT("��"));
				} else {
					question = question + TEXT(" ");
				}
				question = question + SubString(selection, TEXT("/>"), TEXT("</li>")) + TEXT("\r\n");
				//OutputDebugString(SubString(selection, TEXT("/>"), TEXT("</li>")) + TEXT("\n"));
			}
			if (n > 1) {
				answer += sortAnswer(options, n);
			}
			else {
				options.Replace(TEXT(","), TEXT(""));
				answer += options;
			}
		}
		textCallback->onResult(course.getId() + TEXT(".") + course.getName() + TEXT("  ") + sectionName + TEXT("\r\n") + exesName + TEXT("��") + question);
		writeAnswerToFile(TEXT("TextAnswerLibrary\\") + course.getId() + TEXT(".") + course.getName() + TEXT(".txt"), TEXT("\r\n��") + exesName + TEXT("��") + question);
		answer = answer + TEXT("&;&");
	}

}


//-----------------------------------------------------------

CourseHelper::CourseHelper() {
	emptyTextCallback = new EmptyTextCallback;
	createDirIfNotExists(TEXT("BinaryAnswerLibrary\\"));
	createDirIfNotExists(TEXT("TextAnswerLibrary\\"));
	clearBackupFile(TEXT("BinaryAnswerLibrary\\"));
	clearBackupFile(TEXT("TextAnswerLibrary\\"));
}


CourseHelper::~CourseHelper() {
}

void CourseHelper::clearList() {
	while (!courseList.IsEmpty()) {
		delete courseList.GetAt(0);
		courseList.RemoveAt(0);
	}
}

BOOL CourseHelper::refreshCourseGrade(CString courseId) {
	CString url = GetString(IDS_BASE_URL) + GetString(IDS_URL_WK_INDEX);
	url.Replace(TEXT("`courseId`"), courseId);
	CString response = httpClient.Get(url);
	CString score = SubString(response, TEXT("class=\"markNum\">"), TEXT("</strong>"));
#ifdef _DEBUG
	OutputDebugString(TEXT("��ȡ������") + courseId + TEXT("  ") + score + TEXT("\n"));
#endif
	if (score.IsEmpty()) {
		return FALSE;
	}
	for (int i = 0; i < courseList.GetCount(); i++) {
		if (courseList[i]->getId().Compare(courseId) == 0) {
			courseList[i]->setScore(score);//�γ̳ɼ�
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CourseHelper::getSection(CString courseId, CStringArray *sectionId, CStringArray *sectionName) {
	CString response;
	CString url = GetString(IDS_BASE_URL) + GetString(IDS_URL_WK_LEARN);
	url.Replace(TEXT("`courseId`"), courseId);
	response = httpClient.Get(url);

	sectionId->RemoveAll();
	sectionName->RemoveAll();

	int pos = 0;
	int pos1 = 0;
	CString zhang;
	CString jie;
#ifdef _DEBUG
	OutputDebugString(TEXT("ȡ��ĳ�γ̵������½ڣ�\n"));
	OutputDebugString(response);
#endif
	while (1) {
		zhang = SubString(response, TEXT("<dd>"), TEXT("</dd>"), &pos);
		if (zhang.IsEmpty()) {
			zhang = SubString(response, TEXT("<dt name=\"zj\""), TEXT("</dd>"), &pos);//�������
		}
		if (zhang.IsEmpty())break;

		while (1) {
			jie = SubString(zhang, TEXT("<li"), TEXT("</li>"), &pos1);
			if (jie.IsEmpty())break;
			CString tmp;
			tmp = SubString(zhang, TEXT("span title=\""), TEXT("</span>"));
			tmp = tmp.Left(4);//ֻҪ�ڼ���
			sectionId->Add(SubString(jie, TEXT("id=\"j_"), TEXT("\">")));
			sectionName->Add(tmp + TEXT(" ") + SubString(jie, TEXT("title=\""), TEXT("\"")));
			OutputDebugString(tmp + TEXT(" ") + SubString(jie, TEXT("id=\"j_"), TEXT("\">")) + TEXT(" ") + SubString(jie, TEXT("title=\""), TEXT("\"")) + TEXT("\n"));
		}
		pos1 = 0;
	}
	return TRUE;
}

CString CourseHelper::getSectionStatus(CString courseId, CString sectionId, int batchId) {
	/*��ȡ�½ڵ�status������ʱ����ɶ��٣���Ŀ��ɶ��٣�ý��������ɶ���*/
	CString s;
	CString response;
	CString header = GetString(IDS_HEADER_REF_LEARN);
	header.Replace(TEXT("`courseId`"), courseId);
	header.Replace(TEXT("`sectionId`"), sectionId);
	httpClient.AddSendHeader(header);
	s.Format(TEXT("%d"), batchId);
	CString postData = GetString(IDS_POST_DATA_LEARN_LIST) + getScriptSessionId();
	postData.Replace(TEXT("`courseId`"), courseId);
	postData.Replace(TEXT("`sectionId`"), sectionId);
	postData.Replace(TEXT("`batchId`"), s);
	response = httpClient.Post(GetString(IDS_BASE_URL) + GetString(IDS_URL_COMMON), postData);
	response = HexStrToWChars(response);
	return response;
}

BOOL CourseHelper::backupCourse(CourseInfo *course) {
	currentCourse = course;
	return backupFile(TEXT("BinaryAnswerLibrary\\") + course->getId() + TEXT(".") + course->getName() + TEXT(".txt")) && backupFile(TEXT("TextAnswerLibrary\\") + course->getId() + TEXT(".") + course->getName() + TEXT(".txt"));
}

BOOL CourseHelper::commitCourse(CourseInfo *course) {
	currentCourse = course;
	return commitFile(TEXT("BinaryAnswerLibrary\\") + course->getId() + TEXT(".") + course->getName() + TEXT(".txt")) && commitFile(TEXT("TextAnswerLibrary\\") + course->getId() + TEXT(".") + course->getName() + TEXT(".txt"));
}

BOOL CourseHelper::restoreCourse() {
	return restoreFile(TEXT("BinaryAnswerLibrary\\") + currentCourse->getId() + TEXT(".") + currentCourse->getName() + TEXT(".txt")) && restoreFile(TEXT("TextAnswerLibrary\\") + currentCourse->getId() + TEXT(".") + currentCourse->getName() + TEXT(".txt"));
}


CString CourseHelper::getScriptSessionId() {
	CString ScriptSessionId;
	ScriptSessionId = DWRSESSIONID + TEXT("/");
	ScriptSessionId = ScriptSessionId + tokenify(getCurrentTimestamp()) + TEXT("-") + tokenify(GetRandZeroToOne()* 1E16);
	return ScriptSessionId;
}

CString CourseHelper::tokenify(long long  remainder) {
	CString tokenbuf;
	CString charmap(TEXT("1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ*$"));
	while (remainder > 0) {
		tokenbuf = tokenbuf + charmap.GetAt(remainder & 0x3F);
		remainder = remainder / 64;
	}
	return tokenbuf;
}

CString CourseHelper::sortAnswer(CString answerString, int n) {
	int *answerInt = new int[n];
	int iPos = 0;
	CString tmpString;
	if (answerString.IsEmpty()) {
		return TEXT("");
	}
	if (answerString.Left(1).Compare(TEXT(","))) {
		answerString = TEXT(",") + answerString;
	}
	if (answerString.Right(1).Compare(TEXT(","))) {
		answerString = answerString + TEXT(",");
	}
	for (int k = 0; k < n; k++) {
		tmpString = SubString(answerString, TEXT(","), TEXT(","), &iPos);
		answerInt[k] = _ttoi(tmpString);
	}
	int i, j, tmp;
	for (i = 0; i < n - 1; i++) {
		for (j = 0; j < n - i - 1; j++)	{
			if (answerInt[j] > answerInt[j + 1]) {
				tmp = answerInt[j];
				answerInt[j] = answerInt[j + 1];
				answerInt[j + 1] = tmp;
			}
		}
	}
	answerString.Empty();
	for (int k = 0; k < n; k++) {
		tmpString.Format(TEXT("%d,"), answerInt[k]);
		answerString += tmpString;
	}
	if (answerString.Right(1) = TEXT(",")) {//cut the last `,`
		answerString = answerString.Left(answerString.GetLength() - 1);
	}
	delete[] answerInt;
	return answerString;
}

CourseHelper::STATUS CourseHelper::getStatus() {
	return status;
}

CArray<CourseInfo *> *CourseHelper::getList(){
	return &courseList;
}

void CourseHelper::setStatus(STATUS status) {
	if (status == CourseHelper::STATUS::STATUS_DONE) {
		textCallback->onResult(GetString(IDS_DONE));
	}
	this->status = status;
	statusChanged = TRUE;
}

BOOL CourseHelper::isStatusChanged(){
	return statusChanged;
}

void CourseHelper::resetStatusChanged() {
	statusChanged = FALSE;
}

