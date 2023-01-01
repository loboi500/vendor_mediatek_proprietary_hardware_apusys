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
#include <errno.h>
#include <poll.h>
#ifdef __ANDROID__
#include <cutils/properties.h>
#endif

#include "apusys_drv.h"
#include "apusys.h"
#include "apusysCmn.h"
#include "apusysSession.h"
#include "apusysExecutor.h"
#include "apusysCmdFormat.h"


class apusysSubCmd_v1 : public apusysSubCmd {
public:
    apusysSubCmd_v1(class apusysCmd *parent, enum apusys_device_type type, uint32_t idx);
    virtual ~apusysSubCmd_v1();

    int construct(struct apusysMem *mem, unsigned int hdrOfs, unsigned int cmdBufOfs, unsigned int packId);
    int updateCmdBuf(bool beforeExec);
    int updateHeader(void *addr);

    uint64_t getRunInfo(enum apusys_subcmd_runinfo_type type);
};

class apusysCmd_v1 : public apusysCmd {
public:
    apusysCmd_v1(class apusysSession *session);
    virtual ~apusysCmd_v1();

    class apusysSubCmd *createSubCmd(enum apusys_device_type type);
    int build();
    int run();
    int runAsync();
    int wait();
    int runFence(int fence, uint64_t flags);
};

//----------------------------------------------
// apusysSubCmd_v1 implementation
apusysSubCmd_v1::apusysSubCmd_v1(class apusysCmd *parent, enum apusys_device_type type, uint32_t idx):
    apusysSubCmd(parent, type, idx)
{
    UNUSED(parent);
    UNUSED(type);
    UNUSED(idx);
    LOG_ERR("LD 2.0 not support libapu_mdw v1 executor\n");
}

apusysSubCmd_v1::~apusysSubCmd_v1()
{
}

int apusysSubCmd_v1::updateHeader(void *addr)
{
    UNUSED(addr);
    return -EFAULT;
}

int apusysSubCmd_v1::updateCmdBuf(bool beforeExec)
{
    UNUSED(beforeExec);
    return -EFAULT;
}

uint64_t apusysSubCmd_v1::getRunInfo(enum apusys_subcmd_runinfo_type type)
{
    UNUSED(type);
    return 0;
}

int apusysSubCmd_v1::construct(struct apusysMem *mem, unsigned int hdrOfs, unsigned int cmdBufOfs, unsigned int packId)
{
    UNUSED(mem);
    UNUSED(hdrOfs);
    UNUSED(cmdBufOfs);
    UNUSED(packId);
    return -EFAULT;
}

//----------------------------------------------
// apusysCmd_v1 implementation
apusysCmd_v1::apusysCmd_v1(class apusysSession *session): apusysCmd(session)
{
    UNUSED(session);
    LOG_ERR("LD 2.0 not support libapu_mdw v1 executor\n");
}

apusysCmd_v1::~apusysCmd_v1()
{
}

class apusysSubCmd *apusysCmd_v1::createSubCmd(enum apusys_device_type type)
{
    UNUSED(type);
    return nullptr;
}

int apusysCmd_v1::build()
{
    return -EFAULT;
}

int apusysCmd_v1::run()
{
    return -EFAULT;
}

int apusysCmd_v1::runAsync()
{
    return -EFAULT;
}

int apusysCmd_v1::wait()
{
    return -EFAULT;
}

int apusysCmd_v1::runFence(int fence, uint64_t flags)
{
    UNUSED(fence);
    UNUSED(flags);
    return -EFAULT;
}

//------------------------------
apusysExecutor_v1::apusysExecutor_v1(class apusysSession *session) : apusysExecutor(session)
{
    UNUSED(session);
    memset(&mVlmInfo, 0, sizeof(mVlmInfo));
    mCamIonAllocFreeLogEn = 0;
    LOG_ERR("LD 2.0 not support libapu_mdw v1 executor\n");
}

apusysExecutor_v1::~apusysExecutor_v1()
{
}

unsigned int apusysExecutor_v1::getMetaDataSize()
{
    return 0;
}

int apusysExecutor_v1::getMetaData(enum apusys_device_type type, void *metaData)
{
    /* not support */
    UNUSED(type);
    UNUSED(metaData);
    return -EINVAL;
}

int apusysExecutor_v1::setDevicePower(enum apusys_device_type type, unsigned int idx, unsigned int boost)
{
    UNUSED(type);
    UNUSED(idx);
    UNUSED(boost);
    return -EFAULT;
}

int apusysExecutor_v1::sendUserCmd(enum apusys_device_type type, void *cmdbuf)
{
    UNUSED(type);
    UNUSED(cmdbuf);
    return -EFAULT;
}

class apusysCmd *apusysExecutor_v1::createCmd()
{
    return nullptr;
}

int apusysExecutor_v1::deleteCmd(class apusysCmd *cmd)
{
    UNUSED(cmd);
    return -EFAULT;
}

void apusysExecutor_v1::setBufDbgName(int uHandle)
{
    UNUSED(uHandle);
}

struct apusysMem *apusysExecutor_v1::memAlloc(uint32_t size, uint32_t align, enum apusys_mem_type type, uint64_t flags)
{
    UNUSED(size);
    UNUSED(align);
    UNUSED(type);
    UNUSED(flags);
    return nullptr;
}

int apusysExecutor_v1::memFree(struct apusysMem *mem)
{
    UNUSED(mem);
    return -EFAULT;
}

int apusysExecutor_v1::memMapDeviceVa(struct apusysMem *mem)
{
    UNUSED(mem);
    return -EINVAL;
}

int apusysExecutor_v1::memUnMapDeviceVa(struct apusysMem *mem)
{
    UNUSED(mem);
    return -EINVAL;
}

struct apusysMem *apusysExecutor_v1::memImport(int handle, unsigned int size)
{
    UNUSED(handle);
    UNUSED(size);
    return nullptr;
}

int apusysExecutor_v1::memUnImport(struct apusysMem *mem)
{
    UNUSED(mem);
    return -EFAULT;
}

int apusysExecutor_v1::memFlush(struct apusysMem *mem)
{
    UNUSED(mem);
    return -EFAULT;
}

int apusysExecutor_v1::memInvalidate(struct apusysMem *mem)
{
    UNUSED(mem);
    return -EFAULT;
}
