# Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. and/or its licensors.
# Without the prior written permission of MediaTek inc. and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.
#
# MediaTek Inc. (C) 2010. All rights reserved.
#
# BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
# THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
# RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
# AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
# NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
# SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
# SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
# THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
# THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
# CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
# SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
# STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
# CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
# AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
# OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
# MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
#
# The following software/firmware and/or related documentation ("MediaTek Software")
# have been modified by MediaTek Inc. All revisions are subject to any receiver's
# applicable license agreements with MediaTek Inc.

ifneq ($(filter $(TARGET_BUILD_VARIANT),eng userdebug),)

################################################################################
#
################################################################################

LOCAL_PATH := $(call my-dir)

################################################################################
#
################################################################################
include $(CLEAR_VARS)

#-----------------------------------------------------------
# libapusys/libsample header
LOCAL_HEADER_LIBRARIES += libapu_mdw_headers
LOCAL_HEADER_LIBRARIES += libapusys_edma_headers

#-----------------------------------------------------------
LOCAL_SRC_FILES := \
    main.cpp \
    reviserTest.cpp \
    reviserTestCmd.cpp \


#----------------
# libion => ion user space lib (Android standard)
# libion_mtk=> ion user space lib (mtk implementation)
#----------------
LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libc++ \
    libapu_mdw \
    libdmabufheap \
    libapusys_edma \

LOCAL_STATIC_LIBRARIES := \
    libapusys_sample \

LOCAL_WHOLE_STATIC_LIBRARIES := \

LOCAL_MODULE := reviser_test

LOCAL_PROPRIETARY_MODULE := true

LOCAL_MODULE_OWNER := mtk

LOCAL_PRELINK_MODULE := false

LOCAL_MULTILIB = 64
LOCAL_MODULE_STEM_64 = reviser_test64

#-----------------------------------------------------------
ifdef MTK_GENERIC_HAL
    ## layer decoupling 2.0 chips will go to this branch
    ## DO NOT USE platform macro in this branch
    ## consider to create a "common" folder beside the platform folder
    ## and implement single binary for all LD 2.0 chips in common folder

    include $(BUILD_EXECUTABLE)
else
    ## layer decoupling 1.0 chips will go to this branch
    ## platform macro is still supported here
    ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6885 mt6873 mt6853 mt6893 mt6877))
        ## behavior if building mt6853/mt6877/mt6885/mt6873/mt6893 ##
        include $(BUILD_EXECUTABLE)
    endif
endif
endif
