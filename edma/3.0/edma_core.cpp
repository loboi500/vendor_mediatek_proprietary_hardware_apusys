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


typedef enum {
    EDMA_DESC_TYPE0 = 0, //direct copy mode
    EDMA_DESC_TYPE1 = 1,
    EDMA_DESC_TYPE4 = 4,
    EDMA_DESC_TYPE5 = 5,
    EDMA_DESC_TYPE6 = 6,
    EDMA_DESC_TYPE12 = 12,
    EDMA_DESC_TYPE15 = 15,
}EDMA_DESC_T;


inline bool Check3to4Constraint(st_edma_info_nn *InfoNN) {
    if (InfoNN->src_type_size  != 1) return false;
    if (InfoNN->shape_c  != 3) return false;
    // Input is user-provided, so it is contigious with no channel padding.

    return (InfoNN->src_stride_c == 3 && InfoNN->dst_stride_c == 4);
}

inline bool Check4to3Constraint(st_edma_info_nn *InfoNN) {
    if (InfoNN->src_type_size != 1) return false;
    if (InfoNN->shape_c != 3) return false;
    return (InfoNN->src_stride_c == 4 && InfoNN->dst_stride_c == 3);
}

inline bool Check1to4Constraint(st_edma_info_nn *InfoNN) {
    // YUV420 -> RGBA mode has hardware constraints:
    // 1. DespSrcXSizeM1 + 1 must be even -> Shape W must be even.
    // 2. 1 <= DespSrcXSizeM1 + 1 <= 1024 -> 1 <= Shape W <= 1024
    // 3. DespSrcYSizeM1 + 1 must be even -> Shape N*H must be even.
    // 4. DespSrcXStride0 % 16 == 0 -> Shape W must be 16 align.
    if (InfoNN->src_type_size  != 1) return false;
    if (InfoNN->shape_c != 1) return false;
    if (InfoNN->shape_w % 16 != 0 || InfoNN->shape_w > 1024) return false;
    if ((InfoNN->shape_n * InfoNN->shape_h) % 2 != 0) return false;
    // Input is user-provided, so it is contigious with no channel padding.
    EDMA_LOG_INFO("Check1to4Constraint src_stride_c = %d,dst_stride_c = %d \n",
                  InfoNN->src_stride_c, InfoNN->dst_stride_c);

    return (InfoNN->src_stride_c == 1 && InfoNN->dst_stride_c == 4);
}

inline bool Check4to1Constraint(st_edma_info_nn *InfoNN) {
    // RGBA -> YUV420 mode has hardware constraints:
    // 1. DespDstXSizeM1 + 1 must be even -> Shape W must be even.
    // 2. 6 <= DespDstXSizeM1 + 1 <= 1024 -> 6 <= Shape W <= 1024
    // 3. DespDstYSizeM1 + 1 must be even -> Shape N*H must be even.
    // 4. DespDstXStride0 % 16 == 0 -> Shape W must be 16 align.
    if (InfoNN->src_type_size  != 1) return false;
    if (InfoNN->shape_c  != 1) return false;
    // sr_v2_360_640_fake.tflite output shape W (1920) is out of constraint 1024 using
    // RGBA_8 -> YUV420_8 mode. But DE said 'When DespDstXSizeM1 + 1 > 1024,
    // only the UV plane we do not care of will not bit-true.' Therefore,
    // we relax the constraints.
    if (InfoNN->shape_w % 16 != 0) return false;
    if ((InfoNN->shape_n * InfoNN->shape_h) % 2 != 0) return false;
    return (InfoNN->src_stride_c == 4 && InfoNN->dst_stride_c == 1);
}

EDMA_DESC_T checkDescrpType(st_edmaUnifyInfo *shapeInfo)
{

    EDMA_LOG_INFO("shapeInfo->info_type = %d\n", shapeInfo->info_type);

    if (shapeInfo->info_type == EDMA_INFO_GENERAL) {
        st_edma_InputShape *pShape = (st_edma_InputShape *) &shapeInfo->st_info;

        EDMA_LOG_INFO("pShape->inFormat = %d\n", pShape->inFormat);
        EDMA_LOG_INFO("pShape->outFormat = %d\n", pShape->outFormat);

        if ((pShape->inFormat == EDMA_FMT_YUV422_8B) && (pShape->outFormat == EDMA_FMT_ARGB_8))
            return EDMA_DESC_TYPE5;
        else if (pShape->outFormat == EDMA_FMT_ROTATE90 || pShape->outFormat == EDMA_FMT_ROTATE180 ||
                 pShape->outFormat == EDMA_FMT_ROTATE270)
            return EDMA_DESC_TYPE15;
        else if (pShape->outFormat == EDMA_FMT_UVResz2X || pShape->inFormat == EDMA_FMT_UVResz2X)
            return EDMA_DESC_TYPE15;

        else
            return EDMA_DESC_TYPE0;
    } else if (shapeInfo->info_type == EDMA_INFO_NN) {
        st_edma_info_nn *pInfoNN = &shapeInfo->st_info.infoNN;


        switch (pInfoNN->inFormat ) {
        case EDMA_FMT_FP32:
        case EDMA_FMT_FP16:
        case EDMA_FMT_I8:
            return EDMA_DESC_TYPE0;
        case EDMA_FMT_DEPTHTOSPACE:
        case EDMA_FMT_DEPTHTOSPACE_FAST:
        case EDMA_FMT_S2D_FAST:            
            return EDMA_DESC_TYPE15;
        default:
            break;
        }

        if (Check3to4Constraint(pInfoNN)){
            EDMA_LOG_INFO("Check3to4Constraint return type1\n");
            return EDMA_DESC_TYPE1;
        }

        if (Check1to4Constraint(pInfoNN)){
            EDMA_LOG_INFO("Check1to4Constraint return type5\n");
            return EDMA_DESC_TYPE5;
        }
        if (Check4to3Constraint(pInfoNN)){
            EDMA_LOG_INFO("Check4to3Constraint return type15\n");
            return EDMA_DESC_TYPE15;
        }

        return EDMA_DESC_TYPE0;
    }
    return EDMA_DESC_TYPE0;
}

int edma_rotate_descNum(st_edmaUnifyInfo *pInfo)
{
    st_edma_InputShape *shape = &pInfo->st_info.infoShape;

    uint32_t src_w = shape->size_y;
    uint32_t src_h = shape->size_y;
    uint32_t tile_w = 64;
    uint32_t tile_h = 64;

    uint32_t x_tile_num;
    uint32_t y_tile_num;

    if (shape->inFormat == EDMA_FMT_ARGB_8) {
        tile_w = 64;
        tile_h = 64;
    } else if (shape->inFormat == EDMA_FMT_YUV422_8B) {
        tile_w = 128;
        tile_h = 64;
    }

    x_tile_num = (src_w - 1)/tile_w + 1;
    y_tile_num = (src_h - 1)/tile_h + 1;

    printf("edma_rotate_descNum = %d\r\n",  x_tile_num*y_tile_num);
    return  x_tile_num*y_tile_num;
}

int edma_queryTileNum(unsigned char type, st_edmaUnifyInfo *shapeInfo)
{
    st_edma_InputShape *pShape = (st_edma_InputShape *) &shapeInfo->st_info;

    if (type == EDMA_INFO_GENERAL) {
        if (pShape->outFormat == EDMA_FMT_UVResz2X || pShape->inFormat == EDMA_FMT_UVResz2X) {
          if ((pShape->tileNum == 0) && (pShape->size_x > 0))
              return ((pShape->size_x-1)/1024) + 1;
          else
              return pShape->tileNum;
        }
    }

    return 1;
}

int edma_queryDSize(unsigned char type, st_edmaUnifyInfo *shapeInfo)
{

    if (type == EDMA_INFO_GENERAL) {
        if (checkDescrpType(shapeInfo) == EDMA_DESC_TYPE5)
            return  sizeof(st_edmaDescType5);
        else if (checkDescrpType(shapeInfo) == EDMA_DESC_TYPE15) {
            st_edma_InputShape *pShape = (st_edma_InputShape *) &shapeInfo->st_info;
            if (pShape->outFormat == EDMA_FMT_ROTATE90 || pShape->outFormat == EDMA_FMT_ROTATE180 ||
                             pShape->outFormat == EDMA_FMT_ROTATE270)
            return  edma_rotate_descNum(shapeInfo)*(sizeof(st_edmaDescType0) + 256);
            else if (pShape->outFormat == EDMA_FMT_UVResz2X || pShape->inFormat == EDMA_FMT_UVResz2X) {
                EDMA_LOG_INFO("EDMA_FMT_UVResz2X size\n");
                return edma_queryTileNum(type, shapeInfo) * sizeof(st_edmaDescType15);
            }
        else
              return sizeof(st_edmaDescType15);
        } else
            return sizeof(st_edmaDescType0);
    } else if (type == EDMA_INFO_NN){
        const auto type = checkDescrpType(shapeInfo);

        st_edma_info_nn *pInfoNN = &shapeInfo->st_info.infoNN;

        if (pInfoNN->inFormat == EDMA_FMT_DEPTHTOSPACE ) {
            return pInfoNN->shape_h *sizeof(st_edmaDescType15);
        }

        switch (type) {
        case EDMA_DESC_TYPE0:
            return sizeof(st_edmaDescType0);
        case EDMA_DESC_TYPE1:
            return sizeof(st_edmaDescType1);
        case EDMA_DESC_TYPE5:
            return sizeof(st_edmaDescType5);
        case EDMA_DESC_TYPE15:
            return sizeof(st_edmaDescType15);
        case EDMA_DESC_TYPE4:
        case EDMA_DESC_TYPE6:
        case EDMA_DESC_TYPE12:
            return -1; //not support type yet
        }
    } else if (type == EDMA_INFO_DATA){
        return sizeof(st_edmaDescType0); //type 0/1/2 == 64
    }
    return -1; //not support type
}


int edma_queryDNum(unsigned char type, st_edmaUnifyInfo *shapeInfo)
{

    if (type == EDMA_INFO_GENERAL) {
        st_edma_InputShape *pShape = (st_edma_InputShape *) &shapeInfo->st_info;
        if (pShape->outFormat == EDMA_FMT_ROTATE90 || pShape->outFormat == EDMA_FMT_ROTATE180 ||
                         pShape->outFormat == EDMA_FMT_ROTATE270)
            return 2*edma_rotate_descNum(shapeInfo);
        if (pShape->outFormat == EDMA_FMT_UVResz2X || pShape->inFormat == EDMA_FMT_UVResz2X) {
            EDMA_LOG_INFO("EDMA_FMT_UVResz2X\n");

            return edma_queryTileNum(type, shapeInfo);
        }
    }
    if (type == EDMA_INFO_NN) {
        st_edma_info_nn *pInfoNN = &shapeInfo->st_info.infoNN;

        if (pInfoNN->inFormat == EDMA_FMT_DEPTHTOSPACE) {
            return pInfoNN->shape_h;
        }
    }

    return 1; //not support type or only 1 info with 1 decriptor
}

//descriptor size per info
int edma_queryUFBCDSize(st_edmaUnifyInfo *shapeInfo)
{

  EDMA_LOG_INFO("info_type = %d\n", shapeInfo->info_type);

    return sizeof(st_edmaDescType5);
}

int edma_querySliceDSize(st_edmaUnifyInfo *shapeInfo)
{
    EDMA_LOG_INFO("info_type = %d\n", shapeInfo->info_type);
    return sizeof(st_edmaDescType0);  // Use type 0 descriptor
}

int edma_queryPadDNum(st_edmaUnifyInfo *shapeInfo)
{
    if (shapeInfo == nullptr || shapeInfo->info_type != EDMA_INFO_PAD) {
        EDMA_LOG_ERR("%s Invalid user info\n", __func__);
        return 0;
    }

    uint8_t mode = shapeInfo->st_info.infoPad.uc_mode;
    if (mode == EDMA_CONSTANT_PADDING) {
        return 2;
    } else if (mode == EDMA_EDGE_PADDING) {
        return 5;
    } else {
        EDMA_LOG_ERR("%s Invalid padding mode %u\n", __func__, static_cast<uint32_t>(mode));
    }
    return 0;
}

int edma_queryPadDSize(st_edmaUnifyInfo *shapeInfo)
{
    if (shapeInfo == nullptr || shapeInfo->info_type != EDMA_INFO_PAD) {
        EDMA_LOG_ERR("%s Invalid user info\n", __func__);
        return 0;
    }

    uint8_t mode = shapeInfo->st_info.infoPad.uc_mode;
    EDMA_LOG_INFO("info_type = %d, padding mode %u\n", shapeInfo->info_type, static_cast<uint32_t>(mode));
    if (mode == EDMA_CONSTANT_PADDING) {  // constant padding needs two type 0 descriptors
        return (2 * sizeof(st_edmaDescType0));
    } else if (mode == EDMA_EDGE_PADDING) {  // edge padding needs five type 0 descriptors
        return (5 * sizeof(st_edmaDescType0));
    } else {
        EDMA_LOG_ERR("%s Invalid padding mode %u\n", __func__, static_cast<uint32_t>(mode));
        return 0;
    }
}

int edma_queryBayerToRGGBDSize(st_edmaUnifyInfo *shapeInfo)
{
    EDMA_LOG_INFO("info_type = %d\n", shapeInfo->info_type);
    return sizeof(st_edmaDescType15);  // Use type 15 descriptor
}

int edma_queryRGGBToBayerDSize(st_edmaUnifyInfo *shapeInfo)
{
    EDMA_LOG_INFO("info_type = %d\n", shapeInfo->info_type);
    return sizeof(st_edmaDescType15);  // Use type 15 descriptor
}

#define EDMA_MAX_TILE_ROTATE 16

void edma_rotate_addr_list(
    uint8_t rotation_angle,
    uint32_t x_tile_num,
    uint32_t y_tile_num,
    uint32_t *addr_list)
{
    uint32_t addr_matrix[EDMA_MAX_TILE_ROTATE][EDMA_MAX_TILE_ROTATE];

     if (x_tile_num > 16 || y_tile_num > 16) {
          EDMA_LOG_ERR("%s x_tile_num = %d, y_tile_num = %d too big!!\n",
            __func__, x_tile_num, y_tile_num);
          return;
     }

    for(uint32_t i=0; i<y_tile_num; i++) {
        for(uint32_t j=0; j<x_tile_num; j++)
            addr_matrix[i][j] = addr_list[x_tile_num*i+j];
    }
    if(rotation_angle == 0) {
        for(uint32_t i=0; i<y_tile_num; i++)
            for(uint32_t j=0; j<x_tile_num; j++)
                addr_list[x_tile_num*i+j] = addr_matrix[i][j];
    } else if(rotation_angle == 1) {
        for(uint32_t i=0; i<y_tile_num; i++)
            for(uint32_t j=0; j<x_tile_num; j++)
                addr_list[y_tile_num*(x_tile_num-1)+i-y_tile_num*j] = addr_matrix[i][j];
    } else if(rotation_angle == 2) {
        for(uint32_t i=0; i<y_tile_num; i++)
            for(uint32_t j=0; j<x_tile_num; j++)
                addr_list[x_tile_num*y_tile_num-1-x_tile_num*i-j] = addr_matrix[i][j];
    } else if(rotation_angle == 3) {
        for(uint32_t i=0; i<y_tile_num; i++)
            for(uint32_t j=0; j<x_tile_num; j++)
                addr_list[y_tile_num-1-i+y_tile_num*j] = addr_matrix[i][j];
    }
    return;
}

void edma_gen_tile_rotation(
    uint8_t rotation_angle,
    uint8_t buffer_mode,
    uint32_t buffer_addr,
    uint32_t src_tile_addr,
    uint32_t dst_tile_addr,
    uint32_t tile_stride,
    uint32_t *des_list)
{
    uint32_t buffer_offset[4] = {0x00000000, 0x000000fc, 0x00003ffc, 0x00003f00};
    uint32_t buffer_x_stride[4] = {0x00000004, 0x00000100, 0xfffffffc, 0xffffff00};
    uint32_t buffer_y_stride[4] = {0x00000100, 0xfffffffc, 0xffffff00, 0x00000004};

    for(uint32_t i=0; i<80; i++)
        des_list[i] = 0;

    des_list[0] = 0x0000000f;
    des_list[1] = buffer_mode ? 0x00000300 : 0x00000b00;
    des_list[7] = src_tile_addr;
    des_list[8] = buffer_mode ? (buffer_addr + buffer_offset[rotation_angle]) : buffer_offset[rotation_angle];
    des_list[9] = tile_stride;
    des_list[10] = buffer_x_stride[rotation_angle];
    des_list[12] = buffer_y_stride[rotation_angle];
    des_list[13] = 0x00040100;
    des_list[14] = 0x00400040;
    des_list[15] = 0x00400001;
    des_list[36] = 0x00030001;
    des_list[37] = 0x000f0002;
    des_list[38] = 0x11000011;


    des_list[65] = buffer_mode ? 0x00000300 : 0x00000700;
    des_list[71] = buffer_mode ? buffer_addr : 0;
    des_list[72] = dst_tile_addr;
    des_list[73] = 0x00000100;
    des_list[74] = tile_stride;
    des_list[77] = 0x01000100;
    des_list[78] = 0x00400040;
    des_list[79] = 0x00010001;
    return;
}

//uint8_t rotation_angle, // 1: clockwise 90 degrees; 2 clockwise 180 degrees; 3 clockwise 270 degrees
//uint8_t buffer_mode, // 0: use internal buffer; 1: use TCM
//uint32_t buffer_addr, // if TCM is used, allocate 16K for EDMA computation
void edma_gen_256x256_rotation(
    uint8_t rotation_angle,
    uint8_t buffer_mode, //mode = 0 = internal buffer,
    uint32_t buffer_addr, //must >= 16k, can be TCM or internal buffer
    uint32_t src_addr,
    uint32_t dst_addr,
    uint32_t *des_list)
{
    uint32_t src_addr_list[16];
    uint32_t dst_addr_list[16];
    uint32_t tile_stride = 4 * 256;

    for(uint32_t i=0; i<4; i++) {
        for(uint32_t j=0; j<4; j++) {
            src_addr_list[4*i+j] = src_addr + 4 * 64 * j + tile_stride * 64 * i;
            dst_addr_list[4*i+j] = dst_addr + 4 * 64 * j + tile_stride * 64 * i;
        }
    }
    edma_rotate_addr_list(rotation_angle, 4, 4, dst_addr_list);
    for(uint32_t i=0; i<16; i++)
        edma_gen_tile_rotation(rotation_angle, buffer_mode, buffer_addr, src_addr_list[i], dst_addr_list[i], tile_stride, des_list+80*i);

    return;
}

void gen_tile_rotation(
    uint8_t rotation_angle,
    uint8_t buffer_mode,
    uint32_t buffer_addr,
    uint32_t src_tile_addr,
    uint32_t dst_tile_addr,
    uint32_t src_tile_stride,
    uint32_t dst_tile_stride,
    uint32_t src_tile_c,
    uint32_t src_tile_w,
    uint32_t src_tile_h,
    uint32_t *des_list)
{
    uint32_t buffer_offset[4];
    uint32_t buffer_x_stride[4];
    uint32_t buffer_y_stride[4];
    uint32_t dst_tile_c = src_tile_c;
    uint32_t dst_tile_w[4] = {src_tile_w, src_tile_h, src_tile_w, src_tile_h};
    uint32_t dst_tile_h[4] = {src_tile_h, src_tile_w, src_tile_h, src_tile_w};

    buffer_offset[0] = 0;
    buffer_offset[1] = src_tile_c * (src_tile_h - 1);
    buffer_offset[2] = src_tile_c * (src_tile_w * src_tile_h - 1);
    buffer_offset[3] = src_tile_c * src_tile_h * (src_tile_w - 1);
    buffer_x_stride[0] = src_tile_c;
    buffer_x_stride[1] = src_tile_c * src_tile_h;
    buffer_x_stride[2] = - src_tile_c;
    buffer_x_stride[3] = - src_tile_c * src_tile_h;
    buffer_y_stride[0] = src_tile_c * src_tile_w;
    buffer_y_stride[1] = - src_tile_c;
    buffer_y_stride[2] = - src_tile_c * src_tile_w;
    buffer_y_stride[3] = src_tile_c;
    for(uint32_t i=0; i<80; i++)
        des_list[i] = 0; //clear to 0
#ifdef DISABLE_IOMMU
    des_list[0] = 0x0000000f; //iommu?
#else
    des_list[0] = 0x000000af; //iommu?
#endif
    des_list[1] = buffer_mode ? 0x00000300 : 0x00000b00;
    des_list[7] = src_tile_addr;
    des_list[8] = buffer_mode ? (buffer_addr + buffer_offset[rotation_angle]) : buffer_offset[rotation_angle];
    des_list[9] = src_tile_stride;
    des_list[10] = buffer_x_stride[rotation_angle];
    des_list[12] = buffer_y_stride[rotation_angle];
    des_list[13] = (src_tile_c << 16) | (src_tile_c * src_tile_w);
    des_list[14] = (src_tile_w << 16) | src_tile_h;
    des_list[15] = (src_tile_h << 16) | 1;
    des_list[36] = ((src_tile_c - 1) << 16) | 1;
    des_list[37] = ((src_tile_c - 1) << 16) | 2;
    des_list[38] = 0x11000011;
#ifndef DISABLE_IOMMU
    des_list[64] = 0xa0;
#endif
    des_list[65] = buffer_mode ? 0x00000300 : 0x00000700;
    des_list[71] = buffer_mode ? buffer_addr : 0;
    des_list[72] = dst_tile_addr;
    des_list[73] = dst_tile_c * dst_tile_w[rotation_angle];
    des_list[74] = dst_tile_stride;
    des_list[77] = ((dst_tile_c * dst_tile_w[rotation_angle]) << 16) | (dst_tile_c * dst_tile_w[rotation_angle]);
    des_list[78] = (dst_tile_h[rotation_angle] << 16) | dst_tile_h[rotation_angle];
    des_list[79] = 0x00010001;

    return;
}

// 256x256
//for RGBA_8 (tile_w, tile_h, src_c) = (64, 64, 4)
//for YUV422_8 (tile_w, tile_h, src_c) = (128, 64, 2)

void edma_gen_rotation(
    uint8_t rotation_angle,
    uint8_t buffer_mode,
    uint32_t buffer_addr,
    uint32_t tile_w,
    uint32_t tile_h,
    uint32_t src_addr,
    uint32_t dst_addr,
    uint32_t src_c,
    uint32_t src_w,
    uint32_t src_h,
    uint32_t *des_list)
{
    uint32_t x_tile_num = (src_w - 1) / tile_w + 1;
    uint32_t y_tile_num = (src_h - 1) / tile_h + 1;
    //uint32_t src_addr_list[x_tile_num*y_tile_num];
    std::vector<uint32_t> src_addr_list(x_tile_num*y_tile_num);
    std::vector<uint32_t> dst_addr_list(x_tile_num*y_tile_num);

    //uint32_t dst_addr_list[x_tile_num*y_tile_num];
    uint32_t src_tile_stride = src_c * src_w;
    uint32_t dst_tile_stride = ((rotation_angle == 1) | (rotation_angle == 3)) ? (src_c * src_h) : (src_c * src_w);
    uint32_t tile_w_last = (src_w - 1) % tile_w + 1;
    uint32_t tile_h_last = (src_h - 1) % tile_h + 1;
    std::vector<uint32_t> src_w_list(x_tile_num);
    std::vector<uint32_t> src_h_list(y_tile_num);
    //uint32_t src_w_list[x_tile_num];
    //uint32_t src_h_list[y_tile_num];
    uint32_t des_index;

  for(uint32_t i=0; i<y_tile_num; i++)
        for(uint32_t j=0; j<x_tile_num; j++) {
            src_addr_list[x_tile_num*i+j] = src_addr + src_c * tile_w * j + src_c * src_w * tile_h * i;
            if(rotation_angle == 0)
                dst_addr_list[x_tile_num*i+j] = dst_addr + src_c * tile_w * j + src_c * src_w * tile_h * i;
            else if(rotation_angle == 1)
          dst_addr_list[x_tile_num*i+j] = dst_addr + src_c * src_h * tile_w * j + src_c * tile_h * (y_tile_num - 1 - i)
            - src_c * (tile_h - tile_h_last) * ((y_tile_num - 1 - i) != 0);
            else if(rotation_angle == 2)
          dst_addr_list[x_tile_num*i+j] = dst_addr + src_c * tile_w * (x_tile_num - 1 - j) + src_c * src_w * tile_h * (y_tile_num - 1 - i)
            - src_c * (tile_w - tile_w_last) * ((x_tile_num - 1 - j) != 0) - src_c * src_w * (tile_h - tile_h_last) * ((y_tile_num -1 - i) != 0);
            else
          dst_addr_list[x_tile_num*i+j] = dst_addr + src_c * src_h * tile_w * (x_tile_num - 1 - j) + src_c * tile_h * i
            - src_c * src_h * (tile_w - tile_w_last) * ((x_tile_num - 1 - j) != 0);
    }

  for (uint32_t i=0; i<x_tile_num; i++)
        if (i == x_tile_num - 1)
        src_w_list[i] = tile_w_last;
        else
            src_w_list[i] = tile_w;
  for (uint32_t i=0; i<y_tile_num; i++)
        if (i == y_tile_num - 1)
        src_h_list[i] = tile_h_last;
        else
            src_h_list[i] = tile_h;
  for(uint32_t i=0; i<y_tile_num; i++)
        for (uint32_t j=0; j<x_tile_num; j++) {
            des_index = x_tile_num * i + j;
            gen_tile_rotation(rotation_angle, buffer_mode, buffer_addr, src_addr_list[des_index], dst_addr_list[des_index], src_tile_stride, dst_tile_stride, src_c, src_w_list[j], src_h_list[i], des_list+80*(des_index));
        }
    printf("des_index = %d, total size use %d bytes\r\n", des_index, 80*(des_index+1));

    return;
}

/**
    UV resize 2X

    2 bytes per pixel.
    input graph size = 2*x/2*y/2
    ouput graph size = 2*x*y

    edma type15, use src0 & dst 0
    enabel up sampling (only support w<= 1024, hw limitation)
    note: disable up sampling can support w>1024
    @param edmainfo & descriptor buffer
    @return 0 success, -1 error.
*/

int edma_UVRz2X(st_edmaUnifyInfo *pInfo, void *edma_desc)
{
    int i, tileNum;

    st_edmaDescType15 *DescT15 = (st_edmaDescType15 *)edma_desc;

    st_edma_InputShape *shape = &pInfo->st_info.infoShape;
    tileNum = edma_queryTileNum(EDMA_INFO_GENERAL, pInfo);

    EDMA_LOG_INFO("+in, tileNum = %d\n", tileNum);

    for (i=0; i < tileNum; i++) {

        DescT15 = (st_edmaDescType15 *)((uint8_t *)edma_desc + i*sizeof(st_edmaDescType15));

        memset(DescT15, 0, sizeof(st_edmaDescType15));

        DescT15->ui_desp_00_type = EDMA_DESC_TYPE15;
#ifdef DISABLE_IOMMU
        DescT15->ui_desp_00_aruser = 0x0;
        DescT15->ui_desp_00_awuser = 0x0;
#else
        DescT15->ui_desp_00_aruser = 0x2;
        DescT15->ui_desp_00_awuser = 0x2;
#endif

        DescT15->ui_desp_24_src_x_stride_0 = shape->size_x;

        DescT15->ui_desp_28_dst_x_stride_0 = shape->size_x*2; //0x20

        DescT15->ui_desp_2c_src_y_stride_0 = shape->size_x*shape->size_y; //0x20 //dont care

        DescT15->ui_desp_30_dst_y_stride_0 = shape->size_x*shape->size_y*2; //don't care if dst z = 1

        if (i == (tileNum - 1)) {
            DescT15->ui_desp_34_src_x_size_0 = shape->size_x - i*(shape->size_x/tileNum);
            EDMA_LOG_INFO("[%s]last tile size x  = %d\n", __func__, DescT15->ui_desp_34_src_x_size_0);
            
        } else
            DescT15->ui_desp_34_src_x_size_0 = shape->size_x/tileNum; //fixed 4 bytes

        DescT15->ui_desp_34_dst_x_size_0 = DescT15->ui_desp_34_src_x_size_0*2; //uvuvuvuv
        
        DescT15->ui_desp_38_src_y_size_0 = shape->size_y/2; //06/29

        DescT15->ui_desp_38_dst_y_size_0 = shape->size_y;


        DescT15->ui_desp_3c_src_z_size_0 = 1;


        DescT15->ui_desp_3c_dst_z_size_0 =  1; //care full 16 bits

        DescT15->ui_desp_1c_src_addr_0 = shape->inBuf_addr + i*(shape->size_x/tileNum);

        DescT15->ui_desp_20_dst_addr_0 = shape->outBuf_addr + i*2*(shape->size_x/tileNum);

        DescT15->ui_desp_64_src_x_stride_1 = shape->size_x;


        DescT15->ui_desp_6c_src_y_stride_1 = 0;

        DescT15->ui_desp_74_src_x_size_1 = 0; //8

        DescT15->ui_desp_78_src_y_size_1 = 0;

        DescT15->ui_desp_7c_src_z_size_1 = 0;

        DescT15->ui_desp_80_out_extractor_0 = 1; //up sampling enable


        DescT15->ui_desp_88_splitter = 0;

        DescT15->ui_desp_8c_line_packer = 1;
        DescT15->ui_desp_8c_up_sampler = 1; //up sampling enable


        DescT15->ui_desp_90_ar_sender_0 = 1;
        DescT15->ui_desp_90_r_receiver_0 = 3;

        DescT15->ui_desp_94_aw_sender_0 = 2;

        DescT15->ui_desp_94_w_sender_0 = 3;

        DescT15->ui_desp_98_reader = 1;
        DescT15->ui_desp_98_writer = 1;

        DescT15->ui_desp_98_pipe_enable = 0x11;
    }
    return 1;
 }

 /**
    Fast Space to Depth

    input graph size = h*w*c
    ouput graph size = h/2*w/2*4*c

    only support block size = 2

    special edma type15, use src0 + src1 & dst 0 which src1 = src0 + width pixels

    @param edmainfo & descriptor buffer
    @return 0 success, -1 error.
*/
#define FAST_FACTOR 10
 int edma_S2DFast(st_edmaUnifyInfo *pInfo, void *edma_desc){
     st_edmaDescType15 *DescT15 = (st_edmaDescType15 *)edma_desc;
     st_edma_info_nn *pInfoNN = &pInfo->st_info.infoNN;
     __u16 fast_factor = 1;
     if (((pInfoNN->shape_h) % FAST_FACTOR) == 0)
        fast_factor = FAST_FACTOR;
     EDMA_LOG_INFO("%s #2, fast_factor = %d\n", __func__, fast_factor);
     EDMA_LOG_INFO("block_size = %d, dst_type_size = %d\n", pInfoNN->block_size, pInfoNN->dst_type_size);

     DescT15->ui_desp_24_src_x_stride_0 = pInfoNN->shape_w * pInfoNN->shape_c * pInfoNN->dst_type_size * pInfoNN->block_size;

     DescT15->ui_desp_28_dst_x_stride_0 = pInfoNN->shape_w * pInfoNN->shape_c * pInfoNN->dst_type_size*fast_factor; //0x3840

     DescT15->ui_desp_2c_src_y_stride_0 = 0; //0x20 //?

     DescT15->ui_desp_30_dst_y_stride_0 = 0; //don't care if dst z = 1

     DescT15->ui_desp_34_src_x_size_0 = pInfoNN->shape_w * pInfoNN->shape_c * pInfoNN->dst_type_size; //fixed 4 bytes

     DescT15->ui_desp_34_dst_x_size_0 = DescT15->ui_desp_28_dst_x_stride_0*pInfoNN->shape_h/fast_factor; //0x3840
     //DescT15->ui_desp_34_dst_x_size_0 = 2*pInfoNN->block_size * pInfoNN->block_size * pInfoNN->dst_type_size; //0x10


     //DescT15->ui_desp_38_src_y_size_0 = 2;
     DescT15->ui_desp_38_src_y_size_0 = pInfoNN->shape_h/pInfoNN->block_size; //0x500


     DescT15->ui_desp_38_dst_y_size_0 = 1;
     //DescT15->ui_desp_38_dst_y_size_0 = pInfoNN->shape_h/fast_factor; //0x100 = 256


     //DescT15->ui_desp_3c_src_z_size_0 = 2;

     DescT15->ui_desp_3c_src_z_size_0 = 1;


     DescT15->ui_desp_3c_dst_z_size_0 = 1;

     //DescT15->ui_desp_64_src_x_stride_1 = pInfoNN->shape_w * pInfoNN->block_size * pInfoNN->dst_type_size * pInfoNN->block_size;


     DescT15->ui_desp_64_src_x_stride_1 = pInfoNN->shape_w * pInfoNN->shape_c * pInfoNN->dst_type_size * pInfoNN->block_size; //0xb40  = 2880

     DescT15->ui_desp_68_dst_x_stride_1 = 0;

     DescT15->ui_desp_6c_src_y_stride_1 = 0;

     DescT15->ui_desp_70_dst_y_stride_1 = 0x0;

     DescT15->ui_desp_74_src_x_size_1 = pInfoNN->shape_w * pInfoNN->shape_c * pInfoNN->dst_type_size; //0x5a0 = 1440

     DescT15->ui_desp_74_dst_x_size_1 = 0; //0x10

     //DescT15->ui_desp_74_dst_x_size_1 = 2*pInfoNN->block_size * pInfoNN->block_size * pInfoNN->dst_type_size; //0x10


     //DescT15->ui_desp_78_src_y_size_1 = 0x2;
     DescT15->ui_desp_78_src_y_size_1 = pInfoNN->shape_h/pInfoNN->block_size; //0x500 = 1280


     //DescT15->ui_desp_78_dst_y_size_1 = 0x2;
     DescT15->ui_desp_78_dst_y_size_1 = 0;

     //DescT15->ui_desp_7c_src_z_size_1 = 0x2;
     DescT15->ui_desp_7c_src_z_size_1 = 1;

     DescT15->ui_desp_7c_dst_z_size_1 = 0;


     DescT15->ui_desp_88_merger = 3;

     if (pInfoNN->src_type_size == 2) {
        EDMA_LOG_INFO("pInfoNN->src_type_size == 2\n");        
        DescT15->ui_desp_88_merger = 6;
     }

     if (pInfoNN->shape_w >= 8 || pInfoNN->src_type_size == 2) {
     EDMA_LOG_INFO("%s shape_w = %d || src_type_size = 2, use fast mode\n", __func__, pInfoNN->shape_w);
     //dst_type_size = 2 

     DescT15->ui_desp_90_ar_sender_0 = 1;
     DescT15->ui_desp_90_ar_sender_1 = 1;

     DescT15->ui_desp_90_r_receiver_0 = 3; //7->3 08/01

     //DescT15->ui_desp_90_r_receiver_0 = 7; //7->3 08/01
     DescT15->ui_desp_90_r_receiver_1 = 3;
     DescT15->ui_desp_94_aw_sender_0 = 2;

     DescT15->ui_desp_94_w_sender_0 = 0x7; //f ->7 08/01

    } else {
        DescT15->ui_desp_90_ar_sender_0 = 1;
        DescT15->ui_desp_90_ar_sender_1 = 1;

        DescT15->ui_desp_90_r_receiver_0 = 1; //7->3 08/01

        //DescT15->ui_desp_90_r_receiver_0 = 7; //7->3 08/01
        DescT15->ui_desp_90_r_receiver_1 = 1;
        DescT15->ui_desp_94_aw_sender_0 = 2;

        DescT15->ui_desp_94_w_sender_0 = 0x3; //f ->7 08/01

    }

     //DescT15->ui_desp_94_w_sender_0 = 0xf; //f ->7 08/01
     DescT15->ui_desp_98_reader = 2;
     DescT15->ui_desp_98_writer = 1;
     DescT15->ui_desp_98_pipe_enable = 0x13;


     //DescT15->ui_desp_98_pipe_enable = 0x77;

     return 1;


 
 }
  /**
     Fase Depth to Space
     
     input graph size = h*w*c
     ouput graph size = 2*h*2*w*c/4
 
     only support block size = 2
 
     special edma type15, use src0 & dst 0 + dst 1 which dst1 = dst0*c*width + width pixels    
 
     @param edmainfo & descriptor buffer
     @return 0 success, -1 error.
 */

  int edma_D2SFast(st_edmaUnifyInfo *pInfo, void *edma_desc){
     st_edmaDescType15 *DescT15 = (st_edmaDescType15 *)edma_desc;
     st_edma_info_nn *pInfoNN = &pInfo->st_info.infoNN;
     __u16 fast_factor = 1;
     if (((pInfoNN->shape_h) % FAST_FACTOR) == 0)
        fast_factor = FAST_FACTOR;
     EDMA_LOG_INFO("%s #3, fast_factor = %d\n", __func__, fast_factor);
     EDMA_LOG_INFO("block_size = %d, dst_type_size = %d\n", pInfoNN->block_size, pInfoNN->dst_type_size);

     EDMA_LOG_INFO("EDMA_FMT_DEPTHTOSPACE_FAST #3 type15\n");

     if (pInfoNN->block_size > 2) {
         EDMA_LOG_ERR("%s, not spport block sz = %d\n", __func__, pInfoNN->block_size);
         return -1;
     }

     /*4->1, 16->4*/
     //DescT15->ui_desp_24_src_x_stride_0 = 0x10; //byte
     //DescT15->ui_desp_24_src_x_stride_0 = pInfoNN->shape_w * pInfoNN->block_size * pInfoNN->dst_type_size * pInfoNN->block_size;

     DescT15->ui_desp_24_src_x_stride_0 = pInfoNN->shape_w * pInfoNN->shape_c * pInfoNN->dst_type_size;

     DescT15->ui_desp_28_dst_x_stride_0 = pInfoNN->shape_w * pInfoNN->shape_c * pInfoNN->dst_type_size; //0x20

     DescT15->ui_desp_2c_src_y_stride_0 = 0; //0x20 //?

     DescT15->ui_desp_30_dst_y_stride_0 = 0; //don't care if dst z = 1

     DescT15->ui_desp_34_src_x_size_0 = pInfoNN->shape_w * pInfoNN->shape_c * pInfoNN->dst_type_size; //fixed 4 bytes

     DescT15->ui_desp_34_dst_x_size_0 = pInfoNN->shape_w * pInfoNN->dst_type_size * pInfoNN->shape_c / pInfoNN->block_size; //0x10
     //DescT15->ui_desp_34_dst_x_size_0 = 2*pInfoNN->block_size * pInfoNN->block_size * pInfoNN->dst_type_size; //0x10


     //DescT15->ui_desp_38_src_y_size_0 = 2;
     DescT15->ui_desp_38_src_y_size_0 = pInfoNN->shape_h;


     //DescT15->ui_desp_38_dst_y_size_0 = 2;
     DescT15->ui_desp_38_dst_y_size_0 = pInfoNN->shape_h;


     //DescT15->ui_desp_3c_src_z_size_0 = 2;

     DescT15->ui_desp_3c_src_z_size_0 = 1;

     DescT15->ui_desp_3c_dst_z_size_0 = 1;

     //DescT15->ui_desp_64_src_x_stride_1 = pInfoNN->shape_w * pInfoNN->block_size * pInfoNN->dst_type_size * pInfoNN->block_size;


     DescT15->ui_desp_64_src_x_stride_1 = 0;

     DescT15->ui_desp_68_dst_x_stride_1 = pInfoNN->shape_w * pInfoNN->shape_c * pInfoNN->dst_type_size;

     DescT15->ui_desp_6c_src_y_stride_1 = 0;

     DescT15->ui_desp_70_dst_y_stride_1 = 0x0;

     DescT15->ui_desp_74_src_x_size_1 = 0; //8

     DescT15->ui_desp_74_dst_x_size_1 = pInfoNN->shape_w * pInfoNN->dst_type_size * pInfoNN->shape_c / pInfoNN->block_size; //0x10

     //DescT15->ui_desp_74_dst_x_size_1 = 2*pInfoNN->block_size * pInfoNN->block_size * pInfoNN->dst_type_size; //0x10


     //DescT15->ui_desp_78_src_y_size_1 = 0x2;
     DescT15->ui_desp_78_src_y_size_1 = 0;


     //DescT15->ui_desp_78_dst_y_size_1 = 0x2;
     DescT15->ui_desp_78_dst_y_size_1 = pInfoNN->shape_h;

     //DescT15->ui_desp_7c_src_z_size_1 = 0x2;
     DescT15->ui_desp_7c_src_z_size_1 = 0;

     DescT15->ui_desp_7c_dst_z_size_1 = 0x1;


     DescT15->ui_desp_80_out_extractor_0 = 1;

     DescT15->ui_desp_80_out_extractor_1 = 1;

     DescT15->ui_desp_84_in_zero_padder_0 = 1;

     DescT15->ui_desp_88_splitter = 4;


     DescT15->ui_desp_90_ar_sender_0 = 1;
     DescT15->ui_desp_90_r_receiver_0 = 0xb; //7->3

     DescT15->ui_desp_94_aw_sender_0 = 2;
     DescT15->ui_desp_94_aw_sender_1 = 2;

     DescT15->ui_desp_94_w_sender_0 = 5; //7->3
     DescT15->ui_desp_94_w_sender_1 = 5; // 7->3

     DescT15->ui_desp_98_reader = 1;
     DescT15->ui_desp_98_writer = 2;

     DescT15->ui_desp_98_pipe_enable = 0x31;


     //DescT15->ui_desp_98_pipe_enable = 0x77;
     if (pInfoNN->shape_c == 16) {
         EDMA_LOG_INFO("EDMA_FMT_DEPTHTOSPACE_FAST c = 16 - > 4!!!\n");

         DescT15->ui_desp_90_r_receiver_0 = 0xf;
         DescT15->ui_desp_94_w_sender_0 = 7; //7->3
         DescT15->ui_desp_94_w_sender_1 = 7; // 7->3
         DescT15->ui_desp_88_splitter = 3;
         DescT15->ui_desp_80_out_extractor_0 = 0;
         DescT15->ui_desp_80_out_extractor_1 = 0;
         DescT15->ui_desp_84_in_zero_padder_0 = 0;

     }

     if (pInfoNN->src_type_size == 2) {
         EDMA_LOG_INFO("pInfoNN->src_type_size == 2\n");

         DescT15->ui_desp_80_out_extractor_0 = 0;
         DescT15->ui_desp_80_out_extractor_1 = 0;
         DescT15->ui_desp_84_in_zero_padder_0 = 0;
         DescT15->ui_desp_88_splitter = 6;
         DescT15->ui_desp_90_r_receiver_0 = 0xf; //7->3
         DescT15->ui_desp_94_w_sender_0 = 7; //7->3
         DescT15->ui_desp_94_w_sender_1 = 7; // 7->3
     }

     return 1;
}

/*for edma 3.0*/
#define INTERNAL_BUFFER_M
int fillDescV0(st_edmaUnifyInfo *pInfo, void *currDesc, void *headDesc)
{
    st_edmaDescType0 *DescT0 = (st_edmaDescType0 *)currDesc;
    st_edmaDescType5 *DescT5 = (st_edmaDescType5 *)currDesc;
    st_edmaDescType15 *DescT15 = (st_edmaDescType15 *)currDesc;
    st_edma_InputShape *shape = &pInfo->st_info.infoShape;
    EDMA_DESC_T type = checkDescrpType(pInfo);

    const uint32_t offsetBase =
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(currDesc) - reinterpret_cast<uintptr_t>(headDesc));

    if (type == EDMA_DESC_TYPE0) {
        EDMA_LOG_INFO("fillDescV0 [edma30] st_edmaDescType0 size = %d\n", (int)sizeof(st_edmaDescType0));

        EDMA_LOG_INFO("shape->inBuf_addr = 0x%x\n", shape->inBuf_addr);
        EDMA_LOG_INFO("shape->outBuf_addr = 0x%x\n", shape->outBuf_addr);
        EDMA_LOG_INFO("shape->size_x = 0x%x\n", shape->size_x);

        memset(DescT0, 0, sizeof(st_edmaDescType0));

        DescT0->ui_desp_00_type = 0;
#ifdef DISABLE_IOMMU
        DescT0->ui_desp_00_aruser = 0;
        DescT0->ui_desp_00_awuser = 0;
#else
        DescT0->ui_desp_00_aruser = 0x2;
        DescT0->ui_desp_00_awuser = 0x2;
#endif

        DescT0->ui_desp_04_format = 0;

        DescT0->ui_desp_1c_src_addr_0 = shape->inBuf_addr;
        DescT0->ui_desp_20_dst_addr_0 = shape->outBuf_addr;

        DescT0->ui_desp_34_src_x_size_0 = shape->size_x;
        DescT0->ui_desp_34_dst_x_size_0 = shape->size_x;

        DescT0->ui_desp_38_src_y_size_0 = shape->size_y;
        DescT0->ui_desp_38_dst_y_size_0 = shape->size_y;

        DescT0->ui_desp_24_src_x_stride_0 = shape->size_x;
        DescT0->ui_desp_28_dst_x_stride_0 = shape->dst_stride_x;
        DescT0->ui_desp_2c_src_y_stride_0 = shape->size_x*shape->size_y;
        DescT0->ui_desp_30_dst_y_stride_0 = shape->dst_stride_x*shape->dst_stride_y;

        DescT0->ui_desp_3c_src_z_size_0 = shape->size_z;
        DescT0->ui_desp_3c_dst_z_size_0 = shape->size_z;

        if (pInfo->inputBindings != nullptr && pInfo->outputBindings != nullptr) {
            pInfo->inputBindings->AppendNormal(offsetBase + 0x1C, 0);
            pInfo->outputBindings->AppendNormal(offsetBase + 0x20, 0);
        }
    } else if (type == EDMA_DESC_TYPE5) {
        EDMA_LOG_INFO("fillDescV0 [edma30] st_edmaDescType5 size = %d\n", (int)sizeof(st_edmaDescType5));
        EDMA_LOG_INFO("shape->inBuf_addr = 0x%x\n", shape->inBuf_addr);
        EDMA_LOG_INFO("shape->outBuf_addr = 0x%x\n", shape->outBuf_addr);
        EDMA_LOG_INFO("shape->size_x = 0x%x\n", shape->size_x);

        memset(DescT5, 0, sizeof(st_edmaDescType5));

        DescT5->ui_desp_00_type = 5;
        DescT5->ui_desp_00_aruser = 0x2;
        DescT5->ui_desp_00_awuser = 0x2;

        if (shape->inFormat == EDMA_FMT_YUV422_8B &&
            shape->outFormat == EDMA_FMT_ARGB_8)
            DescT5->ui_desp_04_format = 9;

        // 0x08: APU_EDMA3_DESP_08
        DescT5->ui_desp_08_in_swap_mat = 0x8142;
        DescT5->ui_desp_08_out_swap_mat = 0x8421;

        DescT5->ui_desp_1c_src_addr_0 = shape->inBuf_addr;
        DescT5->ui_desp_20_dst_addr_0 = shape->outBuf_addr;

        DescT5->ui_desp_34_src_x_size_0 = shape->size_x;
        if (shape->inFormat == EDMA_FMT_YUV422_8B &&
            shape->outFormat == EDMA_FMT_ARGB_8)
            DescT5->ui_desp_34_dst_x_size_0 = shape->size_x*2; //2byte -> 4bytes
        else
            DescT5->ui_desp_34_dst_x_size_0 = shape->size_x;

        DescT5->ui_desp_38_src_y_size_0 = shape->size_y;
        DescT5->ui_desp_38_dst_y_size_0 = shape->size_y;


        DescT5->ui_desp_24_src_x_stride_0 = shape->size_x;

        DescT5->ui_desp_28_dst_x_stride_0 = shape->dst_stride_x;

        DescT5->ui_desp_2c_src_y_stride_0 = shape->size_x*shape->size_y;

        DescT5->ui_desp_30_dst_y_stride_0 = shape->dst_stride_x*shape->dst_stride_y;


        DescT5->ui_desp_3c_src_z_size_0 = shape->size_z;
        DescT5->ui_desp_3c_dst_z_size_0 = shape->size_z;


        // 0x44: APU_EDMA3_DESP_0C
        DescT5->ui_desp_44_r2y_y2r_mat_00 = 0x400;
        DescT5->ui_desp_44_r2y_y2r_mat_01 = 0x1fff;

        // 0x48: APU_EDMA3_DESP_10
        DescT5->ui_desp_48_r2y_y2r_mat_02 = 0x059c;
        DescT5->ui_desp_48_r2y_y2r_vec_0 = 0x1d32;

        // 0x4C: APU_EDMA3_DESP_14
        DescT5->ui_desp_4c_r2y_y2r_mat_10 = 0x400;
        DescT5->ui_desp_4c_r2y_y2r_mat_11 = 0x1e9f;

        // 0x50: APU_EDMA3_DESP_08
        DescT5->ui_desp_50_r2y_y2r_mat_12 = 0x1d25;
        DescT5->ui_desp_50_r2y_y2r_vec_1 = 0x021d;


        // 0x54: APU_EDMA3_DESP_0C
        DescT5->ui_desp_54_r2y_y2r_mat_20 = 0x400;
        DescT5->ui_desp_54_r2y_y2r_mat_21 = 0x716;


        // 0x58: APU_EDMA3_DESP_10
        DescT5->ui_desp_58_r2y_y2r_mat_22 = 0x1;
        DescT5->ui_desp_58_r2y_y2r_vec_2 = 0x1c74;

        if (pInfo->inputBindings != nullptr && pInfo->outputBindings != nullptr) {
            pInfo->inputBindings->AppendNormal(offsetBase + 0x1C, 0);
            pInfo->outputBindings->AppendNormal(offsetBase + 0x20, 0);
        }
    } else if (type == EDMA_DESC_TYPE15) {
        if (shape->outFormat == EDMA_FMT_UVResz2X || shape->inFormat == EDMA_FMT_UVResz2X) {
            edma_UVRz2X(pInfo, currDesc);
            if (pInfo->inputBindings != nullptr && pInfo->outputBindings != nullptr) {
                pInfo->inputBindings->AppendNormal(offsetBase + 0x1C, 0);
                pInfo->outputBindings->AppendNormal(offsetBase + 0x20, 0);
            }
        } else {
        uint8_t rotation_angle = 1; //default = 90
        uint8_t buffer_mode = 1; // 1 = tcm, 0 = internal buffer
        uint32_t buffer_addr = 0x1d000000;
        uint32_t src_tile_addr = shape->inBuf_addr;
        uint32_t dst_tile_addr = shape->outBuf_addr;
        uint32_t *des_list = (uint32_t *)currDesc;
        uint32_t width, height;

        uint32_t tile_w = 64;
        uint32_t tile_h = 64;
        uint32_t src_c = 4;

        EDMA_LOG_INFO("st_edmaDescType15 size = %zu \n", sizeof(st_edmaDescType15));

        memset(DescT15, 0, sizeof(st_edmaDescType15));

        if (shape->outFormat == EDMA_FMT_ROTATE180)
            rotation_angle = 2;
        else if (shape->outFormat == EDMA_FMT_ROTATE270)
            rotation_angle = 3;

        //edma_gen_256x256_rotation(rotation_angle, buffer_mode, buffer_addr, src_tile_addr, dst_tile_addr, des_list);
#ifdef INTERNAL_BUFFER_M
        buffer_mode = 0;
        buffer_addr = 0;
        EDMA_LOG_INFO("edma internal buffer mode\n");
#endif

        // 256x256
        //for RGBA_8 (tile_w, tile_h, src_c) = (64, 64, 4)
        //for YUV422_8 (tile_w, tile_h, src_c) = (128, 64, 2)
        if (shape->inFormat == EDMA_FMT_ARGB_8) {
            width = shape->size_x/4;
            height = shape->size_y;
        } else {
            EDMA_LOG_INFO("shape->inFormat = %d\n", shape->inFormat);
            height = shape->size_y;
            width = height;
        }
        if (shape->inFormat == EDMA_FMT_YUV422_8B) {
            tile_w = 128;
            tile_h = 64;
            src_c = 2;
        }
        EDMA_LOG_INFO("run edma_gen_rotation #2 width = %d, height = %d, src_c = %d, buffer_mode = %d\n", width, height, src_c, buffer_mode);

        edma_gen_rotation(rotation_angle, buffer_mode, buffer_addr, tile_w, tile_h, src_tile_addr, dst_tile_addr, src_c,
                            width, height, des_list);

        DescT15->ui_desp_00_type = EDMA_DESC_TYPE15;
#ifdef DISABLE_IOMMU
        DescT15->ui_desp_00_aruser = 0x0;
        DescT15->ui_desp_00_awuser = 0x0;
#else
        DescT15->ui_desp_00_aruser = 0x2;
        DescT15->ui_desp_00_awuser = 0x2;
#endif

        EDMA_LOG_INFO("run edma_gen_rotation finished\n");
    }
    }

    return 0;
}


/*for edma 3.0*/

int fillDescData(st_edmaUnifyInfo *pInfo, void *currDesc, void *headDesc)
{
    st_edmaDescType0 *pDesc = (st_edmaDescType0 *)currDesc;
    st_edmaDescType2 *pDescT2 = (st_edmaDescType2 *)currDesc;
    st_edma_InfoData *pIdata = &pInfo->st_info.infoData;

    const uint32_t offsetBase =
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(currDesc) - reinterpret_cast<uintptr_t>(headDesc));

    //EDMA_DESC_T type = EDMA_DESC_TYPE0;
    unsigned int src_size_x = 1;         // unit: byte
    unsigned int src_size_y = 1;         // unit: line, <=65535

    if (pIdata->inFormat == EDMA_FMT_PRODUCER ||
        pIdata->inFormat == EDMA_FMT_CONSUMER) {

        EDMA_LOG_INFO("fillDescData [edma35] [pVID/cVID] = [%d/%d]\n",
            pIdata->pVID, pIdata->cVID);

        memset(pDescT2, 0, sizeof(st_edmaDescType2));

        pDescT2->ui_desp_00_type = 0x2;
#ifdef DISABLE_IOMMU
        pDescT2->ui_desp_00_aruser = 0;
        pDescT2->ui_desp_00_awuser = 0;
#else
        pDescT2->ui_desp_00_aruser = 0x2;
        pDescT2->ui_desp_00_awuser = 0x2;
#endif

        if (pIdata->inFormat == EDMA_FMT_PRODUCER ||
            pIdata->outFormat == EDMA_FMT_PRODUCER) {

            pDescT2->ui_desp_10_hse_pro_enroll_iteration = 1;
            pDescT2->ui_desp_10_hse_pro_enroll_vid = pIdata->pVID;
        }
        if (pIdata->inFormat == EDMA_FMT_CONSUMER ||
            pIdata->outFormat == EDMA_FMT_CONSUMER) {

            pDescT2->ui_desp_14_hse_con_enroll_iteration = 1;
            pDescT2->ui_desp_14_hse_con_enroll_vid = pIdata->cVID;
            pDescT2->ui_desp_18_hse_con_wait = 1;
        }

    } else {
        EDMA_LOG_INFO("fillDescData [edma30] copy size = %d\n", pIdata->copy_size);
        EDMA_LOG_INFO("shape->inBuf_addr = 0x%x\n", pIdata->inBuf_addr);
        EDMA_LOG_INFO("shape->outBuf_addr = 0x%x\n", pIdata->outBuf_addr);

        if (pIdata->copy_size > 64*1024) {
            if (pIdata->copy_size > 64*1024*1024) {
            EDMA_LOG_ERR("pIdata->copy_size = %d not support\n", pIdata->copy_size);
            return -1;
        }
            if (pIdata->copy_size % 1024 == 0) {
                src_size_x = 1024;
                src_size_y = pIdata->copy_size/1024;
            } else {
                EDMA_LOG_ERR("pIdata->copy_size = %d not support, divide 1024 not 0\n", pIdata->copy_size);
                return -1;
            }
        } else {
            src_size_x = pIdata->copy_size;
            src_size_y = 1;
        }

        memset(pDesc, 0, sizeof(st_edmaDescType0));

        pDesc->ui_desp_00_type = 0;
#ifdef DISABLE_IOMMU
        pDesc->ui_desp_00_aruser = 0;
        pDesc->ui_desp_00_awuser = 0;
#else
        pDesc->ui_desp_00_aruser = 0x2;
        pDesc->ui_desp_00_awuser = 0x2;
#endif

        if (pIdata->outFormat == EDMA_FMT_FILL) {
            EDMA_LOG_INFO("fillDescData fill value = %d\n", pIdata->fillValue);
            pDesc->ui_desp_04_format = 1;
            pDesc->ui_desp_08_fill_const = pIdata->fillValue;
        } else {
            pDesc->ui_desp_04_format = 0;
        }

        pDesc->ui_desp_1c_src_addr_0 = pIdata->inBuf_addr;
        pDesc->ui_desp_20_dst_addr_0 = pIdata->outBuf_addr;

        pDesc->ui_desp_34_src_x_size_0 = src_size_x;
        pDesc->ui_desp_34_dst_x_size_0 = src_size_x;

        pDesc->ui_desp_38_src_y_size_0 = src_size_y;
        pDesc->ui_desp_38_dst_y_size_0 = src_size_y;

        pDesc->ui_desp_24_src_x_stride_0 = src_size_x;
        pDesc->ui_desp_28_dst_x_stride_0 = src_size_x;
        pDesc->ui_desp_2c_src_y_stride_0 = src_size_y;
        pDesc->ui_desp_30_dst_y_stride_0 = src_size_y;

        pDesc->ui_desp_3c_src_z_size_0 = 1;
        pDesc->ui_desp_3c_dst_z_size_0 = 1;

        if (pInfo->inputBindings != nullptr && pInfo->outputBindings != nullptr) {
            pInfo->inputBindings->AppendNormal(offsetBase + 0x1C, 0);
            pInfo->outputBindings->AppendNormal(offsetBase + 0x20, 0);
        }
    }
    return 0;

}

/* NN needs to fill desc id*/
/* currDesc need to init 0 at beginning*/
int fillDescNN(st_edmaUnifyInfo *shape, void *currDesc, void *headDesc)
{
    st_edmaDescType0 *DescT0 = (st_edmaDescType0 *)currDesc;
    st_edmaDescType1 *DescT1 = (st_edmaDescType1 *)currDesc;
    st_edmaDescType5 *DescT5 = (st_edmaDescType5 *)currDesc;
    st_edmaDescType15 *DescT15 = (st_edmaDescType15 *)currDesc;
    st_edma_info_nn *pInfoNN = &shape->st_info.infoNN;

    __u8 *pHead = (__u8 *)headDesc;
    __u8 *pDesc = (__u8 *)currDesc;

    __u32 descDiff = (__u32)(pDesc - pHead);
    //st_edma_nn_ouput *pOut = shape->st_info.pInfoNN->pOut;

    EDMA_DESC_T type = checkDescrpType(shape);


    EDMA_LOG_INFO("st_edmaDescType15 size = %zu \n", sizeof(st_edmaDescType15));
    EDMA_LOG_INFO("#5 pInfoNN = %p, currDesc = %p, type = %d   \n", (void *)pInfoNN, currDesc, type);
    EDMA_LOG_INFO("headDesc = %p, diff = 0x%x \n", headDesc, descDiff);

    //normal, fp16-fp32, fp32-fp16, normal 3-4;1-4;4-1 spcial case
    if (type == EDMA_DESC_TYPE0) {
        EDMA_LOG_INFO("fillDescNN [edma30] st_edmaDescType0 size = %d\n", (int)sizeof(st_edmaDescType0));
        EDMA_LOG_INFO("inFormat = %d, outFormat = %d,   \n", pInfoNN->inFormat, pInfoNN->outFormat);

        memset(DescT0, 0, sizeof(st_edmaDescType0));

        DescT0->ui_desp_00_type = EDMA_DESC_TYPE0;
#ifdef DISABLE_IOMMU
        DescT0->ui_desp_00_aruser = 0;
        DescT0->ui_desp_00_awuser = 0;
#else
        DescT0->ui_desp_00_aruser = 0x2;
        DescT0->ui_desp_00_awuser = 0x2;
#endif

//add fp
        if (pInfoNN->inFormat == EDMA_FMT_FP16 && pInfoNN->outFormat == EDMA_FMT_FP32)
            DescT0->ui_desp_04_format = 2; //fp16 - fp32
        else if (pInfoNN->inFormat == EDMA_FMT_FP32 && pInfoNN->outFormat == EDMA_FMT_FP16)
            DescT0->ui_desp_04_format = 3;
        else if (pInfoNN->inFormat == EDMA_FMT_I8 && pInfoNN->outFormat == EDMA_FMT_FP32)
            DescT0->ui_desp_04_format = 4;
        else if (pInfoNN->inFormat == EDMA_FMT_FP32 && pInfoNN->outFormat == EDMA_FMT_I8)
            DescT0->ui_desp_04_format = 5;
        else
            DescT0->ui_desp_04_format = 0; //normal mode

        pInfoNN->src_addr_offset = descDiff + 0x1C;
        pInfoNN->dst_addr_offset = descDiff + 0x20;
        if (shape->inputBindings != nullptr && shape->outputBindings != nullptr) {
            shape->inputBindings->AppendNormal(pInfoNN->src_addr_offset, 0);
            shape->outputBindings->AppendNormal(pInfoNN->dst_addr_offset, 0);
        }

        DescT0->ui_desp_00_des_id = pInfoNN->desc_id;

        if (pInfoNN->inFormat == EDMA_FMT_DEPTHTOSPACE) {
            EDMA_LOG_INFO("EDMA_FMT_DEPTHTOSPACE #3\n");
            EDMA_LOG_INFO("block_size = %d, dst_type_size = %d\n", pInfoNN->block_size, pInfoNN->dst_type_size);
            
            DescT0->ui_desp_34_src_x_size_0 = pInfoNN->shape_c * pInfoNN->src_type_size;
            DescT0->ui_desp_34_dst_x_size_0 = pInfoNN->block_size * pInfoNN->dst_type_size;

            DescT0->ui_desp_38_src_y_size_0 = pInfoNN->shape_w;
            DescT0->ui_desp_38_dst_y_size_0 = pInfoNN->block_size;

            DescT0->ui_desp_24_src_x_stride_0 = pInfoNN->src_stride_c * pInfoNN->src_type_size;
            
            DescT0->ui_desp_28_dst_x_stride_0 = pInfoNN->block_size * pInfoNN->dst_type_size * pInfoNN->shape_w;
            
            DescT0->ui_desp_2c_src_y_stride_0 = pInfoNN->src_stride_w * pInfoNN->src_type_size;
            
            DescT0->ui_desp_30_dst_y_stride_0 = pInfoNN->block_size * pInfoNN->dst_type_size;


            //DescT0->ui_desp_3c_src_z_size_0 = pInfoNN->shape_n * pInfoNN->shape_h;
            //DescT0->ui_desp_3c_dst_z_size_0 = pInfoNN->shape_n * pInfoNN->shape_h;
            DescT0->ui_desp_3c_src_z_size_0 = 1;
            DescT0->ui_desp_3c_dst_z_size_0 = pInfoNN->shape_w;

            


        } else {
        DescT0->ui_desp_34_src_x_size_0 = pInfoNN->shape_c * pInfoNN->src_type_size;
        DescT0->ui_desp_34_dst_x_size_0 = pInfoNN->shape_c * pInfoNN->dst_type_size;

        DescT0->ui_desp_38_src_y_size_0 = pInfoNN->shape_w;
        DescT0->ui_desp_38_dst_y_size_0 = pInfoNN->shape_w;


        DescT0->ui_desp_24_src_x_stride_0 = pInfoNN->src_stride_c * pInfoNN->src_type_size;

        DescT0->ui_desp_28_dst_x_stride_0 = pInfoNN->dst_stride_c * pInfoNN->dst_type_size;

        DescT0->ui_desp_2c_src_y_stride_0 = pInfoNN->src_stride_w   * pInfoNN->src_type_size;

        DescT0->ui_desp_30_dst_y_stride_0 = pInfoNN->dst_stride_w * pInfoNN->dst_type_size;

        DescT0->ui_desp_3c_src_z_size_0 = pInfoNN->shape_n * pInfoNN->shape_h;
        DescT0->ui_desp_3c_dst_z_size_0 = pInfoNN->shape_n * pInfoNN->shape_h;
        }

    }else if (type == EDMA_DESC_TYPE1) {
        // RGB -> RGBA 3->4
        __u16 batch = pInfoNN->shape_n;

        EDMA_LOG_INFO("fillDescNN 3x4 special path #1!!\n");

        memset(DescT1, 0, sizeof(st_edmaDescType1));

        DescT1->ui_desp_00_type = EDMA_DESC_TYPE1;

        DescT1->ui_desp_00_des_id = pInfoNN->desc_id;
#ifdef DISABLE_IOMMU
        DescT1->ui_desp_00_aruser = 0;
        DescT1->ui_desp_00_awuser = 0;
#else
        DescT1->ui_desp_00_aruser = 0x2;
        DescT1->ui_desp_00_awuser = 0x2;
#endif
        DescT1->ui_desp_04_format = 7; // 7: rgb_8_to_rgba_8

        // Default permutation
        DescT1->ui_desp_08_in_swap_mat = 0x8421;
        DescT1->ui_desp_08_out_swap_mat = 0x8421;


        pInfoNN->src_addr_offset = descDiff + 0x1C;
        pInfoNN->dst_addr_offset = descDiff + 0x20;
        if (shape->inputBindings != nullptr && shape->outputBindings != nullptr) {
            shape->inputBindings->AppendNormal(pInfoNN->src_addr_offset, 0);
            shape->outputBindings->AppendNormal(pInfoNN->dst_addr_offset, 0);
        }

        DescT1->ui_desp_34_src_x_size_0 = pInfoNN->shape_w * 3;
        DescT1->ui_desp_34_dst_x_size_0 = pInfoNN->shape_w * 4;

        DescT1->ui_desp_38_src_y_size_0 = batch * pInfoNN->shape_h;
        DescT1->ui_desp_38_dst_y_size_0 = batch * pInfoNN->shape_h;

        DescT1->ui_desp_24_src_x_stride_0 = pInfoNN->src_stride_w;
        DescT1->ui_desp_28_dst_x_stride_0 = pInfoNN->dst_stride_w;
        DescT1->ui_desp_2c_src_y_stride_0 = pInfoNN->src_stride_h   * batch;
        DescT1->ui_desp_30_dst_y_stride_0 = pInfoNN->dst_stride_h  * batch;
        DescT1->ui_desp_3c_src_z_size_0 = 1;
        DescT1->ui_desp_3c_dst_z_size_0 = 1;

    } else if (type == EDMA_DESC_TYPE5) {

        __u16 batch = pInfoNN->shape_n;

        EDMA_LOG_INFO("fillDescNN 1x4 special path #3!!\n");
        EDMA_LOG_INFO("pInfoNN->inFormat = %d!!\n", pInfoNN->inFormat);

        //NN 1 to 4 special case (YUV420 -> RGBA)
        memset(DescT5, 0, sizeof(st_edmaDescType5));

        DescT5->ui_desp_00_type = EDMA_DESC_TYPE5;
        DescT5->ui_desp_00_des_id = pInfoNN->desc_id;
#ifdef DISABLE_IOMMU
        DescT5->ui_desp_00_aruser = 0x0;
        DescT5->ui_desp_00_awuser = 0x0;
#else
        DescT5->ui_desp_00_aruser = 0x2;
        DescT5->ui_desp_00_awuser = 0x2;
#endif


            DescT5->ui_desp_04_format = 8; //8: yuv420_8_to_rgba_8

            // 0x08: APU_EDMA3_DESP_08
            DescT5->ui_desp_08_in_swap_mat = 0x8142;
            DescT5->ui_desp_08_out_swap_mat = 0x8421;

            DescT5->ui_desp_24_src_x_stride_0 = pInfoNN->src_stride_w;
            DescT5->ui_desp_28_dst_x_stride_0 = pInfoNN->dst_stride_w;

            DescT5->ui_desp_2c_src_y_stride_0 = batch * pInfoNN->src_stride_h/2;
            DescT5->ui_desp_30_dst_y_stride_0 = batch * pInfoNN->dst_stride_h;

            DescT5->ui_desp_34_dst_x_size_0 = pInfoNN->shape_w *4;
            DescT5->ui_desp_34_src_x_size_0 = pInfoNN->shape_w;
            DescT5->ui_desp_38_dst_y_size_0 = batch * pInfoNN->shape_h;
            DescT5->ui_desp_38_src_y_size_0 = batch * pInfoNN->shape_h/2;

            DescT5->ui_desp_3c_dst_z_size_0 = 1;
            DescT5->ui_desp_3c_src_z_size_0 = 1;

            DescT5->ui_desp_44_r2y_y2r_mat_00 = 1024;
            DescT5->ui_desp_4c_r2y_y2r_mat_11 = 1024;
            DescT5->ui_desp_58_r2y_y2r_mat_22 = 1024;

            DescT5->ui_desp_64_src_x_stride_1 = pInfoNN->src_stride_w;
            DescT5->ui_desp_6c_src_y_stride_1 = batch * pInfoNN->src_stride_h;

            DescT5->ui_desp_74_src_x_size_1 = pInfoNN->shape_w;
            DescT5->ui_desp_78_src_y_size_1 = batch * pInfoNN->shape_h;

            DescT5->ui_desp_7c_src_z_size_1 = 1;

            pInfoNN->src_addr_offset = descDiff + 0x5C;
            pInfoNN->dst_addr_offset = descDiff + 0x20;
            pInfoNN->dummy_addr_offset = descDiff + 0x1C;
            if (shape->inputBindings != nullptr && shape->outputBindings != nullptr) {
                shape->inputBindings->AppendNormal(pInfoNN->src_addr_offset, 0);
                shape->inputBindings->AppendNormal(pInfoNN->dummy_addr_offset, 0);  // dummy UV is input
                shape->outputBindings->AppendNormal(pInfoNN->dst_addr_offset, 0);
            }

    } else if (type == EDMA_DESC_TYPE15){

        uint32_t *des = (uint32_t *)currDesc;
        memset(des, 0, sizeof(st_edmaDescType15));

        DescT15->ui_desp_00_type = EDMA_DESC_TYPE15;
#ifdef DISABLE_IOMMU
        DescT15->ui_desp_00_aruser = 0x0;
        DescT15->ui_desp_00_awuser = 0x0;
#else
        DescT15->ui_desp_00_aruser = 0x2;
        DescT15->ui_desp_00_awuser = 0x2;
#endif
        DescT15->ui_desp_00_des_id = pInfoNN->desc_id;


        if (pInfoNN->inFormat == EDMA_FMT_DEPTHTOSPACE) {
            //int des_number = pInfoNN->shape_h, des_idx = 0;
            __u16 des_idx = 0;

            EDMA_LOG_INFO("EDMA_FMT_DEPTHTOSPACE #5-5 type15\n");
            EDMA_LOG_INFO("block_size = %d, dst_type_size = %d\n", pInfoNN->block_size, pInfoNN->dst_type_size);


            for (des_idx = 0; des_idx < pInfoNN->shape_h; des_idx++ ) {
                DescT15 = (st_edmaDescType15 *)((__u8 *)currDesc + (__u32)(des_idx * sizeof(st_edmaDescType15)));
                memset(DescT15, 0, sizeof(st_edmaDescType15));

                DescT15->ui_desp_00_type = EDMA_DESC_TYPE15;
#ifdef DISABLE_IOMMU
                DescT15->ui_desp_00_aruser = 0x0;
                DescT15->ui_desp_00_awuser = 0x0;
#else
                DescT15->ui_desp_00_aruser = 0x2;
                DescT15->ui_desp_00_awuser = 0x2;
#endif
                DescT15->ui_desp_00_des_id = pInfoNN->desc_id;

                EDMA_LOG_INFO("des_idx = %d, DescT15 = %p, sizeof(DescT15) = %zu\n", des_idx, (void *)DescT15, sizeof(st_edmaDescType15));
                DescT15->ui_desp_34_src_x_size_0 = pInfoNN->shape_c * pInfoNN->src_type_size;
                DescT15->ui_desp_34_dst_x_size_0 = pInfoNN->block_size * pInfoNN->dst_type_size;

                DescT15->ui_desp_38_src_y_size_0 = pInfoNN->shape_w;
                DescT15->ui_desp_38_dst_y_size_0 = pInfoNN->block_size;

                DescT15->ui_desp_24_src_x_stride_0 = pInfoNN->src_stride_c * pInfoNN->src_type_size;

                DescT15->ui_desp_28_dst_x_stride_0 = pInfoNN->block_size * pInfoNN->dst_type_size * pInfoNN->shape_w;

                DescT15->ui_desp_2c_src_y_stride_0 = pInfoNN->src_stride_w * pInfoNN->src_type_size;

                DescT15->ui_desp_30_dst_y_stride_0 = pInfoNN->block_size * pInfoNN->dst_type_size;


                //DescT0->ui_desp_3c_src_z_size_0 = pInfoNN->shape_n * pInfoNN->shape_h;
                //DescT0->ui_desp_3c_dst_z_size_0 = pInfoNN->shape_n * pInfoNN->shape_h;
                DescT15->ui_desp_3c_src_z_size_0 = 1;
                DescT15->ui_desp_3c_dst_z_size_0 = pInfoNN->shape_w;
                DescT15->ui_desp_90_ar_sender_0 = 1; /*src0 read request, max burst length = 16*/
                DescT15->ui_desp_90_r_receiver_0 = pInfoNN->block_size * pInfoNN->dst_type_size - 1; /*cut data to 2 byte to pipeline*/
                DescT15->ui_desp_94_aw_sender_0 = 2; /*dst0 write request, max burst length = 8*/
                DescT15->ui_desp_94_w_sender_0 = pInfoNN->block_size * pInfoNN->dst_type_size - 1; /*dst write data pipeline send 2 bytes*/
                DescT15->ui_desp_98_reader = 1; /*reader act 1 plane*/
                DescT15->ui_desp_98_writer = 1; /*writer act 1 plane*/
                DescT15->ui_desp_98_pipe_enable = 0x11; /*pipeline enable src0/dst0*/

            }
            pInfoNN->src_addr_offset = descDiff + 0x1C;
            pInfoNN->dst_addr_offset = descDiff + 0x20;
            if (shape->inputBindings != nullptr && shape->outputBindings != nullptr) {
                shape->inputBindings->AppendNormal(pInfoNN->src_addr_offset, 0);
                shape->outputBindings->AppendNormal(pInfoNN->dst_addr_offset, 0);
            }

        } else if(pInfoNN->inFormat == EDMA_FMT_DEPTHTOSPACE_FAST) {
            edma_D2SFast(shape, currDesc);
            pInfoNN->src_addr_offset = descDiff + 0x1C;
            pInfoNN->dst_addr_offset = descDiff + 0x20;
            pInfoNN->src_addr2_offset = descDiff + 0x5C;  // no need
            pInfoNN->dst_addr2_offset = descDiff + 0x60;  // dst + shape_w * (shape_c / block_size) * type_size
            if (shape->inputBindings != nullptr && shape->outputBindings != nullptr) {
                shape->inputBindings->AppendNormal(pInfoNN->src_addr_offset, 0);
                shape->outputBindings->AppendNormal(pInfoNN->dst_addr_offset, 0);
                shape->outputBindings->AppendNormal(pInfoNN->dst_addr2_offset, (pInfoNN->shape_w * (pInfoNN->shape_c / pInfoNN->block_size)) * pInfoNN->dst_type_size);
            }
        } else if (pInfoNN->inFormat == EDMA_FMT_S2D_FAST) {
            edma_S2DFast(shape, currDesc);
            pInfoNN->src_addr_offset = descDiff + 0x1C;
            pInfoNN->dst_addr_offset = descDiff + 0x20;
            pInfoNN->src_addr2_offset = descDiff + 0x5C;  // src + shape_w * type_size
            pInfoNN->dst_addr2_offset = descDiff + 0x60;  // no need
            if (shape->inputBindings != nullptr && shape->outputBindings != nullptr) {
                shape->inputBindings->AppendNormal(pInfoNN->src_addr_offset, 0);
                shape->inputBindings->AppendNormal(pInfoNN->src_addr2_offset, (pInfoNN->shape_w * pInfoNN->src_type_size));
                shape->outputBindings->AppendNormal(pInfoNN->dst_addr_offset, 0);
            }
        } else {

            EDMA_LOG_INFO("fillDescNN 4x3 special path #1!!\n");
            //des[7] = pInfoNN->src_addr_offset;
            //des[8] = pInfoNN->dst_addr_offset;
            des[9] = pInfoNN->src_stride_w;
            des[10] = pInfoNN->dst_stride_w;
            des[13] = ((3 * (pInfoNN->shape_w)) << 16) | (4 * (pInfoNN->shape_w));
            des[14] = ((pInfoNN->shape_h * pInfoNN->shape_n) << 16) | (pInfoNN->shape_h * pInfoNN->shape_n);
            des[15] = 0x00010001;
            des[34] = 0x00600000;
            des[36] = 0x000f0001;
            des[37] = 0x000b0002;
            des[38] = 0x11000011;

            pInfoNN->src_addr_offset = descDiff + 0x1C;
            pInfoNN->dst_addr_offset = descDiff + 0x20;
            if (shape->inputBindings != nullptr && shape->outputBindings != nullptr) {
                shape->inputBindings->AppendNormal(pInfoNN->src_addr_offset, 0);
                shape->outputBindings->AppendNormal(pInfoNN->dst_addr_offset, 0);
            }
        }
    }
    EDMA_LOG_INFO("pInfoNN->src_addr_offset = %d\n", pInfoNN->src_addr_offset);
    EDMA_LOG_INFO("pInfoNN->dst_addr_offset = %d\n", pInfoNN->dst_addr_offset);
    EDMA_LOG_INFO("pInfoNN->dummy_addr_offset = %d\n", pInfoNN->dummy_addr_offset);

    return 0;
}



int fillDescUFBC(st_edmaUnifyInfo *pInfo, void *currDesc, void *headDesc)
{
    st_edmaDescType5 *DescT5 = (st_edmaDescType5 *)currDesc;

    st_edma_InputShape *shape = &pInfo->st_info.infoShape;
    __u8 *pHead = (__u8 *)headDesc;
    __u8 *pCurrDesc = (__u8 *)currDesc;
    __u32 descDiff = (__u32)(pCurrDesc - pHead);

	EDMA_LOG_INFO("fillDescUFBC [edma30] st_edmaDescType5 size = %d #3\n", (int)sizeof(st_edmaDescType5));

    EDMA_LOG_INFO("shape->inBuf_addr = 0x%x\n", shape->inBuf_addr);
    EDMA_LOG_INFO("shape->outBuf_addr = 0x%x\n", shape->outBuf_addr);
    EDMA_LOG_INFO("shape->size_x = 0x%x\n", shape->size_x);
    EDMA_LOG_INFO("pInfo->info_type = %d\n", pInfo->info_type);
    EDMA_LOG_INFO("currDesc addr = %p\n", currDesc);
    EDMA_LOG_INFO("headDesc addr = %p\n", headDesc);

    memset(DescT5, 0, sizeof(st_edmaDescType5));

    DescT5->ui_desp_00_type = 5;
#ifdef DISABLE_IOMMU
    DescT5->ui_desp_00_aruser = 0x0;
    DescT5->ui_desp_00_awuser = 0x0;
#else
    DescT5->ui_desp_00_aruser = 0x2;
    DescT5->ui_desp_00_awuser = 0x2;
#endif

    DescT5->ui_desp_08_in_swap_mat = 0x8421;
    DescT5->ui_desp_08_out_swap_mat = 0x8421;

    /* Tile Info */
    DescT5->ui_desp_0c_ufbc_tile_width = shape->size_x;
    DescT5->ui_desp_0c_ufbc_tile_height= shape->size_y;
    DescT5->ui_desp_14_ufbc_x_offset = 0;
    DescT5->ui_desp_14_ufbc_y_offset = 0;

    /* Frame Info */
	//DescT5->ui_desp_10_ufbc_frame_width = shape->size_x;
	//DescT5->ui_desp_10_ufbc_frame_height = shape->size_y;

    /* UFBC Info */
	//DescT5->ui_desp_18_ufbc_payload_offset = shape->size_x * shape->size_y / 16;

    DescT5->ui_desp_1c_src_addr_0 = shape->inBuf_addr;
    DescT5->ui_desp_20_dst_addr_0 = shape->outBuf_addr;

    shape->src_addr_offset = descDiff + 0x1C;
    shape->dst_addr_offset = descDiff + 0x20;
    if (pInfo->inputBindings != nullptr && pInfo->outputBindings != nullptr) {
        pInfo->inputBindings->AppendNormal(shape->src_addr_offset, shape->inBuf_addr);
        pInfo->outputBindings->AppendNormal(shape->dst_addr_offset, shape->outBuf_addr);
    }
    DescT5->ui_desp_40_ufbc_align_payload_offset = 1; //payload alignment 256 byte
    DescT5->ui_desp_40_ufbc_yuv_transform = 1; //endbale yuv trans in AFBC encoder

    if (shape->uc_desc_type == EDMA_UFBC_ENCODE) {
        DescT5->ui_desp_04_format = EDMA_UFBC_RGBA8_TO_RGBA8_CODE;

        /* In/Out Info  */
        DescT5->ui_desp_34_src_x_size_0 = shape->cropShape.x_size*4;
        DescT5->ui_desp_38_src_y_size_0 = shape->cropShape.y_size;
        DescT5->ui_desp_3c_src_z_size_0 = 1;

        DescT5->ui_desp_24_src_x_stride_0 = shape->dst_stride_x * 4;
        DescT5->ui_desp_2c_src_y_stride_0 = 0;


        // 0x18: APU_EDMA3_DESP_18
        DescT5->ui_desp_34_dst_x_size_0 = 0; /*encode don't care*/
        DescT5->ui_desp_38_dst_y_size_0 = 0;

        DescT5->ui_desp_0c_ufbc_tile_width = ((shape->cropShape.x_size-1)/32 + 1)*32;
        DescT5->ui_desp_0c_ufbc_tile_height = shape->cropShape.y_size; //?


        DescT5->ui_desp_10_ufbc_frame_width = ((shape->size_x-1)/32 + 1)*32; //pixel number in a frame = width pixels
        DescT5->ui_desp_10_ufbc_frame_height = ((shape->size_y-1)/8 + 1)*8; //alignment 8

        DescT5->ui_desp_14_ufbc_x_offset = shape->cropShape.x_offset;
        DescT5->ui_desp_14_ufbc_y_offset = shape->cropShape.y_offset;


		DescT5->ui_desp_18_ufbc_payload_offset = (DescT5->ui_desp_10_ufbc_frame_width*DescT5->ui_desp_10_ufbc_frame_height)/16; /* header size */
    } else if (shape->uc_desc_type == EDMA_UFBC_DECODE) {
        DescT5->ui_desp_04_format = EDMA_UFBC_RGBA8_CODE_TO_RGBA8;
        if (shape->cropShape.x_size > 960)
            EDMA_LOG_ERR("shape->cropShape.x_size = %d > 960 might decode error!!!\n", shape->cropShape.x_size);
        /* In/Out Info  */
        DescT5->ui_desp_34_dst_x_size_0 = shape->cropShape.x_size*4;
        DescT5->ui_desp_38_dst_y_size_0 = shape->cropShape.y_size;
        DescT5->ui_desp_3c_dst_z_size_0 = 1;

        DescT5->ui_desp_28_dst_x_stride_0 = shape->dst_stride_x*4;
        DescT5->ui_desp_30_dst_y_stride_0 = 0;

        DescT5->ui_desp_34_src_x_size_0 = 0; /*decode don't care*/
        DescT5->ui_desp_38_src_y_size_0 = 0;


        DescT5->ui_desp_0c_ufbc_tile_width = shape->cropShape.x_size;
        DescT5->ui_desp_0c_ufbc_tile_height = shape->cropShape.y_size;

        DescT5->ui_desp_10_ufbc_frame_width = shape->size_x; //pixel number in a frame = width pixels
        DescT5->ui_desp_10_ufbc_frame_height = shape->size_y; //alignment 8


        DescT5->ui_desp_14_ufbc_x_offset = shape->cropShape.x_offset;
        DescT5->ui_desp_14_ufbc_y_offset = shape->cropShape.y_offset;

        DescT5->ui_desp_18_ufbc_payload_offset = (DescT5->ui_desp_10_ufbc_frame_width*DescT5->ui_desp_10_ufbc_frame_height)/16; /* header size */

	}

	return 0;
}

int fillDescSlice(st_edmaUnifyInfo *pInfo, void *currDesc, void *headDesc)
{
    if (pInfo == nullptr || currDesc == nullptr || headDesc == nullptr) {
        EDMA_LOG_ERR("Invalid arguments, curr %p, head %p\n", currDesc, headDesc);
        return -1;
    }

    st_edmaDescType0* descT0 = reinterpret_cast<st_edmaDescType0 *>(currDesc);
    st_edmaDescInfoType0* infoT0 = &pInfo->st_info.info0;
    const uint32_t offsetBase =
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(currDesc) - reinterpret_cast<uintptr_t>(headDesc));

    EDMA_LOG_INFO("info->ui_src_addr_0 = 0x%x\n", infoT0->ui_src_addr_0);
    EDMA_LOG_INFO("info->ui_dst_addr_0 = 0x%x\n", infoT0->ui_dst_addr_0);
    EDMA_LOG_INFO("input shape = %u x %u x %u\n",
                  infoT0->ui_src_size_x, infoT0->ui_src_size_y, infoT0->ui_src_size_z);
    EDMA_LOG_INFO("input pitch = %u x %u x %u\n",
                  infoT0->ui_src_pitch_x_0, infoT0->ui_src_pitch_y_0, infoT0->ui_src_pitch_z_0);
    EDMA_LOG_INFO("output shape = %u x %u x %u\n",
                  infoT0->ui_dst_size_x, infoT0->ui_dst_size_y, infoT0->ui_dst_size_z);
    EDMA_LOG_INFO("output pitch = %u x %u x %u\n",
                  infoT0->ui_dst_pitch_x_0, infoT0->ui_dst_pitch_y_0, infoT0->ui_dst_pitch_z_0);
    EDMA_LOG_INFO("currDesc %p, headDesc %p\n", currDesc, headDesc);

    memset(descT0, 0, sizeof(st_edmaDescType0));
    descT0->ui_desp_00_type = 0;  // Use type 0 descriptor
#ifdef DISABLE_IOMMU
    descT0->ui_desp_00_aruser = 0;
    descT0->ui_desp_00_awuser = 0;
#else
    descT0->ui_desp_00_aruser = 0x2;
    descT0->ui_desp_00_awuser = 0x2;
#endif
    descT0->ui_desp_00_des_id = infoT0->uc_desc_id;
    descT0->ui_desp_04_format = 0;  // 0: normal mode
    descT0->ui_desp_04_safe_mode = 1;
    descT0->ui_desp_1c_src_addr_0 = infoT0->ui_src_addr_0;
    descT0->ui_desp_20_dst_addr_0 = infoT0->ui_dst_addr_0;
    descT0->ui_desp_24_src_x_stride_0 = infoT0->ui_src_pitch_x_0;
    descT0->ui_desp_28_dst_x_stride_0 = infoT0->ui_dst_pitch_x_0;
    descT0->ui_desp_2c_src_y_stride_0 = infoT0->ui_src_pitch_y_0;
    descT0->ui_desp_30_dst_y_stride_0 = infoT0->ui_dst_pitch_y_0;
    descT0->ui_desp_34_src_x_size_0 = infoT0->ui_src_size_x;
    descT0->ui_desp_34_dst_x_size_0 = infoT0->ui_dst_size_x;
    descT0->ui_desp_38_src_y_size_0 = infoT0->ui_src_size_y;
    descT0->ui_desp_38_dst_y_size_0 = infoT0->ui_dst_size_y;
    descT0->ui_desp_3c_src_z_size_0 = infoT0->ui_src_size_z;
    descT0->ui_desp_3c_dst_z_size_0 = infoT0->ui_dst_size_z;

    if (pInfo->inputBindings != nullptr && pInfo->outputBindings != nullptr) {
        pInfo->inputBindings->AppendNormal(offsetBase + 0x1C, infoT0->ui_src_addr_0);
        pInfo->outputBindings->AppendNormal(offsetBase + 0x20, infoT0->ui_dst_addr_0);
    }

    return 0;
}

int fillDescPad(st_edmaUnifyInfo *pInfo, void *currDesc, void *headDesc)
{
    if (pInfo == nullptr || currDesc == nullptr || headDesc == nullptr) {
        EDMA_LOG_ERR("Invalid arguments, curr %p, head %p\n", currDesc, headDesc);
        return -1;
    }

    st_edmaDescType0* descT0 = reinterpret_cast<st_edmaDescType0 *>(currDesc);
    st_edmaDescInfoPad* infoPad = &pInfo->st_info.infoPad;
    const uint32_t offsetBase =
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(currDesc) - reinterpret_cast<uintptr_t>(headDesc));

    EDMA_LOG_INFO("mode = %u, padding constant = 0x%u\n",
                  static_cast<uint32_t>(infoPad->uc_mode), infoPad->ui_pad_constant);
    EDMA_LOG_INFO("input shape = %u x %u, element size = %u\n",
                  infoPad->ui_src_size_x, infoPad->ui_src_size_y,
                  static_cast<uint32_t>(infoPad->uc_elem_size));
    EDMA_LOG_INFO("input X stride %u, output X stride %u\n",
                  infoPad->ui_src_stride_x, infoPad->ui_dst_stride_x);
    EDMA_LOG_INFO("paddings X = (%u, %u) paddings Y = (%u, %u)\n",
                  static_cast<uint32_t>(infoPad->ui_paddings_before_x),
                  static_cast<uint32_t>(infoPad->ui_paddings_after_x),
                  static_cast<uint32_t>(infoPad->ui_paddings_before_y),
                  static_cast<uint32_t>(infoPad->ui_paddings_after_y));
    EDMA_LOG_INFO("currDesc %p, headDesc %p\n", currDesc, headDesc);

    const uint32_t dstXSize = infoPad->ui_src_size_x + infoPad->ui_paddings_before_x
                            + infoPad->ui_paddings_after_x;
    const uint32_t dstYSize = infoPad->ui_src_size_y + infoPad->ui_paddings_before_y
                            + infoPad->ui_paddings_after_y;
    const uint32_t dstStart = infoPad->ui_paddings_before_y * infoPad->ui_dst_stride_x
                            + infoPad->ui_paddings_before_x;

    if (infoPad->uc_mode == EDMA_CONSTANT_PADDING) {
        memset(descT0, 0, 2 * sizeof(st_edmaDescType0));  // constant padding needs 2 descriptors

        // First descriptor, where fill_const is 32 bit
        descT0->ui_desp_00_type = 0;  // use type 0 descriptor
#ifdef DISABLE_IOMMU
        descT0->ui_desp_00_aruser = 0;
        descT0->ui_desp_00_awuser = 0;
#else
        descT0->ui_desp_00_aruser = 0x2;
        descT0->ui_desp_00_awuser = 0x2;
#endif
        descT0->ui_desp_00_des_id = infoPad->uc_desc_id;
        descT0->ui_desp_04_format = 1;  // 1: fill mode
        descT0->ui_desp_04_safe_mode = 1;
        if (infoPad->uc_elem_size == 1) {
            descT0->ui_desp_08_fill_const = (infoPad->ui_pad_constant & 0xff) |
                                            ((infoPad->ui_pad_constant & 0xff) << 8) |
                                            ((infoPad->ui_pad_constant & 0xff) << 16) |
                                            ((infoPad->ui_pad_constant & 0xff) << 24);
        } else if (infoPad->uc_elem_size == 2) {
            descT0->ui_desp_08_fill_const = (infoPad->ui_pad_constant & 0xffff) |
                                            ((infoPad->ui_pad_constant & 0xffff) << 16);
        } else {
            descT0->ui_desp_08_fill_const = infoPad->ui_pad_constant;
        }
        descT0->ui_desp_1c_src_addr_0 = 0;  // unused in fill mode
        descT0->ui_desp_20_dst_addr_0 = infoPad->ui_dst_addr;
        descT0->ui_desp_24_src_x_stride_0 = 0;  // unused in fill mode
        descT0->ui_desp_28_dst_x_stride_0 = infoPad->ui_dst_stride_x * infoPad->uc_elem_size;
        descT0->ui_desp_2c_src_y_stride_0 = 0;  // unused in fill mode
        descT0->ui_desp_30_dst_y_stride_0 = descT0->ui_desp_28_dst_x_stride_0 * infoPad->ui_dst_stride_y;
        descT0->ui_desp_34_src_x_size_0 = 0;
        descT0->ui_desp_34_dst_x_size_0 = dstXSize * infoPad->uc_elem_size;
        descT0->ui_desp_38_src_y_size_0 = 0;
        descT0->ui_desp_38_dst_y_size_0 = dstYSize;
        descT0->ui_desp_3c_src_z_size_0 = 0;
        descT0->ui_desp_3c_dst_z_size_0 = 1;
        if (pInfo->inputBindings != nullptr && pInfo->outputBindings != nullptr) {
            pInfo->outputBindings->AppendNormal(offsetBase + 0x20, descT0->ui_desp_20_dst_addr_0);
        }

        // Second descriptor
        descT0++;
        descT0->ui_desp_00_type = 0;  // use type 0 descriptor
#ifdef DISABLE_IOMMU
        descT0->ui_desp_00_aruser = 0;
        descT0->ui_desp_00_awuser = 0;
#else
        descT0->ui_desp_00_aruser = 0x2;
        descT0->ui_desp_00_awuser = 0x2;
#endif
        descT0->ui_desp_00_des_id = infoPad->uc_desc_id + 1;
        descT0->ui_desp_04_format = 0;  // 0: normal mode
        descT0->ui_desp_04_safe_mode = 1;
        descT0->ui_desp_1c_src_addr_0 = infoPad->ui_src_addr;
        descT0->ui_desp_20_dst_addr_0 = infoPad->ui_dst_addr + dstStart * infoPad->uc_elem_size;
        descT0->ui_desp_24_src_x_stride_0 = infoPad->ui_src_stride_x * infoPad->uc_elem_size;
        descT0->ui_desp_28_dst_x_stride_0 = infoPad->ui_dst_stride_x * infoPad->uc_elem_size;
        descT0->ui_desp_2c_src_y_stride_0 = descT0->ui_desp_24_src_x_stride_0 * infoPad->ui_src_stride_y;
        descT0->ui_desp_30_dst_y_stride_0 = descT0->ui_desp_28_dst_x_stride_0 * infoPad->ui_dst_stride_y;
        descT0->ui_desp_34_src_x_size_0 = infoPad->ui_src_size_x * infoPad->uc_elem_size;
        descT0->ui_desp_34_dst_x_size_0 = infoPad->ui_src_size_x * infoPad->uc_elem_size;
        descT0->ui_desp_38_src_y_size_0 = infoPad->ui_src_size_y;
        descT0->ui_desp_38_dst_y_size_0 = infoPad->ui_src_size_y;
        descT0->ui_desp_3c_src_z_size_0 = 1;
        descT0->ui_desp_3c_dst_z_size_0 = 1;
        if (pInfo->inputBindings != nullptr && pInfo->outputBindings != nullptr) {
            pInfo->inputBindings->AppendNormal(offsetBase + sizeof(st_edmaDescType0) + 0x1C, descT0->ui_desp_1c_src_addr_0);
            pInfo->outputBindings->AppendNormal(offsetBase + sizeof(st_edmaDescType0) + 0x20, descT0->ui_desp_20_dst_addr_0);
        }
    } else if (infoPad->uc_mode == EDMA_EDGE_PADDING) {
        memset(descT0, 0, 5 * sizeof(st_edmaDescType0));  // edge padding needs 5 descriptors

        // First descriptor (central part)
        descT0->ui_desp_00_type = 0;  // use type 0 descriptor
#ifdef DISABLE_IOMMU
        descT0->ui_desp_00_aruser = 0;
        descT0->ui_desp_00_awuser = 0;
#else
        descT0->ui_desp_00_aruser = 0x2;
        descT0->ui_desp_00_awuser = 0x2;
#endif
        descT0->ui_desp_00_des_id = infoPad->uc_desc_id;
        descT0->ui_desp_04_format = 0;  // 0: normal mode
        descT0->ui_desp_04_safe_mode = 1;
        descT0->ui_desp_04_atomic_mode = 1;
        descT0->ui_desp_1c_src_addr_0 = infoPad->ui_src_addr;
        descT0->ui_desp_20_dst_addr_0 = infoPad->ui_dst_addr + dstStart * infoPad->uc_elem_size;
        descT0->ui_desp_24_src_x_stride_0 = infoPad->ui_src_stride_x * infoPad->uc_elem_size;
        descT0->ui_desp_28_dst_x_stride_0 = infoPad->ui_dst_stride_x * infoPad->uc_elem_size;
        descT0->ui_desp_2c_src_y_stride_0 = descT0->ui_desp_24_src_x_stride_0 * infoPad->ui_src_stride_y;
        descT0->ui_desp_30_dst_y_stride_0 = descT0->ui_desp_28_dst_x_stride_0 * infoPad->ui_dst_stride_y;
        descT0->ui_desp_34_src_x_size_0 = infoPad->ui_src_size_x * infoPad->uc_elem_size;
        descT0->ui_desp_34_dst_x_size_0 = infoPad->ui_src_size_x * infoPad->uc_elem_size;
        descT0->ui_desp_38_src_y_size_0 = infoPad->ui_src_size_y;
        descT0->ui_desp_38_dst_y_size_0 = infoPad->ui_src_size_y;
        descT0->ui_desp_3c_src_z_size_0 = 1;
        descT0->ui_desp_3c_dst_z_size_0 = 1;
        if (pInfo->inputBindings != nullptr && pInfo->outputBindings != nullptr) {
            pInfo->inputBindings->AppendNormal(offsetBase + 0x1C, descT0->ui_desp_1c_src_addr_0);
            pInfo->outputBindings->AppendNormal(offsetBase + 0x20, descT0->ui_desp_20_dst_addr_0);
        }

        // Second descriptor (left part)
        descT0++;
        descT0->ui_desp_00_type = 0;  // use type 0 descriptor
#ifdef DISABLE_IOMMU
        descT0->ui_desp_00_aruser = 0;
        descT0->ui_desp_00_awuser = 0;
#else
        descT0->ui_desp_00_aruser = 0x2;
        descT0->ui_desp_00_awuser = 0x2;
#endif
        descT0->ui_desp_00_des_id = infoPad->uc_desc_id + 1;
        descT0->ui_desp_04_format = 0;  // 0: normal mode
        descT0->ui_desp_04_safe_mode = 1;
        descT0->ui_desp_04_atomic_mode = 1;
        descT0->ui_desp_1c_src_addr_0 = infoPad->ui_dst_addr + infoPad->ui_paddings_before_x * infoPad->uc_elem_size;
        descT0->ui_desp_20_dst_addr_0 = infoPad->ui_dst_addr;
        descT0->ui_desp_24_src_x_stride_0 = infoPad->ui_dst_stride_x * infoPad->uc_elem_size;
        descT0->ui_desp_28_dst_x_stride_0 = infoPad->ui_dst_stride_x * infoPad->uc_elem_size;
        descT0->ui_desp_2c_src_y_stride_0 = 0;
        descT0->ui_desp_30_dst_y_stride_0 = infoPad->uc_elem_size;
        descT0->ui_desp_34_src_x_size_0 = infoPad->uc_elem_size;
        descT0->ui_desp_34_dst_x_size_0 = infoPad->uc_elem_size;
        descT0->ui_desp_38_src_y_size_0 = dstYSize;
        descT0->ui_desp_38_dst_y_size_0 = dstYSize;
        descT0->ui_desp_3c_src_z_size_0 = infoPad->ui_paddings_before_x;
        descT0->ui_desp_3c_dst_z_size_0 = infoPad->ui_paddings_before_x;
        if (infoPad->ui_paddings_before_x == 0) {
            descT0->ui_desp_04_read_tmp_buf = 1;
            descT0->ui_desp_04_write_tmp_buf = 1;
            descT0->ui_desp_34_src_x_size_0 = 1;
            descT0->ui_desp_34_dst_x_size_0 = 1;
            descT0->ui_desp_38_src_y_size_0 = 1;
            descT0->ui_desp_38_dst_y_size_0 = 1;
            descT0->ui_desp_3c_src_z_size_0 = 1;
            descT0->ui_desp_3c_dst_z_size_0 = 1;
        }
        if (pInfo->inputBindings != nullptr && pInfo->outputBindings != nullptr) {
            pInfo->inputBindings->AppendNormal(offsetBase + 1 * sizeof(st_edmaDescType0) + 0x1C, descT0->ui_desp_1c_src_addr_0);
            pInfo->outputBindings->AppendNormal(offsetBase + 1 * sizeof(st_edmaDescType0) + 0x20, descT0->ui_desp_20_dst_addr_0);
        }

        // Third descriptor (right part)
        descT0++;
        descT0->ui_desp_00_type = 0;  // use type 0 descriptor
#ifdef DISABLE_IOMMU
        descT0->ui_desp_00_aruser = 0;
        descT0->ui_desp_00_awuser = 0;
#else
        descT0->ui_desp_00_aruser = 0x2;
        descT0->ui_desp_00_awuser = 0x2;
#endif
        descT0->ui_desp_00_des_id = infoPad->uc_desc_id + 2;
        descT0->ui_desp_04_format = 0;  // 0: normal mode
        descT0->ui_desp_04_safe_mode = 1;
        descT0->ui_desp_04_atomic_mode = 1;
        descT0->ui_desp_1c_src_addr_0 = infoPad->ui_dst_addr + (infoPad->ui_paddings_before_x + infoPad->ui_src_size_x - 1) * infoPad->uc_elem_size;
        descT0->ui_desp_20_dst_addr_0 = infoPad->ui_dst_addr + (infoPad->ui_paddings_before_x + infoPad->ui_src_size_x) * infoPad->uc_elem_size;
        descT0->ui_desp_24_src_x_stride_0 = infoPad->ui_dst_stride_x * infoPad->uc_elem_size;
        descT0->ui_desp_28_dst_x_stride_0 = infoPad->ui_dst_stride_x * infoPad->uc_elem_size;
        descT0->ui_desp_2c_src_y_stride_0 = 0;
        descT0->ui_desp_30_dst_y_stride_0 = infoPad->uc_elem_size;
        descT0->ui_desp_34_src_x_size_0 = infoPad->uc_elem_size;
        descT0->ui_desp_34_dst_x_size_0 = infoPad->uc_elem_size;
        descT0->ui_desp_38_src_y_size_0 = dstYSize;
        descT0->ui_desp_38_dst_y_size_0 = dstYSize;
        descT0->ui_desp_3c_src_z_size_0 = infoPad->ui_paddings_after_x;
        descT0->ui_desp_3c_dst_z_size_0 = infoPad->ui_paddings_after_x;
        if (infoPad->ui_paddings_after_x == 0) {
            descT0->ui_desp_04_read_tmp_buf = 1;
            descT0->ui_desp_04_write_tmp_buf = 1;
            descT0->ui_desp_34_src_x_size_0 = 1;
            descT0->ui_desp_34_dst_x_size_0 = 1;
            descT0->ui_desp_38_src_y_size_0 = 1;
            descT0->ui_desp_38_dst_y_size_0 = 1;
            descT0->ui_desp_3c_src_z_size_0 = 1;
            descT0->ui_desp_3c_dst_z_size_0 = 1;
        }
        if (pInfo->inputBindings != nullptr && pInfo->outputBindings != nullptr) {
            pInfo->inputBindings->AppendNormal(offsetBase + 2 * sizeof(st_edmaDescType0) + 0x1C, descT0->ui_desp_1c_src_addr_0);
            pInfo->outputBindings->AppendNormal(offsetBase + 2 * sizeof(st_edmaDescType0) + 0x20, descT0->ui_desp_20_dst_addr_0);
        }

        // Fourth descriptor (top part)
        descT0++;
        descT0->ui_desp_00_type = 0;  // use type 0 descriptor
#ifdef DISABLE_IOMMU
        descT0->ui_desp_00_aruser = 0;
        descT0->ui_desp_00_awuser = 0;
#else
        descT0->ui_desp_00_aruser = 0x2;
        descT0->ui_desp_00_awuser = 0x2;
#endif
        descT0->ui_desp_00_des_id = infoPad->uc_desc_id + 3;
        descT0->ui_desp_04_format = 0;  // 0: normal mode
        descT0->ui_desp_04_safe_mode = 1;
        descT0->ui_desp_04_atomic_mode = 1;
        descT0->ui_desp_1c_src_addr_0 = infoPad->ui_dst_addr + infoPad->ui_dst_stride_x * infoPad->ui_paddings_before_y * infoPad->uc_elem_size;
        descT0->ui_desp_20_dst_addr_0 = infoPad->ui_dst_addr;
        descT0->ui_desp_24_src_x_stride_0 = 1;
        descT0->ui_desp_28_dst_x_stride_0 = 1;
        descT0->ui_desp_2c_src_y_stride_0 = 0;
        descT0->ui_desp_30_dst_y_stride_0 = infoPad->ui_dst_stride_x * infoPad->uc_elem_size;
        descT0->ui_desp_34_src_x_size_0 = dstXSize * infoPad->uc_elem_size;
        descT0->ui_desp_34_dst_x_size_0 = dstXSize * infoPad->uc_elem_size;
        descT0->ui_desp_38_src_y_size_0 = 1;
        descT0->ui_desp_38_dst_y_size_0 = 1;
        descT0->ui_desp_3c_src_z_size_0 = infoPad->ui_paddings_before_y;
        descT0->ui_desp_3c_dst_z_size_0 = infoPad->ui_paddings_before_y;
        if (infoPad->ui_paddings_before_y == 0) {
            descT0->ui_desp_04_read_tmp_buf = 1;
            descT0->ui_desp_04_write_tmp_buf = 1;
            descT0->ui_desp_34_src_x_size_0 = 1;
            descT0->ui_desp_34_dst_x_size_0 = 1;
            descT0->ui_desp_38_src_y_size_0 = 1;
            descT0->ui_desp_38_dst_y_size_0 = 1;
            descT0->ui_desp_3c_src_z_size_0 = 1;
            descT0->ui_desp_3c_dst_z_size_0 = 1;
        }
        if (pInfo->inputBindings != nullptr && pInfo->outputBindings != nullptr) {
            pInfo->inputBindings->AppendNormal(offsetBase + 3 * sizeof(st_edmaDescType0) + 0x1C, descT0->ui_desp_1c_src_addr_0);
            pInfo->outputBindings->AppendNormal(offsetBase + 3 * sizeof(st_edmaDescType0) + 0x20, descT0->ui_desp_20_dst_addr_0);
        }

        // Fifth descriptor (bottom part)
        descT0++;
        descT0->ui_desp_00_type = 0;  // use type 0 descriptor
#ifdef DISABLE_IOMMU
        descT0->ui_desp_00_aruser = 0;
        descT0->ui_desp_00_awuser = 0;
#else
        descT0->ui_desp_00_aruser = 0x2;
        descT0->ui_desp_00_awuser = 0x2;
#endif
        descT0->ui_desp_00_des_id = infoPad->uc_desc_id + 4;
        descT0->ui_desp_04_format = 0;  // 0: normal mode
        descT0->ui_desp_04_safe_mode = 1;
        descT0->ui_desp_04_atomic_mode = 0;  // do not need atomic_mode for the last descriptor
        descT0->ui_desp_1c_src_addr_0 = infoPad->ui_dst_addr + infoPad->ui_dst_stride_x * (infoPad->ui_paddings_before_y + infoPad->ui_src_size_y - 1) * infoPad->uc_elem_size;
        descT0->ui_desp_20_dst_addr_0 = infoPad->ui_dst_addr + infoPad->ui_dst_stride_x * (infoPad->ui_paddings_before_y + infoPad->ui_src_size_y) * infoPad->uc_elem_size;
        descT0->ui_desp_24_src_x_stride_0 = 1;
        descT0->ui_desp_28_dst_x_stride_0 = 1;
        descT0->ui_desp_2c_src_y_stride_0 = 0;
        descT0->ui_desp_30_dst_y_stride_0 = infoPad->ui_dst_stride_x * infoPad->uc_elem_size;
        descT0->ui_desp_34_src_x_size_0 = dstXSize * infoPad->uc_elem_size;
        descT0->ui_desp_34_dst_x_size_0 = dstXSize * infoPad->uc_elem_size;
        descT0->ui_desp_38_src_y_size_0 = 1;
        descT0->ui_desp_38_dst_y_size_0 = 1;
        descT0->ui_desp_3c_src_z_size_0 = infoPad->ui_paddings_after_y;
        descT0->ui_desp_3c_dst_z_size_0 = infoPad->ui_paddings_after_y;
        if (infoPad->ui_paddings_after_y == 0) {
            descT0->ui_desp_04_read_tmp_buf = 1;
            descT0->ui_desp_04_write_tmp_buf = 1;
            descT0->ui_desp_34_src_x_size_0 = 1;
            descT0->ui_desp_34_dst_x_size_0 = 1;
            descT0->ui_desp_38_src_y_size_0 = 1;
            descT0->ui_desp_38_dst_y_size_0 = 1;
            descT0->ui_desp_3c_src_z_size_0 = 1;
            descT0->ui_desp_3c_dst_z_size_0 = 1;
        }
        if (pInfo->inputBindings != nullptr && pInfo->outputBindings != nullptr) {
            pInfo->inputBindings->AppendNormal(offsetBase + 4 * sizeof(st_edmaDescType0) + 0x1C, descT0->ui_desp_1c_src_addr_0);
            pInfo->outputBindings->AppendNormal(offsetBase + 4 * sizeof(st_edmaDescType0) + 0x20, descT0->ui_desp_20_dst_addr_0);
        }
    } else {
        EDMA_LOG_ERR("Invalid padding mode %u\n", static_cast<uint32_t>(infoPad->uc_mode));
        return -1;
    }
    return 0;
}

int fillDescBayerToRGGB(st_edmaUnifyInfo *pInfo, void *currDesc, void *headDesc)
{
    if (pInfo == nullptr || currDesc == nullptr || headDesc == nullptr) {
        EDMA_LOG_ERR("Invalid arguments, curr %p, head %p\n", currDesc, headDesc);
        return -1;

    }

    st_edmaDescType15* descT15 = reinterpret_cast<st_edmaDescType15 *>(currDesc);
    st_edmaDescInfoBayer* bayerInfo = &pInfo->st_info.infoBayer;

    // Support BAYER_10 or BAYER_12 to RGGB_16 only
    if (bayerInfo->uc_in_bits != 10 && bayerInfo->uc_in_bits != 12) {
        EDMA_LOG_ERR("Invalid input bits = %u\n", static_cast<uint32_t>(bayerInfo->uc_in_bits));
        return -1;
    }
    if (bayerInfo->uc_out_bits != 16) {
        EDMA_LOG_ERR("Invalid output bits = %u\n", static_cast<uint32_t>(bayerInfo->uc_out_bits));
        return -1;
    }

    const uint32_t offsetBase = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(currDesc) - reinterpret_cast<uintptr_t>(headDesc));
    const uint32_t srcXSizeBytes = (bayerInfo->ui_src_size_x * 2 /*RG or GB*/ * static_cast<uint32_t>(bayerInfo->uc_in_bits)) / 8;
    const uint32_t dstXSizeBytes = (bayerInfo->ui_dst_size_x * 4 /*RGGB*/ * static_cast<uint32_t>(bayerInfo->uc_out_bits)) / 8;

    EDMA_LOG_INFO("Input size %u x %u, stride %u x %u, element size = %u (bits)\n",
                  bayerInfo->ui_src_size_x, bayerInfo->ui_src_size_y,
                  bayerInfo->ui_src_stride_x, bayerInfo->ui_src_stride_y,
                  static_cast<uint32_t>(bayerInfo->uc_in_bits));
    EDMA_LOG_INFO("Output size %u x %u, pitch %u x %u, element size = %u (bits)\n",
                  bayerInfo->ui_dst_size_x, bayerInfo->ui_dst_size_y,
                  bayerInfo->ui_dst_stride_x, bayerInfo->ui_dst_stride_y,
                  static_cast<uint32_t>(bayerInfo->uc_out_bits));
    EDMA_LOG_INFO("Input addr %u, output addr %u\n",
                  bayerInfo->ui_src_addr, bayerInfo->ui_dst_addr);
    EDMA_LOG_INFO("currDesc %p, headDesc %p\n", currDesc, headDesc);

    if (srcXSizeBytes > bayerInfo->ui_src_stride_x) {
        EDMA_LOG_ERR("Invalid input x stride %u > %u\n", srcXSizeBytes, bayerInfo->ui_src_stride_x);
        return -1;
    }

    if (dstXSizeBytes > bayerInfo->ui_dst_stride_x) {
        EDMA_LOG_ERR("Invalid output x stride %u > %u\n", dstXSizeBytes, bayerInfo->ui_dst_stride_x);
        return -1;
    }

    memset(descT15, 0, sizeof(st_edmaDescType15));
    descT15->ui_desp_00_type = 15;
#ifdef DISABLE_IOMMU
    descT15->ui_desp_00_aruser = 0;
    descT15->ui_desp_00_awuser = 0;
#else
    descT15->ui_desp_00_aruser = 0x2;
    descT15->ui_desp_00_awuser = 0x2;
#endif
    descT15->ui_desp_00_des_id = bayerInfo->uc_desc_id;
    descT15->ui_desp_1c_src_addr_0 = bayerInfo->ui_src_addr;
    descT15->ui_desp_20_dst_addr_0 = bayerInfo->ui_dst_addr;
    descT15->ui_desp_24_src_x_stride_0 = bayerInfo->ui_src_stride_x * 2;
    descT15->ui_desp_28_dst_x_stride_0 = bayerInfo->ui_dst_stride_x;
    descT15->ui_desp_2c_src_y_stride_0 = bayerInfo->ui_src_stride_y;
    descT15->ui_desp_30_dst_y_stride_0 = bayerInfo->ui_dst_stride_y;
    descT15->ui_desp_34_src_x_size_0 = srcXSizeBytes;
    descT15->ui_desp_34_dst_x_size_0 = dstXSizeBytes;
    descT15->ui_desp_38_src_y_size_0 = bayerInfo->ui_src_size_y / 2;
    descT15->ui_desp_38_dst_y_size_0 = bayerInfo->ui_dst_size_y;
    descT15->ui_desp_3c_src_z_size_0 = 1;
    descT15->ui_desp_3c_dst_z_size_0 = 1;
    descT15->ui_desp_5c_src_addr_1 = descT15->ui_desp_1c_src_addr_0 + bayerInfo->ui_src_stride_x;
    descT15->ui_desp_64_src_x_stride_1 = descT15->ui_desp_24_src_x_stride_0;
    descT15->ui_desp_6c_src_y_stride_1 = descT15->ui_desp_2c_src_y_stride_0;
    descT15->ui_desp_74_src_x_size_1 = descT15->ui_desp_34_src_x_size_0;
    descT15->ui_desp_74_dst_x_size_1 = 0;
    descT15->ui_desp_78_src_y_size_1 = descT15->ui_desp_38_src_y_size_0;
    descT15->ui_desp_78_dst_y_size_1 = 0;
    descT15->ui_desp_7c_src_z_size_1 = descT15->ui_desp_3c_src_z_size_0;
    descT15->ui_desp_7c_dst_z_size_1 = 0;
    if (bayerInfo->uc_in_bits == 10) {
        descT15->ui_desp_84_in_zero_padder_0 = 7;
        descT15->ui_desp_84_in_zero_padder_1 = 7;
    } else if (bayerInfo->uc_in_bits == 12) {
        descT15->ui_desp_84_in_zero_padder_0 = 0xb;
        descT15->ui_desp_84_in_zero_padder_1 = 0xb;
    }
    descT15->ui_desp_88_merger = 6;
    descT15->ui_desp_90_ar_sender_0 = 1;
    descT15->ui_desp_90_ar_sender_1 = 1;
    if (bayerInfo->uc_in_bits == 10) {
        descT15->ui_desp_90_r_receiver_0 = 4;
        descT15->ui_desp_90_r_receiver_1 = 4;
    } else if (bayerInfo->uc_in_bits == 12) {
        descT15->ui_desp_90_r_receiver_0 = 5;
        descT15->ui_desp_90_r_receiver_1 = 5;
    }
    descT15->ui_desp_94_aw_sender_0 = 2;
    descT15->ui_desp_94_w_sender_0 = 0xf;
    descT15->ui_desp_98_reader = 2;
    descT15->ui_desp_98_writer = 1;
    descT15->ui_desp_98_pipe_enable = 0x13;

    // Add bindings
    if (pInfo->inputBindings != nullptr && pInfo->outputBindings != nullptr) {
        pInfo->inputBindings->AppendNormal(offsetBase + 0x1C, descT15->ui_desp_1c_src_addr_0);
        pInfo->inputBindings->AppendNormal(offsetBase + 0x5C, descT15->ui_desp_5c_src_addr_1);
        pInfo->outputBindings->AppendNormal(offsetBase + 0x20, descT15->ui_desp_20_dst_addr_0);
    }

    return 0;
}

int fillDescRGGBToBayer(st_edmaUnifyInfo *pInfo, void *currDesc, void *headDesc)
{
    if (pInfo == nullptr || currDesc == nullptr || headDesc == nullptr) {
        EDMA_LOG_ERR("Invalid arguments, curr %p, head %p\n", currDesc, headDesc);
        return -1;

    }

    st_edmaDescType15* descT15 = reinterpret_cast<st_edmaDescType15 *>(currDesc);
    st_edmaDescInfoBayer* bayerInfo = &pInfo->st_info.infoBayer;

    // Support RGGB_16 to BAYER_10 or BAYER_12 only
    if (bayerInfo->uc_in_bits != 16) {
        EDMA_LOG_ERR("Invalid input bits = %u\n", static_cast<uint32_t>(bayerInfo->uc_in_bits));
        return -1;
    }
    if (bayerInfo->uc_out_bits != 10 && bayerInfo->uc_out_bits != 12) {
        EDMA_LOG_ERR("Invalid output bits = %u\n", static_cast<uint32_t>(bayerInfo->uc_out_bits));
        return -1;
    }

    const uint32_t offsetBase = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(currDesc) - reinterpret_cast<uintptr_t>(headDesc));
    const uint32_t srcXSizeBytes = (bayerInfo->ui_src_size_x * 4 /*RGGB*/ * static_cast<uint32_t>(bayerInfo->uc_in_bits)) / 8;
    const uint32_t dstXSizeBytes = (bayerInfo->ui_dst_size_x * 2 /*RG or GB*/ * static_cast<uint32_t>(bayerInfo->uc_out_bits)) / 8;

    EDMA_LOG_INFO("Input size %u x %u, stride %u x %u, element size = %u (bits)\n",
                  bayerInfo->ui_src_size_x, bayerInfo->ui_src_size_y,
                  bayerInfo->ui_src_stride_x, bayerInfo->ui_src_stride_y,
                  static_cast<uint32_t>(bayerInfo->uc_in_bits));
    EDMA_LOG_INFO("Output size %u x %u, stride %u x %u, element size = %u (bits)\n",
                  bayerInfo->ui_dst_size_x, bayerInfo->ui_dst_size_y,
                  bayerInfo->ui_dst_stride_x, bayerInfo->ui_dst_stride_y,
                  static_cast<uint32_t>(bayerInfo->uc_out_bits));
    EDMA_LOG_INFO("Input addr %u, output addr %u\n",
                  bayerInfo->ui_src_addr, bayerInfo->ui_dst_addr);
    EDMA_LOG_INFO("currDesc %p, headDesc %p\n", currDesc, headDesc);

    if (srcXSizeBytes > bayerInfo->ui_src_stride_x) {
        EDMA_LOG_ERR("Invalid input x stride %u > %u\n", srcXSizeBytes, bayerInfo->ui_src_stride_x);
        return -1;
    }

    if (dstXSizeBytes > bayerInfo->ui_dst_stride_x) {
        EDMA_LOG_ERR("Invalid output x stride %u > %u\n", dstXSizeBytes, bayerInfo->ui_dst_stride_x);
        return -1;
    }

    memset(descT15, 0, sizeof(st_edmaDescType15));
    descT15->ui_desp_00_type = 15;
#ifdef DISABLE_IOMMU
    descT15->ui_desp_00_aruser = 0;
    descT15->ui_desp_00_awuser = 0;
#else
    descT15->ui_desp_00_aruser = 0x2;
    descT15->ui_desp_00_awuser = 0x2;
#endif
    descT15->ui_desp_00_des_id = bayerInfo->uc_desc_id;
    descT15->ui_desp_08_in_swap_mat = 0x8142;
    descT15->ui_desp_08_out_swap_mat = 0x8421;
    descT15->ui_desp_1c_src_addr_0 = bayerInfo->ui_src_addr;
    descT15->ui_desp_20_dst_addr_0 = bayerInfo->ui_dst_addr;
    descT15->ui_desp_24_src_x_stride_0 = bayerInfo->ui_src_stride_x;
    descT15->ui_desp_28_dst_x_stride_0 = bayerInfo->ui_dst_stride_x * 2;
    descT15->ui_desp_2c_src_y_stride_0 = bayerInfo->ui_src_stride_y;
    descT15->ui_desp_30_dst_y_stride_0 = bayerInfo->ui_dst_stride_y;
    descT15->ui_desp_34_src_x_size_0 = srcXSizeBytes;
    descT15->ui_desp_34_dst_x_size_0 = dstXSizeBytes;
    descT15->ui_desp_38_src_y_size_0 = bayerInfo->ui_src_size_y;
    descT15->ui_desp_38_dst_y_size_0 = bayerInfo->ui_dst_size_y / 2;
    descT15->ui_desp_3c_src_z_size_0 = 1;
    descT15->ui_desp_3c_dst_z_size_0 = 1;
    descT15->ui_desp_44_r2y_y2r_mat_00 = 0x400;
    descT15->ui_desp_4c_r2y_y2r_mat_11 = 0x400;
    descT15->ui_desp_54_r2y_y2r_mat_20 = 0x400;
    descT15->ui_desp_5c_src_addr_1 = 0;
    descT15->ui_desp_60_dst_addr_1 = descT15->ui_desp_20_dst_addr_0 + bayerInfo->ui_dst_stride_x;
    descT15->ui_desp_68_dst_x_stride_1 = descT15->ui_desp_28_dst_x_stride_0;
    descT15->ui_desp_70_dst_y_stride_1 = descT15->ui_desp_30_dst_y_stride_0;
    descT15->ui_desp_74_src_x_size_1 = 0;
    descT15->ui_desp_74_dst_x_size_1 = descT15->ui_desp_34_dst_x_size_0;
    descT15->ui_desp_78_src_y_size_1 = 0;
    descT15->ui_desp_78_dst_y_size_1 = descT15->ui_desp_38_dst_y_size_0;
    descT15->ui_desp_7c_src_z_size_1 = 0;
    descT15->ui_desp_7c_dst_z_size_1 = descT15->ui_desp_3c_dst_z_size_0;
    if (bayerInfo->uc_out_bits == 10) {
        descT15->ui_desp_80_out_extractor_0 = 2;
        descT15->ui_desp_80_out_extractor_1 = 2;
    } else if (bayerInfo->uc_out_bits == 12) {
        descT15->ui_desp_80_out_extractor_0 = 4;
        descT15->ui_desp_80_out_extractor_1 = 4;
    }
    descT15->ui_desp_88_splitter = 6;
    descT15->ui_desp_90_ar_sender_0 = 1;
    descT15->ui_desp_90_r_receiver_0 = 0xf;
    descT15->ui_desp_94_aw_sender_0 = 2;
    descT15->ui_desp_94_aw_sender_1 = 2;
    if (bayerInfo->uc_out_bits == 10) {
        descT15->ui_desp_94_w_sender_0 = 4;
        descT15->ui_desp_94_w_sender_1 = 4;
    } else if (bayerInfo->uc_out_bits == 12) {
        descT15->ui_desp_94_w_sender_0 = 5;
        descT15->ui_desp_94_w_sender_1 = 5;
    }
    descT15->ui_desp_98_reader = 1;
    descT15->ui_desp_98_writer = 2;
    descT15->ui_desp_98_pipe_enable = 0x31;

    // Add bindings
    if (pInfo->inputBindings != nullptr && pInfo->outputBindings != nullptr) {
        pInfo->inputBindings->AppendNormal(offsetBase + 0x1C, descT15->ui_desp_1c_src_addr_0);
        pInfo->outputBindings->AppendNormal(offsetBase + 0x20, descT15->ui_desp_20_dst_addr_0);
        pInfo->outputBindings->AppendNormal(offsetBase + 0x60, descT15->ui_desp_60_dst_addr_1);
    }

    return 0;
}
