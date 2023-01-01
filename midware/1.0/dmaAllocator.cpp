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
#include <string>
#include <vector>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <error.h>

#include "apusysMem.h"
#include "apusys_drv.h"
#include "dmaAllocator.h"
#include "apusysCmn.h"

apusysDmaMem::apusysDmaMem()
{
    va = 0;
    iova = 0;
    iova_size = 0;
    offset = 0;
    size = 0;
    align = 0;

    type = APUSYS_USER_MEM_DRAM;

    /* dma specific */
    mMemFd = 0;
}

apusysDmaMem::~apusysDmaMem()
{

}

int apusysDmaMem::getMemFd()
{
    return mMemFd;
}

dmaAllocator::dmaAllocator(int devFd)
{

    /* check apusys fd */
    if (devFd <= 0)
    {
        LOG_ERR("invalid dev fd(%d)\n", devFd);
        assert (devFd >= 0);
    }

    mDevFd = devFd;
}

dmaAllocator::~dmaAllocator()
{
}

IApusysMem * dmaAllocator::alloc(unsigned int size)
{
    return this->alloc(size, PAGE_ALIGN, 0);
}

IApusysMem * dmaAllocator::alloc(unsigned int size, unsigned int align, unsigned int cache)
{
    struct apusys_mem ioctlMem;
    int ret = 0;
    unsigned long long uva;
    apusysDmaMem * dmaMem = nullptr;
    UNUSED(align);
    UNUSED(cache);

    ioctlMem.iova = 0;
    ioctlMem.size = size;
    ioctlMem.mem_type = APUSYS_MEM_DRAM_DMA;

    ret = ioctl (mDevFd, APUSYS_IOCTL_MEM_ALLOC, &ioctlMem);
    if (ret)
    {
        LOG_ERR("ioctl mem alloc fail(%d)\n", ret);
        return NULL;
    }
    uva = (unsigned long long) mmap (NULL, ioctlMem.size, PROT_READ | PROT_WRITE,
                           MAP_SHARED, mDevFd, (size_t) ioctlMem.iova);

    if ((void*) uva == MAP_FAILED)
    {
        LOG_ERR("mmap() fail, error: %d, %s\n", errno, strerror (errno));
        goto release_mem;
    }

    dmaMem = new apusysDmaMem;
    if (dmaMem == nullptr)
    {
        LOG_ERR("allocate apusys dma mem struct fail\n");
        goto unmap_mem;
    }

    dmaMem->va = uva;
    dmaMem->iova = ioctlMem.iova;
    dmaMem->offset = 0;
    dmaMem->size = ioctlMem.size;
    dmaMem->type = APUSYS_USER_MEM_DRAM;
    LOG_DEBUG("alloc dmaMem(0x%x/0x%llx/%d)\n", dmaMem->iova, dmaMem->va, dmaMem->size);

    mMemList.push_back (dmaMem);

    return (IApusysMem *) dmaMem;

unmap_mem:
    ret = munmap ((void*) uva, (size_t) ioctlMem.size);
    if (ret)
    {
        LOG_ERR("munmap fail(%d)\n", ret);
    }

release_mem:
    ret = ioctl (mDevFd, APUSYS_IOCTL_MEM_FREE, &ioctlMem);
    if (ret)
    {
        LOG_ERR("ioctl mem free fail(%d)\n", ret);
    }
    return NULL;
}

bool dmaAllocator::free(IApusysMem * mem)
{
    apusysDmaMem * dmaMem = nullptr;
    struct apusys_mem ioctlMem;
    int ret = 0;
    auto iter = mMemList.begin();

    dmaMem = (apusysDmaMem *) mem;

    /* delete from mem list */
    for (iter = mMemList.begin(); iter != mMemList.end(); iter++)
    {
        if (dmaMem == *iter)
        {
            break;
        }
    }
    if (iter == mMemList.end())
    {
        LOG_ERR("find dmaMem from mem list fail(%p)\n", (void *)dmaMem);
        return false;
    }

    mMemList.erase(iter);

    ioctlMem.iova = dmaMem->iova;
    ioctlMem.size = dmaMem->size;
    ioctlMem.mem_type = APUSYS_MEM_DRAM_DMA;

    ret = munmap ((void*) dmaMem->va, (size_t) dmaMem->size);
    if (ret)
    {
        LOG_ERR("munmap fail(%d)\n", ret);
        return false;
    }

    ret = ioctl (mDevFd, APUSYS_IOCTL_MEM_FREE, &ioctlMem);
    if (ret)
    {
        LOG_ERR("ioctl mem alloc fail(%d)\n", ret);
        return false;
    }

    LOG_DEBUG("free dmaMem(0x%x/0x%llx/%d)\n", dmaMem->iova, dmaMem->va, dmaMem->size);
    delete dmaMem;

    return true;

}

bool dmaAllocator::flush(IApusysMem * mem)
{
    apusysDmaMem * dmaMem = nullptr;

    dmaMem = (apusysDmaMem *) mem;
    LOG_DEBUG("dmaMem(0x%x/0x%llx/%d)\n", dmaMem->iova, dmaMem->va, dmaMem->size);
    if (msync ((void*) dmaMem->va, (size_t) dmaMem->size, MS_INVALIDATE | MS_SYNC)) {
        LOG_ERR("msync fail %s\n", strerror(errno));
        return false;
    }
    LOG_DEBUG("Done\n");
    return true;
}

bool dmaAllocator::invalidate(IApusysMem * mem)
{
    apusysDmaMem * dmaMem = nullptr;

    dmaMem = (apusysDmaMem *) mem;
    if (msync ((void*) dmaMem->va, (size_t) dmaMem->size, MS_INVALIDATE)) {
        LOG_ERR("msync fail %s\n", strerror(errno));
        return false;
    }
    LOG_DEBUG("Done\n");
    return true;
}

IApusysMem * dmaAllocator::import(int shareFd, unsigned int size)
{
    UNUSED(shareFd);
    UNUSED(size);

    LOG_ERR("DMA Not Support Import!\n");
    return nullptr;
}

bool dmaAllocator::unimport(IApusysMem * mem)
{
    UNUSED(mem);

    LOG_ERR("DMA Not Support UnImport!\n");
    return false;
}
