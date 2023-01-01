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

#pragma once

#define UNUSED(x) (void)(x)

extern int gLogLevel;

enum {
	APUSYS_LOG_BITMAP_INFO,
	APUSYS_LOG_BITMAP_FLOW,
	APUSYS_LOG_BITMAP_CMD,
	APUSYS_LOG_BITMAP_MEM,
	APUSYS_LOG_BITMAP_PERF,

	APUSYS_LOG_BITMAP_MAX,
};

#ifdef __ANDROID__
#undef LOG_TAG
#define LOG_TAG "apusys"
#include <utils/Trace.h>
#include <log/log.h>

#define PROPERTY_DEBUG_APUSYS_LOGLEVEL "debug.apusys.loglevel"

#define LOG_ERR_HELPER(x, ...)  ALOGE("%s: " x "%s", __FUNCTION__, __VA_ARGS__)
#define LOG_WARN_HELPER(x, ...) ALOGW("%s: " x "%s", __FUNCTION__, __VA_ARGS__)
#define LOG_INFO_HELPER(x, ...) ALOGI("%s: " x "%s", __FUNCTION__, __VA_ARGS__)
#define LOG_DEBUG_HELPER(mask, x, ...) ALOGD_IF(gLogLevel & (1 << mask), "%s/%d: " x "%s", __FUNCTION__, __LINE__, __VA_ARGS__)

#define LOG_ERR(...)    LOG_ERR_HELPER(__VA_ARGS__, "")
#define LOG_WARN(...)   LOG_WARN_HELPER(__VA_ARGS__, "")
#define LOG_INFO(...) \
	{ \
		if (gLogLevel & (1 << APUSYS_LOG_BITMAP_INFO)) \
			LOG_INFO_HELPER(__VA_ARGS__, ""); \
	}
#define LOG_DEBUG(...)  LOG_DEBUG_HELPER(APUSYS_LOG_BITMAP_FLOW, __VA_ARGS__, "")
#define CLOG_DEBUG(...) LOG_DEBUG_HELPER(APUSYS_LOG_BITMAP_CMD, __VA_ARGS__, "")
#define MLOG_DEBUG(...) LOG_DEBUG_HELPER(APUSYS_LOG_BITMAP_MEM, __VA_ARGS__, "")
#define PLOG_DEBUG(...) LOG_DEBUG_HELPER(APUSYS_LOG_BITMAP_CMD, __VA_ARGS__, "")
#define LOG_CON(...) LOG_INFO(__VA_ARGS__)

#else
#define PROPERTY_DEBUG_APUSYS_LOGLEVEL "DEBUG_APUSYS_LOGLEVEL"
#define LOG_PREFIX "[apusys]"
#define LOG_PRINT_HELPER(type, x, ...) printf(LOG_PREFIX"[%s]%s: " x "%s", type, __func__, __VA_ARGS__)
#define LOG_ERR(...)      LOG_PRINT_HELPER("error", __VA_ARGS__ ,"")
#define LOG_WARN(...)     LOG_PRINT_HELPER("warn", __VA_ARGS__ ,"")
#define LOG_INFO(...)     LOG_PRINT_HELPER("info", __VA_ARGS__ ,"")

#define LOG_DEBUG_HELPER(mask, x, ...) \
    { \
        if (gLogLevel & (1 << mask)) \
           printf(LOG_PREFIX"[debug]%s/%d: " x "%s", __func__, __LINE__, __VA_ARGS__); \
    }
#define LOG_DEBUG(...)  LOG_DEBUG_HELPER(APUSYS_LOG_BITMAP_FLOW, __VA_ARGS__, "")
#define CLOG_DEBUG(...) LOG_DEBUG_HELPER(APUSYS_LOG_BITMAP_CMD, __VA_ARGS__, "")
#define MLOG_DEBUG(...) LOG_DEBUG_HELPER(APUSYS_LOG_BITMAP_MEM, __VA_ARGS__, "")
#define PLOG_DEBUG(...) LOG_DEBUG_HELPER(APUSYS_LOG_BITMAP_CMD, __VA_ARGS__, "")
#define LOG_CON(...) LOG_INFO(__VA_ARGS__)

#endif
#define DEBUG_TAG LOG_DEBUG("%d\n",__LINE__)


void apusysTraceBegin(const char *info);
void apusysTraceEnd(void);
