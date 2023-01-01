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

#include "apusys.h"
#include "apusysCmd.h"

struct apusysMem {
    int handle;
    void *vaddr;
    uint32_t size;
    uint32_t align;
    uint64_t deviceVa;
    int mtype; //dram/vlm
    bool cacheable;
    bool highaddr;
};

struct apusysCmdBuf {
    enum apusys_cb_direction dir;
    struct apusysMem *mem;
};

#define APUSYS_SUBCMD_BOOST_MAX (100)

class apusysSession {
private:
    int mDevFd;
    uint32_t mMetaDataSize;
    int mVersion;
    std::string mThdName;
    std::unordered_map<void *, struct apusysMem *> mMemMap;
    std::unordered_map<void *, struct apusysCmdBuf *> mCmdBufMap;
    std::mutex mMemMapMtx;
    std::vector<class apusysCmd *> mCmdList;
    std::mutex mCmdListMtx;

    class apusysExecutor *mExecutor;

    struct apusysMem *getMemSizeObj(enum apusys_mem_type type);

public:
    apusysSession(int devFd);
    ~apusysSession();

    /* get device fd */
    int getDevFd();
    struct apusysCmdBuf *cmdBufGetObj(void *vaddr);
    struct apusysMem *memGetObj(void *vaddr);

    /* basic functions */
    uint64_t queryInfo(enum apusys_session_info type);
    uint32_t queryDeviceNum(enum apusys_device_type type);
    int queryDeviceMetaData(enum apusys_device_type type, void *meta);
    int setDevicePower(enum apusys_device_type type, unsigned int idx, unsigned int boost);
    int sendUserCmd(enum apusys_device_type type, void *cmdbuf);

    /* cmdbuf functions */
    void *cmdBufAlloc(uint32_t size, uint32_t align);
    int cmdBufFree(void *vaddr);

    /* memory functions */
    void *memAlloc(uint32_t size, uint32_t align, enum apusys_mem_type type, uint64_t flags);
    int memFree(void *vaddr);
    void *memImport(int handle, unsigned int size);
    int memUnImport(void *vaddr);
    int memFlush(void *vaddr);
    int memInvalidate(void *vaddr);
    uint64_t memGetInfoFromHostPtr(void *vaddr, enum apusys_mem_info type);
    int memSetParamViaHostPtr(void *vaddr, enum apusys_mem_info type, uint64_t val);

    /* cmd functions */
    class apusysCmd *createCmd();
    int deleteCmd(class apusysCmd *cmd);
};
