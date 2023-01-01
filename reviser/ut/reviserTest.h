#pragma once

#include "apusys.h"

#define RVR_PREFIX "[reviser_test]"
#define RVR_PRINT_HELPER(type, x, ...) printf(RVR_PREFIX"[%s]%s: " x "%s", type, __func__, __VA_ARGS__)
#define RVR_ERR(...)      RVR_PRINT_HELPER("error", __VA_ARGS__ ,"")
#define RVR_WARN(...)     RVR_PRINT_HELPER("warn", __VA_ARGS__ ,"")
#define RVR_INFO(...)     RVR_PRINT_HELPER("info", __VA_ARGS__ ,"")


class IReviserTest {
private:
	apusys_session_t *mSession;
public:
	unsigned int mSetnum;
	unsigned int mVlmSize;
	unsigned int mLoopNum;
	virtual bool runCase(unsigned int subCaseIdx) = 0;
	IReviserTest();
	virtual ~IReviserTest();
	apusys_session_t *getSession();
};

class reviserTestCmd : public IReviserTest {
public:
	reviserTestCmd();
	~reviserTestCmd();
	bool runCase(unsigned int subCaseIdx);
	static void showUsage();
};
