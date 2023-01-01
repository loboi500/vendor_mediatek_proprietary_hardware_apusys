#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <assert.h>

#include "edmaTest.h"

IEdmaTest::IEdmaTest()
{
	mEngine = new EdmaEngine("edma_engine");
	mConfig = 0;
	if (mEngine == nullptr) {
		printf("create sample engine fail\n");
		assert(mEngine != nullptr);
	}
	printf("IEdmaTest::IEdmaTest() create sample edma engine !!\n");
}

IEdmaTest::~IEdmaTest()
{
	if (mEngine == nullptr) {
		printf("delete edma engine fail\n");
		return;
	}
	delete mEngine;
}

