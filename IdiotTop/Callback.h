#pragma once

template<typename RESULT>
class Callback {
public:
	virtual void onResult(RESULT result) = 0;
};


