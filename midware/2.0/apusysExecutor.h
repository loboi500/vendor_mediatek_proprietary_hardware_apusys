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

#include <string>
#include <vector>
#include <unordered_map>
#include <map>

#include "apusysSession.h"

#define APUSYS_MINFO_HANDLE_SIZE (32)

/*
 * Abstraction layer to kernel interface
 */
class apusysExecutor {
protected:
    class apusysSession *mSession;
    std::vector<unsigned int>mDevNumList;

    uint32_t mMetaDataSize;
    std::map <int, std::string>mMetaMap;

public:
    apusysExecutor(class apusysSession *session);
    virtual ~apusysExecutor();

    static unsigned int getVersion(int devFd);
    unsigned int getDeviceNum(enum apusys_device_type type);
    virtual unsigned int getMetaDataSize();
    virtual int getMetaData(enum apusys_device_type type, void *metaData);

    virtual int setDevicePower(enum apusys_device_type type, unsigned int idx, unsigned int boost);
    virtual int sendUserCmd(enum apusys_device_type type, void *cmdbuf);

    /* cmd functions */
    virtual class apusysCmd *createCmd();
    virtual int deleteCmd(class apusysCmd *cmd);

    /* memory functions */
    virtual struct apusysMem *memAlloc(uint32_t size, uint32_t align, enum apusys_mem_type type, uint64_t flags);
    virtual int memFree(struct apusysMem *mem);
    virtual int memMapDeviceVa(struct apusysMem *mem);
    virtual int memUnMapDeviceVa(struct apusysMem *mem);
    virtual struct apusysMem *memImport(int handle, unsigned int size);
    virtual int memUnImport(struct apusysMem *mem);
    virtual int memFlush(struct apusysMem *mem);
    virtual int memInvalidate(struct apusysMem *mem);
};

class apusysExecutor_v1 : public apusysExecutor {
private:
    std::mutex mMtx;
    std::string mClientName;
    std::string mProcessName;
    int mCamIonAllocFreeLogEn;
    static int mIonFd;
    static int mIonFdRefCnt;
    static std::mutex mIonFdMtx;
    /* mem support infos */
    std::vector<struct apusysMem *>mMemInfos;
    std::unordered_map<struct apusysMem *, void *> mMemMap;
    struct apusysMem mVlmInfo;

    struct apusysMem *vlmAlloc(enum apusys_mem_type type, uint32_t size, uint32_t align, uint64_t flags);
    int vlmFree(struct apusysMem *mem);

    void setBufDbgName(int uHandle);

public:
    apusysExecutor_v1(class apusysSession *session);
    virtual ~apusysExecutor_v1();

    unsigned int getMetaDataSize();
    int getMetaData(enum apusys_device_type type, void *metaData);

    int setDevicePower(enum apusys_device_type type, unsigned int idx, unsigned int boost);
    int sendUserCmd(enum apusys_device_type type, void *cmdbuf);

    class apusysCmd *createCmd();
    int deleteCmd(class apusysCmd *cmd);

    /* memory functions */
    struct apusysMem *memAlloc(uint32_t size, uint32_t align, enum apusys_mem_type type, uint64_t flags);
    int memFree(struct apusysMem *mem);
    int memMapDeviceVa(struct apusysMem *mem);
    int memUnMapDeviceVa(struct apusysMem *mem);
    struct apusysMem *memImport(int handle, unsigned int size);
    int memUnImport(struct apusysMem *mem);
    int memFlush(struct apusysMem *mem);
    int memInvalidate(struct apusysMem *mem);
};

class apusysExecutor_v2 : public apusysExecutor {
private:
    /* mem support infos */
    struct apusysMem *getMemInfos(enum apusys_mem_type type);
    bool isMemInfos(struct apusysMem *mem);
    std::vector<struct apusysMem *>mMemInfos;
    std::unordered_map<void *, struct apusysMem *> mMemInfosMap;
    std::string mClientName;

    std::mutex mMtx;

public:
    apusysExecutor_v2(class apusysSession *session);
    virtual ~apusysExecutor_v2();

    unsigned int getMetaDataSize();
    int getMetaData(enum apusys_device_type type, void *metaData);

    int setDevicePower(enum apusys_device_type type, unsigned int idx, unsigned int boost);
    int sendUserCmd(enum apusys_device_type type, void *cmdbuf);

    /* cmd functions */
    class apusysCmd *createCmd();
    int deleteCmd(class apusysCmd *cmd);

    /* memory functions */
    struct apusysMem *memAlloc(uint32_t size, uint32_t align, enum apusys_mem_type type, uint64_t flags);
    int memFree(struct apusysMem *mem);
    int memMapDeviceVa(struct apusysMem *mem);
    int memUnMapDeviceVa(struct apusysMem *mem);
    struct apusysMem *memImport(int handle, unsigned int size);
    int memUnImport(struct apusysMem *mem);
    int memFlush(struct apusysMem *mem);
    int memInvalidate(struct apusysMem *mem);
};
