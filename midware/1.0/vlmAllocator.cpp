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
#include <vector>
#include <assert.h>
#include <mutex>

#include "apusysMem.h"
#include "apusys_drv.h"
#include "vlmAllocator.h"
#include "apusysCmn.h"


apusysVlmMem::apusysVlmMem()
{
    va = 0;
    iova = 0;
    offset = 0;
    size = 0;
    align = 0;

    type = APUSYS_USER_MEM_VLM;
}

apusysVlmMem::~apusysVlmMem()
{
}

int apusysVlmMem::getMemFd()
{
    return 0;
}

vlmAllocator::vlmAllocator(unsigned int start, unsigned int size)
{
    mStartAddr = start;
    mSize = size;
}

vlmAllocator::~vlmAllocator()
{
}

IApusysMem * vlmAllocator::alloc(unsigned int size, unsigned int align, unsigned int cache)
{
    apusysVlmMem *mem = nullptr;
    UNUSED(size);
    UNUSED(align);
    UNUSED(cache);

    if (mStartAddr == 0 || mSize == 0)
    {
        LOG_DEBUG("no vlm support\n");
        return nullptr;
    }

    mem = new apusysVlmMem;
    assert(mem != nullptr);

    mem->iova = mStartAddr;
    mem->size = mSize;
    mem->type = APUSYS_USER_MEM_VLM;

    mMemList.push_back(mem);

    return (IApusysMem *)mem;
}

bool vlmAllocator::free(IApusysMem * mem)
{
    apusysVlmMem * vlmMem = (apusysVlmMem *)mem;
    std::vector<IApusysMem *>::iterator iter;

    /* check argument */
    if (mem == nullptr)
    {
        LOG_ERR("invalid argument\n");
        return false;
    }

    /* delete from mem list */
    for (iter = mMemList.begin(); iter != mMemList.end(); iter++)
    {
        if (vlmMem == *iter)
        {
            break;
        }
    }
    if (iter == mMemList.end())
    {
        LOG_ERR("find ionMem from mem list fail(%p)\n", (void *)vlmMem);
        return false;
    }

    mMemList.erase(iter);
    delete vlmMem;

    return true;
}

bool vlmAllocator::flush(IApusysMem * mem)
{
    UNUSED(mem);

    LOG_WARN("vlm type memory don't support flush\n");
    return false;
}

bool vlmAllocator::invalidate(IApusysMem * mem)
{
    UNUSED(mem);

    LOG_WARN("vlm type memory don't support invalidate\n");
    return false;
}

IApusysMem * vlmAllocator::import(int shareFd, unsigned int size)
{
    UNUSED(shareFd);
    UNUSED(size);

    LOG_WARN("vlm type memory don't support import\n");
    return nullptr;
}

bool vlmAllocator::unimport(IApusysMem * mem)
{
    UNUSED(mem);

    LOG_WARN("vlm type memory don't support unimport\n");
    return false;
}

