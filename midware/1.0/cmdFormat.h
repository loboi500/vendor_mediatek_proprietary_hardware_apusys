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


#ifndef __APUSYS_CMD_FORMAT_H__
#define __APUSYS_CMD_FORMAT_H__

#define APUSYS_MAGIC_NUMBER 0x3d2070ece309c231
#define APUSYS_CMD_VERSION 0x1

#define VALUE_SUBGRAPH_PACK_ID_NONE      0
#define VALUE_SUBGRAPH_CTX_ID_NONE       0
#define VALUE_SUBGRAPH_BOOST_NONE       0xFF

#define TYPE_SUBGRAPH_SCOFS_ELEMENT unsigned int
#define TYPE_CMD_SUCCESSOR_ELEMENT unsigned int
#define TYPE_CMD_PREDECCESSOR_CMNT_ELEMENT unsigned int

#define SIZE_SUBGRAPH_SCOFS_ELEMENT \
	sizeof(TYPE_SUBGRAPH_SCOFS_ELEMENT)
#define SIZE_CMD_SUCCESSOR_ELEMENT \
	sizeof(TYPE_CMD_SUCCESSOR_ELEMENT)
#define SIZE_CMD_PREDECCESSOR_CMNT_ELEMENT \
	sizeof(TYPE_CMD_PREDECCESSOR_CMNT_ELEMENT)

/* flag bitmap of apusys cmd */
enum {
	CMD_FLAG_BITMAP_POWERSAVE = 0,
	CMD_FLAG_BITMAP_FENCE = 1,
	CMD_FLAG_BITMAP_FORCEDUAL = 62,
};

/* fd flag of codebuf info offset */
enum {
	SUBGRAPH_CODEBUF_INFO_BIT_FD = 31,
};

struct apusys_cmd_hdr {
	unsigned long long magic;
	unsigned long long uid;
	unsigned char version;
	unsigned char priority;
	unsigned short hard_limit;
	unsigned short soft_limit;
	unsigned short usr_pid;
	unsigned long long flag_bitmap;
	unsigned int num_sc;
	unsigned int ofs_scr_list;     // successor list offset
	unsigned int ofs_pdr_cnt_list; // predecessor count list offset
	unsigned int scofs_list_entry; // subcmd offset's list offset
} __attribute__((__packed__));

struct apusys_cmd_hdr_fence {
	unsigned long long fd;
	unsigned int total_time;
	unsigned int status;
} __attribute__((__packed__));

struct apusys_sc_hdr_cmn {
	unsigned int dev_type;
	unsigned int driver_time;
	unsigned int ip_time;
	unsigned int suggest_time;
	unsigned int bandwidth;
	unsigned int tcm_usage;
	unsigned char tcm_force;
	unsigned char boost_val;
	unsigned char pack_id;
	unsigned char reserved;
	unsigned int mem_ctx;
	unsigned int cb_info_size; // codebuf info size
	unsigned int ofs_cb_info;  // codebuf info offset
} __attribute__((__packed__));

struct apusys_sc_hdr_mdla {
	unsigned int ofs_cb_info_dual0;
	unsigned int ofs_cb_info_dual1;
	unsigned int ofs_pmu_info;
} __attribute__((__packed__));

#endif
