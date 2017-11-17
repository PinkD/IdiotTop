#pragma once

class User {
public:
	User() {}

	~User() {}

	CString getUsername() {
		return username;
	}

	CString getPassword() {
		return password;
	}

	void Init(CString username, CString password) {
		this->password = password;
		this->username = username;
	}

private:
	CString username;
	CString password;
};

