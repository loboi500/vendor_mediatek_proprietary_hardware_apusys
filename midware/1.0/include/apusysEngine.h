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

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include "apusysCmd.h"
#include "apusysMem.h"
#include "apusys.h"

class apusysDev : public IApusysDev {
public:
    apusysDev();
    ~apusysDev();

    int type;
    int idx;

    unsigned long long hnd;
};

class apusysFirmware : public IApusysFirmware {
private:
public:
    apusysFirmware();
    ~apusysFirmware();

    std::string name;
    int type;
    int idx;
    unsigned int magic;

    IApusysMem * buf;

};

class apusysEngine : public IApusysEngine {
private:
    int mFd; // device fd

    std::string mUserName;
    pid_t mUserPid;
    std::vector<apusysDev *> mAllocDevList;
    std::mutex mAllocDevListMtx;
    std::unordered_map<unsigned long long, unsigned long long> mAsyncList; //<IApusysMem *, cmd_id>
    std::mutex mAsyncListMtx;
    std::unordered_map<unsigned long long, unsigned long long> mFirmwareList; //<IApusysFirmware *, IApusysMem *>
    std::mutex mFirmwareListMtx;
    std::map<int, int> mDevNum; // <device type, device num>

    /* from kernel */
    unsigned long long mMagicNum;
    unsigned char mCmdVersion;
    unsigned long long mDevSupport;
    unsigned int mMemSupport;
    unsigned int mVlmStart;
    unsigned int mVlmSize;

    int getDevNum(int type);

public:
    apusysEngine(const char * userName);
    apusysEngine();
    virtual ~apusysEngine();

    IMemAllocator * mDramAllocator;
    IMemAllocator * mVlmAllocator;

    /* cmd functions */
    IApusysCmd * initCmd();
    bool destroyCmd(IApusysCmd * cmd);
    bool runCmd(IApusysCmd * cmd);
    bool runCmdAsync(IApusysCmd * cmd);
    bool waitCmd(IApusysCmd * cmd);

    bool runCmd(IApusysMem * cmdBuf);
    bool runCmdAsync(IApusysMem * cmdBuf);
    bool waitCmd(IApusysMem * cmdBuf);
    bool checkCmdValid(IApusysMem * cmdBuf);

    bool sendUserCmdBuf(IApusysMem * cmdBuf, APUSYS_DEVICE_E deviceType);
    bool sendUserCmdBuf(IApusysMem * cmdBuf, APUSYS_DEVICE_E deviceType, int idx);

    /* memory functions */
    IApusysMem * memAlloc(size_t size);
    IApusysMem * memAlloc(size_t size, unsigned int align);
    IApusysMem * memAlloc(size_t size, unsigned int align, unsigned int cache);
    IApusysMem * memAlloc(size_t size, unsigned int align, unsigned int cache, APUSYS_USER_MEM_E type);
    bool memFree(IApusysMem * mem);
    bool memSync(IApusysMem * mem);
    bool memInvalidate(IApusysMem * mem);
    IApusysMem * memImport(int shareFd, unsigned int size);
    bool memUnImport(IApusysMem * mem);

    /* device */
    int getDeviceNum(APUSYS_DEVICE_E type);
    IApusysDev * devAlloc(APUSYS_DEVICE_E deviceType);
    bool devFree(IApusysDev * idev);

    /* power */
    bool setPower(APUSYS_DEVICE_E deviceType, unsigned int idx, unsigned int boostVal);

    /* firmware */
    IApusysFirmware * loadFirmware(APUSYS_DEVICE_E deviceType, unsigned int magic, unsigned int idx, std::string name, IApusysMem * fwBuf);
    bool unloadFirmware(IApusysFirmware * hnd);
};
