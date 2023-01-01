#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <random>
#include <thread>
#include <unistd.h>
#include <fcntl.h>

#include <linux/ioctl.h>

#include "apusysTest.h"

#define SAMPLE_META_DATA "0x15556"

enum {
    UT_MISC_BASIC,
    UT_MISC_CMD,
    UT_MISC_METADATA,
    UT_MISC_UCMD,
    UT_MISC_SETPOWER,
    UT_MISC_REBUILDCMD,

    UT_MISC_MAX,
};

apusysTestMisc::apusysTestMisc()
{
    return;
}

apusysTestMisc::~apusysTestMisc()
{
    return;
}

static bool basicTest(apusysTestMisc *inst, struct testCaseParam &param)
{
    void *cmdBuf = nullptr, *mem = nullptr;
    uint64_t device_va = 0;
    unsigned int testSize = 1024;
    bool ret = false;
    UNUSED(param);

    /* allocate cmdbuf */
    cmdBuf = apusysSession_cmdBufAlloc(inst->getSession(), testSize, 0);
    if (cmdBuf == nullptr)
        goto out;

    memset(cmdBuf, 0x5A, testSize);
    LOG_DEBUG("alloc command buffer(%p) ok\n", cmdBuf);

    /* allocate memory */
    mem = apusysSession_memAlloc(inst->getSession(), testSize, 0, APUSYS_MEM_TYPE_DRAM, 0);
    if (mem == nullptr)
        goto failAllocMem;

    memset(mem, 0xFF, testSize);
    LOG_DEBUG("alloc mem(%p) ok\n", mem);

    /* get device va from host va */
    device_va = apusysSession_memGetInfoFromHostPtr(inst->getSession(), mem, APUSYS_MEM_INFO_GET_DEVICE_VA);
    if (device_va)
        LOG_INFO("get device va(%p->0x%llx) ok\n", mem, (unsigned long long)device_va);
    else
        goto failGetDeviceVa;

    ret = true;

failGetDeviceVa:
    if (apusysSession_memFree(inst->getSession(), mem))
        LOG_ERR("free mem fail\n");
failAllocMem:
    if (apusysSession_cmdBufFree(inst->getSession(), cmdBuf))
        LOG_ERR("free cmd buffer fail\n");
out:
    return ret;
}

static bool simpleCmdTest(apusysTestMisc *inst, struct testCaseParam &param)
{
    apusys_session_t *session = nullptr;
    apusys_cmd_t *cmd = nullptr;
    std::vector<apusys_subcmd_t *> subCmdList;
    apusys_subcmd_t *tmpSubCmd = nullptr;
    unsigned int metaDataSize = 0, deviceNum = 0, subCmdSize = 0, total_subcmd = 5;
    unsigned int i = 0, j = 0, per = 0;
    void *metaData = nullptr, *tmpCmdBuf = nullptr;
    std::vector<void *> cmdBufList;
    bool ret = false;
    uint64_t runInfo = 0, cmdRunInfo = 0;

    /* create session */
    session = apusysSession_createInstance();
    if (session == nullptr) {
        LOG_ERR("create instance fail\n");
        goto out;
    }

    /* query device support */
    deviceNum = apusysSession_queryDeviceNum(session ,APUSYS_DEVICE_SAMPLE);
    if (deviceNum <= 0) {
        LOG_ERR("get dummy device num fail\n");
        goto failQueryDeviceNum;
    }

    /* query device meta data */
    metaDataSize = apusysSession_queryInfo(session, APUSYS_SESSION_INFO_METADATA_SIZE);
    if (metaDataSize <= 0) {
        LOG_ERR("query meta data size fail\n");
    }

    if (metaDataSize) {
        metaData = malloc(metaDataSize);
        if (metaData == nullptr) {
            LOG_ERR("alloc buffer for meta data(%u) fail\n", metaDataSize);
        }
    }

    if (apusysSession_queryDeviceMetaData(session, APUSYS_DEVICE_SAMPLE, metaData)) {
        LOG_ERR("get meta data fail\n");
    }

    /* create cmd */
    cmd = apusysSession_createCmd(session);
    if (cmd == nullptr) {
        LOG_ERR("create cmd fail\n");
        goto failCreateCmd;
    }

    /* setup cmd param */
    if (apusysCmd_setParam(cmd, CMD_PARAM_PRIORITY, APUSYS_PRIORITY_MEDIUM)) {
        LOG_ERR("set priority fail\n");
        goto failSetCmd;
    }

    /* get sample command from sample's codegen */
    subCmdSize = inst->getCodeGen()->getSampleCmdSize();

    /* setup subcmd */
    for (i = 0; i < total_subcmd; i++) {
        /* create subcmd */
        tmpSubCmd = apusysCmd_createSubcmd(cmd, APUSYS_DEVICE_SAMPLE);
        if (tmpSubCmd == nullptr) {
            LOG_ERR("create subcmd(%u) fail\n", i);
            goto failSetupSubCmd;
        }
        subCmdList.push_back(tmpSubCmd);

        /* create cmdbuf for subcmd */
        tmpCmdBuf = apusysSession_cmdBufAlloc(session, subCmdSize, 0);
        if (tmpCmdBuf == nullptr) {
            LOG_ERR("alloc cmdbuf(%u) fail\n", i);
            goto failSetupSubCmd;
        }
        cmdBufList.push_back(tmpCmdBuf);

        /* fill cmdbuf by sample's codegen */
        if (inst->getCodeGen()->createSampleCmd(tmpCmdBuf)) {
            LOG_ERR("gen sample cmd fail\n");
            goto failSetupSubCmd;
        }

        /* set delay time */
        if (inst->getCodeGen()->setCmdProp(tmpCmdBuf, SAMPLE_CMD_DELAYMS, 30)) {
            LOG_ERR("set sample cmd delay time fail\n");
            goto failSetupSubCmd;
        }

        /* add cmdbuf to subcmd */
        if (apusysSubCmd_addCmdBuf(tmpSubCmd, tmpCmdBuf, APUSYS_CB_BIDIRECTIONAL)) {
            LOG_ERR("add cmdbuf(%u) fail\n", i);
            goto failSetupSubCmd;
        }


        /* setup subcmd param */
        if (apusysSubCmd_setParam(tmpSubCmd, SC_PARAM_BOOST_VALUE, 50)) {
            LOG_ERR("set boost fail\n");
            goto failSetupSubCmd;
        }

        /* setup dependency as sequential execution */
        if (i == 0)
            continue;
        if (apusysCmd_setDependencyEdge(cmd, subCmdList.at(i-1), subCmdList.at(i))) {
            LOG_ERR("set dependency(%u->%u) fail\n", i-1, i);
            goto failSetupSubCmd;
        }
    }

    /* build cmd */
    if (apusysCmd_build(cmd)) {
        LOG_ERR("build cmd fail\n");
        goto failBuildCmd;
    }

    for (j = 0; j < param.loopCnt; j++) {
        _SHOW_EXEC_PERCENT(j, param.loopCnt, per);

        if (param.c.execType == UT_ARG_EXECTYPE_ASYNC) {
            /* execution */
            if (apusysCmd_runAsync(cmd)) {
                LOG_ERR("run cmd async fail\n");
                goto failRunCmd;
            }
            if (apusysCmd_wait(cmd)) {
                LOG_ERR("wait cmd fail\n");
                goto failRunCmd;
            }
        } else {
            /* execution */
            if (apusysCmd_run(cmd)) {
                LOG_ERR("run cmd fail\n");
                goto failRunCmd;
            }
        }

        for (i = 0; i < cmdBufList.size(); i++) {
            /* check execution result by sample codegen */
            if (inst->getCodeGen()->checkSampleCmdDone(cmdBufList.at(i))) {
                LOG_ERR("sample cmd(%u) not done(%u)\n", i, j);
                goto failSampleCmd;
            }

            /* check subcmd exec information */
            runInfo = apusysSubCmd_getRunInfo(subCmdList.at(i), SC_RUNINFO_IPTIME);
            if (runInfo <= 25*1000 || runInfo > 50*1000) {
                LOG_ERR("check subcmd(%u) ip time(%llu) fail\n", i, static_cast<unsigned long long>(runInfo));
                goto failSampleCmd;
            }
            LOG_DEBUG("iptime(%llu)\n", static_cast<unsigned long long>(runInfo));

            runInfo = apusysSubCmd_getRunInfo(subCmdList.at(i), SC_RUNINFO_DRIVERTIME);
            if (runInfo <= 25*1000 || runInfo > 100*1000) {
                LOG_ERR("check subcmd(%u) driver time(%llu) fail\n", i, static_cast<unsigned long long>(runInfo));
                goto failSampleCmd;
            }
            LOG_DEBUG("driver time(%llu)\n", static_cast<unsigned long long>(runInfo));

            runInfo = apusysSubCmd_getRunInfo(subCmdList.at(i), SC_RUNINFO_BANDWIDTH);
            if (runInfo > 1) {
                LOG_ERR("check subcmd(%u) bw(%llu) fail\n", i, static_cast<unsigned long long>(runInfo));
                goto failSampleCmd;
            }
            LOG_DEBUG("bw(%llu)\n", static_cast<unsigned long long>(runInfo));

            runInfo = apusysSubCmd_getRunInfo(subCmdList.at(i), SC_RUNINFO_BOOST);
            if (runInfo != 50) {
                LOG_ERR("check subcmd(%u) boost(%llu) fail\n", i, static_cast<unsigned long long>(runInfo));
                goto failSampleCmd;
            }
            LOG_DEBUG("boost(%llu)\n", static_cast<unsigned long long>(runInfo));

            runInfo = apusysSubCmd_getRunInfo(subCmdList.at(i), SC_RUNINFO_VLMUSAGE);
            if (runInfo != 0) {
                LOG_ERR("check subcmd(%u) tcm usage(%llu) fail\n", i, static_cast<unsigned long long>(runInfo));
                goto failSampleCmd;
            }
            LOG_DEBUG("tcm usage(%llu)\n", static_cast<unsigned long long>(runInfo));
        }

        /* check cmd exec information */
        cmdRunInfo = apusysCmd_getRunInfo(cmd, CMD_RUNINFO_STATUS);
        if (cmdRunInfo) {
            LOG_ERR("check cmd status(%llu) fail\n", static_cast<unsigned long long>(cmdRunInfo));
            goto failSampleCmd;
        }
        LOG_DEBUG("cmd status(%llu)\n", static_cast<unsigned long long>(cmdRunInfo));

        cmdRunInfo = apusysCmd_getRunInfo(cmd, CMD_RUNINFO_TOTALTIME_US);
        if (cmdRunInfo <= 30*1000) {
            LOG_ERR("check cmd total(%llu) time fail\n", static_cast<unsigned long long>(cmdRunInfo));
        }
        LOG_DEBUG("cmd total time(%llu) us\n", static_cast<unsigned long long>(cmdRunInfo));
    }


    ret = true;

failSampleCmd:
failRunCmd:
failBuildCmd:
failSetupSubCmd:
    /* delete cmdbuf list */
    while (!cmdBufList.empty()) {
        tmpCmdBuf = cmdBufList.back();
        apusysSession_cmdBufFree(session, tmpCmdBuf);
        cmdBufList.pop_back();
    }
failSetCmd:
    apusysSession_deleteCmd(session, cmd);
failCreateCmd:
    free(metaData);
failQueryDeviceNum:
    apusysSession_deleteInstance(session);
out:
    return ret;
}

static bool metaDataTest(apusysTestMisc *inst, struct testCaseParam &param)
{
    apusys_session_t *session = nullptr;
    std::string meta;
    unsigned int metaDataSize = 0, deviceNum = 9;
    void *metaData = nullptr;
    bool ret = true;
    UNUSED(inst);
    UNUSED(param);

    /* create session */
    session = apusysSession_createInstance();
    if (session == nullptr)
        goto out;

    /* query device support */
    deviceNum = apusysSession_queryDeviceNum(session ,APUSYS_DEVICE_SAMPLE);
    if (deviceNum <= 0) {
        LOG_ERR("device(%d) support %u cores\n", APUSYS_DEVICE_SAMPLE, deviceNum);
        return false;
    }

    /* query device meta data */
    metaDataSize = apusysSession_queryInfo(session, APUSYS_SESSION_INFO_METADATA_SIZE);
    if (metaDataSize <= 0) {
        LOG_ERR("device(%d) not support meta data size\n", APUSYS_DEVICE_SAMPLE);
        return false;
    }

    metaData = malloc(metaDataSize);
    if (metaData == nullptr)
        return false;

    if (apusysSession_queryDeviceMetaData(session, APUSYS_DEVICE_SAMPLE, metaData)) {
        LOG_ERR("device(%d) not support meta data size\n", APUSYS_DEVICE_SAMPLE);
        ret = false;
        goto out;
    }

    if (strncmp(SAMPLE_META_DATA, static_cast<const char *>(metaData), metaDataSize)) {
        LOG_ERR("meta data not match(%s/%s)\n", SAMPLE_META_DATA, static_cast<char *>(metaData));
        ret = false;
        goto out;
    }

    LOG_INFO("device(%d) support meta data(%u/%s)\n",
        APUSYS_DEVICE_SAMPLE, metaDataSize, static_cast<char *>(metaData));
out:
    free(metaData);
    return ret;
}

static bool uCmdTest(apusysTestMisc *inst, struct testCaseParam &param)
{
    struct sample_ucmd {
        unsigned long long magic;
        int cmd_idx;

        int u_write;
    };

    UNUSED(param);
    apusys_session_t *session = nullptr;
    struct sample_ucmd *ucmd = nullptr;
    int ret = 0;

    session = inst->getSession();
    ucmd = static_cast<struct sample_ucmd *>(apusysSession_cmdBufAlloc(session, sizeof(*ucmd), 16));
    if (!ucmd) {
        LOG_ERR("alloc buffer for ucmd fail\n");
        return false;
    }
    memset(ucmd, 0, sizeof(*ucmd));
    ucmd->cmd_idx = 0x66;
    ucmd->magic = 0x15556;
    ucmd->u_write = 0;
    ret = apusysSession_sendUserCmd(session, APUSYS_DEVICE_SAMPLE, ucmd);
    if (!ret) {
        if (ucmd->u_write != 0x1234)
            ret = -EINVAL;
    } else {
        LOG_ERR("send user cmd fail\n");
    }
    apusysSession_cmdBufFree(session, ucmd);

    if (ret)
        return false;

    return true;
}

static bool setPowerTest(apusysTestMisc *inst, struct testCaseParam &param)
{
    apusys_session_t *session = nullptr;
    int ret = 0;
    UNUSED(param);

    session = inst->getSession();
    ret = apusysSession_setDevicePower(session, APUSYS_DEVICE_SAMPLE, 0, 100);
    if (ret)
        return false;

    return true;
}

static bool rebuildCmdTest(apusysTestMisc *inst, struct testCaseParam &param)
{
    apusys_session_t *session = nullptr;
    apusys_cmd_t *cmd = nullptr;
    apusys_subcmd_t *subcmd = nullptr;
    unsigned int subCmdSize = 0, i = 0;
    void *cmdbuf = nullptr;
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
    if (cmd == nullptr)
        goto out;

    subcmd = apusysCmd_createSubcmd(cmd, APUSYS_DEVICE_SAMPLE);
    if (subcmd == nullptr) {
        LOG_ERR("create sample subcmd fail\n");
        goto freeCmd;
    }

    cmdbuf = apusysSession_cmdBufAlloc(session, subCmdSize, 256);
    if (cmdbuf == nullptr) {
        LOG_ERR("alloc cmdbuf fail\n");
        goto freeCmd;
    }

    if (inst->getCodeGen()->createSampleCmd(cmdbuf)) {
        LOG_ERR("fill sample cmd fail\n");
        goto freeCmdBuf;
    }

    /* add cmdbuf to subcmd */
    if (apusysSubCmd_addCmdBuf(subcmd, cmdbuf, APUSYS_CB_BIDIRECTIONAL)) {
        LOG_ERR("add cmdbuf into subcmd fail\n");
        goto freeCmdBuf;
    }

    for (i = 0; i < param.loopCnt; i++) {
        /* build cmd */
        if (apusysCmd_build(cmd))
            goto freeCmdBuf;

        if (param.c.execType == UT_ARG_EXECTYPE_ASYNC) {
            /* execution */
            if (apusysCmd_runAsync(cmd)) {
                LOG_ERR("run cmd async fail\n");
                goto freeCmdBuf;
            }

            if (apusysCmd_wait(cmd)) {
                LOG_ERR("wait cmd fail\n");
                goto freeCmdBuf;
            }
        } else {
            /* execution */
            if (apusysCmd_run(cmd)) {
                LOG_ERR("run cmd sync fail\n");
                goto freeCmdBuf;
            }
        }

        if (inst->getCodeGen()->checkSampleCmdDone(cmdbuf)) {
            LOG_ERR("check sample return value fail\n");
            goto freeCmdBuf;
        }
    }

    ret = 0;

freeCmdBuf:
    apusysSession_cmdBufFree(session, cmdbuf);
freeCmd:
    apusysSession_deleteCmd(session, cmd);
out:
    if (ret)
        return false;

    return true;
}

static bool(*testFunc[UT_MISC_MAX])(apusysTestMisc *inst, struct testCaseParam &param) = {
    basicTest,
    simpleCmdTest,
    metaDataTest,
    uCmdTest,
    setPowerTest,
    rebuildCmdTest,
};

bool apusysTestMisc::runCase(unsigned int subCaseIdx, struct testCaseParam &param)
{
    if (subCaseIdx >= UT_MISC_MAX) {
        LOG_ERR("utMisc(%u) undefined\n", subCaseIdx);
        return false;
    }

    return testFunc[subCaseIdx](this, param);
}

void apusysTestMisc::showUsage()
{
    LOG_CON("       <-c case>               [%d] misc test\n", APU_MDW_UT_MISC);
    LOG_CON("           <-s subIdx>             [%d] basic test\n", UT_MISC_BASIC);
    LOG_CON("           <-s subIdx>             [%d] simple cmd test\n", UT_MISC_CMD);
    LOG_CON("           <-s subIdx>             [%d] meta data test\n", UT_MISC_METADATA);
    LOG_CON("           <-s subIdx>             [%d] ucmd test\n", UT_MISC_UCMD);
    LOG_CON("           <-s subIdx>             [%d] set power test\n", UT_MISC_SETPOWER);
    LOG_CON("           <-s subIdx>             [%d] rebuild cmd test\n", UT_MISC_REBUILDCMD);

    return;
}
