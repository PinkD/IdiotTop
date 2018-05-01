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
	courseHelper->setStatus(STATUS_LOGIN);
	return courseHelper->loginCallback->onResult(RESULT_LOGIN_OK);
#endif

	CString response;

	//JSESSIONID
	response = courseHelper->httpClient.Get(GetString(IDS_BASE_URL));
	if (courseHelper->httpClient.GetCookie().IsEmpty()) {
		return courseHelper->loginCallback->onResult(RESULT_NETWORK_FAIL);
	} else {
#ifdef _DEBUG
		OutputDebugString(TEXT("获得") + courseHelper->httpClient.GetCookie() + TEXT("\n"));
#endif
	}

	//DWRSESSIONID
	courseHelper->httpClient.AddSendHeader(TEXT("Referer"), GetString(IDS_BASE_URL) + GetString(IDS_INDEX_HTM));
	response = courseHelper->httpClient.Post(GetString(IDS_BASE_URL) + GetString(IDS_URL_GEN_ID), GetString(IDS_POST_DATA_GEN_ID));

	courseHelper->DWRSESSIONID = SubString(response, TEXT("r.handleCallback(\"0\",\"0\",\""), TEXT("\");")); 
#ifdef _DEBUG
	OutputDebugString(TEXT("获得的DWRSESSIONID：") + courseHelper->DWRSESSIONID + TEXT("\n"));
#endif
	if (courseHelper->DWRSESSIONID.IsEmpty()) {
		return courseHelper->loginCallback->onResult(RESULT_NETWORK_FAIL);
	} else {
		courseHelper->httpClient.AddCookie(TEXT("DWRSESSIONID"), courseHelper->DWRSESSIONID);
	}
#ifdef _DEBUG
	OutputDebugString(TEXT("获得的Cookies：") + courseHelper->httpClient.GetCookie() + TEXT("\n"));
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
	OutputDebugString(TEXT("登陆后返回的内容：\n") + response + TEXT("\n"));
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
	//刷新所有课程并更新在窗口上
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


	CString TD;//一列
	int TDpos = 0;
	int index = -1;

	CString TR;//一行
	int TRpos = 0;

	while (TRUE) {
		TR = SubString(response, TEXT("<tr>\\r\\n                <td>"), TEXT("</tr>\\r\\n"), &TRpos);//取出一行
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

void CourseHelper::answerCourse(Callback<CString> *textCallback, int index) {
	setStatus(STATUS_ANSWER);
	this->textCallback = textCallback;
	stop = FALSE;
	selected = index;
	SubmitThreadpoolWork(CreateThreadpoolWork(answerCourseAsync, this, NULL));
}

void CourseHelper::answerCourseAsync(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work) {//刷题子线程
	CourseHelper *courseHelper = (CourseHelper *)context;
	returnIfStop();
	courseHelper->answerCourse(*courseHelper->courseList[courseHelper->selected]);
	courseHelper->setStatus(STATUS_DONE);
}


void CourseHelper::answerCourse(CourseInfo course) {//刷某节课
	CStringArray allSectionId;//课程里面每节的ID
	CStringArray allSectionName;//课程里面每节的名字
	CStringArray allTimeSection;//课程里面没刷时间的节的名字

	if (getSection(course.getId(), &allSectionId, &allSectionName)) {//获取所有章节
		answerAllSection(course, allSectionId, allSectionName, &allTimeSection);
		returnIfStop();
		initPostTime(allTimeSection);//刷时间
		returnIfStop();
	} else {
		textCallback->onResult(course.getId() + TEXT(" ") + course.getName() + TEXT("获取课程章节失败！"));
	}
}


void CourseHelper::answerAllSection(CourseInfo course, CStringArray &allSectionId, CStringArray &allSectionName, CStringArray *allTimeSection) {//刷某一节
	if (course.getStatus() == "已结业") {
		textCallback->onResult(TEXT("\r\n") + course.getName() + TEXT("已结业"));
#ifdef _DEBUG
		OutputDebugString(course.getName() + TEXT("已结业"));
#endif
		return;
	}
	for (int i = 0; i < allSectionId.GetCount(); i++) {
		returnIfStop();
		textCallback->onResult(TEXT("开始刷：") + allSectionId[i] + TEXT(".") + allSectionName[i] + TEXT("\n"));
#ifdef _DEBUG
		OutputDebugString(TEXT("开始刷：") + allSectionId[i] + TEXT(".") + allSectionName[i] + TEXT("\n"));
#endif
		answerSection(course, allSectionId[i], allSectionName[i], allTimeSection);
		refreshCourseGrade(course.getId());
	}
#ifdef _DEBUG
	OutputDebugString(TEXT("已经刷完课程") + course.getName() + TEXT("\n"));
#endif
}

void CourseHelper::answerSection(CourseInfo course, CString sectionId, CString sectionName, CStringArray *allTimeSection) {//将某章的题一起提交
	CString Exes;//一道完整的题
	CString ExesName;//题的类型和第几题
	CString question;//问题和备选答案
	CString answer;//待提交的答案
	CString answerId;//题的ID
	CString selection;//某个选项
	CString allInfo;
	CString tmp;
	CString response;
	int Pos = 0;
	int Pos1 = 0;
	CString courseAnswer;
	CString sectionStatus;
	BOOL answered = FALSE;
	BOOL replied = FALSE;
	BOOL timed = FALSE;

	courseAnswer = readAnswerFromFile(TEXT("BinaryAnswerLibrary\\") + course.getId() + TEXT(".") + course.getName() + TEXT(".txt"));//读取课程答案
	loadSection(course.getId(), sectionId);
	sectionStatus = getSectionStatus(course.getId(), sectionId, 2);
	if (sectionStatus.Find(TEXT("OK</strong> \\r\\n                 <span class=\\\"explain_rate\\\"><a href=\\\"javascript:;\\\" onclick=\\\"atPage(\\\'时间说明")) != -1) {//时间没刷完，加入待刷队列
		timed = TRUE;
	} else {
		allTimeSection->Add(TEXT("|") + course.getId() + TEXT("|") + course.getName() + TEXT("|") + sectionId + TEXT("|") + sectionName + TEXT("|"));
	}
	if (sectionStatus.Find(TEXT("OK</strong> \\r\\n                 <span class=\\\"explain_rate\\\"><a href=\\\"javascript:;\\\" onclick=\\\"atPage(\\\'习题说明")) != -1) {//习题刷过了
		answered = TRUE;
	}
	if (sectionStatus.Find(TEXT("OK</strong> \\r\\n             <span class=\\\"explain_rate\\\"><a href=\\\"javascript:;\\\" onclick=\\\"atPage(\\\'媒材说明")) != -1) {//媒体刷完了
		replied = TRUE;
	}

	if (answered && replied && timed) {
		textCallback->onResult(TEXT("\r\n本节已完成：") + course.getName() + TEXT(" ") + sectionName);
		return;//全部刷完了就返回
	}

	//开始获取本节的所有内容，包括媒体评价和习题
	CString header = GetString(IDS_HEADER_REF_LEARN);
	header.Replace(TEXT("`courseId`"), course.getId());
	header.Replace(TEXT("`sectionId"), sectionId);
	httpClient.AddSendHeader(header);
	CString postData = GetString(IDS_POST_DATA_LEARN_STATUS) + getScriptSessionId();
	postData.Replace(TEXT("`courseId`"), course.getId());
	postData.Replace(TEXT("`sectionId`"), sectionId);
	response = httpClient.Post(GetString(IDS_BASE_URL) + GetString(IDS_URL_COMMON), postData);

	if (!replied) {
		mediaReply(response, course.getName(), sectionName);//先媒体评价
	}

	response = SubString(response, TEXT("<span class=\\\"delNum"), TEXT("</dd>"));
	response = HexStrToWChars(response);

	//提交本章的题库
	answer = SubString(courseAnswer, TEXT("<") + sectionId + TEXT(">"), TEXT("</") + sectionId + TEXT(">"));
	if (response.Find(TEXT("正确率：100%")) != -1 || answered == TRUE) {
		textCallback->onResult(TEXT("\r\n本节题已做完：") + course.getName() + TEXT(" ") + sectionName);
		return;
	}
	if (!answer.IsEmpty()) {//有题库，且未答题
		textCallback->onResult(TEXT("\r\n提交题库：") + course.getName() + TEXT(" ") + sectionName);
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
		OutputDebugString(TEXT("答题返回:") + response);
#endif
		textCallback->onResult(TEXT("\r\n等待下一章节..."));
		Sleep(CourseHelper::DELAY_TIME);//延时一段时间，保险一点
	} else {
		textCallback->onResult(TEXT("\r\n无题库：") + course.getName() + TEXT(" ") + sectionName);
	}

}

void CourseHelper::mediaReply(CString allInfo, CString courseName, CString sectionName) {
	//对课程中的媒体进行评价
	CString mediaId;
	int pos = 0;
	CString s;

	textCallback->onResult(TEXT("\r\n提交媒体评论：") + courseName + TEXT(" ") + sectionName);
	while (1) {
		returnIfStop();
		mediaId = SubString(allInfo, TEXT("parent.showMediaRight("), TEXT(")"), &pos);
		if (mediaId.IsEmpty()) {
#ifdef _DEBUG
			OutputDebugString(TEXT("本节媒体已经评价完：") + courseName + TEXT(" ") + sectionName + TEXT("\n"));
#endif
			break;
		}
#ifdef _DEBUG
		OutputDebugString(TEXT("正在提交媒体评论：") + courseName + TEXT(" ") + sectionName + TEXT(" ") + mediaId + TEXT(" "));
#endif
		CString header = GetString(IDS_HEADER_REF_MEDIA);
		header.Replace(TEXT("`mediaId`"), mediaId);
		httpClient.AddSendHeader(header);
		CString postData = GetString(IDS_POST_DATA_MEDIA) + getScriptSessionId();
		postData.Replace(TEXT("`mediaId`"), mediaId);
		s = httpClient.Post(GetString(IDS_BASE_URL) + GetString(IDS_URL_DO), postData);//提交
#ifdef _DEBUG
		OutputDebugString(TEXT("已提交，返回：") + s + "\n");
#endif
	}
}

void CourseHelper::initPostTime(CStringArray &allTimeSection) {
	//这是刷时间的线程, 客户端每15秒像服务器发送一段内容，每次发送，batchId++，当发送第四次时,客户端会再次发送内容来获取当前课程的剩余时间,这里使用N个线程来刷时间，可以大幅度加快刷时间

	setStatus(STATUS_VIDEO);
	textCallback->onResult(TEXT("开始刷时间："));

	CString section;
	CString courseId;//当前进行的课程代码
	CString courseName;//当前进行的课程名字
	CString sectionId;
	CString	sectionName;
	int iPos;

	int NumSection = allTimeSection.GetCount();
	for (int i = 0; i < NumSection; i++) {
		returnIfStop();
		//取出一个
		iPos = 0;
		section = allTimeSection[0];
		allTimeSection.RemoveAt(0);
		courseId = SubString(section, TEXT("|"), TEXT("|"), &iPos);
		courseName = SubString(section, TEXT("|"), TEXT("|"), &iPos);
		sectionId = SubString(section, TEXT("|"), TEXT("|"), &iPos);
		sectionName = SubString(section, TEXT("|"), TEXT("|"), &iPos);

		//loadSection(courseId, sectionId);
		postTime(courseId, courseName, sectionId, sectionName);
		refreshCourseGrade(courseId);
	}
}

void CourseHelper::postTime(CString courseId, CString courseName, CString sectionId, CString sectionName) {
	CString response;
	int begin = 2;
	int batchId = begin;
	CString lastTime = TEXT("---");//上次剩余时间

	CString fmt;
	while (1) {
		returnIfStop();
		//每刷一次时间都获取一次剩余时间，这样可以多个应用程序刷的时候快速刷完时间
		response = getSectionStatus(courseId, sectionId, batchId);
		if (response.Find(TEXT("OK</strong> \\r\\n                 <span class=\\\"explain_rate\\\"><a href=\\\"javascript:;\\\" onclick=\\\"atPage(\\\'时间说明")) != -1) {
			//刷完了
			textCallback->onResult(TEXT("\r\n该章节的时间刷完了：") + courseName + TEXT(" ") + sectionName);
#ifdef _DEBUG
			OutputDebugString(TEXT("该章节的时间刷完了：") + courseName + TEXT(" ") + sectionName + TEXT("\n"));
#endif
			return;
		}
		else {
			CString remainTime = SubString(response, TEXT("已学时间\\\">"), TEXT("</span>"));
			remainTime = remainTime + TEXT("/") + SubString(response, TEXT("总学习时间\\\">"), TEXT("</span>"));

			//if ((batchId - begin) % 4 == 0)//每4次
			if (lastTime.CollateNoCase(remainTime) != 0) {//当剩余时间有改变时刷新
				lastTime = remainTime;
				textCallback->onResult(TEXT("\r\n剩余时间：") + courseName + TEXT(" ") + sectionName + TEXT(" ") + remainTime);
#ifdef _DEBUG
				OutputDebugString(TEXT("剩余时间：") + courseName + TEXT(" ") + sectionName + TEXT(" ") + remainTime + TEXT("\n"));
#endif
				batchId++;
				begin = batchId;
			}
		}
		for (int j = 1; j <= 15; j++) {//延时15秒
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
			textCallback->onResult(TEXT("\r\n好像没有续成功= =(如果一直是显示这个，那就凉了)\r\n") + courseName + TEXT(" ") + sectionName);
#ifdef _DEBUG
			OutputDebugString(TEXT("POST时间出错：") + courseName + TEXT(" ") + sectionName + TEXT("\r\n"));
#endif
		} else {
			textCallback->onResult(TEXT("\r\n续了15秒：") + courseName + TEXT(" ") + sectionName);
#ifdef _DEBUG
			OutputDebugString(TEXT("POST了一次时间：") + courseName + TEXT(" ") + sectionName);
#endif
		}
#ifdef _DEBUG
		OutputDebugString(TEXT("刷时间返回的内容：\n") + response);
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
	CStringArray allSectionId;//每个课程里面每节的ID
	CStringArray allSectionName;//每个课程里面每节的名字

	if (getSection(course.getId(), &allSectionId, &allSectionName)) {//获取所有章节
		getAllSectionAnswerLib(course, allSectionId, allSectionName);
	} else {
		textCallback->onResult(course.getId() + TEXT(" ") + course.getName() + TEXT("\r\n获取课程章节失败！"));
	}
}

void CourseHelper::getAllSectionAnswerLib(CourseInfo course, CStringArray &allSectionId, CStringArray &alSctionName) {//提取某一节
	for (int i = 0; i < allSectionId.GetCount(); i++) {
		returnIfStop();
		textCallback->onResult(TEXT("开始提取该节答案：") + allSectionId[i] + TEXT(".") + alSctionName[i] + TEXT("\r\n"));
#ifdef _DEBUG
		OutputDebugString(TEXT("开始提取该节答案：") + allSectionId[i] + TEXT(".") + alSctionName[i] + TEXT("\r\n"));
#endif
		getSectionAnswerLib(course, allSectionId[i], alSctionName[i]);
	}
#ifdef _DEBUG
	OutputDebugString(TEXT("已经提取完课程") + course.getName() + TEXT("\n"));
#endif
}

void CourseHelper::getSectionAnswerLib(CourseInfo course, CString sectionId, CString sectionName) {

	CString exes;//一道完整的题
	CString exesName;//题的类型和第几题
	CString question;//问题和备选答案
	CString answer;//待提交的答案
	CString answerId;//题的ID
	CString selection;//某个选项
	CString allInfo;
	CString tmp;
	CString response;
	int Pos = 0;
	int Pos1 = 0;
	CString courseAnswer;

	//获取本章状态
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


	//将本章的题全部提取出来，用于后台的提取
	while (1) {
		returnIfStop();
		exes.Empty(); exesName.Empty(); question.Empty(); answerId.Empty(); tmp.Empty(); selection.Empty();
		Pos1 = 0;
		exes = SubString(response, TEXT("<li name=\\\"xt\\\""), TEXT("\\r\\n                   \\r\\n                 </li>"), &Pos);//每一道题
		if (exes.IsEmpty()) {
			exes = SubString(response, TEXT("<li>"), TEXT("\\r\\n\\r\\n                  \\r\\n"), &Pos);//两种可能的结尾
		}
		if (exes.IsEmpty()) {//这一节的题完了
			answer = answer.Left(answer.GetLength() - 3);//去掉末尾的&;&
			answer = TEXT("<") + sectionId + TEXT(">") + answer + TEXT("</") + sectionId + TEXT(">\r\n");
			writeAnswerToFile(TEXT("BinaryAnswerLibrary\\") + course.getId() + TEXT(".") + course.getName() + TEXT(".txt"), answer);
			textCallback->onResult(TEXT("\r\n该节的题已经提取完，下一节！"));
#ifdef _DEBUG
			OutputDebugString(TEXT("\n该节的题已经提取完，下一节！") + sectionName + TEXT("\r\n"));
#endif
			return;//到下一节！
		}
		//if (Exes.Find(TEXT("ed-ans")) == -1)continue;//判断是否回答正确，如果不正确就不要
		exesName = SubString(exes, TEXT("<h5>"), TEXT("\\r\\n"), &Pos1);//第几题
		answerId = SubString(exes, TEXT("id=\\\""), TEXT("\\\""));
		answerId = answerId.Right(answerId.GetLength() - 3);//xt_4260 dx_2182 mx_2180 pd_2187只要后面的数字
		answer = answer + answerId + TEXT("&=&");

		if (exesName.Find(TEXT("填空题")) != -1) {
			int Last = 0;
			int Pos2 = 0;
			question = SubString(exes, TEXT("<p>\\r\\n\\t\\t\\t\\t\\t\\t"), TEXT("\\r\\n                   </p>"), &Pos1);
			while (1) {//取出有几个空
				Pos2 = question.Find(TEXT("\\r\\n                             "), Last);
				if (Pos2 == -1) {
					Pos2 = question.Find(TEXT("\\r\\n\\t\\t\\t\\t\\t "), Last);
				}
				if (Pos2 == -1) {
					tmp = tmp + question.Right(question.GetLength() - Last);
					break;
				}
				tmp = tmp + question.Mid(Last, Pos2 - Last) + TEXT("【   】");
				Last = Pos2;
				answer = answer + SubString(question, TEXT("value=\\\""), TEXT("\\\" />）"), &Last) + TEXT("&,&");
				if (question.Find(TEXT("\\r\\n\\t\\t\\t\\t\\t\\t"), Last) == -1) {
					Last = Last + 6;//  \" />）的长度
				} else {
					Last = question.Find(TEXT("\\r\\n\\t\\t\\t\\t\\t\\t"), Last);
					Last = Last + 16;// \r\n\t\t\t\t\t\t的长度
				}
			}
			if (answer.Right(3) = TEXT("&,&")) {
				answer = answer.Left(answer.GetLength() - 3);//去掉末尾的&,&
			}
			question = TEXT("\r\nQuestion：") + tmp + TEXT("\r\nAnswer：") + answer + TEXT("\r\n");
			//OutputDebugString(TEXT("\nQuestion：") + Question);
		} else if (exesName.Find(TEXT("单选题")) != -1 || exesName.Find(TEXT("多选题")) != -1 || exesName.Find(TEXT("判断题")) != -1) {
			CString options;
			int n = 0;
			question = SubString(exes, TEXT("<p>"), TEXT("</p>"), &Pos1);
			//OutputDebugString(TEXT("\nQuestion：") + Question + TEXT("\nAnswer：\n"));
			question = TEXT("\r\nQuestion：") + question + TEXT("\r\nAnswer：\r\n");
			while (1) {
				selection = SubString(exes, TEXT("<li"), TEXT("\\r\\n"), &Pos1);
				if (selection.IsEmpty()) {
					break;
				}else if ((selection.Find(TEXT("checked=\\\"checked\\\"")) != -1)) {
					options = options + SubString(selection, TEXT("value=\\\""), TEXT("\\\"")) + TEXT(",");
					n++;
					//Answer = Answer + SubString(Selection, TEXT("value=\\\""), TEXT("\\\"")) + TEXT(",");
					question = question + TEXT("√");
					//OutputDebugString(TEXT("√"));
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
		textCallback->onResult(course.getId() + TEXT(".") + course.getName() + TEXT("  ") + sectionName + TEXT("\r\n") + exesName + TEXT("：") + question);
		writeAnswerToFile(TEXT("TextAnswerLibrary\\") + course.getId() + TEXT(".") + course.getName() + TEXT(".txt"), TEXT("\r\n【") + exesName + TEXT("】") + question);
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
	OutputDebugString(TEXT("获取分数：") + courseId + TEXT("  ") + score + TEXT("\n"));
#endif
	if (score.IsEmpty()) {
		return FALSE;
	}
	for (int i = 0; i < courseList.GetCount(); i++) {
		if (courseList[i]->getId().Compare(courseId) == 0) {
			courseList[i]->setScore(score);//课程成绩
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
	OutputDebugString(TEXT("取出某课程的所有章节：\n"));
	OutputDebugString(response);
#endif
	while (1) {
		zhang = SubString(response, TEXT("<dd>"), TEXT("</dd>"), &pos);
		if (zhang.IsEmpty()) {
			zhang = SubString(response, TEXT("<dt name=\"zj\""), TEXT("</dd>"), &pos);//两种情况
		}
		if (zhang.IsEmpty())break;

		while (1) {
			jie = SubString(zhang, TEXT("<li"), TEXT("</li>"), &pos1);
			if (jie.IsEmpty())break;
			CString tmp;
			tmp = SubString(zhang, TEXT("span title=\""), TEXT("</span>"));
			tmp = tmp.Left(4);//只要第几章
			sectionId->Add(SubString(jie, TEXT("id=\"j_"), TEXT("\">")));
			sectionName->Add(tmp + TEXT(" ") + SubString(jie, TEXT("title=\""), TEXT("\"")));
			OutputDebugString(tmp + TEXT(" ") + SubString(jie, TEXT("id=\"j_"), TEXT("\">")) + TEXT(" ") + SubString(jie, TEXT("title=\""), TEXT("\"")) + TEXT("\n"));
		}
		pos1 = 0;
	}
	return TRUE;
}

void CourseHelper::loadSection(CString courseId, CString sectionId) {
	CString s;
	CString response;
	CString header = GetString(IDS_HEADER_REF_LEARN);
	header.Replace(TEXT("`courseId`"), courseId);
	header.Replace(TEXT("`sectionId`"), sectionId);
	s = header.Mid(9, header.GetLength() - 9);
	s = httpClient.Get(s);
	httpClient.AddSendHeader(header);
	CString postData = GetString(IDS_POST_DATA_LOAD_PAGE) + getScriptSessionId();
	postData.Replace(TEXT("`courseId`"), courseId);
	postData.Replace(TEXT("`sectionId`"), sectionId);
	response = httpClient.Post(GetString(IDS_BASE_URL) + GetString(IDS_URL_LOAD_PAGE), postData);
	postData = GetString(IDS_POST_DATA_TOP_DH_NUM) + getScriptSessionId();
	postData.Replace(TEXT("`courseId`"), courseId);
	postData.Replace(TEXT("`sectionId`"), sectionId);
	response = httpClient.Post(GetString(IDS_BASE_URL) + GetString(IDS_URL_COMMON), postData);
}

CString CourseHelper::getSectionStatus(CString courseId, CString sectionId, int batchId) {
	/*获取章节的status，例如时间完成多少，题目完成多少，媒体评价完成多少*/
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

