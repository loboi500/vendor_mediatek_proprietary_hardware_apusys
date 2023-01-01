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
#include <assert.h>

#include "sampleEngine.h"
#include "sampleCmn.h"
#ifdef __ANDROID__
#include <cutils/properties.h>
#endif

#define SAMPLE_REQUEST_NAME_SIZE 32

struct sampleRequest {
    char name[SAMPLE_REQUEST_NAME_SIZE];
    unsigned int algoId;
    unsigned int delayMs;

    unsigned char driverDone;
};

class sampleCmd;

class sampleEngine : public ISampleEngine {
private:
    std::vector<sampleCmd *> mRequestList;
    std::mutex mListMtx;

public:
    sampleEngine(const char * userName);
    ~sampleEngine();
    ISampleCmd * createSampleCmd();
    bool destroySampleCmd(ISampleCmd * cmd);
    ISampleCmd * getSampleCmd(int idx);
    bool runSampleCmd(ISampleCmd * cmd);
};

class sampleCmd : public ISampleCmd {
private:
    std::mutex mMtx;

    struct apusysCmdProp mProp; // for mApusysCmd
    IApusysMem * mSubCmdBuf;

    sampleEngine * mEngine;
    std::string mName;
    unsigned int mAlgoId;
    unsigned int mDelayMs;

public:
    sampleCmd(sampleEngine * engine);
    ~sampleCmd();
    bool setup(std::string & name, unsigned int algoId, unsigned int delayMs);
    bool setPriority(unsigned char priority);
    bool setProp(struct apusysCmdProp & prop);
    bool getProp(struct apusysCmdProp & prop);
    bool checkDone();
    IApusysMem * compile();
};


//-----------------------------------
int gUtSampleLogLevel = 0;

static void getLogLevel()
{
#ifdef __ANDROID__
    char prop[100];

    memset(prop, 0, sizeof(prop));
    property_get(PROPERTY_DEBUG_APUSYSUT_LOGLEVEL, prop, "0");
    gUtSampleLogLevel = atoi(prop);
#else
    char *prop;

    prop = std::getenv("PROPERTY_DEBUG_APUSYSUT_LOGLEVEL");
    if (prop != nullptr)
    {
        gUtSampleLogLevel = atoi(prop);
    }
#endif

    LOG_DEBUG("debug loglevel = %d\n", gUtSampleLogLevel);

    return;
}

sampleCmd::sampleCmd(sampleEngine * engine)
{
    struct sampleRequest * req = nullptr;

    assert(engine != nullptr);
    mSubCmdBuf = engine->memAlloc(sizeof(struct sampleRequest));
    assert(mSubCmdBuf != nullptr);

    req = (struct sampleRequest *)mSubCmdBuf->va;
    memset(req, 0, sizeof(struct sampleRequest));

    LOG_DEBUG("mSubCmdBuf = %p, (0x%llx/0x%d)\n", (void *)mSubCmdBuf, mSubCmdBuf->va, mSubCmdBuf->size);

    mProp.priority = APUSYS_PRIORITY_NORMAL;
    mProp.hardLimit= 0;
    mProp.softLimit = 0;
    mProp.powerSave = false;

    mName.clear();
    mAlgoId = 0;
    mDelayMs = 0;

    mEngine = engine;
}

sampleCmd::~sampleCmd()
{
    if (mSubCmdBuf != nullptr)
    {
        if(mEngine->memFree(mSubCmdBuf) == false)
        {
            LOG_ERR("fail to free subcmd buf\n");
        }
    }
}

bool sampleCmd::setup(std::string & name, unsigned int algoId, unsigned int delayMs)
{
    struct sampleRequest * req = (struct sampleRequest *)mSubCmdBuf->va;
    std::vector<int> dependency;
    unsigned int len = 0;

    if (req == nullptr)
    {
        LOG_ERR("no valid request\n");
        return false;
    }

    dependency.clear();

    len = sizeof(req->name) < strlen(name.c_str()) ? sizeof(req->name) : strlen(name.c_str());
    LOG_DEBUG("setup request(%s/0x%x/%u) len(%d)\n", name.c_str(), algoId, delayMs, len);
    len = len >= sizeof(req->name) ? sizeof(req->name)-1 : len;

    /* assign value */
    req->algoId = algoId;
    req->delayMs = delayMs;
    strncpy(req->name, name.c_str(), len);

    return true;
}

bool sampleCmd::setPriority(unsigned char priority)
{
    if(priority >= APUSYS_PRIORITY_MAX)
    {
        return false;
    }

    mProp.priority = priority;

    return true;
}

bool sampleCmd::setProp(struct apusysCmdProp & prop)
{
    if(prop.priority >= APUSYS_PRIORITY_MAX)
    {
        return false;
    }

    mProp = prop;

    return true;
}

bool sampleCmd::getProp(struct apusysCmdProp & prop)
{
    prop = mProp;
    return true;
}

bool sampleCmd::checkDone()
{
    struct sampleRequest * req = nullptr;

    if(mSubCmdBuf == nullptr)
    {
        LOG_ERR("invalid sub cmd buffer\n");
        return false;
    }

    req = (struct sampleRequest *)mSubCmdBuf->va;

    if (req->driverDone != 1)
    {
        LOG_ERR("driver dont execute this subcmd(%d)\n", req->driverDone);
        return false;
    }

    req->driverDone = 0;

    return true;
}

IApusysMem * sampleCmd::compile()
{
    struct sampleRequest * req = nullptr;

    if(mSubCmdBuf == nullptr)
    {
        LOG_ERR("invalid sub cmd buffer\n");
        return nullptr;
    }

    req = (struct sampleRequest *)mSubCmdBuf->va;
    LOG_DEBUG("==============================================================\n");
    LOG_DEBUG("| sample library getCmdBuf                                   |\n");
    LOG_DEBUG("-------------------------------------------------------------|\n");
    LOG_DEBUG("| name     = %-32s                |\n", req->name);
    LOG_DEBUG("| algo id  = 0x%-16x                              |\n", req->algoId);
    LOG_DEBUG("| delay ms = %-16u                                |\n", req->delayMs);
    LOG_DEBUG("| driver done(should be 0) = %-2d                              |\n", req->driverDone);
    LOG_DEBUG("==============================================================\n");

    /* always modify directly in setup() and constructor, return cmdbuf directly */
    return mSubCmdBuf;
}

//-----------------------------------
ISampleEngine::ISampleEngine(const char * userName):apusysEngine(userName)
{
    return;
}
//-----------------------------------

sampleEngine::sampleEngine(const char * userName):ISampleEngine(userName)
{
    mRequestList.clear();
    getLogLevel();

    return;
}

sampleEngine::~sampleEngine()
{
    std::vector<sampleCmd*>::iterator iter;
    DEBUG_TAG;

    /* free all memory */
    std::unique_lock<std::mutex> mutexLock(mListMtx);
    LOG_DEBUG("mRequestList size(%lu)\n", (unsigned long)mRequestList.size());
    for (iter = mRequestList.begin(); iter != mRequestList.end(); iter++)
    {
        delete(*iter);
    }

    mRequestList.clear();
    DEBUG_TAG;

    return;
}

ISampleCmd * sampleEngine::createSampleCmd()
{
    std::unique_lock<std::mutex> mutexLock(mListMtx);
    sampleCmd * cmd = new sampleCmd(this);

    if (cmd == nullptr)
    {
        LOG_ERR("allocate sample cmd fail\n");
        return nullptr;
    }

    mRequestList.push_back(cmd);
    LOG_DEBUG("subcmd = %p\n", (void *)cmd);

    return (ISampleCmd *)cmd;
}

bool sampleEngine::destroySampleCmd(ISampleCmd * ICmd)
{
    std::vector<sampleCmd *>::iterator iter;

    std::unique_lock<std::mutex> mutexLock(mListMtx);
    for (iter = mRequestList.begin(); iter != mRequestList.end(); iter++)
    {
        if (*iter == ICmd)
        {
            LOG_DEBUG("subcmd = %p\n", (void *)*iter);
            LOG_DEBUG("mRequestList size(%lu)\n", (unsigned long)mRequestList.size());
            delete (*iter);
            mRequestList.erase(iter);
            LOG_DEBUG("mRequestList size(%lu)\n", (unsigned long)mRequestList.size());
            return true;
        }
    }

    return false;
}

/*
 * contruct sample's subCmd and return
 * it's used for user program contruct big apusys cmd
 */
ISampleCmd * sampleEngine::getSampleCmd(int idx)
{
    if ((unsigned int)idx >= mRequestList.size())
    {
        LOG_ERR("wrong idx(%d/%d)\n", idx, (int)mRequestList.size());
        return nullptr;
    }

    return (ISampleCmd *)mRequestList.at(idx);
}

bool sampleEngine::runSampleCmd(ISampleCmd * ICmd)
{
    IApusysCmd * apusysCmd = nullptr;
    sampleCmd * cmd = (sampleCmd *)ICmd;
    std::vector<int> dependency;
    IApusysMem * cmdBuf;
    struct apusysCmdProp prop;

    /* check argument */
    if (cmd == nullptr)
    {
        LOG_ERR("invalid argument\n");
        return false;
    }

    /* init apusysCmd */
    apusysCmd = initCmd();
    if (apusysCmd == nullptr)
    {
        LOG_ERR("init apusys cmd fail\n");
        return false;
    }

    /* get property */
    if (cmd->getProp(prop) == false)
    {
        LOG_ERR("get prop from sample fail\n");
        return false;
    }

    /* set property */
    if (apusysCmd->setCmdProperty(prop) == false)
    {
        LOG_ERR("set prop to apusys cmd fail\n");
        return false;
    }

    /* get cmd buffer from sample command */
    cmd = (sampleCmd *)ICmd;
    cmdBuf = cmd->compile();
    if (cmdBuf == nullptr)
    {
        destroyCmd(apusysCmd);
        return false;
    }

    /* assign to apusys cmd */
    apusysCmd->addSubCmd(cmdBuf, APUSYS_DEVICE_SAMPLE, dependency, 0);

    /* run cmd */
    if(runCmd(apusysCmd) == false)
    {
        destroyCmd(apusysCmd);
        return false;
    }

    destroyCmd(apusysCmd);

    if (cmd->checkDone() == false)
    {
        LOG_ERR("sample cmd didn't be executed\n");
        return false;
    }

    return true;
}

ISampleEngine * ISampleEngine::createInstance(const char * userName)
{
    sampleEngine * engine = new sampleEngine(userName);

    return (ISampleEngine *)engine;
}

bool ISampleEngine::deleteInstance(ISampleEngine * engine)
{
    if (engine == nullptr)
    {
        LOG_ERR("invalid argument\n");
        return false;
    }

    delete (sampleEngine *)engine;
    return true;
}
