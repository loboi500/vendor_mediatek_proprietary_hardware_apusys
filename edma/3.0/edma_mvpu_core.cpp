#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>

#include "edma_cmn.h"

#include "edma_info.h"
#include "edma_core_hw.h"

EDMA_INFO_TYPE checkMVPUDescrpType(st_edmaUnifyInfo *shapeInfo)
{
    //EDMA_LOG_INFO("Original type = %d\n", shapeInfo->info_type);

    EDMA_INFO_TYPE type_real = EDMA_DESC_INFO_TYPE_0;
    switch(shapeInfo->info_type) {
    case EDMA_DESC_INFO_TYPE_0:
        type_real = EDMA_DESC_INFO_TYPE_0;
        break;
    case EDMA_DESC_INFO_TYPE_1:
        type_real = EDMA_DESC_INFO_TYPE_1;
        break;
    case EDMA_DESC_INFO_TYPE_5:
    {
        st_edmaDescInfoType5 *pst_curr_old_desc_info = (st_edmaDescInfoType5 *) &shapeInfo->st_info.info5;
        if (pst_curr_old_desc_info->uc_fmt == EDMA_FMT_RGB_8_TO_RGBA_8)
            type_real = EDMA_DESC_INFO_TYPE_1;
        else
            type_real = EDMA_DESC_INFO_TYPE_5;
        break;
    }
    case EDMA_DESC_INFO_ROTATE:
        type_real = EDMA_DESC_INFO_ROTATE;
        break;
    case EDMA_DESC_INFO_SPLITN:
        type_real = EDMA_DESC_INFO_SPLITN;
        break;
    case EDMA_DESC_INFO_MERGEN:
        type_real = EDMA_DESC_INFO_MERGEN;
        break;
    case EDMA_DESC_INFO_UNPACK:
        type_real = EDMA_DESC_INFO_UNPACK;
        break;
    case EDMA_DESC_INFO_SWAP2:
        type_real = EDMA_DESC_INFO_SWAP2;
        break;
    case EDMA_DESC_INFO_565TO888:
        type_real = EDMA_DESC_INFO_565TO888;
        break;
    case EDMA_DESC_INFO_PACK:
        type_real = EDMA_DESC_INFO_PACK;
        break;
    default:
        type_real = shapeInfo->info_type;
        break;
    }

    return type_real;
}


unsigned int edma_queryMVPUTaskNum(st_edmaUnifyInfo *shapeInfo)
{
    if (shapeInfo != NULL)
        return 1;
    else
        return 0;
}


unsigned int edma_queryMVPUDNum(st_edmaUnifyInfo *shapeInfo)
{
    unsigned int task_num = 0;
    unsigned int dnum = 0;
    task_num = (edma_queryMVPUTaskNum(shapeInfo))&0xFFFFF; // max 20bit

    switch (shapeInfo->info_type){
    case EDMA_DESC_INFO_TYPE_0:
    case EDMA_DESC_INFO_TYPE_1:
    case EDMA_DESC_INFO_TYPE_5:
        dnum = (unsigned int)(task_num);
        break;
    case EDMA_DESC_INFO_ROTATE:
        dnum = (unsigned int)(task_num*3);
        break;
    case EDMA_DESC_INFO_SPLITN:
        dnum = (unsigned int)(task_num*3);
        break;
    case EDMA_DESC_INFO_MERGEN:
        dnum = (unsigned int)(task_num*3);
        break;
    case EDMA_DESC_INFO_UNPACK:
        dnum = (unsigned int)(task_num);
        break;
    case EDMA_DESC_INFO_SWAP2:
        dnum = (unsigned int)(task_num);
        break;
    case EDMA_DESC_INFO_565TO888:
        dnum = (unsigned int)(task_num);
        break;
    case EDMA_DESC_INFO_PACK:
        dnum = (unsigned int)(task_num);
        break;
    default:
        dnum = (unsigned int)(-1); //not support type
        break;
    }

    return (unsigned int)dnum;
}

unsigned int edma_queryMVPURPNum(st_edmaUnifyInfo *shapeInfo)
{
    unsigned int task_num = 0;
    unsigned int rpnum = 0;
    task_num = (edma_queryMVPUTaskNum(shapeInfo))&0xFFFFF; // max 20bit

    switch (shapeInfo->info_type){
    case EDMA_DESC_INFO_TYPE_0:
    case EDMA_DESC_INFO_TYPE_1:
        rpnum = (unsigned int)(task_num*2);
        break;
    case EDMA_DESC_INFO_TYPE_5:
        rpnum = (unsigned int)(task_num*4);
        break;
    case EDMA_DESC_INFO_ROTATE:
    {
        rpnum = (unsigned int)(task_num*2);
        break;
        /*
        st_edmaDescInfoRotate *pst_curr_old_desc_info = &shapeInfo->st_info.infoRotate;
        unsigned int fmt = 0;
        fmt = pst_curr_old_desc_info->uc_fmt;

        if (fmt == EDMA_FMT_ROTATE_90 || fmt == EDMA_FMT_ROTATE_270)
            rpnum = task_num*6;
        else if (fmt == EDMA_FMT_ROTATE_180)
            rpnum = task_num*12;
        else
            EDMA_LOG_INFO("Wrong fmt %d\n", fmt);
        break;
        */
    }
    case EDMA_DESC_INFO_SPLITN:
        rpnum = (unsigned int)(task_num*5);
        break;
    case EDMA_DESC_INFO_MERGEN:
        rpnum = (unsigned int)(task_num*5);
        break;
    case EDMA_DESC_INFO_UNPACK:
        rpnum = (unsigned int)(task_num*2);
        break;
    case EDMA_DESC_INFO_SWAP2:
    case EDMA_DESC_INFO_565TO888:
    case EDMA_DESC_INFO_PACK:
        rpnum = (unsigned int)(task_num*2);
        break;
    default:
        rpnum = (unsigned int)(-1); //not support type
        break;
    }

    //EDMA_LOG_INFO("edma_queryMVPURPNum, task_num %d, rpnum %d\n", task_num, rpnum);
    return (unsigned int)rpnum;
}

//descriptor size per info
unsigned int edma_queryMVPUDSize(st_edmaUnifyInfo *shapeInfo)
{
    EDMA_INFO_TYPE type_real = EDMA_DESC_INFO_TYPE_0;
    unsigned int dsize = 0;
    unsigned int task_num = 1;

    task_num = (edma_queryMVPUTaskNum(shapeInfo))&0xFFFFF; // max 20bit
    EDMA_LOG_INFO("edma_queryMVPUDSize task_num %d\n", task_num);

    type_real = checkMVPUDescrpType(shapeInfo);
    switch (type_real){
    case EDMA_DESC_INFO_TYPE_0:
        dsize = (unsigned int)((unsigned int)sizeof(st_edmaDescType0) * task_num);
        break;
    case EDMA_DESC_INFO_TYPE_1:
        dsize = (unsigned int)((unsigned int)sizeof(st_edmaDescType1) * task_num);
        break;
    case EDMA_DESC_INFO_TYPE_5:
        dsize = (unsigned int)((unsigned int)sizeof(st_edmaDescType5) * task_num);
        break;
    case EDMA_DESC_INFO_ROTATE:
    {
        st_edmaDescInfoRotate *pst_curr_old_desc_info = &shapeInfo->st_info.infoRotate;
        unsigned int fmt = 0;
        fmt = pst_curr_old_desc_info->uc_fmt;

        dsize = (unsigned int)((unsigned int)sizeof(st_edmaDescType0) * task_num * 2);
        if (fmt == EDMA_FMT_ROTATE_90 || fmt == EDMA_FMT_ROTATE_270)
            dsize += (unsigned int)((unsigned int)sizeof(st_edmaDescType0) * task_num);
        else if (fmt == EDMA_FMT_ROTATE_180)
            dsize += (unsigned int)((unsigned int)sizeof(st_edmaDescType15) * task_num);
        else
            EDMA_LOG_INFO("Wrong fmt %d\n", fmt);
        break;
    }
    case EDMA_DESC_INFO_SPLITN:
        dsize = (unsigned int)((unsigned int)sizeof(st_edmaDescType0) * task_num);
        dsize += (unsigned int)((unsigned int)sizeof(st_edmaDescType15) * task_num * 2);
        break;
    case EDMA_DESC_INFO_MERGEN:
        dsize = (unsigned int)((unsigned int)sizeof(st_edmaDescType0) * task_num);
        dsize += (unsigned int)((unsigned int)sizeof(st_edmaDescType15) * task_num * 2);
        break;
    case EDMA_DESC_INFO_UNPACK:
        dsize = (unsigned int)((unsigned int)sizeof(st_edmaDescType0) * task_num);
        break;
    case EDMA_DESC_INFO_SWAP2:
        dsize = (unsigned int)((unsigned int)sizeof(st_edmaDescType15) * task_num);
        break;
    case EDMA_DESC_INFO_565TO888:
        dsize = (unsigned int)((unsigned int)sizeof(st_edmaDescType15) * task_num);
        break;
    case EDMA_DESC_INFO_PACK:
        dsize = (unsigned int)((unsigned int)sizeof(st_edmaDescType15) * task_num);
        break;
    default:
        dsize = (unsigned int)(-1); //not support type
        break;
    }

    EDMA_LOG_INFO("edma_queryMVPUDSize TYPE %d, task_num %d total dsize: 0x%x\n", type_real, task_num, dsize);
    return (unsigned int)dsize;
}

void edma_MVPUtransTaskInfo(st_edmaTaskInfo *pst_old_edma_task_info, st_edmaTaskInfo *pst_new_edma_task_info)
{
    // check input emda task info
    if ((pst_old_edma_task_info==NULL) || (pst_old_edma_task_info->info_num==0) ||
        (pst_old_edma_task_info->edma_info==NULL))
    {
        pst_new_edma_task_info->info_num = 0;
        pst_new_edma_task_info->edma_info = NULL;
        return;
    }
    memcpy(pst_new_edma_task_info, pst_old_edma_task_info, sizeof(st_edmaTaskInfo)); //remve old 

}

