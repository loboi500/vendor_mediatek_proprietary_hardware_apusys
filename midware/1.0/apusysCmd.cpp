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
#include <memory>
#include <algorithm>
#include <assert.h>
#include <unistd.h>

#include "apusysCmd.h"
#include "apusysMem.h"
#include "apusysCmn.h"

#include "cmdFormat.h"

apusysSubCmd::apusysSubCmd(int idx, int deviceType, IApusysMem * subCmdBuf, apusysCmd * parentCmd)
{
    assert(parentCmd != nullptr);
    assert(subCmdBuf != nullptr);
    LOG_DEBUG("idx = %d\n", idx);
    assert(idx >= 0);
    DEBUG_TAG;

    mIdx = idx;
    mSubCmdBuf = subCmdBuf;
    mParentCmd = parentCmd;
    mInfoEntry = 0;

    mPredecessorList.clear();
    mSuccessorList.clear();
    mDirty = false;

    mDeviceType = deviceType;
    mCtxId = VALUE_SUBGRAPH_CTX_ID_NONE;
    mPackId = 0;

    mTuneParam.bandwidth = 0;
    mTuneParam.boostVal = VALUE_SUBGRAPH_BOOST_NONE;
    mTuneParam.estimateTime = 0;
    mTuneParam.suggestTime = 0;
    mTuneParam.tcmForce = false;

    return;
}

apusysSubCmd::~apusysSubCmd()
{
    return;
}

unsigned int apusysSubCmd::_getBandwidthFromBuf()
{
    struct apusys_sc_hdr_cmn *hdr = nullptr;

    if (_getParentCmd()->getCmdBuf() == nullptr || mInfoEntry == 0)
    {
        LOG_WARN("get subcmd bandwidth fail, maybe modified but no construct cmd\n");
        return 0;
    }
    hdr = (struct apusys_sc_hdr_cmn *)mInfoEntry;

    return hdr->bandwidth;
}

unsigned int apusysSubCmd::_getExecTimeFromBuf()
{
    struct apusys_sc_hdr_cmn *hdr = nullptr;

    if (_getParentCmd()->getCmdBuf() == nullptr || mInfoEntry == 0)
    {
        LOG_WARN("get subcmd execution time fail, maybe modified but no construct cmd\n");
        return 0;
    }
    hdr = (struct apusys_sc_hdr_cmn *)mInfoEntry;

    return hdr->driver_time;
}

unsigned int apusysSubCmd::_getIpTimeFromBuf()
{
    struct apusys_sc_hdr_cmn *hdr = nullptr;

    if (_getParentCmd()->getCmdBuf() == nullptr || mInfoEntry == 0)
    {
        LOG_WARN("get subcmd execution time fail, maybe modified but no construct cmd\n");
        return 0;
    }
    hdr = (struct apusys_sc_hdr_cmn *)mInfoEntry;

    return hdr->ip_time;
}

unsigned int apusysSubCmd::_getTcmUsageFromBuf()
{
    struct apusys_sc_hdr_cmn *hdr = nullptr;

    if (_getParentCmd()->getCmdBuf() == nullptr || mInfoEntry == 0)
    {
        LOG_WARN("get subcmd tcm usage fail, maybe modified but no construct cmd\n");
        return 0;
    }
    hdr = (struct apusys_sc_hdr_cmn *)mInfoEntry;

    return hdr->tcm_usage;
}

void apusysSubCmd::_setDirtyFlag()
{
    mDirty = true;
}

apusysCmd* apusysSubCmd::_getParentCmd()
{
    return mParentCmd;
}

IApusysMem * apusysSubCmd::getSubCmdBuf()
{
    return mSubCmdBuf;
}

/*
 * apusysSubCmd public functions
 */
void apusysSubCmd::setInfoEntry(unsigned long long va)
{
    mInfoEntry = va;
}

void apusysSubCmd::setPredecessorList(std::vector<int>& predecessorList)
{
    mPredecessorList = predecessorList;
}

std::vector<int>& apusysSubCmd::getPredecessorList()
{
    return mPredecessorList;
}

void apusysSubCmd::setSuccessorList(std::vector<int>& successorList)
{
    mSuccessorList = successorList;
}

std::vector<int>& apusysSubCmd::getSuccessorList()
{
    return mSuccessorList;
}

void apusysSubCmd::setPackId(unsigned int packId)
{
    mPackId = packId;
}

unsigned int apusysSubCmd::getPackId()
{
    return mPackId;
}

int apusysSubCmd::getDeviceType()
{
    return mDeviceType;
}

void apusysSubCmd::setIdx(int idx)
{
    mIdx = idx;
}

int apusysSubCmd::getIdx()
{
    return mIdx;
}

bool apusysSubCmd::setCtxId(unsigned int ctxId)
{
    if (ctxId > mIdx && ctxId != VALUE_SUBGRAPH_CTX_ID_NONE)
    {
        LOG_WARN("ctx id is larger than idx(%u/%d)\n", ctxId, mIdx);
    }

    mCtxId = ctxId;
    return true;
}

unsigned int apusysSubCmd::getCtxId()
{
    return mCtxId;
}

/*
 * apusysSubCmd private functions
 */
bool apusysSubCmd::constructHeader()
{
    struct apusys_sc_hdr_cmn *hdr = (struct apusys_sc_hdr_cmn *)mInfoEntry;

    if (hdr == nullptr)
    {
        LOG_ERR("miss subcmd info entry\n");
        return false;
    }

    hdr->dev_type = mDeviceType;
    hdr->driver_time = mTuneParam.estimateTime;
    hdr->suggest_time = mTuneParam.suggestTime;
    hdr->bandwidth = mTuneParam.bandwidth;
    hdr->tcm_usage = 0;
    hdr->tcm_force = mTuneParam.tcmForce;
    hdr->boost_val = mTuneParam.boostVal;
    hdr->reserved = 0;
    hdr->mem_ctx = mCtxId;
    hdr->cb_info_size = mSubCmdBuf->size;   // codebuf info size
    hdr->ofs_cb_info = mSubCmdBuf->getMemFd() | (1UL << SUBGRAPH_CODEBUF_INFO_BIT_FD);    // codebuf info offset
    hdr->pack_id = mPackId;

    return true;
}

bool apusysSubCmd::getRunInfo(struct apusysSubCmdRunInfo & info)
{
    info.bandwidth = _getBandwidthFromBuf();
    info.tcmUsage = _getTcmUsageFromBuf();
    info.execTime = _getExecTimeFromBuf();
    info.ipTime = _getIpTimeFromBuf();

    return true;
}

struct apusysSubCmdTuneParam& apusysSubCmd::getTuneParam()
{
    return mTuneParam;
}

bool apusysSubCmd::setTuneParam(struct apusysSubCmdTuneParam& tune)
{
    struct apusys_sc_hdr_cmn *hdr = (struct apusys_sc_hdr_cmn *)mInfoEntry;

    if (tune.boostVal > 100 && tune.boostVal != VALUE_SUBGRAPH_BOOST_NONE)
    {
        LOG_ERR("wrong boost value(%d)\n", (int)tune.boostVal);
        return false;
    }

    mTuneParam = tune;
    LOG_DEBUG("mTuneParam:(%u/%d/%u/%u/%d)\n", mTuneParam.bandwidth, mTuneParam.boostVal, mTuneParam.estimateTime, mTuneParam.suggestTime, mTuneParam.tcmForce);

    if (mInfoEntry != 0)
    {
        LOG_DEBUG("write parameter into subcmd header\n");
        hdr->bandwidth = mTuneParam.bandwidth;
        hdr->driver_time = mTuneParam.estimateTime;
        hdr->suggest_time = mTuneParam.suggestTime;
        hdr->tcm_force = mTuneParam.tcmForce;
        hdr->boost_val = mTuneParam.boostVal;
    }

    return true;
}

bool apusysSubCmd::setIOParam()
{
    struct apusys_sc_hdr_cmn *hdr = (struct apusys_sc_hdr_cmn *)mInfoEntry;

    if (mInfoEntry != 0)
    {
        LOG_DEBUG("write i/o param into subcmd header(%u->%u)\n", hdr->boost_val, mTuneParam.boostVal);
        hdr->boost_val = mTuneParam.boostVal;
        return true;
    }

    return false;
}

apusysCmd::apusysCmd(IApusysEngine * engine)
{
    assert(engine != nullptr);

    /* init private member */
    //mCmdId = (unsigned long long)this;
    mSubCmdList.clear();
    mCmdBuf = 0;
    mDirty = false;
    mDependencySize = 0;
    mTypeCmdNum.assign(APUSYS_DEVICE_MAX, 0);
    mEngine = engine; // for memory allocator
    mExecutePid = 0;
    mExecuteTid = 0;
    mPackNum = 0;
    mIsExecuting = false;
    mFenceEnable = false;

    /* user may set */
    mProperty.hardLimit = 0;
    mProperty.softLimit = 0;
    mProperty.priority = APUSYS_PRIORITY_NORMAL;
    mProperty.powerSave = false;
    mFlagBitmap.reset();

    return;
}

apusysCmd::~apusysCmd()
{
    apusysSubCmd * subcmd = nullptr;

    /* delete all subcmd */
    while(!mSubCmdList.empty())
    {
        subcmd = mSubCmdList.back();
        if (subcmd == nullptr)
        {
            LOG_ERR("release all subcmd fail, iter null\n");
            break;
        }

        delete subcmd;
        mSubCmdList.pop_back();
    }

    /* free apusys cmdbuf */
    if (mCmdBuf != nullptr)
    {
        if (mEngine->memFree(mCmdBuf) == false)
        {
            LOG_ERR("release cmd fail, free origin cmdbuf error\n");
        }
    }
    else
    {
        LOG_ERR("miss mCmdbuf\n");
    }

    mTypeCmdNum.clear();

    return;
}

/*
 * apusysCmd private functions
 */
unsigned int apusysCmd::_calcCmdSize()
{
    unsigned int numSubgraph = 0;
    unsigned int cmdSize = 0;

    /* header + dependency + numsubgraphs + subgraphs + pmu infos */
    numSubgraph = mSubCmdList.size();
    if (numSubgraph == 0)
    {
        return 0;
    }

    cmdSize = (sizeof(struct apusys_cmd_hdr) - sizeof(unsigned int)) + sizeof(struct apusys_cmd_hdr_fence)
        + numSubgraph * SIZE_SUBGRAPH_SCOFS_ELEMENT + mDependencySize
        + numSubgraph * SIZE_CMD_SUCCESSOR_ELEMENT + numSubgraph * sizeof(struct apusys_sc_hdr_cmn);

    /* sample/vpu command add unsigned int for pack cmd */
    LOG_DEBUG("subcmd device num mdla(%d) dsp(%d)\n", mTypeCmdNum.at(APUSYS_DEVICE_MDLA), mTypeCmdNum.at(APUSYS_DEVICE_VPU));
    cmdSize += sizeof(struct apusys_sc_hdr_mdla) * mTypeCmdNum.at(APUSYS_DEVICE_MDLA);

    LOG_DEBUG("cmd size : %u:(%u/%lu*%lu/%lu/%lu/%lu*%lu/%u*%u/%u*%d)\n",
        cmdSize, (unsigned int)(sizeof(struct apusys_cmd_hdr) - sizeof(unsigned int)),
        (unsigned long)numSubgraph, (unsigned long)SIZE_SUBGRAPH_SCOFS_ELEMENT,
        (unsigned long)sizeof(struct apusys_cmd_hdr_fence),
        (unsigned long)mDependencySize, (unsigned long)numSubgraph, (unsigned long)SIZE_CMD_PREDECCESSOR_CMNT_ELEMENT,
        numSubgraph, (unsigned int)sizeof(struct apusys_sc_hdr_cmn),
        (unsigned int)sizeof(struct apusys_sc_hdr_mdla), mTypeCmdNum.at(APUSYS_DEVICE_MDLA));

    return cmdSize;
}

bool apusysCmd::_setCmdPropertyToBuf()
{
    struct apusys_cmd_hdr *cmdHdr = nullptr;

    if (mCmdBuf == nullptr)
    {
        LOG_WARN("miss cmdBuf\n");
        return false;
    }

    cmdHdr = (struct apusys_cmd_hdr *)mCmdBuf->va;
    if (cmdHdr == nullptr)
    {
        LOG_WARN("miss common header va\n");
        return false;
    }

    cmdHdr->priority = mProperty.priority;
    cmdHdr->hard_limit = mProperty.hardLimit;
    cmdHdr->soft_limit = mProperty.softLimit;
    cmdHdr->flag_bitmap = mFlagBitmap.to_ullong();

    return true;
}

unsigned int apusysCmd::size()
{
    return (unsigned int)mSubCmdList.size();
}

bool apusysCmd::constructCmd()
{
    std::unique_lock<std::mutex> mutexLock(mMtx);
    unsigned int cmdBufSize = 0, numSubCmd = 0;
    int i = 0, j = 0;
    apusysSubCmd * subCmd = nullptr;
    unsigned long long subCmdVa = 0, cmdBufVa = 0;
    unsigned long long dependencyVa = 0, tempDependecyAddr = 0, pdrCntListVa = 0;
    unsigned int subCmdListSize = 0;
    std::vector<int> tmpList;
    struct apusys_cmd_hdr *cmdHdr = nullptr;
    DEBUG_TAG;

    if (mIsExecuting == true)
    {
        LOG_ERR("cmd(0x%llx) is executed by pid(%d/%d)\n", (unsigned long long)mCmdBuf, mExecutePid, mExecuteTid);
        return false;
    }

    /* check cmdbuf */
    if (mDirty == false)
    {
        if (mCmdBuf != nullptr)
        {
            LOG_DEBUG("re-assign i/o param\n");
            for (i = 0; (unsigned int)i < mSubCmdList.size(); i++)
            {
                subCmd = (apusysSubCmd *)getSubCmd(i);
                if (subCmd == nullptr)
                {
                    LOG_ERR("miss subcmd(%d)\n", i);
                    return false;
                }
                subCmd->setIOParam();
            }

            return true;
        }
        else
        {
            LOG_ERR("construct cmd w/o any subcmd\n");
            return false;
        }
    }
    DEBUG_TAG;

    /* calculate total apusys size for alloc */
    cmdBufSize = _calcCmdSize();
    if (cmdBufSize == 0)
    {
        LOG_ERR("apusys no subcmd\n");
        return false;
    }

    if (mCmdBuf != nullptr)
    {
        if (mEngine->memFree(mCmdBuf) == false)
        {
            LOG_ERR("re-construct cmd fail, free origin cmdbuf error\n");
            return false;
        }
        else
        {
            /* re-construct apusys cmd */
            mCmdBuf = nullptr;
            LOG_WARN("re-construct cmd w/o cmdbuf\n");
            //return constructCmd();
        }
    }

    /* allocate cmdBuf */
    mCmdBuf = mEngine->memAlloc(cmdBufSize);
    if (mCmdBuf == nullptr)
    {
        LOG_ERR("allocate cmdbuf(%d) fail\n", cmdBufSize);
        return false;
    }

    /* num of subcmd */
    numSubCmd = mSubCmdList.size();
    subCmdListSize = numSubCmd * SIZE_SUBGRAPH_SCOFS_ELEMENT;

    /* fill header */
    cmdBufVa = mCmdBuf->va;

    cmdHdr = (struct apusys_cmd_hdr *)cmdBufVa;

    cmdHdr->magic = APUSYS_MAGIC_NUMBER;
    cmdHdr->uid = (unsigned long long)mCmdBuf;
    cmdHdr->version = APUSYS_CMD_VERSION;
    cmdHdr->usr_pid = 0;
    //cmdHdr->usr_pid = (unsigned short)getpid();
    if (_setCmdPropertyToBuf() == false)
    {
        LOG_ERR("set cmd property fail\n");
    }
    cmdHdr->num_sc = numSubCmd;

    subCmdVa = (unsigned long long)&cmdHdr->scofs_list_entry + subCmdListSize + sizeof(apusys_cmd_hdr_fence);
    for (i = 0; (unsigned int)i < numSubCmd; i++)
    {
        subCmd = mSubCmdList.at(i);

        /* assign subgraph's offset */
        *(TYPE_SUBGRAPH_SCOFS_ELEMENT *)((uint64_t)cmdHdr + (sizeof(*cmdHdr)-SIZE_SUBGRAPH_SCOFS_ELEMENT) +
            SIZE_SUBGRAPH_SCOFS_ELEMENT * i) = subCmdVa - cmdBufVa;
        //*(TYPE_SUBGRAPH_SCOFS_ELEMENT *)(&cmdHdr->scofs_list_entry + i) = subCmdVa - cmdBufVa;
        subCmd->setInfoEntry(subCmdVa);
        if (subCmd->constructHeader() == false)
        {
            LOG_ERR("construct #%d subcmd fail\n", i);
            return false;
        }

        subCmdVa += sizeof(struct apusys_sc_hdr_cmn);
        switch (subCmd->getDeviceType())
        {
            case APUSYS_DEVICE_MDLA:
                subCmdVa += sizeof(struct apusys_sc_hdr_mdla);
                break;
            default:
                break;
        }
    }
    /* fill dependency list */
    dependencyVa = subCmdVa;
    pdrCntListVa = dependencyVa + mDependencySize;
    cmdHdr->ofs_scr_list = dependencyVa - cmdBufVa;
    cmdHdr->ofs_pdr_cnt_list = pdrCntListVa - cmdBufVa;

    tempDependecyAddr = dependencyVa;
    for (i = 0; (unsigned int)i < numSubCmd; i++)
    {
        /* setup self subcmd id */
        subCmd = mSubCmdList.at(i);
        *(TYPE_CMD_SUCCESSOR_ELEMENT *)tempDependecyAddr = (TYPE_CMD_SUCCESSOR_ELEMENT)subCmd->getSuccessorList().size();
        tempDependecyAddr += SIZE_CMD_SUCCESSOR_ELEMENT;
        LOG_DEBUG("#%d subcmd: set successor size(%lu)\n", i, (unsigned long)subCmd->getSuccessorList().size());

        /* setup successor id */
        for (j = 0; (unsigned int)j < subCmd->getSuccessorList().size(); j++)
        {
            if (subCmd->getSuccessorList().at(j) == subCmd->getIdx())
            {
                continue;
            }
            *(TYPE_CMD_SUCCESSOR_ELEMENT *)tempDependecyAddr = (TYPE_CMD_SUCCESSOR_ELEMENT)subCmd->getSuccessorList().at(j);
            LOG_DEBUG("#%d subcmd: set successor (%d)\n", i, subCmd->getSuccessorList().at(j));
            tempDependecyAddr += SIZE_CMD_SUCCESSOR_ELEMENT;
        }

        /* setup predecessor cnt */
        *(TYPE_CMD_PREDECCESSOR_CMNT_ELEMENT *)pdrCntListVa = subCmd->getPredecessorList().size();
        pdrCntListVa += SIZE_CMD_PREDECCESSOR_CMNT_ELEMENT;
    }

    /* mark this buffer is not dirty */
    mDirty = false;

    return true;
}

/*
 * public functions
 */
/* cmd functions */
struct apusysCmdProp& apusysCmd::getCmdProperty()
{
    return mProperty;
}

bool apusysCmd::setCmdProperty(const struct apusysCmdProp &prop)
{
    /* critical session */
    std::unique_lock<std::mutex> mutexLock(mMtx);

    if (prop.priority > APUSYS_PRIORITY_MAX)
    {
        LOG_ERR("prop invalid param(%hu/%hu/%u/%d)\n", prop.hardLimit, prop.softLimit, prop.priority, prop.powerSave);
        return false;
    }

    mProperty = prop;
    mFlagBitmap.set(CMD_FLAG_BITMAP_POWERSAVE, prop.powerSave);
    if (mCmdBuf == nullptr)
    {
        mDirty = true;
    }
    else
    {
        /* setup header */
        if (_setCmdPropertyToBuf() == false)
        {
            mDirty = true;
        }
    }

    return true;
}

int apusysCmd::addSubCmd(IApusysMem * subCmdBuf, APUSYS_DEVICE_E deviceType, std::vector<int> & dependency)
{
    struct apusysSubCmdTuneParam tune;

    memset(&tune, 0, sizeof(tune));
    tune.boostVal = VALUE_SUBGRAPH_BOOST_NONE;

    return addSubCmd(subCmdBuf, deviceType, dependency, VALUE_SUBGRAPH_CTX_ID_NONE);
}

int apusysCmd::addSubCmd(IApusysMem * subCmdBuf, APUSYS_DEVICE_E deviceType,
                         std::vector<int> & dependency,
                         unsigned int ctxId)
{
    int subCmdId = -1;
    unsigned int i = 0;
    apusysSubCmd * subCmd = nullptr;
    apusysSubCmd * tmpSubCmd = nullptr;
    std::vector<int> tmpList;

    if (mEngine->getDeviceNum(deviceType) <= 0)
    {
        LOG_ERR("not support device type(%d)\n\n", deviceType);
        return -1;
    }

    /* check argument valid */
    if(subCmdBuf == nullptr)
    {
        LOG_ERR("allocate subCmd fail\n");
        return -1;
    }



    /* critical session */
    std::unique_lock<std::mutex> mutexLock(mMtx);
    /* get subCmd idx */
    subCmdId = mSubCmdList.size();

    /* new subCmd */
    subCmd = new apusysSubCmd(subCmdId, deviceType, subCmdBuf, this);
    if(subCmd == nullptr)
    {
        LOG_ERR("allocate subCmd fail\n");
        return -1;
    }

    /* set variable */
    subCmd->setCtxId(ctxId);
    subCmd->setPredecessorList(dependency);
    subCmd->setPackId(0);

    /* add sub cmd to private list */
    mSubCmdList.push_back(subCmd);
    mTypeCmdNum.at(deviceType)++;

    /* add be-dependency list */
    for (i = 0; i < dependency.size(); i++)
    {
        tmpSubCmd = mSubCmdList.at(dependency.at(i));
        tmpList = tmpSubCmd->getSuccessorList();
        tmpList.push_back(subCmd->getIdx());
        tmpSubCmd->setSuccessorList(tmpList);
    }

    /* mark this apusys cmd has been changed, should construct apusys cmd again before fire cmd */
    mDirty = true;

    /* record total dependency size for easy to calculate total cmdbuf size */
    mDependencySize += ((1 + dependency.size()) * SIZE_CMD_SUCCESSOR_ELEMENT);

    LOG_DEBUG("subCmdId = %d, size = %d\n",subCmdId, subCmd->getSubCmdBuf()->size);

    return subCmdId;
}

IApusysSubCmd * apusysCmd::getSubCmd(int idx)
{
    if ((unsigned int)idx >= mSubCmdList.size())
    {
        LOG_ERR("invalid idx(%d/%lu)\n", idx, (unsigned long)mSubCmdList.size());
        return nullptr;
    }

    LOG_DEBUG("subCmd(%d/%p)\n", idx, (void *)mSubCmdList.at(idx));

    return (IApusysSubCmd *)mSubCmdList.at(idx);
}

int apusysCmd::packSubCmd(IApusysMem * subCmdBuf, APUSYS_DEVICE_E deviceType, int subCmdIdx)
{
    apusysSubCmd * subCmd = nullptr;
    apusysSubCmd * targetSubCmd = nullptr, * tempSubCmd = nullptr;
    std::vector<int> tmpList;
    int retIdx = 0;
    unsigned int i = 0;

    /* check argument */
    if (mEngine->getDeviceNum(deviceType) <= 0)
    {
        LOG_ERR("not support device type(%d)\n\n", deviceType);
        return -1;
    }

    if (deviceType >= APUSYS_DEVICE_MAX)
    {
        LOG_ERR("device(%d) command not support pack function\n", deviceType);
        return -1;
    }

    if (subCmdBuf == nullptr)
    {
        LOG_ERR("invalid sub-command buffer\n");
        return -1;
    }

    if (subCmdIdx > (int)(mSubCmdList.size()-1))
    {
        LOG_ERR("pack idx over list number(%d/%u)\n", subCmdIdx, (unsigned int)mSubCmdList.size());
        return -1;
    }

    /* critical session */
    std::unique_lock<std::mutex> mutexLock(mMtx);

    targetSubCmd = mSubCmdList.at(subCmdIdx);
    if (targetSubCmd->getDeviceType() != deviceType)
    {
        LOG_ERR("target subcmd(%d) type(%d) is not same support pack function\n", subCmdIdx, targetSubCmd->getDeviceType());
        return -1;
    }

    /* get idx */
    retIdx = mSubCmdList.size();

    /* new subCmd */
    subCmd = new apusysSubCmd(retIdx, deviceType, subCmdBuf, this);
    if(subCmd == nullptr)
    {
        LOG_ERR("allocate subCmd fail\n");
        return -1;
    }

    /* set variable */
    subCmd->setPredecessorList(targetSubCmd->getPredecessorList());
    subCmd->setSuccessorList(targetSubCmd->getSuccessorList());
    subCmd->setTuneParam(targetSubCmd->getTuneParam());
    if (targetSubCmd->getPackId() == 0) {
        mPackNum++;
        targetSubCmd->setPackId(mPackNum);
        LOG_DEBUG("pack id(%d)\n", mPackNum);
    }
    subCmd->setPackId(targetSubCmd->getPackId());

    /* packed cmd's ctx id should be the same */
    subCmd->setCtxId(targetSubCmd->getCtxId());

    /* set subcmd's idx */
    mSubCmdList.push_back(subCmd);
    mTypeCmdNum.at(deviceType)++;

    mDependencySize += ((1 + subCmd->getPredecessorList().size()) * SIZE_CMD_SUCCESSOR_ELEMENT);
    /* add dependency list to next subcmd */
    for (i = 0;i<subCmd->getSuccessorList().size(); i++)
    {
        tempSubCmd = mSubCmdList.at(subCmd->getSuccessorList().at(i));
        tmpList = tempSubCmd->getPredecessorList();
        tmpList.push_back(retIdx);
        std::sort(tmpList.begin(), tmpList.end());
        tempSubCmd->setPredecessorList(tmpList);
        mDependencySize += SIZE_CMD_SUCCESSOR_ELEMENT;
    }

    /* add be dependency to prev subcmd */
    for (i = 0;i<subCmd->getPredecessorList().size(); i++)
    {
        tempSubCmd = mSubCmdList.at(subCmd->getPredecessorList().at(i));
        tmpList = tempSubCmd->getSuccessorList();
        tmpList.push_back(retIdx);
        std::sort(tmpList.begin(), tmpList.end());
        tempSubCmd->setSuccessorList(tmpList);
    }

    mDirty = true;
    LOG_INFO("return idx(%d)\n", retIdx);

    return retIdx;
}

IApusysMem * apusysCmd::getCmdBuf()
{
    std::unique_lock<std::mutex> mutexLock(mMtx);

    if(mDirty == true)
    {
        if(constructCmd() == false)
        {
            return nullptr;
        }
    }

    if(mCmdBuf == nullptr)
    {
        LOG_ERR("cmd buffer invalid(%p)\n", (void *)mCmdBuf);
        return nullptr;
    }

    return mCmdBuf;
}

unsigned long long apusysCmd::getCmdId()
{
    return (unsigned long long)mCmdBuf;
}

bool setCmdIdToBuf(IApusysMem *cmdBuf, unsigned long long cmdId)
{
    struct apusys_cmd_hdr *cmdHdr = nullptr;
    UNUSED(cmdId);

    if (cmdBuf == nullptr)
    {
        LOG_ERR("no cmdBuf\n");
        return false;
    }

    cmdHdr = (struct apusys_cmd_hdr *)cmdBuf->va;
    if (cmdHdr == nullptr)
    {
        LOG_ERR("no cmdBuf va\n");
        return false;
    }

    cmdHdr->uid = (unsigned long long)cmdBuf;
    return true;
}

bool apusysCmd::setCmdId(unsigned long long cmdId)
{
    return setCmdIdToBuf(mCmdBuf, cmdId);
}

bool apusysCmd::checkCmdValid(IApusysMem * cmdBuf)
{
    struct apusys_cmd_hdr *cmdHdr = nullptr;

    LOG_INFO("check apusys cmd valid...\n");

    /* check cmd buffer */
    if (cmdBuf == nullptr)
    {
        LOG_ERR("invalid argument\n");
        return false;
    }

    cmdHdr = (struct apusys_cmd_hdr *)cmdBuf->va;
    if (cmdHdr == nullptr)
    {
        LOG_ERR("invalid cmdbuf addr(%p)\n", (void *)cmdHdr);
        return false;
    }

    /* check header*/
    if (cmdHdr->magic != APUSYS_MAGIC_NUMBER)
    {
        LOG_ERR("invalid magic number\n");
        return false;
    }

    if (cmdHdr->uid == 0)
    {
        LOG_ERR("invalid cmd id\n");
        return false;
    }

    if (cmdHdr->version != APUSYS_CMD_VERSION)
    {
        LOG_ERR("invalid cmd version(%d/%d)\n", cmdHdr->version, APUSYS_CMD_VERSION);
        return false;
    }

    if (cmdHdr->priority >= APUSYS_PRIORITY_MAX)
    {
        LOG_ERR("invalid priority(%d)\n", cmdHdr->priority);
        return false;
    }

    if (cmdHdr->num_sc == 0)
    {
        LOG_ERR("invalid num of subcmd(%d)\n", cmdHdr->num_sc);
        return false;
    }

    LOG_INFO("check apusys cmd valid done\n");

    return true;;
}

void apusysCmd::printCmdTime(IApusysMem * cmdBuf)
{
    UNUSED(cmdBuf);

    return;
}

void apusysCmd::printCmd(IApusysMem * cmdBuf)
{
    unsigned long long cmdBufVa = 0;

    LOG_DEBUG("print apusys cmd valid...\n");

    /* check cmd buffer */
    if (cmdBuf == nullptr)
    {
        LOG_ERR("invalid argument\n");
        return;
    }

    cmdBufVa = cmdBuf->va;
    if (cmdBufVa == 0)
    {
        LOG_ERR("invalid cmdbuf addr(0x%llx)\n", cmdBufVa);
        return;
    }
#if 0
    /* print header*/
    LOG_DEBUG("======================================\n");
    LOG_DEBUG(" apusys command header\n");
    LOG_DEBUG("--------------------------------------\n");
    LOG_DEBUG(" magic number = 0x%llx\n", _getMagicNumFromBuf());
    LOG_DEBUG(" cmd id       = 0x%llx\n", _getCmdIdFromBuf());
    LOG_DEBUG(" cmd version  = 0x%x\n", _getCmdVersionFromBuf());
    LOG_DEBUG(" priority     = %d\n", _getPriorityFromBuf());
    LOG_DEBUG(" #sub cmd     = 0x%u\n", _getNumSubcmdFromBuf());
    LOG_DEBUG("======================================\n");
#endif
    return;
}

bool apusysCmd::markExecute()
{
    std::unique_lock<std::mutex> mutexLock(mMtx);

    if (mIsExecuting == true)
    {
        LOG_ERR("cmd(0x%llx) is executing by pid(%d/%d)\n", (unsigned long long)mCmdBuf, mExecutePid, mExecuteTid);
        return false;
    }

    mIsExecuting = true;
    mExecutePid = getpid();
    mExecuteTid = gettid();

    return true;
}

bool apusysCmd::unMarkExecute()
{
    std::unique_lock<std::mutex> mutexLock(mMtx);

    if (mIsExecuting == false)
    {
        LOG_WARN("cmd(0x%llx) is not executed\n", (unsigned long long)mCmdBuf);
    }

    mIsExecuting = false;
    mExecutePid = 0;
    mExecuteTid = 0;

    return true;
}

bool apusysCmd::setFence(bool enable)
{
    mFenceEnable = enable;
    mFlagBitmap.set(CMD_FLAG_BITMAP_FENCE, mFenceEnable);
    if (mCmdBuf != nullptr)
    {
        _setCmdPropertyToBuf();
    }

    return true;
}

bool apusysCmd::getFence(struct apusysCmdFence &fence)
{
    struct apusys_cmd_hdr_fence *hdr = nullptr;
    int numSubCmd = mSubCmdList.size();;
    unsigned int ofs;

    if (mCmdBuf == nullptr || numSubCmd <= 0)
        return false;

    ofs = sizeof(struct apusys_cmd_hdr) + ((numSubCmd - 1) * SIZE_SUBGRAPH_SCOFS_ELEMENT);
    /* check size */
    if (ofs > mCmdBuf->size)
        return false;

    hdr = (struct apusys_cmd_hdr_fence *)(mCmdBuf->va + ofs);

    fence.fd = hdr->fd;
    fence.status = hdr->status;
    fence.totalTime = hdr->total_time;

    return true;
}

