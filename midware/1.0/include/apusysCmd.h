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

#pragma once

#include <cstdint>
#include <vector>
#include <deque>
#include <string>
#include <mutex>
#include <bitset>

#include "apusys.h"

class apusysCmd;

struct apusysCmdFence {
    unsigned long long fd;
    unsigned int totalTime;
    unsigned int status;
};

class apusysSubCmd : public IApusysSubCmd  {
private:
    int mDeviceType; // device type
    unsigned int mIdx; // idx in apusys cmd
    IApusysMem * mSubCmdBuf; // sub cmd memory buffer
    std::vector<int> mPredecessorList; // dependency list
    std::vector<int> mSuccessorList; //which subcmd dependent on this subcmd

    unsigned int mCtxId; // memory ctx id
    unsigned int mPackId; //pack id
    unsigned long long mInfoEntry;

    struct apusysSubCmdTuneParam mTuneParam; //in

    bool mDirty;
    apusysCmd* mParentCmd;

    apusysCmd* _getParentCmd();
    void _setDirtyFlag();

    unsigned int _getBandwidthFromBuf();
    unsigned int _getExecTimeFromBuf();
    unsigned int _getIpTimeFromBuf();
    unsigned int _getTcmUsageFromBuf();

public:
    apusysSubCmd(int idx, int deviceType, IApusysMem * subCmdBuf, apusysCmd * parentCmd);
    ~apusysSubCmd();

    /* internal functions */
    IApusysMem * getSubCmdBuf();

    void setPredecessorList(std::vector<int>& predecessorList);
    std::vector<int>& getPredecessorList();
    void setSuccessorList(std::vector<int>& successorList);
    std::vector<int>& getSuccessorList();
    void setPackId(unsigned int packId);
    unsigned int getPackId();

    /* subcmd info header functions */
    void setInfoEntry(unsigned long long va);
    bool constructHeader();

    /* export functions for user */
    int getDeviceType();
    void setIdx(int idx);
    int getIdx();
    bool setCtxId(unsigned int ctxId);
    unsigned int getCtxId();

    bool getRunInfo(struct apusysSubCmdRunInfo& info);
    struct apusysSubCmdTuneParam& getTuneParam();
    bool setTuneParam(struct apusysSubCmdTuneParam& tune);
    bool setIOParam();
};

class apusysCmd : public IApusysCmd {
private:
    //unsigned long long mCmdId;
    unsigned int mDependencySize; // recore all subcmd's dependency size

    IApusysMem * mCmdBuf; // whole apusys cmd
    std::vector<unsigned long long> mSubCmdEntryList;
    std::vector<apusysSubCmd *> mSubCmdList;

    std::vector<int> mTypeCmdNum; // record how many type subcmd in this cmd
    unsigned int mPackNum; // record how many packed subcmd in this cmd

    std::mutex mMtx;
    bool mDirty;

    int mExecutePid;
    int mExecuteTid;
    bool mIsExecuting;

    bool mFenceEnable;

    /* for memory allocator */
    IApusysEngine * mEngine;

    /* set by user */
    struct apusysCmdProp mProperty;
    std::bitset<64> mFlagBitmap;

    /* calculate total apusys cmd size for allocate memory */
    unsigned int _calcCmdSize();

    /* set cmdProperty to cmd buffer's header */
    bool _setCmdPropertyToBuf();

    /* mCmdBuf operation functions */
    void _printCmd();

public:
    apusysCmd(IApusysEngine * engine);
    ~apusysCmd();

    /* cmd functions */
    unsigned int size();
    struct apusysCmdProp& getCmdProperty();
    bool setCmdProperty(const struct apusysCmdProp &prop);
    int addSubCmd(IApusysMem * subCmdBuf, APUSYS_DEVICE_E deviceType,
                  std::vector<int> & dependency);
    int addSubCmd(IApusysMem * subCmdBuf, APUSYS_DEVICE_E deviceType,
                  std::vector<int> & dependency,
                  unsigned int ctxId);
    IApusysSubCmd * getSubCmd(int idx);
    int packSubCmd(IApusysMem * subCmdBuf, APUSYS_DEVICE_E deviceType,
                   int subCmdIdx);

    bool constructCmd();

    /* for apusys engine only */
    IApusysMem * getCmdBuf();
    unsigned long long getCmdId();
    bool setCmdId(unsigned long long cmdId);

    static bool checkCmdValid(IApusysMem * cmdBuf);
    static void printCmdTime(IApusysMem * cmdBuf);
    void printCmd(IApusysMem * cmdBuf);

    bool markExecute();
    bool unMarkExecute();

    bool setFence(bool enable);
    bool getFence(struct apusysCmdFence &fence);
};

bool setCmdIdToBuf(IApusysMem *cmdBuf, unsigned long long cmdId);

