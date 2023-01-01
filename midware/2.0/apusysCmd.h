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

#pragma once

#include <algorithm>
#include <mutex>
#include <bitset>

#include "apusys.h"

class apusysCmd;

enum apusys_cmd_dirty {
    APUSYS_CMD_DIRTY_PARAMETER,
    APUSYS_CMD_DIRTY_CMDBUF,
    APUSYS_CMD_DIRTY_SUBCMD,
};

class apusysSubCmd {
protected:
    std::vector<struct apusysCmdBuf *>mCmdBufList;
    std::vector<uint32_t> mSuccessorList;
    std::mutex mMtx;

    uint32_t mIdx;
    uint32_t mDevType;

    /* param */
    uint32_t mExpectMs;
    uint32_t mSuggestMs;
    uint32_t mVlmUsage;
    uint32_t mVlmForce;
    uint32_t mVlmCtx;
    uint32_t mBoostVal;
    uint32_t mTurboBoost;
    uint32_t mMinBoost;
    uint32_t mMaxBoost;
    uint32_t mHseEnable;

    class apusysCmd *mParentCmd;

public:
    apusysSubCmd(class apusysCmd *parent, enum apusys_device_type type, uint32_t idx);
    virtual ~apusysSubCmd();
    int addCmdBuf(void *vaddr, enum apusys_cb_direction dir);
    int setParam(enum apusys_subcmd_param_type op, uint64_t val);
    virtual uint64_t getRunInfo(enum apusys_subcmd_runinfo_type type);

    uint32_t getIdx();
    void printInfo(int level);
    uint32_t getCmdBufNum();
    struct apusysCmdBuf *getCmdBuf(uint32_t idx);
};

class apusysCmd {
protected:
    std::vector<class apusysSubCmd *> mSubCmdList;
    std::vector<std::vector<uint8_t>>mAdjMatrix;
    std::vector<uint32_t> mPackIdList;
    std::mutex mMtx;

    /* param */
    uint32_t mPriority;
    uint32_t mHardlimit;
    uint32_t mSoftlimit;
    uint64_t mUsrid;
    uint32_t mPowerSave;
    uint32_t mPowerPolicy;
    uint32_t mDelayPowerOffTime;
    uint32_t mAppType;

    std::bitset<64> mDirty;

    class apusysSession *mSession;

    bool mIsRunning;
    std::mutex mExecMtx;
    pid_t mExecutePid;
    pid_t mExecuteTid;

public:
    apusysCmd(class apusysSession *session);
    virtual ~apusysCmd();

    void setDirty(enum apusys_cmd_dirty type);
    class apusysSession *getSession();

    virtual class apusysSubCmd *createSubCmd(enum apusys_device_type type);
    int setDependencyEdge(class apusysSubCmd *predecessor, class apusysSubCmd *successor);
    int setDependencyPack(class apusysSubCmd *main, class apusysSubCmd *appendix);
    int setParam(enum apusys_cmd_param_type op, uint64_t val);
    virtual uint64_t getRunInfo(enum apusys_cmd_runinfo_type type);
    void printInfo(int level);
    virtual int build();
    virtual int run();
    virtual int runAsync();
    virtual int wait();
    virtual int runFence(int fence, uint64_t flags);
};
