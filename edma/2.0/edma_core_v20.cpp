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

int edmav20_queryDSize(unsigned char type, st_edmaUnifyInfo *shapeInfo)
{
    EDMA_LOG_NONE("%s type = %d, info_type = %d \n", __func__, type, shapeInfo->info_type);
    return sizeof(st_edmaDesc20);
}

int edmav20_queryDNum(unsigned char type, st_edmaUnifyInfo *shapeInfo)
{
    EDMA_LOG_NONE("%s type = %d, info_type = %d \n", __func__, type, shapeInfo->info_type);
    return 1; //not support type
}

int edmav20_querySliceDSize(st_edmaUnifyInfo *shapeInfo)
{
    EDMA_LOG_WARN("%s not supported for info_type = %d\n", __func__, shapeInfo->info_type);
    return 0;  // not supported for edma 2.0
}

//descriptor size per info
int edmav20_queryUFBCDSize(st_edmaUnifyInfo *shapeInfo)
{
    EDMA_LOG_NONE("%s info_type = %d \n", __func__, shapeInfo->info_type);
    return sizeof(st_edmaDesc20);
}

/*for edma 2.0*/
int fillDescV20(st_edmaUnifyInfo *pInfo, void *edma_desc)
{
    st_edmaDesc20 *pDesc = (st_edmaDesc20 *)edma_desc;
    st_edma_InputShape *shape = &pInfo->st_info.infoShape;


    memset(pDesc, 0, sizeof(st_edmaDesc20));

    EDMA_LOG_INFO("fillDescV0 st_edmaDesc20 size = %zu\n", sizeof(st_edmaDesc20));
    EDMA_LOG_INFO("shape inFormat = %d, outFormat = %d\n", shape->inFormat, shape->outFormat);

    pDesc->DespDmaAruser = 0x2;
    pDesc->DespDmaAwuser = 0x2;
    pDesc->DespIntEnable = 1;

    pDesc->DespSrcAddr0 = shape->inBuf_addr;
    pDesc->DespDstAddr0 = shape->outBuf_addr;

    pDesc->DespSrcXSizeM1 = shape->size_x-1; /*important !!! 2.0 != 3.0*/

    pDesc->DespDstXSizeM1 = shape->size_x-1;

    if (shape->inFormat == EDMA_FMT_YUV422_8B &&
        shape->outFormat == EDMA_FMT_ARGB_8)
        pDesc->DespDstXSizeM1 = shape->size_x*2-1;

    pDesc->DespSrcYSizeM1 = shape->size_y-1;
    pDesc->DespDstYSizeM1 = shape->size_y-1;

    pDesc->DespSrcZSizeM1 = shape->size_z-1;  /*important !!! 2.0 != 3.0*/

    pDesc->DespSrcXStride0 = shape->size_x;
    pDesc->DespSrcYStride0 = shape->size_y;


    pDesc->DespDstXStride0 = shape->dst_stride_x;
    pDesc->DespDstYStride0 = shape->dst_stride_y;

    if (shape->inFormat < 20) {
        pDesc->DespInFormat = shape->inFormat;
    } else {
        pDesc->DespRawEnable = 1;
        pDesc->DespInFormat = shape->inFormat - 20;
    }

    pDesc->DespOutFormat = shape->outFormat;

    return 1;

}


/*for edma 2.0*/

int fillDescDataV20(st_edmaUnifyInfo *pInfo, void *edma_desc)
{
    st_edmaDesc20 *pDesc = (st_edmaDesc20 *)edma_desc;

    st_edma_InfoData *pIdata = &pInfo->st_info.infoData;
    unsigned int src_size_x = 1;         // unit: byte
    // ui_src_size_x <=65535
    unsigned int src_size_y = 1, src_size_z = 1;         // unit: line, <=65535

    EDMA_LOG_INFO("fillDescData [edma20] copy size = %d\n", pIdata->copy_size);

    EDMA_LOG_INFO("shape->inBuf_addr = 0x%x\n", pIdata->inBuf_addr);
    EDMA_LOG_INFO("shape->outBuf_addr = 0x%x\n", pIdata->outBuf_addr);


    if (pIdata->copy_size > 64*1024) {
        if (pIdata->copy_size > 128*1024*1024) {
            EDMA_LOG_ERR("pIdata->copy_size = %d not support\n", pIdata->copy_size);
            return -1;
        }
        if (pIdata->copy_size % 1024 == 0) {
            src_size_x = 1024;
            src_size_y = pIdata->copy_size/1024;
        } else {
            EDMA_LOG_ERR("pIdata->copy_size = %d not support\n", pIdata->copy_size);
            return -1;
        }
    } else {
        src_size_x = pIdata->copy_size;
        src_size_y = 1;
    }

    memset(pDesc, 0, sizeof(st_edmaDesc20));

    pDesc->DespDmaAruser = 0x2;
    pDesc->DespDmaAwuser = 0x2;
    pDesc->DespIntEnable = 1;

    pDesc->DespSrcAddr0 = pIdata->inBuf_addr;
    pDesc->DespDstAddr0 = pIdata->outBuf_addr;

    if (pIdata->outFormat == EDMA_FMT_FILL) {
        EDMA_LOG_INFO("fillDescData fill value = %d\n", pIdata->fillValue);
        pDesc->DespOutFillMode = 1;
        //pIdata->fillValue; edma20 only support set fill value @ ctrl register 0x18
    }




    pDesc->DespSrcXSizeM1 = src_size_x-1; /*important !!! 2.0 != 3.0*/

    pDesc->DespDstXSizeM1 = src_size_x-1;

    pDesc->DespSrcYSizeM1 = src_size_y-1;
    pDesc->DespDstYSizeM1 = src_size_y-1;

    pDesc->DespSrcZSizeM1 = src_size_z-1;  /*important !!! 2.0 != 3.0*/

    pDesc->DespSrcXStride0 = src_size_x;
    pDesc->DespSrcYStride0 = src_size_y; //don't care if z == 1


    pDesc->DespDstXStride0 = src_size_x;
    pDesc->DespDstYStride0 = src_size_y; //don't care if z == 1

    return 0;
}


int fillDescUFBCV20(st_edmaUnifyInfo *pInfo, void *edma_desc, void *headDesc)
{
    st_edmaDesc20 *pDesc = (st_edmaDesc20 *)edma_desc;
    st_edma_InputShape *shape = &pInfo->st_info.infoShape;
    __u8 *pHead = (__u8 *)headDesc;
    __u8 *pCurrDesc = (__u8 *)edma_desc;
    __u32 descDiff = (__u32)(pCurrDesc - pHead);

    memset(pDesc, 0, sizeof(st_edmaDesc20));

    EDMA_LOG_INFO("shape->inBuf_addr = 0x%x\n", shape->inBuf_addr);
    EDMA_LOG_INFO("shape->outBuf_addr = 0x%x\n", shape->outBuf_addr);
    EDMA_LOG_INFO("shape->size_x = 0x%x\n", shape->size_x);
    EDMA_LOG_INFO("shape->size_y = 0x%x\n", shape->size_y);

    EDMA_LOG_INFO("pInfo->info_type = %d\n", pInfo->info_type);
    EDMA_LOG_INFO("edma_desc addr = %p\n", edma_desc);

    pDesc->DespDmaAruser = 0x2;
    pDesc->DespDmaAwuser = 0x2;
    pDesc->DespIntEnable = 1;

    pDesc->DespSrcAddr0 = shape->inBuf_addr;
    //encode: tile image src buffer
    //
    pDesc->DespDstAddr0 = shape->outBuf_addr;
    //encode: compressed big image buffer
    pDesc->DespParamA |= (0x1 << 10); //payload alignment 256 byte
    //pDesc->DespParamM = (shape->size_x*shape->size_y)/16; /* header size */
    shape->src_addr_offset = descDiff + 0x2C;
    shape->dst_addr_offset = descDiff + 0x34;

    if (shape->uc_desc_type == EDMA_UFBC_ENCODE) {

        pDesc->UfbcYuvTransform = 1; //endbale yuv trans in AFBC encoder
        pDesc->DespInFormat = EDMA_FMT_ARGB_8;
        pDesc->DespOutFormat = 0xE; //ARGB bitstream
        //pDesc->DespCmprsDstPxl = shape->size_x; //pixel number in a tile = width pixels
        pDesc->DespCmprsDstPxl = shape->cropShape.x_size; //pixel number in a tile = width pixels

        pDesc->DespSrcXSizeM1 = shape->cropShape.x_size*4-1; /*important !!! 2.0 != 3.0*/
        pDesc->DespSrcYSizeM1 = shape->cropShape.y_size-1;
        pDesc->DespSrcZSizeM1 = 0;

        pDesc->DespSrcXStride0 = shape->cropShape.x_size*4;
        pDesc->DespSrcYStride0 = 0; //don't care

        // 0x18: APU_EDMA3_DESP_18
        pDesc->DespDstXSizeM1 = 0; /*important !!! 2.0 != 3.0*/
        pDesc->DespDstYSizeM1 = shape->cropShape.y_size-1;

        pDesc->DespDstXStridePxl = ((shape->size_x-1)/32 + 1)*32; //pixel number in a frame = width pixels
        pDesc->DespDstYStridePxl = ((shape->size_y-1)/8 + 1)*8; //alignment 8

        pDesc->DespCmprsDstPxl = ((shape->cropShape.x_size-1)/32 + 1)*32;

        pDesc->DespDstXOffsetM1 = shape->cropShape.x_offset;
        pDesc->DespDstYOffsetM1 = shape->cropShape.y_offset;


        pDesc->DespParamM = (pDesc->DespDstXStridePxl*pDesc->DespDstYStridePxl)/16; /* header size */

    } else if (shape->uc_desc_type == EDMA_UFBC_DECODE) {
        pDesc->DespInSwap = 1;  // B is at lower bit position, R is at higher bit position.
        pDesc->DespInFormat = 0xE;
        pDesc->DespOutFormat = EDMA_FMT_ARGB_8;
        pDesc->DespCmprsSrcPxl = shape->cropShape.x_size; //pixel number in a tile  = width pixels
        /* In/Out Info  */
        pDesc->DespDstXSizeM1 = shape->cropShape.x_size*4-1; /*important !!! 2.0 != 3.0*/
        pDesc->DespDstYSizeM1 = shape->cropShape.y_size-1;
        //pDesc->DespDstZSizeM1 = shape->size_z-1;

        pDesc->DespDstXStride0 = shape->cropShape.x_size*4;
        pDesc->DespDstYStride0 = 0;

        // 0x18: APU_EDMA3_DESP_18
        pDesc->DespSrcXSizeM1 = 0; /*important !!! 2.0 != 3.0*/
        pDesc->DespSrcYSizeM1 = shape->cropShape.y_size-1;


        pDesc->DespSrcXStridePxl = ((shape->size_x-1)/32 + 1)*32; //pixel number in a frame = width pixels
        pDesc->DespSrcYStridePxl = ((shape->size_y-1)/8 + 1)*8; //alignment 8

        pDesc->DespCmprsSrcPxl = ((shape->cropShape.x_size-1)/32 + 1)*32;

        pDesc->DespParamM = (pDesc->DespSrcXStridePxl*pDesc->DespSrcYStridePxl)/16; /* header size */

        pDesc->DespSrcXOffsetM1 = shape->cropShape.x_offset;
        pDesc->DespSrcYOffsetM1 = shape->cropShape.y_offset;

    }

    return 0;
}


