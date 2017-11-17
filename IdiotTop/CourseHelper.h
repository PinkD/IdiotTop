#pragma once

#include "HttpClient.h"
#include "Resource.h" 
#include "Func.h"
#include "Code.h"
#include "CourseInfo.h"
#include "Callback.h"
#include "User.h"


class CourseHelper {
public:
	enum STATUS {
		STATUS_UN_LOGIN = IDS_NOT_LOGIN,
		STATUS_LOGIN = IDS_LOGINED,
		STATUS_LIBRARY = IDS_LIBRARY,
		STATUS_ANSWER = IDS_ANSWER,
		STATUS_VIDEO = IDS_VIDEO,
		STATUS_DONE = IDS_DONE
	};

	enum LOGIN_RESULT {
		RESULT_LOGIN_OK,
		RESULT_NETWORK_FAIL,
		RESULT_LOGIN_FAILT,
		RESULT_NO_RETRY,
	};

	enum OTHER_RESULT {
		RESULT_OK,
		RESULT_FAIL
	};


public:
	CourseHelper();
	virtual ~CourseHelper();
	STATUS getStatus();
	CArray<CourseInfo *> *getList();
	BOOL isStatusChanged();
	void resetStatusChanged();

	void login(CString username, CString password, Callback<LOGIN_RESULT> *callback);
	void refreshCourse(Callback<CourseHelper::OTHER_RESULT> *callback);
	void answerAllCourse(Callback<CString> *textCallback);
	void getAllAnswerLib(Callback<CString> *textCallback);
	void cancelAnswerCourse();
	void cancelGetAnswerLib();

private:
	class EmptyTextCallback : Callback<CString> {
	public:
		virtual void onResult(CString result) {
#ifdef _DEBUG
			OutputDebugString(result);
#endif
			//ignore
		}
	};
	CArray<CourseInfo *> courseList;
	HttpClient httpClient;
	STATUS status = STATUS::STATUS_UN_LOGIN;//TODO:status change
	BOOL statusChanged = TRUE;
	CString DWRSESSIONID;
	User user;
	static BOOL stop;


	CourseInfo *currentCourse;

	Callback<CourseHelper::OTHER_RESULT> *otherCallback;
	Callback<LOGIN_RESULT> *loginCallback;
	Callback<CString> *textCallback;
	EmptyTextCallback *emptyTextCallback;

	void setStatus(STATUS status);
	void clearList();

	CString getScriptSessionId();
	CString tokenify(long long  remainder);
	CString sortAnswer(CString Answer, int n);

	BOOL refreshCourseGrade(CString courseId);
	BOOL getSection(CString courseId, CStringArray *sectionId, CStringArray *sectionName);
	CString getSectionStatus(CString courseId, CString sectionId, int batchId);
	BOOL backupCourse(CourseInfo *course);
	BOOL commitCourse(CourseInfo *course);
	BOOL restoreCourse();


	void mediaReply(CString allInfo, CString courseName, CString sectionName);
	void initPostTime(CStringArray &allTimeSection);
	void postTime(CString courseId, CString courseName, CString sectionId, CString sectionName);

	void answerCourse(CourseInfo course);
	void answerAllSection(CourseInfo course, CStringArray &AllsectionId, CStringArray &AllsectionName, CStringArray *allTimeSection);
	void answerSection(CourseInfo course, CString sectionId, CString sectionName, CStringArray *allTimeSection);

	void getCourseAnswerLib(CourseInfo course);
	void getAllSectionAnswerLib(CourseInfo course, CStringArray & allSectionId, CStringArray & alSctionName);
	void getSectionAnswerLib(CourseInfo course, CString sectionId, CString sectionName);

	static void CALLBACK loginAsync(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work);
	static void CALLBACK refreshCourseAsync(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work);
	static void CALLBACK answerAllCourseAsync(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work);
	static void CALLBACK getAllAnswerLibAsync(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work);

	static const int DELAY_TIME = 1000;
};