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
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include <ion/ion.h> //system/core/libion/include/ion.h
#include <linux/ion_drv.h>
#include <ion.h> //libion_mtk/include/ion.h
#include <mt_iommu_port.h>
#include <sys/mman.h>
#include <cutils/properties.h>

#include "apusysMem.h"
#include "apusys_drv.h"
#include "ionAllocator.h"
#include "apusysCmn.h"

#ifdef MTK_APUSYS_IOMMU_LEGACY
#define APUSYS_IOMMU_PORT M4U_PORT_VPU
#else
#define APUSYS_IOMMU_PORT M4U_PORT_L21_APU_FAKE_DATA
#endif

#define ION_CLIENT_NAME "apusysMemAllocator:"
#define MTK_CAM_ION_LOG_THREAD (10*1024*1024) //10MB

/* init static variable in ion allocator */
int ionAllocator::mIonFdRefCnt = 0;
int ionAllocator::mIonFd = -1;
std::mutex ionAllocator::mIonFdMutex;

apusysIonMem::apusysIonMem()
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
    ion_uhandle = 0;
    ionLen = 0;
    ionIova = 0;

}

apusysIonMem::~apusysIonMem()
{

}

int apusysIonMem::getMemFd()
{
    return ion_share_fd;
}

ionAllocator::ionAllocator(int devFd)
{
    char thdName[16];
    const char *processName;
    pthread_t thdInst;

    /* check apusys fd */
    if (devFd <= 0)
    {
        LOG_ERR("invalid dev fd(%d)\n", devFd);
        assert(devFd >= 0);
    }

    mCamIonAllocFreeLogEn = property_get_int32("vendor.camera.ion.alloc.free.log.enable", 0);
    processName = getprogname();
    if (processName)
    {
        mProcessName.assign(processName);
    }
    mClientName.assign(ION_CLIENT_NAME);
    thdInst = pthread_self();
    if (!pthread_getname_np(thdInst, thdName, sizeof(thdName)))
    {
        mClientName.append(thdName);
        mClientName.append(":");
    }
    mClientName.append(std::to_string(gettid()));

    /* allocate ion fd */
    mIonFdMutex.lock();
    if (mIonFdRefCnt <= 0)
    {
        mIonFd = mt_ion_open(mClientName.c_str());
        if (mIonFd < 0)
        {
            LOG_ERR("create ion fd fail\n");
            mIonFdMutex.unlock();
            abort();
        }
    }
    mIonFdRefCnt++;
    mIonFdMutex.unlock();

    mMemList.clear();
    mDevFd = devFd;

    LOG_DEBUG("ion allocator init done(%d)\n", mIonFdRefCnt);
}

ionAllocator::~ionAllocator()
{
    mIonFdMutex.lock();
    if (mIonFdRefCnt == 0)
    {
        LOG_ERR("ion mFd cnt confused\n");
    }
    else
    {
        mIonFdRefCnt--;
        if (mIonFdRefCnt == 0)
        {
            LOG_DEBUG("close ion fd\n");
            ion_close(mIonFd);
        }
    }

    mIonFdMutex.unlock();
    LOG_DEBUG("ion allocator destroy done(%d)\n", mIonFdRefCnt);
}

void ionAllocator::setBufDbgName(apusysIonMem *mem)
{
    struct ion_mm_data data;
    const char dbgName[] = "APU_Buffer";
    int ret = 0;

    memset(&data, 0, sizeof(data));
    data.mm_cmd = ION_MM_SET_DEBUG_INFO;
    data.buf_debug_info_param.handle = mem->ion_uhandle;
    strncpy(data.buf_debug_info_param.dbg_name, dbgName, strlen(dbgName));
    data.buf_debug_info_param.dbg_name[strlen(dbgName)] = '\0';

    /* log enable, camerahalserver match, size > 10MB */
    if (mProcessName.find("camerahalserver") != std::string::npos) {
        data.buf_debug_info_param.value1 = 67;
        data.buf_debug_info_param.value2 = 97;
        data.buf_debug_info_param.value3 = 109;
        data.buf_debug_info_param.value4 = 0;
    }

    ret = ion_custom_ioctl(mIonFd, ION_CMD_MULTIMEDIA, &data);
    if (ret == (-ION_ERROR_CONFIG_LOCKED))
    {
        LOG_WARN("ion_custom_ioctl Double config after map phy address, ionDev/hanelde(%d/0x%lx)\n", mIonFd, (unsigned long)mem->ion_uhandle);
    }
    else if (ret)
    {
        LOG_ERR("ion_custom_ioctl MULTIMEDIA failed, ionDev/hanelde(%d/0x%lx)\n", mIonFd, (unsigned long)mem->ion_uhandle);
    }

    return;
}

static long getTimeNs()
{
    struct timespec getTime;

    clock_gettime(CLOCK_MONOTONIC, &getTime);

    return getTime.tv_sec * 1000000000 + getTime.tv_nsec;
}

IApusysMem * ionAllocator::alloc(unsigned int size, unsigned int align, unsigned int cache)
{
    struct apusys_mem ctlmem;
    apusysIonMem * ionMem = nullptr;
    ion_user_handle_t bufHandle = 0;
    int shareFd = 0, ret = 0;
    long time0 = 0, time1 = 0;
    unsigned long long uva;
    std::unique_lock<std::mutex> mutexLock(mMemListMtx);

    if((align > PAGE_ALIGN) || ( PAGE_ALIGN % align !=0 ))
    {
        LOG_ERR("fail to get ion buffer align must be %d Not %d\n", PAGE_ALIGN, align);
        return nullptr;
    }

    time0 = getTimeNs();
    if(cache > 0)
    {
        /* alloc ion memory cached */
        if(ion_alloc_mm(mIonFd, size, PAGE_ALIGN, 3, &bufHandle))
        {
            LOG_ERR("fail to get ion buffer, (devFd=%d, buf_handle = %d, len=%d)\n", mIonFd, bufHandle, size);
            return nullptr;
        }
    } else
    {
        /* alloc ion memory non-cached*/
        if(ion_alloc_mm(mIonFd, size, PAGE_ALIGN, 0, &bufHandle))
        {
            LOG_ERR("fail to get ion buffer, (devFd=%d, buf_handle = %d, len=%d)\n", mIonFd, bufHandle, size);
            return nullptr;
        }
    }
    time1 = getTimeNs();
    if (mCamIonAllocFreeLogEn && size > MTK_CAM_ION_LOG_THREAD)
    {
        ALOGI("[libapusys] ion alloc size = %u, format=BLOB, caller=%s, costTime = %lu ns\n", size, mClientName.c_str(), time1 - time0);
    }

    /* set ion memory shared */
    if(ion_share(mIonFd, bufHandle, &shareFd))
    {
        LOG_ERR("fail to get ion buffer handle (devFd=%d, shared_fd = %d, len=%d)\n", mIonFd, shareFd, size);
        goto shareFdFail;
    }

    /* map user va */
    uva = (unsigned long long)ion_mmap(mIonFd, NULL, (size_t)size, PROT_READ|PROT_WRITE, MAP_SHARED, shareFd, 0);
    if ((void *)uva == MAP_FAILED)
    {
        LOG_ERR("get uva failed.\n");
        goto mmapFail;
    }


    memset(&ctlmem, 0, sizeof(struct apusys_mem));
    /* get iova from kernel */
    ctlmem.uva = uva;
    ctlmem.iova = 0;
    ctlmem.size = size;
    ctlmem.iova_size = 0;
    ctlmem.mem_type = APUSYS_MEM_DRAM_ION;
    ctlmem.align = align;
    ctlmem.fd = shareFd;

    //LOG_DEBUG("ctlmem(%d/%d/0x%x/0x%llx/0x%llx/%d)\n",mDevFd, mIonFd, ctlmem.iova, ctlmem.uva, ctlmem.kva, ctlmem.size);

    ret = ioctl(mDevFd, APUSYS_IOCTL_MEM_MAP, &ctlmem);
    if(ret)
    {
        LOG_ERR("ioctl mem ctl fail(%d)\n", ret);
        goto out;
    }
    //LOG_DEBUG("ctlmem(%d/%d/0x%x/0x%llx/0x%llx/%d)\n",mDevFd, mIonFd, ctlmem.iova, ctlmem.uva, ctlmem.kva, ctlmem.size);


    /* allocate apusysMem */
    ionMem = new apusysIonMem;
    if (ionMem == nullptr)
    {
        LOG_ERR("allocate apusys ion mem struct fail\n");
        goto out;
    }

    ionMem->khandle = ctlmem.khandle;
    ionMem->va = uva;
    ionMem->iova = ctlmem.iova;
    ionMem->offset = 0;
    ionMem->size = size;
    ionMem->iova_size = ctlmem.iova_size;
    ionMem->ion_share_fd = shareFd;
    ionMem->ion_uhandle = bufHandle;
    ionMem->type = APUSYS_USER_MEM_DRAM;

    setBufDbgName(ionMem);

    this->print(ionMem);

    mMemList.push_back(ionMem);
    //LOG_DEBUG("mMemList size (%lu)\n", mMemList.size());

    return (IApusysMem *)ionMem;
out:
    if(ion_munmap(mIonFd, (void*)uva, size))
    {
        LOG_ERR("ion unmap fail\n");
    }
mmapFail:
    if(ion_share_close(mIonFd, shareFd))
    {
        LOG_ERR("ion close share fail\n");
    }
shareFdFail:
    if(ion_free(mIonFd, bufHandle))
    {
        LOG_ERR("ion free fail, fd = %d, len = %d\n", mIonFd, size);
    }
    return nullptr;
}

bool ionAllocator::free(IApusysMem * mem)
{
    apusysIonMem * ionMem = (apusysIonMem *)mem;
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
    ctlmem.mem_type = APUSYS_MEM_DRAM_ION;
    ctlmem.fd = ionMem->ion_share_fd;

    ret = ioctl(mDevFd, APUSYS_IOCTL_MEM_UNMAP, &ctlmem);
    if(ret)
    {
        LOG_ERR("ioctl mem free fail(%d)\n", ret);
    }


    /* free */
    if(ion_munmap(mIonFd, (void*)ionMem->va, ionMem->size))
    {
        LOG_ERR("ion unmap fail\n");
        return false;
    }
    if(ion_share_close(mIonFd, ionMem->ion_share_fd))
    {
        LOG_ERR("ion close share fail\n");
        return false;
    }
    if(ion_free(mIonFd, ionMem->ion_uhandle))
    {
        LOG_ERR("ion free fail, fd = %d, len = %d\n", mIonFd, ionMem->size);
        return false;
    }

    this->print(ionMem);

    if (mCamIonAllocFreeLogEn && ionMem->size > MTK_CAM_ION_LOG_THREAD)
    {
        ALOGI("[libapusys] ion free size = %u, format=, caller=%s\n", ionMem->size, mClientName.c_str());
    }

    //LOG_DEBUG("mMemList size (%lu)\n", mMemList.size());
    delete ionMem;

    return true;
}

bool ionAllocator::flush(IApusysMem * mem)
{

    apusysIonMem * ionMem = (apusysIonMem *)mem;
    struct ion_sys_data sys_data;

    sys_data.sys_cmd = ION_SYS_CACHE_SYNC;
    sys_data.cache_sync_param.handle = ionMem->ion_uhandle;
    sys_data.cache_sync_param.sync_type = ION_CACHE_FLUSH_BY_RANGE;
    sys_data.cache_sync_param.va = (void*) ionMem->va;
    sys_data.cache_sync_param.size = ionMem->size;

    if (ion_custom_ioctl(mIonFd, ION_CMD_SYSTEM, &sys_data)) {
        LOG_ERR("ion flush fail\n");
        return false;
    }

    return true;
}

bool ionAllocator::invalidate(IApusysMem * mem)
{
    apusysIonMem * ionMem = (apusysIonMem *)mem;
    struct ion_sys_data sys_data;


    sys_data.sys_cmd = ION_SYS_CACHE_SYNC;
    sys_data.cache_sync_param.handle = ionMem->ion_uhandle;
    sys_data.cache_sync_param.sync_type = ION_CACHE_INVALID_BY_RANGE;
    sys_data.cache_sync_param.va = (void*) ionMem->va;
    sys_data.cache_sync_param.size = ionMem->size;

    if (ion_custom_ioctl(mIonFd, ION_CMD_SYSTEM, &sys_data)) {
        LOG_ERR("ion invalidate fail\n");
        return false;
    }

    return true;
}

IApusysMem * ionAllocator::import(int shareFd, unsigned int size)
{
    struct apusys_mem ctlmem;
    apusysIonMem * ionMem = nullptr;
    ion_user_handle_t bufHandle;
    int ret = 0;
    unsigned long long uva;

    std::unique_lock<std::mutex> mutexLock(mMemListMtx);

    /* set ion memory shared */
    if(ion_import(mIonFd, shareFd, &bufHandle))
    {
        LOG_ERR("fail to get ion buffer by import (devFd=%d, shared_fd = %d)\n", mIonFd, shareFd);
        goto importFdFail;
    }

    /* map user va */
    uva = (unsigned long long)ion_mmap(mIonFd, NULL, (size_t)size, PROT_READ|PROT_WRITE, MAP_SHARED, shareFd, 0);
    if ((void *)uva == MAP_FAILED)
    {
        LOG_ERR("get uva failed.\n");
        goto mmapFail;
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
        goto out;
    }
    //LOG_DEBUG("ctlmem(%d/%d/0x%x/0x%x/0x%llx/0x%llx/%d)\n", mDevFd, mIonFd, ctlmem.iova, ctlmem.iova_size, ctlmem.uva, ctlmem.kva, ctlmem.size);


    /* allocate apusysMem */
    ionMem = new apusysIonMem;
    if (ionMem == nullptr)
    {
        LOG_ERR("allocate apusys ion mem struct fail\n");
        goto out;
    }

    ionMem->khandle = ctlmem.khandle;
    ionMem->va = uva;
    ionMem->iova = ctlmem.iova;
    ionMem->iova_size = ctlmem.iova_size;
    ionMem->offset = 0;
    ionMem->size = size;
    ionMem->ion_share_fd = shareFd;
    ionMem->ion_uhandle = bufHandle;

    this->print(ionMem);

    mMemList.push_back(ionMem);
    //LOG_DEBUG("mMemList size (%lu)\n", mMemList.size());

    return (IApusysMem *)ionMem;
out:
    if(ion_munmap(mIonFd, (void*)uva, size))
    {
        LOG_ERR("ion unmap fail\n");
    }
mmapFail:
    if(ion_free(mIonFd, bufHandle))
    {
        LOG_ERR("ion free fail, fd = %d, len = %d\n", mIonFd, size);
    }

importFdFail:
    return nullptr;
}

bool ionAllocator::unimport(IApusysMem * mem)
{
    apusysIonMem * ionMem = (apusysIonMem *)mem;
    struct apusys_mem ctlmem;
    int ret = 0;
    bool isFind = false;
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
    }

    /* free */
    if(ion_munmap(mIonFd, (void*)ionMem->va, ionMem->size))
    {
        LOG_ERR("ion unmap fail\n");
        return false;
    }
    if(ion_free(mIonFd, ionMem->ion_uhandle))
    {
        LOG_ERR("ion free fail, fd = %d, len = %d\n", mIonFd, ionMem->size);
        return false;
    }

    this->print(ionMem);
    //LOG_DEBUG("mMemList size (%lu)\n", mMemList.size());

    delete ionMem;

    return true;
}

void ionAllocator::print(IApusysMem * mem)
{
    apusysIonMem * ionMem = (apusysIonMem *)mem;

    MLOG_DEBUG("ionMem(%d/0x%llx/0x%x/0x%x/%d/0x%x/0x%llx/%d/%p)\n",
        ionMem->ion_share_fd, ionMem->va, ionMem->iova,
        ionMem->iova + ionMem->iova_size - 1,
        ionMem->size, ionMem->iova_size,
        ionMem->khandle, ionMem->ion_uhandle, (void *)ionMem);
}

