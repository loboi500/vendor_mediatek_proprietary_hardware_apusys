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
#include <unistd.h>

#include "sampleEngine.h"
#include "sampleCmn.h"
#ifdef __ANDROID__
#include <cutils/properties.h>
#endif

#define SAMPLE_REQUEST_NAME_SIZE (32)
#define SAMPLE_DEFAULT_ALGOID (0x5A5A5A5A)
#define SAMPLE_DEFAULT_DELAYMS (20)

struct sampleRequest {
    char name[SAMPLE_REQUEST_NAME_SIZE];
    unsigned int algoId;
    unsigned int delayMs;

    unsigned int driverDone;
} __attribute__((__packed__));

//-----------------------------------
int gUtSampleLogLevel = 0;

static void getLogLevel()
{
#ifdef __ANDROID__
    char prop[100];

    memset(prop, 0, sizeof(prop));
    property_get(PROPERTY_DEBUG_APUSYS_LOGLEVEL, prop, "0");
    gUtSampleLogLevel = atoi(prop);
#else
    char *prop;

    prop = std::getenv(PROPERTY_DEBUG_APUSYS_LOGLEVEL);
    if (prop != nullptr)
        gUtSampleLogLevel = atoi(prop);
#endif

    LOG_DEBUG("debug loglevel = %d\n", gUtSampleLogLevel);

    return;
}

sampleCodeGen::sampleCodeGen()
{
    getLogLevel();
}

sampleCodeGen::~sampleCodeGen()
{
}

unsigned int sampleCodeGen::getSampleCmdSize()
{
    return sizeof(struct sampleRequest);
}

int sampleCodeGen::checkSampleCmdDone(void *cmdBuf)
{
    struct sampleRequest *req = (struct sampleRequest *)cmdBuf;

    if (!req->driverDone)
        return -EINVAL;

    req->driverDone = 0;
    return 0;
}

int sampleCodeGen::createSampleCmd(void *cmdBuf)
{
    struct sampleRequest *req = (struct sampleRequest *)cmdBuf;

    if (cmdBuf == nullptr)
        return -EINVAL;

    req->delayMs = SAMPLE_DEFAULT_DELAYMS;
    req->algoId = SAMPLE_DEFAULT_ALGOID;
    snprintf(req->name, sizeof(req->name) - 1, "sample test[%10d]", getpid());
    req->driverDone = 0;

    return 0;
}

int sampleCodeGen::setCmdProp(void *cmdBuf, enum sample_cmd_prop prop, uint64_t val)
{
    struct sampleRequest *req = (struct sampleRequest *)cmdBuf;

    switch (prop) {
        case SAMPLE_CMD_ALGOID:
            req->algoId = static_cast<unsigned int>(val);
            break;

        case SAMPLE_CMD_DELAYMS:
            req->delayMs = static_cast<unsigned int>(val);
            break;

        default:
            LOG_WARN("not support set prop(%d)\n", prop);
            break;
    }

    return 0;
}

int sampleCodeGen::destroySampleCmd(void *cmdBuf)
{
    struct sampleRequest *req = (struct sampleRequest *)cmdBuf;

    memset(req, 0, sizeof(*req));
    return 0;
}
