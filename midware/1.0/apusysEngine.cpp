/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2019. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include "apusys_drv.h"

#include <sys/ioctl.h>
#include <cutils/properties.h>

#include "apusys.h"
#include "apusysEngine.h"
#include "apusysCmn.h"
#include "apusysCmd.h"

#include "ionAllocator.h"
#include "ionAospAllocator.h"
#include "dmaAllocator.h"

#include "vlmAllocator.h"

int gLogLevel = 0;
int gPrintTime = 0;

static void getLogLevel()
{
#ifdef __ANDROID__
    char prop[100];

    memset(prop, 0, sizeof(prop));
    property_get(PROPERTY_DEBUG_APUSYS_LOGLEVEL, prop, "0");
    gLogLevel = atoi(prop);
#else
    char *prop;

    prop = std::getenv("PROPERTY_DEBUG_APUSYS_LOGLEVEL");
    if (prop != nullptr)
    {
        gLogLevel = atoi(prop);
    }
#endif

    LOG_DEBUG("debug loglevel = %d\n", gLogLevel);

    return;
}

static void getPrintTime()
{
#ifdef __ANDROID__
    char prop[100];

    memset(prop, 0, sizeof(prop));
    property_get(PROPERTY_DEBUG_APUSYS_PRINTTIME, prop, "0");
    gPrintTime= atoi(prop);
#else
    char *prop;

    prop = std::getenv("PROPERTY_DEBUG_APUSYS_PRINTTIME");
    if (prop != nullptr)
    {
        gPrintTime = atoi(prop);
    }
#endif

    LOG_DEBUG("debug print time = %d\n", gPrintTime);

    return;
}

apusysDev::apusysDev()
{
    idx = -1;
    type = -1;
    hnd = 0;
    return;
}

apusysDev::~apusysDev()
{
    idx = -1;
    type = -1;
    hnd = 0;
    return;
}

apusysFirmware::apusysFirmware()
{
    name.clear();
    idx = -1;
    type = -1;
    buf = nullptr;
    magic = 0;

    return;
}

apusysFirmware::~apusysFirmware()
{
    return;
}

apusysEngine::apusysEngine(const char * userName)
{
    int ret = 0, idx = 0;
    unsigned long long devSup = 0;
    struct apusys_ioctl_hs ioctlHandShake;

    getLogLevel();
    getPrintTime();

    mFd = open("/dev/apusys", O_RDWR | O_SYNC);
    if (mFd < 0)
    {
        LOG_ERR("===========================================\n");
        LOG_ERR("| open apusys device node fail, errno(%d/%s)|\n", errno, strerror(errno));
        LOG_ERR("===========================================\n");
        abort();
        return;
    }

    LOG_DEBUG("apusys dev node = %d\n", mFd);

    /* assign user information */
    //mUserName.assign(name);
    mUserPid = getpid();
    mAllocDevList.clear();
    mAsyncList.clear();
    mFirmwareList.clear();
    mDevNum.clear();

    memset(&ioctlHandShake, 0, sizeof(ioctlHandShake));
    ioctlHandShake.type = APUSYS_HANDSHAKE_BEGIN;
    /* handshake with driver */
    ret = ioctl(mFd, APUSYS_IOCTL_HANDSHAKE, &ioctlHandShake);
    if (ret)
    {
        LOG_ERR("apusys handshake fail(%s)\n", strerror(errno));
    }

    /* assign information from kernel */
    mMagicNum = ioctlHandShake.magic_num;
    mCmdVersion = ioctlHandShake.cmd_version;
    mMemSupport = ioctlHandShake.begin.mem_support;
    mDevSupport = ioctlHandShake.begin.dev_support;
    mVlmStart = ioctlHandShake.begin.vlm_start;
    mVlmSize = ioctlHandShake.begin.vlm_size;

    //assert(ioctlHandShake.begin.dev_type_max == APUSYS_DEVICE_MAX);
    assert(ioctlHandShake.begin.dev_support != 0);
    assert(mMagicNum == APUSYS_MAGIC_NUMBER);
    assert(mCmdVersion == APUSYS_CMD_VERSION);

    /* query all device num */
    devSup = mDevSupport;
    LOG_DEBUG("device support = 0x%llx\n", mDevSupport);
    while (devSup && idx < APUSYS_DEVICE_MAX)
    {
        if (devSup & 1<<idx)
        {
            memset(&ioctlHandShake, 0, sizeof(ioctlHandShake));
            ioctlHandShake.type = APUSYS_HANDSHAKE_QUERY_DEV;
            ioctlHandShake.dev.type = idx;
            LOG_DEBUG("query device(%d/%d)\n", APUSYS_HANDSHAKE_QUERY_DEV, idx);
            ret = ioctl(mFd, APUSYS_IOCTL_HANDSHAKE, &ioctlHandShake);
            if (ret)
            {
                LOG_ERR("apusys handshake fail(%s)\n", strerror(errno));
            }
            LOG_DEBUG("device(%d) has (%d) core in this platform\n", idx, ioctlHandShake.dev.num);

            mDevNum.insert(std::pair<int, int>(idx, (int)ioctlHandShake.dev.num));

            devSup &= ~(1<<idx);
        }
        idx++;
    }

    if (mMemSupport & 1UL << APUSYS_MEM_DRAM_ION)
    {
        LOG_INFO("MTK ION allocator 0x%x\n", mMemSupport);
        mDramAllocator = (IMemAllocator *)new ionAllocator(mFd);
    } else if(mMemSupport & 1UL << APUSYS_MEM_DRAM_DMA)
    {
        LOG_INFO("DMA allocator 0x%x\n", mMemSupport);
        mDramAllocator = (IMemAllocator *)new dmaAllocator(mFd);
    } else if(mMemSupport & 1UL << APUSYS_MEM_DRAM_ION_AOSP)
    {
        LOG_INFO("AOSP ION allocator 0x%x\n", mMemSupport);
        mDramAllocator = (IMemAllocator *)new ionAospAllocator(mFd);
    } else
    {
        LOG_ERR("Unsupported allocator: 0x%x\n", mMemSupport);
        mDramAllocator = nullptr;
    }

    if (mMemSupport & 1UL << APUSYS_MEM_VLM)
    {
        mVlmAllocator = (IMemAllocator *)new vlmAllocator(mVlmStart, mVlmSize);
    }
    else
    {
        LOG_DEBUG("not support vlm\n");
        mVlmAllocator = nullptr;
    }

    assert(mDramAllocator != nullptr);

    if (userName == nullptr)
    {
        LOG_WARN("userName is null\n");
        mUserName.assign("defaultUser");
    }
    else
    {
        mUserName.assign(userName);
    }

    LOG_INFO("user(%s) create apusys engine\n", mUserName.c_str());
}

apusysEngine::apusysEngine():apusysEngine("defaultUser")
{
    assert(mDramAllocator != nullptr);
}

apusysEngine::~apusysEngine()
{
    if (mVlmAllocator != nullptr)
    {
        delete mVlmAllocator;
        mVlmAllocator = nullptr;
    }

    if (mDramAllocator != nullptr)
    {
        delete mDramAllocator;
        mDramAllocator = nullptr;
    }

    close(mFd);
}

IApusysCmd * apusysEngine::initCmd()
{
    IApusysCmd * cmd = (IApusysCmd *)new apusysCmd((IApusysEngine *)this);
    if(cmd == nullptr)
    {
        LOG_ERR("init apusys cmd fail\n");
        return nullptr;
    }

    return cmd;
}

bool apusysEngine::destroyCmd(IApusysCmd * cmd)
{
    delete (apusysCmd *)cmd;

    return true;
}

bool apusysEngine::runCmd(IApusysCmd * cmd)
{
    apusysCmd * cmdImpl = (apusysCmd *)cmd;
    IApusysMem * cmdBuf = nullptr;
    bool ret = false;

    if(cmd == nullptr)
    {
        LOG_ERR("invalid argument\n");
        return false;
    }

    cmdImpl->setFence(false);

    if(cmdImpl->constructCmd() == false)
    {
        LOG_ERR("construct cmd buffer fail\n");
        return false;
    }

    cmdBuf = cmdImpl->getCmdBuf();
    if(cmdBuf == nullptr)
    {
        LOG_ERR("extract cmd buffer fail\n");
        return false;
    }

    if (cmdImpl->markExecute() == false)
    {
        return false;
    }

    ret = runCmd(cmdBuf);

    cmdImpl->unMarkExecute();

    return ret;
}

bool apusysEngine::runCmdAsync(IApusysCmd * cmd)
{
    apusysCmd * cmdImpl = (apusysCmd *)cmd;
    IApusysMem * cmdBuf = nullptr;
    bool ret = false;

    if(cmd == nullptr)
    {
        LOG_ERR("invalid argument\n");
        return false;
    }

    cmdImpl->setFence(true);

    if(cmdImpl->constructCmd() == false)
    {
        LOG_ERR("construct cmd buffer fail\n");
        return false;
    }

    cmdBuf = cmdImpl->getCmdBuf();
    if(cmdBuf == nullptr)
    {
        LOG_ERR("extract cmd buffer async fail\n");
        return false;
    }

    if (cmdImpl->markExecute() == false)
    {
        return false;
    }

    ret = runCmdAsync(cmdBuf);

    if (ret == false)
    {
        cmdImpl->unMarkExecute();
    }

    return ret;
}


static bool sync_wait(int fd)
{
    struct pollfd fds;
    int ret;
    if (fd < 0) {
        errno = EINVAL;
        return false;
    }
    fds.fd = fd;
    fds.events = POLLIN;
    fds.revents = 0;
    do {
        ret = poll(&fds, 1, -1); /* Wait forever */

        if (ret > 0) {
            if (fds.revents & (POLLERR | POLLNVAL)) {
                errno = EINVAL;
                return false;
            }
            return true;
        } else if (ret == 0) { /* NEVER */
            errno = ETIME;
            return false;
        }
    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));

    return false;
}


bool apusysEngine::waitCmd(IApusysCmd * cmd)
{
    IApusysMem * cmdBuf = nullptr;
    apusysCmd * cmdImpl = (apusysCmd *)cmd;
    apusysCmdFence fence;
    bool ret = false;

    cmdBuf = cmdImpl->getCmdBuf();
    if(cmdBuf == nullptr)
    {
        LOG_ERR("wait cmd fail, can't find key\n");
        return false;
    }

    if (cmdImpl->getFence(fence) == false)
    {
        LOG_WARN("get fence fail\n");
        return false;
    }


    ret = sync_wait(fence.fd);
    close(fence.fd);
    cmdImpl->unMarkExecute();

    return ret;
}

bool apusysEngine::runCmd(IApusysMem * cmdBuf)
{
    int ret = 0;
    struct apusys_ioctl_cmd ioctlCmd;

    if (cmdBuf == nullptr)
    {
        LOG_ERR("invalid argument\n");
        return false;
    }

    if(setCmdIdToBuf(cmdBuf, (unsigned long long)cmdBuf) == false)
    {
        LOG_WARN("set cmd id fail\n");
    }

    memset(&ioctlCmd, 0, sizeof(ioctlCmd));
    LOG_DEBUG("ioctlCmd(0x%x/0x%llx/%d)\n",cmdBuf->iova, cmdBuf->va, cmdBuf->size);
    ioctlCmd.mem_fd = cmdBuf->getMemFd();
    ioctlCmd.size = cmdBuf->size;
    ioctlCmd.offset = 0;

    LOG_INFO("runCmd (0x%llx) sync\n", (unsigned long long)cmdBuf);
    ret = ioctl(mFd, APUSYS_IOCTL_RUN_CMD_SYNC, &ioctlCmd);
    if(ret)
    {
        LOG_ERR("run cmd (0x%llx) sync fail(%s)\n", (unsigned long long)cmdBuf, strerror(errno));
        return false;
    }

    if (gPrintTime)
    {
        apusysCmd::printCmdTime(cmdBuf);
    }

    return true;
}

bool apusysEngine::runCmdAsync(IApusysMem * cmdBuf)
{
    int ret = 0;
    struct apusys_ioctl_cmd ioctlCmd;

    if (cmdBuf == nullptr)
    {
        LOG_ERR("invalid argument\n");
        return false;
    }

    if(setCmdIdToBuf(cmdBuf, (unsigned long long)cmdBuf) == false)
    {
        LOG_WARN("set cmd id fail\n");
    }

    memset(&ioctlCmd, 0, sizeof(ioctlCmd));
    LOG_DEBUG("ioctlCmd(0x%x/0x%llx/%d)\n",cmdBuf->iova, cmdBuf->va, cmdBuf->size);
    ioctlCmd.mem_fd = cmdBuf->getMemFd();
    ioctlCmd.size = cmdBuf->size;
    ioctlCmd.offset = 0;

    LOG_INFO("runCmd (0x%llx) async\n", (unsigned long long)cmdBuf);
    ret = ioctl(mFd, APUSYS_IOCTL_RUN_CMD_ASYNC, &ioctlCmd);
    if(ret)
    {
        LOG_ERR("run cmd (0x%llx) async fail(%s)\n", (unsigned long long)cmdBuf, strerror(errno));
        return false;
    }

    std::unique_lock<std::mutex> mutexLock(mAsyncListMtx);
    mAsyncList.insert({(unsigned long long)cmdBuf, ioctlCmd.cmd_id});

    return true;
}

bool apusysEngine::waitCmd(IApusysMem * cmdBuf)
{
    struct apusys_ioctl_cmd ioctlCmd;
    unsigned long long cmdId = 0;
    int ret = 0;

    if(cmdBuf == nullptr)
    {
        LOG_ERR("wait cmd fail, can't find key\n");
        return false;
    }

    mAsyncListMtx.lock();
    auto got = mAsyncList.find((unsigned long long)cmdBuf);
    if (got == mAsyncList.end())
    {
        LOG_ERR("wrong async cmd(%p) to wait\n", (void *)cmdBuf);

        mAsyncListMtx.unlock();
        return false;
    }

    cmdId = got->second;
    mAsyncList.erase(got);

    mAsyncListMtx.unlock();

    memset(&ioctlCmd, 0, sizeof(ioctlCmd));
    ioctlCmd.cmd_id = cmdId;

    LOG_INFO("wait cmd (%p)\n", (void *)cmdBuf);
    ret = ioctl(mFd, APUSYS_IOCTL_WAIT_CMD, &ioctlCmd);
    if(ret)
    {
        LOG_ERR("wait cmd fail(%s)\n", strerror(errno));
        return false;
    }

    if (gPrintTime)
    {
        apusysCmd::printCmdTime(cmdBuf);
    }

    return true;
}

bool apusysEngine::checkCmdValid(IApusysMem * cmdBuf)
{
    return apusysCmd::checkCmdValid(cmdBuf);
}

bool apusysEngine::sendUserCmdBuf(IApusysMem * cmdBuf, APUSYS_DEVICE_E deviceType)
{
    return sendUserCmdBuf(cmdBuf, deviceType, 0);
}

bool apusysEngine::sendUserCmdBuf(IApusysMem * cmdBuf, APUSYS_DEVICE_E deviceType, int idx)
{
    struct apusys_ioctl_ucmd ioctlUserCmd;
    int deviceNum = 0, ret = 0;

    /* check cmd buffer */
    if (cmdBuf == nullptr)
    {
        LOG_ERR("invalid argument\n");
        return false;
    }

    /* check device support */
    deviceNum = getDeviceNum(deviceType);
    if (deviceNum <= 0 || idx >= deviceNum)
    {
        LOG_ERR("device(%d/%d) not support\n", idx, deviceNum);
        return false;
    }

    /* send to kernel */
    memset(&ioctlUserCmd, 0, sizeof(ioctlUserCmd));
    ioctlUserCmd.dev_type = deviceType;
    ioctlUserCmd.idx = idx;
    ioctlUserCmd.mem_fd = cmdBuf->getMemFd();
    ioctlUserCmd.offset = 0;
    ioctlUserCmd.size = cmdBuf->size;

    ret = ioctl(mFd, APUSYS_IOCTL_USER_CMD, &ioctlUserCmd);
    if(ret)
    {
        LOG_ERR("send user cmd fail(%s)\n", strerror(errno));
        return false;
    }

    return true;
}

/* memory functions */
IApusysMem * apusysEngine::memAlloc(size_t size, unsigned int align, unsigned int cache, APUSYS_USER_MEM_E type)
{
    IApusysMem * mem = nullptr;

    switch(type)
    {
        case APUSYS_USER_MEM_DRAM:
            mem = mDramAllocator->alloc(size, align, cache);
            if (mem != nullptr)
            {
                if (memSync(mem) == false)
                {
                    LOG_ERR("sync memory at allocation fail\n");
                }
            }
            break;
        case APUSYS_USER_MEM_VLM:
            if (mVlmAllocator != nullptr)
            {
                mem = mVlmAllocator->alloc(size, align, cache);
            }
            break;
        default:
            return nullptr;
    }

    return mem;
}

IApusysMem * apusysEngine::memAlloc(size_t size, unsigned int align, unsigned int cache)
{
    return mDramAllocator->alloc(size, align, cache);
}

IApusysMem * apusysEngine::memAlloc(size_t size, unsigned int align)
{
    return mDramAllocator->alloc(size, align, NONCACHED);
}

IApusysMem * apusysEngine::memAlloc(size_t size)
{
    return mDramAllocator->alloc(size, PAGE_ALIGN, NONCACHED);
}

bool apusysEngine::memFree(IApusysMem * mem)
{
    bool ret = false;

    if (mem == nullptr)
    {
        LOG_ERR("invalid argument\n");
        return false;
    }

    switch(mem->type)
    {
        case APUSYS_USER_MEM_DRAM:
            ret = mDramAllocator->free(mem);
            break;
        case APUSYS_USER_MEM_VLM:
            if (mVlmAllocator != nullptr)
            {
                ret = mVlmAllocator->free(mem);
            }
            break;
        default:
            return false;
    }

    return ret;
}

bool apusysEngine::memSync(IApusysMem * mem)
{
    return mDramAllocator->flush(mem);
}

bool apusysEngine::memInvalidate(IApusysMem * mem)
{
    return mDramAllocator->invalidate(mem);
}

IApusysMem * apusysEngine::memImport(int shareFd, unsigned int size)
{
    return mDramAllocator->import(shareFd, size);
}
bool apusysEngine::memUnImport(IApusysMem * mem)
{
    return mDramAllocator->unimport(mem);
}

int apusysEngine::getDeviceNum(APUSYS_DEVICE_E type)
{
    std::map<int, int>::iterator iter;

    /* cehck */
    iter = mDevNum.find(type);
    if (iter == mDevNum.end())
    {
        LOG_ERR("don't support this device type(%d)\n", type);
        return -1;
    }

    return iter->second;
}

IApusysDev * apusysEngine::devAlloc(APUSYS_DEVICE_E deviceType)
{
    struct apusys_ioctl_dev ioctlDev;
    apusysDev * userDev = nullptr;
    int ret = 0;

    /* check type */
    if (getDeviceNum(deviceType) <= 0)
    {
        LOG_ERR("not support device type(%d)\n\n", deviceType);
        return nullptr;
    }

    memset(&ioctlDev, 0, sizeof(struct apusys_ioctl_dev));

    ioctlDev.dev_type = deviceType;
    ioctlDev.num = 1;
    ioctlDev.dev_idx = -1;
    ioctlDev.handle = 0;

    ret = ioctl(mFd, APUSYS_IOCTL_DEVICE_ALLOC, &ioctlDev);
    if(ret || ioctlDev.handle == 0 || ioctlDev.dev_idx == -1)
    {
        LOG_ERR("alloc dev(%d) fail(%s/0x%llx/%d)\n", deviceType, strerror(errno), ioctlDev.handle, ioctlDev.dev_idx);
        return nullptr;
    }

    /* new virtual dev for user */
    userDev = new apusysDev;
    if (userDev == nullptr)
    {
        LOG_ERR("alloc userdev(%d) fail\n", deviceType);
        ret = ioctl(mFd, APUSYS_IOCTL_DEVICE_FREE, &ioctlDev);
        if(ret)
            LOG_ERR("alloc dev(%d) fail(%s)\n", deviceType, strerror(errno));

        return nullptr;
    }

    userDev->idx = ioctlDev.dev_idx;
    userDev->type = ioctlDev.dev_type;
    userDev->hnd = ioctlDev.handle;

    std::unique_lock<std::mutex> mutexLock(mAllocDevListMtx);
    mAllocDevList.push_back(userDev);

    LOG_DEBUG("alloc dev(%d/%p) ok, idx(%d)\n", deviceType, (void *)userDev, ioctlDev.dev_idx);

    return (IApusysDev *)userDev;
}

bool apusysEngine::devFree(IApusysDev * idev)
{
    apusysDev * dev = (apusysDev *)idev;
    std::vector<apusysDev *>::iterator iter;
    struct apusys_ioctl_dev ioctlDev;
    std::unique_lock<std::mutex> mutexLock(mAllocDevListMtx);
    int ret = 0;

    /* check argument */
    if (dev == nullptr)
    {
        LOG_ERR("invalid argument\n");
        return false;
    }

    LOG_DEBUG("free dev(%p/%u)\n", (void *)dev, (unsigned int)mAllocDevList.size());
    memset(&ioctlDev, 0, sizeof(struct apusys_ioctl_dev));

    /* query dev from allocDevList */
    for (iter = mAllocDevList.begin(); iter != mAllocDevList.end(); iter++)
    {
        if (dev == *iter)
        {
            /* delete from allocDevList */
            mAllocDevList.erase(iter);
            /* call ioctl to notify kernel to free device */
            ioctlDev.dev_type = dev->type;
            ioctlDev.num = 1;
            ioctlDev.dev_idx = dev->idx;
            ioctlDev.handle = dev->hnd;
            ret = ioctl(mFd, APUSYS_IOCTL_DEVICE_FREE, &ioctlDev);
            if(ret)
            {
                LOG_ERR("free dev(%d/%d) fail(%s)\n", dev->type, dev->idx, strerror(errno));
                return false;
            }

            delete dev;
            return true;
        }
    }

    return false;
}


bool apusysEngine::setPower(APUSYS_DEVICE_E deviceType, unsigned int idx, unsigned int boostVal)
{
    int devNum = 0, ret = 0;
    struct apusys_ioctl_power ioctlPower;

    /* check device num and idx */
    devNum = getDeviceNum(deviceType);
    if (devNum <= 0 || (int)idx >= devNum)
    {
        LOG_ERR("wrong device(%d) idx(%d/%d)\n", deviceType, idx, devNum);
        return false;
    }

    memset(&ioctlPower, 0, sizeof(struct apusys_ioctl_power));
    ioctlPower.dev_type = deviceType;
    ioctlPower.idx = idx;
    ioctlPower.boost_val = boostVal;
    ret = ioctl(mFd, APUSYS_IOCTL_SET_POWER, &ioctlPower);
    if(ret)
    {
        LOG_ERR("ioctl set power fail(%s)\n", strerror(errno));
        return false;
    }

    return true;
}

IApusysFirmware * apusysEngine::loadFirmware(APUSYS_DEVICE_E deviceType, unsigned int magic,
                                            unsigned int idx, std::string name, IApusysMem * fwBuf)
{
    int devNum = 0, ret = 0;
    struct apusys_ioctl_fw ioctlFw;
    apusysFirmware * fw = nullptr;

    if (fwBuf == nullptr)
    {
        LOG_ERR("fw buffer(%p) is invalid\n", (void *)fwBuf);
        return 0;
    }

    devNum = getDeviceNum(deviceType);
    if (devNum <= 0 || (int)idx >= devNum)
    {
        LOG_ERR("don't support device type(%d) idx(%d/%d)\n", deviceType, devNum, idx);
        return 0;
    }

    fw = new apusysFirmware;
    if (fw == nullptr)
    {
        LOG_ERR("new firmware handle failed\n");
        return 0;
    }

    memset(&ioctlFw, 0, sizeof(struct apusys_ioctl_fw));
    ioctlFw.dev_type = deviceType;
    ioctlFw.idx = idx;
    ioctlFw.mem_fd = fwBuf->getMemFd();
    ioctlFw.magic = magic;
    ioctlFw.offset = 0;
    ioctlFw.size = fwBuf->size;
    strncpy(ioctlFw.name, name.c_str(), sizeof(ioctlFw.name)-1);

    ret = ioctl(mFd, APUSYS_IOCTL_FW_LOAD, &ioctlFw);
    if(ret)
    {
        LOG_ERR("ioctl load firmware fail(%s)\n", strerror(errno));
        delete fw;
        return nullptr;
    }

    fw->buf = fwBuf;
    fw->idx = idx;
    fw->magic = magic;
    fw->type = deviceType;
    fw->name = name;

    std::unique_lock<std::mutex> mutexLock(mFirmwareListMtx);
    mFirmwareList.insert({(unsigned long long)fw, (unsigned long long)fwBuf});

    LOG_DEBUG("fw: memfd = %d/%d\n", fwBuf->getMemFd(), fw->buf->getMemFd());

    return (IApusysFirmware *)fw;
}

bool apusysEngine::unloadFirmware(IApusysFirmware * hnd)
{
    bool ret = true;
    struct apusys_ioctl_fw ioctlFw;
    apusysFirmware * fw = (apusysFirmware *)hnd;

    /* get from list */
    mFirmwareListMtx.lock();
    auto got = mFirmwareList.find((unsigned long long)hnd);
    if (got == mFirmwareList.end())
    {
        LOG_ERR("unknown firmware handle(%p)\n", (void *)hnd);
        mFirmwareListMtx.unlock();
        return false;
    }

    mFirmwareList.erase(got);
    mFirmwareListMtx.unlock();

    memset(&ioctlFw, 0, sizeof(struct apusys_ioctl_fw));
    strncpy(ioctlFw.name, fw->name.c_str(), sizeof(ioctlFw.name)-1);
    ioctlFw.dev_type = fw->type;
    ioctlFw.idx = fw->idx;
    ioctlFw.magic = fw->magic;
    ioctlFw.mem_fd = fw->buf->getMemFd();
    ioctlFw.offset = 0;
    ioctlFw.size = fw->buf->size;
    LOG_DEBUG("fw: memfd = %d\n", fw->buf->getMemFd());

    if (ioctl(mFd, APUSYS_IOCTL_FW_UNLOAD, &ioctlFw))
    {
        LOG_ERR("ioctl unload firmware fail(%s)\n", strerror(errno));
        ret = false;
    }

    delete fw;

    return ret;
}

//---------------------------------------------
IApusysEngine * IApusysEngine::createInstance(const char * userName)
{
    return new apusysEngine(userName);
}

bool IApusysEngine::deleteInstance(IApusysEngine * engine)
{
    if (engine == nullptr)
    {
        LOG_ERR("invalid argument\n");
        return false;
    }

    delete (apusysEngine *)engine;
    return true;
}

