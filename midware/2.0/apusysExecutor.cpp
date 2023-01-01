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

#include "apusys_drv.h"
#include "apusysCmn.h"
#include "apusysExecutor.h"
#include "apusysSession.h"
/* TODO */
#include "./v2_0/mdw_drv.h"

#define APUSYS_MEM_TMP_OFFSET (0x800000000000)

apusysExecutor::apusysExecutor(class apusysSession *session)
{
    if (session == nullptr)
        LOG_ERR("invalid session input\n");

    mSession = session;
    mMetaDataSize = 0;
    mMetaMap.clear();
    mDevNumList.clear();
    mDevNumList.resize(64);
}

apusysExecutor::~apusysExecutor()
{
}

unsigned int apusysExecutor::getVersion(int devFd)
{
    struct apusys_ioctl_hs ioctlHandShake;
    union mdw_hs_args hs;
    unsigned int version = 0;
    unsigned int mMemSupport;


    memset(&ioctlHandShake, 0, sizeof(ioctlHandShake));
    ioctlHandShake.type = APUSYS_HANDSHAKE_BEGIN;
    /* handshake with driver */
    if (ioctl(devFd, APUSYS_IOCTL_HANDSHAKE, &ioctlHandShake) == 0) {
        mMemSupport = ioctlHandShake.begin.mem_support;

        if (mMemSupport & 1UL << APUSYS_MEM_DRAM_ION) {
            LOG_DEBUG("MTK ION allocator 0x%x\n", mMemSupport);
            version = 1;
        } else if(mMemSupport & 1UL << APUSYS_MEM_DRAM_DMA) {
            LOG_DEBUG("DMA allocator 0x%x\n", mMemSupport);
        } else {
            LOG_ERR("Unsupported allocator: 0x%x\n", mMemSupport);
        }
    } else {
        memset(&hs, 0, sizeof(hs));
        hs.in.op = MDW_HS_IOCTL_OP_BASIC;
        if (ioctl(devFd, APU_MDW_IOCTL_HANDSHAKE, &hs)) {
            LOG_ERR("handshake fail(%s)\n", strerror(errno));
            version = 2;
        } else {
            if (hs.out.arg.basic.version <= 2)
                version = 2;
            else
                version = hs.out.arg.basic.version;
        }
    }

    return version;
}

unsigned int apusysExecutor::getDeviceNum(enum apusys_device_type type)
{
    if (type >= this->mDevNumList.size())
        return 0;

    return this->mDevNumList.at(type);
}

unsigned int apusysExecutor::getMetaDataSize()
{
    return 0;
}

int apusysExecutor::getMetaData(enum apusys_device_type type, void *metaData)
{
    UNUSED(metaData);
    UNUSED(type);
    return -EINVAL;
}

int apusysExecutor::setDevicePower(enum apusys_device_type type, unsigned int idx, unsigned int boost)
{
    UNUSED(type);
    UNUSED(idx);
    UNUSED(boost);
    return -EINVAL;
}

int apusysExecutor::sendUserCmd(enum apusys_device_type type, void *cmdbuf)
{
    UNUSED(type);
    UNUSED(cmdbuf);
    return -EINVAL;
}

class apusysCmd *apusysExecutor::createCmd()
{
    class apusysCmd *cmd = nullptr;

    cmd = new apusysCmd(mSession);

    return cmd;
}

int apusysExecutor::deleteCmd(class apusysCmd *cmd)
{
    delete cmd;
    return 0;
}

struct apusysMem *apusysExecutor::memAlloc(uint32_t size, uint32_t align, enum apusys_mem_type type, uint64_t flags)
{
    struct apusysMem *mem = nullptr;
    UNUSED(align);

    if (type == APUSYS_MEM_TYPE_VLM && size == 0)
        return nullptr;

    /* allocate for apusys mem information */
    mem = new apusysMem;
    if (mem == nullptr)
        return nullptr;
    memset(mem, 0, sizeof(*mem));

    mem->highaddr = (flags & F_APUSYS_MEM_HIGHADDR) ? true : false;
    mem->cacheable = (flags & F_APUSYS_MEM_CACHEABLE) ? true : false;
    mem->mtype = type;
    mem->align = align;
    mem->size = size;
    mem->vaddr = malloc(mem->size);
    if (mem->vaddr == nullptr) {
        LOG_ERR("allocate apusys memory(%u) fail\n", mem->size);
        delete mem;
        return nullptr;
    }
    memset(mem->vaddr, 0, mem->size);

    mem->deviceVa = reinterpret_cast<uint64_t>(mem->vaddr) | APUSYS_MEM_TMP_OFFSET;

    return mem;
}

int apusysExecutor::memFree(struct apusysMem *mem)
{
    if (mem == nullptr)
        return -EINVAL;

    free(mem->vaddr);
    delete mem;

    return 0;
}

int apusysExecutor::memMapDeviceVa(struct apusysMem *mem)
{
    UNUSED(mem);
    return -EINVAL;
}

int apusysExecutor::memUnMapDeviceVa(struct apusysMem *mem)
{
    UNUSED(mem);
    return -EINVAL;
}

struct apusysMem *apusysExecutor::memImport(int handle, unsigned int size)
{
    UNUSED(handle);
    UNUSED(size);
    return nullptr;
}

int apusysExecutor::memUnImport(struct apusysMem *mem)
{
    UNUSED(mem);
    return -EINVAL;
}

int apusysExecutor::memFlush(struct apusysMem *mem)
{
    UNUSED(mem);
    return -EINVAL;
}

int apusysExecutor::memInvalidate(struct apusysMem *mem)
{
    UNUSED(mem);
    return -EINVAL;
}
