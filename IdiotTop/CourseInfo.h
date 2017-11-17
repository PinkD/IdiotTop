#pragma once
class CourseInfo {
public:
	CourseInfo(CString id, CString name, CString status, CString score, CString deadline);
	~CourseInfo();
	CString toString();
	CString getId();
	CString getName();
	CString getStatus();
	CString getScore();
	CString getDeadline();

	void setScore(CString score);

private:
	CString id;
	CString name;
	CString status;
	CString score;
	CString deadline;
};

