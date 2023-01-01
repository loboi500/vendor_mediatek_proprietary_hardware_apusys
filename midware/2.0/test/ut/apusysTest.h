#pragma once

#include <bitset>
#include "apusysTestCmn.h"
#include "sampleEngine.h"
#include "apusys.h"

#define _SHOW_EXEC_PERCENT(cur, total, per) \
    { \
        unsigned int tmp = per; \
        per = (100 * cur / total); \
        if (per != tmp) \
        { \
            LOG_INFO("Test(%d/%d) Progress: %d%%(%d/%d)\n", gettid(), getpid(), per, cur, total); \
        } \
    }

#define UT_PARAM_RANDOM (0xFFFFFFFF)

enum {
    APU_MDW_UT_CMD,
    APU_MDW_UT_MEM,
    APU_MDW_UT_MISC,

    APU_MDW_UT_MAX,
};

enum {
    UT_PARAM_OPBIT_EXEC,
    UT_PARAM_OPBIT_NUMOFSUBCMD,
    UT_PARAM_OPBIT_DEPENDENCY,
    UT_PARAM_OPBIT_PRIORITY,
    UT_PARAM_OPBIT_HARDLIMIT,
    UT_PARAM_OPBIT_SOFTLIMIT,
    UT_PARAM_OPBIT_POWERSAVE,
    UT_PARAM_OPBIT_POWERPOLICY,
    UT_PARAM_OPBIT_NUMOFPACK,
    UT_PARAM_OPBIT_BOOST,
    UT_PARAM_OPBIT_TURBO,
    UT_PARAM_OPBIT_DELAYMS,
    UT_PARAM_OPBIT_FASTMEMSIZE,
    UT_PARAM_OPBIT_FASTMEMFORCE,
};

enum {
    UT_ARG_MULTIPOLICY_SCHED,
    UT_ARG_MULTIPOLICY_SINGLE,
    UT_ARG_MULTIPOLICY_MULTI,
};

enum {
    UT_ARG_EXECTYPE_SYNC,
    UT_ARG_EXECTYPE_ASYNC,
    UT_ARG_EXECTYPE_FENCE,
};

enum {
    UT_ARG_DEPENDENCY_RANDOM,
    UT_ARG_DEPENDENCY_SEQ,
    UT_ARG_DEPENDENCY_NONE,
};

struct utCmdParam {
    std::bitset<64> opBitMask;
    unsigned int delayMs;

    unsigned int dependency;
    unsigned int execType;
    unsigned int numOfSubcmd;
    unsigned int priority;
    unsigned int hardLimit;
    unsigned int softLimit;
    unsigned int multiPolicy;
    bool powerSave;
    unsigned int powerPolicy;

    unsigned int numOfPack;
    unsigned int boost;
    bool turbo;
    unsigned int estimateTime;
    unsigned int suggestTime;
    unsigned int bandwidth;
    unsigned int fastMemSize;
    bool fastMemForce;
};

struct testCaseParam {
    unsigned int loopCnt;
    unsigned int threadNum;

    struct utCmdParam c;
};

class apusysTest {
private:
    apusys_session_t *mSession;
    class sampleCodeGen *mCodeGen;

public:
    apusysTest();
    virtual ~apusysTest();
    apusys_session_t *getSession();
    class sampleCodeGen *getCodeGen();
    virtual bool runCase(unsigned int subCaseIdx, struct testCaseParam &param) = 0;
};

class apusysTestCmd : public apusysTest {
public:
    apusysTestCmd();
    ~apusysTestCmd();
    bool runCase(unsigned int subCaseIdx, struct testCaseParam &param);
    static void showUsage();
};

class apusysTestMem : public apusysTest {
public:
    apusysTestMem();
    ~apusysTestMem();
    bool runCase(unsigned int subCaseIdx, struct testCaseParam &param);
    static void showUsage();
};

class apusysTestMisc : public apusysTest {
public:
    apusysTestMisc();
    ~apusysTestMisc();
    bool runCase(unsigned int subCaseIdx, struct testCaseParam &param);
    static void showUsage();
};
