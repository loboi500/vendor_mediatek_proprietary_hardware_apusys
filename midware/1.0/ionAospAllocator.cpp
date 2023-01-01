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

#include <ion/ion.h>  // system/memory/libion/include/
#include <linux/ion_drv.h>
#include <linux/dma-buf.h>
#include <ion.h>  // libion_mtk/include/ion.h
#include <mt_iommu_port.h>
#include <sys/mman.h>
#include <../kernel-headers/linux/ion_4.19.h>  // system/memory/libion/include/
#include "apusysMem.h"
#include "apusys_drv.h"
#include "ionAospAllocator.h"
#include "apusysCmn.h"

#if 1 //def MTK_APUSYS_IOMMU_LEGACY
#define APUSYS_IOMMU_PORT M4U_PORT_VPU
#else
#define APUSYS_IOMMU_PORT M4U_PORT_L21_APU_FAKE_DATA
#endif

#define MAX_HEAP_COUNT ION_HEAP_TYPE_CUSTOM

apusysIonAospMem::apusysIonAospMem()
{
    va = 0;
    iova = 0;
    iova_size = 0;
    offset = 0;
    size = 0;
    align = 0;

    type = APUSYS_USER_MEM_DRAM;

    /* ion specific */
    khandle = 0;
    ion_share_fd = 0;
    ionLen = 0;
    ionIova = 0;

}

apusysIonAospMem::~apusysIonAospMem()
{

}

int apusysIonAospMem::getMemFd()
{
    return ion_share_fd;
}

ionAospAllocator::ionAospAllocator(int devFd)
{
    int ret = 0;

    DEBUG_TAG;

    /* check apusys fd */
    if (devFd <= 0)
    {
        LOG_ERR("invalid dev fd(%d)\n", devFd);
        assert(devFd >= 0);
    }

    /* allocate ion fd */
    mIonFd = open("/dev/ion", O_RDWR);
    if (mIonFd < 0)
    {
        LOG_ERR("create ion fd fail, errno(%d/%s)\n", errno, strerror(errno));
        assert(mIonFd >= 0);
    }

    ret = ion_is_legacy(mIonFd);
    if(ret)
    {
        LOG_ERR("ion is mtk legacy ion return(%d)\n", ret);
        assert(ret == 0);
    }

    mMemList.clear();
    mDevFd = devFd;

    LOG_DEBUG("ion allocator init done\n");

    return;
}

ionAospAllocator::~ionAospAllocator()
{
    if (mIonFd)
        close(mIonFd);

    return;
}

IApusysMem * ionAospAllocator::alloc(unsigned int size, unsigned int align, unsigned int cache)
{
    struct apusys_mem ctlmem;
    apusysIonAospMem * ionMem = nullptr;
    int shareFd = 0;
    int ret = 0;
    unsigned long long uva;
    std::unique_lock<std::mutex> mutexLock(mMemListMtx);
    struct ion_heap_data *heap_data;
    unsigned int heap_id;
    int i, heap_cnt;
    unsigned int flag;

    if((align > PAGE_ALIGN) || ( PAGE_ALIGN % align !=0 ))
    {
        LOG_ERR("fail to get ion buffer align must be %d Not %d\n", PAGE_ALIGN, align);
        return nullptr;
    }

    /* query the number of heaps ion driver can support */
    ret = ion_query_heap_cnt(mIonFd, &heap_cnt);
    if (ret < 0) {
        LOG_ERR("ion_query_heap_cnt Failed %s\n", strerror(errno));
        return nullptr;
    }

    heap_data = (struct ion_heap_data *) std::malloc(heap_cnt * sizeof(*heap_data));
    if (heap_data == nullptr) {
        LOG_ERR("std::malloc Failed\n");
        return nullptr;
    }
    memset(heap_data, 0, heap_cnt * sizeof(*heap_data));
    ret = ion_query_get_heaps(mIonFd, heap_cnt, heap_data);
    if (ret < 0) {
        LOG_ERR("ion_query_get_heaps Failed: %s\n", strerror(errno));
        goto err_query;
    }
    /* get heap id by the heap name */
    for (i = 0; i < heap_cnt; i++)
    {
        if (strcmp(heap_data[i].name, "ion_system_heap") == 0)
        {
            heap_id = heap_data[i].heap_id;
            break;
        }
    }
    if (i >= heap_cnt)
    {
        LOG_ERR("get_ion_heap_id Failed: %s\n", strerror(errno));
        goto err_query;
    }


    /* ion alloc buffer + mmap */
    if(cache > 0)
    {
        flag = ION_FLAG_CACHED;
    } else
    {
        flag = 0;
    }


    ret = ion_alloc_fd(mIonFd, size, 0, 1 << heap_id, flag, &shareFd);
    if (ret < 0) {
        LOG_ERR("ion_alloc_fd(): %s (%d)\n", strerror(errno), ret);
        goto err_alloc;
    }

    /* mmap buffer */
    uva = (unsigned long long) mmap(NULL, size, PROT_READ|PROT_WRITE,
    MAP_SHARED, shareFd, 0);
    if ((void *) uva == MAP_FAILED) {
        LOG_ERR("ion mmap fail %s\n", strerror(errno));
        goto err_mmap;
    }

    memset(&ctlmem, 0, sizeof(struct apusys_mem));
    /* get iova from kernel */
    ctlmem.uva = uva;
    ctlmem.iova = 0;
    ctlmem.size = size;
    ctlmem.iova_size = 0;
    ctlmem.mem_type = APUSYS_MEM_DRAM_ION_AOSP;
    ctlmem.align = align;
    ctlmem.fd = shareFd;

    ret = ioctl(mDevFd, APUSYS_IOCTL_MEM_MAP, &ctlmem);
    if (ret)
    {
        LOG_ERR("APUSYS_IOCTL_MEM_MAP: %s (%d)\n",
            strerror(errno), errno);
        goto err_iovamap;
    }

    /* allocate apusysMem */
    ionMem = new apusysIonAospMem;
    if (ionMem == nullptr)
    {
        LOG_ERR("allocate apusys ion mem struct fail\n");
        goto err_apusysmem;
    }

    ionMem->khandle = ctlmem.khandle;
    ionMem->va = uva;
    ionMem->iova = ctlmem.iova;
    ionMem->offset = 0;
    ionMem->size = size;
    ionMem->iova_size = ctlmem.iova_size;
    ionMem->ion_share_fd = shareFd;
    ionMem->type = APUSYS_USER_MEM_DRAM;

    this->print(ionMem);

    mMemList.push_back(ionMem);

    std::free((void *)heap_data);

    return (IApusysMem *)ionMem;

err_apusysmem:
    ret = ioctl(mDevFd, APUSYS_IOCTL_MEM_UNMAP, &ctlmem);
    if (ret)
    {
        LOG_ERR("APUSYS_IOCTL_MEM_UNMAP: %s (%d)\n",
            strerror(ret), ret);
    }
err_iovamap:
    /* unmap the buffer properly in the end */
    if(munmap((void*)uva, size))
    {
        LOG_ERR("ion unmap fail\n");
    }
err_mmap:
    close(shareFd);
err_alloc:
err_query:
    std::free((void *)heap_data);

    return nullptr;
}

bool ionAospAllocator::free(IApusysMem * mem)
{
    apusysIonAospMem * ionMem = (apusysIonAospMem *)mem;
    struct apusys_mem ctlmem;
    int ret = 0;
    std::vector<IApusysMem *>::iterator iter;
    std::unique_lock<std::mutex> mutexLock(mMemListMtx);

    /* check argument */
    if (mem == nullptr)
    {
        LOG_ERR("invalid argument\n");
        return false;
    }

    /* delete from mem list */
    for (iter = mMemList.begin(); iter != mMemList.end(); iter++)
    {
        if (ionMem == *iter)
        {
            break;
        }
    }
    if (iter == mMemList.end())
    {
        LOG_ERR("find ionMem from mem list fail(%p)\n", (void *)ionMem);
        return false;
    }

    mMemList.erase(iter);

    memset(&ctlmem, 0, sizeof(struct apusys_mem));
    /* get kva from kernel */

    ctlmem.khandle = ionMem->khandle;
    ctlmem.uva = ionMem->va;
    ctlmem.size = ionMem->size;
    ctlmem.iova = ionMem->iova;
    ctlmem.iova_size = ionMem->iova_size;
    ctlmem.align = ionMem->align;
    ctlmem.mem_type = APUSYS_MEM_DRAM_ION_AOSP;
    ctlmem.fd = ionMem->ion_share_fd;

    ret = ioctl(mDevFd, APUSYS_IOCTL_MEM_UNMAP, &ctlmem);
    if (ret)
    {
        LOG_ERR("APUSYS_IOCTL_MEM_UNMAP: %s (%d)\n",
            strerror(ret), ret);
        return false;
    }


    /* unmap the buffer properly in the end */
    if (munmap((void*)ionMem->va, ionMem->size))
    {
        LOG_ERR("ion unmap fail\n");
        return false;
    }
    /* close the buffer fd */
    if (close(ionMem->ion_share_fd))
    {
        LOG_ERR("ion close share fail\n");
        return false;
    }

    this->print(ionMem);

    //LOG_DEBUG("mMemList size (%lu)\n", mMemList.size());
    delete ionMem;

    return true;
}

bool ionAospAllocator::flush(IApusysMem * mem)
{
    struct dma_buf_sync  sync_start;
    apusysIonAospMem * ionMem = (apusysIonAospMem *)mem;
    int ret = 0;

    sync_start.flags = DMA_BUF_SYNC_RW | DMA_BUF_SYNC_START;
    ret = ioctl(ionMem->ion_share_fd, DMA_BUF_IOCTL_SYNC, &sync_start);
    if (ret < 0) {
        LOG_ERR("ion invalidate fail %d (%s)\n", ret, strerror(errno));
        return false;
    }
    sync_start.flags = DMA_BUF_SYNC_RW | DMA_BUF_SYNC_END;
    ret = ioctl(ionMem->ion_share_fd, DMA_BUF_IOCTL_SYNC, &sync_start);
    if (ret < 0) {
        LOG_ERR("ion invalidate fail %d (%s)\n", ret, strerror(errno));
        return false;
    }

    this->print(ionMem);

    return true;
}

bool ionAospAllocator::invalidate(IApusysMem * mem)
{
    struct dma_buf_sync sync_start;
    apusysIonAospMem * ionMem = (apusysIonAospMem *)mem;
    int ret = 0;

    sync_start.flags = DMA_BUF_SYNC_READ | DMA_BUF_SYNC_START;
    ret = ioctl(ionMem->ion_share_fd, DMA_BUF_IOCTL_SYNC, &sync_start);
    if (ret < 0) {
        LOG_ERR("ion invalidate fail %d (%s)\n", ret, strerror(errno));
        return false;
    }
    sync_start.flags = DMA_BUF_SYNC_READ | DMA_BUF_SYNC_END;
    ret = ioctl(ionMem->ion_share_fd, DMA_BUF_IOCTL_SYNC, &sync_start);
    if (ret < 0) {
        LOG_ERR("ion invalidate fail %d (%s)\n", ret, strerror(errno));
        return false;
    }


    this->print(ionMem);

    return true;
}

IApusysMem * ionAospAllocator::import(int shareFd, unsigned int size)
{
    struct apusys_mem ctlmem;
    apusysIonAospMem * ionMem = nullptr;
    int ret = 0;
    unsigned long long uva;

    /* mmap buffer */
    uva = (unsigned long long) mmap(NULL, size, PROT_READ|PROT_WRITE,
    MAP_SHARED, shareFd, 0);
    if ((void *) uva == MAP_FAILED) {
        LOG_ERR("mmap() failed: %d, %s\n", errno, strerror(errno));
        goto err_mmap;
    }
    if (!uva) {
        LOG_ERR("mmap() returned null\n");
        goto err_mmap;
    }

    memset(&ctlmem, 0, sizeof(struct apusys_mem));
    /* get iova from kernel */
    ctlmem.uva = uva;
    ctlmem.iova = 0;
    ctlmem.iova_size = 0;
    ctlmem.mem_type = APUSYS_MEM_DRAM_ION;
    ctlmem.align = 0;
    ctlmem.fd = shareFd;

    //LOG_DEBUG("ctlmem(%d/%d/0x%x/0x%x/0x%llx/0x%llx/%d)\n", mDevFd, mIonFd, ctlmem.iova, ctlmem.iova_size, ctlmem.uva, ctlmem.kva, ctlmem.size);

    ret = ioctl(mDevFd, APUSYS_IOCTL_MEM_IMPORT, &ctlmem);
    if(ret)
    {
        LOG_ERR("ioctl mem ctl fail(%d)\n", ret);
        goto err_iovamap;
    }
    //LOG_DEBUG("ctlmem(%d/%d/0x%x/0x%x/0x%llx/0x%llx/%d)\n", mDevFd, mIonFd, ctlmem.iova, ctlmem.iova_size, ctlmem.uva, ctlmem.kva, ctlmem.size);


    /* allocate apusysMem */
    ionMem = new apusysIonAospMem;
    if (ionMem == nullptr)
    {
        LOG_ERR("allocate apusys ion mem struct fail\n");
        goto err_apusysmem;
    }

    ionMem->khandle = ctlmem.khandle;
    ionMem->va = uva;
    ionMem->iova = ctlmem.iova;
    ionMem->iova_size = ctlmem.iova_size;
    ionMem->offset = 0;
    ionMem->size = size;
    ionMem->ion_share_fd = shareFd;

    this->print(ionMem);

    mMemList.push_back(ionMem);
    //LOG_DEBUG("mMemList size (%lu)\n", mMemList.size());

    return (IApusysMem *)ionMem;

err_apusysmem:
    ret = ioctl(mDevFd, APUSYS_IOCTL_MEM_UNIMPORT, &ctlmem);
    if(ret)
    {
        LOG_ERR("ioctl mem ctl fail(%d)\n", ret);
    }
err_iovamap:
    /* unmap the buffer properly in the end */
    if(munmap((void *)uva, size))
    {
        LOG_ERR("ion unmap fail\n");
    }
err_mmap:
    return nullptr;
}

bool ionAospAllocator::unimport(IApusysMem * mem)
{
    apusysIonAospMem * ionMem = (apusysIonAospMem *)mem;
    struct apusys_mem ctlmem;
    int ret = 0;
    bool isFind = false;
    std::vector<IApusysMem *>::iterator iter;

    /* check argument */
    if (mem == nullptr)
    {
        LOG_ERR("invalid argument\n");
        return false;
    }

    /* delete from mem list */
    for (iter = mMemList.begin(); iter != mMemList.end(); iter++)
    {
        if (ionMem == *iter)
        {
            isFind = true;
            break;
        }
    }
    if (!isFind)
    {
        LOG_ERR("find ionMem from mem list fail(%p)\n", (void *)ionMem);
        return false;
    }

    mMemList.erase(iter);

    memset(&ctlmem, 0, sizeof(struct apusys_mem));

    ctlmem.khandle = ionMem->khandle;
    ctlmem.uva = ionMem->va;
    ctlmem.iova = ionMem->iova;
    ctlmem.iova_size = ionMem->iova_size;
    ctlmem.size = ionMem->size;
    ctlmem.align = ionMem->align;
    ctlmem.mem_type = APUSYS_MEM_DRAM_ION;
    ctlmem.fd = ionMem->ion_share_fd;

    ret = ioctl(mDevFd, APUSYS_IOCTL_MEM_UNIMPORT, &ctlmem);
    if(ret)
    {
        LOG_ERR("ioctl mem unmap iova fail(%d)\n", ret);
        return false;
    }

    /* unmap the buffer properly in the end */
    if(munmap((void*)ionMem->va, ionMem->size))
    {
        LOG_ERR("ion unmap fail\n");
        return false;
    }

    this->print(ionMem);
    //LOG_DEBUG("mMemList size (%lu)\n", mMemList.size());

    delete ionMem;

    return true;
}

void ionAospAllocator::print(IApusysMem * mem)
{
    apusysIonAospMem * ionMem = (apusysIonAospMem *)mem;

    MLOG_DEBUG("ionMem(%d/0x%llx/0x%x/0x%x/%d/0x%x/0x%llx/%p)\n",
        ionMem->ion_share_fd, ionMem->va, ionMem->iova,
        ionMem->iova + ionMem->iova_size - 1,
        ionMem->size, ionMem->iova_size,
        ionMem->khandle, (void *)ionMem);
}
