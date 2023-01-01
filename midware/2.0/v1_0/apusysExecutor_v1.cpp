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
#include <numeric>

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

#include <ion/ion.h> //system/core/libion/include/ion.h
#include <linux/ion_drv.h>
#include <ion.h> //libion_mtk/include/ion.h
#include <mt_iommu_port.h>
#include <sys/mman.h>

#define APUSYS_IOMMU_PORT M4U_PORT_L21_APU_FAKE_DATA
#define MTK_CAM_ION_LOG_THREAD (10*1024*1024) //10MB
#define ION_CLIENT_NAME "apusysMemAllocator_V10:"
#define MDW_DEFAULT_ALIGN (16)
#define MDW_APU_VLM_HANDLE (0xA15CAFE)
#define MDW_ALIGN(x, align) ((x+align-1) & (~(align-1)))

struct apusysMem_v1 {
    struct apusysMem mem;
    int kHandle;
    int uHandle;
    int shareFd;
    unsigned int deviceVaSize;
};

class apusysSubCmd_v1 : public apusysSubCmd {
private:
    std::vector<void *>mSuBCmdBufPtr;
    void *mHdrAddr;

public:
    apusysSubCmd_v1(class apusysCmd *parent, enum apusys_device_type type, uint32_t idx);
    virtual ~apusysSubCmd_v1();

    int construct(struct apusysMem *mem, unsigned int hdrOfs, unsigned int cmdBufOfs, unsigned int packId);
    int updateCmdBuf(bool beforeExec);
    int updateHeader(void *addr);

    uint64_t getRunInfo(enum apusys_subcmd_runinfo_type type);
    int getDeviceType();
};

class apusysCmd_v1 : public apusysCmd {
private:
    void *mCmdMem;
    std::vector<void *>mSuBCmdHdrPtr;
    unsigned long long mCmdId;

    std::vector<std::vector<apusysSubCmd_v1 *>>mModifiedSubCmdList;
    std::vector<std::vector<uint8_t>>mModifiedAdjMatrix;
    std::vector<uint32_t> mModifiedPackIdList;

    unsigned int calcCmdMemSize();
    int construct();
    int updateSubCmdBuf(bool beforeExec);
    int updateHeader();
    int preExec();
    int enableFence();
    int setFence(bool enable);
    int getFence(struct apusys_cmd_hdr_fence *fence);
    void printFinalCmdBuf();
    int updateModifiedInfo();

public:
    apusysCmd_v1(class apusysSession *session);
    virtual ~apusysCmd_v1();

    uint64_t getRunInfo(enum apusys_cmd_runinfo_type type);
    class apusysSubCmd *createSubCmd(enum apusys_device_type type);
    int build();
    int run();
    int runAsync();
    int wait();
    int runFence(int fence, uint64_t flags);
};

int apusysExecutor_v1::mIonFd = -1;
int apusysExecutor_v1::mIonFdRefCnt = 0;
std::mutex apusysExecutor_v1::mIonFdMtx;

//----------------------------------------------
// static functions
static long getTimeNs()
{
    struct timespec getTime;

    clock_gettime(CLOCK_MONOTONIC, &getTime);

    return getTime.tv_sec * 1000000000 + getTime.tv_nsec;
}

//----------------------------------------------
// apusysSubCmd_v1 implementation
apusysSubCmd_v1::apusysSubCmd_v1(class apusysCmd *parent, enum apusys_device_type type, uint32_t idx):
    apusysSubCmd(parent, type, idx)
{
    mSuBCmdBufPtr.clear();
    mHdrAddr = nullptr;
}

apusysSubCmd_v1::~apusysSubCmd_v1()
{
}

int apusysSubCmd_v1::updateHeader(void *addr)
{
    struct apusys_sc_hdr *sh = nullptr;

    if (addr == nullptr)
        return -EINVAL;

    this->mHdrAddr = addr;
    sh = static_cast<struct apusys_sc_hdr *>(addr);
    sh->dev_type = mDevType;
    sh->driver_time = 0;
    sh->ip_time = 0;
    sh->suggest_time = mSuggestMs;
    sh->bandwidth = 0;
    sh->tcm_usage = mVlmUsage;
    sh->tcm_force = mVlmForce;
    sh->boost_val = mBoostVal;
    sh->mem_ctx = mVlmCtx;

    return 0;
}

int apusysSubCmd_v1::updateCmdBuf(bool beforeExec)
{
    unsigned int i = 0;
    struct apusysCmdBuf *cb = nullptr;

    if (mCmdBufList.size() != mSuBCmdBufPtr.size()) {
        LOG_ERR("cmdbuf size not matched(%u/%u)\n",
            static_cast<unsigned int>(mCmdBufList.size()),
            static_cast<unsigned int>(mSuBCmdBufPtr.size()));
        return -EINVAL;
    }

    for (i = 0; i < mCmdBufList.size(); i++) {
        cb = mCmdBufList.at(i);

        if (beforeExec == true) {
            if (cb->dir == APUSYS_CB_IN || cb->dir == APUSYS_CB_BIDIRECTIONAL) {
                LOG_DEBUG("subcmd(%u) cmdbuf(%u) duplicated(%u) %s execution\n",
                    mIdx, i, cb->dir, beforeExec == true ? "before" : "after");
                memcpy(mSuBCmdBufPtr.at(i), cb->mem->vaddr, cb->mem->size);
            }
        } else {
            if (cb->dir == APUSYS_CB_OUT || cb->dir == APUSYS_CB_BIDIRECTIONAL) {
                LOG_DEBUG("subcmd(%u) cmdbuf(%u) duplicated(%u) %s execution\n",
                    mIdx, i, cb->dir, beforeExec == true ? "before" : "after");
                memcpy(cb->mem->vaddr, mSuBCmdBufPtr.at(i), cb->mem->size);
            }
        }
    }

    return 0;
}

int apusysSubCmd_v1::getDeviceType()
{
    return mDevType;
}

uint64_t apusysSubCmd_v1::getRunInfo(enum apusys_subcmd_runinfo_type type)
{
    struct apusys_sc_hdr *sh = static_cast<struct apusys_sc_hdr *>(this->mHdrAddr);
    uint64_t ret = 0;

    if (sh == nullptr) {
        LOG_ERR("no subcmd header\n");
        return 0;
    }

    switch (type) {
    case SC_RUNINFO_IPTIME:
        ret = sh->ip_time;
        break;

    case SC_RUNINFO_DRIVERTIME:
        ret = sh->driver_time;
        break;

    case SC_RUNINFO_TIMESTAMP_START:
    case SC_RUNINFO_TIMESTAMP_END:
        LOG_DEBUG("v1 no support timestamp\n");
        break;

    case SC_RUNINFO_BANDWIDTH:
        ret = sh->bandwidth;
        break;

    case SC_RUNINFO_BOOST:
        ret = sh->boost_val;
        break;

    case SC_RUNINFO_VLMUSAGE:
        LOG_DEBUG("v1 no tcm usage\n");
        break;

    default:
        LOG_ERR("invalid runInfo type(%d)\n", type);
        break;
    }

    return ret;
}

int apusysSubCmd_v1::construct(struct apusysMem *mem, unsigned int hdrOfs, unsigned int cmdBufOfs, unsigned int packId)
{
    struct apusys_sc_hdr *sh = nullptr;
    struct apusysCmdBuf *cb = nullptr;
    struct apusys_mdla_codebuf_info *mdlaInfo = nullptr;
    struct apusys_edma_codebuf_info *edmaInfo = nullptr;
    struct apusys_mdla_pmu_info *mdlaPmuInfo = nullptr;
    void *tmpCmdBufAddr = nullptr, *hdrAddr = nullptr;
    unsigned int i = 0, curCmdBufOfs = 0, isSMP = 0;

    if (mem == nullptr)
        return -EINVAL;

    /* legacy cmd specific */
    if ((mDevType == APUSYS_DEVICE_MDLA && mCmdBufList.size() < 2) ||
        (mDevType == APUSYS_DEVICE_EDMA && mCmdBufList.size() != 2) ||
        (mDevType == APUSYS_DEVICE_SAMPLE && mCmdBufList.size() != 1) ||
        (mDevType == APUSYS_DEVICE_VPU && mCmdBufList.size() != 1)) {
        LOG_ERR("v1 dev(%u) don't support num_cmdbufs(%u)\n",
            static_cast<unsigned int>(mDevType), static_cast<unsigned int>(mCmdBufList.size()));
        return -EINVAL;
    }

    /* header */
    hdrAddr = reinterpret_cast<void *>(reinterpret_cast<unsigned long long>(mem->vaddr) + hdrOfs);
    this->mHdrAddr = hdrAddr;
    sh = static_cast<struct apusys_sc_hdr *>(hdrAddr);
    if (sh->dev_type == APUSYS_DEVICE_MDLA && mDevType == APUSYS_DEVICE_MDLA) {
        sh->pack_id = 0;
        isSMP = 1;
        CLOG_DEBUG("bypass header construct(%u) dev(%u) and clear pack id\n", mIdx, mDevType);
    } else {
        this->updateHeader(sh);
        sh->pack_id = packId;
    }

    CLOG_DEBUG("construct subcmd(%d) dev(%u) input offset(%u/%u) num_cmdbufs(%u)\n",
        mIdx, mDevType, hdrOfs, cmdBufOfs, static_cast<unsigned int>(mCmdBufList.size()));

    /* clear cmdbuf ptr */
    mSuBCmdBufPtr.clear();
    curCmdBufOfs = cmdBufOfs;
    for (i = 0; i < mCmdBufList.size(); i++) {
        cb = mCmdBufList.at(i);
        /* offset for cmdbuf alignment */
        if (cb->mem->align)
            curCmdBufOfs = MDW_ALIGN(curCmdBufOfs, cb->mem->align);
        else
            curCmdBufOfs = MDW_ALIGN(curCmdBufOfs, MDW_DEFAULT_ALIGN);

        CLOG_DEBUG("subcmd(%d) dev(%u) cmdbuf(%u) offset(%u) align(%u)\n", mIdx, mDevType, i, curCmdBufOfs, cb->mem->align);

        /* fill codebuf information */
        if (i == 0) {
            sh->cb_info_size = cb->mem->size;
            if (isSMP) {
                sh->ofs_cb_dual1_info = curCmdBufOfs;
                sh->ofs_cb_dual0_info = sh->ofs_cb_info;
                sh->ofs_cb_info = 0;
            } else {
                sh->ofs_cb_info = curCmdBufOfs;
            }
        } else if (i == 1) {
            if (mDevType == APUSYS_DEVICE_MDLA) {
                /* update mdla codebuf info addr */
                mdlaInfo = reinterpret_cast<struct apusys_mdla_codebuf_info *>(mCmdBufList.at(0)->mem->vaddr);
                CLOG_DEBUG("mdla :\n");
                CLOG_DEBUG(" offset = 0x%x\n", mdlaInfo->offset);
                CLOG_DEBUG(" reserved = 0x%x\n", mdlaInfo->reserved);
                CLOG_DEBUG(" size = %u\n", mdlaInfo->size);
                CLOG_DEBUG(" addr = 0x%x -> 0x%x\n", mdlaInfo->addr, static_cast<unsigned int>(mem->deviceVa + curCmdBufOfs));
                CLOG_DEBUG(" code_offset = 0x%x\n", mdlaInfo->code_offset);
                CLOG_DEBUG(" code_count = %u\n", mdlaInfo->code_count);
                CLOG_DEBUG(" code_out_cmd_id = %u\\n", mdlaInfo->code_out_cmd_id);
                mdlaInfo->addr = mem->deviceVa + curCmdBufOfs;
            } else if (mDevType == APUSYS_DEVICE_EDMA) {
                /* update edma codebuf info addr */
                edmaInfo = reinterpret_cast<struct apusys_edma_codebuf_info *>(mCmdBufList.at(0)->mem->vaddr);
                CLOG_DEBUG("edma :\n");
                CLOG_DEBUG(" handle = 0x%llx\n", edmaInfo->handle);
                CLOG_DEBUG(" cmd_count = %u\n", edmaInfo->cmd_count);
                CLOG_DEBUG(" addr = 0x%x -> 0x%x\n", edmaInfo->addr, static_cast<unsigned int>(mem->deviceVa + curCmdBufOfs));
                CLOG_DEBUG(" fill_value = %u\n", edmaInfo->fill_value);
                CLOG_DEBUG(" desp_iommu_en = %u\n", edmaInfo->desp_iommu_en);
                edmaInfo->addr = mem->deviceVa + curCmdBufOfs;
            } else {
                LOG_ERR("v1 dev(%u) don't support num(%u) cmdbuf\n",
                    static_cast<unsigned int>(mDevType), static_cast<unsigned int>(mCmdBufList.size()));
                return -EINVAL;
            }
        } else if (i == 2) {
            if (mDevType == APUSYS_DEVICE_MDLA) {
                /* pmu info */
                sh->ofs_pmu_info = curCmdBufOfs;
                CLOG_DEBUG("mdla pmu ofs: 0x%x\n", sh->ofs_pmu_info);
            } else {
                LOG_ERR("dev(%u) don't support cmdbufs(%u)\n", static_cast<unsigned int>(mDevType), i);
                return -EINVAL;
            }
        } else if (i == 3) {
            if (mDevType == APUSYS_DEVICE_MDLA) {
                mdlaPmuInfo = reinterpret_cast<struct apusys_mdla_pmu_info *>(mCmdBufList.at(2)->mem->vaddr);
                mdlaPmuInfo->ofs_result0 = curCmdBufOfs;
                CLOG_DEBUG("mdla pmu result0 ofs: 0x%x\n", mdlaPmuInfo->ofs_result0);
            } else {
                LOG_ERR("dev(%u) don't support cmdbufs(%u)\n", static_cast<unsigned int>(mDevType), i);
                return -EINVAL;
            }
        } else if (i == 4) {
            if (mDevType == APUSYS_DEVICE_MDLA) {
                mdlaPmuInfo = reinterpret_cast<struct apusys_mdla_pmu_info *>(mCmdBufList.at(2)->mem->vaddr);
                mdlaPmuInfo->ofs_result1 = curCmdBufOfs;
                CLOG_DEBUG("mdla pmu result1 ofs: 0x%x\n", mdlaPmuInfo->ofs_result1);
            } else {
                LOG_ERR("dev(%u) don't support cmdbufs(%u)\n", static_cast<unsigned int>(mDevType), i);
                return -EINVAL;
            }
        } else {
            LOG_ERR("v1 don't support cmdbuf(%u)\n", i);
            return -EINVAL;
        }

        tmpCmdBufAddr = reinterpret_cast<void *>(reinterpret_cast<unsigned long long>(mem->vaddr) + curCmdBufOfs);
        LOG_DEBUG("subcmd(%u)cmdbuf(%u) ofs(0x%x)\n", mIdx, i, curCmdBufOfs);
        memcpy(tmpCmdBufAddr, cb->mem->vaddr, cb->mem->size);
        mSuBCmdBufPtr.push_back(tmpCmdBufAddr);
        /* update next cmdbuf addr */
        curCmdBufOfs += cb->mem->size;
    }

    return curCmdBufOfs - cmdBufOfs;
}

//----------------------------------------------
// apusysCmd_v1 implementation
apusysCmd_v1::apusysCmd_v1(class apusysSession *session): apusysCmd(session)
{
    mCmdMem = nullptr;
    mCmdId = 0;
    mSuBCmdHdrPtr.clear();
    mModifiedSubCmdList.clear();
    mModifiedPackIdList.clear();
}

apusysCmd_v1::~apusysCmd_v1()
{
    class apusysSubCmd_v1 *subcmd = nullptr;

    mMtx.lock();
    while (!mSubCmdList.empty()) {
        subcmd = static_cast<class apusysSubCmd_v1 *>(mSubCmdList.back());
        delete subcmd;
        mSubCmdList.pop_back();
    }

    if (mCmdMem != nullptr) {
        this->getSession()->memFree(mCmdMem);
        mCmdMem = nullptr;
    }
    mMtx.unlock();
}

uint64_t apusysCmd_v1::getRunInfo(enum apusys_cmd_runinfo_type type)
{
    struct apusys_cmd_hdr_fence fenceInfo;
    struct apusys_cmd_hdr *h = static_cast<struct apusys_cmd_hdr *>(mCmdMem);
    int ret = 0;

    if (this->getFence(&fenceInfo)) {
        LOG_WARN("Cmd(%p): no fence\n", static_cast<void *>(this));
        goto out;
    }

    if (h == nullptr) {
        LOG_WARN("Cmd(%p): no cmd mem\n", static_cast<void *>(this));
        goto out;
    }

    switch (type) {
    case CMD_RUNINFO_STATUS:
        ret = (h->flag_bitmap & (CMD_FLAG_BITMASK_STATUS << CMD_FLAG_BITMAP_OFS_STATUS)) >> CMD_FLAG_BITMAP_OFS_STATUS;
        break;

    case CMD_RUNINFO_TOTALTIME_US:
        ret = fenceInfo.total_time;
        break;

    default:
        LOG_DEBUG("not support(%d)\n", type);
        break;
    }

    LOG_DEBUG("Cmd(%p): get cmd run infomaiton(%d/%llu)\n",
        static_cast<void *>(this), type, static_cast<unsigned long long>(ret));

out:
    return ret;
}

class apusysSubCmd *apusysCmd_v1::createSubCmd(enum apusys_device_type type)
{
    class apusysSubCmd *subcmd = nullptr;
    uint32_t idx = 0, i = 0;

    /* check device type exist */
    if (!this->getSession()->queryDeviceNum(type))
        return nullptr;

    std::unique_lock<std::mutex> mutexLock(mMtx);
    idx = mSubCmdList.size();
    subcmd = new apusysSubCmd_v1(this, type, idx);
    if (subcmd == nullptr) {
        LOG_ERR("new subcmd fail\n");
        goto out;
    }

    /* add to subcmd list */
    mSubCmdList.push_back(subcmd);

    /* resize adjust list */
    mAdjMatrix.resize(mSubCmdList.size());
    for (i = 0; i < mAdjMatrix.size(); i++)
        mAdjMatrix.at(i).resize(mSubCmdList.size());

    /* resize packid list and push 0 */
    mPackIdList.push_back(0);
    this->setDirty(APUSYS_CMD_DIRTY_SUBCMD);

    LOG_DEBUG("Cmd v1(%p): create #%u-subcmd(%d/%p)\n",
       static_cast<void *>(this), idx, type, static_cast<void *>(subcmd));
out:
    return subcmd;
}

int apusysCmd_v1::updateSubCmdBuf(bool beforeExec)
{
    class apusysSubCmd_v1 *subcmd = nullptr;
    unsigned int i = 0;
    int ret = 0;

    apusysTraceBegin(__func__);
    for (i = 0; i < mSubCmdList.size(); i++) {
        subcmd = static_cast<class apusysSubCmd_v1 *>(mSubCmdList.at(i));
        ret = subcmd->updateCmdBuf(beforeExec);
        if (ret)
            break;
    }
    apusysTraceEnd();

    return ret;
}

int apusysCmd_v1::setFence(bool enable)
{
    struct apusys_cmd_hdr *hdr = nullptr;
    struct apusys_cmd_hdr_fence *fh = nullptr;
    struct apusysMem *mem = nullptr;
    int numSubCmd = mModifiedSubCmdList.size();
    unsigned int ofs;

    if (mCmdMem == nullptr || numSubCmd <= 0) {
        LOG_ERR("no cmdmem\n");
        return -EINVAL;
    }

    mem = mSession->memGetObj(mCmdMem);
    if (mem == nullptr) {
        LOG_ERR("can't fine mem\n");
        return -ENOMEM;
    }

    ofs = sizeof(struct apusys_cmd_hdr) + ((numSubCmd - 1) * SIZE_SUBGRAPH_SCOFS_ELEMENT);
    /* check size */
    if (ofs + sizeof(struct apusys_cmd_hdr_fence) > mem->size) {
        LOG_ERR("size invalid (%u/%u/%u)\n", ofs, static_cast<unsigned int>(sizeof(struct apusys_cmd_hdr_fence)), mem->size);
        return -EINVAL;
    }
    hdr = reinterpret_cast<struct apusys_cmd_hdr *>(reinterpret_cast<unsigned long long>(mCmdMem));
    fh = reinterpret_cast<struct apusys_cmd_hdr_fence *>(reinterpret_cast<unsigned long long>(mCmdMem) + ofs);

    if (enable == true)
        hdr->flag_bitmap |= (1ULL << CMD_FLAG_BITMAP_OFS_FENCE);
    else
        hdr->flag_bitmap &= ~(1ULL << CMD_FLAG_BITMAP_OFS_FENCE);
    memset(fh, 0, sizeof(*fh));

    return 0;
}

int apusysCmd_v1::getFence(struct apusys_cmd_hdr_fence *fence)
{
    struct apusys_cmd_hdr_fence *fh = nullptr;
    struct apusysMem *mem = nullptr;
    int numSubCmd = mModifiedSubCmdList.size();;
    unsigned int ofs;

    if (mCmdMem == nullptr || numSubCmd <= 0)
        return -EINVAL;

    mem = mSession->memGetObj(mCmdMem);
    if (mem == nullptr)
        return -ENOMEM;

    ofs = sizeof(struct apusys_cmd_hdr) + ((numSubCmd - 1) * SIZE_SUBGRAPH_SCOFS_ELEMENT);
    /* check size */
    if (ofs + sizeof(struct apusys_cmd_hdr_fence) > mem->size)
        return -EINVAL;

    fh = reinterpret_cast<struct apusys_cmd_hdr_fence *>(reinterpret_cast<unsigned long long>(mCmdMem) + ofs);
    memcpy(fence, fh, sizeof(*fh));

    return 0;
}

unsigned int apusysCmd_v1::calcCmdMemSize()
{
    unsigned int numSubCmd = 0, cmdSize = 0, i = 0, j = 0;
    unsigned int dListSize = 0, pListSize = 0, scHdrSize = 0, scOfsListSize = 0, aHdrSize = 0, totalHdrSize = 0;
    struct apusysCmdBuf *tmpCmdBuf = nullptr;

    numSubCmd = mModifiedSubCmdList.size();
    if (numSubCmd == 0)
        return 0;

    /* size of apusys header */
    aHdrSize = sizeof(struct apusys_cmd_hdr) + sizeof(struct apusys_cmd_hdr_fence);

    /* size of subcmd header offest list */
    scOfsListSize = numSubCmd * SIZE_SUBGRAPH_SCOFS_ELEMENT;

    /* size of sucessor list */
    dListSize = mModifiedAdjMatrix.size() * (mModifiedAdjMatrix.size() + 1) * SIZE_CMD_SUCCESSOR_ELEMENT;

    /* size of predecessor cnt list */
    pListSize = mModifiedAdjMatrix.size() * SIZE_CMD_SUCCESSOR_ELEMENT;

    /* size of subcmd headers */
    scHdrSize = numSubCmd * sizeof(struct apusys_sc_hdr);

    totalHdrSize = aHdrSize + scOfsListSize + dListSize + pListSize + scHdrSize;
    cmdSize = totalHdrSize;

    /* size of subcmds, calc with alignment */
    for (i = 0; i < mSubCmdList.size(); i++) {
        for (j = 0; j < mSubCmdList.at(i)->getCmdBufNum(); j++) {
            tmpCmdBuf = mSubCmdList.at(i)->getCmdBuf(j);
            if (tmpCmdBuf == nullptr) {
                LOG_ERR("get cmdbuf(%u/%u) fail\n", i, j);
                return 0;
            }
            if (tmpCmdBuf->mem->align)
                cmdSize = MDW_ALIGN(cmdSize, tmpCmdBuf->mem->align) + tmpCmdBuf->mem->size;
            else
                cmdSize = MDW_ALIGN(cmdSize, MDW_DEFAULT_ALIGN) + tmpCmdBuf->mem->size;

            LOG_DEBUG("subcmd(%u-%u) cmdbuf size(%u) align(%u) cmdSize(%u)\n",
                i, j, tmpCmdBuf->mem->size, tmpCmdBuf->mem->align, cmdSize);
        }
    }

    /* total cmd size */
    LOG_DEBUG("cmd size = %u(%u/%u*%u/%u/%u/%u=%u*%u/%u)\n",
        cmdSize, aHdrSize, numSubCmd,
        static_cast<unsigned int>(SIZE_SUBGRAPH_SCOFS_ELEMENT),
        dListSize, pListSize, scHdrSize, numSubCmd,
        static_cast<unsigned int>(sizeof(struct apusys_sc_hdr)),
        cmdSize - totalHdrSize);

    return cmdSize;
}

int apusysCmd_v1::updateHeader()
{
    struct apusys_cmd_hdr *h = static_cast<struct apusys_cmd_hdr *>(mCmdMem);
    class apusysSubCmd_v1 *subcmd = nullptr;
    unsigned int i = 0;
    int ret = 0;

    if (h == nullptr || mModifiedSubCmdList.size() != mSuBCmdHdrPtr.size())
        return -EINVAL;

    h->magic = APUSYS_MAGIC_NUMBER;
    h->uid = reinterpret_cast<uint64_t>(this);
    h->version = APUSYS_CMD_VERSION;
    h->priority = this->mPriority;
    h->hard_limit = this->mHardlimit;
    h->soft_limit = this->mSoftlimit;
    h->usr_pid = this->mUsrid;
    h->num_sc = mModifiedSubCmdList.size();

    for (i = 0; i < mModifiedSubCmdList.size(); i++) {
        subcmd = static_cast<class apusysSubCmd_v1 *>(mModifiedSubCmdList.at(i).at(0));
        ret = subcmd->updateHeader(mSuBCmdHdrPtr.at(i));
        if (ret) {
            LOG_ERR("update subcmd(%u) header fail(%d)\n", i, ret);
            break;
        }
    }

    if (!ret)
        mDirty.set(APUSYS_CMD_DIRTY_PARAMETER, 0);

    return ret;
}

int apusysCmd_v1::updateModifiedInfo()
{
    uint32_t i = 0, j = 0, idx = 0;
    class apusysSubCmd_v1 *sc = nullptr;
    std::vector<class apusysSubCmd_v1 *> tmpList;
    std::vector<uint32_t>tmpPackedList;

    tmpPackedList.clear();
    mModifiedAdjMatrix = mAdjMatrix;

    /* update subcmd list */
    mModifiedSubCmdList.clear();
    /* update modified subcmd list for legacy construction(smp) */
    for (i = 0; i < mSubCmdList.size(); i++) {
        sc = static_cast<class apusysSubCmd_v1 *>(mSubCmdList.at(i));
        if (mPackIdList.at(i) && sc->getDeviceType() == APUSYS_DEVICE_MDLA) {
            /* pack id exist, find same packed subcmd in modified subcmd list */
            for (j = 0; j < mModifiedSubCmdList.size(); j++) {

                if (mPackIdList.at(mModifiedSubCmdList.at(j).at(0)->getIdx()) == mPackIdList.at(i)) {
                    CLOG_DEBUG("update sc(%u/%u) to modified list(%u), pack id(%u)\n",
                        sc->getIdx(), i, j, mPackIdList.at(i));
                    mModifiedSubCmdList.at(j).push_back(sc);
                    tmpPackedList.push_back(i);

                    sc = nullptr;
                    break;
                }
            }
        }

        if (sc == nullptr)
            continue;

        tmpList.clear();
        tmpList.push_back(sc);
        mModifiedPackIdList.push_back(mPackIdList.at(i));
        mModifiedSubCmdList.push_back(tmpList);
    }

    /* clear packed smp subcmd's dependency */
    while (!tmpPackedList.empty()) {
        idx = tmpPackedList.back();
        tmpPackedList.pop_back();
        /* delete row */
        mModifiedAdjMatrix.erase(mModifiedAdjMatrix.begin()+idx);
        /* delete colume */
        for (i = 0; i < mModifiedAdjMatrix.size(); i++)
            mModifiedAdjMatrix.at(i).erase(mModifiedAdjMatrix.at(i).begin()+idx);
    }

    CLOG_DEBUG("update modified list(%u->%u) matrix size(%u*%u)\n",
        static_cast<unsigned int>(mSubCmdList.size()),
        static_cast<unsigned int>(mModifiedSubCmdList.size()),
        static_cast<unsigned int>(mModifiedAdjMatrix.size()),
        static_cast<unsigned int>(mModifiedAdjMatrix.at(0).size()));

    return 0;
}


int apusysCmd_v1::construct()
{
    struct apusys_cmd_hdr *h = static_cast<struct apusys_cmd_hdr *>(mCmdMem);
    struct apusysMem *mem = nullptr;
    uint32_t *tmpU32List = nullptr;
    class apusysSubCmd_v1 *subcmd = nullptr;
    unsigned int i = 0, ofs = 0, j = 0, subcmdBufOfs = 0, tmpIdx = 0;

    if (h == nullptr)
        return -EINVAL;

    if (mModifiedPackIdList.size() != mModifiedSubCmdList.size())
        return -EINVAL;

    mem = mSession->memGetObj(mCmdMem);
    if (mem == nullptr)
        return -ENOMEM;

    apusysTraceBegin(__func__);

    mSuBCmdHdrPtr.clear();

    /* setup header for parameter */
    h->magic = APUSYS_MAGIC_NUMBER;
    h->uid = reinterpret_cast<uint64_t>(this);
    h->version = APUSYS_CMD_VERSION;
    h->priority = this->mPriority;
    h->hard_limit = this->mHardlimit;
    h->soft_limit = this->mSoftlimit;
    h->usr_pid = this->mUsrid;
    if (mPowerSave)
        h->flag_bitmap |= (1ULL << CMD_FLAG_BITMAP_OFS_POWERSAVE);
    h->num_sc = mModifiedSubCmdList.size();

    /* init offset */
    ofs = sizeof(struct apusys_cmd_hdr) - SIZE_SUBGRAPH_SCOFS_ELEMENT + (h->num_sc * SIZE_SUBGRAPH_SCOFS_ELEMENT);
    ofs += sizeof(struct apusys_cmd_hdr_fence);

    /* 1. construct: successor offset list */
    /* 1.a setup succssor list offset */
    h->ofs_scr_list = ofs;
    tmpU32List = reinterpret_cast<unsigned int *>(reinterpret_cast<unsigned long long>(h) + ofs);
    tmpIdx = 0;
    /* 1.b fill successor list */
    for (i = 0; i < h->num_sc; i++) {
        tmpU32List[tmpIdx] = std::accumulate(mModifiedAdjMatrix.at(i).begin(), mModifiedAdjMatrix.at(i).end(), 0);
        tmpIdx++;

        for (j = 0; j < mModifiedAdjMatrix.at(i).size(); j++) {
            if (!mModifiedAdjMatrix.at(i).at(j))
                continue;
            tmpU32List[tmpIdx] = j;
            tmpIdx++;
        }
    }
    ofs += tmpIdx * SIZE_SUBGRAPH_SCOFS_ELEMENT;

    /* 2. construct: predecessor list */
    /* 2.a setup predecessor list offset */
    h->ofs_pdr_cnt_list = ofs;
    tmpU32List = reinterpret_cast<unsigned int *>(reinterpret_cast<unsigned long long>(h) + ofs);
    tmpIdx = 0;
    /* 2.b fill predecessor cnt list */
    for (i = 0; i < mModifiedAdjMatrix.size(); i++) {
        for (j = 0; j < mModifiedAdjMatrix.size(); j++) {
            if (mModifiedAdjMatrix.at(j).at(i)) {
                if (i == j)
                    continue;
                tmpU32List[tmpIdx]++;
            }
        }
        CLOG_DEBUG("subcm(%u)predecessor cnt(%u)\n", tmpIdx , tmpU32List[tmpIdx]);
        tmpIdx++;
    }

    ofs += tmpIdx * SIZE_SUBGRAPH_SCOFS_ELEMENT;

    /* calculate subcmd cmdbuf offset */
    subcmdBufOfs = ofs + (mModifiedSubCmdList.size() * sizeof(struct apusys_sc_hdr));
    tmpU32List = reinterpret_cast<uint32_t *>(reinterpret_cast<unsigned long long>(h) + sizeof(*h) - SIZE_SUBGRAPH_SCOFS_ELEMENT);

    /* 3. update subcmd header */
    for (i = 0; i < mModifiedSubCmdList.size(); i++) {
        tmpU32List[i] = ofs;
        CLOG_DEBUG("  subcmd(%u/%p+%u) ofs=%u\n", i, this->mCmdMem, tmpU32List[i], ofs);
        mSuBCmdHdrPtr.push_back(reinterpret_cast<void *>(reinterpret_cast<unsigned long long>(mCmdMem) + ofs));
        for (j = 0; j < mModifiedSubCmdList.at(i).size(); j++) {
            subcmd = mModifiedSubCmdList.at(i).at(j);
            subcmdBufOfs += subcmd->construct(mem, ofs, subcmdBufOfs, mModifiedPackIdList.at(i));
            if (j)
                h->flag_bitmap |= (2ULL << CMD_FLAG_BITMAP_OFS_FORCEDUAL);
        }
        ofs += sizeof(struct apusys_sc_hdr);
    }

    apusysTraceEnd();
    return 0;
}

int apusysCmd_v1::build()
{
    unsigned int cmdSize = 0;
    int ret = 0;

    this->printInfo(0);
    std::unique_lock<std::mutex> mutexLock(mMtx);

    apusysTraceBegin(__func__);

    updateModifiedInfo();

    /* check cmd memory exist */
    if (mCmdMem != nullptr) {
        /* cmdbuf modified, should free and rebuild */
        LOG_DEBUG("cmd memory already exist, free and rebuild.\n");
        if (mSession->memFree(mCmdMem)) {
            LOG_ERR("free cmd memory to rebuild fail\n");
            ret = -EINVAL;
            goto out;
        }
        mCmdMem = nullptr;
    }

    /* get cmd size */
    cmdSize = this->calcCmdMemSize();
    if (cmdSize == 0) {
        LOG_ERR("no subcmd in this cmd\n");
        ret = -EINVAL;
        goto out;
    }

    /* allocate memory to build cmd */
    mCmdMem = this->getSession()->memAlloc(cmdSize, 0, APUSYS_MEM_TYPE_DRAM, 0);
    if (mCmdMem == nullptr) {
        LOG_ERR("alloc memory to build cmd fail\n");
        ret = -ENOMEM;
        goto out;
    }
    LOG_DEBUG("alloc cmd mem(%p/%u)\n", mCmdMem, cmdSize);

    /* construct header */
    if (this->construct()) {
        LOG_ERR("construct cmd header fail\n");
        ret = -EINVAL;
        goto FailHeaderConstruct;
    }

    /* flush for hw execution */
    mSession->memFlush(mCmdMem);

    this->printFinalCmdBuf();
    mDirty.reset();

    goto out;

FailHeaderConstruct:
    this->getSession()->memFree(mCmdMem);
out:
    apusysTraceEnd();
    return ret;
}

void apusysCmd_v1::printFinalCmdBuf()
{
    struct apusys_cmd_hdr *h = nullptr;
    struct apusys_sc_hdr *sh = nullptr;
    uint32_t *tmpU32List = nullptr;
    uint32_t i = 0, j = 0, tmpCnt = 0, tmpIdx = 0;

    if (this->mCmdMem == nullptr) {
        LOG_ERR("no cmd memory\n");
        return;
    }

    CLOG_DEBUG("final cmdbuf content:\n");
    h = static_cast<struct apusys_cmd_hdr *>(this->mCmdMem);
    CLOG_DEBUG("-------------------------------------------\n");
    CLOG_DEBUG(" apusys header:\n");
    CLOG_DEBUG("  magic: 0x%llx\n", h->magic);
    CLOG_DEBUG("  uid: 0x%llx\n", h->uid);
    CLOG_DEBUG("  version: %u\n", h->version);
    CLOG_DEBUG("  priority: %u\n", h->priority);
    CLOG_DEBUG("  hard_limit: %u\n", h->hard_limit);
    CLOG_DEBUG("  soft_limit: %u\n", h->soft_limit);
    CLOG_DEBUG("  usr_pid: %u\n", h->usr_pid);
    CLOG_DEBUG("  flag_bitmap: 0x%llx\n", h->flag_bitmap);
    CLOG_DEBUG("  num_sc: %u\n", h->num_sc);
    CLOG_DEBUG("  ofs_scr_list: %u\n", h->ofs_scr_list);
    CLOG_DEBUG("  ofs_pdr_cnt_list: %u\n", h->ofs_pdr_cnt_list);
    CLOG_DEBUG("  scofs_list_entry: %u\n", h->scofs_list_entry);
    CLOG_DEBUG("-------------------------------------------\n");
    CLOG_DEBUG(" successor list:\n");
    tmpU32List =  reinterpret_cast<uint32_t *>(reinterpret_cast<unsigned long long>(this->mCmdMem) + h->ofs_scr_list);
    for (i = 0; i < h->num_sc; i++) {
        tmpCnt = tmpU32List[tmpIdx];
        tmpIdx++;
        CLOG_DEBUG("  subcmd(%u) successor cnt(%u)\n", i, tmpCnt);
        for(j = 0; j < tmpCnt; j++) {
            CLOG_DEBUG("    (%u)\n", tmpU32List[tmpIdx]);
            tmpIdx++;
        }
    }
    CLOG_DEBUG("-------------------------------------------\n");
    CLOG_DEBUG(" predecessor list:\n");
    tmpU32List =  reinterpret_cast<uint32_t *>(reinterpret_cast<unsigned long long>(this->mCmdMem) + h->ofs_pdr_cnt_list);
    tmpIdx = 0;
    for (i = 0; i < h->num_sc; i++) {
        CLOG_DEBUG("  subcmd(%u) predecessor cnt(%u)\n", i, tmpU32List[tmpIdx]);
        tmpIdx++;
    }
    CLOG_DEBUG("-------------------------------------------\n");
    CLOG_DEBUG(" subcmd header list:\n");
    tmpU32List = reinterpret_cast<uint32_t *>(reinterpret_cast<unsigned long long>(this->mCmdMem) + sizeof(struct apusys_cmd_hdr) - SIZE_SUBGRAPH_SCOFS_ELEMENT);
    for (i = 0; i < h->num_sc; i++) {
        sh = reinterpret_cast<struct apusys_sc_hdr *>(reinterpret_cast<unsigned long long>(this->mCmdMem) + tmpU32List[i]);
        CLOG_DEBUG("  subcmd(%u/%p=%p+%u)\n", i, (void *)sh, this->mCmdMem, tmpU32List[i]);
        CLOG_DEBUG("    dev_type: %u\n", sh->dev_type);
        CLOG_DEBUG("    driver_time: %u\n", sh->driver_time);
        CLOG_DEBUG("    ip_time: %u\n", sh->ip_time);
        CLOG_DEBUG("    suggest_time: %u\n", sh->suggest_time);
        CLOG_DEBUG("    bandwidth: %u\n", sh->bandwidth);
        CLOG_DEBUG("    tcm_usage: %u\n", sh->tcm_usage);
        CLOG_DEBUG("    tcm_force: %u\n", sh->tcm_force);
        CLOG_DEBUG("    boost_val: %u\n", sh->boost_val);
        CLOG_DEBUG("    pack_id: %u\n", sh->pack_id);
        CLOG_DEBUG("    mem_ctx: %u\n", sh->mem_ctx);
        CLOG_DEBUG("    cb_info_size: %u\n", sh->cb_info_size);
        CLOG_DEBUG("    ofs_cb_info: %u\n", sh->ofs_cb_info);
        CLOG_DEBUG("    ofs_cb_dual0_info: %u\n", sh->ofs_cb_dual0_info);
        CLOG_DEBUG("    ofs_cb_dual1_info: %u\n", sh->ofs_cb_dual1_info);
        CLOG_DEBUG("    ofs_pmu_info: %u\n", sh->ofs_pmu_info);
    }
}

int apusysCmd_v1::preExec()
{
    int ret = 0;

    apusysTraceBegin(__func__);

    /* if cmdMem not exist or cmdbuf dirty, rebuild before run */
    if (mCmdMem == nullptr || mDirty.test(APUSYS_CMD_DIRTY_CMDBUF)) {
        LOG_DEBUG("build cmd before run(%p/%llu)\n", mCmdMem, mDirty.to_ullong());
        ret = this->build();
        if (ret) {
            LOG_ERR("build before run fail(%d)\n", ret);
            goto out;
        }
    };

    /* if parameter dirty, set parameter before run */
    if (mDirty.test(APUSYS_CMD_DIRTY_PARAMETER)) {
        LOG_DEBUG("set parameter cmd before run(%p/%llu)\n", mCmdMem, mDirty.to_ullong());
        ret = this->updateHeader();
    }

out:
    apusysTraceEnd();
    return ret;
}

int apusysCmd_v1::run()
{
    struct apusys_ioctl_cmd ioctlCmd;
    struct apusysMem *cmdMem = nullptr;
    int ret = 0;

    apusysTraceBegin(__func__);
    std::unique_lock<std::mutex> mutexLock(mExecMtx);

    if (mIsRunning == true) {
        LOG_ERR("cmd(%p) is running, can't run again\n", static_cast<void *>(this));
        ret = -EBUSY;
        goto out;
    }

    ret = this->preExec();
    if (ret)
        goto out;

    ret = setFence(false);
    if (ret)
        goto out;

    /* update cmdbuf before execution */
    this->updateSubCmdBuf(true);

    /* flush */
    this->getSession()->memFlush(mCmdMem);

    /* set cmd to execution */
    cmdMem = mSession->memGetObj(mCmdMem);
    if (cmdMem == nullptr) {
        LOG_ERR("can't find cmdbuf\n");
        ret = -ENOMEM;
        goto out;
    }
    memset(&ioctlCmd, 0, sizeof(ioctlCmd));
    ioctlCmd.mem_fd = cmdMem->handle;
    ioctlCmd.size = cmdMem->size;

    mIsRunning = true;
    mExecutePid = getpid();
    mExecuteTid = gettid();

    LOG_INFO("runCmd(0x%llx/0x%llx) sync\n",
        reinterpret_cast<unsigned long long>(this),
        reinterpret_cast<unsigned long long>(mCmdMem));
    ret = ioctl(mSession->getDevFd(), APUSYS_IOCTL_RUN_CMD_SYNC, &ioctlCmd);
    if(ret) {
        ret = -(abs(errno));
        LOG_ERR("runCmd(0x%llx/0x%llx) sync fail(%d/%s)\n",
            reinterpret_cast<unsigned long long>(this),
            reinterpret_cast<unsigned long long>(mCmdMem),
            ret, strerror(errno));
        this->printInfo(1);
    } else {
        LOG_INFO("runCmd(0x%llx/0x%llx) sync done\n",
            reinterpret_cast<unsigned long long>(this),
            reinterpret_cast<unsigned long long>(mCmdMem));
    }
    /* invalidate */
    this->getSession()->memInvalidate(mCmdMem);

    /* update cmdbuf after execution */
    this->updateSubCmdBuf(false);

    mIsRunning = false;
    mExecutePid = 0;
    mExecuteTid = 0;

out:
    apusysTraceEnd();
    return ret;
}

int apusysCmd_v1::runAsync()
{
    struct apusys_ioctl_cmd ioctlCmd;
    struct apusysMem *cmdMem = nullptr;
    int ret = 0;

    apusysTraceBegin(__func__);
    std::unique_lock<std::mutex> mutexLock(mExecMtx);
    if (mIsRunning == true) {
        LOG_ERR("cmd(%p) is running by(%d/%d), can't run again\n",
            static_cast<void *>(this), mExecutePid, mExecuteTid);
        ret = -EBUSY;
        goto out;
    }

    ret = this->preExec();
    if (ret)
        goto out;

    ret = setFence(true);
    if (ret)
        goto out;

    /* update cmdbuf before execution */
    this->updateSubCmdBuf(true);

    /* flush */
    this->getSession()->memFlush(mCmdMem);

    /* set cmd to execution */
    cmdMem = mSession->memGetObj(mCmdMem);
    if (cmdMem == nullptr) {
        LOG_ERR("can't find cmdbuf\n");
        return -ENOMEM;
    }
    memset(&ioctlCmd, 0, sizeof(ioctlCmd));
    ioctlCmd.mem_fd = cmdMem->handle;
    ioctlCmd.size = cmdMem->size;
    LOG_INFO("runCmd(0x%llx/0x%llx) async\n",
        reinterpret_cast<unsigned long long>(this),
        reinterpret_cast<unsigned long long>(mCmdMem));
    ret = ioctl(mSession->getDevFd(), APUSYS_IOCTL_RUN_CMD_ASYNC, &ioctlCmd);
    if(ret) {
        ret = -(abs(errno));
        LOG_ERR("run cmd (0x%llx/0x%llx) async fail(%d/%s)\n",
            reinterpret_cast<unsigned long long>(this),
            reinterpret_cast<unsigned long long>(mCmdMem),
            ret, strerror(errno));
    } else {
        mCmdId = ioctlCmd.cmd_id;
        mIsRunning = true;
        mExecutePid = getpid();
        mExecuteTid = gettid();
        LOG_INFO("runCmd(0x%llx/0x%llx) async done, need wait\n",
            reinterpret_cast<unsigned long long>(this),
            reinterpret_cast<unsigned long long>(mCmdMem));
    }

out:
    apusysTraceEnd();
    return ret;
}

int apusysCmd_v1::wait()
{
    struct pollfd fds;
    int ret = -EINVAL;
    struct apusys_cmd_hdr_fence fenceInfo;

    apusysTraceBegin(__func__);
    std::unique_lock<std::mutex> mutexLock(mExecMtx);
    if (mIsRunning != true) {
        LOG_ERR("cmd(%p) is not running, can't be waited\n", static_cast<void *>(this));
        ret = -EINVAL;
        goto out;
    }

    ret = this->getFence(&fenceInfo);
    if (ret)
        goto out;

    if (fenceInfo.fd <= 0) {
        ret = -EINVAL;
        goto out;
    }
    LOG_DEBUG("fence = %llu\n", fenceInfo.fd);

    /* poll fence */
    fds.fd = fenceInfo.fd;
    fds.events = POLLIN;
    fds.revents = 0;
    do {
        ret = poll(&fds, 1, -1); /* Wait forever */

        if (ret > 0) {
            if (fds.revents & (POLLERR | POLLNVAL)) {
                errno = EINVAL;
                ret = -(abs(errno));
                goto out;
            }

            goto waitSuccess;
        } else if (ret == 0) { /* NEVER */
            errno = ETIME;
            ret = -(abs(errno));
            goto out;
        }
    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));

waitSuccess:
    close(fenceInfo.fd);

    /* invalidate */
    this->getSession()->memInvalidate(mCmdMem);

    /* update cmdbuf after execution */
    this->updateSubCmdBuf(false);

    ret = 0;
    mCmdId = 0;
    mIsRunning = false;
    mExecutePid = 0;
    mExecuteTid = 0;
out:
    apusysTraceEnd();
    return ret;
}

int apusysCmd_v1::runFence(int fence, uint64_t flags)
{
    struct apusys_ioctl_cmd ioctlCmd;
    struct apusys_cmd_hdr_fence fenceInfo;
    struct apusysMem *cmdMem = nullptr;
    int ret = -EINVAL;
    UNUSED(flags);

    apusysTraceBegin(__func__);
    std::unique_lock<std::mutex> mutexLock(mExecMtx);
    if (fence > 0) {
        LOG_ERR("v1 executor don't support input fence fd(%d)\n", fence);
        ret = -EINVAL;
        goto out;
    }

    if (mIsRunning == true) {
        LOG_ERR("cmd(%p) is running, can't run again\n", static_cast<void *>(this));
        ret = -EBUSY;
        goto out;
    }

    ret = this->preExec();
    if (ret)
        goto out;

    ret = setFence(true);
    if (ret)
        goto out;

    /* update cmdbuf before execution */
    this->updateSubCmdBuf(true);

    /* flush */
    this->getSession()->memFlush(mCmdMem);

    /* set cmd to execution */
    cmdMem = mSession->memGetObj(mCmdMem);
    if (cmdMem == nullptr) {
        LOG_ERR("can't find cmdbuf\n");
        return -ENOMEM;
    }
    memset(&ioctlCmd, 0, sizeof(ioctlCmd));
    ioctlCmd.mem_fd = cmdMem->handle;
    ioctlCmd.size = cmdMem->size;
    LOG_INFO("runCmd(0x%llx) async\n", reinterpret_cast<unsigned long long>(mCmdMem));
    ret = ioctl(mSession->getDevFd(), APUSYS_IOCTL_RUN_CMD_ASYNC, &ioctlCmd);
    if(ret) {
        ret = -(abs(errno));
        LOG_ERR("run cmd (0x%llx) async fail(%s)\n", reinterpret_cast<unsigned long long>(mCmdMem), strerror(errno));
    } else {
        /* get fence fd */
        ret = this->getFence(&fenceInfo);
        if (ret)
            return ret;
        mCmdId = ioctlCmd.cmd_id;
        ret = fenceInfo.fd;
    }

out:
    apusysTraceEnd();
    return ret;
}

//------------------------------
apusysExecutor_v1::apusysExecutor_v1(class apusysSession *session) : apusysExecutor(session)
{
    char thdName[16];
    pthread_t thdInst;
    unsigned int idx = 0;
    unsigned long long devSup = 0;
    struct apusys_ioctl_hs ioctlHandShake;
    const char* progname = NULL;

    /* get camera */
    mCamIonAllocFreeLogEn = property_get_int32("vendor.camera.ion.alloc.free.log.enable", 0);
    progname = getprogname();
    if (progname != NULL)
        mProcessName.assign(progname);

    mClientName.assign(ION_CLIENT_NAME);
    thdInst = pthread_self();
    if (!pthread_getname_np(thdInst, thdName, sizeof(thdName))) {
        mClientName.append(thdName);
        mClientName.append(":");
    }
    mClientName.append(std::to_string(gettid()));

    mIonFdMtx.lock();
    if (mIonFdRefCnt <= 0) {
        /* allocate ion fd */
        mIonFd = mt_ion_open(mClientName.c_str());
        if (mIonFd < 0) {
            LOG_ERR("create ion fd fail\n");
            mIonFdMtx.unlock();
            abort();
        }
    }
    mIonFdRefCnt++;
    mIonFdMtx.unlock();

    /* handshake with kernel */
    memset(&ioctlHandShake, 0, sizeof(ioctlHandShake));
    ioctlHandShake.type = APUSYS_HANDSHAKE_BEGIN;
    if(ioctl(mSession->getDevFd(), APUSYS_IOCTL_HANDSHAKE, &ioctlHandShake))
        LOG_ERR("apusys handshake fail(%s)\n", strerror(errno));

    /* set vlm information */
    memset(&mVlmInfo, 0, sizeof(mVlmInfo));
    mVlmInfo.vaddr = reinterpret_cast<void *>(ioctlHandShake.begin.vlm_start);
    mVlmInfo.size = ioctlHandShake.begin.vlm_size;
    mVlmInfo.deviceVa = static_cast<uint32_t>(ioctlHandShake.begin.vlm_start);
    mVlmInfo.mtype = APUSYS_MEM_TYPE_VLM;
    mVlmInfo.handle = MDW_APU_VLM_HANDLE;
    mMetaDataSize = 0;

    /* get supported device num */
    devSup = ioctlHandShake.begin.dev_support;
    while (devSup != 0) {
        if (!(devSup & (1ULL << idx)))
            goto next;

        memset(&ioctlHandShake, 0, sizeof(ioctlHandShake));
        ioctlHandShake.type = APUSYS_HANDSHAKE_QUERY_DEV;
        ioctlHandShake.dev.type = idx;
        if (ioctl(mSession->getDevFd(), APUSYS_IOCTL_HANDSHAKE, &ioctlHandShake)) {
            LOG_ERR("apusys query dev(%u) num fail(%s)\n", idx, strerror(errno));
            break;
        }

        mDevNumList.at(idx) = ioctlHandShake.dev.num;
        LOG_DEBUG("dev(%u) support %u cores\n", idx, mDevNumList.at(idx) );
next:
        devSup &= ~(1ULL << idx);
        idx++;
    }

    LOG_DEBUG("init done, ionFdRefCnt(%d)\n", mIonFdRefCnt);
}

apusysExecutor_v1::~apusysExecutor_v1()
{
    mIonFdMtx.lock();
    if (mIonFdRefCnt <= 0) {
        LOG_ERR("ionFdRefCnt confuse(%d)\n", mIonFdRefCnt);
    } else {
        mIonFdRefCnt--;
        if (mIonFdRefCnt == 0)
            ion_close(mIonFd);
    }
    LOG_DEBUG("deinit done, ionFdRefCnt(%d)\n", mIonFdRefCnt);
    mIonFdMtx.unlock();
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
    struct apusys_ioctl_power ioctlPwr;
    unsigned int devNum = 0;
    int ret = 0;

    devNum = mSession->queryDeviceNum(type);
    if (devNum <= 0 || idx >= devNum)
        return -ENODEV;

    memset(&ioctlPwr, 0, sizeof(ioctlPwr));
    ioctlPwr.dev_type = type;
    ioctlPwr.idx = idx;
    ioctlPwr.boost_val = boost;
    ret = ioctl(mSession->getDevFd(), APUSYS_IOCTL_SET_POWER, &ioctlPwr);
    if (ret)
        LOG_ERR("set power(%d/%u/%u) fail(%s)\n", type, idx, boost, strerror(errno));

    return ret;
}

int apusysExecutor_v1::sendUserCmd(enum apusys_device_type type, void *cmdbuf)
{
    struct apusys_ioctl_ucmd ioctlUCmd;
    struct apusysCmdBuf *pCb = nullptr;
    void *mem = nullptr;
    unsigned int devNum = 0;
    int ret = 0;

    if (cmdbuf == nullptr)
        return -EINVAL;

    /* check target device exist */
    devNum = mSession->queryDeviceNum(type);
    if (devNum <= 0)
        return -ENODEV;

    /* get cmdbuf instance */
    pCb = mSession->cmdBufGetObj(cmdbuf);
    if (pCb == nullptr) {
        LOG_ERR("ucmd cmdbuf(%p) to dev(%d) invalid\n", cmdbuf, type);
        ret = -EINVAL;
        goto out;
    }

    /* allocate memory */
    mem = mSession->memAlloc(pCb->mem->size, pCb->mem->align, APUSYS_MEM_TYPE_DRAM, F_APUSYS_MEM_32BIT);
    if (mem == nullptr) {
        LOG_ERR("alloc mem to dev(%d) ucmd fail\n", type);
        ret = -ENOMEM;
        goto out;
    }
    memcpy(mem, cmdbuf, pCb->mem->size);

    memset(&ioctlUCmd, 0, sizeof(ioctlUCmd));
    ioctlUCmd.dev_type = type;
    ioctlUCmd.idx = 0;
    ioctlUCmd.mem_fd = mSession->memGetInfoFromHostPtr(mem, APUSYS_MEM_INFO_GET_HANDLE);
    ioctlUCmd.offset = 0;
    ioctlUCmd.size = mSession->memGetInfoFromHostPtr(mem, APUSYS_MEM_INFO_GET_SIZE);
    ret = ioctl(mSession->getDevFd(), APUSYS_IOCTL_USER_CMD, &ioctlUCmd);
    if(ret)
        LOG_ERR("send dev(%d) user cmd fail(%s)\n", type, strerror(errno));

    /* free memory */
    mSession->memFree(mem);

out:
    return ret;
}

class apusysCmd *apusysExecutor_v1::createCmd()
{
    class apusysCmd_v1 *cmd = nullptr;

    cmd = new apusysCmd_v1(mSession);
    if (cmd == nullptr) {
        LOG_ERR("create cmd fail\n");
        return nullptr;
    }

    return static_cast<class apusysCmd *>(cmd);
}

int apusysExecutor_v1::deleteCmd(class apusysCmd *cmd)
{
    class apusysCmd_v1 *vcmd = nullptr;

    if (cmd == nullptr)
        return -EINVAL;

    vcmd = static_cast<class apusysCmd_v1 *>(cmd);
    delete vcmd;

    return 0;
}

void apusysExecutor_v1::setBufDbgName(int uHandle)
{
    struct ion_mm_data data;
    const char dbgName[] = "APU_Buffer";
    int ret = 0;

    memset(&data, 0, sizeof(data));
    data.mm_cmd = ION_MM_SET_DEBUG_INFO;
    data.buf_debug_info_param.handle = uHandle;
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
        LOG_WARN("ion_custom_ioctl Double config after map phy address, ionDev/hanelde(%d/%d)\n", mIonFd, uHandle);
    else if (ret)
        LOG_ERR("ion_custom_ioctl MULTIMEDIA failed, ionDev/hanelde(%d/%d)\n", mIonFd, uHandle);
}

struct apusysMem *apusysExecutor_v1::vlmAlloc(enum apusys_mem_type type, uint32_t size, uint32_t align, uint64_t flags)
{
    struct apusysMem *vlm = nullptr;
    UNUSED(flags);

    if (!size)
        return &mVlmInfo;

    if (size > mVlmInfo.size)
        goto out;

    vlm = new apusysMem;
    if (vlm == nullptr)
        goto out;

    vlm->align = align;
    vlm->vaddr = malloc(32);
    if (vlm->vaddr == nullptr) {
        goto freeApuMem;
    }
    vlm->size = size;
    vlm->deviceVa = mVlmInfo.deviceVa;
    vlm->mtype = type;
    vlm->handle = MDW_APU_VLM_HANDLE;
    MLOG_DEBUG("mem(%u/0x%llx/%u/%d)\n",
        vlm->mtype, static_cast<unsigned long long>(vlm->deviceVa), vlm->size, vlm->handle);
    goto out;

freeApuMem:
    delete vlm;
    vlm = nullptr;
out:
    return vlm;
}

int apusysExecutor_v1::vlmFree(struct apusysMem *mem)
{
    if (mem == &mVlmInfo)
        return 0;

    MLOG_DEBUG("mem(%u/0x%llx/%u/%d)\n",
        mem->mtype, static_cast<unsigned long long>(mem->deviceVa), mem->size, mem->handle);

    free(mem->vaddr);
    delete mem;
    return 0;
}

struct apusysMem *apusysExecutor_v1::memAlloc(uint32_t size, uint32_t align, enum apusys_mem_type type, uint64_t flags)
{
    struct apusysMem_v1 *vmem = nullptr;
    ion_user_handle_t bufHandle = 0;
    long time0 = 0, time1 = 0;
    int shareFd = 0;
    void *uva = nullptr;
    unsigned int ionFlags = 0;

    /* virtual local memory only support size query */
    if (type == APUSYS_MEM_TYPE_VLM ||
        type == APUSYS_MEM_TYPE_LOCAL) {
        return this->vlmAlloc(type, size, align, flags);
    } else if (type != APUSYS_MEM_TYPE_DRAM) {
        LOG_ERR("not support mem type(%d)\n", type);
        return nullptr;
    }

    /* alloc dram allocation */
    apusysTraceBegin(__func__);

    /* allocate for apusys mem information */
    vmem = new apusysMem_v1;
    if (vmem == nullptr)
        goto out;

    memset(vmem, 0, sizeof(*vmem));
    vmem->mem.highaddr = (flags & F_APUSYS_MEM_HIGHADDR) ? true : false;
    vmem->mem.cacheable = (flags & F_APUSYS_MEM_CACHEABLE) ? true : false;
    vmem->mem.mtype = type;
    vmem->mem.align = align;
    vmem->mem.size = size;

    /* alloc ion memory */
    if (vmem->mem.cacheable)
        ionFlags = 3;

    time0 = getTimeNs();
    if(ion_alloc_mm(mIonFd, vmem->mem.size, vmem->mem.align, ionFlags, &bufHandle)) {
        LOG_ERR("alloc ion buffer fail(devFd=%d, buf_handle = %d, len=%d, cacheable=%d)\n",
            mIonFd, bufHandle, size, vmem->mem.cacheable);
        goto failAllocIon;
    }
    time1 = getTimeNs();
    if (mCamIonAllocFreeLogEn && size > MTK_CAM_ION_LOG_THREAD)
        ALOGI("[libapusys] ion alloc size = %u, format=BLOB, caller=%s, costTime = %lu ns\n", size, mClientName.c_str(), time1 - time0);

    /* set ion memory shared */
    if(ion_share(mIonFd, bufHandle, &shareFd))
    {
        LOG_ERR("fail to get ion buffer handle (devFd=%d, shared_fd = %d, len=%d)\n", mIonFd, shareFd, size);
        goto failSetIonShare;
    }

    /* map user va */
    uva = ion_mmap(mIonFd, NULL, vmem->mem.size, PROT_READ|PROT_WRITE, MAP_SHARED, shareFd, 0);
    if (uva == MAP_FAILED)
    {
        LOG_ERR("get uva failed.\n");
        goto failMmapIon;
    }

    /* assign uva/device va */
    vmem->mem.vaddr = uva;
    vmem->uHandle = bufHandle;
    vmem->mem.handle = shareFd;
    vmem->shareFd = shareFd;

    this->setBufDbgName(bufHandle);
    MLOG_DEBUG("mem alloc(%p/%u/%d) dva(0x%llx) flag(0x%llx)\n",
        vmem->mem.vaddr, vmem->mem.size, vmem->mem.handle,
        static_cast<unsigned long long>(vmem->mem.deviceVa),
        static_cast<unsigned long long>(flags));

    /* insert to map */
    mMtx.lock();
    mMemMap.insert({&vmem->mem, vmem});
    mMtx.unlock();

    goto out;

failMmapIon:
    ion_share_close(mIonFd, shareFd);
failSetIonShare:
    if (ion_free(mIonFd, bufHandle))
        LOG_WARN("free mem(%d) fail\n", mIonFd);
failAllocIon:
    delete vmem;
    vmem = nullptr;
out:
    apusysTraceEnd();
    if (vmem)
        return &vmem->mem;
    return nullptr;
}

int apusysExecutor_v1::memFree(struct apusysMem *mem)
{
    struct apusysMem_v1 *vmem = nullptr;
    std::unordered_map<struct apusysMem *, void *>::const_iterator got;
    int ret = 0;

    if (mem == nullptr)
        return -EINVAL;

    if (mem->mtype == APUSYS_MEM_TYPE_VLM ||
        mem->mtype == APUSYS_MEM_TYPE_LOCAL) {
        return this->vlmFree(mem);
    } else if (mem->mtype != APUSYS_MEM_TYPE_DRAM) {
        LOG_ERR("not support mem type(%d)\n", mem->mtype);
        return -EINVAL;
    }

    MLOG_DEBUG("mem free(%p/%u/%d) dva(0x%llx)\n",
        mem->vaddr, mem->size, mem->handle,
        static_cast<unsigned long long>(mem->deviceVa));

    std::unique_lock<std::mutex> mutexLock(mMtx);
    apusysTraceBegin(__func__);
    /* find vaddr from map */
    got = mMemMap.find(mem);
    if (got == mMemMap.end()) {
        LOG_ERR("no apusysMem(%p)\n", static_cast<void *>(mem));
        ret = -EINVAL;
        goto out;
    }
    vmem = static_cast<apusysMem_v1 *>(got->second);

    if (ion_munmap(mIonFd, vmem->mem.vaddr, vmem->mem.size)) {
        LOG_ERR("ion unmap fail\n");
        ret = -EINVAL;
        goto out;
    }

    if(ion_share_close(mIonFd, vmem->shareFd)) {
        LOG_ERR("ion unmap fail\n");
        ret = -EINVAL;
        goto out;
    }

    if(ion_free(mIonFd, vmem->uHandle)) {
        LOG_ERR("ion unmap fail\n");
        ret = -EINVAL;
        goto out;
    }

    if (mCamIonAllocFreeLogEn && vmem->mem.size > MTK_CAM_ION_LOG_THREAD)
        ALOGI("[libapusys] ion free size = %u, format=, caller=%s\n", vmem->mem.size, mClientName.c_str());

    mMemMap.erase(mem);
    delete vmem;

out:
    apusysTraceEnd();
    return ret;
}

int apusysExecutor_v1::memMapDeviceVa(struct apusysMem *mem)
{
    struct apusys_mem ioctlMem;
    struct apusysMem_v1 *vmem = nullptr;
    std::unordered_map<struct apusysMem *, void *>::const_iterator got;
    int ret = 0;

    if (mem == nullptr)
        return -EINVAL;

    if (mem->mtype == APUSYS_MEM_TYPE_VLM ||
        mem->mtype == APUSYS_MEM_TYPE_LOCAL)
        return 0;

    std::unique_lock<std::mutex> mutexLock(mMtx);
    apusysTraceBegin(__func__);
    /* find vaddr from map */
    got = mMemMap.find(mem);
    if (got == mMemMap.end()) {
        LOG_ERR("no apusysMem(%p)\n", static_cast<void *>(mem));
        ret = -EINVAL;
        goto out;
    }
    vmem = static_cast<apusysMem_v1 *>(got->second);

    memset(&ioctlMem, 0, sizeof(ioctlMem));
    /* get iova from kernel */
    ioctlMem.uva = reinterpret_cast<unsigned long long>(vmem->mem.vaddr);
    ioctlMem.iova = 0;
    ioctlMem.size = vmem->mem.size;
    ioctlMem.iova_size = 0;
    ioctlMem.mem_type = APUSYS_MEM_DRAM_ION;
    ioctlMem.align = vmem->mem.align;
    ioctlMem.fd = mem->handle;
    if (ioctl(mSession->getDevFd(), APUSYS_IOCTL_MEM_MAP, &ioctlMem)) {
        LOG_ERR("ioctl map device va fail(%s)\n", strerror(errno));
        ret = -abs(errno);
        goto out;
    }

    /* assign device va */
    vmem->mem.deviceVa = ioctlMem.iova;
    vmem->kHandle = ioctlMem.khandle;
    MLOG_DEBUG("mem map(%p/%u/%d) dva(0x%llx)\n",
        mem->vaddr, mem->size, mem->handle,
        static_cast<unsigned long long>(mem->deviceVa));

out:
    apusysTraceEnd();
    return 0;
}

int apusysExecutor_v1::memUnMapDeviceVa(struct apusysMem *mem)
{
    struct apusys_mem ioctlMem;
    struct apusysMem_v1 *vmem = nullptr;
    std::unordered_map<struct apusysMem *, void *>::const_iterator got;
    int ret = 0;

    if (mem == nullptr)
        return -EINVAL;

    if (mem->mtype == APUSYS_MEM_TYPE_VLM ||
        mem->mtype == APUSYS_MEM_TYPE_LOCAL)
        return 0;

    MLOG_DEBUG("mem unmap(%p/%u/%d) dva(0x%llx)\n",
        mem->vaddr, mem->size, mem->handle,
        static_cast<unsigned long long>(mem->deviceVa));

    std::unique_lock<std::mutex> mutexLock(mMtx);
    apusysTraceBegin(__func__);
    /* find vaddr from map */
    got = mMemMap.find(mem);
    if (got == mMemMap.end()) {
        LOG_ERR("no apusysMem(%p)\n", static_cast<void *>(mem));
        ret = -EINVAL;
        goto out;
    }
    vmem = static_cast<apusysMem_v1 *>(got->second);

    /* check device va mapped */
    if (vmem->mem.deviceVa == 0) {
        LOG_ERR("mem(%p) no device va map\n", mem->vaddr);
        ret = -EINVAL;
        goto out;
    }

    memset(&ioctlMem, 0, sizeof(ioctlMem));
    ioctlMem.khandle = vmem->kHandle;
    ioctlMem.fd = vmem->shareFd;
    ioctlMem.iova_size = vmem->deviceVaSize;
    ioctlMem.uva = reinterpret_cast<unsigned long long>(vmem->mem.vaddr);
    ioctlMem.size = vmem->mem.size;
    ioctlMem.iova = vmem->mem.deviceVa;
    ioctlMem.align = vmem->mem.align;
    ioctlMem.mem_type = APUSYS_MEM_DRAM_ION;

    if (ioctl(mSession->getDevFd(), APUSYS_IOCTL_MEM_UNMAP, &ioctlMem)) {
        LOG_ERR("ioctl unmap device va fail(%s)\n", strerror(errno));
        ret = -abs(errno);
        goto out;
    }

    /* clear device va */
    vmem->mem.deviceVa = 0;
    vmem->kHandle = 0;

out:
    apusysTraceEnd();
    return ret;
}

struct apusysMem *apusysExecutor_v1::memImport(int handle, unsigned int size)
{
    struct apusys_mem ioctlMem;
    struct apusysMem_v1 *vmem = nullptr;
    ion_user_handle_t bufHandle;
    void *uva = nullptr;

    if (handle == MDW_APU_VLM_HANDLE)
        return &mVlmInfo;

    /* set ion memory shared */
    if(ion_import(mIonFd, handle, &bufHandle))
    {
        LOG_ERR("fail to get ion buffer by import (devFd=%d, shared_fd = %d)\n", mIonFd, handle);
        goto failImportIon;
    }

    /* map user va */
    uva = ion_mmap(mIonFd, NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, handle, 0);
    if (uva == MAP_FAILED)
    {
        LOG_ERR("get uva failed.\n");
        goto failIonMmap;
    }

    memset(&ioctlMem, 0, sizeof(ioctlMem));
    /* get iova from kernel */
    ioctlMem.uva = reinterpret_cast<unsigned long long>(uva);
    ioctlMem.iova = 0;
    ioctlMem.iova_size = 0;
    ioctlMem.mem_type = APUSYS_MEM_DRAM_ION;
    ioctlMem.align = 0;
    ioctlMem.fd = handle;

    if (ioctl(mSession->getDevFd(), APUSYS_IOCTL_MEM_IMPORT, &ioctlMem)) {
        LOG_ERR("ioctl import device va fail(%s)\n", strerror(errno));
        goto failMapDeviceVa;
    }

    vmem = new apusysMem_v1;
    if (vmem == nullptr)
        goto failAllocContainer;

    memset(vmem, 0, sizeof(*vmem));
    vmem->kHandle = ioctlMem.khandle;
    vmem->mem.vaddr = uva;
    vmem->mem.deviceVa = ioctlMem.iova;
    vmem->deviceVaSize = ioctlMem.iova_size;
    vmem->mem.size = size;
    vmem->shareFd = handle;
    vmem->mem.handle = handle;
    vmem->uHandle = bufHandle;
    MLOG_DEBUG("mem import(%p/%u/%d->%d) dva(0x%llx)\n",
        vmem->mem.vaddr, vmem->mem.size, handle, vmem->mem.handle,
        static_cast<unsigned long long>(vmem->mem.deviceVa));

    mMtx.lock();
    mMemMap.insert({&vmem->mem, vmem});
    mMtx.unlock();

    return &vmem->mem;

failAllocContainer:
failMapDeviceVa:
    ion_munmap(mIonFd, uva, size);
failIonMmap:
    if (ion_free(mIonFd, bufHandle))
        LOG_ERR("free ion fail\n");
failImportIon:
    return nullptr;
}

int apusysExecutor_v1::memUnImport(struct apusysMem *mem)
{
    struct apusys_mem ioctlMem;
    struct apusysMem_v1 *vmem = nullptr;
    int ret = 0;

    MLOG_DEBUG("mem unimport(%p/%u/%d) dva(0x%llx)\n",
        mem->vaddr, mem->size, mem->handle,
        static_cast<unsigned long long>(mem->deviceVa));

    if (mem->mtype == APUSYS_MEM_TYPE_VLM ||
        mem->mtype == APUSYS_MEM_TYPE_LOCAL)
        return 0;

    std::unordered_map<struct apusysMem *, void *>::const_iterator got;
    /* find vaddr from map */
    got = mMemMap.find(mem);
    if (got == mMemMap.end()) {
        LOG_ERR("no apusysMem(%p)\n", static_cast<void *>(mem));
        return -EINVAL;
    }
    vmem = static_cast<apusysMem_v1 *>(got->second);

    memset(&ioctlMem, 0, sizeof(ioctlMem));
    ioctlMem.khandle = vmem->kHandle;
    ioctlMem.uva = reinterpret_cast<unsigned long long>(vmem->mem.vaddr);
    ioctlMem.iova = vmem->mem.deviceVa;
    ioctlMem.iova_size = vmem->deviceVaSize;
    ioctlMem.size = vmem->mem.size;
    ioctlMem.align = vmem->mem.align;
    ioctlMem.mem_type = APUSYS_MEM_DRAM_ION;
    ioctlMem.fd = vmem->shareFd;

    if (ioctl(mSession->getDevFd(), APUSYS_IOCTL_MEM_UNIMPORT, &ioctlMem)) {
        LOG_ERR("ioctl unimport device va fail(%s)\n", strerror(errno));
        ret = -EINVAL;
        goto out;
    }

    if(ion_munmap(mIonFd, vmem->mem.vaddr, vmem->mem.size))
    {
        LOG_ERR("ion unmap fail\n");
        ret = -EINVAL;
        goto out;
    }

    if(ion_free(mIonFd, vmem->uHandle))
    {
        LOG_ERR("ion free fail, fd = %d, len = %d\n", mIonFd, vmem->mem.size);
        ret = -EINVAL;
        goto out;
    }

    mMemMap.erase(mem);
    delete vmem;

out:
    return ret;
}

int apusysExecutor_v1::memFlush(struct apusysMem *mem)
{
    struct ion_sys_data sysData;
    struct apusysMem_v1 *vmem = nullptr;

    std::unordered_map<struct apusysMem *, void *>::const_iterator got;
    /* find vaddr from map */
    got = mMemMap.find(mem);
    if (got == mMemMap.end()) {
        LOG_ERR("no apusysMem(%p)\n", static_cast<void *>(mem));
        return -EINVAL;
    }
    vmem = static_cast<apusysMem_v1 *>(got->second);

    memset(&sysData, 0, sizeof(sysData));
    sysData.sys_cmd = ION_SYS_CACHE_SYNC;
    sysData.cache_sync_param.handle = vmem->uHandle;
    sysData.cache_sync_param.sync_type = ION_CACHE_FLUSH_BY_RANGE;
    sysData.cache_sync_param.va = vmem->mem.vaddr;
    sysData.cache_sync_param.size = vmem->mem.size;

    if (ion_custom_ioctl(mIonFd, ION_CMD_SYSTEM, &sysData)) {
        LOG_ERR("ion memory flush fail(%s)\n", strerror(errno));
        return -ENOMEM;
    }

    return 0;
}

int apusysExecutor_v1::memInvalidate(struct apusysMem *mem)
{
    struct ion_sys_data sysData;
    struct apusysMem_v1 *vmem = nullptr;

    std::unordered_map<struct apusysMem *, void *>::const_iterator got;
    /* find vaddr from map */
    got = mMemMap.find(mem);
    if (got == mMemMap.end()) {
        LOG_ERR("no apusysMem(%p)\n", static_cast<void *>(mem));
        return -EINVAL;
    }
    vmem = static_cast<apusysMem_v1 *>(got->second);

    memset(&sysData, 0, sizeof(sysData));
    sysData.sys_cmd = ION_SYS_CACHE_SYNC;
    sysData.cache_sync_param.handle = vmem->uHandle;
    sysData.cache_sync_param.sync_type = ION_CACHE_INVALID_BY_RANGE;
    sysData.cache_sync_param.va = vmem->mem.vaddr;
    sysData.cache_sync_param.size = vmem->mem.size;

    if (ion_custom_ioctl(mIonFd, ION_CMD_SYSTEM, &sysData)) {
        LOG_ERR("ion memory invalidate fail(%s)\n", strerror(errno));
        return -ENOMEM;
    }

    return 0;
}
