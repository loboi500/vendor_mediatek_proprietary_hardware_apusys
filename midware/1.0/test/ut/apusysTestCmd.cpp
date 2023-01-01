#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <random>
#include <thread>
#include <unistd.h>

#include "apusysTest.h"
#include "apusys.h"

#define UT_SUBCMD_MAX 10

enum {
    UT_CMD_DIRECTLY,
    UT_CMD_NOCTX,
    UT_CMD_SEQUENTIAL_SYNC,
    UT_CMD_SEQUENTIAL_ASYNC,
    UT_CMD_RANDOM_SYNC,
    UT_CMD_RANDOM_ASYNC,
    UT_CMD_SIMPLE_PACK,
    UT_CMD_RANDOM_PACK,
    UT_CMD_PRIORITY,
    UT_CMD_BOOSTVAL_RUN,
    UT_CMD_MULTITHREAD_RUN,
    UT_CMD_SEQUENTIAL_CONTI_SYNC,
    UT_CMD_RANDOM_CONTI_SYNC,

    UT_CMD_MAX,
};

apusysTestCmd::apusysTestCmd()
{
    return;
}

apusysTestCmd::~apusysTestCmd()
{
    return;
}

static bool _createSampleCmd(apusysTestCmd * caseInst, IApusysCmd * apusysCmd, std::vector<std::vector<int>> &cmdOrder, std::vector<ISampleCmd *> &cmdList)
{
    unsigned int i = 0;
    ISampleCmd * cmd = nullptr;
    IApusysMem * cmdBuf = nullptr;
    std::string name;

    if (caseInst == nullptr || apusysCmd == nullptr)
    {
        LOG_ERR("invalid arguments\n");
        return false;
    }

    name.assign("command test");

    cmdList.clear();
    LOG_DEBUG("sample construct cmd size(%u), list size(%u)\n", (unsigned int )cmdOrder.size(), (unsigned int)cmdList.size());

    for (i = 0; i < cmdOrder.size(); i++)
    {
        cmd = caseInst->getEngine()->createSampleCmd();
        if (cmd == nullptr)
        {
            LOG_ERR("fail to get sample command(%d)\n", i);
            return false;
        }

        cmd->setup(name, i, 50);
        //cmdBuf = caseInst->getEngine()->getSubCmdBuf(cmd);
        cmdBuf = cmd->compile();
        if (cmdBuf == nullptr)
        {
            LOG_ERR("fail to get sample command buffer(%d)\n", i);
            return false;
        }

        if (apusysCmd->addSubCmd(cmdBuf, APUSYS_DEVICE_SAMPLE, cmdOrder.at(i), i) < 0)
        {
            LOG_ERR("fail to add command buffer to apusys command(%d)\n", i);
            return false;
        }

        cmdList.push_back(cmd);
    }

    return true;
}

static bool _getDpList(std::vector<std::vector<int>> & dpList, int num , bool random)
{
    std::vector<int> instDp;
    std::random_device rd;
    std::default_random_engine gen = std::default_random_engine(rd());
    std::uniform_int_distribution<int> dis(1, num);
    unsigned int i = 0, j = 0;
    int randomNum = 0;

    dpList.clear();

    if (random == true)
    {
        /* random */
        for (i = 0; i < (unsigned int)num ; i++)
        {
            instDp.clear();
            if(i != 0)
            {
                randomNum = dis(gen) % i;
                LOG_DEBUG("randomNum = %d/%d\n", i, randomNum);
                for (j = i-1; j >= i-randomNum; j--)
                {
                    instDp.push_back(j);
                }
            }

            dpList.push_back(instDp);
        }
    }
    else
    {
        /* sequentail dependency list */
        for (i = 0; i < (unsigned int)num ; i++)
        {
            instDp.clear();
            if (i != 0)
            {
                instDp.push_back(i-1);
            }

            dpList.push_back(instDp);
        }
    }

    LOG_INFO("APUSYS CMD Dependency(%d): [ ", num);
    /* print dependency */
    for (i = 0; i < dpList.size(); i++)
    {
        LOG_CON(" {");
        for (j = 0; j < dpList.at(i).size(); j++)
        {
            LOG_CON("%d ", dpList.at(i).at(j));
        }
        LOG_CON("}");
    }
    LOG_CON(" ]\n");

    return true;
}

static bool _deleteSampleCmd(apusysTestCmd * caseInst, std::vector<ISampleCmd *> &cmdList)
{
    unsigned int i = 0, size = 0;
    ISampleCmd * cmd = nullptr;

    size = cmdList.size();
    for (i = 0; i < size; i++)
    {
        cmd = cmdList.back();
        cmdList.pop_back();

        if (cmd->checkDone() == false)
        {
            LOG_ERR("#%lu subcmd wasn't executed\n", (unsigned long)cmdList.size()-i-1);
            return false;
        }

        if (caseInst->getEngine()->destroySampleCmd(cmd) == false)
        {
            LOG_ERR("release sample command list(%lu/%lu) fail\n", (unsigned long)i, (unsigned long)cmdList.size());
            return false;
        }
    }

    return true;
}

static void _sampleThreadRun(apusysTestCmd * caseInst, ISampleCmd * cmd, int *flag, unsigned int loopNum)
{
    unsigned int i = 0;

    LOG_INFO("start cmd run loop(%lu)\n", (unsigned long)loopNum);

    for (i = 0; i < loopNum; i++)
    {
        if (caseInst->getEngine()->runSampleCmd(cmd) == false)
        {
            LOG_ERR("run sample cmd in thread fail\n");
            break;
        }
        else
        {
            //LOG_INFO("run sample cmd in thread ok\n");
            *flag = 1;
        }
    }

    if (i >= loopNum)
    {
        LOG_INFO("run sample cmd in thread ok\n");
    }
    return;
}

static bool syncRun(apusysTestCmd * caseInst, unsigned int loopNum, bool random)
{
    std::vector<std::vector<int>> dpList;
    IApusysCmd * apusysCmd = nullptr;
    std::vector<ISampleCmd *> cmdList;
    int num = 0, per = 0;
    unsigned int i = 0;
    bool ret = true;

    std::random_device rd;
    std::default_random_engine gen = std::default_random_engine(rd());
    std::uniform_int_distribution<int> dis(1, UT_SUBCMD_MAX);

    for (i = 0; i < loopNum; i++)
    {
        _SHOW_EXEC_PERCENT(i, (int)loopNum, per);

        apusysCmd = caseInst->getEngine()->initCmd();
        if (apusysCmd == nullptr)
        {
            LOG_ERR("init apusys command fail\n");
            ret = false;
            goto out;
        }

        dpList.clear();
        num = dis(gen);

        if (_getDpList(dpList, num, random) == false)
        {
            LOG_ERR("get dependency fail, num(%d), round(%d/%u)\n", num, i, loopNum);
            ret = false;
            goto out;
        }

        if (_createSampleCmd(caseInst, apusysCmd, dpList, cmdList) == false)
        {
            LOG_ERR("fail to create sample command list(%d/%u)\n", i, loopNum);
            ret = false;
            goto out;
        }

        LOG_INFO("run apusys cmd with %d subcmd\n", (int)apusysCmd->size());

        if (caseInst->getEngine()->runCmd(apusysCmd) == false)
        {
            LOG_ERR("run command list fail(%d/%u)\n", i, loopNum);
            ret = false;
            goto out;
        }

        if (_deleteSampleCmd(caseInst, cmdList) == false)
        {
            LOG_ERR("fail to delete sample command list(%d/%u)\n", i, loopNum);
            ret = false;
            goto out;
        }

        if (caseInst->getEngine()->destroyCmd(apusysCmd) == false)
        {
            LOG_ERR("delete apusys command fail(%d/%u)\n", i, loopNum);
            ret = false;
            goto out;
        }
    }

out:

    return ret;
}

static bool asyncRun(apusysTestCmd * caseInst, unsigned int loopNum, bool random)
{
    std::vector<std::vector<int>> dpList;
    IApusysCmd * apusysCmd = nullptr;
    std::vector<ISampleCmd *> cmdList;
    int num = 0, per = 0;
    unsigned int i = 0;
    bool ret = true;

    std::random_device rd;
    std::default_random_engine gen = std::default_random_engine(rd());
    std::uniform_int_distribution<int> dis(1, UT_SUBCMD_MAX);

    for (i = 0; i < loopNum; i++)
    {
        _SHOW_EXEC_PERCENT(i, (int)loopNum, per);

        apusysCmd = caseInst->getEngine()->initCmd();
        if (apusysCmd == nullptr)
        {
            LOG_ERR("init apusys command fail\n");
            ret = false;
            goto out;
        }

        dpList.clear();
        num = dis(gen);

        if (_getDpList(dpList, num, random) == false)
        {
            LOG_ERR("get dependency fail, num(%d), round(%d/%u)\n", num, i, loopNum);
            ret = false;
            goto out;
        }

        if (_createSampleCmd(caseInst, apusysCmd, dpList, cmdList) == false)
        {
            LOG_ERR("fail to create sample command list(%d/%u)\n", i, loopNum);
            ret = false;
            goto out;
        }

        if (caseInst->getEngine()->runCmdAsync(apusysCmd) == false)
        {
            LOG_ERR("run command async list fail(%d/%u)\n", i, loopNum);
            ret = false;
            goto out;
        }

        if (caseInst->getEngine()->waitCmd(apusysCmd) == false)
        {
            LOG_ERR("wait cmd fail(%d/%u)\n", i, loopNum);
            ret = false;
            goto out;
        }

        if (_deleteSampleCmd(caseInst, cmdList) == false)
        {
            LOG_ERR("fail to delete sample command list(%d/%u)\n", i, loopNum);
            ret = false;
            goto out;
        }

        if (caseInst->getEngine()->destroyCmd(apusysCmd) == false)
        {
            LOG_ERR("delete apusys command fail(%d/%u)\n", i, loopNum);
            ret = false;
            goto out;
        }
    }

out:

    return ret;
}

static bool noCtxRun(apusysTestCmd * caseInst, unsigned int loopNum)
{
    ISampleCmd * cmd = nullptr;
    std::string name;
    unsigned int i = 0;
    int per = 0;
    IApusysCmd * apusysCmd = nullptr;
    IApusysMem * cmdBuf = nullptr;
    std::vector<int> dp;
    struct apusysSubCmdRunInfo runInfo;

    dp.clear();
    name.assign("cmdTest:noCtxRun");

    for (i = 0; i < loopNum; i++)
    {
        _SHOW_EXEC_PERCENT(i, (int)loopNum, per);

        apusysCmd = caseInst->getEngine()->initCmd();
        if (apusysCmd == nullptr)
        {
            LOG_ERR("init apusys command fail\n");
            return false;
        }

        cmd = caseInst->getEngine()->createSampleCmd();
        if (cmd == nullptr)
        {
           LOG_ERR("cmdTest-noCtxRun: cmd path get cmd fail\n");
           return false;
        }

        if (cmd->setup(name, 0x9453, 0) == false)
        {
            return false;
        }

        if (cmd->setPriority(APUSYS_PRIORITY_NORMAL) == false)
        {
            return false;
        }
        //cmdBuf = caseInst->getEngine()->getSubCmdBuf(cmd);
        cmdBuf = cmd->compile();

        if (apusysCmd->addSubCmd(cmdBuf, APUSYS_DEVICE_SAMPLE, dp, 0) < 0)
        {
            LOG_ERR("fail to add command buffer to apusys command(%d)\n", i);
            return false;
        }

        if (caseInst->getEngine()->runCmd(apusysCmd) == false)
        {
            LOG_ERR("run command list fail(%d/%u)\n", i, loopNum);
            return false;
        }

        if (apusysCmd->getSubCmd(0) == nullptr)
        {
            LOG_ERR("get subcmd null(0), (%d/%u)\n", i, loopNum);
            return false;
        }

        if (apusysCmd->getSubCmd(0)->getRunInfo(runInfo) == false)
        {
            LOG_ERR("get run information of subcmd fail\n");
            return false;
        }
        LOG_INFO("runinfo: bandwidth(%u) tcmUsage(%u) driverTime(%u) ipTime(%u)\n",
            runInfo.bandwidth, runInfo.tcmUsage, runInfo.execTime, runInfo.ipTime);

        if (cmd->checkDone() == false)
        {
            LOG_ERR("driver done's bit is not set\n");
            return false;
        }

        if (caseInst->getEngine()->destroySampleCmd(cmd) == false)
        {
            LOG_ERR("release sample cmd fail\n");
            return false;
        }

        if (caseInst->getEngine()->destroyCmd(apusysCmd) == false)
        {
            LOG_ERR("delete apusys command fail(%d/%u)\n", i, loopNum);
            return false;
        }
    }

    return true;
}

static bool directlyRun(apusysTestCmd * caseInst, unsigned int loopNum)
{
    ISampleCmd * cmd = nullptr;
    std::string name;
    unsigned int i = 0;
    int per = 0;

    for (i=0; i<loopNum; i++)
    {
        _SHOW_EXEC_PERCENT(i, (int)loopNum, per);

        cmd = caseInst->getEngine()->createSampleCmd();
        if (cmd == nullptr)
        {
           LOG_ERR("cmdTest-directlyRun: cmd path get cmd fail\n");
           return false;
        }

        name.assign("cmdTest:directlyRun");

        if (cmd->setup(name, 0x9453, 20) == false)
        {
           goto runFalse;
        }

        if (cmd->setPriority(APUSYS_PRIORITY_NORMAL) == false)
        {
           goto runFalse;
        }

        if (caseInst->getEngine()->runSampleCmd(cmd) == false)
        {
           goto runFalse;
        }

        if (caseInst->getEngine()->destroySampleCmd(cmd) == false)
        {
            goto runFalse;
        }
    }

    return true;

runFalse:
    if (caseInst->getEngine()->destroySampleCmd(cmd) == false)
    {
        LOG_ERR("cmdTest-directlyRun: release cmd fail\n");
    }
    return false;
}

static bool packRun(apusysTestCmd * caseInst, unsigned int loopNum)
{
    std::vector<std::vector<int>> dpList;
    IApusysCmd * apusysCmd = nullptr;
    std::vector<ISampleCmd *> cmdList;
    std::vector<ISampleCmd *> packCmdList;
    ISampleCmd * packCmd = nullptr;
    IApusysMem * packCmdBuf = nullptr;
    int num = 0, j = 0, packNum = 0, per = 0;
    unsigned int i = 0;
    bool ret = true;
    std::string name;

    std::random_device rd;
    std::default_random_engine gen = std::default_random_engine(rd());
    std::uniform_int_distribution<int> dis(1, UT_SUBCMD_MAX);

    name.assign("pack command");

    for (i = 0; i < loopNum; i++)
    {
        _SHOW_EXEC_PERCENT(i, (int)loopNum, per);

        apusysCmd = caseInst->getEngine()->initCmd();
        if (apusysCmd == nullptr)
        {
            LOG_ERR("init apusys command fail\n");
            ret = false;
            goto out;
        }

        dpList.clear();
        num = dis(gen);

        if (_getDpList(dpList, num, true) == false)
        {
            LOG_ERR("get dependency fail, num(%d), round(%d/%u)\n", num, i, loopNum);
            ret = false;
            goto out;
        }

        if (_createSampleCmd(caseInst, apusysCmd, dpList, cmdList) == false)
        {
            LOG_ERR("fail to create sample command list(%d/%u)\n", i, loopNum);
            ret = false;
            goto out;
        }

        /* packing... */
        packCmdList.clear();

        packNum = dis(gen) % num;
        LOG_INFO("pack command num = %d\n", packNum);
        for (j = 0; j < packNum; j++)
        {
            packCmd = caseInst->getEngine()->createSampleCmd();
            if (packCmd == nullptr)
            {
                LOG_ERR("fail to get sample command for packing(%d/%d/%u)\n", j, i, loopNum);
                return false;
            }

            packCmdList.push_back(packCmd);

            packCmd->setup(name, i+j, 100);
            //packCmdBuf = caseInst->getEngine()->getSubCmdBuf(packCmd);
            packCmdBuf = packCmd->compile();
            if (packCmdBuf == nullptr)
            {
                LOG_ERR("fail to get sample command buffer(%d)\n", i);
                return false;
            }

            LOG_INFO("pack command to %d subCmd\n", j);
            if (apusysCmd->packSubCmd(packCmdBuf, APUSYS_DEVICE_SAMPLE, j) < 0)
            {
                LOG_ERR("pack command fail(%d)\n", j);
                return false;
            }
        }

        if (caseInst->getEngine()->runCmd(apusysCmd) == false)
        {
            LOG_ERR("run command async list fail(%d/%u)\n", i, loopNum);
            ret = false;
            goto out;
        }

        /* release packing cmd */
        for (j = packNum-1; j >= 0; j--)
        {
            packCmd = packCmdList.back();
            packCmdList.pop_back();

            if (packCmd->checkDone() == false)
            {
                LOG_ERR("packing subCmd(%d) is not set\n", j);
                return false;
            }

            if (caseInst->getEngine()->destroySampleCmd(packCmd) == false)
            {
                LOG_ERR("release pack command list(%d/%d) fail\n", j, packNum);
                return false;
            }
        }

        if (_deleteSampleCmd(caseInst, cmdList) == false)
        {
            LOG_ERR("fail to delete sample command list(%d/%u)\n", i, loopNum);
            ret = false;
            goto out;
        }

        if (caseInst->getEngine()->destroyCmd(apusysCmd) == false)
        {
            LOG_ERR("delete apusys command fail(%d/%u)\n", i, loopNum);
            ret = false;
            goto out;
        }
    }

out:

    return ret;
}

static bool simplePackRun(apusysTestCmd * caseInst, unsigned int loopNum)
{
    bool ret = true;
    int num = 2, j = 0, idx = 0, per = 0;
    unsigned int i = 0;
    std::vector<ISampleCmd*> sampleCmdList;
    IApusysCmd * apusysCmd = nullptr;
    std::vector<int> dpList;

    dpList.clear();

    for (i = 0; i < loopNum; i++)
    {
        _SHOW_EXEC_PERCENT(i, (int)loopNum, per);

        sampleCmdList.clear();
        sampleCmdList.resize(num);
        for (j = 0; j < num; j++)
        {
            sampleCmdList.at(j) = caseInst->getEngine()->createSampleCmd();
            if (sampleCmdList.at(j) == nullptr)
            {
                LOG_ERR("create sample cmd(%d) fail, rount(%lu/%lu)\n", j, (unsigned long)i, (unsigned long)loopNum);
                return false;
            }
        }

        apusysCmd = caseInst->getEngine()->initCmd();
        if (apusysCmd == nullptr)
        {
            LOG_ERR("init apusys command fail\n");
            return false;
        }

        idx = apusysCmd->addSubCmd(sampleCmdList.at(0)->compile(), APUSYS_DEVICE_SAMPLE, dpList);
        LOG_INFO("add subcmd idx = %d\n", idx);
        if (idx < 0)
        {
            LOG_ERR("add sample cmd fail\n");
            return false;
        }

        idx = apusysCmd->packSubCmd(sampleCmdList.at(1)->compile(), APUSYS_DEVICE_SAMPLE, idx);
        LOG_INFO("pack subcmd idx = %d\n", idx);
        if (idx < 0)
        {
            LOG_ERR("pack sample cmd fail\n");
            return false;
        }

        if (caseInst->getEngine()->destroyCmd(apusysCmd) == false)
        {
            LOG_ERR("delete apusys command fail(%d/%u)\n", i, loopNum);
            return false;
        }

        for (j = 0; j < num; j++)
        {
            if (caseInst->getEngine()->destroySampleCmd(sampleCmdList.at(j))== false)
            {
                LOG_ERR("destroySampleCmd sample cmd(%d) fail, rount(%lu/%lu)\n", j, (unsigned long)i ,(unsigned long)loopNum);
                return false;
            }
        }
    }

    return ret;
}

static bool priorityTest(apusysTestCmd * caseInst, unsigned int loopNum)
{
    std::vector<ISampleCmd *>sampleCmdList;
    std::string name;
    int i = 0, totalCmd = 6, per = 0, flag[6];
    unsigned int count = 0;
    bool ret = true;


    name.assign("cmdTest:priorityTest");

    sampleCmdList.resize(totalCmd);

    for (count = 0; count < loopNum; count++)
    {
        _SHOW_EXEC_PERCENT(count, (int)loopNum, per);

        memset(flag, 0, sizeof(flag));
        LOG_INFO("prority test round(%d)\n", count);
        for (i = 0; i < totalCmd; i++)
        {
            sampleCmdList.at(i) = caseInst->getEngine()->createSampleCmd();
            if (sampleCmdList.at(i) == nullptr)
            {
                LOG_ERR("get sample cmd(%d) fail\n", i);
                ret = false;
                goto runFalse;
            }

            if (sampleCmdList.at(i)->setup(name, 0x0, 1500)== false)
            {
                LOG_ERR("setup sample cmd(%d) fail\n", i);
                ret = false;
                goto runFalse;
            }

            if (sampleCmdList.at(i)->setPriority(APUSYS_PRIORITY_NORMAL) == false)
            {
                LOG_ERR("setProp sample cmd(%d) fail\n", i);
                ret = false;
                goto runFalse;
            }
        }

        for (i=totalCmd-2;i<totalCmd;i++)
        {
            if (sampleCmdList.at(i)->setPriority(APUSYS_PRIORITY_HIGH) == false)
            {
                LOG_ERR("setProp sample cmd to high priority fail\n");
                ret = false;
                goto runFalse;
            }
        }

        std::thread a0(_sampleThreadRun, caseInst, sampleCmdList.at(0), &flag[0], 1);
        std::thread a1(_sampleThreadRun, caseInst, sampleCmdList.at(1), &flag[1], 1);
        usleep(1000*200);
        std::thread a2(_sampleThreadRun, caseInst, sampleCmdList.at(2), &flag[2], 1);
        std::thread a3(_sampleThreadRun, caseInst, sampleCmdList.at(3), &flag[3], 1);
        usleep(1000*20);
        std::thread a4(_sampleThreadRun, caseInst, sampleCmdList.at(4), &flag[4], 1);
        std::thread a5(_sampleThreadRun, caseInst, sampleCmdList.at(5), &flag[5], 1);

        LOG_INFO("check status\n");
        for (i = 0; i < 20; i++)
        {
            usleep(1000*300);
            if (flag[4] == 1 && flag[5] == 1 && flag[2] == 0 && flag[3] == 0)
            {
                LOG_INFO("status ok\n");
                ret = true;
                break;
            }
            LOG_INFO("check priority status(%d/%d/%d/%d/%d/%d)\n",flag[0],flag[1],flag[2],flag[3],flag[4],flag[5]);
        }

        LOG_INFO("join threads...(%d/%d/%d/%d/%d/%d)\n",flag[0],flag[1],flag[2],flag[3],flag[4],flag[5]);
        a0.join();
        a1.join();
        a2.join();
        a3.join();
        a4.join();
        a5.join();
        LOG_INFO("join threads done\n");
    }

runFalse:
    for (i=0;i<totalCmd;i++)
    {
        if (sampleCmdList.at(i) != nullptr)
        {
            if (caseInst->getEngine()->destroySampleCmd(sampleCmdList.at(i)) == false)
            {
                LOG_ERR("release cmd(%d) fail\n", i);
            }
        }
    }

    return ret;
}

static bool boostValRun(apusysTestCmd * caseInst, unsigned int loopNum)
{
    std::vector<std::vector<int>> dpList;
    IApusysCmd * apusysCmd = nullptr;
    std::vector<ISampleCmd *> cmdList;
    bool ret = true;
    unsigned int count = 0;
    int i = 0, num = 10, per = 0;
    struct apusysSubCmdTuneParam tune;

    std::random_device rd;
    std::default_random_engine gen = std::default_random_engine(rd());
    std::uniform_int_distribution<int> dis(1, 100);

    memset(&tune, 0, sizeof(tune));
    DEBUG_TAG;
    apusysCmd = caseInst->getEngine()->initCmd();
    if (apusysCmd == nullptr)
    {
        LOG_ERR("init apusys command fail\n");
        return false;
    }

    DEBUG_TAG;
    dpList.clear();
    if (_getDpList(dpList, num, false) == false)
    {
        LOG_ERR("get dependency fail, num(%d)\n", num);
        return false;
    }

    DEBUG_TAG;
    /* allocate 10 sample cmd */
    if (_createSampleCmd(caseInst, apusysCmd, dpList, cmdList) == false)
    {
        LOG_ERR("fail to create sample command list\n");
        return false;
    }

    /* adjust boost value and run */
    for (count = 0; count < loopNum; count++)
    {
        _SHOW_EXEC_PERCENT(count, (int)loopNum, per);

        LOG_CON("[apusys_test] boostVal:[");
        for(i = 0; i < num; i++)
        {
            tune.boostVal = dis(gen);
            if (apusysCmd->getSubCmd(i) == nullptr)
            {
                LOG_CON("\n[apusys_test] subcmd(%d) null\n", i);
                ret = false;
                goto failGetSubCmd;
            }
            apusysCmd->getSubCmd(i)->setTuneParam(tune);
            LOG_CON("%d/",tune.boostVal);
        }
        LOG_CON("]\n");

        /* run */
        if (caseInst->getEngine()->runCmd(apusysCmd) == false)
        {
            LOG_ERR("run command list fail(%d/%u)\n", count, loopNum);
            return false;
        }
    }
    DEBUG_TAG;

failGetSubCmd:

    /* free sample cmd */
    if (_deleteSampleCmd(caseInst, cmdList) == false)
    {
        LOG_ERR("fail to delete sample command list\n");
        return false;
    }

    if (caseInst->getEngine()->destroyCmd(apusysCmd) == false)
    {
        LOG_ERR("delete apusys command fail\n");
        return false;
    }

    return ret;
}

static bool multiThreadDirectlyRun(apusysTestCmd * caseInst, unsigned int loopNum, unsigned int threads)
{
    std::vector<ISampleCmd *> cmdList;
    std::vector<int> flagList;
    ISampleCmd * cmd = nullptr;
    std::vector<std::thread> threadList;
    std::string name;
    unsigned int i = 0;

    /* prepare sample cmd */
    for (i = 0; i < threads; i++)
    {
        cmd = caseInst->getEngine()->createSampleCmd();
        if (cmd == nullptr)
        {
            LOG_ERR("create cmd(%d) fail\n", i);
            goto runFalse;
        }

        cmdList.push_back(cmd);

        name.assign("cmdTest:multiDirectlyRun");

        if (cmd->setup(name, 0x1111, 0) == false)
        {
           goto runFalse;
        }

        if (cmd->setPriority(APUSYS_PRIORITY_NORMAL) == false)
        {
           goto runFalse;
        }
    }

    threadList.resize(threads);
    flagList.resize(threads);

    /* run in thread */
    for (i=0; i<threads; i++)
    {
        threadList.at(i) = std::thread(_sampleThreadRun, caseInst, cmdList.at(i), &flagList.at(i), loopNum);
    }

    /* join threads */
    for (i=0; i<threads; i++)
    {
        threadList.at(i).join();
    }

    /* delete cmd */
    for (i = 0; i < cmdList.size(); i++)
    {
        if (caseInst->getEngine()->destroySampleCmd(cmdList.at(i)) == false)
        {
            LOG_ERR("cmdTest-directlyRun: release cmd(%d) fail\n", i);
        }
    }

    return true;

runFalse:
    for (i = 0; i < cmdList.size(); i++)
    {
        if (caseInst->getEngine()->destroySampleCmd(cmdList.at(i)) == false)
        {
            LOG_ERR("cmdTest-directlyRun: release cmd(%d) fail\n", i);
        }
    }
    return false;
}

static bool syncRunConti(apusysTestCmd * caseInst, unsigned int loopNum, bool random)
{
    std::vector<std::vector<int>> dpList;
    IApusysCmd * apusysCmd = nullptr;
    std::vector<ISampleCmd *> cmdList;
    int num = 0, per = 0, j = 0;
    unsigned int i = 0;
    bool ret = true;
    ISampleCmd * sampleCmd = nullptr;

    std::random_device rd;
    std::default_random_engine gen = std::default_random_engine(rd());
    std::uniform_int_distribution<int> dis(1, UT_SUBCMD_MAX);

        apusysCmd = caseInst->getEngine()->initCmd();
        if (apusysCmd == nullptr)
        {
            LOG_ERR("init apusys command fail\n");
            ret = false;
            goto out;
        }

        dpList.clear();
        num = dis(gen);

        if (_getDpList(dpList, num, random) == false)
        {
            LOG_ERR("get dependency fail, num(%d), round(%d/%u)\n", num, i, loopNum);
            ret = false;
            goto out;
        }

        if (_createSampleCmd(caseInst, apusysCmd, dpList, cmdList) == false)
        {
            LOG_ERR("fail to create sample command list(%d/%u)\n", i, loopNum);
            ret = false;
            goto out;
        }

        LOG_INFO("run apusys cmd with %d subcmd\n", (int)apusysCmd->size());
        for (i = 0; i < loopNum; i++)
        {
            _SHOW_EXEC_PERCENT(i, (int)loopNum, per);

            if (caseInst->getEngine()->runCmd(apusysCmd) == false)
            {
                LOG_ERR("run command list fail(%d/%u)\n", i, loopNum);
                ret = false;
                goto runFail;
            }

            /* last time check done exec in delete sample cmd */
            if (i < loopNum-1)
            {
                for (j = 0; j < num; j++)
                {
                    sampleCmd = cmdList.at(j);
                    if (sampleCmd->checkDone() == false)
                    {
                        LOG_ERR("#%d subcmd wasn't executed\n", num-j-1);
                        ret = false;
                        goto runFail;
                    }
                }
            }
        }

runFail:
        if (_deleteSampleCmd(caseInst, cmdList) == false)
        {
            LOG_ERR("fail to delete sample command list(%d/%u)\n", i, loopNum);
            ret = false;
            goto out;
        }

        if (caseInst->getEngine()->destroyCmd(apusysCmd) == false)
        {
            LOG_ERR("delete apusys command fail(%d/%u)\n", i, loopNum);
            ret = false;
            goto out;
        }

out:

    return ret;
}

bool apusysTestCmd::runCase(unsigned int subCaseIdx, unsigned int loopNum, unsigned int threadNum)
{
    if(loopNum == 0)
    {
        LOG_ERR("cmdTest: loop number(%u), do nothing\n", loopNum);
        return false;
    }

    switch(subCaseIdx)
    {
        case UT_CMD_DIRECTLY:
            return directlyRun(this, loopNum);

        case UT_CMD_NOCTX:
            return noCtxRun(this, loopNum);

        case UT_CMD_SEQUENTIAL_SYNC:
            return syncRun(this, loopNum, false);

        case UT_CMD_SEQUENTIAL_ASYNC:
            return asyncRun(this, loopNum, false);

        case UT_CMD_RANDOM_SYNC:
            return syncRun(this, loopNum, true);

        case UT_CMD_RANDOM_ASYNC:
            return asyncRun(this, loopNum, true);

        case UT_CMD_SIMPLE_PACK:
            return simplePackRun(this, loopNum);

        case UT_CMD_RANDOM_PACK:
            return packRun(this, loopNum);

        case UT_CMD_PRIORITY:
            return priorityTest(this, loopNum);

        case UT_CMD_BOOSTVAL_RUN:
            return boostValRun(this, loopNum);

        case UT_CMD_MULTITHREAD_RUN:
            return multiThreadDirectlyRun(this, loopNum, threadNum);

        case UT_CMD_SEQUENTIAL_CONTI_SYNC:
            return syncRunConti(this, loopNum, false);

        case UT_CMD_RANDOM_CONTI_SYNC:
            return syncRunConti(this, loopNum, true);

        default:
            LOG_ERR("cmdTest: no subtest(%u)\n", subCaseIdx);
    }

    return false;
}

void apusysTestCmd::showUsage()
{
    LOG_CON("       <-c case>               [%d] command test\n", APUSYS_UT_CMD);
    LOG_CON("           <-s subIdx>             [%d] directly test\n", UT_CMD_DIRECTLY);
    LOG_CON("                                   [%d] no mem ctx test\n", UT_CMD_NOCTX);
    LOG_CON("                                   [%d] sequential sync test\n", UT_CMD_SEQUENTIAL_SYNC);
    LOG_CON("                                   [%d] sequential async test\n", UT_CMD_SEQUENTIAL_ASYNC);
    LOG_CON("                                   [%d] random sync test\n", UT_CMD_RANDOM_SYNC);
    LOG_CON("                                   [%d] random async test\n", UT_CMD_RANDOM_ASYNC);
    LOG_CON("                                   [%d] sample packing api test\n", UT_CMD_SIMPLE_PACK);
    LOG_CON("                                   [%d] random packing test\n", UT_CMD_RANDOM_PACK);
    LOG_CON("                                   [%d] priority test\n", UT_CMD_PRIORITY);
    LOG_CON("                                   [%d] boost value run test\n", UT_CMD_BOOSTVAL_RUN);
    LOG_CON("                                   [%d] multi thread test\n", UT_CMD_MULTITHREAD_RUN);
    LOG_CON("                                   [%d] sync run continue test\n", UT_CMD_SEQUENTIAL_CONTI_SYNC);
    LOG_CON("                                   [%d] sync run continue test\n", UT_CMD_RANDOM_CONTI_SYNC);

    return;
}
