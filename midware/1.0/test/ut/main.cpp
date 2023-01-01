#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <cstdint>
#include <getopt.h>
#ifdef __ANDROID__
#include <cutils/properties.h>
#endif

#include "apusysTest.h"

#define APUSYS_UT_LOOP_MAX 1000000000
#define APUSYS_UT_THREAD_MAX 20

int gUtLogLevel = 0;

static void showHelp(void)
{
    LOG_CON("==========================================================================\n");
    LOG_CON("  apusys sample test \n");
    LOG_CON("==========================================================================\n");
    LOG_CON(" prototype: apusys_test64 [-c case][-s subIdx][-l loop][-t threads]\n\n");
    LOG_CON(" important: sample.ko for apusys is nesscary\n");
    LOG_CON("--------------------------------------------------------------------------\n");
    LOG_CON("  -c --case            test case index\n");
    LOG_CON("  -s --subIdx          test case sub-index, user set [case] and [subIdx] to select test case\n");
    apusysTestCmd::showUsage();
    apusysTestMem::showUsage();
    apusysTestMisc::showUsage();
    LOG_CON("  -l --loop            (optional) number of loop count, default=1 \n");
    LOG_CON("  -t --threads         (optional) number of con-current test processes, default=1 \n");
    LOG_CON("  -v --verbose         (optional) print debug log\n");
    LOG_CON("==========================================================================\n");
    return;
}

static struct option longOptions[] =
{
    {"case", required_argument, 0, 'c'},
    {"subIdx", required_argument, 0, 's'},
    {"loop", required_argument, 0, 'l'},
    {"threads", required_argument, 0, 't'},
    {"verbose", no_argument, 0, 'v'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

int main(int argc, char * argv[])
{
    unsigned int testCase = -1;
    unsigned int subTestCase = -1;
    unsigned int loopNum = 1;
    unsigned int threadNum = 1;
    int ret = 0;
    IApusysTest * testInst = nullptr;

    while (1) {
        int c;
        int optionIdx;
        c = getopt_long(argc, argv, "c:s:l:t:vh",
            longOptions, &optionIdx);

        if (c == -1)
            break;

        switch (c) {
            case 'c':
                testCase = (unsigned int)atoi(optarg);
                break;

            case 's':
                subTestCase = (unsigned int)atoi(optarg);
                break;

            case 'l':
                loopNum = (unsigned int)atoi(optarg);
                break;

            case 't':
                threadNum = (unsigned int)atoi(optarg);
                break;

            case 'v':
                gUtLogLevel = 1;
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
    if (loopNum > APUSYS_UT_LOOP_MAX)
    {
        loopNum = APUSYS_UT_LOOP_MAX;
    }
    if (threadNum >= APUSYS_UT_THREAD_MAX)
    {
        threadNum = APUSYS_UT_THREAD_MAX;
    }
    if (testCase >= APUSYS_UT_MAX)
    {
        LOG_ERR("invalid test case idx(%d/%d) \n", testCase, APUSYS_UT_MAX-1);
        showHelp();
        return -1;
    }
    LOG_INFO("testcase(%d-%d), loopNum(%d), thread(%d)\n", testCase, subTestCase, loopNum, threadNum);

    switch (testCase) {
        case APUSYS_UT_CMD:
            testInst = new apusysTestCmd;
            break;
        case APUSYS_UT_MEM:
            testInst = new apusysTestMem;
            break;
        case APUSYS_UT_MISC:
            testInst = new apusysTestMisc;
            break;
        default:
            break;
    }

    if (testInst == nullptr)
    {
        LOG_ERR("get ut inst(%d-%d) fail\n", testCase, subTestCase);
        return -1;
    }

    if (testInst->runCase(subTestCase, loopNum, threadNum) == false)
    {
        ret = -1;
        LOG_ERR("case(%d-%d) loopNum(%d) threadNum(%d) fail\n", testCase, subTestCase, loopNum, threadNum);
    }
    else
    {
        LOG_INFO("case(%d-%d) loopNum(%d) threadNum(%d) ok\n", testCase, subTestCase, loopNum, threadNum);
    }

    delete testInst;

    return ret;
}

