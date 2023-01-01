#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <cstdint>
#include <getopt.h>

#include "apusysTest.h"

#define APUSYS_UT_LOOP_MAX (10000000)
#define APUSYS_UT_THREAD_MAX (20)

int gUtLogLevel = 0;

static void showHelp(void)
{
    LOG_CON("==========================================================================\n");
    LOG_CON("  apusys sample test \n");
    LOG_CON("==========================================================================\n");
    LOG_CON(" prototype: apusys_test64 [-c case][-s subIdx][-l loop][-t threads]\n\n");
    LOG_CON("--------------------------------------------------------------------------\n");
    LOG_CON("  -c --case            test case index\n");
    LOG_CON("  -s --subIdx          test case sub-index, user set [case] and [subIdx] to select test case\n");
    apusysTestCmd::showUsage();
    apusysTestMem::showUsage();
    apusysTestMisc::showUsage();
    LOG_CON("  -l --loop            (optional, int) number of loop count, default=1 \n");
    LOG_CON("  -t --threads         (optional, int) number of con-current test processes, default=1 \n");
    LOG_CON("  -n --numsc           (optional, int/random) number of subcmd in apusys cmd, default=1 \n");
    LOG_CON("  -e --exec            (optional, string) execution type: sync/async/fence, default=sync \n");
    LOG_CON("  -d --dependency      (optional, string) dependency setting: random/seq/none, default=random \n");
    LOG_CON("  -o --softlimit       (optional, int/random) apusys cmd deadline, default=0 \n");
    LOG_CON("  -a --hardlimit       (optional, int/random) apusys cmd abort time, default=0 \n");
    LOG_CON("  -m --multi           (optional, string)) multi policy: sched/single/multi, default:sched \n");
    LOG_CON("  -p --priority        (optional, string) apusys cmd priority, normal/high/ultra \n");
    LOG_CON("  -k --pack            (optional, int/random) number of pack cmd, default=0 \n");
    LOG_CON("  -b --boost           (optional, int/random) boost value, default=100 \n");
    LOG_CON("  -u --turbo           (optional) turbo mechansim \n");
    LOG_CON("  -y --delay           (optional, int/random) sample cmd delay time(ms), default=20 \n");
    LOG_CON("  -x --fastmemSize     (optional, int/random) number of fast mem size, default=0 \n");
    LOG_CON("  -f --fastmemForce    (optional) force cmd use fast mem \n");
    LOG_CON("  -w --powersave       (optional) power save mechanism \n");
    LOG_CON("  -r --power policy    (optional) power policy \n");
    LOG_CON("  -v --verbose         (optional) print debug log\n");
    LOG_CON("  -h --help            help information\n");
    LOG_CON("==========================================================================\n");
    return;
}

static struct option longOptions[] =
{
    {"case", required_argument, 0, 'c'},
    {"subIdx", required_argument, 0, 's'},
    {"loop", required_argument, 0, 'l'},
    {"threads", required_argument, 0, 't'},
    {"numsc", required_argument, 0, 'n'},
    {"exec", required_argument, 0, 'e'},
    {"dependency", required_argument, 0, 'd'},
    {"softlimit", required_argument, 0, 'o'},
    {"hardlimit", required_argument, 0, 'a'},
    {"multi", required_argument, 0, 'm'},
    {"priority", required_argument, 0, 'p'},
    {"pack", required_argument, 0, 'k'},
    {"boost", required_argument, 0, 'b'},
    {"turbo", no_argument, 0, 'u'},
    {"delay", required_argument, 0, 'y'},
    {"fastmemSize", required_argument, 0, 'x'},
    {"fastmemForce", required_argument, 0, 'f'},
    {"powersave", required_argument, 0, 'w'},
    {"powerpolicy", required_argument, 0, 'r'},
    {"verbose", no_argument, 0, 'v'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

int main(int argc, char * argv[])
{
    unsigned int testCase = -1;
    unsigned int subTestCase = -1;
    struct testCaseParam param;
    int ret = 0;
    apusysTest * testInst = nullptr;
    time_t testSeed = 0;

    memset(&param, 0, sizeof(param));
    param.c.opBitMask.reset();

    while (1) {
        int c;
        int optionIdx;
        c = getopt_long(argc, argv, "c:s:l:t:d:n:e:o:a:m:p:k:b:uy:x:fwr:vh",
            longOptions, &optionIdx);

        if (c == -1)
            break;

        switch (c) {
        case 'c':
            testCase = (unsigned int)atoi(optarg);
            if (testCase >= APU_MDW_UT_MAX) {
                LOG_ERR("case(%u) unknown\n", testCase);
                return -1;
            }
            break;

        case 's':
            subTestCase = (unsigned int)atoi(optarg);
            break;

        case 'l':
            param.loopCnt = (unsigned int)atoi(optarg);
            if (param.loopCnt > APUSYS_UT_LOOP_MAX) {
                LOG_WARN("limit loop(%u->%u)\n", param.loopCnt, APUSYS_UT_LOOP_MAX);
                param.loopCnt = APUSYS_UT_LOOP_MAX;
            }
            break;

        case 't':
            param.threadNum = (unsigned int)atoi(optarg);
            if (param.threadNum > APUSYS_UT_THREAD_MAX) {
                LOG_WARN("limit threads(%u->%u)\n", param.threadNum, APUSYS_UT_THREAD_MAX);
                param.threadNum = APUSYS_UT_THREAD_MAX;
            }
            break;

        case 'd':
            if (!strcmp(optarg, "random")) {
                param.c.dependency = UT_ARG_DEPENDENCY_RANDOM;
            } else if (!strcmp(optarg, "seq")) {
                param.c.dependency  = UT_ARG_DEPENDENCY_SEQ;
            } else if (!strcmp(optarg, "none")) {
                param.c.dependency  = UT_ARG_DEPENDENCY_NONE;
            } else {
                LOG_ERR("unknown dependency(%s)\n", optarg);
                return -1;
            }
            param.c.opBitMask.set(UT_PARAM_OPBIT_DEPENDENCY);
            break;

        case 'n':
            if (!strcmp(optarg, "random")) {
                param.c.numOfSubcmd = UT_PARAM_RANDOM;
            } else {
                param.c.numOfSubcmd = (unsigned int)atoi(optarg);
            }
            param.c.opBitMask.set(UT_PARAM_OPBIT_NUMOFSUBCMD);
            break;

        case 'e':
            if (!strcmp(optarg, "sync")) {
                param.c.execType = UT_ARG_EXECTYPE_SYNC;
            } else if (!strcmp(optarg, "async")) {
                param.c.execType = UT_ARG_EXECTYPE_ASYNC;
            } else if (!strcmp(optarg, "fence")) {
                param.c.execType = UT_ARG_EXECTYPE_FENCE;
            } else {
                LOG_ERR("unknown exec type(%s)\n", optarg);
                return -1;
            }
            param.c.opBitMask.set(UT_PARAM_OPBIT_EXEC);
            break;

        case 'o':
            if (!strcmp(optarg, "random"))
                param.c.softLimit = UT_PARAM_RANDOM;
            else
                param.c.softLimit = (unsigned int)atoi(optarg);
            param.c.opBitMask.set(UT_PARAM_OPBIT_SOFTLIMIT);
            break;

        case 'a':
            if (!strcmp(optarg, "random"))
                param.c.hardLimit = UT_PARAM_RANDOM;
            else
                param.c.hardLimit = (unsigned int)atoi(optarg);
            param.c.opBitMask.set(UT_PARAM_OPBIT_HARDLIMIT);
            break;

        case 'm':
            if (!strcmp(optarg, "sched")) {
                param.c.execType = UT_ARG_MULTIPOLICY_SCHED;
            } else if (!strcmp(optarg, "single")) {
                param.c.execType = UT_ARG_MULTIPOLICY_SINGLE;
            } else if (!strcmp(optarg, "multi")) {
                param.c.execType = UT_ARG_MULTIPOLICY_MULTI;
            } else {
                LOG_ERR("unknown exec type(%s)\n", optarg);
                return -1;
            }
            //param.c.opBitMask.set(UT_PARAM_OPBIT_MULTIPOLICY);
            break;

        case 'p':
            if (!strcmp(optarg, "normal")) {
                param.c.opBitMask = 20;
            } else if (!strcmp(optarg, "high")) {
                param.c.opBitMask = 10;
            } else if (!strcmp(optarg, "ultra")) {
                param.c.opBitMask = 0;
            } else {
                LOG_ERR("unknown exec type(%s)\n", optarg);
                return -1;
            }
            param.c.opBitMask.set(UT_PARAM_OPBIT_PRIORITY);
            break;

        case 'k':
            if (!strcmp(optarg, "random"))
                param.c.numOfPack = UT_PARAM_RANDOM;
            else
                param.c.numOfPack= (unsigned int)atoi(optarg);
            param.c.opBitMask.set(UT_PARAM_OPBIT_NUMOFPACK);
            break;

        case 'b':
            if (!strcmp(optarg, "random"))
                param.c.boost = UT_PARAM_RANDOM;
            else
                param.c.boost= (unsigned int)atoi(optarg);
            param.c.opBitMask.set(UT_PARAM_OPBIT_BOOST);
            break;

        case 'u':
            param.c.turbo = true;
            param.c.opBitMask.set(UT_PARAM_OPBIT_TURBO);
            break;

        case 'y':
            if (!strcmp(optarg, "random"))
                param.c.delayMs = UT_PARAM_RANDOM;
            else
                param.c.delayMs = (unsigned int)atoi(optarg);
            param.c.opBitMask.set(UT_PARAM_OPBIT_DELAYMS);
            break;

        case 'x':
            if (!strcmp(optarg, "random")) {
                param.c.fastMemSize = UT_PARAM_RANDOM;
            } else {
                param.c.fastMemSize = (unsigned int)atoi(optarg);
            }
            param.c.opBitMask.set(UT_PARAM_OPBIT_FASTMEMSIZE);
            break;

        case 'f':
            param.c.fastMemForce = true;
            param.c.opBitMask.set(UT_PARAM_OPBIT_FASTMEMFORCE);
            break;

        case 'w':
            param.c.powerSave= true;
            param.c.opBitMask.set(UT_PARAM_OPBIT_POWERSAVE);
            break;

        case 'r':
            param.c.powerPolicy = (unsigned int)atoi(optarg);
            param.c.opBitMask.set(UT_PARAM_OPBIT_POWERPOLICY);
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

    testSeed = time(NULL);
    if (testSeed < 0)
        return -1;

    srand(testSeed);
    param.loopCnt = param.loopCnt > 0 ? param.loopCnt : 1;
    param.threadNum = param.threadNum > 0 ? param.threadNum : 1;
    param.c.numOfSubcmd = param.c.numOfSubcmd > 0 ? param.c.numOfSubcmd : 1;
    LOG_INFO("testcase(%u-%u), loopNum(%u), thread(%u), paramMask(%llu)\n",
        testCase, subTestCase, param.loopCnt, param.threadNum, param.c.opBitMask.to_ullong());

    switch (testCase) {
    case APU_MDW_UT_CMD:
        testInst = new apusysTestCmd;
        break;
    case APU_MDW_UT_MEM:
        testInst = new apusysTestMem;
        break;
    case APU_MDW_UT_MISC:
        testInst = new apusysTestMisc;
        break;
    default:
        break;
    }

    if (testInst == nullptr) {
        LOG_ERR("get ut inst(%d-%d) fail\n", testCase, subTestCase);
        showHelp();
        return -1;
    }

    LOG_DEBUG("run case(%u-%u)\n", testCase, subTestCase);
    if (testInst->runCase(subTestCase, param) == false) {
        ret = -1;
        LOG_ERR("[result] case(%u-%u) loopNum(%u) threadNum(%u) fail\n", testCase, subTestCase, param.loopCnt, param.threadNum);
    } else {
        LOG_INFO("[result] case(%u-%u) loopNum(%u) threadNum(%u) ok\n", testCase, subTestCase, param.loopCnt, param.threadNum);
    }

    delete testInst;

    return ret;
}

