#pragma once

#include "edmaEngine.h"

struct st {
	unsigned char blockSz: 6;
};

struct header_AFBC {
	unsigned int block_num;
	struct st subblocks[16*10];
};

typedef enum {
	EDMA_TEST_NONE = 0, //direct copy mode
	EDMA_TEST_SHOW_DESC = 1 ,
	EDMA_TEST_FORCE_ERROR = 1 << 1,
	EDMA_TEST_T3 = 1 << 2,
}EDMA_TEST_CONFIG;


class IEdmaTest {
private:
	EdmaEngine * mEngine;
	unsigned int mConfig;
public:
	IEdmaTest();
	virtual ~IEdmaTest();
	virtual int runCase(unsigned int loop, unsigned int caseID) = 0;
	EdmaEngine * getEngine() {return mEngine;}
	unsigned int getConfig() { return mConfig;}
	void setConfig(unsigned int config) { mConfig = config; }
};

class edmaTestCmd : public IEdmaTest {
	private:
	std::mutex apusys_mtx;
	int directlyRun(edmaTestCmd * caseInst, unsigned int loopNum, unsigned int caseID);

public:
	edmaTestCmd();
	~edmaTestCmd();
	int runCase(unsigned int loopNum, unsigned int caseID);
};
