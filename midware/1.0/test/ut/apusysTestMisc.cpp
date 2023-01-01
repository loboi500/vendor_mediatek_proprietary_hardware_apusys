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
#include "apusys.h"

/* set power test */
#define SAMPLE_BOOST_MAGIC 87
#define SAMPLE_OPP_MAGIC 7

/* firmware test*/
#define SAMPLE_FW_MAGIC 0x35904
#define SAMPLE_FW_PTN 0x8A

/* send usercmd test */
#define SAMPLE_USERCMD_IDX 0x66
#define SAMPLE_USERCMD_MAGIC 0x15556
struct sample_usercmd {
	unsigned long long magic;
	int cmd_idx;

	int u_write;
};

/* debugfs queue test */
#define APUSYS_DBGFS_QUEUE_PATH "/d/apusys_midware/device/vpu/queue"
#define APUSYS_SYSFS_QUEUE_PATH "/sys/devices/platform/apusys/queue/dsp_task_num"
#define APUSYS_DEV_NODE_NAME "/dev/apusys"

/* secure lock dont include header */
struct apusys_ioctl_sec {
	int dev_type;
	unsigned int core_num;

	unsigned int reserved0;
	unsigned int reserved1;
	unsigned int reserved2;
	unsigned int reserved3;
	unsigned int reserved4;
	unsigned int reserved5;
};

#define APUSYS_SECURE_IOCTL_LOCK _IOW('A', 60, int)
#define APUSYS_SECURE_IOCTL_UNLOCK _IOW('A', 61, int)

enum {
    UT_MISC_SETPOWER,
    UT_MISC_FIRMWARE,
    UT_MISC_DEVALLOC_SIMPLE,
    UT_MISC_DEVALLOC_RUN,
    UT_MISC_USERCMD,
    UT_MISC_QUEUE_LEN,
    UT_MISC_SECDEV_SIMPLE,
    UT_MISC_SECDEV_RUN,
    UT_MISC_ABORT,

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

static void threadRun(apusysTestMisc * caseInst, ISampleCmd * cmd, int *flag)
{
    if (caseInst->getEngine()->runSampleCmd(cmd) == false)
    {
        LOG_ERR("run sample cmd in thread fail\n");
    }
    else
    {
        LOG_INFO("run sample cmd in thread ok\n");
        *flag = 1;
    }

    return;
}

static bool setPowerTest(apusysTestMisc * caseInst, unsigned int loopNum)
{
    bool ret = true;
    int i = 0, deviceNum = 0;
    UNUSED(loopNum);

    deviceNum = caseInst->getEngine()->getDeviceNum(APUSYS_DEVICE_SAMPLE);
    if (deviceNum <= 0)
    {
        LOG_ERR("sample devic num = %d\n", deviceNum);
        return false;
    }

    LOG_INFO("set sample device power, total device num(%d)\n", deviceNum);

    for (i = 0; i < deviceNum; i++)
    {
        if (caseInst->getEngine()->setPower(APUSYS_DEVICE_SAMPLE, i, SAMPLE_BOOST_MAGIC) == false)
        {
            LOG_ERR("sample dev(%d) set power fail\n", i);
            return false;
        }
    }

    return ret;
}

static bool firmwareTest(apusysTestMisc * caseInst, unsigned int loopNum)
{
    bool ret = true;
    int deviceNum = 0, i = 0, j = 0, size = 1024*1024*1, per = 0;
    unsigned int count = 0;
    unsigned char *ptn = nullptr;
    IApusysMem * fwBuf = nullptr;
    IApusysFirmware * fwHnd = nullptr;
    std::string fwName;

    fwName.assign("utTest");

    deviceNum = caseInst->getEngine()->getDeviceNum(APUSYS_DEVICE_SAMPLE);
    if (deviceNum <= 0)
    {
        LOG_ERR("sample devic num = %d\n", deviceNum);
        return false;
    }

    LOG_INFO("sample device num = %d\n", deviceNum);

    for (count = 0; count < loopNum; count ++)
    {
        _SHOW_EXEC_PERCENT(count, (int)loopNum, per);

        for (i = 0; i < deviceNum; i++)
        {
            LOG_INFO("firmware test sample device(%d)\n", i);
            fwBuf = caseInst->getEngine()->memAlloc(size);
            if (fwBuf == nullptr)
            {
                LOG_ERR("allocate firmware buffer fail\n");
                return false;
            }

            fwHnd = caseInst->getEngine()->loadFirmware(APUSYS_DEVICE_SAMPLE, SAMPLE_FW_MAGIC, i, fwName, fwBuf);
            if (fwHnd == nullptr)
            {
                LOG_ERR("load firmware fail\n");
                goto runFalse;
            }

            ptn = (unsigned char *)fwBuf->va;
            /* check firmware buffer */
            LOG_INFO("check fw buffer after load firmware...\n");
            for (j = 0; j < size; j++)
            {
                if (*ptn != SAMPLE_FW_PTN)
                {
                    LOG_ERR("%p ptn not match\n", (void *)ptn);
                    break;
                }
                ptn++;
            }

            if (caseInst->getEngine()->unloadFirmware(fwHnd) == false)
            {
                LOG_ERR("unload firmware fail\n");
                goto runFalse;
            }

            ptn = (unsigned char *)fwBuf->va;
            /* check firmware buffer */
            LOG_INFO("check fw buffer after unload firmware...\n");
            for (j = 0; j < size; j++)
            {
                if (*ptn != 0x0)
                {
                    LOG_ERR("%p ptn not match\n", (void *)ptn);
                    break;
                }
                ptn++;
            }

            if (caseInst->getEngine()->memFree(fwBuf) == false)
            {
                LOG_ERR("free firmware buffer fail\n");
            }
        }
    }

    return ret;

runFalse:
    if (caseInst->getEngine()->memFree(fwBuf) == false)
    {
        LOG_ERR("free firmware buffer fail\n");
    }
    return false;
}

static bool deviceAllocSimpleTest(apusysTestMisc * caseInst, unsigned int loopNum)
{
    std::vector<IApusysDev *> devList;
    int deviceNum = 0, i = 0, per = 0;
    unsigned int count = 0;

    /* get number of devices */
    deviceNum = caseInst->getEngine()->getDeviceNum(APUSYS_DEVICE_SAMPLE);
    if (deviceNum <= 0)
    {
        LOG_ERR("sample devic num = %d\n", deviceNum);
        return false;
    }

    devList.resize(deviceNum);
    for (count = 0; count < loopNum; count++)
    {
        _SHOW_EXEC_PERCENT(count, (int)loopNum, per);
        for (i = 0; i < deviceNum; i++)
        {
            devList.at(i) = caseInst->getEngine()->devAlloc(APUSYS_DEVICE_SAMPLE);
            if (devList.at(i) == nullptr)
            {
                LOG_ERR("allocate sample device(%d) fail\n", i);
                return false;
            }
        }

        for (i = 0; i < deviceNum; i++)
        {
            if (caseInst->getEngine()->devFree(devList.at(i)))
            {
                if (devList.at(i) == nullptr)
                {
                    LOG_ERR("free sample device(%d) fail\n", i);
                    return false;
                }
            }
        }
    }

    devList.clear();

    return true;
}

static bool deviceAllocRunTest(apusysTestMisc * caseInst, unsigned int loopNum)
{
    bool ret = true;
    int deviceNum = 0, i = 0, j = 0, flag[2], per = 0;
    unsigned int count = 0;
    std::vector<ISampleCmd *> sampleCmdList;
    std::vector<IApusysDev *> devList;
    std::string name;
    int waitUs = 1000*700;

    name.assign("devAlloc Test");

    /* get number of devices */
    deviceNum = caseInst->getEngine()->getDeviceNum(APUSYS_DEVICE_SAMPLE);
    if (deviceNum <= 0)
    {
        LOG_ERR("sample devic num = %d\n", deviceNum);
        return false;
    }

    /* resize */
    sampleCmdList.resize(deviceNum);
    devList.resize(deviceNum);

    for (count = 0; count < loopNum; count ++)
    {
        _SHOW_EXEC_PERCENT(count, (int)loopNum, per);
        /* create sample cmd */
        for (i = 0; i < deviceNum ; i++)
        {
            memset(flag, 0, sizeof(flag));

            sampleCmdList.at(i)  = caseInst->getEngine()->createSampleCmd();
            if (sampleCmdList.at(i) == nullptr)
            {
                LOG_ERR("create sample cmd(%d) fail\n", i);
                return false;
            }

            if (sampleCmdList.at(i)->setup(name, 0x0, 200)== false)
            {
                LOG_ERR("setup sample cmd(%d) fail\n", i);
                ret = false;
                goto runFalse1;
            }

            if (sampleCmdList.at(i)->setPriority(APUSYS_PRIORITY_NORMAL) == false)
            {
                LOG_ERR("setProp sample cmd(%d) fail\n", i);
                ret = false;
                goto runFalse1;
            }
        }

        /* run first time */
        LOG_INFO("run first time\n");
        std::thread a0(threadRun, caseInst, sampleCmdList.at(0), &flag[0]);
        std::thread a1(threadRun, caseInst, sampleCmdList.at(1), &flag[1]);
        usleep(waitUs); // 700ms
        if (flag[0] != 1 || flag[1] != 1)
        {
            LOG_INFO("first sample cmd fail\n");
            a0.join();
            a1.join();
            ret = false;
            goto runFalse1;
        }
        a0.join();
        a1.join();

        /* allocate sample device */
        memset(flag, 0, sizeof(flag));
        LOG_INFO("allocate device...\n");

        for (j = 0; j < deviceNum; j++)
        {
            devList.at(j) = caseInst->getEngine()->devAlloc(APUSYS_DEVICE_SAMPLE);
            if (devList.at(j) == nullptr)
            {
                ret = false;
                goto runFalse1;
            }
        }

        /* run second time */
        LOG_INFO("run second time\n");
        std::thread a2(threadRun, caseInst, sampleCmdList.at(0), &flag[0]);
        std::thread a3(threadRun, caseInst, sampleCmdList.at(1), &flag[1]);

        usleep(waitUs); // 500ms

        /* check result */
        if (flag[0] == 1 || flag[1] == 1)
        {
            LOG_INFO("secod sample cmd ok, should be not executed\n");
            a2.join();
            a3.join();
            ret = false;
            goto runFalse2;
        }

        LOG_INFO("free device...\n");
        /* free sample device */
        for (j = 0; j < deviceNum; j++)
        {
            caseInst->getEngine()->devFree(devList.at(j));
            if (devList.at(j) == nullptr)
            {
                ret = false;
                goto runFalse1;
            }
        }

        usleep(waitUs); // 500ms

        if (flag[0] != 1 || flag[1] != 1)
        {
            LOG_INFO("second sample cmd fail\n");
            a0.join();
            a1.join();
            ret = false;
            goto runFalse1;
        }

        a2.join();
        a3.join();

        /* release sample cmd */
        LOG_INFO("release sample cmd\n");
        for (j=0;i<deviceNum;j++)
        {
            if (sampleCmdList.at(j) != nullptr)
            {
                if (caseInst->getEngine()->destroySampleCmd(sampleCmdList.at(j)) == false)
                {
                    LOG_ERR("release cmd(%d) fail\n", j);
                    ret = false;
                    goto runFalse0;
                }
            }
        }
    }

    return ret;

runFalse2:
    /* free sample device */
    for (j = 0; j < deviceNum; j++)
    {
        caseInst->getEngine()->devFree(devList.at(j));
        if (devList.at(j) == nullptr)
        {
            LOG_ERR("free device(%d) fail\n", j);
        }
    }
runFalse1:
    /* release sample cmd */
    for (j=0;j<deviceNum;j++)
    {
        if (sampleCmdList.at(j) != nullptr)
        {
            if (caseInst->getEngine()->destroySampleCmd(sampleCmdList.at(j)) == false)
            {
                LOG_ERR("release cmd(%d) fail\n", j);
            }
        }
    }
runFalse0:

    return false;
}

static bool sendUserCmdTest(apusysTestMisc * caseInst, unsigned int loopNum)
{
    bool ret = true;
    int i = 0, deviceNum = 0, per = 0;
    unsigned int count = 0;
    IApusysMem * mem = nullptr;
    struct sample_usercmd *ucmd = nullptr;

    for (count = 0; count < loopNum; count ++)
    {
        _SHOW_EXEC_PERCENT(count, (int)loopNum, per);

        deviceNum = caseInst->getEngine()->getDeviceNum(APUSYS_DEVICE_SAMPLE);
        if (deviceNum <= 0)
        {
            LOG_ERR("sample devic num = %d\n", deviceNum);
            return false;
        }

        LOG_INFO("send sample user cmd, total device num(%d)\n", deviceNum);

        mem = caseInst->getEngine()->memAlloc(sizeof(struct sample_usercmd));
        if (mem == nullptr)
        {
            LOG_ERR("alloc memory as user cmd buffer fail\n");
            return false;
        }

        ucmd = (struct sample_usercmd *)mem->va;
        ucmd->cmd_idx = SAMPLE_USERCMD_IDX;
        ucmd->magic= SAMPLE_USERCMD_MAGIC;

        /* send user cmd */
        for (i = 0; i < deviceNum; i++)
        {
            LOG_INFO("send device(%d)\n", i);
            ucmd->u_write = i;
            if (caseInst->getEngine()->sendUserCmdBuf(mem, APUSYS_DEVICE_SAMPLE, i) == false)
            {
                LOG_ERR("send user cmd to sample dev(%d) fail\n", i);
                if (caseInst->getEngine()->memFree(mem) == false)
                {
                    LOG_ERR("free user cmd buffer fail\n");
                }
                return false;
            }
        }

        if (caseInst->getEngine()->memFree(mem) == false)
        {
            LOG_ERR("free user cmd buffer fail\n");
            ret = false;
        }
    }

    return ret;
}

static bool getQueueLenTest(apusysTestMisc * caseInst, unsigned int loopNum)
{
    bool ret = true;
    int dbgFd = -1, readRet = 0, per = 0;
    unsigned int queueLen = 0, count = 0;
    char queueStr[32];
    std::string dbgNodeName, sysNodeName;
    UNUSED(caseInst);

    dbgNodeName.clear();
    dbgNodeName.assign(APUSYS_DBGFS_QUEUE_PATH);
    sysNodeName.clear();
    sysNodeName.assign(APUSYS_SYSFS_QUEUE_PATH);

    for (count = 0; count < loopNum; count++)
    {
        _SHOW_EXEC_PERCENT(count, (int)loopNum, per);

        /* open debugfs */
        dbgFd = open(dbgNodeName.c_str(), O_RDONLY);
        if (dbgFd < 0)
        {
            LOG_ERR("open debugfs for get queue num fail(%d)\n", dbgFd);
            return false;
        }

        memset(&queueStr, 0, sizeof(queueStr));

        /* read queue */
        readRet = read(dbgFd, queueStr, sizeof(queueStr));
        if (readRet < 0)
        {
            LOG_ERR("read queue length from dbgfs fail(%d)\n", readRet);
            goto testSysfsNode;
        }

        queueLen = atoi(queueStr);

        LOG_INFO("get queue length(%u) from dbgfs\n", queueLen);

testSysfsNode:
        close(dbgFd);

        /* open sysfs */
        dbgFd = open(sysNodeName.c_str(), O_RDONLY);
        if (dbgFd < 0)
        {
            LOG_ERR("open debugfs for get queue num fail(%d)\n", dbgFd);
            return false;
        }

        memset(&queueStr, 0, sizeof(queueStr));

        /* read queue */
        readRet = read(dbgFd, queueStr, sizeof(queueStr));
        if (readRet < 0)
        {
            LOG_ERR("read queue length from sysfs fail(%d)\n", readRet);
            close(dbgFd);
            ret = false;
            break;
        }

        queueLen = atoi(queueStr);

        LOG_INFO("get queue length(%u) from sysfs\n", queueLen);

        close(dbgFd);
    }

    return ret;
}

static bool _secureLockDev(int fd, int deviceType)
{
    int ret = 0;
    struct apusys_ioctl_sec sec;

    memset(&sec, 0, sizeof(sec));
    sec.dev_type = deviceType;
    ret = ioctl(fd, APUSYS_SECURE_IOCTL_LOCK, &sec);
    if (ret)
    {
        LOG_ERR("secure lock dev(%d) fail(%d)\n", deviceType, ret);
        return false;
    }

    return true;
}

static bool _secureUnlockDev(int fd, int deviceType)
{
    int ret = 0;
    struct apusys_ioctl_sec sec;

    memset(&sec, 0, sizeof(sec));
    sec.dev_type = deviceType;
    ret = ioctl(fd, APUSYS_SECURE_IOCTL_UNLOCK, &sec);
    if (ret)
    {
        LOG_ERR("secure unlock dev(%d) fail(%d)\n", deviceType, ret);
        return false;
    }

    return true;
}

static bool secureLockSimpleTest(apusysTestMisc * caseInst, unsigned int loopNum, unsigned int type)
{
    bool ret = true;
    int fd = 0, per = 0;
    unsigned int count = 0;
    UNUSED(caseInst);

    fd = open(APUSYS_DEV_NODE_NAME, O_RDWR | O_SYNC);
    if (fd < 0)
    {
        LOG_ERR("open apusys fd fail(%d)\n", errno);
        return false;
    }

    /*  */
    for (count = 0; count < loopNum; count++)
    {
        _SHOW_EXEC_PERCENT(count, (int)loopNum, per);

        LOG_INFO("secure lock dev(%d)...\n", type);
        if (_secureLockDev(fd, type) == false)
        {
            LOG_ERR("sec lock device fail\n");
            ret = false;
            goto out;
        }
        LOG_INFO("secure lock dev(%d) done\n", type);

        LOG_INFO("secure unlock dev(%d)...\n", type);
        if (_secureUnlockDev(fd, type) == false)
        {
            LOG_ERR("sec unlock device fail\n");
            ret = false;
            goto out;
        }
        LOG_INFO("secure unlock dev(%d) done\n", type);
    }

out:
    close(fd);
    return ret;
}

static bool secureLockRunTest(apusysTestMisc * caseInst, unsigned int loopNum)
{
    bool ret = true;
    int fd = 0, per = 0, deviceNum = 0;
    unsigned int count = 0, i = 0;
    std::vector<int> flagList;
    std::vector<ISampleCmd *> sampleCmdList;
    std::vector<IApusysDev *> devList;
    std::string name;
    int waitUs = 1000*700;

    fd = open(APUSYS_DEV_NODE_NAME, O_RDWR | O_SYNC);
    if (fd < 0)
    {
        LOG_ERR("open apusys fd fail(%d)\n", errno);
        return false;
    }

    name.assign("secure Lock Test");

    deviceNum = caseInst->getEngine()->getDeviceNum(APUSYS_DEVICE_SAMPLE);
    if (deviceNum <= 0)
    {
        LOG_ERR("sample devic num = %d\n", deviceNum);
        ret = false;
        goto runFalse;
    }

    /* resize */
    sampleCmdList.resize(deviceNum);
    devList.resize(deviceNum);
    flagList.resize(deviceNum);

    for (count = 0; count < loopNum; count++)
    {
        _SHOW_EXEC_PERCENT(count, (int)loopNum, per);

        for (i = 0; i < flagList.size(); i++)
        {
            flagList.at(i) = 0;
        }

        LOG_INFO("round (%d/%u)\n", count, loopNum);
        /* create sample cmd */
        for (i = 0; i < (unsigned int)deviceNum ; i++)
        {
            sampleCmdList.at(i) = caseInst->getEngine()->createSampleCmd();
            if (sampleCmdList.at(i) == nullptr)
            {
                LOG_ERR("create sample cmd(%d) fail\n", i);
                ret = false;
                goto runFalse;
            }

            if (sampleCmdList.at(i)->setup(name, 0x0, 200)== false)
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

        LOG_INFO("run first time\n");
        std::thread a0(threadRun, caseInst, sampleCmdList.at(0), &flagList.at(0));
        std::thread a1(threadRun, caseInst, sampleCmdList.at(1), &flagList.at(1));
        usleep(waitUs); // 700ms

        if (flagList.at(0) != 1 || flagList.at(1) != 1)
        {
            LOG_ERR("#0 time sample cmd not done\n");
            ret = false;
            a0.join();
            a1.join();
            goto runFalse;
        }
        a0.join();
        a1.join();

        /* lock first */
        LOG_INFO("secure lock dev(%d)...\n", APUSYS_DEVICE_SAMPLE);
        if (_secureLockDev(fd, APUSYS_DEVICE_SAMPLE) == false)
        {
            LOG_ERR("sec lock device fail\n");
            ret = false;
            goto runFalse;
        }
        LOG_INFO("secure lock dev(%d) done\n", APUSYS_DEVICE_SAMPLE);

        /* run cmd in other threads */
        /* run first time */
        for (i = 0; i < flagList.size(); i++)
        {
            flagList.at(i) = 0;
        }
        LOG_INFO("run second time\n");
        std::thread a2(threadRun, caseInst, sampleCmdList.at(0), &flagList.at(0));
        std::thread a3(threadRun, caseInst, sampleCmdList.at(1), &flagList.at(1));
        usleep(waitUs); // 700ms
        if (flagList.at(0) == 1 || flagList.at(1) == 1)
        {
            LOG_ERR("#1 time sample cmd done, but should lock\n");
            a2.join();
            a3.join();
            ret = false;
            if (_secureUnlockDev(fd, APUSYS_DEVICE_SAMPLE) == false)
            {
                LOG_ERR("sec unlock device fail\n");
            }
            goto runFalse;
        }

        LOG_INFO("secure unlock dev(%d)...\n", APUSYS_DEVICE_SAMPLE);
        if (_secureUnlockDev(fd, APUSYS_DEVICE_SAMPLE) == false)
        {
            LOG_ERR("sec unlock device fail\n");
            ret = false;
            goto runFalse;
        }
        LOG_INFO("secure unlock dev(%d) done\n", APUSYS_DEVICE_SAMPLE);
        usleep(waitUs); // 700ms

        if (flagList.at(0) != 1 || flagList.at(1) != 1)
        {
            LOG_ERR("#2 time sample cmd not done\n");
            ret = false;
            goto runFalse;
        }

        a2.join();
        a3.join();
    }

runFalse:
    close(fd);
    return ret;
}

static bool abortTestRun(apusysTestMisc * caseInst, unsigned int loopNum)
{
    IApusysCmd * apusysCmd = nullptr;
    ISampleCmd *cmd;
    unsigned int i = 0;
    int per = 0, delayTime = 100;
    bool ret = true;
    std::string name;
    std::vector<int> dp;
    struct apusysCmdProp prop;

    name.assign("abortTest");
    memset(&prop, 0, sizeof(prop));

    prop.priority = APUSYS_PRIORITY_NORMAL;
    prop.hardLimit = 50; //abort time 50ms

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
        apusysCmd->setCmdProperty(prop);

        cmd = caseInst->getEngine()->createSampleCmd();
        if (cmd == nullptr)
        {
            LOG_ERR("init sample command fail\n");
            ret = false;
            goto out;
        }
        cmd->setup(name, 0x8740, delayTime); //delay 100ms
        if (apusysCmd->addSubCmd(cmd->compile(), APUSYS_DEVICE_SAMPLE, dp) < 0)
        {
            LOG_ERR("add sample cmd fail\n");
            return false;
        }

        if (caseInst->getEngine()->runCmd(apusysCmd) == true)
        {
            LOG_ERR("abort test pass, but should be fail(%d/%u)\n", i, loopNum);
            ret = false;
            goto out;
        }

        if (caseInst->getEngine()->destroySampleCmd(cmd) == false)
        {
            LOG_ERR("release sample command fail(%d/%u)\n", i, loopNum);
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


bool apusysTestMisc::runCase(unsigned int subCaseIdx, unsigned int loopNum, unsigned int threadNum)
{
    if(loopNum == 0)
    {
        LOG_ERR("loop number(%u), do nothing\n", loopNum);
        return false;
    }

    switch(subCaseIdx)
    {
        case UT_MISC_SETPOWER:
            return setPowerTest(this, loopNum);

        case UT_MISC_FIRMWARE:
            return firmwareTest(this, loopNum);

        case UT_MISC_DEVALLOC_SIMPLE:
            return deviceAllocSimpleTest(this, loopNum);

        case UT_MISC_DEVALLOC_RUN:
            return deviceAllocRunTest(this, loopNum);

        case UT_MISC_USERCMD:
            return sendUserCmdTest(this, loopNum);

        case UT_MISC_QUEUE_LEN:
            return getQueueLenTest(this, loopNum);

        case UT_MISC_SECDEV_SIMPLE:
            return secureLockSimpleTest(this, loopNum, threadNum);

        case UT_MISC_SECDEV_RUN:
            return secureLockRunTest(this, loopNum);

        case UT_MISC_ABORT:
            return abortTestRun(this, loopNum);

        default:
            LOG_ERR("no subtest(%u)\n", subCaseIdx);
    }

    return false;
}

void apusysTestMisc::showUsage()
{
    LOG_CON("       <-c case>               [%d] misc test\n", APUSYS_UT_MISC);
    LOG_CON("           <-s subIdx>             [%d] setpower test\n", UT_MISC_SETPOWER);
    LOG_CON("                                   [%d] firmware test\n", UT_MISC_FIRMWARE);
    LOG_CON("                                   [%d] device allocation simple test\n", UT_MISC_DEVALLOC_SIMPLE);
    LOG_CON("                                   [%d] device allocation run test\n", UT_MISC_DEVALLOC_RUN);
    LOG_CON("                                   [%d] user defined cmd test\n", UT_MISC_USERCMD);
    LOG_CON("                                   [%d] get queue num from dbgfs test\n", UT_MISC_QUEUE_LEN);
    LOG_CON("                                   [%d] secure lock api test<-t type>\n", UT_MISC_SECDEV_SIMPLE);
    LOG_CON("                                   [%d] secure lock and normal run test\n", UT_MISC_SECDEV_RUN);
    LOG_CON("                                   [%d] abort cmd test\n", UT_MISC_ABORT);

    return;
}

