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
#include <numeric>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <errno.h>

#include "apusysCmn.h"
#include "apusysSession.h"

apusysSubCmd::apusysSubCmd(class apusysCmd *parent, enum apusys_device_type type, uint32_t idx)
{
    mParentCmd = parent;

    mDevType = type;
    mIdx = idx;
    mCmdBufList.clear();
    mExpectMs = DEFAULT_SC_PARAM_EXPECTED_MS;
    mSuggestMs = DEFAULT_SC_PARAM_SUGGEST_MS;
    mVlmUsage = DEFAULT_SC_PARAM_VLM_USAGE;
    mVlmForce = DEFAULT_SC_PARAM_VLM_FORCE;
    mVlmCtx = DEFAULT_SC_PARAM_VLM_CTX;
    mBoostVal = DEFAULT_SC_PARAM_BOOST_VALUE;
    mTurboBoost = DEFAULT_SC_PARAM_TURBO_BOOST_VALUE;
    mMinBoost = DEFAULT_SC_PARAM_MIN_BOOST_VALUE;
    mMaxBoost = DEFAULT_SC_PARAM_MAX_BOOST_VALUE;
    mHseEnable = DEFAULT_SC_PARAM_HSE_ENABLE;
}

apusysSubCmd::~apusysSubCmd()
{
}

uint32_t apusysSubCmd::getIdx(void)
{
    return mIdx;
}

int apusysSubCmd::addCmdBuf(void *vaddr, enum apusys_cb_direction dir)
{
    std::unique_lock<std::mutex> mutexLock(mMtx);
    unsigned int i = 0;
    int ret = 0;
    struct apusysCmdBuf *cb = nullptr;

    /* check cmdbuf repeated */
    for (i = 0; i < mCmdBufList.size(); i++) {
        if (mCmdBufList.at(i)->mem->vaddr == vaddr) {
            LOG_ERR("add repeated cmdbuf(%p)\n", vaddr);
            ret = -EEXIST;
            goto out;
        }
    }

    /* check cmdbuf belong this session */
    cb = mParentCmd->getSession()->cmdBufGetObj(vaddr);
    if (cb == nullptr) {
        LOG_ERR("no this cmdbuf(%p)\n", vaddr);
        ret = -ENOENT;
        goto out;
    }

    cb->dir = dir;
    LOG_DEBUG("SubCmd(%p): add cmdbuf(%p/%u/%d) dir(%d)\n",
        static_cast<void *>(this), cb->mem->vaddr, cb->mem->size, cb->mem->handle, cb->dir);

    /* insert to list */
    mCmdBufList.push_back(cb);

    this->mParentCmd->setDirty(APUSYS_CMD_DIRTY_CMDBUF);

out:
    return ret;
}

int apusysSubCmd::setParam(enum apusys_subcmd_param_type op, uint64_t val)
{
    int ret = 0;

    switch (op) {
        case SC_PARAM_EXPECTED_MS: {
            mExpectMs = static_cast<uint32_t>(val);
            break;
        }
        case SC_PARAM_SUGGEST_MS: {
            mSuggestMs = static_cast<uint32_t>(val);
            break;
        }
        case SC_PARAM_VLM_USAGE: {
            mVlmUsage = static_cast<uint32_t>(val);
            break;
        }
        case SC_PARAM_VLM_FORCE: {
            mVlmForce = static_cast<uint32_t>(val);
            break;
        }
        case SC_PARAM_VLM_CTX: {
            mVlmCtx = static_cast<uint32_t>(val);
            break;
        }
        case SC_PARAM_TURBO_BOOST: {
            mTurboBoost = static_cast<uint32_t>(val);
            break;
        }
        case SC_PARAM_MIN_BOOST: {
            mMinBoost = static_cast<uint32_t>(val);
            break;
        }
        case SC_PARAM_MAX_BOOST: {
            mMaxBoost = static_cast<uint32_t>(val);
            break;
        }
        case SC_PARAM_HSE_ENABLE: {
            mHseEnable = static_cast<uint32_t>(val);
            break;
        }

        case SC_PARAM_BOOST_VALUE: {
            unsigned long bst = static_cast<uint32_t>(val);
            if (bst > APUSYS_SUBCMD_BOOST_MAX) {
                LOG_ERR("boost val(%lu) invalid\n", bst);
                ret = -EINVAL;
            } else {
                mBoostVal = bst;
            }

            break;
        }
        default: {
            LOG_DEBUG("param(%d) not support\n", op);
            break;
        }
    }
    LOG_DEBUG("SubCmd(%p): set param(%d/0x%llx)\n",
        static_cast<void *>(this), op, static_cast<unsigned long long>(val));

    this->mParentCmd->setDirty(APUSYS_CMD_DIRTY_PARAMETER);

    return ret;
}

uint64_t apusysSubCmd::getRunInfo(enum apusys_subcmd_runinfo_type type)
{
    UNUSED(type);
    return 0;
}

uint32_t apusysSubCmd::getCmdBufNum()
{
    return mCmdBufList.size();
}

struct apusysCmdBuf *apusysSubCmd::getCmdBuf(uint32_t idx)
{
    if (idx > mCmdBufList.size())
        return nullptr;

    return mCmdBufList.at(idx);
}

void apusysSubCmd::printInfo(int level)
{
    if (level) {
        LOG_WARN(" subcmd(%u/%p) type(%u) boost(%u/%u) vlm(%u/%u) hse(%u)\n",
            mIdx, static_cast<void *>(this), mDevType, mBoostVal, mTurboBoost, mVlmCtx, mVlmUsage, mHseEnable);
        return;
    }

    CLOG_DEBUG("------------------------------\n");
    CLOG_DEBUG(" subcmd(%u/%p)\n", mIdx, static_cast<void *>(this));
    CLOG_DEBUG(" boost value(%u)\n", mBoostVal);
    CLOG_DEBUG(" expect ms(%u)\n", mExpectMs);
    CLOG_DEBUG(" suggest ms(%u)\n", mSuggestMs);
    CLOG_DEBUG(" vlm usage(%u)\n", mVlmUsage);
    CLOG_DEBUG(" vlm force(%u)\n", mVlmForce);
    CLOG_DEBUG(" vlm ctx(%u)\n", mVlmCtx);
    CLOG_DEBUG(" turbo boost(%u)\n", mTurboBoost);
    CLOG_DEBUG(" min boost(%u)\n", mMinBoost);
    CLOG_DEBUG(" max boost(%u)\n", mMaxBoost);
    CLOG_DEBUG(" HSE enable(%u)\n", mHseEnable);
    CLOG_DEBUG("------------------------------\n");
}

apusysCmd::apusysCmd(class apusysSession *session)
{
    mSession = session;
    mSubCmdList.clear();
    mAdjMatrix.clear();
    mPackIdList.clear();
    mPriority = DEFAULT_CMD_PARAM_PRIORITY;
    mHardlimit = DEFAULT_CMD_PARAM_HARDLIMIT;
    mSoftlimit = DEFAULT_CMD_PARAM_SOFTLIMIT;
    mUsrid = DEFAULT_CMD_PARAM_USRID;
    mPowerSave = DEFAULT_CMD_PARAM_POWERSAVE;
    mPowerPolicy = DEFAULT_CMD_PARAM_POWERPOLICY;
    mDelayPowerOffTime = DEFAULT_CMD_PARAM_DELAY_POWER_OFF_TIME;
    mAppType = DEFAULT_CMD_PARAM_APPTYPE;

    mDirty.reset();

    mIsRunning = false;
    mExecutePid = 0;
    mExecuteTid = 0;
}

apusysCmd::~apusysCmd()
{
    class apusysSubCmd *subcmd = nullptr;

    //std::unique_lock<std::mutex> mutexLock(mMtx);
    while(!mSubCmdList.empty()) {
        subcmd = mSubCmdList.back();
        mSubCmdList.pop_back();
        delete subcmd;
    }
}

void apusysCmd::setDirty(enum apusys_cmd_dirty type)
{
    mDirty.set(type);
}

class apusysSession *apusysCmd::getSession()
{
    return mSession;
}

class apusysSubCmd *apusysCmd::createSubCmd(enum apusys_device_type type)
{
    class apusysSubCmd *subcmd = nullptr;
    uint32_t idx = 0, i = 0;

    /* check device type exist */
    if (!this->getSession()->queryDeviceNum(type)) {
        LOG_ERR("not support device type(%d)\n", type);
        return nullptr;
    }

    std::unique_lock<std::mutex> mutexLock(mMtx);
    idx = mSubCmdList.size();
    subcmd = new apusysSubCmd(this, type, idx);
    if (subcmd == nullptr) {
        LOG_ERR("new subcmd fail(%d)\n", type);
        goto out;
    }

    /* add to subcmd list */
    mSubCmdList.push_back(subcmd);

    /* resize adjust matrix */
    mAdjMatrix.resize(mSubCmdList.size());
    for (i = 0; i < mAdjMatrix.size(); i++)
        mAdjMatrix.at(i).resize(mSubCmdList.size());

    /* resize packid list and push 0 */
    mPackIdList.push_back(0);

    this->setDirty(APUSYS_CMD_DIRTY_CMDBUF);

     LOG_DEBUG("Cmd(%p): create #%u-subcmd(%d/%p)\n",
        static_cast<void *>(this), idx, type, static_cast<void *>(subcmd));
out:
    return subcmd;
}

int apusysCmd::setDependencyEdge(class apusysSubCmd *predecessor, class apusysSubCmd *successor)
{
    uint32_t predecessorIdx = 0, successorIdx = 0, i = 0, sucessorPackId = 0, predecessorPackId = 0;

    if (predecessor == nullptr || successor == nullptr)
        return -EINVAL;

    std::unique_lock<std::mutex> mutexLock(mMtx);

    predecessorIdx = predecessor->getIdx();
    successorIdx = successor->getIdx();
    predecessorPackId = mPackIdList.at(predecessorIdx);
    sucessorPackId = mPackIdList.at(successorIdx);

    /* check same idx */
    if (predecessorIdx == successorIdx) {
        LOG_ERR("can't set edge dependency with same subcmd(%u)(%p/%p)\n",
            predecessorIdx, static_cast<void *>(predecessor), static_cast<void *>(successor));
        return -EINVAL;
    }

    /* check same pack id */
    if (predecessorPackId && predecessorPackId == sucessorPackId) {
        LOG_ERR("can't set edge dependency with same pack id(%u)(%p/%p)\n",
            predecessorPackId, static_cast<void *>(predecessor), static_cast<void *>(successor));
        return -EINVAL;
    }

    /* check adj matrix circular */
    if (mAdjMatrix.at(successorIdx).at(predecessorIdx)) {
        LOG_ERR("circular dependency set(%u->%u)\n", predecessorIdx, successorIdx);
        return -EINVAL;
    }

    LOG_DEBUG("Cmd(%p): set edge(%u->%u): pack id(%u/%u) size(%u)\n",
        static_cast<void *>(this), predecessorIdx, successorIdx,
        predecessorPackId, sucessorPackId,
        static_cast<unsigned int>(mAdjMatrix.size()));

    if (!sucessorPackId) {
        /* sucessor no packed, setup predecessor directly */
        mAdjMatrix.at(predecessorIdx).at(successorIdx) = true;
    } else {
        /* setup predecessor's dependency with all sucessor packed subcmd */
        for (i = 0; i < mSubCmdList.size(); i++) {
            if (mPackIdList.at(i) == sucessorPackId)
                mAdjMatrix.at(predecessorIdx).at(i) = true;
        }
    }

    /* setup all predecessor packed subcmd dependency */
    if (predecessorPackId) {
        for (i = 0; i < mSubCmdList.size(); i++) {
            if (mPackIdList.at(i) == predecessorPackId)
                mAdjMatrix.at(i) = mAdjMatrix.at(predecessorIdx);
        }
    }

    this->setDirty(APUSYS_CMD_DIRTY_CMDBUF);

    return 0;
}

int apusysCmd::setDependencyPack(class apusysSubCmd *main, class apusysSubCmd *appendix)
{
    uint32_t mainIdx = 0, appendixIdx = 0, i = 0;

    if (main == nullptr || appendix == nullptr)
        return -EINVAL;

    std::unique_lock<std::mutex> mutexLock(mMtx);

    mainIdx = main->getIdx();
    appendixIdx = appendix->getIdx();

    if (mPackIdList.at(appendixIdx) != 0) {
        LOG_ERR("Cmd(%p): appendix dependency already packed(%u)\n",
            static_cast<void *>(this), mPackIdList.at(appendixIdx));
        return -EINVAL;
    }

    /* self pack, return directly */
    if (mainIdx == appendixIdx) {
        LOG_DEBUG("Cmd(%p): self pack(%u)\n", static_cast<void *>(this), mainIdx);
        return 0;
    }

    if (!mPackIdList.at(mainIdx)) {
        /* if main subcmd's pack id == 0, setup pack id */
        mPackIdList.at(mainIdx) = mainIdx + 1;
    }
    /* set same pack id to appendix subcmd */
    mPackIdList.at(appendixIdx) = mPackIdList.at(mainIdx);

    /* update adj matrix, horizontal */
    mAdjMatrix.at(appendixIdx) = mAdjMatrix.at(mainIdx);
    /* update adj matrix, vertical */
    for (i = 0; i < mAdjMatrix.size(); i++) {
        if (mAdjMatrix.at(i).at(mainIdx))
            mAdjMatrix.at(i).at(appendixIdx) = true;
    }

    this->setDirty(APUSYS_CMD_DIRTY_CMDBUF);
    LOG_DEBUG("Cmd(%p): set pack(%u->%u): pack id(%u)\n",
        static_cast<void *>(this), mainIdx, appendixIdx, mPackIdList.at(mainIdx));

    return 0;
}

int apusysCmd::setParam(enum apusys_cmd_param_type op, uint64_t val)
{
    switch (op) {
        case CMD_PARAM_PRIORITY: {
            mPriority = static_cast<uint32_t>(val);
            break;
        }
        case CMD_PARAM_HARDLIMIT: {
            mHardlimit = static_cast<uint32_t>(val);
            break;
        }
        case CMD_PARAM_SOFTLIMIT: {
            mSoftlimit = static_cast<uint32_t>(val);
            break;
        }
        case CMD_PARAM_USRID: {
            mUsrid = static_cast<uint32_t>(val);
            break;
        }
        case CMD_PARAM_POWERSAVE: {
            mPowerSave = static_cast<uint32_t>(val);
            break;
        }
        case CMD_PARAM_POWERPOLICY: {
            mPowerPolicy = static_cast<uint32_t>(val);
            break;
        }
        case CMD_PARAM_DELAY_POWER_OFF_MS: {
            mDelayPowerOffTime = static_cast<uint32_t>(val);
            break;
        }
        case CMD_PARAM_APPTYPE: {
            mAppType = static_cast<uint32_t>(val);
            break;
        }
        default: {
            LOG_DEBUG("op(%d) not support\n", op);
            break;
        }
    };

    LOG_DEBUG("Cmd(%p): set param(%d/0x%llx)\n",
        static_cast<void *>(this), op, static_cast<unsigned long long>(val));
    this->setDirty(APUSYS_CMD_DIRTY_PARAMETER);

    return 0;
}

uint64_t apusysCmd::getRunInfo(enum apusys_cmd_runinfo_type type)
{
    UNUSED(type);

    LOG_WARN("Cmd(%p): no executor\n", static_cast<void *>(this));
    return 0;
}

void apusysCmd::printInfo(int level)
{
    unsigned int i = 0;

    if (level) {
        LOG_WARN(" cmd(%p): num_subcmds(%u) priority(%u) hardlimit(%u) softlimit(%u) dtime(%u) power(%u/%u) \n",
            static_cast<void *>(this), static_cast<uint32_t>(mSubCmdList.size()),
            mPriority, mHardlimit, mSoftlimit, mDelayPowerOffTime,
            mAppType, mPowerPolicy);
        for (i = 0; i < mSubCmdList.size(); i++)
            mSubCmdList.at(i)->printInfo(level);
        return;
    }

    CLOG_DEBUG("==============================\n");
    CLOG_DEBUG(" cmd(%p) information\n", static_cast<void *>(this));
    CLOG_DEBUG("------------------------------\n");
    CLOG_DEBUG(" num subcmds(%u)\n", static_cast<uint32_t>(mSubCmdList.size()));
    CLOG_DEBUG(" priority(%u)\n", mPriority);
    CLOG_DEBUG(" hard limit(%u)\n", mHardlimit);
    CLOG_DEBUG(" soft limit(%u)\n", mSoftlimit);
    CLOG_DEBUG(" user id(%llu)\n", static_cast<unsigned long long>(mUsrid));
    CLOG_DEBUG(" power save(%u)\n", mPowerSave);
    CLOG_DEBUG(" power policy(%u)\n", mPowerPolicy);
    CLOG_DEBUG(" delay power off time(%u)\n", mDelayPowerOffTime);
    CLOG_DEBUG(" apptype(%u)\n", mAppType);
    CLOG_DEBUG(" dependency:\n");
    for (i = 0; i < mSubCmdList.size(); i++)
        mSubCmdList.at(i)->printInfo(level);
    CLOG_DEBUG("===============================\n");
}

int apusysCmd::build()
{
    this->printInfo(0);
    return -EINVAL;
}

int apusysCmd::run()
{
    return -EINVAL;
}

int apusysCmd::runAsync()
{
    return -EINVAL;
}

int apusysCmd::wait()
{
    return -EINVAL;
}

int apusysCmd::runFence(int fence, uint64_t flags)
{
    UNUSED(fence);
    UNUSED(flags);

    return -EINVAL;
}

