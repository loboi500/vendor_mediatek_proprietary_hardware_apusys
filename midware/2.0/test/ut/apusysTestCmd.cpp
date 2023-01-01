#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <random>
#include <thread>
#include <unistd.h>
#include <poll.h>

#include "apusysTest.h"
#include "apusys.h"

enum {
    UT_CMD_SIMPLE,
    UT_CMD_PREEMPT,
    UT_CMD_PRIORITY,
    UT_CMD_ABORT,
    UT_CMD_FENCE,
    UT_CMD_FASTMEM,
    UT_CMD_MAXSCCTX,
    UT_CMD_PACK,

    UT_CMD_MAX,
};

#define UT_PARAM_NUMOFSUBCMD_MAX (10)
#define UT_PARAM_DELAYMS_DEFAULT (20)
#define UT_PARAM_DELAYMS_MAX (3000)
#define UT_PARAM_SOFTLIMIT_MAX (200)
#define UT_PARAM_HARDLIMIT_MAX (3000)
#define UT_PARAM_BOOST_MAX (100)

static void setCmdParamFromParam(struct testCaseParam param, apusys_cmd_t *cmd)
{
    /* priority */
    if (param.c.opBitMask.test(UT_PARAM_OPBIT_PRIORITY))
        apusysCmd_setParam(cmd, CMD_PARAM_PRIORITY, static_cast<uint64_t>(param.c.priority));
    else
        apusysCmd_setParam(cmd, CMD_PARAM_PRIORITY, 20);

    /* softlimit */
    if (param.c.opBitMask.test(UT_PARAM_OPBIT_SOFTLIMIT)) {
        if (param.c.softLimit == UT_PARAM_RANDOM)
            apusysCmd_setParam(cmd, CMD_PARAM_SOFTLIMIT, static_cast<uint64_t>(rand() % UT_PARAM_SOFTLIMIT_MAX));
        else
            apusysCmd_setParam(cmd, CMD_PARAM_SOFTLIMIT, static_cast<uint64_t>(param.c.softLimit));
    }

    /* hardlimit */
    if (param.c.opBitMask.test(UT_PARAM_OPBIT_HARDLIMIT)) {
        if (param.c.hardLimit == UT_PARAM_RANDOM)
            apusysCmd_setParam(cmd, CMD_PARAM_HARDLIMIT, static_cast<uint64_t>(rand() % UT_PARAM_HARDLIMIT_MAX));
        else
            apusysCmd_setParam(cmd, CMD_PARAM_HARDLIMIT, static_cast<uint64_t>(param.c.hardLimit));
    }

    /* power save */
    if (param.c.opBitMask.test(UT_PARAM_OPBIT_POWERSAVE))
        apusysCmd_setParam(cmd, CMD_PARAM_POWERSAVE, static_cast<uint64_t>(param.c.powerSave));

    /* power policy */
    if (param.c.opBitMask.test(UT_PARAM_OPBIT_POWERPOLICY))
        apusysCmd_setParam(cmd, CMD_PARAM_POWERPOLICY, static_cast<uint64_t>(param.c.powerPolicy));
}

static void setSubCmdParamFromParam(struct testCaseParam param, apusys_subcmd_t *subcmd, apusys_session_t *session)
{
    void* vlm = nullptr;
    uint32_t vlmSize = 0;

    /* boost */
    if (param.c.opBitMask.test(UT_PARAM_OPBIT_BOOST)) {

        if (param.c.boost == UT_PARAM_RANDOM)
            apusysSubCmd_setParam(subcmd, SC_PARAM_BOOST_VALUE, static_cast<uint64_t>(rand() % UT_PARAM_BOOST_MAX));
        else
            apusysSubCmd_setParam(subcmd, SC_PARAM_BOOST_VALUE, static_cast<uint64_t>(param.c.boost <= UT_PARAM_BOOST_MAX ? param.c.boost: UT_PARAM_BOOST_MAX));
    }

    /* turbo */
    if (param.c.opBitMask.test(UT_PARAM_OPBIT_TURBO))
        apusysSubCmd_setParam(subcmd, SC_PARAM_TURBO_BOOST, 1);

    /* fast memory force */
    if (param.c.opBitMask.test(UT_PARAM_OPBIT_FASTMEMFORCE))
        apusysSubCmd_setParam(subcmd, SC_PARAM_VLM_FORCE, static_cast<uint64_t>(param.c.fastMemForce));

    /* fast memory size */
    if (param.c.opBitMask.test(UT_PARAM_OPBIT_FASTMEMSIZE)) {
        vlm = apusysSession_memAlloc(session, 0, 0, APUSYS_MEM_TYPE_VLM, 0);
        if (vlm == nullptr) {
            LOG_DEBUG("alloc vlm fail\n");
            return;
        }
        vlmSize = apusysSession_memGetInfoFromHostPtr(session, vlm, APUSYS_MEM_INFO_GET_SIZE);
        if (vlmSize == 0) {
            LOG_DEBUG("no vlm support\n");
        } else {
            LOG_DEBUG("vlm size = %u\n", vlmSize);
        }

        if (param.c.fastMemSize == UT_PARAM_RANDOM) {
            if (vlmSize)
                apusysSubCmd_setParam(subcmd, SC_PARAM_VLM_USAGE, static_cast<uint64_t>(rand() % vlmSize));
            else
                apusysSubCmd_setParam(subcmd, SC_PARAM_VLM_USAGE, static_cast<uint64_t>(vlmSize));
        } else if (param.c.fastMemSize > vlmSize) {
            LOG_WARN("fastmem oversize, set(%lu->%lu)\n", static_cast<unsigned long>(param.c.fastMemSize), static_cast<unsigned long>(vlmSize));
            apusysSubCmd_setParam(subcmd, SC_PARAM_VLM_USAGE, static_cast<uint64_t>(vlmSize));
        } else
            apusysSubCmd_setParam(subcmd, SC_PARAM_VLM_USAGE, static_cast<uint64_t>(param.c.fastMemSize));

        apusysSession_memFree(session, vlm);
    }

}

static void setSampleCmdParam(struct testCaseParam param, class sampleCodeGen *cg, void *cmdBuf)
{
    if (cg->createSampleCmd(cmdBuf)) {
        LOG_ERR("create sample cmdbuf fail\n");
        return;
    }

    if (param.c.delayMs == 0)
        cg->setCmdProp(cmdBuf, SAMPLE_CMD_DELAYMS, static_cast<uint64_t>(UT_PARAM_DELAYMS_DEFAULT));
    else if (param.c.delayMs == UT_PARAM_RANDOM)
        cg->setCmdProp(cmdBuf, SAMPLE_CMD_DELAYMS, static_cast<uint64_t>(rand()% UT_PARAM_DELAYMS_MAX));
    else if (param.c.delayMs > UT_PARAM_DELAYMS_MAX)
        cg->setCmdProp(cmdBuf, SAMPLE_CMD_DELAYMS, static_cast<uint64_t>(UT_PARAM_DELAYMS_MAX));
    else
        cg->setCmdProp(cmdBuf, SAMPLE_CMD_DELAYMS, static_cast<uint64_t>(param.c.delayMs));
}

static int setCmdDependency(struct testCaseParam param, apusys_cmd_t *cmd, std::vector<apusys_subcmd_t *>subCmdList, unsigned int numSc)
{
    unsigned int numPack = 0, i = 0, j = 0, numDependency = 0;
    int ret = 0;

    if (numPack > subCmdList.size())
        return -EINVAL;
    numPack = subCmdList.size() - numSc;

    switch (param.c.dependency) {
    case UT_ARG_DEPENDENCY_RANDOM:
        for (i = 1; i < numSc; i++) {
            numDependency = static_cast<unsigned int>(rand() % i);
            for (j = i-1; j >= i-numDependency; j--) {
                ret = apusysCmd_setDependencyEdge(cmd, subCmdList.at(j), subCmdList.at(i));
                if (ret)
                    goto out;
            }
        }
        break;
    case UT_ARG_DEPENDENCY_SEQ:
        for (i = 1; i < numSc; i++) {
            ret = apusysCmd_setDependencyEdge(cmd, subCmdList.at(i-1), subCmdList.at(i));
            if (ret)
                goto out;
        }
        break;
    case UT_ARG_DEPENDENCY_NONE:
    default:
        break;
    }

    /* pack dependency */
    for (i = 0; i <numPack; i++) {
        ret = apusysCmd_setDependencyPack(cmd, subCmdList.at(i), subCmdList.at(numSc + i));
        if (ret)
            goto out;
    }

out:
    return ret;
}

static unsigned int getNumOfSubcmd(struct testCaseParam param)
{
    unsigned int numSc = 0;

    if (param.c.numOfSubcmd == UT_PARAM_RANDOM)
        numSc = rand() % UT_PARAM_NUMOFSUBCMD_MAX;
    else if (!param.c.numOfSubcmd)
        numSc = 1;
    else if (param.c.numOfSubcmd > UT_PARAM_NUMOFSUBCMD_MAX)
        numSc = UT_PARAM_NUMOFSUBCMD_MAX;
    else
        numSc = param.c.numOfSubcmd;

    if (!numSc)
        numSc = 1;

    return numSc;
}

static unsigned int getNumOfPack(struct testCaseParam param, unsigned int numSc)
{
    unsigned int numPack = 0;

    if (param.c.numOfPack == UT_PARAM_RANDOM)
        numPack = rand() % (numSc + 1);
    else if (param.c.numOfPack > numSc)
        numPack = numSc;
    else
        numPack = param.c.numOfPack;

    return numPack;
}

static void utCmdThreadRun(apusysTestCmd *inst, struct testCaseParam param, int *ret)
{
    apusys_cmd_t *cmd = nullptr;
    apusys_subcmd_t *tmpSubCmd = nullptr;
    std::vector<apusys_subcmd_t *> subCmdList;
    void *tmpSampleCmdBuf = nullptr;
    std::vector<void *> sampleCmdBufList;
    apusys_session_t *session = inst->getSession();
    unsigned int numSc = 0, numPack = 0, i = 0, j = 0, sampleCmdSize = 0, per = 0;
    std::vector<int> dList;

    /* clear list */
    sampleCmdBufList.clear();
    subCmdList.clear();
    dList.clear();

    /* get cmd gen */
    sampleCmdSize = inst->getCodeGen()->getSampleCmdSize();

    cmd = apusysSession_createCmd(session);
    if (cmd == nullptr)
        return;

    setCmdParamFromParam(param, cmd);

    numSc = getNumOfSubcmd(param);
    numPack = getNumOfPack(param, numSc);
    LOG_DEBUG("numSc(%u), numPack(%u)\n", numSc, numPack);

    /* setup subcmds */
    for (i = 0; i < numSc + numPack; i++) {
        tmpSubCmd = apusysCmd_createSubcmd(cmd, APUSYS_DEVICE_SAMPLE);
        if (tmpSubCmd == nullptr) {
            LOG_ERR("create subcmd fail(%u)\n", i);
            goto failCreateSubCmd;
        }

        /* allocate cmd buffer */
        tmpSampleCmdBuf = apusysSession_cmdBufAlloc(session, sampleCmdSize, 128);
        if (tmpSampleCmdBuf == nullptr)
            goto failCmdBufAlloc;

        /* set sample cmd */
        setSampleCmdParam(param, inst->getCodeGen(), tmpSampleCmdBuf);

        /* add cmdbuf to subcmd */
        apusysSubCmd_addCmdBuf(tmpSubCmd, tmpSampleCmdBuf, APUSYS_CB_BIDIRECTIONAL);

        /* setup apusys subcmd param */
        setSubCmdParamFromParam(param, tmpSubCmd, session);

        /* push to list */
        sampleCmdBufList.push_back(tmpSampleCmdBuf);
        subCmdList.push_back(tmpSubCmd);
    }

    /* setup dependency */
    if (setCmdDependency(param, cmd, subCmdList,numSc)) {
        LOG_ERR("set dependency fail\n");
        goto failSetDependency;
    }

    /* build cmd */
    if (apusysCmd_build(cmd)) {
        LOG_ERR("build cmd fail\n");
        goto failBuildCmd;
    }

    /* run cmd and check result */
    for (i = 0; i < param.loopCnt; i++) {
        _SHOW_EXEC_PERCENT(i, param.loopCnt, per);

        if (param.c.execType == UT_ARG_EXECTYPE_SYNC) {
            if (apusysCmd_run(cmd)) {
                LOG_ERR("run cmd fail(%u)\n", i);
                goto failRunCmd;
            }
         } else if (param.c.execType == UT_ARG_EXECTYPE_ASYNC) {
            if (apusysCmd_runAsync(cmd)) {
                LOG_ERR("run cmd async fail(%u)\n", i);
                goto failRunCmd;
            }
            if (apusysCmd_wait(cmd)) {
                LOG_ERR("wait cmd fail(%u)\n", i);
                goto failRunCmd;
            }
         } else {
             LOG_ERR("(%u)not support exec(%u)\n", i, param.c.execType);
             goto failRunCmd;
         }

        /* check result */
        for (j = 0; j < sampleCmdBufList.size(); j++) {
            if (inst->getCodeGen()->checkSampleCmdDone(sampleCmdBufList.at(j))) {
                LOG_ERR("sample cmd(%u) not done(%u)\n", j, i);
                goto failRunCmd;
            }
        }
    }

    *ret = true;

    LOG_DEBUG("\n");

failRunCmd:
failBuildCmd:
failSetDependency:
failCmdBufAlloc:
    while (!sampleCmdBufList.empty()) {
        tmpSampleCmdBuf = sampleCmdBufList.back();
        if (apusysSession_cmdBufFree(session, tmpSampleCmdBuf))
            LOG_ERR("free cmdbuf fail\n");
        sampleCmdBufList.pop_back();
    }

failCreateSubCmd:
    apusysSession_deleteCmd(session, cmd);
}

static bool utCmdTrigger(apusysTestCmd *inst, struct testCaseParam &param)
{
    std::vector<int> retVec;
    std::vector<std::thread> threadVec;
    unsigned int i = 0;
    bool ret = true;

    retVec.resize(param.threadNum);
    threadVec.clear();

    for (i = 0; i < param.threadNum; i++) {
        retVec[i] = false;
        threadVec.push_back(std::thread(utCmdThreadRun, inst, param, &retVec[i]));
    }

    for (i = 0; i < threadVec.size(); i ++)
        threadVec.at(i).join();

    for (i = 0; i < retVec.size(); i ++) {
        if (retVec.at(i) != true) {
            LOG_ERR("thread(%u) fail\n", i);
            ret = false;
        }
    }

    return ret;
}

static bool utCmdSimple(apusysTestCmd *inst, struct testCaseParam &param)
{
    LOG_INFO("loop(%u) thread(%u)\n", param.loopCnt, param.threadNum);

    return utCmdTrigger(inst, param);
}

static bool utCmdPreempt(apusysTestCmd *inst, struct testCaseParam &param)
{
    std::vector<int> retVec;
    std::vector<std::thread> threadVec;
    unsigned int i = 0, defaultThreadNum = 3;
    unsigned int defaultSoftlimit = 66, normalDelay = 200, deadlineDelay = 100;
    bool ret = true;

    /* setup default param */
    param.threadNum = param.threadNum > defaultThreadNum ? param.threadNum : defaultThreadNum;
    param.c.opBitMask.set(UT_PARAM_OPBIT_SOFTLIMIT);
    param.c.softLimit = 0;
    param.c.opBitMask.set(UT_PARAM_OPBIT_DELAYMS);

    retVec.resize(param.threadNum);
    threadVec.clear();

    LOG_INFO("loop(%u) thread(%u)\n", param.loopCnt, param.threadNum);

    /* trigger n-1 thread w/o softlimit */
    param.c.softLimit = 0;
    param.c.delayMs = normalDelay;
    LOG_DEBUG("normal task(%u) softlimit(%u) delay(%u)\n", param.threadNum - 1, param.c.softLimit, param.c.delayMs);
    for (i = 0; i < param.threadNum - 1; i++) {
        retVec[i] = false;
        threadVec.push_back(std::thread(utCmdThreadRun, inst, param, &retVec[i]));
    }

    /* trigger 1 thread w/ softlimit */
    param.c.softLimit = defaultSoftlimit;
    param.c.delayMs = deadlineDelay;
    param.loopCnt *= 2;
    LOG_DEBUG("deadline task(%u) softlimit(%u) delay(%u)\n", i, param.c.softLimit, param.c.delayMs);
    retVec[i] = false;
    threadVec.push_back(std::thread(utCmdThreadRun, inst, param, &retVec[i]));

    /* wait thread done */
    for (i = 0; i < threadVec.size(); i ++)
        threadVec.at(i).join();

    /* check return value */
    for (i = 0; i < retVec.size(); i ++) {
        if (retVec.at(i) != true) {
            LOG_ERR("thread(%u) fail\n", i);
            ret = false;
        }
    }

    return ret;
}

static bool utCmdPriority(apusysTestCmd *inst, struct testCaseParam &param)
{
    std::vector<int> retVec;
    std::vector<std::thread> threadVec;
    unsigned int i = 0, j = 0, t = 0, per = 0, coreNum = 0, loopCnt = 0;
    unsigned int triggerGapUs = 20 * 1000;
    bool ret = true;

    LOG_INFO("loop(%u) thread(%u)\n", param.loopCnt, param.threadNum);

    coreNum = apusysSession_queryDeviceNum(inst->getSession(), APUSYS_DEVICE_SAMPLE);

    loopCnt = param.loopCnt;
    param.threadNum = 1;
    param.loopCnt = 1;

    /* disable softlimit */
    param.c.opBitMask.reset(UT_PARAM_OPBIT_SOFTLIMIT);
    param.c.softLimit = 0;
    /* enable delayms/priority */
    param.c.opBitMask.set(UT_PARAM_OPBIT_DELAYMS);
    param.c.opBitMask.set(UT_PARAM_OPBIT_PRIORITY);

    for (t = 0; t < loopCnt; t++) {
        _SHOW_EXEC_PERCENT(t, loopCnt, per);

        retVec.clear();
        threadVec.clear();
        retVec.resize(3 * coreNum);

        /* trigger n normal cmd to occupied dummy device */
        param.c.delayMs = 500;
        param.c.priority = APUSYS_PRIORITY_MEDIUM;
        for (i = 0; i < coreNum; i++) {
            retVec[i] = false;
            threadVec.push_back(std::thread(utCmdThreadRun, inst, param, &retVec[i]));
        }
        usleep(triggerGapUs);

        /* trigger n normal cmd to execute */
        param.c.delayMs = 200;
        param.c.priority = APUSYS_PRIORITY_MEDIUM;
        for (; i < 2*coreNum; i++) {
            retVec[i] = false;
            threadVec.push_back(std::thread(utCmdThreadRun, inst, param, &retVec[i]));
        }
        usleep(triggerGapUs);

        /* trigger n high cmd to execute */
        param.c.delayMs = 200;
        param.c.priority = APUSYS_PRIORITY_HIGH;
        for (; i < 3*coreNum; i++) {
            retVec[i] = false;
            threadVec.push_back(std::thread(utCmdThreadRun, inst, param, &retVec[i]));
        }

        /* wait thread done */
        for (i = i - 1; i >= threadVec.size() - coreNum; i--)
            threadVec.at(i).join();

        /* check 2nd normal cmd, should be false becaused not done */
        for (j = i; j >= threadVec.size() - 2*coreNum; j--) {
            if (retVec.at(j) != false) {
                LOG_ERR("normal already done, priority no effect\n");
                ret = false;
            }
        }

        for (j = 0; j <= i; j++)
            threadVec.at(j).join();

        /* check return value */
        for (i = 0; i < retVec.size(); i++) {
            if (retVec.at(i) != true) {
                LOG_ERR("thread(%u) fail\n", i);
                ret = false;
            }
        }

        if (ret == false) {
            LOG_ERR("loop(%u) fail\n", t);
            break;
        }
    }

    return ret;
}

static bool utCmdAbort(apusysTestCmd *inst, struct testCaseParam &param)
{
    unsigned int abortHardLimit = 50, abortDelayms = 100;
    unsigned int t = 0, loopCnt = 0, per = 0;
    bool ret = true;

    param.c.opBitMask.set(UT_PARAM_OPBIT_DELAYMS);
    param.c.opBitMask.set(UT_PARAM_OPBIT_HARDLIMIT);
    loopCnt = param.loopCnt;
    param.loopCnt = 1;

    LOG_DEBUG("fix delayMs(%u->%u)\n", param.c.delayMs, abortDelayms);
    param.c.delayMs = abortDelayms;
    LOG_DEBUG("fix hardLimit(%u->%u)\n", param.c.hardLimit, abortHardLimit);
    param.c.hardLimit = abortHardLimit;

    LOG_INFO("loop(%u) thread(%u)\n", param.loopCnt, param.threadNum);

    /* should be fail */
    for (t = 0; t < loopCnt; t++) {
        _SHOW_EXEC_PERCENT(t, loopCnt, per);
        ret = utCmdTrigger(inst, param);
        if (ret == true)
            break;
    }

    return !ret;
}

static int waitSyncFile(int fd, int timeoutMs)
{
    int ret = 0;
    struct pollfd fds;

    memset(&fds, 0, sizeof(fds));
    fds.fd = fd;
    fds.events = POLLIN;
    do {
        ret = poll(&fds, 1, timeoutMs);
        if (ret > 0) {
            if (fds.revents & (POLLERR | POLLNVAL)) {
                errno = EINVAL;
                return -EINVAL;
            }

            return 0;
        } else if (ret == 0) {
            errno = ETIME;
            return -ETIME;
        }
    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));

    return ret;
}

static bool utCmdFence(apusysTestCmd *inst, struct testCaseParam &param)
{
    apusys_session_t *session = nullptr;
    std::vector<apusys_cmd_t *>cmdList;
    apusys_cmd_t *cmd = nullptr;
    std::vector<apusys_subcmd_t *>subcmdList;
    apusys_subcmd_t *subcmd = nullptr;
    std::vector<void *>cmdbufList;
    void *cmdbuf = nullptr;
    unsigned int subCmdSize = 0, i = 0;
    int ret = -EINVAL, fenceA = 0, fenceB = 0, fenceC = 0, fenceD = 0;

    /* get sample command from sample's codegen */
    subCmdSize = inst->getCodeGen()->getSampleCmdSize();
    if (!subCmdSize) {
        LOG_ERR("invalid sample cmd size\n");
        return false;
    }

    session = inst->getSession();

    /* create 3 cmd */
    for (i = 0; i < 3; i++) {
        cmd = apusysSession_createCmd(session);
        if (cmd == nullptr) {
            LOG_ERR("create cmd(%u) fail\n", i);
            goto freeCmd;
        }
        cmdList.push_back(cmd);
    }

    /* create 3 subcmd */
    for (i = 0; i< cmdList.size(); i++) {
        subcmd = apusysCmd_createSubcmd(cmdList.at(i), APUSYS_DEVICE_SAMPLE);
        if (subcmd == nullptr) {
            LOG_ERR("create subcmd(%u) fail\n", i);
            goto freeCmd;
        }
        subcmdList.push_back(subcmd);
    }

    /* create and setup cmdbuf */
    for (i = 0; i < subcmdList.size(); i++) {
        cmdbuf = apusysSession_cmdBufAlloc(session, subCmdSize, 256);
        if (cmdbuf == nullptr) {
            LOG_ERR("alloc cmdbuf fail\n");
            goto freeCmdBuf;
        }
        cmdbufList.push_back(cmdbuf);

        /* setup cmd content */
        if (inst->getCodeGen()->createSampleCmd(cmdbuf)) {
            LOG_ERR("fill sample cmd fail\n");
            goto freeCmdBuf;
        }

        /* setup subcmd delay ms */
        if (param.c.delayMs && param.c.delayMs < 2000)
            inst->getCodeGen()->setCmdProp(cmdbuf, SAMPLE_CMD_DELAYMS, param.c.delayMs);

        /* add cmdbuf to subcmd */
        if (apusysSubCmd_addCmdBuf(subcmdList.at(i), cmdbuf, APUSYS_CB_BIDIRECTIONAL)) {
            LOG_ERR("add cmdbuf into subcmd fail\n");
            goto freeCmdBuf;
        }
    }

    fenceA = apusysCmd_runFence(cmdList.at(0), 0, 0);
    if (fenceA < 0) {
        LOG_ERR("run cmd(A) fail(%d)\n", fenceA);
        goto freeCmdBuf;
    }

    fenceB = apusysCmd_runFence(cmdList.at(1), fenceA, 0);
    if (fenceB < 0) {
        LOG_ERR("run cmd(B) fail(%d)\n", fenceB);
        goto freeCmdBuf;
    }

    fenceC = apusysCmd_runFence(cmdList.at(2), fenceB, 0);
    if (fenceC < 0) {
        LOG_ERR("run cmd(C) fail(%d)\n", fenceC);
        goto freeCmdBuf;
    }
    /* trigger cmd(2) 2 time to verify repeated run */
    fenceD = apusysCmd_runFence(cmdList.at(2), fenceC, 0);
    if (fenceD < 0) {
        LOG_ERR("run cmd(C) 2 times fail(%d)\n", fenceD);
        goto freeCmdBuf;
    }

    if (waitSyncFile(fenceD, 10*1000)) {
        LOG_ERR("wait fence fail\n");
        goto freeCmdBuf;
    }

    close(fenceD);
    close(fenceC);
    close(fenceB);
    close(fenceA);

    for (i = 0; i < cmdbufList.size(); i++) {
        if (inst->getCodeGen()->checkSampleCmdDone(cmdbufList.at(i))) {
            LOG_ERR("check cmdbuf(%u) fail\n", i);
            ret = false;
        }
    }

    ret = 0;

freeCmdBuf:
    while (!cmdbufList.empty()) {
        cmdbuf = cmdbufList.back();
        cmdbufList.pop_back();
        apusysSession_cmdBufFree(session, cmdbuf);
    }
freeCmd:
    while (!cmdList.empty()) {
        cmd = cmdList.back();
        cmdList.pop_back();
        apusysSession_deleteCmd(session, cmd);
    }

    if (ret)
        return false;

    return true;
}

static bool utCmdFastmem(apusysTestCmd *inst, struct testCaseParam &param)
{
    unsigned int defaultMemSize = 128*1024;

    LOG_INFO("loop(%u) thread(%u)\n", param.loopCnt, param.threadNum);

    param.c.opBitMask.set(UT_PARAM_OPBIT_FASTMEMSIZE);
    if (!param.c.fastMemSize)
        param.c.fastMemSize = defaultMemSize;

    return utCmdTrigger(inst, param);
}

static bool utCmdMaxScCtx(apusysTestCmd *inst, struct testCaseParam &param)
{
    apusys_session_t *session = nullptr;
    apusys_cmd_t *cmd = nullptr;
    std::vector<apusys_subcmd_t *>subcmdList;
    apusys_subcmd_t *subcmd = nullptr;
    std::vector<void *>cmdbufList;
    void *cmdbuf = nullptr;
    unsigned int subCmdSize = 0, i = 0, maxSc = 64;
    int ret = -EINVAL;

    /* get sample command from sample's codegen */
    subCmdSize = inst->getCodeGen()->getSampleCmdSize();
    if (!subCmdSize) {
        LOG_ERR("invalid sample cmd size\n");
        return false;
    }

    session = inst->getSession();

    /* create cmd */
    cmd = apusysSession_createCmd(session);
    if (cmd == nullptr) {
        LOG_ERR("create cmd(%u) fail\n", i);
        goto freeCmd;
    }

    /* create 64 subcmd */
    for (i = 0; i < maxSc; i++) {
        subcmd = apusysCmd_createSubcmd(cmd, APUSYS_DEVICE_SAMPLE);
        if (subcmd == nullptr) {
            LOG_ERR("create subcmd(%u) fail\n", i);
            goto freeCmd;
        }

        if (apusysSubCmd_setParam(subcmd, SC_PARAM_VLM_CTX, i)) {
            LOG_ERR("set #%u sc ctx fail\n", i);
            goto freeCmd;
        }

        subcmdList.push_back(subcmd);
    }

    /* create and setup cmdbuf */
    for (i = 0; i < subcmdList.size(); i++) {
        cmdbuf = apusysSession_cmdBufAlloc(session, subCmdSize, 256);
        if (cmdbuf == nullptr) {
            LOG_ERR("alloc cmdbuf fail\n");
            goto freeCmdBuf;
        }
        cmdbufList.push_back(cmdbuf);

        /* setup cmd content */
        if (inst->getCodeGen()->createSampleCmd(cmdbuf)) {
            LOG_ERR("fill sample cmd fail\n");
            goto freeCmdBuf;
        }

        /* setup subcmd delay ms */
        if (param.c.delayMs && param.c.delayMs < 2000)
            inst->getCodeGen()->setCmdProp(cmdbuf, SAMPLE_CMD_DELAYMS, param.c.delayMs);

        /* add cmdbuf to subcmd */
        if (apusysSubCmd_addCmdBuf(subcmdList.at(i), cmdbuf, APUSYS_CB_BIDIRECTIONAL)) {
            LOG_ERR("add cmdbuf into subcmd fail\n");
            goto freeCmdBuf;
        }
    }

    ret = apusysCmd_run(cmd);
    if (ret) {
        LOG_ERR("run max cmd with sc/ctx\n");
        goto freeCmdBuf;
    }

    for (i = 0; i < cmdbufList.size(); i++) {
        if (inst->getCodeGen()->checkSampleCmdDone(cmdbufList.at(i))) {
            LOG_ERR("check cmdbuf(%u) fail\n", i);
            ret = false;
        }
    }

    ret = 0;

freeCmdBuf:
    while (!cmdbufList.empty()) {
        cmdbuf = cmdbufList.back();
        cmdbufList.pop_back();
        apusysSession_cmdBufFree(session, cmdbuf);
    }
freeCmd:
    if (cmd)
        apusysSession_deleteCmd(session, cmd);

    if (ret)
        return false;

    return true;
}

static bool utCmdPack(apusysTestCmd *inst, struct testCaseParam &param)
{
    LOG_INFO("loop(%u) thread(%u)\n", param.loopCnt, param.threadNum);

    param.c.opBitMask.set(UT_PARAM_OPBIT_NUMOFSUBCMD);
    param.c.opBitMask.set(UT_PARAM_OPBIT_NUMOFPACK);
    param.c.numOfSubcmd= UT_PARAM_RANDOM;
    param.c.numOfPack = UT_PARAM_RANDOM;

    return utCmdTrigger(inst, param);
}

static bool(*testFunc[UT_CMD_MAX])(apusysTestCmd *inst, struct testCaseParam &param) = {
    utCmdSimple,
    utCmdPreempt,
    utCmdPriority,
    utCmdAbort,
    utCmdFence,
    utCmdFastmem,
    utCmdMaxScCtx,
    utCmdPack,
};

apusysTestCmd::apusysTestCmd()
{
}

apusysTestCmd::~apusysTestCmd()
{
}

bool apusysTestCmd::runCase(unsigned int subCaseIdx, struct testCaseParam &param)
{
    if (subCaseIdx >= UT_CMD_MAX)
    {
        LOG_ERR("utCmd(%u) undefined\n", subCaseIdx);
        return false;
    }

    return testFunc[subCaseIdx](this, param);
}

void apusysTestCmd::showUsage()
{
    LOG_CON("       <-c case>               [%d] command test\n", APU_MDW_UT_CMD);
    LOG_CON("           <-s subIdx>             [%d] simple test\n", UT_CMD_SIMPLE);
    LOG_CON("                                   [%d] preemption test\n", UT_CMD_PREEMPT);
    LOG_CON("                                   [%d] priority test\n", UT_CMD_PRIORITY);
    LOG_CON("                                   [%d] abort test\n", UT_CMD_ABORT);
    LOG_CON("                                   [%d] fence execution test\n", UT_CMD_FENCE);
    LOG_CON("                                   [%d] fast mem test\n", UT_CMD_FASTMEM);
    LOG_CON("                                   [%d] max sc/ctx test\n", UT_CMD_MAXSCCTX);
    LOG_CON("                                   [%d] pack test\n", UT_CMD_PACK);

    return;
}
