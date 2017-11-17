#include "stdafx.h"
#include "CourseInfo.h"


CourseInfo::~CourseInfo() {
}

CourseInfo::CourseInfo(CString id, CString name, CString status, CString score, CString deadline) : id(id), name(name), status(status), score(score), deadline(deadline) {
}

CString CourseInfo::toString() {
	CString json;
	json.Format(TEXT("\
	{\
		\"id\":\"%s\",\
		\"name\":\"%s\",\
		\"status\":\"%s\",\
		\"score\":\"%s\",\
		\"deadline\":\"%s\"\
	}"), id, name, status, score, deadline);
	return json;
}

CString CourseInfo::getId() {
	return id;
}

CString CourseInfo::getName() {
	return name;
}

CString CourseInfo::getStatus() {
	return status;
}

CString CourseInfo::getScore() {
	return score;
}

CString CourseInfo::getDeadline() {
	return deadline;
}

void CourseInfo::setScore(CString score) {
	this->score = score;
}

