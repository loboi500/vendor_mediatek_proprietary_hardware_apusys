/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2021. All rights reserved.
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
#include <sys/mman.h>
#include <string.h>
#include <poll.h>
#include <linux/dma-buf.h>
#include <linux/types.h>

#include "apusys_drv.h"
#include "apusysCmn.h"
#include "apusysExecutor.h"
#include "apusysSession.h"

/* TODO */
#include "./v2_0/mdw_drv.h"

#define APU_MDW_NAME "apu_mdw"
#define MDW_DEFAULT_TIMEOUT_MS (30 * 1000)
typedef uint64_t u64;

class apusysSubCmd_v2 : public apusysSubCmd {
private:
    std::vector<struct mdw_subcmd_cmdbuf> mCmdBufInList;

public:
    apusysSubCmd_v2(class apusysCmd *parent, enum apusys_device_type type, uint32_t idx);
    virtual ~apusysSubCmd_v2();

    uint64_t getRunInfo(enum apusys_subcmd_runinfo_type type);
    void setupInfo(struct mdw_subcmd_info &info, uint32_t packId);
};

class apusysCmd_v2 : public apusysCmd {
private:
    std::vector<int> mFenceList;
    void *mExecInfo;

    int release();
    int submit(int fence, uint64_t flag);
    int construct();

public:
    apusysCmd_v2(class apusysSession *session);
    virtual ~apusysCmd_v2();

    int getSubCmdExecInfos(uint32_t idx, struct mdw_subcmd_exec_info &info);
    uint64_t getRunInfo(enum apusys_cmd_runinfo_type type);
    class apusysSubCmd *createSubCmd(enum apusys_device_type type);
    int build();
    int run();
    int runAsync();
    int wait();
    int runFence(int fence, uint64_t flags);
};

struct mdwExecInfo {
    struct mdw_cmd_exec_info c;
    struct mdw_subcmd_exec_info sc;
};

//----------------------------------------------
// apusysSubCmd_v2 implementation
apusysSubCmd_v2::apusysSubCmd_v2(class apusysCmd *parent, enum apusys_device_type type, uint32_t idx):
    apusysSubCmd(parent, type, idx)
{
    mCmdBufInList.clear();
    LOG_DEBUG("apusysSubCmd_v2(%p)\n", static_cast<void *>(this));
}

apusysSubCmd_v2::~apusysSubCmd_v2()
{
    LOG_DEBUG("apusysSubCmd_v2(%p)\n", static_cast<void *>(this));
}

uint64_t apusysSubCmd_v2::getRunInfo(enum apusys_subcmd_runinfo_type type)
{
    struct mdw_subcmd_exec_info execInfos;
    class apusysCmd_v2 *cmd = static_cast<class apusysCmd_v2 *>(this->mParentCmd);
    uint64_t ret = 0;

    if (cmd->getSubCmdExecInfos(mIdx, execInfos)) {
        LOG_ERR("get runInfo(%d) fail\n", type);
        return 0;
    }

    switch (type) {
    case SC_RUNINFO_IPTIME:
        ret = execInfos.ip_time;
        break;

    case SC_RUNINFO_DRIVERTIME:
        ret = execInfos.driver_time;
        break;

    case SC_RUNINFO_TIMESTAMP_START:
        ret = execInfos.ip_start_ts;
        break;

    case SC_RUNINFO_TIMESTAMP_END:
        ret = execInfos.ip_end_ts;
        break;

    case SC_RUNINFO_BANDWIDTH:
        ret = execInfos.bw;
        break;

    case SC_RUNINFO_BOOST:
        ret = execInfos.boost;
        break;

    case SC_RUNINFO_VLMUSAGE:
        ret = execInfos.tcm_usage;
        break;

    default:
        break;
    }

    return ret;
}

void apusysSubCmd_v2::setupInfo(struct mdw_subcmd_info &info, uint32_t packId)
{
    unsigned int i = 0;
    struct mdw_subcmd_exec_info einfo;
    class apusysCmd_v2 *parent = static_cast<class apusysCmd_v2 *>(mParentCmd);

    std::unique_lock<std::mutex> mutexLock(mMtx);

    memset(&einfo, 0, sizeof(einfo));
    if (parent->getSubCmdExecInfos(mIdx, einfo))
        LOG_WARN("get exec infos fail\n");

    /* setup subcmd infos */
    info.type = mDevType;
    info.suggest_time = mSuggestMs;
    info.vlm_usage = mVlmUsage;
    info.vlm_ctx_id = mVlmCtx;
    info.vlm_force = mVlmForce;
    info.boost = mBoostVal;
    info.turbo_boost = mTurboBoost;
    info.min_boost = mMinBoost;
    info.max_boost = mMaxBoost;
    info.hse_en = mHseEnable;
    info.pack_id = packId;
    info.driver_time = einfo.driver_time;
    info.ip_time = einfo.ip_time;
    info.bw = einfo.bw;

    mCmdBufInList.clear();
    mCmdBufInList.resize(mCmdBufList.size());
    info.num_cmdbufs = mCmdBufList.size();
    /* setup cmdbufs */
    for (i = 0; i < mCmdBufList.size(); i++) {
        mCmdBufInList.at(i).handle = static_cast<uint64_t>(mCmdBufList.at(i)->mem->handle);
        mCmdBufInList.at(i).size = mCmdBufList.at(i)->mem->size;
        mCmdBufInList.at(i).align = mCmdBufList.at(i)->mem->align;
        mCmdBufInList.at(i).direction = mCmdBufList.at(i)->dir;
    }
    info.cmdbufs = reinterpret_cast<uint64_t>(&mCmdBufInList[0]);
}

//----------------------------------------------
static int waitSyncFile(int fd, int timeoutMs)
{
    int ret = 0;
    struct pollfd fds;

    memset(&fds, 0, sizeof(fds));
    fds.fd = fd;
    fds.events = POLLIN;
    fds.revents = 0;
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

//----------------------------------------------
// apusysCmd_v2 implementation
apusysCmd_v2::apusysCmd_v2(class apusysSession *session):apusysCmd(session)
{
    mFenceList.clear();
    mExecInfo = nullptr;
    LOG_DEBUG("apusysCmd_v2(%p)\n", static_cast<void *>(this));
}

apusysCmd_v2::~apusysCmd_v2()
{
    class apusysSubCmd_v2 *subcmd = nullptr;

    LOG_DEBUG("apusysCmd_v2(%p)\n", static_cast<void *>(this));

    apusysTraceBegin(__func__);
    mMtx.lock();

    this->release();

    while (!mSubCmdList.empty()) {
        subcmd = static_cast<class apusysSubCmd_v2 *>(mSubCmdList.back());
        delete subcmd;
        mSubCmdList.pop_back();
    }

    mMtx.unlock();
    apusysTraceEnd();
}

int apusysCmd_v2::getSubCmdExecInfos(uint32_t idx, struct mdw_subcmd_exec_info &info)
{
    uint32_t size = 0, expectedSize = 0;
    struct mdwExecInfo *execInfos = nullptr;
    struct mdw_subcmd_exec_info *scExecInfos = nullptr;

    /* check mem exist */
    if (mExecInfo == nullptr) {
        LOG_ERR("no exec info mem\n");
        return -ENOMEM;
    }

    /* check idx */
    if (idx >= mSubCmdList.size()) {
        LOG_ERR("invalid idx(%u/%u)\n", idx, static_cast<unsigned int>(mSubCmdList.size()));
        return -EINVAL;
    }

    /* check size */
    size = this->mSession->memGetInfoFromHostPtr(mExecInfo, APUSYS_MEM_INFO_GET_SIZE);
    expectedSize = sizeof(struct mdw_cmd_exec_info) + mSubCmdList.size() * sizeof(struct mdw_subcmd_exec_info);
    if (size != expectedSize) {
        LOG_ERR("invalid size(%u/%u)\n", size, expectedSize);
        return -EINVAL;
    }

    execInfos = static_cast<struct mdwExecInfo *>(mExecInfo);
    scExecInfos = &execInfos->sc;
    memcpy(static_cast<void *>(&info), static_cast<void *>(&scExecInfos[idx]), sizeof(*scExecInfos));

    return 0;
}

uint64_t apusysCmd_v2::getRunInfo(enum apusys_cmd_runinfo_type type)
{
    struct mdwExecInfo *execInfos = nullptr;
    uint64_t ret = 0;

    /* check mem exist */
    if (!mExecInfo) {
        LOG_ERR("no exec info mem\n");
        return -ENOMEM;
    }

    execInfos = static_cast<struct mdwExecInfo *>(mExecInfo);

    switch (type) {
    case CMD_RUNINFO_STATUS:
        if (execInfos->c.sc_rets)
            ret = CMD_ERRNO_HW_TIMEOUT;
        else if (execInfos->c.ret)
            ret = CMD_ERRNO_HARDLIMIT_EXCEED;

        LOG_DEBUG("Cmd(%p): get status(%llu/%lld/0x%llx)\n",
            static_cast<void *>(this), static_cast<unsigned long long>(ret),
            static_cast<signed long long>(execInfos->c.ret),
            static_cast<unsigned long long>(execInfos->c.sc_rets));
        break;

    case CMD_RUNINFO_TOTALTIME_US:
        ret = execInfos->c.total_us;
        break;

    default:
        LOG_DEBUG("not support(%d)\n", type);
        break;
    }

    LOG_DEBUG("Cmd(%p): get cmd run infomaiton(%d/%llu)\n",
        static_cast<void *>(this), type, static_cast<unsigned long long>(ret));

    return ret;
}

class apusysSubCmd *apusysCmd_v2::createSubCmd(enum apusys_device_type type)
{
    class apusysSubCmd *subcmd = nullptr;
    uint32_t idx = 0, i = 0;

    /* check device type exist */
    if (!this->getSession()->queryDeviceNum(type))
        return nullptr;

    std::unique_lock<std::mutex> mutexLock(mMtx);
    idx = mSubCmdList.size();
    subcmd = new apusysSubCmd_v2(this, type, idx);
    if (subcmd == nullptr) {
        LOG_ERR("new subcmd(%d) fail\n", type);
        goto out;
    }

    /* add to subcmd list */
    mSubCmdList.push_back(subcmd);

    /* resize adjust list */
    mAdjMatrix.resize(mSubCmdList.size());
    for (i = 0; i < mAdjMatrix.size(); i++)
        mAdjMatrix.at(i).resize(mSubCmdList.size());

    /* resize packid list and push 0 */
    mPackIdList.push_back(0);

    this->setDirty(APUSYS_CMD_DIRTY_SUBCMD);
    LOG_DEBUG("Cmd v2(%p): create #%u-subcmd(%d/%p)\n",
       static_cast<void *>(this), idx, type, static_cast<void *>(subcmd));
out:
    return subcmd;
}

int apusysCmd_v2::construct()
{
    unsigned int size = 0;
    int ret = 0;

    apusysTraceBegin(__func__);

    if (!mSubCmdList.size()) {
        LOG_ERR("Cmd v2(%p): no subcmds\n", static_cast<void *>(this));
        ret = -EINVAL;
        goto out;
    }

    if (mExecInfo != nullptr) {
        LOG_INFO("Cmd v2(%p): free execInfo for rebuild cmd\n", static_cast<void *>(this));
        if (this->mSession->cmdBufFree(mExecInfo)) {
            LOG_WARN("Cmd v2(%p): free execInfo fail\n", static_cast<void *>(this));
        }
        mExecInfo = nullptr;
    }

    size = sizeof(struct mdw_cmd_exec_info) + mSubCmdList.size() * sizeof(struct mdw_subcmd_exec_info);
    mExecInfo = this->mSession->cmdBufAlloc(size, 0);
    if (mExecInfo == nullptr) {
        LOG_ERR("Cmd v2(%p): alloc execInfo fail\n", static_cast<void *>(this));
        ret = -ENOMEM;
        goto out;
    }

    mDirty.reset();
out:
    apusysTraceEnd();

    return ret;
}

int apusysCmd_v2::build()
{
    int ret = 0;

    mMtx.lock();
    apusysTraceBegin(__func__);

    ret = this->construct();

    apusysTraceEnd();
    mMtx.unlock();

    return ret;
}

int apusysCmd_v2::release()
{
    apusysTraceBegin(__func__);

    if (mExecInfo != nullptr) {
        if (this->mSession->cmdBufFree(mExecInfo))
            LOG_WARN("Cmd v2(%p): free execInfo fail\n", static_cast<void *>(this));

        mExecInfo = nullptr;
    }

    this->setDirty(APUSYS_CMD_DIRTY_SUBCMD);
    apusysTraceEnd();

    return 0;
}

int apusysCmd_v2::submit(int inFence, uint64_t flags)
{
    union mdw_cmd_args args;
    std::vector<struct mdw_subcmd_info> subCmdInfos;
    class apusysSubCmd_v2 *subCmd;
    std::vector<uint8_t> adjMatrix;
    unsigned int i = 0, j = 0;
    int ret = 0;
    UNUSED(flags);

    apusysTraceBegin(__func__);

    /* rebuild cmd */
    if (mDirty.test(APUSYS_CMD_DIRTY_SUBCMD) ||
        mDirty.test(APUSYS_CMD_DIRTY_CMDBUF))
        this->construct();

    this->printInfo(0);

    /* subcmd info */
    subCmdInfos.clear();
    subCmdInfos.resize(mSubCmdList.size());
    adjMatrix.clear();
    adjMatrix.resize(mSubCmdList.size() * mSubCmdList.size());
    for (i = 0; i < mSubCmdList.size(); i++) {
        subCmd = static_cast<apusysSubCmd_v2 *>(mSubCmdList.at(i));
        subCmd->setupInfo(subCmdInfos.at(i), mPackIdList.at(i));
        /* construct */
        for (j = 0; j < mAdjMatrix.at(i).size(); j++) {
            adjMatrix.at(i * mAdjMatrix.at(i).size() + j) = mAdjMatrix.at(i).at(j);
            CLOG_DEBUG(" adj matrix(%u) = %u\n",
                static_cast<unsigned int>(i * mAdjMatrix.at(i).size() + j),
                adjMatrix.at(i * mAdjMatrix.at(i).size() + j));
        }
    }

    /* cmd info */
    memset(&args, 0, sizeof(args));
    args.in.op = MDW_CMD_IOCTL_RUN;
    args.in.arg.exec.usr_id = mUsrid;
    args.in.arg.exec.uid = reinterpret_cast<uint64_t>(this);
    args.in.arg.exec.priority = mPriority;
    args.in.arg.exec.hardlimit = mHardlimit;
    args.in.arg.exec.softlimit = mSoftlimit;
    args.in.arg.exec.power_save = mPowerSave;
    args.in.arg.exec.power_plcy = mPowerPolicy;
    args.in.arg.exec.power_dtime = mDelayPowerOffTime;
    args.in.arg.exec.app_type = mAppType;
    args.in.arg.exec.flags = 0;
    args.in.arg.exec.num_subcmds = subCmdInfos.size();
    args.in.arg.exec.subcmd_infos = reinterpret_cast<uint64_t>(&subCmdInfos[0]);
    args.in.arg.exec.adj_matrix = reinterpret_cast<uint64_t>(&adjMatrix[0]);
    args.in.arg.exec.fence = static_cast<uint64_t>(inFence);
    args.in.arg.exec.exec_infos = this->mSession->memGetInfoFromHostPtr(mExecInfo, APUSYS_MEM_INFO_GET_HANDLE);
    CLOG_DEBUG("cmd header:\n");
    CLOG_DEBUG(" num subcmds = %u\n", static_cast<unsigned int>(mSubCmdList.size()));
    CLOG_DEBUG(" usr id = 0x%llx\n", static_cast<unsigned long long>(args.in.arg.exec.usr_id));
    CLOG_DEBUG(" uid = 0x%llx\n", static_cast<unsigned long long>(args.in.arg.exec.uid));
    CLOG_DEBUG(" priority = %u\n", args.in.arg.exec.priority);
    CLOG_DEBUG(" hardlimit = %u\n", args.in.arg.exec.hardlimit);
    CLOG_DEBUG(" softlimit = %u\n", args.in.arg.exec.softlimit);
    CLOG_DEBUG(" powersave = %u\n", args.in.arg.exec.power_save);
    CLOG_DEBUG(" power_plcy = %u\n", args.in.arg.exec.power_plcy);
    CLOG_DEBUG(" power_dtime = %u\n", args.in.arg.exec.power_dtime);
    CLOG_DEBUG(" app_type = %u\n", args.in.arg.exec.app_type);
    CLOG_DEBUG(" num_subcmds = %u\n", args.in.arg.exec.num_subcmds);
    CLOG_DEBUG(" subcmd_infos = 0x%llx\n", static_cast<unsigned long long>(args.in.arg.exec.subcmd_infos));
    CLOG_DEBUG(" adj_matrix = 0x%llx\n", static_cast<unsigned long long>(args.in.arg.exec.adj_matrix));
    CLOG_DEBUG(" fence = %llu\n", static_cast<unsigned long long>(args.in.arg.exec.fence));
    CLOG_DEBUG(" exec_infos = %llu\n", static_cast<unsigned long long>(args.in.arg.exec.exec_infos));
    ret = ioctl(mSession->getDevFd(), APU_MDW_IOCTL_CMD, &args);
    if (ret) {
        ret = -(abs(errno));
        LOG_ERR("run cmd fail(%d/%s)\n", ret, strerror(errno));
        goto out;
    }

    ret = args.out.arg.exec.fence;
    if (!ret) {
        ret = -EINVAL;
        LOG_ERR("Cmd v2(%p): no fence return\n", static_cast<void *>(this));
        goto out;
    }

out:
    apusysTraceEnd();
    LOG_DEBUG("Cmd v2(%p): run sumbit done(%d)\n", static_cast<void *>(this), ret);

    return ret;
}

int apusysCmd_v2::wait()
{
    int waitFence = 0, ret = 0, timeout = -1;
    struct mdwExecInfo *eInfo = static_cast<struct mdwExecInfo *>(mExecInfo);

    apusysTraceBegin(__func__);
    mMtx.lock();
    /* check fence list */
    if (!mFenceList.size()) {
        LOG_ERR("Cmd v2(%p): no fence to wait\n", static_cast<void *>(this));
        ret = -EINVAL;
        goto unLockCmdMtx;
    }

    /* get timeout ms */
    if (!mHardlimit)
        timeout = MDW_DEFAULT_TIMEOUT_MS;
    else
        timeout = mHardlimit;

    /* get sync file fd */
    waitFence = mFenceList.back();
    if (waitFence < 0) {
        LOG_ERR("Cmd v2(%p): invalid fd(%d)\n", static_cast<void *>(this), waitFence);
        ret = -EINVAL;
        goto unLockCmdMtx;
    }
    mFenceList.pop_back();
    mMtx.unlock();

    /* wait sync file */
    LOG_DEBUG("Cmd v2(%p): wait fence(%d)\n", static_cast<void *>(this), waitFence);
    ret = waitSyncFile(waitFence, timeout);
    if (!ret && eInfo != nullptr) {
        if (eInfo->c.sc_rets)
            ret = -EIO;
        else {
            LOG_DEBUG("ret(%d/%lld/0x%llx)\n", ret, static_cast<long long>(eInfo->c.ret), static_cast<unsigned long long>(eInfo->c.sc_rets));
            ret = eInfo->c.ret;
        }
    }

    if (ret) {
        LOG_ERR("Cmd v2(%p):wait fence(%d) subcmds(0x%llx) fail(%d/%s)\n",
            static_cast<void *>(this), waitFence,
            static_cast<unsigned long long>(eInfo->c.sc_rets), ret, strerror(abs(ret)));
        this->printInfo(1);
    }
    /* close sync file */
    close(waitFence);

    goto out;

unLockCmdMtx:
    mMtx.unlock();
out:
    apusysTraceEnd();
    return ret;
}

int apusysCmd_v2::run()
{
    int waitFence = 0, ret = 0, timeout = -1;
    struct mdwExecInfo *eInfo = static_cast<struct mdwExecInfo *>(mExecInfo);

    LOG_INFO("Cmd v2(%p): run\n", static_cast<void *>(this));
    apusysTraceBegin(__func__);

    mMtx.lock();
    waitFence = this->submit(0, 0);
    if (waitFence < 0) {
        /* set error code */
        ret = waitFence;
        goto unLockCmdMtx;
    }

    if (!mHardlimit)
        timeout = MDW_DEFAULT_TIMEOUT_MS;
    else
        timeout = mHardlimit;

    mMtx.unlock();

    apusysTraceBegin("wait");
    ret = waitSyncFile(waitFence, timeout);
    if (!ret && eInfo != nullptr) {
        if (eInfo->c.sc_rets) {
            ret = -EIO;
        } else {
            LOG_DEBUG("Cmd v2(%p):ret(%d->%lld/0x%llx)\n",
                static_cast<void *>(this), ret,
                static_cast<long long>(eInfo->c.ret), static_cast<unsigned long long>(eInfo->c.sc_rets));

            ret = eInfo->c.ret;
        }
    }

    if (ret) {
        LOG_ERR("Cmd v2(%p):wait fence(%d) subcmds(0x%llx) fail(%d/%s)\n",
            static_cast<void *>(this), waitFence,
            static_cast<unsigned long long>(eInfo->c.sc_rets), ret, strerror(abs(ret)));
        this->printInfo(1);
    }

    apusysTraceEnd();

    close(waitFence);
    goto out;

unLockCmdMtx:
    mMtx.unlock();
out:
    apusysTraceEnd();
    LOG_INFO("Cmd v2(%p): run done(%d)\n", static_cast<void *>(this), ret);

    return ret;
}

int apusysCmd_v2::runAsync()
{
    int ret = 0, waitFence = 0;

    LOG_INFO("Cmd v2(%p): runAsync\n", static_cast<void *>(this));
    apusysTraceBegin(__func__);

    mMtx.lock();
    waitFence = this->submit(0, 0);
    if (waitFence < 0) {
        /* set error code */
        ret = waitFence;
        goto out;
    }

    /* push wait fence to list */
    mFenceList.push_back(waitFence);

out:
    mMtx.unlock();
    apusysTraceEnd();
    LOG_INFO("Cmd v2(%p): runAsync done(%d)\n", static_cast<void *>(this), ret);
    return ret;
}

int apusysCmd_v2::runFence(int inFence, uint64_t flags)
{
    int waitFence = 0;

    LOG_INFO("Cmd v2(%p): runFence(%d/0x%llx)\n",
        static_cast<void *>(this), inFence, static_cast<unsigned long long>(flags));

    apusysTraceBegin(__func__);

    mMtx.lock();
    waitFence = this->submit(inFence, 0);
    if (waitFence < 0)
        LOG_ERR("Cmd v2(%p): runFence fail(%d)\n", static_cast<void *>(this), waitFence);

    mMtx.unlock();
    apusysTraceEnd();
    LOG_INFO("Cmd v2(%p): runFence done(%d)\n", static_cast<void *>(this), waitFence);

    return waitFence;
}

apusysExecutor_v2::apusysExecutor_v2(class apusysSession *session) : apusysExecutor(session)
{
    union mdw_hs_args hs;
    unsigned int idx = 0;
    unsigned long long devSup = 0;
    unsigned long long memBitMask = 0;
    std::string meta;
    struct apusysMem *mem = nullptr;
    pthread_t thdInst;
    char thdName[16];

    mMemInfos.clear();
    mMemInfosMap.clear();
    mClientName.clear();

    /* setup client name */
    mClientName.append(std::to_string(gettid()));
    thdInst = pthread_self();
    if (!pthread_getname_np(thdInst, thdName, sizeof(thdName))) {
        mClientName.append(":");
        mClientName.append(thdName);
    }

    memset(&hs, 0, sizeof(hs));
    hs.in.op = MDW_HS_IOCTL_OP_BASIC;
    if (ioctl(mSession->getDevFd(), APU_MDW_IOCTL_HANDSHAKE, &hs)) {
        LOG_ERR("handshake fail(%s)\n", strerror(errno));
        abort();
    }

    mMetaDataSize = hs.out.arg.basic.meta_size;
    devSup = hs.out.arg.basic.dev_bitmask;
    memBitMask = hs.out.arg.basic.mem_bitmask;
    LOG_DEBUG("%s: version(%llu) metaSize(%u) devBitMask(0x%llx) memBitMask(0x%llx)\n",
        mClientName.c_str(), static_cast<unsigned long long>(hs.out.arg.basic.version),
        mMetaDataSize, devSup, memBitMask);

    /* query device support */
    idx = 0;
    while (devSup != 0) {
        if (!(devSup & (1ULL << idx)))
            goto dNext;

        memset(&hs, 0, sizeof(hs));
        hs.in.op = MDW_HS_IOCTL_OP_DEV;
        hs.in.arg.dev.type = idx;
        if (ioctl(mSession->getDevFd(), APU_MDW_IOCTL_HANDSHAKE, &hs)) {
            LOG_ERR("apusys query dev(%u) num fail(%s)\n", idx, strerror(errno));
            break;
        }

        mMetaMap.insert(std::pair<int, std::string>(idx, meta.assign(hs.out.arg.dev.meta)));
        mDevNumList.at(idx) = hs.out.arg.dev.num;
        LOG_DEBUG("dev(%u) support %u cores, meta(%s)\n",
            idx, mDevNumList.at(idx), hs.out.arg.dev.meta);
dNext:
        devSup &= ~(1ULL << idx);
        idx++;
    }

    /* query memory type support */
    idx = 0;
    while (memBitMask != 0) {
        if (!(memBitMask & (1ULL << idx))) {
            mMemInfos.push_back(nullptr);
            goto mNext;
        }

        memset(&hs, 0, sizeof(hs));
        hs.in.op = MDW_HS_IOCTL_OP_MEM;
        hs.in.arg.mem.type = idx;
        if (ioctl(mSession->getDevFd(), APU_MDW_IOCTL_HANDSHAKE, &hs)) {
            LOG_ERR("apusys query mem(%u) info fail(%s)\n", idx, strerror(errno));
            break;
        }

        mem = new apusysMem;
        if (mem == nullptr) {
            LOG_ERR("alloc for mem infos(%u) fail\n", idx);
            goto mNext;
        } else {
            /* alloc */
            mem->vaddr = malloc(APUSYS_MINFO_HANDLE_SIZE);
            if (mem->vaddr) {
                mem->deviceVa = hs.out.arg.mem.start;
                mem->size = hs.out.arg.mem.size;
                mMemInfosMap.insert({mem->vaddr, mem});
            } else {
                delete mem;
                mem = nullptr;
            }
            mMemInfos.push_back(mem);
            LOG_DEBUG("mem(%u) support(%p): (0x%llx/0x%x)\n",
                idx, static_cast<void *>(mem),
                static_cast<unsigned long long>(hs.out.arg.mem.start),
                static_cast<unsigned int>(hs.out.arg.mem.size));
        }

mNext:
        memBitMask &= ~(1ULL << idx);
        idx++;
    }
}

apusysExecutor_v2::~apusysExecutor_v2()
{
    struct apusysMem *mem = nullptr;

    while (!mMemInfos.empty()) {
        mem = mMemInfos.back();
        mMemInfos.pop_back();

        if (mem) {
            free(mem->vaddr);
            delete mem;
        }
    }

    LOG_DEBUG("%d\n", __LINE__);
}

unsigned int apusysExecutor_v2::getMetaDataSize()
{
    LOG_DEBUG("meta data size = %u\n", mMetaDataSize);
    return mMetaDataSize;
}

int apusysExecutor_v2::getMetaData(enum apusys_device_type type, void *metaData)
{
    auto it = mMetaMap.find(static_cast<int>(type));
    if (it == mMetaMap.end())
        return -EINVAL;

    it->second.copy(static_cast<char *>(metaData), mMetaDataSize, 0);
    LOG_DEBUG("meta(%d/%s/%p)\n", type, static_cast<char *>(metaData), metaData);
    return 0;
}

int apusysExecutor_v2::setDevicePower(enum apusys_device_type type, unsigned int idx, unsigned int boost)
{
    union mdw_util_args args;
    int ret = 0;
    uint32_t devNum = 0;

    devNum = this->getDeviceNum(type);
    if (devNum == 0 || devNum <= idx) {
        LOG_ERR("don't support dev(%u/%u)(%u)\n", type, idx, devNum);
        return -ENODEV;
    }

    apusysTraceBegin(__func__);

    memset(&args, 0, sizeof(args));
    args.in.op = MDW_UTIL_IOCTL_SETPOWER;
    args.in.arg.power.dev_type = type;
    args.in.arg.power.core_idx = idx;
    args.in.arg.power.boost = boost;
    ret = ioctl(mSession->getDevFd(), APU_MDW_IOCTL_UTIL, &args);
    if (ret)
        LOG_ERR("set power(%d/%u) fail(%s)\n", type, idx, strerror(errno));

    apusysTraceEnd();
    return ret;
}

int apusysExecutor_v2::sendUserCmd(enum apusys_device_type type, void *cmdbuf)
{
    union mdw_util_args args;
    struct apusysCmdBuf *pCb = nullptr;
    int ret = 0;

    if (cmdbuf == nullptr)
        return -EINVAL;

    if (!this->getDeviceNum(type)) {
        LOG_ERR("don't support device(%d)\n", type);
        return -ENODEV;
    }

    /* get cmdbuf instance */
    pCb = mSession->cmdBufGetObj(cmdbuf);
    if (pCb == nullptr) {
        LOG_ERR("ucmd cmdbuf(%p) to dev(%d) invalid\n", cmdbuf, type);
        ret = -EINVAL;
        goto out;
    }

    memset(&args, 0, sizeof(args));
    args.in.op = MDW_UTIL_IOCTL_UCMD;
    args.in.arg.ucmd.dev_type = type;
    args.in.arg.ucmd.size = pCb->mem->size;
    args.in.arg.ucmd.handle = reinterpret_cast<uint64_t>(cmdbuf);
    ret = ioctl(mSession->getDevFd(), APU_MDW_IOCTL_UTIL, &args);
    if (ret)
        LOG_ERR("ucmd(%d/%u) fail(%s)\n", type, pCb->mem->size, strerror(errno));

out:
    return ret;
}

class apusysCmd *apusysExecutor_v2::createCmd()
{
    class apusysCmd_v2 *cmd = nullptr;

    cmd = new apusysCmd_v2(mSession);
    if (cmd == nullptr) {
        LOG_ERR("create cmd fail\n");
        return nullptr;
    }

    return static_cast<class apusysCmd *>(cmd);
}

int apusysExecutor_v2::deleteCmd(class apusysCmd *cmd)
{
    class apusysCmd_v2 *vcmd = nullptr;

    if (cmd == nullptr)
        return -EINVAL;

    vcmd = static_cast<class apusysCmd_v2 *>(cmd);
    delete vcmd;

    return 0;
}

struct apusysMem *apusysExecutor_v2::getMemInfos(enum apusys_mem_type type)
{
    struct apusysMem *mem = nullptr;

    if (type >= mMemInfos.size() || mMemInfos.at(type) == nullptr)
        goto out;

    mem = mMemInfos.at(type);
    LOG_DEBUG("query memInfos type(%d): %p/%p\n", type, static_cast<void *>(mem), mem->vaddr);

out:
    return mem;
}

bool apusysExecutor_v2::isMemInfos(struct apusysMem *mem)
{
    std::unordered_map<void *, struct apusysMem *>::const_iterator got;
    bool ret = false;

    /* check free target is minfos or not */
    got = mMemInfosMap.find(mem->vaddr);
    if (got != mMemInfosMap.end())
        ret = true;

    LOG_DEBUG("check mInfos(%d/%p/%p/0x%llx) -> %d\n",
        mem->mtype, static_cast<void *>(mem),
        mem->vaddr, static_cast<unsigned long long>(mem->deviceVa),
        ret);

    return ret;
}

struct apusysMem *apusysExecutor_v2::memAlloc(uint32_t size, uint32_t align, enum apusys_mem_type type, uint64_t flags)
{
    union mdw_mem_args args;
    struct apusysMem *mem = nullptr;
    std::string bufName;

    apusysTraceBegin(__func__);

    /* query empty object for size query if argument size is zero */
    if (!size) {
        mem = this->getMemInfos(type);
        goto out;
    }

    mem = new apusysMem;
    if (mem == nullptr)
        goto out;

    memset(&args, 0, sizeof(args));
    args.in.op = MDW_MEM_IOCTL_ALLOC;
    args.in.arg.alloc.type = type;
    args.in.arg.alloc.size = size;
    args.in.arg.alloc.align = align;
    args.in.arg.alloc.flags = flags;
    /* allocate memory */
    if (ioctl(mSession->getDevFd(), APU_MDW_IOCTL_MEM, &args)) {
        LOG_ERR("alloc mem(%u/%u/%d/0x%llx) fail(%s)\n", size, align, type, static_cast<unsigned long long>(flags), strerror(errno));
        goto freeApusysMem;
    }

    memset(mem, 0, sizeof(*mem));
    mem->handle = args.out.arg.alloc.handle;
    mem->size = size;
    mem->align = align;
    mem->mtype = type;
    mem->highaddr = (flags & F_APUSYS_MEM_HIGHADDR) ? true : false;
    mem->cacheable = (flags & F_APUSYS_MEM_CACHEABLE) ? true : false;

    /* assign name */
    bufName.clear();
    bufName.append("APU_BUF(");
    bufName.append(std::to_string(type));
    bufName.append("):");
    bufName.append(mClientName);
    bufName.resize(DMA_BUF_NAME_LEN-1);
    if (ioctl(static_cast<int>(mem->handle), DMA_BUF_SET_NAME_B, reinterpret_cast<u64>(bufName.c_str())))
        LOG_DEBUG("set name(%s) fail\n", bufName.c_str());

    /* mapping vaddr */
    if (type == APUSYS_MEM_TYPE_DRAM) {
        mem->vaddr = mmap(NULL, mem->size, PROT_READ|PROT_WRITE, MAP_SHARED, mem->handle, 0);
        if (mem->vaddr == MAP_FAILED) {
            LOG_ERR("mem(%d/%d) get vaddr fail(%s)\n", mem->handle, mem->mtype, strerror(errno));
            goto freeMem;
        }
    } else {
        /* alloc fake vaddr for handle */
        mem->vaddr = malloc(32);
        if (mem->vaddr == nullptr) {
            LOG_ERR("alloc vaddr for mem(%d) fail\n", type);
            goto freeMem;
        }
    }

    MLOG_DEBUG("mem alloc(%p/%u/%d/%d) dva(0x%llx) flag(0x%llx)\n",
        mem->vaddr, mem->size, mem->handle, mem->mtype,
        static_cast<unsigned long long>(mem->deviceVa),
        static_cast<unsigned long long>(flags));

    goto out;

freeMem:
    close(mem->handle);

freeApusysMem:
    delete mem;
    mem = nullptr;

out:
    apusysTraceEnd();

    return mem;
}

int apusysExecutor_v2::memFree(struct apusysMem *mem)
{
    int ret = 0;
    std::unordered_map<void *, struct apusysMem *>::const_iterator got;

    apusysTraceBegin(__func__);
    MLOG_DEBUG("mem free(%p/%u/%d/%d) dva(0x%llx)\n",
        mem->vaddr, mem->size, mem->handle, mem->mtype,
        static_cast<unsigned long long>(mem->deviceVa));

    /* check free target is minfos or not */
    if (this->isMemInfos(mem))
        goto out;

    if (mem->mtype == APUSYS_MEM_TYPE_DRAM) {
        if(munmap(mem->vaddr, mem->size))
            LOG_ERR("mem(%d/%d) unmap fail\n", mem->handle, mem->mtype);
    } else {
        free(mem->vaddr);
    }

    close(mem->handle);
    delete mem;
out:
    apusysTraceEnd();
    return ret;
}

int apusysExecutor_v2::memMapDeviceVa(struct apusysMem *mem)
{
    union mdw_mem_args args;
    int ret = 0;

    apusysTraceBegin(__func__);
    if (this->isMemInfos(mem))
        goto out;

    memset(&args, 0, sizeof(args));
    args.in.op = MDW_MEM_IOCTL_MAP;
    args.in.arg.map.handle = mem->handle;
    args.in.arg.map.size = mem->size;
    if (ioctl(mSession->getDevFd(), APU_MDW_IOCTL_MEM, &args)) {
        LOG_ERR("map mem(%d/%d) fail(%s)\n", mem->handle, mem->mtype, strerror(errno));
        ret = -EINVAL;
    } else {
        mem->deviceVa = args.out.arg.map.device_va;
    }

    MLOG_DEBUG("mem map(%p/%u/%d/%d) dva(0x%llx)\n",
        mem->vaddr, mem->size, mem->handle, mem->mtype,
        static_cast<unsigned long long>(mem->deviceVa));

out:
    apusysTraceEnd();
    return ret;
}

int apusysExecutor_v2::memUnMapDeviceVa(struct apusysMem *mem)
{
    union mdw_mem_args args;
    int ret = 0;

    MLOG_DEBUG("mem unmap(%p/%u/%d/%d) dva(0x%llx)\n",
        mem->vaddr, mem->size, mem->handle, mem->mtype,
        static_cast<unsigned long long>(mem->deviceVa));

    apusysTraceBegin(__func__);
    if (this->isMemInfos(mem))
        goto out;

    memset(&args, 0, sizeof(args));
    args.in.op = MDW_MEM_IOCTL_UNMAP;
    args.in.arg.unmap.handle = mem->handle;
    if (ioctl(mSession->getDevFd(), APU_MDW_IOCTL_MEM, &args)) {
        LOG_ERR("unmap mem(%d/%d) fail(%s)\n", mem->handle, mem->mtype, strerror(errno));
        ret = -EINVAL;
    } else {
        mem->deviceVa = 0;
    }

out:
    apusysTraceEnd();
    return ret;
}

struct apusysMem *apusysExecutor_v2::memImport(int handle, unsigned int size)
{
    union mdw_mem_args args;
    struct apusysMem *mem = nullptr;
    int dupHandle = 0;

    dupHandle = dup(handle);
    if (dupHandle < 0) {
        LOG_ERR("dup fd fail(%d->%d)\n", handle, dupHandle);
        return nullptr;
    }

    memset(&args, 0, sizeof(args));
    args.in.op = MDW_MEM_IOCTL_MAP;
    args.in.arg.map.handle = dupHandle;
    args.in.arg.map.size = size;
    if (ioctl(mSession->getDevFd(), APU_MDW_IOCTL_MEM, &args)) {
        LOG_ERR("import mem(%d) fail(%s)\n", handle, strerror(errno));
        goto closeDupHandle;
    }

    if (!args.out.arg.map.device_va) {
        LOG_ERR("import mem(0x%llx/%u) handle(%d) fail(%s)\n",
            static_cast<unsigned long long>(args.out.arg.map.device_va),
            size, handle, strerror(errno));
        goto unimportMem;
    }

    mem = new struct apusysMem;
    if (mem == nullptr)
        goto unimportMem;

    memset(mem, 0, sizeof(*mem));
    mem->handle = dupHandle;
    mem->size = size;
    mem->deviceVa = args.out.arg.map.device_va;
    mem->mtype = args.out.arg.map.type;
    if (mem->mtype == APUSYS_MEM_TYPE_DRAM) {
        mem->vaddr = mmap(NULL, mem->size, PROT_READ|PROT_WRITE, MAP_SHARED, mem->handle, 0);
        if (mem->vaddr == MAP_FAILED) {
            LOG_ERR("mem(%d) map uva fail(%s)\n", handle, strerror(errno));
            goto freeMem;
        }
    } else {
        /* alloc fake vaddr for handle */
        mem->vaddr = malloc(32);
        if (mem->vaddr == nullptr) {
            LOG_ERR("alloc vaddr for mem(%d) fail\n", mem->mtype);
            goto freeMem;
        }
    }

    MLOG_DEBUG("mem import(%p/%u/%d->%d) type(%d) dva(0x%llx)\n",
        mem->vaddr, mem->size, handle, mem->handle, mem->mtype,
        static_cast<unsigned long long>(mem->deviceVa));

    return mem;

freeMem:
    delete mem;
unimportMem:
    memset(&args, 0, sizeof(args));
    args.in.op = MDW_MEM_IOCTL_UNMAP;
    args.in.arg.unmap.handle = handle;
    if (ioctl(mSession->getDevFd(), APU_MDW_IOCTL_MEM, &args))
        LOG_ERR("unimport mem(%d) fail(%s)\n", handle, strerror(errno));
closeDupHandle:
    close(dupHandle);

    LOG_ERR("import mem(0x%llx/%u) handle(%d) fail(%s)\n",
        static_cast<unsigned long long>(args.out.arg.map.device_va),
        size, handle, strerror(errno));

    return nullptr;
}

int apusysExecutor_v2::memUnImport(struct apusysMem *mem)
{
    union mdw_mem_args args;
    int ret = 0;

    MLOG_DEBUG("mem unimport(%p/%u/%d) dva(0x%llx)\n",
        mem->vaddr, mem->size, mem->handle,
        static_cast<unsigned long long>(mem->deviceVa));

    if (mem->mtype == APUSYS_MEM_TYPE_DRAM) {
        if(munmap(mem->vaddr, mem->size))
            LOG_ERR("mem(%d) unmap fail\n", mem->handle);
    } else {
        free(mem->vaddr);
    }

    memset(&args, 0, sizeof(args));
    args.in.op = MDW_MEM_IOCTL_UNMAP;
    args.in.arg.unmap.handle = mem->handle;
    ret = ioctl(mSession->getDevFd(), APU_MDW_IOCTL_MEM, &args);
    if (ret) {
        LOG_ERR("unimport mem(%d) fail(%s)\n", mem->handle, strerror(errno));
    } else {
        close(mem->handle);
        delete mem;
    }

    return ret;
}

int apusysExecutor_v2::memFlush(struct apusysMem *mem)
{
    struct dma_buf_sync sync;
    int ret = 0;

    apusysTraceBegin(__func__);

    memset(&sync, 0, sizeof(sync));
    sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW;
    ret = ioctl(mem->handle, DMA_BUF_IOCTL_SYNC, &sync);
    if (ret) {
        LOG_ERR("flush mem(%d) fail(%s)\n", mem->handle, strerror(errno));
        ret = -abs(errno);
    }

    MLOG_DEBUG("mem flush(%p/%u/%d) dva(0x%llx)\n",
        mem->vaddr, mem->size, mem->handle,
        static_cast<unsigned long long>(mem->deviceVa));

    apusysTraceEnd();

    return ret;
}

int apusysExecutor_v2::memInvalidate(struct apusysMem *mem)
{
    struct dma_buf_sync sync;
    int ret = 0;

    apusysTraceBegin(__func__);

    memset(&sync, 0, sizeof(sync));
    sync.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW;
    ret = ioctl(mem->handle, DMA_BUF_IOCTL_SYNC, &sync);
    if (ret) {
        LOG_ERR("Invalidate mem(%d) fail(%s)\n", mem->handle, strerror(errno));
        ret = -abs(errno);
    }

    MLOG_DEBUG("mem Invalidate(%p/%u/%d) dva(0x%llx)\n",
        mem->vaddr, mem->size, mem->handle,
        static_cast<unsigned long long>(mem->deviceVa));

    apusysTraceEnd();

    return ret;
}
