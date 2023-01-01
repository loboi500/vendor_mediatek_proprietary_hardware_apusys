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
#ifdef __ANDROID__
#include <cutils/properties.h>
#endif

#include "apusysCmn.h"
#include "apusysExecutor.h"
#include "apusysSession.h"

int gLogLevel = 0;

static void getLogLevel()
{
#ifdef __ANDROID__
    char prop[100];

    memset(prop, 0, sizeof(prop));
    property_get(PROPERTY_DEBUG_APUSYS_LOGLEVEL, prop, "0");
    gLogLevel = atoi(prop);
#else
    char *prop;

    prop = std::getenv(PROPERTY_DEBUG_APUSYS_LOGLEVEL);
    if (prop != nullptr)
        gLogLevel = atoi(prop);
#endif

    LOG_DEBUG("debug loglevel = %d\n", gLogLevel);
    return;
}

apusysSession::apusysSession(int devFd)
{
    char thdName[16];
    pthread_t thdInst;

    /* clear private member */
    mDevFd = devFd;
    mMemMap.clear();
    mVersion = apusysExecutor::getVersion(devFd);

    /* get log level */
    getLogLevel();

    /* get thread name */
    memset(thdName, 0, sizeof(thdName));
    thdInst = pthread_self();
    if (!pthread_getname_np(thdInst, thdName, sizeof(thdName)))
        mThdName.assign(thdName);

    /* new executor for kernel interface */
    if (mVersion == 0)
        mExecutor = new apusysExecutor(this);
    else if (mVersion == 1)
        mExecutor = new apusysExecutor_v1(this);
    else if (mVersion == 2)
        mExecutor = new apusysExecutor_v2(this);
    else {
        LOG_ERR("unknown version(%d)\n", mVersion);
        abort();
    }

    /* get meta data size */
    mMetaDataSize = mExecutor->getMetaDataSize();

    LOG_INFO("Seesion(%p): thd(%s) version(%d) log(%d)\n",
        static_cast<void *>(this), mThdName.c_str(), mVersion, gLogLevel);
}

apusysSession::~apusysSession()
{
    class apusysCmd *cmd = nullptr;

    LOG_DEBUG("Session(%p)...\n", static_cast<void *>(this));

    /* delete all cmds */
    while(!mCmdList.empty()) {
        cmd = mCmdList.back();
        LOG_DEBUG("%d cmd(%p) size(%u)\n", __LINE__, (void *)cmd, (uint32_t)mCmdList.size());
        mCmdList.pop_back();
        this->deleteCmd(cmd);
    }

    /* free residual cmdbuf */
    while (!mCmdBufMap.empty()) {
        auto itCmdBuf = mCmdBufMap.cbegin();
        if (this->cmdBufFree(itCmdBuf->second->mem->vaddr)) {
            LOG_ERR("free residual cmdbuf(%p) fail\n", itCmdBuf->second->mem->vaddr);
            break;
        }
    }

    /* free residual mem */
    while (!mMemMap.empty()) {
        auto itMem = mMemMap.cbegin();
        if (this->memFree(itMem->second->vaddr)) {
            LOG_ERR("free residual mem(%p) fail\n", itMem->second->vaddr);
            break;
        }
    }

    /* delete executor */
    if (mVersion == 0)
        delete static_cast<apusysExecutor *>(mExecutor);
    else if (mVersion == 1)
        delete static_cast<apusysExecutor_v1 *>(mExecutor);
    else if (mVersion == 2)
        delete static_cast<apusysExecutor_v2 *>(mExecutor);

    close(mDevFd);
    LOG_DEBUG("Session(%p) done\n", static_cast<void *>(this));
}

int apusysSession::getDevFd()
{
    return mDevFd;
}

uint32_t apusysSession::queryDeviceNum(enum apusys_device_type type)
{
    return mExecutor->getDeviceNum(type);
}

uint64_t apusysSession::queryInfo(enum apusys_session_info type)
{
    uint64_t ret = 0;

    switch (type) {
    case APUSYS_SESSION_INFO_VERSION:
        ret = static_cast<uint64_t>(mVersion);
        break;
    case APUSYS_SESSION_INFO_METADATA_SIZE:
        ret = static_cast<uint64_t>(mMetaDataSize);
        break;
    default:
        break;
    };

    return ret;
}

int apusysSession::queryDeviceMetaData(enum apusys_device_type type, void *meta)
{
    return mExecutor->getMetaData(type, meta);
}

int apusysSession::setDevicePower(enum apusys_device_type type, unsigned int idx, unsigned int boost)
{
    return mExecutor->setDevicePower(type, idx, boost);
}

int apusysSession::sendUserCmd(enum apusys_device_type type, void *cmdbuf)
{
    return mExecutor->sendUserCmd(type, cmdbuf);
}

void *apusysSession::cmdBufAlloc(uint32_t size, uint32_t align)
{
    struct apusysCmdBuf *cb = nullptr;

    apusysTraceBegin(__func__);
    /* check size */
    if (size == 0) {
        LOG_ERR("invalid size(%u)\n", size);
        goto out;
    }

    /* allocate memory from dram */
    cb = new apusysCmdBuf;
    if (cb == nullptr)
        goto out;

    memset(cb, 0, sizeof(*cb));
    cb->mem = mExecutor->memAlloc(size, align, APUSYS_MEM_TYPE_DRAM, F_APUSYS_MEM_HIGHADDR|F_APUSYS_MEM_CACHEABLE);
    if (cb->mem == nullptr)
        goto freeMem;;

    LOG_DEBUG("Session(%p): alloc cmdbuf(%p/%u/%d)\n",
        static_cast<void *>(this), cb->mem->vaddr, cb->mem->size, cb->mem->handle);

    /* insert cmd buffer to map */
    mMemMapMtx.lock();
    mCmdBufMap.insert({cb->mem->vaddr, cb});
    mMemMapMtx.unlock();

    goto out;

freeMem:
    delete cb;
    cb = nullptr;
out:
    apusysTraceEnd();
    if (cb)
        return cb->mem->vaddr;
    return nullptr;
}

int apusysSession::cmdBufFree(void *vaddr)
{
    std::unordered_map<void *, struct apusysCmdBuf *>::const_iterator got;
    int ret = 0;

    apusysTraceBegin(__func__);
    std::unique_lock<std::mutex> mutexLock(mMemMapMtx);
    /* find vaddr from map */
    got = mCmdBufMap.find(vaddr);
    if (got == mCmdBufMap.end()) {
        LOG_ERR("no cb(%p)\n", vaddr);
        ret = -ENOMEM;
        goto out;
    }

    LOG_DEBUG("Session(%p): free cmdbuf(%p/%u/%d)\n",
        static_cast<void *>(this),
        got->second->mem->vaddr, got->second->mem->size, got->second->mem->handle);

    /* delete memory by executor */
    ret = mExecutor->memFree(got->second->mem);

    delete got->second;
    /* erase from map if not vlm total info */
    mCmdBufMap.erase(vaddr);

out:
    apusysTraceEnd();
    return ret;
}

struct apusysCmdBuf *apusysSession::cmdBufGetObj(void *vaddr)
{
    std::unordered_map<void *, struct apusysCmdBuf *>::const_iterator got;

    std::unique_lock<std::mutex> mutexLock(mMemMapMtx);
    /* find vaddr from map */
    got = mCmdBufMap.find(vaddr);
    if (got == mCmdBufMap.end())
        return nullptr;

    return got->second;
}

void *apusysSession::memAlloc(uint32_t size, uint32_t align, enum apusys_mem_type type, uint64_t flags)
{
    struct apusysMem *mem = nullptr;
    int ret = 0;

    apusysTraceBegin(__func__);

    /* allocate memory from dram */
    mem = mExecutor->memAlloc(size, align, type, flags);
    if (mem == nullptr)
        goto out;

    /* map device va */
    ret = mExecutor->memMapDeviceVa(mem);
    if (ret) {
        LOG_ERR("map device va fail(%u/%u/%d/0x%llx)\n", size, align, type, static_cast<unsigned long long>(flags));
        goto free_mem;
    }

    LOG_DEBUG("Session(%p): alloc mem(%p/%u/%d) dva(0x%llx) flags(0x%llx)\n",
        static_cast<void *>(this),
        mem->vaddr, mem->size, mem->handle,
        static_cast<unsigned long long>(mem->deviceVa),
        static_cast<unsigned long long>(flags));

    /* insert cmd buffer to map */
    mMemMapMtx.lock();
    mMemMap.insert({mem->vaddr, mem});
    mMemMapMtx.unlock();

    goto out;

free_mem:
    mExecutor->memFree(mem);
    mem = nullptr;
out:
    apusysTraceEnd();

    if (mem)
        return mem->vaddr;
    return nullptr;
}

int apusysSession::memFree(void *vaddr)
{
    std::unordered_map<void *, struct apusysMem *>::const_iterator got;
    int ret = 0;

    apusysTraceBegin(__func__);

    std::unique_lock<std::mutex> mutexLock(mMemMapMtx);

    /* find vaddr from map */
    got = mMemMap.find(vaddr);
    if (got == mMemMap.end()) {
        LOG_ERR("no mem(%p)\n", vaddr);
        ret = -ENOMEM;
        goto out;
    }

    LOG_DEBUG("Session(%p): free mem(%p/%u/%d) dva(0x%llx)\n",
        static_cast<void *>(this),
        got->second->vaddr, got->second->size, got->second->handle,
        static_cast<unsigned long long>(got->second->deviceVa));

    /* unmap memory by executor */
    ret = mExecutor->memUnMapDeviceVa(got->second);
    if (ret)
        LOG_ERR("mem(%p) unmap device va fail\n", vaddr);
    /* delete memory by executor */
    ret = mExecutor->memFree(got->second);
    if (ret)
        LOG_ERR("mem(%p) free fail\n", vaddr);

    /* erase from map if not vlm total info */
    mMemMap.erase(vaddr);

out:
    apusysTraceEnd();
    return ret;
}

struct apusysMem *apusysSession::memGetObj(void *vaddr)
{
    std::unordered_map<void *, struct apusysMem *>::const_iterator got;

    std::unique_lock<std::mutex> mutexLock(mMemMapMtx);
    /* find vaddr from map */
    got = mMemMap.find(vaddr);
    if (got == mMemMap.end())
        return nullptr;

    return got->second;
}

void *apusysSession::memImport(int handle, unsigned int size)
{
    struct apusysMem *mem = nullptr;

    apusysTraceBegin(__func__);

    mem = mExecutor->memImport(handle, size);
    if (mem == nullptr) {
        LOG_ERR("import memory(%d/%u) fail\n", handle, size);
        goto out;
    }

    /* insert cmd buffer to map */
    mMemMapMtx.lock();
    mMemMap.insert({mem->vaddr, mem});
    mMemMapMtx.unlock();

    LOG_DEBUG("Session(%p): import mem(%p/%u/%d) dva(0x%llx)\n",
        static_cast<void *>(this),
        mem->vaddr, mem->size, mem->handle,
        static_cast<unsigned long long>(mem->deviceVa));

out:
    apusysTraceEnd();
    if (mem)
        return mem->vaddr;
    return nullptr;
}

int apusysSession::memUnImport(void *vaddr)
{
    std::unordered_map<void *, struct apusysMem *>::const_iterator got;
    int ret = -EINVAL;

    apusysTraceBegin(__func__);
    std::unique_lock<std::mutex> mutexLock(mMemMapMtx);
    /* find vaddr from map */
    got = mMemMap.find(vaddr);
    if (got == mMemMap.end())
        goto out;

    LOG_DEBUG("Session(%p): unimport mem(%p/%u/%d) dva(0x%llx)\n",
        static_cast<void *>(this),
        got->second->vaddr, got->second->size, got->second->handle,
        static_cast<unsigned long long>(got->second->deviceVa));

    ret = mExecutor->memUnImport(got->second);
    if (!ret)
        mMemMap.erase(vaddr);

out:
    apusysTraceEnd();
    return ret;
}

int apusysSession::memFlush(void *vaddr)
{
    std::unordered_map<void *, struct apusysMem *>::const_iterator got;
    int ret = -EINVAL;

    apusysTraceBegin(__func__);
    std::unique_lock<std::mutex> mutexLock(mMemMapMtx);
    /* find vaddr from map */
    got = mMemMap.find(vaddr);
    if (got == mMemMap.end())
        goto out;

    LOG_DEBUG("Session(%p): flush mem(%p/%u/%d) dva(0x%llx)\n",
        static_cast<void *>(this),
        got->second->vaddr, got->second->size, got->second->handle,
        static_cast<unsigned long long>(got->second->deviceVa));

    ret = mExecutor->memFlush(got->second);
out:
    apusysTraceEnd();
    return ret;
}

int apusysSession::memInvalidate(void *vaddr)
{
    std::unordered_map<void *, struct apusysMem *>::const_iterator got;
    int ret = -EINVAL;

    apusysTraceBegin(__func__);
    std::unique_lock<std::mutex> mutexLock(mMemMapMtx);
    /* find vaddr from map */
    got = mMemMap.find(vaddr);
    if (got == mMemMap.end())
        goto out;

    LOG_DEBUG("Session(%p): invalidate mem(%p/%u/%d) dva(0x%llx)\n",
        static_cast<void *>(this),
        got->second->vaddr, got->second->size, got->second->handle,
        static_cast<unsigned long long>(got->second->deviceVa));

    ret = mExecutor->memInvalidate(got->second);
out:
    apusysTraceEnd();
    return ret;
}

uint64_t apusysSession::memGetInfoFromHostPtr(void *vaddr, enum apusys_mem_info type)
{
    struct apusysMem *mem = nullptr;
    std::unordered_map<void *, struct apusysMem *>::const_iterator mGot;
    std::unordered_map<void *, struct apusysCmdBuf *>::const_iterator cGot;
    uint64_t retVal = 0;

    std::unique_lock<std::mutex> mutexLock(mMemMapMtx);
    mGot = mMemMap.find(vaddr);
    if (mGot == mMemMap.end()) {
        cGot = mCmdBufMap.find(vaddr);
        if (cGot == mCmdBufMap.end()) {
            LOG_ERR("no mem(%p)\n", vaddr);
            goto out;
        } else {
            mem = cGot->second->mem;
        }
    } else {
        mem = mGot->second;
    }

    switch (type) {
        case APUSYS_MEM_INFO_GET_DEVICE_VA:
            retVal = mem->deviceVa;
            break;

        case APUSYS_MEM_INFO_GET_SIZE:
            retVal = mem->size;
            break;

        case APUSYS_MEM_INFO_GET_HANDLE:
            retVal = static_cast<uint64_t>(mem->handle);
            break;

        default:
            LOG_DEBUG("not support op(%d)\n", type);
            retVal = 0;
            break;
    }

    LOG_DEBUG("Session(%p): get mem(%p/%u/%d) info(%d/0x%llx)\n",
        static_cast<void *>(this), mem->vaddr, mem->size, mem->handle,
        type, static_cast<unsigned long long>(retVal));
out:
    return retVal;
}

int apusysSession::memSetParamViaHostPtr(void *vaddr, enum apusys_mem_info type, uint64_t val)
{
    struct apusysMem *mem = nullptr;
    UNUSED(type);
    UNUSED(val);

    mem = this->memGetObj(vaddr);
    if (mem == nullptr)
        return -EINVAL;

    LOG_ERR("Session(%p): don't support mem set param(%d/%llu)\n",
        static_cast<void *>(this), type, static_cast<unsigned long long>(val));

    return -EINVAL;
}

class apusysCmd *apusysSession::createCmd()
{
    class apusysCmd *cmd = nullptr;

    cmd = mExecutor->createCmd();
    if (cmd == nullptr) {
        LOG_ERR("new apusys cmd fail\n");
        return nullptr;
    }

    std::unique_lock<std::mutex> mutexLock(mCmdListMtx);
    mCmdList.push_back(cmd);

    LOG_DEBUG("Session(%p) create cmd(%p)\n",
        static_cast<void *>(this), static_cast<void *>(cmd));
    return cmd;
}

int apusysSession::deleteCmd(class apusysCmd *cmd)
{
    if (cmd == nullptr)
        return -EINVAL;

    std::unique_lock<std::mutex> mutexLock(mCmdListMtx);
    auto it = std::find(mCmdList.begin(), mCmdList.end(), cmd);
    if (it == mCmdList.end())
        return -EINVAL;

    LOG_DEBUG("Session(%p) delete cmd(%p)\n",
        static_cast<void *>(this), static_cast<void *>(cmd));
    mExecutor->deleteCmd(cmd);
    mCmdList.erase(it);

    return 0;
}
