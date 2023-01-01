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
#include <cstring>
#include <vector>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include "apusys_drv.h"

#include <sys/ioctl.h>
#include <cutils/properties.h>

#include "apusys.h"
#include "apusysEngine.h"
#include "apusysCmn.h"
#include "apusysCmd.h"

#include "ionAllocator.h"
#include "ionAospAllocator.h"
#include "dmaAllocator.h"

#include "vlmAllocator.h"

apusysSubCmd::apusysSubCmd(int idx, int deviceType, IApusysMem * subCmdBuf, apusysCmd * parentCmd)
{
    UNUSED(idx);
    UNUSED(deviceType);
    UNUSED(subCmdBuf);
    UNUSED(parentCmd);

    mDeviceType = 0;
    mIdx = 0;
    mSubCmdBuf = nullptr;
    mPredecessorList.clear();
    mSuccessorList.clear();

    mCtxId = 0;
    mPackId = 0;
    mInfoEntry = 0;
    memset(&mTuneParam, 0, sizeof(mTuneParam));
    mDirty = false;
    mParentCmd = nullptr;

    LOG_ERR("LD 2.0 not support libapusys\n");
    abort();
}

apusysSubCmd::~apusysSubCmd()
{
}

unsigned int apusysSubCmd::_getBandwidthFromBuf()
{
    return 0;
}

unsigned int apusysSubCmd::_getExecTimeFromBuf()
{
    return 0;
}

unsigned int apusysSubCmd::_getIpTimeFromBuf()
{
    return 0;
}

unsigned int apusysSubCmd::_getTcmUsageFromBuf()
{
    return 0;
}

void apusysSubCmd::_setDirtyFlag()
{
}

apusysCmd* apusysSubCmd::_getParentCmd()
{
    return nullptr;
}

IApusysMem * apusysSubCmd::getSubCmdBuf()
{
    return nullptr;
}

void apusysSubCmd::setInfoEntry(unsigned long long va)
{
    UNUSED(va);
}

void apusysSubCmd::setPredecessorList(std::vector<int>& predecessorList)
{
    UNUSED(predecessorList);
}

std::vector<int>& apusysSubCmd::getPredecessorList()
{
    return mPredecessorList;
}

void apusysSubCmd::setSuccessorList(std::vector<int>& successorList)
{
    UNUSED(successorList);
}

std::vector<int>& apusysSubCmd::getSuccessorList()
{
    return mSuccessorList;
}

void apusysSubCmd::setPackId(unsigned int packId)
{
    UNUSED(packId);
}

unsigned int apusysSubCmd::getPackId()
{
    return 0;
}

int apusysSubCmd::getDeviceType()
{
    return 0;
}

void apusysSubCmd::setIdx(int idx)
{
    UNUSED(idx);
}

int apusysSubCmd::getIdx()
{
    return 0;
}

bool apusysSubCmd::setCtxId(unsigned int ctxId)
{
    UNUSED(ctxId);
    return false;
}

unsigned int apusysSubCmd::getCtxId()
{
    return 0;
}

bool apusysSubCmd::constructHeader()
{
    return false;
}

bool apusysSubCmd::getRunInfo(struct apusysSubCmdRunInfo & info)
{
    UNUSED(info);
    return false;
}

struct apusysSubCmdTuneParam& apusysSubCmd::getTuneParam()
{
    return mTuneParam;
}

bool apusysSubCmd::setTuneParam(struct apusysSubCmdTuneParam& tune)
{
    UNUSED(tune);
    return false;
}

bool apusysSubCmd::setIOParam()
{
    return false;
}

apusysCmd::apusysCmd(IApusysEngine * engine)
{
    UNUSED(engine);

    mDependencySize = 0;
    mCmdBuf = nullptr;
    mSubCmdEntryList.clear();
    mSubCmdList.clear();
    mTypeCmdNum.clear();
    mPackNum = 0;

    mDirty = false;
    mExecutePid = 0;
    mExecuteTid = 0;
    mIsExecuting = false;
    mFenceEnable = false;
    mEngine = nullptr;
    memset(&mProperty, 0, sizeof(mProperty));
    mFlagBitmap.reset();

    LOG_ERR("LD 2.0 not support libapusys\n");
    abort();
}

apusysCmd::~apusysCmd()
{
}

unsigned int apusysCmd::_calcCmdSize()
{
    return 0;
}

bool apusysCmd::_setCmdPropertyToBuf()
{
    return false;
}

unsigned int apusysCmd::size()
{
    return 0;
}

bool apusysCmd::constructCmd()
{
    return false;
}

struct apusysCmdProp& apusysCmd::getCmdProperty()
{
    return mProperty;
}

bool apusysCmd::setCmdProperty(const struct apusysCmdProp &prop)
{
    UNUSED(prop);
    return false;
}

int apusysCmd::addSubCmd(IApusysMem * subCmdBuf, APUSYS_DEVICE_E deviceType, std::vector<int> & dependency)
{
    UNUSED(subCmdBuf);
    UNUSED(deviceType);
    UNUSED(dependency);
    return 0;
}

int apusysCmd::addSubCmd(IApusysMem * subCmdBuf, APUSYS_DEVICE_E deviceType,
                         std::vector<int> & dependency,
                         unsigned int ctxId)
{
    UNUSED(subCmdBuf);
    UNUSED(deviceType);
    UNUSED(dependency);
    UNUSED(ctxId);
    return 0;
}

IApusysSubCmd * apusysCmd::getSubCmd(int idx)
{
    UNUSED(idx);
    return nullptr;
}

int apusysCmd::packSubCmd(IApusysMem * subCmdBuf, APUSYS_DEVICE_E deviceType, int subCmdIdx)
{
    UNUSED(subCmdBuf);
    UNUSED(deviceType);
    UNUSED(subCmdIdx);
    return 0;
}

IApusysMem * apusysCmd::getCmdBuf()
{
    return nullptr;
}

unsigned long long apusysCmd::getCmdId()
{
    return 0;
}

bool apusysCmd::setCmdId(unsigned long long cmdId)
{
    UNUSED(cmdId);
    return false;
}

bool apusysCmd::checkCmdValid(IApusysMem * cmdBuf)
{
    UNUSED(cmdBuf);
    return false;
}

void apusysCmd::printCmdTime(IApusysMem * cmdBuf)
{
    UNUSED(cmdBuf);
}

void apusysCmd::printCmd(IApusysMem * cmdBuf)
{
    UNUSED(cmdBuf);
}

bool apusysCmd::markExecute()
{
    return false;
}

bool apusysCmd::unMarkExecute()
{
    return false;
}

bool apusysCmd::setFence(bool enable)
{
    UNUSED(enable);
    return false;
}

bool apusysCmd::getFence(struct apusysCmdFence &fence)
{
    UNUSED(fence);
    return false;
}

apusysEngine::apusysEngine(const char * userName)
{
    UNUSED(userName);

    LOG_ERR("===========================================\n");
    LOG_ERR("| LD 2.0 not suppot libapusys\n");
    LOG_ERR("===========================================\n");
    abort();
}

apusysEngine::apusysEngine():apusysEngine("defaultUser")
{
}

apusysEngine::~apusysEngine()
{
}

IApusysCmd * apusysEngine::initCmd()
{
    return nullptr;
}

bool apusysEngine::destroyCmd(IApusysCmd * cmd)
{
    UNUSED(cmd);
    return false;
}

bool apusysEngine::runCmd(IApusysCmd * cmd)
{
    UNUSED(cmd);
    return false;
}

bool apusysEngine::runCmdAsync(IApusysCmd * cmd)
{
    UNUSED(cmd);
    return false;
}

bool apusysEngine::waitCmd(IApusysCmd * cmd)
{
    UNUSED(cmd);
    return false;
}

bool apusysEngine::runCmd(IApusysMem * cmdBuf)
{
    UNUSED(cmdBuf);
    return false;
}

bool apusysEngine::runCmdAsync(IApusysMem * cmdBuf)
{
    UNUSED(cmdBuf);
    return false;
}

bool apusysEngine::waitCmd(IApusysMem * cmdBuf)
{
    UNUSED(cmdBuf);
    return false;
}

bool apusysEngine::checkCmdValid(IApusysMem * cmdBuf)
{
    UNUSED(cmdBuf);
    return false;
}

bool apusysEngine::sendUserCmdBuf(IApusysMem * cmdBuf, APUSYS_DEVICE_E deviceType)
{
    UNUSED(cmdBuf);
    UNUSED(deviceType);
    return false;
}

bool apusysEngine::sendUserCmdBuf(IApusysMem * cmdBuf, APUSYS_DEVICE_E deviceType, int idx)
{
    UNUSED(cmdBuf);
    UNUSED(deviceType);
    UNUSED(idx);
    return false;
}

IApusysMem * apusysEngine::memAlloc(size_t size, unsigned int align, unsigned int cache, APUSYS_USER_MEM_E type)
{
    UNUSED(size);
    UNUSED(align);
    UNUSED(cache);
    UNUSED(type);
    return nullptr;
}

IApusysMem * apusysEngine::memAlloc(size_t size, unsigned int align, unsigned int cache)
{
    UNUSED(size);
    UNUSED(align);
    UNUSED(cache);
    return nullptr;
}

IApusysMem * apusysEngine::memAlloc(size_t size, unsigned int align)
{
    UNUSED(size);
    UNUSED(align);
    return nullptr;
}

IApusysMem * apusysEngine::memAlloc(size_t size)
{
    UNUSED(size);
    return nullptr;
}

bool apusysEngine::memFree(IApusysMem * mem)
{
    UNUSED(mem);
    return false;
}

bool apusysEngine::memSync(IApusysMem * mem)
{
    UNUSED(mem);
    return false;
}

bool apusysEngine::memInvalidate(IApusysMem * mem)
{
    UNUSED(mem);
    return false;
}

IApusysMem * apusysEngine::memImport(int shareFd, unsigned int size)
{
    UNUSED(shareFd);
    UNUSED(size);
    return nullptr;
}

bool apusysEngine::memUnImport(IApusysMem * mem)
{
    UNUSED(mem);
    return false;
}

int apusysEngine::getDeviceNum(APUSYS_DEVICE_E type)
{
    UNUSED(type);
    return 0;
}

IApusysDev * apusysEngine::devAlloc(APUSYS_DEVICE_E deviceType)
{
    UNUSED(deviceType);
    return nullptr;
}

bool apusysEngine::devFree(IApusysDev * idev)
{
    UNUSED(idev);
    return false;
}

bool apusysEngine::setPower(APUSYS_DEVICE_E deviceType, unsigned int idx, unsigned int boostVal)
{
    UNUSED(deviceType);
    UNUSED(idx);
    UNUSED(boostVal);
    return false;
}

IApusysFirmware * apusysEngine::loadFirmware(APUSYS_DEVICE_E deviceType, unsigned int magic,
                                            unsigned int idx, std::string name, IApusysMem * fwBuf)
{
    UNUSED(deviceType);
    UNUSED(idx);
    UNUSED(magic);
    UNUSED(name);
    UNUSED(fwBuf);
    return nullptr;
}

bool apusysEngine::unloadFirmware(IApusysFirmware * hnd)
{
    UNUSED(hnd);
    return false;
}

//---------------------------------------------
IApusysEngine * IApusysEngine::createInstance(const char * userName)
{
    UNUSED(userName);
    LOG_ERR("LD2.0 not support libapusys\n");
    return nullptr;
}

bool IApusysEngine::deleteInstance(IApusysEngine * engine)
{
    UNUSED(engine);
    LOG_ERR("LD2.0 not support libapusys\n");
    return false;
}

