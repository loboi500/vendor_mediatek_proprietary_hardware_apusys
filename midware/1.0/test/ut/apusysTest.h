#pragma once

#include "sampleEngine.h"
#include "apusysTestCmn.h"

#define _SHOW_EXEC_PERCENT(cur, total, per) \
    { \
        int tmp = per; \
        per = (100 * cur / total); \
        if (per != tmp) \
        { \
            LOG_INFO("[Test Progress] %d%%(%d/%d)\n", per, cur, total); \
        } \
    }

enum {
    APUSYS_UT_CMD,
    APUSYS_UT_MEM,
    APUSYS_UT_MISC,

    APUSYS_UT_MAX,
};

class IApusysTest {
private:
    ISampleEngine * mEngine;

public:
    IApusysTest();
    virtual ~IApusysTest();
    virtual bool runCase(unsigned int subCaseIdx, unsigned int loop, unsigned int threadNum) = 0;
    ISampleEngine * getEngine() {return mEngine;}
};

class apusysTestCmd : public IApusysTest {
public:
    apusysTestCmd();
    ~apusysTestCmd();
    bool runCase(unsigned int subCaseIdx, unsigned int loopNum, unsigned int threadNum);
    static void showUsage();
};

class apusysTestMem : public IApusysTest{
public:
    apusysTestMem();
    virtual ~apusysTestMem();
    bool runCase(unsigned int subCaseIdx, unsigned int loopNum, unsigned int threadNum);
    static void showUsage();
};

class apusysTestMisc : public IApusysTest {
public:
    apusysTestMisc();
    ~apusysTestMisc();
    bool runCase(unsigned int subCaseIdx, unsigned int loopNum, unsigned int threadNum);
    static void showUsage();
};
