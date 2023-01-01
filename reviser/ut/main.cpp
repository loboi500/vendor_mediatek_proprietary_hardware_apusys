#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <cstdint>
#include <getopt.h>
#ifdef __ANDROID__
#include <cutils/properties.h>
#endif

#include "reviserTest.h"

#define APUSYS_UT_LOOP_MAX 1000000000
#define APUSYS_UT_THREAD_MAX 10


static void showHelp(void)
{
	printf("==========================================================================\n");
	printf("  reviser sample test \n");
	printf("==========================================================================\n");
	printf(" ex: reviser_test64 -l 1 -c 0 -s 0 -n 0\n\n");
	printf("--------------------------------------------------------------------------\n");
	printf("  -l --loop            (optional) number of loop count, default=1 \n");
	printf("  -c --case            test case index\n");
	printf("  -n --number          Use number to control subcmd number (Use 0 for random)\n");
	printf("  -s --subIdx          test case sub-index, user set [case] and [subIdx] to select test case\n");
	printf("  -x --vlm_size        vlm size\n");
	reviserTestCmd::showUsage();
	printf("==========================================================================\n");
	return;
}

static struct option longOptions[] =
{
	{"loop", required_argument, 0, 'l'},
	{"help", no_argument, 0, 'h'},
	{"case", required_argument, 0, 'c'},
	{"subIdx", required_argument, 0, 's'},
	{"number", required_argument, 0, 'n'},
	{"vlm_size", required_argument, 0, 'x'},
	{0, 0, 0, 0}
};

int main(int argc, char * argv[])
{
	unsigned int loopNum = 1;
	int ret = 0;
	IReviserTest * testInst = nullptr;
	unsigned int testCase = -1;
	unsigned int subTestCase = -1;
	unsigned int set_num = 0;
	unsigned int vlm_size = 0;

	while (1) {
		int c;
		int optionIdx;
		c = getopt_long(argc, argv, "l:c:n:s:x:h",
			longOptions, &optionIdx);

		if (c == -1)
			break;

		switch (c) {
		case 'l':
			loopNum = (unsigned int)atoi(optarg);
			break;
		case 'c':
			testCase = (unsigned int)atoi(optarg);
			break;
		case 'n':
			set_num = (unsigned int)atoi(optarg);
			break;
		case 's':
			subTestCase = (unsigned int)atoi(optarg);
			break;
		case 'x':
			vlm_size = (unsigned int)atoi(optarg);
			break;
		case 'h':
		case '?':
			showHelp();
			return 0;
		default:
			break;
		}
	}

	/* check argument */
	if (loopNum > APUSYS_UT_LOOP_MAX) {
		loopNum = APUSYS_UT_LOOP_MAX;
	}

	testInst = new reviserTestCmd;
	if (testInst == nullptr) {
		printf("get ut inst fail\n");
		return -1;
	}

	testInst->mLoopNum = loopNum;
	testInst->mSetnum = set_num;
	testInst->mVlmSize = vlm_size;
	if (testInst->runCase(subTestCase) == false) {
		ret = -1;
		printf("loopNum(%d) fail\n",loopNum);
	}
	else {
		printf("loopNum(%d) ok\n", loopNum);
	}

	delete testInst;

	return ret;
}

