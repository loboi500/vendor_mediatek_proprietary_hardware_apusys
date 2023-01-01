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

// Descriptor type 0 checker
static void checkFormat(st_edmaDescInfoType0 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if ((pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_NORMAL_MODE) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_FILL_MODE)/* &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_F16_TO_F32) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_F32_TO_F16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_I8_TO_F32) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_F32_TO_I8) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_I8_TO_I8) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_RAW_8_TO_RAW_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_RAW_10_TO_RAW_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_RAW_12_TO_RAW_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_RAW_14_TO_RAW_16)*/)
    {
        EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
    }
}

static void checkRawShift(st_edmaDescInfoType0 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if ((pst_in_edma_desc_info->uc_raw_shift!=RAW_SHIFT_ALIGN_TO_LSB) &&
        (pst_in_edma_desc_info->uc_raw_shift!=RAW_SHIFT_ALIGN_TO_LSB_LS_2_BITS) &&
        (pst_in_edma_desc_info->uc_raw_shift!=RAW_SHIFT_ALIGN_TO_LSB_LS_4_BITS) &&
        (pst_in_edma_desc_info->uc_raw_shift!=RAW_SHIFT_ALIGN_TO_LSB_LS_6_BITS) &&
        (pst_in_edma_desc_info->uc_raw_shift!=RAW_SHIFT_ALIGN_TO_LSB_LS_8_BITS))
    {
        EDMA_LOG_ERR("unsupported raw shift: %d\n", pst_in_edma_desc_info->uc_raw_shift);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_RAW_SHIFT);
    }
}

static void checkSrcSizeX(st_edmaDescInfoType0 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    unsigned char uc_data_unit = 0;
    unsigned int ui_data_bit_size = 0;

    switch (pst_in_edma_desc_info->uc_fmt)
    {
        case EDMA_FMT_NORMAL_MODE: case EDMA_FMT_I8_TO_F32: case EDMA_FMT_I8_TO_I8: case EDMA_FMT_RAW_8_TO_RAW_16:
            uc_data_unit = 8;
            break;
        case EDMA_FMT_F16_TO_F32:
            uc_data_unit = 16;
            break;
        case EDMA_FMT_F32_TO_F16: case EDMA_FMT_F32_TO_I8:
            uc_data_unit = 32;
            break;
        case EDMA_FMT_RAW_10_TO_RAW_16:
            uc_data_unit = 10;
            break;
        case EDMA_FMT_RAW_12_TO_RAW_16:
            uc_data_unit = 12;
            break;
        case EDMA_FMT_RAW_14_TO_RAW_16:
            uc_data_unit = 14;
            break;
        default: // others
            EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
            //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
            break;
    }

    ui_data_bit_size = pst_in_edma_desc_info->ui_src_size_x * 8;
    if (ui_data_bit_size != ((ui_data_bit_size/(unsigned int)uc_data_unit)*(unsigned int)uc_data_unit))
    {
        EDMA_LOG_ERR("ui_src_size_x*8 == %d*n for format #%d\n", uc_data_unit, pst_in_edma_desc_info->uc_fmt);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_X);
    }

    if (pst_in_edma_desc_info->ui_src_size_x > MAX_EDMA_SIZE_X)
    {
        EDMA_LOG_ERR("ui_src_size_x <= %d\n", MAX_EDMA_SIZE_X);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_X);
    }
}

static void checkSrcSizeY(st_edmaDescInfoType0 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if (pst_in_edma_desc_info->ui_src_size_y > MAX_EDMA_SIZE_Y)
    {
        EDMA_LOG_ERR("ui_src_size_y <= %d\n", MAX_EDMA_SIZE_Y);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_Y);
    }
}

static void checkSrcSizeZ(st_edmaDescInfoType0 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if (pst_in_edma_desc_info->ui_src_size_z > MAX_EDMA_SIZE_Z)
    {
        EDMA_LOG_ERR("ui_src_size_z <= %d\n", MAX_EDMA_SIZE_Z);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_Z);
    }
}

static void checkDstSizeX(st_edmaDescInfoType0 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    unsigned char uc_data_unit = 0;
    unsigned int ui_data_bit_size = 0;

    switch (pst_in_edma_desc_info->uc_fmt)
    {
        case EDMA_FMT_NORMAL_MODE: case EDMA_FMT_FILL_MODE: case EDMA_FMT_F32_TO_I8: case EDMA_FMT_I8_TO_I8:
            uc_data_unit = 8;
            break;
        case EDMA_FMT_F32_TO_F16: case EDMA_FMT_RAW_8_TO_RAW_16:  case EDMA_FMT_RAW_10_TO_RAW_16: case EDMA_FMT_RAW_12_TO_RAW_16: case EDMA_FMT_RAW_14_TO_RAW_16:
            uc_data_unit = 16;
            break;
        case EDMA_FMT_I8_TO_F32: case EDMA_FMT_F16_TO_F32:
            uc_data_unit = 32;
            break;
        default: // others
            EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
            //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
            break;
    }

    ui_data_bit_size = pst_in_edma_desc_info->ui_dst_size_x * 8;
    if (ui_data_bit_size != ((ui_data_bit_size/(unsigned int)uc_data_unit)*(unsigned int)uc_data_unit))
    {
        EDMA_LOG_ERR("ui_dst_size_x*8 == %d*n for format #%d\n", uc_data_unit, pst_in_edma_desc_info->uc_fmt);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_X);
    }

    if (pst_in_edma_desc_info->ui_dst_size_x > MAX_EDMA_SIZE_X)
    {
        EDMA_LOG_ERR("ui_dst_size_x <= %d\n", MAX_EDMA_SIZE_X);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_X);
    }

    switch (pst_in_edma_desc_info->uc_fmt)
    {
        case EDMA_FMT_NORMAL_MODE:
            if (pst_in_edma_desc_info->ui_dst_size_x != pst_in_edma_desc_info->ui_src_size_x)
            {
                EDMA_LOG_ERR("ui_dst_size_x == ui_src_size_x for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_X);
            }
            break;
        case EDMA_FMT_FILL_MODE:
            // do nothing
            break;
        default: // others
            EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
            //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
            break;
    }
}

static void checkDstSizeY(st_edmaDescInfoType0 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if (pst_in_edma_desc_info->ui_dst_size_y > MAX_EDMA_SIZE_Y)
    {
        EDMA_LOG_ERR("ui_dst_size_y <= %d\n", MAX_EDMA_SIZE_Y);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Y);
    }

    switch (pst_in_edma_desc_info->uc_fmt)
    {
        case EDMA_FMT_NORMAL_MODE:
            if (pst_in_edma_desc_info->ui_dst_size_y != pst_in_edma_desc_info->ui_src_size_y)
            {
                EDMA_LOG_ERR("ui_dst_size_y == ui_src_size_y for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Y);
            }
            break;
        case EDMA_FMT_FILL_MODE:
            // do nothing
            break;
        default: // others
            EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
            //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
            break;
    }
}

static void checkDstSizeZ(st_edmaDescInfoType0 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if (pst_in_edma_desc_info->ui_dst_size_z > MAX_EDMA_SIZE_Z)
    {
        EDMA_LOG_ERR("ui_dst_size_z <= %d\n", MAX_EDMA_SIZE_Z);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Z);
    }

    switch (pst_in_edma_desc_info->uc_fmt)
    {
        case EDMA_FMT_NORMAL_MODE:
            if (pst_in_edma_desc_info->ui_dst_size_z != pst_in_edma_desc_info->ui_src_size_z)
            {
                EDMA_LOG_ERR("ui_dst_size_z == ui_src_size_z for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Z);
            }
            break;
        case EDMA_FMT_FILL_MODE:
            // do nothing
            break;
        default: // others
            EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
            //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
            break;
    }
}

void fillDescType0(st_edmaDescInfoType0 *pst_edma_desc_info, st_edmaDescType0 *pst_edma_desc, int *pi_errorCode=NULL)
{
    EDMA_LOG_INFO("fillDescType0\n");
    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = pst_edma_desc_info->uc_desc_type;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_handshake_enable = 0; // TBD
    pst_edma_desc->ui_desp_00_reserved_0 = 0;

    // 0x04: APU_EDMA3_DESP_04
    checkFormat(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_format = pst_edma_desc_info->uc_fmt;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    checkRawShift(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 0;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 0;
    pst_edma_desc->ui_desp_04_write_tmp_buf = 0;
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0;
    pst_edma_desc->ui_desp_04_reserved_1 = 0;
    pst_edma_desc->ui_desp_04_raw_shift = pst_edma_desc_info->uc_raw_shift;
    pst_edma_desc->ui_desp_04_reserved_2 = 0;

    EDMA_LOG_INFO("Dec  Type: %d\n", pst_edma_desc->ui_desp_00_type);
    EDMA_LOG_INFO("     FMT : %d\n", pst_edma_desc->ui_desp_04_format);

    // 0x08: APU_EDMA3_DESP_08
    pst_edma_desc->ui_desp_08_fill_const = pst_edma_desc_info->ui_fill_value;

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_i2f_min = pst_edma_desc_info->fl_fix8tofp32_min;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_i2f_scale = pst_edma_desc_info->fl_fix8tofp32_scale;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_f2i_min = pst_edma_desc_info->fl_fp32tofix8_min;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_f2i_scale = pst_edma_desc_info->fl_fp32tofix8_scale;

    // 0x1C: APU_EDMA3_DESP_1C
    pst_edma_desc->ui_desp_1c_src_addr_0 = pst_edma_desc_info->ui_src_addr_0;

    // 0x20: APU_EDMA3_DESP_20
    pst_edma_desc->ui_desp_20_dst_addr_0 = pst_edma_desc_info->ui_dst_addr_0;

    // 0x24: APU_EDMA3_DESP_24
    pst_edma_desc->ui_desp_24_src_x_stride_0 = pst_edma_desc_info->ui_src_pitch_x_0;

    // 0x28: APU_EDMA3_DESP_28
    pst_edma_desc->ui_desp_28_dst_x_stride_0 = pst_edma_desc_info->ui_dst_pitch_x_0;

    // 0x2C: APU_EDMA3_DESP_2C
    pst_edma_desc->ui_desp_2c_src_y_stride_0 = pst_edma_desc_info->ui_src_pitch_y_0;

    // 0x30: APU_EDMA3_DESP_30
    pst_edma_desc->ui_desp_30_dst_y_stride_0 = pst_edma_desc_info->ui_dst_pitch_y_0;

    // 0x34: APU_EDMA3_DESP_34
    if (pst_edma_desc_info->uc_fmt != EDMA_FMT_FILL_MODE)
    {
        checkSrcSizeX(pst_edma_desc_info, pi_errorCode);
        //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    }
    pst_edma_desc->ui_desp_34_src_x_size_0 = pst_edma_desc_info->ui_src_size_x;
    checkDstSizeX(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_34_dst_x_size_0 = pst_edma_desc_info->ui_dst_size_x;

    // 0x38: APU_EDMA3_DESP_38
    if (pst_edma_desc_info->uc_fmt != EDMA_FMT_FILL_MODE)
    {
        checkSrcSizeY(pst_edma_desc_info, pi_errorCode);
        //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    }
    pst_edma_desc->ui_desp_38_src_y_size_0 = pst_edma_desc_info->ui_src_size_y;
    checkDstSizeY(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc_info->ui_dst_size_y;

    // 0x3c: APU_EDMA3_DESP_3C
    if (pst_edma_desc_info->uc_fmt != EDMA_FMT_FILL_MODE)
    {
        checkSrcSizeZ(pst_edma_desc_info, pi_errorCode);
        //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    }
    pst_edma_desc->ui_desp_3c_src_z_size_0 = pst_edma_desc_info->ui_src_size_z;
    checkDstSizeZ(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_3c_dst_z_size_0 = pst_edma_desc_info->ui_dst_size_z;

    EDMA_LOG_INFO("SRC  addr: 0x%08x\n", pst_edma_desc->ui_desp_1c_src_addr_0);
    EDMA_LOG_INFO("DST  addr: 0x%08x\n", pst_edma_desc->ui_desp_20_dst_addr_0);
    EDMA_LOG_INFO("SRC  size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_src_x_size_0, pst_edma_desc->ui_desp_38_src_y_size_0, pst_edma_desc->ui_desp_3c_src_z_size_0);
    EDMA_LOG_INFO("   stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_24_src_x_stride_0, pst_edma_desc->ui_desp_2c_src_y_stride_0);
}

// Descriptor type 1 checker
static void checkFormat(st_edmaDescInfoType1 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if ((pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_RGB_8_TO_RGBA_8)/* &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_RGBA_102_TO_RGBA_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_RGBA_F1110_TO_RGBA_F16)*/)
    {
        EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
    }
}

static void checkSrcSizeX(st_edmaDescInfoType1 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    unsigned char uc_data_unit = 0;
    unsigned int ui_data_bit_size = 0;

    switch (pst_in_edma_desc_info->uc_fmt)
    {
        case EDMA_FMT_RGB_8_TO_RGBA_8:
            uc_data_unit = 24;
            break;
        case EDMA_FMT_RGBA_102_TO_RGBA_16: case EDMA_FMT_RGBA_F1110_TO_RGBA_F16:
            uc_data_unit = 32;
            break;
        default: // others
            EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
            //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
            break;
    }

    ui_data_bit_size = pst_in_edma_desc_info->ui_src_size_x * 8;
    if (ui_data_bit_size != ((ui_data_bit_size/(unsigned int)uc_data_unit)*(unsigned int)uc_data_unit))
    {
        EDMA_LOG_ERR("ui_src_size_x*8 == %d*n for format #%d\n", uc_data_unit, pst_in_edma_desc_info->uc_fmt);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_X);
    }

    if (pst_in_edma_desc_info->uc_fmt == EDMA_FMT_RGB_8_TO_RGBA_8)
    {

        if (pst_in_edma_desc_info->ui_src_size_x > MAX_EDMA_RGB_8_TO_RGBA_8_SIZE_X)
        {
            EDMA_LOG_ERR("ui_src_size_x <= %d\n", MAX_EDMA_RGB_8_TO_RGBA_8_SIZE_X);
            //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_X);
        }
    }
    else // (pst_in_edma_desc_info->uc_fmt != EDMA_FMT_RGB_8_TO_RGBA_8)
    {
        if (pst_in_edma_desc_info->ui_src_size_x > MAX_EDMA_SIZE_X)
        {
            EDMA_LOG_ERR("ui_src_size_x <= %d\n", MAX_EDMA_SIZE_X);
            //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_X);
        }
    }
}

static void checkSrcSizeY(st_edmaDescInfoType1 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if (pst_in_edma_desc_info->ui_src_size_y > MAX_EDMA_SIZE_Y)
    {
        EDMA_LOG_ERR("ui_src_size_y <= %d\n", MAX_EDMA_SIZE_Y);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_Y);
    }
}

static void checkSrcSizeZ(st_edmaDescInfoType1 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if (pst_in_edma_desc_info->ui_src_size_z > MAX_EDMA_SIZE_Z)
    {
        EDMA_LOG_ERR("ui_src_size_z <= %d\n", MAX_EDMA_SIZE_Z);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_Z);
    }
}

static void checkDstSizeX(st_edmaDescInfoType1 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    unsigned char uc_data_unit = 0;
    unsigned int ui_data_bit_size = 0;

    switch (pst_in_edma_desc_info->uc_fmt)
    {
        case EDMA_FMT_RGB_8_TO_RGBA_8:
            uc_data_unit = 32;
            break;
        case EDMA_FMT_RGBA_102_TO_RGBA_16: case EDMA_FMT_RGBA_F1110_TO_RGBA_F16:
            uc_data_unit = 16;
            break;
        default: // others
            EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
            //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
            break;
    }

    ui_data_bit_size = pst_in_edma_desc_info->ui_dst_size_x * 8;
    if (ui_data_bit_size != ((ui_data_bit_size/(unsigned int)uc_data_unit)*(unsigned int)uc_data_unit))
    {
        EDMA_LOG_ERR("ui_dst_size_x*8 == %d*n for format #%d\n", uc_data_unit, pst_in_edma_desc_info->uc_fmt);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_X);
    }

    if (pst_in_edma_desc_info->ui_dst_size_x > MAX_EDMA_SIZE_X)
    {
        EDMA_LOG_ERR("ui_dst_size_x <= %d\n", MAX_EDMA_SIZE_X);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_X);
    }

    switch (pst_in_edma_desc_info->uc_fmt)
    {
        case EDMA_FMT_RGB_8_TO_RGBA_8:
            if (pst_in_edma_desc_info->ui_dst_size_x != ((pst_in_edma_desc_info->ui_src_size_x/3)*4))
            {
                EDMA_LOG_ERR("ui_dst_size_x == (ui_src_size_x/3)*4 for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_X);
            }
            break;
        default: // others
            EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
            //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
            break;
    }
}

static void checkDstSizeY(st_edmaDescInfoType1 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if (pst_in_edma_desc_info->ui_dst_size_y > MAX_EDMA_SIZE_Y)
    {
        EDMA_LOG_ERR("ui_dst_size_y <= %d\n", MAX_EDMA_SIZE_Y);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Y);
    }

    switch (pst_in_edma_desc_info->uc_fmt)
    {
        case EDMA_FMT_RGB_8_TO_RGBA_8:
            if (pst_in_edma_desc_info->ui_dst_size_y != pst_in_edma_desc_info->ui_src_size_y)
            {
                EDMA_LOG_ERR("ui_dst_size_y == ui_src_size_y for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Y);
            }
            break;
        default: // others
            EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
            //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
            break;
    }
}

static void checkDstSizeZ(st_edmaDescInfoType1 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if (pst_in_edma_desc_info->ui_dst_size_z > MAX_EDMA_SIZE_Z)
    {
        EDMA_LOG_ERR("ui_dst_size_z <= %d\n", MAX_EDMA_SIZE_Z);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Z);
    }

    switch (pst_in_edma_desc_info->uc_fmt)
    {
        case EDMA_FMT_RGB_8_TO_RGBA_8:
            if (pst_in_edma_desc_info->ui_dst_size_z != pst_in_edma_desc_info->ui_src_size_z)
            {
                EDMA_LOG_ERR("ui_dst_size_z == ui_src_size_z for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Z);
            }
            break;
        default: // others
            EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
            //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
            break;
    }
}

void fillDescType1(st_edmaDescInfoType1 *pst_edma_desc_info, st_edmaDescType1 *pst_edma_desc, int *pi_errorCode=NULL)
{
    EDMA_LOG_INFO("fillDescType1\n");

    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = pst_edma_desc_info->uc_desc_type;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_handshake_enable = 0; // TBD
    pst_edma_desc->ui_desp_00_reserved_0 = 0;

    // 0x04: APU_EDMA3_DESP_04
    checkFormat(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_format = pst_edma_desc_info->uc_fmt;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 0;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 0;
    pst_edma_desc->ui_desp_04_write_tmp_buf = 0;
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0;
    pst_edma_desc->ui_desp_04_pad_const= pst_edma_desc_info->us_alpha;
    pst_edma_desc->ui_desp_04_reserved_1 = 0;

    EDMA_LOG_INFO("Dec  Type: %d\n", pst_edma_desc->ui_desp_00_type);
    EDMA_LOG_INFO("     FMT : %d\n", pst_edma_desc->ui_desp_04_format);

    // 0x08: APU_EDMA3_DESP_08
    pst_edma_desc->ui_desp_08_in_swap_mat = (pst_edma_desc_info->uc_src_alpha_pos==EDMA_RGB2RGBA_ALPHA_POS_HIGH) ? 0x8421 : 0x1842;
    pst_edma_desc->ui_desp_08_out_swap_mat= (pst_edma_desc_info->uc_dst_alpha_pos==EDMA_RGB2RGBA_ALPHA_POS_HIGH) ? 0x8421 : 0x1842;

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_reserved_0 = 0;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_reserved_0 = 0;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_reserved_0 = 0;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_reserved_0 = 0;

    // 0x1C: APU_EDMA3_DESP_1C
    pst_edma_desc->ui_desp_1c_src_addr_0 = pst_edma_desc_info->ui_src_addr_0;

    // 0x20: APU_EDMA3_DESP_20
    pst_edma_desc->ui_desp_20_dst_addr_0 = pst_edma_desc_info->ui_dst_addr_0;

    // 0x24: APU_EDMA3_DESP_24
    pst_edma_desc->ui_desp_24_src_x_stride_0 = pst_edma_desc_info->ui_src_pitch_x_0;

    // 0x28: APU_EDMA3_DESP_28
    pst_edma_desc->ui_desp_28_dst_x_stride_0 = pst_edma_desc_info->ui_dst_pitch_x_0;

    // 0x2C: APU_EDMA3_DESP_2C
    pst_edma_desc->ui_desp_2c_src_y_stride_0 = pst_edma_desc_info->ui_src_pitch_y_0;

    // 0x30: APU_EDMA3_DESP_30
    pst_edma_desc->ui_desp_30_dst_y_stride_0 = pst_edma_desc_info->ui_dst_pitch_y_0;

    // 0x34: APU_EDMA3_DESP_34
    checkSrcSizeX(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_34_src_x_size_0 = pst_edma_desc_info->ui_src_size_x;
    checkDstSizeX(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_34_dst_x_size_0 = pst_edma_desc_info->ui_dst_size_x;

    // 0x38: APU_EDMA3_DESP_38
    checkSrcSizeY(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_38_src_y_size_0 = pst_edma_desc_info->ui_src_size_y;
    checkDstSizeY(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc_info->ui_dst_size_y;

    // 0x3c: APU_EDMA3_DESP_3C
    checkSrcSizeZ(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_3c_src_z_size_0 = pst_edma_desc_info->ui_src_size_z;
    checkDstSizeZ(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_3c_dst_z_size_0 = pst_edma_desc_info->ui_dst_size_z;
    EDMA_LOG_INFO("fillDescType1 END\n");
}

// Descriptor type 5 checker
static void checkFormat(st_edmaDescInfoType5 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if ((pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_YUV420_8_TO_RGBA_8) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_YUV422_8_TO_RGBA_8)/* &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_RGB_8_TO_YUV420_8)*/ &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_RGBA_8_TO_YUV420_8)/* &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_YUV420_16_TO_RGBA_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_YUV420_P010_TO_RGBA_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_YUV420_MTK_TO_RGBA_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_YUV420_P010_TO_YUV420_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_YUV420_MTK_TO_YUV420_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_RGBA_16_TO_YUV420_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_RGBA_102_TO_YUV420_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_RGBA_8_TO_RGBA_8_CODE) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_YUV420_8_TO_YUV420_8_CODE) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_RGB_8_TO_YUV420_8_CODE) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_RGBA_102_TO_RGBA_10_CODE) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_YUV420_16_TO_YUV420_10_CODE) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_RGBA_16_TO_YUV420_10_CODE) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_RGBA_8_CODE_TO_RGBA_8) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_YUV420_8_CODE_TO_YUV420_8) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_YUV420_8_CODE_TO_RGBA_8) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_RGBA_10_CODE_TO_RGBA_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_YUV420_10_CODE_TO_YUV420_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_YUV420_10_CODE_TO_RGBA_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_BAYER_10_TO_AINR_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_BAYER_12_TO_AINR_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_4CELL_10_TO_AINR_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_4CELL_12_TO_AINR_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_AINR_16_TO_BAYER_10) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_AINR_16_TO_BAYER_12) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_AINR_16_TO_BAYER_16) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_AINR_16_TO_3PFULLG_12_G) &&
        (pst_in_edma_desc_info->uc_fmt!=EDMA_FMT_AINR_16_TO_3PFULLG_12_RB)*/)
    {
        EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
    }
}

static void checkSrcLayout(st_edmaDescInfoType5 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if ((pst_in_edma_desc_info->uc_src_layout!=EDMA_LAYOUT_INTERLEAVED) &&
        (pst_in_edma_desc_info->uc_src_layout!=EDMA_LAYOUT_SEMI_PLANAR) &&
        (pst_in_edma_desc_info->uc_src_layout!=EDMA_LAYOUT_PLANAR))
    {
        EDMA_LOG_ERR("unsupported layout: %d\n", pst_in_edma_desc_info->uc_src_layout);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_LAYOUT);
    }
}

static void checkSrcItlvFmt(st_edmaDescInfoType5 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    switch (pst_in_edma_desc_info->uc_src_layout)
    {
        case EDMA_LAYOUT_INTERLEAVED:
            switch (pst_in_edma_desc_info->uc_fmt)
            {
                case EDMA_FMT_YUV422_8_TO_RGBA_8:
                    if ((pst_in_edma_desc_info->uc_src_itlv_fmt!=EDMA_ITLV_FMT_YUV422_YUYV) &&
                        (pst_in_edma_desc_info->uc_src_itlv_fmt!=EDMA_ITLV_FMT_YUV422_YVYU))
                    {
                        EDMA_LOG_ERR("unsupported interleaved format: %d\n", pst_in_edma_desc_info->uc_src_itlv_fmt);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_ITLV_FMT);
                    }
                    break;
                case EDMA_FMT_RGBA_8_TO_YUV420_8:
                    if ((pst_in_edma_desc_info->uc_src_itlv_fmt!=EDMA_ITLV_FMT_RGBA_RGBA) &&
                        (pst_in_edma_desc_info->uc_src_itlv_fmt!=EDMA_ITLV_FMT_RGBA_ARGB) &&
                        (pst_in_edma_desc_info->uc_src_itlv_fmt!=EDMA_ITLV_FMT_RGBA_BGRA) &&
                        (pst_in_edma_desc_info->uc_src_itlv_fmt!=EDMA_ITLV_FMT_RGBA_ABGR))
                    {
                        EDMA_LOG_ERR("unsupported interleaved format: %d\n", pst_in_edma_desc_info->uc_src_itlv_fmt);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_ITLV_FMT);
                    }
                    break;
                default: // others
                    // do nothing
                    break;
            }
            break;
        case EDMA_LAYOUT_SEMI_PLANAR:
            switch (pst_in_edma_desc_info->uc_fmt)
            {
                case EDMA_FMT_YUV420_8_TO_RGBA_8:
                    if ((pst_in_edma_desc_info->uc_src_itlv_fmt!=EDMA_ITLV_FMT_YUV420_UV) &&
                        (pst_in_edma_desc_info->uc_src_itlv_fmt!=EDMA_ITLV_FMT_YUV420_VU))
                    {
                        EDMA_LOG_ERR("unsupported interleaved format: %d\n", pst_in_edma_desc_info->uc_src_itlv_fmt);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_ITLV_FMT);
                    }
                    break;
                default: // others
                    // do nothing
                    break;
            }
            break;
        default: // case EDMA_LAYOUT_PLANAR:
            // TBD
            break;
    }
}

static void checkSrcSizeX0(st_edmaDescInfoType5 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    unsigned char uc_data_unit = 0;
    unsigned int ui_data_bit_size = 0;

    switch (pst_in_edma_desc_info->uc_src_layout)
    {
        case EDMA_LAYOUT_INTERLEAVED:
            switch (pst_in_edma_desc_info->uc_fmt)
            {
                case EDMA_FMT_YUV422_8_TO_RGBA_8: case EDMA_FMT_RGBA_8_TO_YUV420_8:
                    uc_data_unit = 32;
                    break;
                default: // others
                    EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
                    //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
                    break;
            }

            ui_data_bit_size = pst_in_edma_desc_info->ui_src_size_x_0 * 8;
            if (ui_data_bit_size != ((ui_data_bit_size/(unsigned int)uc_data_unit)*(unsigned int)uc_data_unit))
            {
                EDMA_LOG_ERR("ui_src_size_x_0*8 == %d*n for format #%d\n", uc_data_unit, pst_in_edma_desc_info->uc_fmt);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_X);
            }

            switch (pst_in_edma_desc_info->uc_fmt)
            {
                case EDMA_FMT_YUV422_8_TO_RGBA_8:
                    if (pst_in_edma_desc_info->ui_src_size_x_0 > MAX_EDMA_YUV422_8_TO_RGBA_8_SIZE_X)
                    {
                        EDMA_LOG_ERR("ui_src_size_x_0 <= %d\n", MAX_EDMA_YUV422_8_TO_RGBA_8_SIZE_X);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_X);
                    }

                    if ((pst_in_edma_desc_info->ui_src_size_x_0&0x3) != 0)
                    {
                        EDMA_LOG_ERR("ui_src_size_x_0 == 4*n for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_X);
                    }
                    break;
                case EDMA_FMT_RGBA_8_TO_YUV420_8:
                    if (pst_in_edma_desc_info->ui_src_size_x_0 > MAX_EDMA_RGBA_8_TO_YUV420_8_SIZE_X)
                    {
                        EDMA_LOG_ERR("ui_src_size_x_0 <= %d\n", MAX_EDMA_RGBA_8_TO_YUV420_8_SIZE_X);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_X);
                    }

                    if ((pst_in_edma_desc_info->ui_src_size_x_0&0x7) != 0)
                    {
                        EDMA_LOG_ERR("ui_src_size_x_0 == 8*n for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_X);
                    }
                    break;
                default: // others
                    EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
                    //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
                    break;
            }
            break;
        case EDMA_LAYOUT_SEMI_PLANAR:
            switch (pst_in_edma_desc_info->uc_fmt)
            {
                case EDMA_FMT_YUV420_8_TO_RGBA_8:
                    uc_data_unit = 8;
                    break;
                default: // others
                    EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
                    //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
                    break;
            }

            ui_data_bit_size = pst_in_edma_desc_info->ui_src_size_x_0 * 8;
            if (ui_data_bit_size != ((ui_data_bit_size/(unsigned int)uc_data_unit)*(unsigned int)uc_data_unit))
            {
                EDMA_LOG_ERR("ui_src_size_x_0*8 == %d*n for format #%d\n", uc_data_unit, pst_in_edma_desc_info->uc_fmt);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_X);
            }

            switch (pst_in_edma_desc_info->uc_fmt)
            {
                case EDMA_FMT_YUV420_8_TO_RGBA_8:
                    if (pst_in_edma_desc_info->ui_src_size_x_0 > MAX_EDMA_YUV420_8_TO_RGBA_8_SIZE_X)
                    {
                        EDMA_LOG_ERR("ui_src_size_x_0 <= %d\n", MAX_EDMA_YUV420_8_TO_RGBA_8_SIZE_X);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_X);
                    }

                    if ((pst_in_edma_desc_info->ui_src_size_x_0&0x1) != 0)
                    {
                        EDMA_LOG_ERR("ui_src_size_x_0 == 2*n for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_X);
                    }
                    break;
                default: // others
                    EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
                    //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
                    break;
            }
            break;
        default: // case EDMA_LAYOUT_PLANAR:
            // TBD
            break;
    }
}

static void checkSrcSizeY0(st_edmaDescInfoType5 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    switch (pst_in_edma_desc_info->uc_fmt)
    {
        case EDMA_FMT_YUV420_8_TO_RGBA_8: case EDMA_FMT_RGBA_8_TO_YUV420_8:
            if (pst_in_edma_desc_info->ui_src_size_y_0 > MAX_EDMA_SIZE_Y_EVEN)
            {
                EDMA_LOG_ERR("ui_src_size_y_0 <= %d\n", MAX_EDMA_SIZE_Y_EVEN);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_Y);
            }
            if ((pst_in_edma_desc_info->ui_src_size_y_0&0x1) != 0)
            {
                EDMA_LOG_ERR("ui_src_size_y_0 must be even\n");
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_Y);
            }
            break;
        case EDMA_FMT_YUV422_8_TO_RGBA_8:
            if (pst_in_edma_desc_info->ui_src_size_y_0 > MAX_EDMA_SIZE_Y)
            {
                EDMA_LOG_ERR("ui_src_size_y_0 <= %d\n", MAX_EDMA_SIZE_Y);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_Y);
            }
            break;
        default: // others
            EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
            //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
            break;
    }
}

static void checkSrcSizeZ0(st_edmaDescInfoType5 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if (pst_in_edma_desc_info->ui_src_size_z_0 > MAX_EDMA_SIZE_Z)
    {
        EDMA_LOG_ERR("ui_src_size_z_0 <= %d\n", MAX_EDMA_SIZE_Z);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_Z);
    }
}

static void checkSrcSizeX1(st_edmaDescInfoType5 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    unsigned char uc_data_unit = 0;
    unsigned int ui_data_bit_size = 0;

    switch (pst_in_edma_desc_info->uc_src_layout)
    {
        case EDMA_LAYOUT_SEMI_PLANAR:
            switch (pst_in_edma_desc_info->uc_fmt)
            {
                case EDMA_FMT_YUV420_8_TO_RGBA_8:
                    uc_data_unit = 16;
                    break;
                default: // others
                    EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
                    //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
                    break;
            }

            ui_data_bit_size = pst_in_edma_desc_info->ui_src_size_x_1 * 8;
            if (ui_data_bit_size != ((ui_data_bit_size/(unsigned int)uc_data_unit)*(unsigned int)uc_data_unit))
            {
                EDMA_LOG_ERR("ui_src_size_x_1*8 == %d*n for format #%d\n", uc_data_unit, pst_in_edma_desc_info->uc_fmt);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_X);
            }

            switch (pst_in_edma_desc_info->uc_fmt)
            {
                case EDMA_FMT_YUV420_8_TO_RGBA_8:
                    if (pst_in_edma_desc_info->ui_src_size_x_1 > MAX_EDMA_YUV420_8_TO_RGBA_8_SIZE_X)
                    {
                        EDMA_LOG_ERR("ui_src_size_x_1 <= %d\n", MAX_EDMA_YUV420_8_TO_RGBA_8_SIZE_X);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_X);
                    }

                    if (pst_in_edma_desc_info->ui_src_size_x_1 != pst_in_edma_desc_info->ui_src_size_x_0)
                    {
                        EDMA_LOG_ERR("ui_src_size_x_1 == ui_src_size_x_0 for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_X);
                    }
                    break;
                default: // others
                    EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
                    //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
                    break;
            }
            break;
        default: // case EDMA_LAYOUT_INTERLEAVED: case EDMA_LAYOUT_PLANAR:
            // TBD
            break;
    }
}

static void checkSrcSizeY1(st_edmaDescInfoType5 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    switch (pst_in_edma_desc_info->uc_src_layout)
    {
        case EDMA_LAYOUT_SEMI_PLANAR:
            switch (pst_in_edma_desc_info->uc_fmt)
            {
                case EDMA_FMT_YUV420_8_TO_RGBA_8:
                    if (pst_in_edma_desc_info->ui_src_size_y_1 > MAX_EDMA_SIZE_Y_EVEN_HALF)
                    {
                        EDMA_LOG_ERR("ui_src_size_y_1 <= %d\n", MAX_EDMA_SIZE_Y_EVEN_HALF);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_Y);
                    }

                    if (pst_in_edma_desc_info->ui_src_size_y_1 != (pst_in_edma_desc_info->ui_src_size_y_0/2))
                    {
                        EDMA_LOG_ERR("ui_src_size_y_1 == ui_src_size_y_0/2 for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_Y);
                    }
                    break;
                default: // others
                    EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
                    //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
                    break;
            }
            break;
        default: // case EDMA_LAYOUT_INTERLEAVED: case EDMA_LAYOUT_PLANAR:
            // TBD
            break;
    }
}

static void checkSrcSizeZ1(st_edmaDescInfoType5 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if (pst_in_edma_desc_info->ui_src_size_z_1 > MAX_EDMA_SIZE_Z)
    {
        EDMA_LOG_ERR("ui_src_size_z_1 <= %d\n", MAX_EDMA_SIZE_Z);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_SRC_SIZE_Z);
    }
}

static void checkDstLayout(st_edmaDescInfoType5 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if ((pst_in_edma_desc_info->uc_dst_layout!=EDMA_LAYOUT_INTERLEAVED) &&
        (pst_in_edma_desc_info->uc_dst_layout!=EDMA_LAYOUT_SEMI_PLANAR) &&
        (pst_in_edma_desc_info->uc_dst_layout!=EDMA_LAYOUT_PLANAR))
    {
        EDMA_LOG_ERR("unsupported layout: %d\n", pst_in_edma_desc_info->uc_dst_layout);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_LAYOUT);
    }
}

static void checkDstItlvFmt(st_edmaDescInfoType5 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    switch (pst_in_edma_desc_info->uc_dst_layout)
    {
        case EDMA_LAYOUT_INTERLEAVED:
            switch (pst_in_edma_desc_info->uc_fmt)
            {
                case EDMA_FMT_YUV420_8_TO_RGBA_8: case EDMA_FMT_YUV422_8_TO_RGBA_8:
                    if ((pst_in_edma_desc_info->uc_dst_itlv_fmt!=EDMA_ITLV_FMT_RGBA_RGBA) &&
                        (pst_in_edma_desc_info->uc_dst_itlv_fmt!=EDMA_ITLV_FMT_RGBA_ARGB) &&
                        (pst_in_edma_desc_info->uc_dst_itlv_fmt!=EDMA_ITLV_FMT_RGBA_BGRA) &&
                        (pst_in_edma_desc_info->uc_dst_itlv_fmt!=EDMA_ITLV_FMT_RGBA_ABGR))
                    {
                        EDMA_LOG_ERR("unsupported interleaved format: %d\n", pst_in_edma_desc_info->uc_dst_itlv_fmt);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_ITLV_FMT);
                    }
                    break;
                default: // others
                    // do nothing
                    break;
            }
            break;
        case EDMA_LAYOUT_SEMI_PLANAR:
            switch (pst_in_edma_desc_info->uc_fmt)
            {
                case EDMA_FMT_RGBA_8_TO_YUV420_8:
                    if ((pst_in_edma_desc_info->uc_dst_itlv_fmt!=EDMA_ITLV_FMT_YUV420_UV) &&
                        (pst_in_edma_desc_info->uc_dst_itlv_fmt!=EDMA_ITLV_FMT_YUV420_VU))
                    {
                        EDMA_LOG_ERR("unsupported interleaved format: %d\n", pst_in_edma_desc_info->uc_dst_itlv_fmt);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_ITLV_FMT);
                    }
                    break;
                default: // others
                    // do nothing
                    break;
            }
            break;
        default: // case EDMA_LAYOUT_PLANAR:
            // TBD
            break;
    }
}

static void checkDstSizeX0(st_edmaDescInfoType5 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    unsigned char uc_data_unit = 0;
    unsigned int ui_data_bit_size = 0;

    switch(pst_in_edma_desc_info->uc_dst_layout)
    {
        case EDMA_LAYOUT_INTERLEAVED:
            switch (pst_in_edma_desc_info->uc_fmt)
            {
                case EDMA_FMT_YUV420_8_TO_RGBA_8: case EDMA_FMT_YUV422_8_TO_RGBA_8:
                    uc_data_unit = 32;
                    break;
                default: // others
                    EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
                    //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
                    break;
            }

            ui_data_bit_size = pst_in_edma_desc_info->ui_dst_size_x_0 * 8;
            if (ui_data_bit_size != ((ui_data_bit_size/(unsigned int)uc_data_unit)*(unsigned int)uc_data_unit))
            {
                EDMA_LOG_ERR("ui_dst_size_x_0*8 == %d*n for format #%d\n", uc_data_unit, pst_in_edma_desc_info->uc_fmt);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_X);
            }

            switch (pst_in_edma_desc_info->uc_fmt)
            {
                case EDMA_FMT_YUV420_8_TO_RGBA_8:
                    if (pst_in_edma_desc_info->ui_dst_size_x_0 > (MAX_EDMA_YUV420_8_TO_RGBA_8_SIZE_X*4))
                    {
                        EDMA_LOG_ERR("ui_dst_size_x_0 <= %d\n", (MAX_EDMA_YUV420_8_TO_RGBA_8_SIZE_X*4));
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_X);
                    }

                    if (pst_in_edma_desc_info->ui_dst_size_x_0 != (pst_in_edma_desc_info->ui_src_size_x_0*4))
                    {
                        EDMA_LOG_ERR("ui_dst_size_x_0 == ui_src_size_x_0*4 for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_X);
                    }
                    break;
                case EDMA_FMT_YUV422_8_TO_RGBA_8:
                    if (pst_in_edma_desc_info->ui_dst_size_x_0 > (MAX_EDMA_YUV422_8_TO_RGBA_8_SIZE_X*2))
                    {
                        EDMA_LOG_ERR("ui_dst_size_x_0 <= %d\n", (MAX_EDMA_YUV422_8_TO_RGBA_8_SIZE_X*2));
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_X);
                    }

                    if (pst_in_edma_desc_info->ui_dst_size_x_0 != (pst_in_edma_desc_info->ui_src_size_x_0*2))
                    {
                        EDMA_LOG_ERR("ui_dst_size_x_0 == ui_src_size_x_0*2 for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_X);
                    }
                    break;
                default: // others
                    EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
                    //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
                    break;
            }
            break;
        case EDMA_LAYOUT_SEMI_PLANAR:
            switch (pst_in_edma_desc_info->uc_fmt)
            {
                case EDMA_FMT_RGBA_8_TO_YUV420_8:
                    uc_data_unit = 8;
                    break;
                default: // others
                    EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
                    //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
                    break;
            }

            ui_data_bit_size = pst_in_edma_desc_info->ui_dst_size_x_0 * 8;
            if (ui_data_bit_size != ((ui_data_bit_size/(unsigned int)uc_data_unit)*(unsigned int)uc_data_unit))
            {
                EDMA_LOG_ERR("ui_dst_size_x_0*8 == %d*n for format #%d\n", uc_data_unit, pst_in_edma_desc_info->uc_fmt);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_X);
            }

            switch (pst_in_edma_desc_info->uc_fmt)
            {
                case EDMA_FMT_RGBA_8_TO_YUV420_8:
                    if (pst_in_edma_desc_info->ui_dst_size_x_0 > (MAX_EDMA_RGBA_8_TO_YUV420_8_SIZE_X>>2))
                    {
                        EDMA_LOG_ERR("ui_dst_size_x_0 <= %d\n", (MAX_EDMA_RGBA_8_TO_YUV420_8_SIZE_X>>2));
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_X);
                    }

                    if (pst_in_edma_desc_info->ui_dst_size_x_0 != (pst_in_edma_desc_info->ui_src_size_x_0>>2))
                    {
                        EDMA_LOG_ERR("ui_dst_size_x_0 == ui_src_size_x_0/4 for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_X);
                    }
                    break;
                default: // others
                    EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
                    //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
                    break;
            }
            break;
        default: // case EDMA_LAYOUT_PLANAR:
            // TBD
            break;
    }
}

static void checkDstSizeY0(st_edmaDescInfoType5 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    switch (pst_in_edma_desc_info->uc_fmt)
    {
        case EDMA_FMT_YUV420_8_TO_RGBA_8: case EDMA_FMT_RGBA_8_TO_YUV420_8:
            if (pst_in_edma_desc_info->ui_dst_size_y_0 > MAX_EDMA_SIZE_Y_EVEN)
            {
                EDMA_LOG_ERR("ui_dst_size_y_0 <= %d\n", MAX_EDMA_SIZE_Y_EVEN);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Y);
            }

            if (pst_in_edma_desc_info->ui_dst_size_y_0 != pst_in_edma_desc_info->ui_src_size_y_0)
            {
                EDMA_LOG_ERR("ui_dst_size_y_0 == ui_src_size_y_0 for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Y);
            }
            break;
        case EDMA_FMT_YUV422_8_TO_RGBA_8:
            if (pst_in_edma_desc_info->ui_dst_size_y_0 > MAX_EDMA_SIZE_Y)
            {
                EDMA_LOG_ERR("ui_dst_size_y_0 <= %d\n", MAX_EDMA_SIZE_Y);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Y);
            }

            if (pst_in_edma_desc_info->ui_dst_size_y_0 != pst_in_edma_desc_info->ui_src_size_y_0)
            {
                EDMA_LOG_ERR("ui_dst_size_y_0 == ui_src_size_y_0 for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Y);
            }
            break;
        default: // others
            EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
            //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
            break;
    }
}

static void checkDstSizeZ0(st_edmaDescInfoType5 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if (pst_in_edma_desc_info->ui_dst_size_z_0 > MAX_EDMA_SIZE_Z)
    {
        EDMA_LOG_ERR("ui_dst_size_z_0 <= %d\n", MAX_EDMA_SIZE_Z);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Z);
    }

    if (pst_in_edma_desc_info->ui_dst_size_z_0 != pst_in_edma_desc_info->ui_src_size_z_0)
    {
        EDMA_LOG_ERR("ui_dst_size_z_0 == ui_src_size_z_0 for format #%d\n", pst_in_edma_desc_info->uc_fmt);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Z);
    }
}

static void checkDstSizeX1(st_edmaDescInfoType5 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    unsigned char uc_data_unit = 0;
    unsigned int ui_data_bit_size = 0;

    switch(pst_in_edma_desc_info->uc_dst_layout)
    {
        case EDMA_LAYOUT_SEMI_PLANAR:
            switch (pst_in_edma_desc_info->uc_fmt)
            {
                case EDMA_FMT_RGBA_8_TO_YUV420_8:
                    uc_data_unit = 16;
                    break;
                default: // others
                    EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
                    //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
                    break;
            }

            ui_data_bit_size = pst_in_edma_desc_info->ui_dst_size_x_1 * 8;
            if (ui_data_bit_size != ((ui_data_bit_size/(unsigned int)uc_data_unit)*(unsigned int)uc_data_unit))
            {
                EDMA_LOG_ERR("ui_dst_size_x_1*8 == %d*n for format #%d\n", uc_data_unit, pst_in_edma_desc_info->uc_fmt);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_X);
            }

            switch (pst_in_edma_desc_info->uc_fmt)
            {
                case EDMA_FMT_RGBA_8_TO_YUV420_8:
                    if (pst_in_edma_desc_info->ui_dst_size_x_1 > (MAX_EDMA_RGBA_8_TO_YUV420_8_SIZE_X>>2))
                    {
                        EDMA_LOG_ERR("ui_dst_size_x_1 <= %d\n", (MAX_EDMA_RGBA_8_TO_YUV420_8_SIZE_X>>2));
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_X);
                    }

                    if (pst_in_edma_desc_info->ui_dst_size_x_1 != (pst_in_edma_desc_info->ui_src_size_x_0>>2))
                    {
                        EDMA_LOG_ERR("ui_dst_size_x_1 == ui_src_size_x_0/4 for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_X);
                    }
                    break;
                default: // others
                    EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
                    //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
                    break;
            }
            break;
        default: // case EDMA_LAYOUT_INTERLEAVED: case EDMA_LAYOUT_PLANAR:
            // TBD
            break;
    }
}

static void checkDstSizeY1(st_edmaDescInfoType5 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    switch (pst_in_edma_desc_info->uc_fmt)
    {
        case EDMA_FMT_RGBA_8_TO_YUV420_8:
            if (pst_in_edma_desc_info->ui_dst_size_y_1 > MAX_EDMA_SIZE_Y_EVEN_HALF)
            {
                EDMA_LOG_ERR("ui_dst_size_y_1 <= %d\n", MAX_EDMA_SIZE_Y_EVEN_HALF);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Y);
            }

            if (pst_in_edma_desc_info->ui_dst_size_y_1 != (pst_in_edma_desc_info->ui_src_size_y_0>>1))
            {
                EDMA_LOG_ERR("ui_dst_size_y_1 == ui_src_size_y_0/2 for format #%d\n", pst_in_edma_desc_info->uc_fmt);
                //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Y);
            }
            break;
        default: // others
            EDMA_LOG_ERR("unsupported format: %d\n", pst_in_edma_desc_info->uc_fmt);
            //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
            break;
    }
}

static void checkDstSizeZ1(st_edmaDescInfoType5 *pst_in_edma_desc_info, int *pi_errorCode=NULL)
{
    if (pst_in_edma_desc_info->ui_dst_size_z_1 > MAX_EDMA_SIZE_Z)
    {
        EDMA_LOG_ERR("ui_dst_size_z_1 <= %d\n", MAX_EDMA_SIZE_Z);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Z);
    }

    if (pst_in_edma_desc_info->ui_dst_size_z_1 != pst_in_edma_desc_info->ui_src_size_z_0)
    {
        EDMA_LOG_ERR("ui_dst_size_z_1 == ui_src_size_z_0 for format #%d\n", pst_in_edma_desc_info->uc_fmt);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_DST_SIZE_Z);
    }
}

void fillDescType5(st_edmaDescInfoType5 *pst_edma_desc_info, st_edmaDescType5 *pst_edma_desc, int *pi_errorCode=NULL)
{
    short as_color_mat[3][3], as_color_vec[3];
    EDMA_LOG_INFO("fillDescType5\n");
    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = pst_edma_desc_info->uc_desc_type;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_reserved_0 = 0;
    pst_edma_desc->ui_desp_00_src_0_addr_cbfc_extension = 0; // TBD
    pst_edma_desc->ui_desp_00_dst_0_addr_cbfc_extension = 0; // TBD

    // 0x04: APU_EDMA3_DESP_04
    checkFormat(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_format = pst_edma_desc_info->uc_fmt;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 0;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 0; // TBD
    pst_edma_desc->ui_desp_04_write_tmp_buf = 0; // TBD
    checkSrcLayout(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    checkSrcItlvFmt(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0; // TBD
    pst_edma_desc->ui_desp_04_reserved_1 = 0;
    pst_edma_desc->ui_desp_04_pad_const = pst_edma_desc_info->us_alpha;

    EDMA_LOG_INFO("Dec  Type: %d\n", pst_edma_desc->ui_desp_00_type);
    EDMA_LOG_INFO("     FMT : %d\n", pst_edma_desc->ui_desp_04_format);

    // 0x08: APU_EDMA3_DESP_08
    switch (pst_edma_desc_info->uc_fmt)
    {
        case EDMA_FMT_YUV420_8_TO_RGBA_8: case EDMA_FMT_YUV422_8_TO_RGBA_8:
            if (pst_edma_desc_info->uc_fmt == EDMA_FMT_YUV420_8_TO_RGBA_8)
            {
                if ((pst_edma_desc_info->uc_src_layout==EDMA_LAYOUT_SEMI_PLANAR) && (pst_edma_desc_info->uc_src_itlv_fmt==EDMA_ITLV_FMT_YUV420_VU))
                {
                    pst_edma_desc->ui_desp_08_in_swap_mat = 0x8124;
                }
                else // ((pst_edma_desc_info->uc_src_layout!=EDMA_LAYOUT_SEMI_PLANAR) || (pst_edma_desc_info->uc_src_itlv_fmt==EDMA_ITLV_FMT_YUV420_UV))
                {
                    pst_edma_desc->ui_desp_08_in_swap_mat = 0x8142;
                }
            }
            else // (pst_edma_desc_info->uc_fmt == EDMA_FMT_YUV422_8_TO_RGBA_8)
            {
                if ((pst_edma_desc_info->uc_src_layout==EDMA_LAYOUT_INTERLEAVED) && (pst_edma_desc_info->uc_src_itlv_fmt==EDMA_ITLV_FMT_YUV422_YVYU))
                {
                    pst_edma_desc->ui_desp_08_in_swap_mat = 0x8124;
                }
                else // ((pst_edma_desc_info->uc_src_layout!=EDMA_LAYOUT_INTERLEAVED) || (pst_edma_desc_info->uc_src_itlv_fmt==EDMA_ITLV_FMT_YUV422_YUYV))
                {
                    pst_edma_desc->ui_desp_08_in_swap_mat = 0x8142;
                }
            }

            if (pst_edma_desc_info->uc_dst_layout == EDMA_LAYOUT_INTERLEAVED)
            {
                switch (pst_edma_desc_info->uc_dst_itlv_fmt)
                {
                    case EDMA_ITLV_FMT_RGBA_ARGB:
                        pst_edma_desc->ui_desp_08_out_swap_mat = 0x1842;
                        break;
                    case EDMA_ITLV_FMT_RGBA_BGRA:
                        pst_edma_desc->ui_desp_08_out_swap_mat = 0x8124;
                        break;
                    case EDMA_ITLV_FMT_RGBA_ABGR:
                        pst_edma_desc->ui_desp_08_out_swap_mat = 0x1248;
                        break;
                    default: // case EDMA_ITLV_FMT_RGBA_RGBA:
                        pst_edma_desc->ui_desp_08_out_swap_mat = 0x8421;
                        break;
                }
            }
            else // (pst_edma_desc_info->uc_dst_layout != EDMA_LAYOUT_INTERLEAVED)
            {
                pst_edma_desc->ui_desp_08_out_swap_mat = 0x8421;
            }
            break;
        case EDMA_FMT_RGBA_8_TO_YUV420_8:
            if (pst_edma_desc_info->uc_src_layout == EDMA_LAYOUT_INTERLEAVED)
            {
                switch (pst_edma_desc_info->uc_src_itlv_fmt)
                {
                    case EDMA_ITLV_FMT_RGBA_ARGB:
                        pst_edma_desc->ui_desp_08_in_swap_mat = 0x4218;
                        break;
                    case EDMA_ITLV_FMT_RGBA_BGRA:
                        pst_edma_desc->ui_desp_08_in_swap_mat = 0x8124;
                        break;
                    case EDMA_ITLV_FMT_RGBA_ABGR:
                        pst_edma_desc->ui_desp_08_in_swap_mat = 0x1248;
                        break;
                    default: // case EDMA_ITLV_FMT_RGBA_RGBA:
                        pst_edma_desc->ui_desp_08_in_swap_mat = 0x8421;
                        break;
                }
            }
            else // (pst_edma_desc_info->uc_src_layout != EDMA_LAYOUT_INTERLEAVED)
            {
                pst_edma_desc->ui_desp_08_in_swap_mat = 0x8421;
            }

            if (pst_edma_desc_info->uc_dst_layout == EDMA_LAYOUT_SEMI_PLANAR)
            {
                if (pst_edma_desc_info->uc_dst_itlv_fmt==EDMA_ITLV_FMT_YUV420_UV)
                {
                    pst_edma_desc->ui_desp_08_out_swap_mat = 0x8214;
                }
                else // (pst_edma_desc_info->uc_dst_itlv_fmt==EDMA_ITLV_FMT_YUV420_VU)
                {
                    pst_edma_desc->ui_desp_08_out_swap_mat = 0x8124;
                }
            }
            else // (pst_edma_desc_info->uc_dst_layout != EDMA_LAYOUT_SEMI_PLANAR)
            {
                pst_edma_desc->ui_desp_08_out_swap_mat = 0x8421;
            }
            break;
        default: // others
            pst_edma_desc->ui_desp_08_in_swap_mat = 0x8421;
            pst_edma_desc->ui_desp_08_out_swap_mat = 0x8421;
            break;
    }

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_ufbc_tile_width = 0; // TBD
    pst_edma_desc->ui_desp_0c_reserved_0 = 0;
    pst_edma_desc->ui_desp_0c_ufbc_tile_height = 0; // TBD
    pst_edma_desc->ui_desp_0c_reserved_1 = 0;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_ufbc_frame_width = 0; // TBD
    pst_edma_desc->ui_desp_10_reserved_0 = 0;
    pst_edma_desc->ui_desp_10_ufbc_frame_height = 0; // TBD
    pst_edma_desc->ui_desp_10_reserved_1 = 0;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_ufbc_x_offset = 0; // TBD
    pst_edma_desc->ui_desp_14_reserved_0 = 0;
    pst_edma_desc->ui_desp_14_ufbc_y_offset = 0; // TBD
    pst_edma_desc->ui_desp_14_reserved_1 = 0;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_ufbc_payload_offset = 0; // TBD

    checkSrcSizeX0(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    checkSrcSizeY0(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    checkSrcSizeZ0(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    if (pst_edma_desc_info->uc_src_layout == EDMA_LAYOUT_SEMI_PLANAR)
    {
        checkSrcSizeX1(pst_edma_desc_info, pi_errorCode);
        //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
        checkSrcSizeY1(pst_edma_desc_info, pi_errorCode);
        //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
        checkSrcSizeZ1(pst_edma_desc_info, pi_errorCode);
        //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    }
    checkDstLayout(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    checkDstItlvFmt(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    checkDstSizeX0(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    checkDstSizeY0(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    checkDstSizeZ0(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    if (pst_edma_desc_info->uc_dst_layout == EDMA_LAYOUT_SEMI_PLANAR)
    {
        checkDstSizeX1(pst_edma_desc_info, pi_errorCode);
        //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
        checkDstSizeY1(pst_edma_desc_info, pi_errorCode);
        //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
        checkDstSizeZ1(pst_edma_desc_info, pi_errorCode);
        //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    }
    switch (pst_edma_desc_info->uc_fmt)
    {
        case EDMA_FMT_YUV420_8_TO_RGBA_8: case EDMA_FMT_YUV422_8_TO_RGBA_8:
            if (pst_edma_desc_info->uc_fmt == EDMA_FMT_YUV420_8_TO_RGBA_8)
            {
                // 0x1C: APU_EDMA3_DESP_1C
                pst_edma_desc->ui_desp_1c_src_addr_0 = pst_edma_desc_info->ui_src_addr_1;
                // 0x20: APU_EDMA3_DESP_20
                pst_edma_desc->ui_desp_20_dst_addr_0 = pst_edma_desc_info->ui_dst_addr_0;
                // 0x24: APU_EDMA3_DESP_24
                pst_edma_desc->ui_desp_24_src_x_stride_0 = pst_edma_desc_info->ui_src_pitch_x_1;
                // 0x28: APU_EDMA3_DESP_28
                pst_edma_desc->ui_desp_28_dst_x_stride_0 = pst_edma_desc_info->ui_dst_pitch_x_0;
                // 0x2C: APU_EDMA3_DESP_2C
                pst_edma_desc->ui_desp_2c_src_y_stride_0 = pst_edma_desc_info->ui_src_pitch_y_1;
                // 0x30: APU_EDMA3_DESP_30
                pst_edma_desc->ui_desp_30_dst_y_stride_0 = pst_edma_desc_info->ui_dst_pitch_y_0;
                // 0x34: APU_EDMA3_DESP_34
                pst_edma_desc->ui_desp_34_src_x_size_0 = pst_edma_desc_info->ui_src_size_x_1;
                pst_edma_desc->ui_desp_34_dst_x_size_0 = pst_edma_desc_info->ui_dst_size_x_0;
                // 0x38: APU_EDMA3_DESP_38
                pst_edma_desc->ui_desp_38_src_y_size_0 = pst_edma_desc_info->ui_src_size_y_1;
                pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc_info->ui_dst_size_y_0;
                // 0x3C: APU_EDMA3_DESP_3C
                pst_edma_desc->ui_desp_3c_src_z_size_0 = pst_edma_desc_info->ui_src_size_z_1;
                pst_edma_desc->ui_desp_3c_dst_z_size_0 = pst_edma_desc_info->ui_dst_size_z_0;
                // 0x5C: APU_EDMA3_DESP_5C
                pst_edma_desc->ui_desp_5c_src_addr_1 = pst_edma_desc_info->ui_src_addr_0;
                // 0x60: APU_EDMA3_DESP_60
                pst_edma_desc->ui_desp_60_dst_addr_1 = pst_edma_desc_info->ui_dst_addr_1;
                // 0x64: APU_EDMA3_DESP_64
                pst_edma_desc->ui_desp_64_src_x_stride_1 = pst_edma_desc_info->ui_src_pitch_x_0;
                // 0x68: APU_EDMA3_DESP_68
                pst_edma_desc->ui_desp_68_dst_x_stride_1 = pst_edma_desc_info->ui_dst_pitch_x_1;
                // 0x6C: APU_EDMA3_DESP_6C
                pst_edma_desc->ui_desp_6c_src_y_stride_1 = pst_edma_desc_info->ui_src_pitch_y_0;
                // 0x70: APU_EDMA3_DESP_70
                pst_edma_desc->ui_desp_70_dst_y_stride_1 = pst_edma_desc_info->ui_dst_pitch_y_1;
                // 0x74: APU_EDMA3_DESP_74
                pst_edma_desc->ui_desp_74_src_x_size_1 = pst_edma_desc_info->ui_src_size_x_0;
                pst_edma_desc->ui_desp_74_dst_x_size_1 = pst_edma_desc_info->ui_dst_size_x_1;
                // 0x78: APU_EDMA3_DESP_78
                pst_edma_desc->ui_desp_78_src_y_size_1 = pst_edma_desc_info->ui_src_size_y_0;
                pst_edma_desc->ui_desp_78_dst_y_size_1 = pst_edma_desc_info->ui_dst_size_y_1;
                // 0x7C: APU_EDMA3_DESP_7C
                pst_edma_desc->ui_desp_7c_src_z_size_1 = pst_edma_desc_info->ui_src_size_z_0;
                pst_edma_desc->ui_desp_7c_dst_z_size_1 = pst_edma_desc_info->ui_dst_size_z_1;
            }
            else // (pst_edma_desc_info->uc_fmt == EDMA_FMT_YUV422_8_TO_RGBA_8)
            {
                // 0x1C: APU_EDMA3_DESP_1C
                pst_edma_desc->ui_desp_1c_src_addr_0 = pst_edma_desc_info->ui_src_addr_0;
                // 0x20: APU_EDMA3_DESP_20
                pst_edma_desc->ui_desp_20_dst_addr_0 = pst_edma_desc_info->ui_dst_addr_0;
                // 0x24: APU_EDMA3_DESP_24
                pst_edma_desc->ui_desp_24_src_x_stride_0 = pst_edma_desc_info->ui_src_pitch_x_0;
                // 0x28: APU_EDMA3_DESP_28
                pst_edma_desc->ui_desp_28_dst_x_stride_0 = pst_edma_desc_info->ui_dst_pitch_x_0;
                // 0x2C: APU_EDMA3_DESP_2C
                pst_edma_desc->ui_desp_2c_src_y_stride_0 = pst_edma_desc_info->ui_src_pitch_y_0;
                // 0x30: APU_EDMA3_DESP_30
                pst_edma_desc->ui_desp_30_dst_y_stride_0 = pst_edma_desc_info->ui_dst_pitch_y_0;
                // 0x34: APU_EDMA3_DESP_34
                pst_edma_desc->ui_desp_34_src_x_size_0 = pst_edma_desc_info->ui_src_size_x_0;
                pst_edma_desc->ui_desp_34_dst_x_size_0 = pst_edma_desc_info->ui_dst_size_x_0;
                // 0x38: APU_EDMA3_DESP_38
                pst_edma_desc->ui_desp_38_src_y_size_0 = pst_edma_desc_info->ui_src_size_y_0;
                pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc_info->ui_dst_size_y_0;
                // 0x3C: APU_EDMA3_DESP_3C
                pst_edma_desc->ui_desp_3c_src_z_size_0 = pst_edma_desc_info->ui_src_size_z_0;
                pst_edma_desc->ui_desp_3c_dst_z_size_0 = pst_edma_desc_info->ui_dst_size_z_0;
                // 0x5C: APU_EDMA3_DESP_5C
                pst_edma_desc->ui_desp_5c_src_addr_1 = pst_edma_desc_info->ui_src_addr_1;
                // 0x60: APU_EDMA3_DESP_60
                pst_edma_desc->ui_desp_60_dst_addr_1 = pst_edma_desc_info->ui_dst_addr_1;
                // 0x64: APU_EDMA3_DESP_64
                pst_edma_desc->ui_desp_64_src_x_stride_1 = pst_edma_desc_info->ui_src_pitch_x_1;
                // 0x68: APU_EDMA3_DESP_68
                pst_edma_desc->ui_desp_68_dst_x_stride_1 = pst_edma_desc_info->ui_dst_pitch_x_1;
                // 0x6C: APU_EDMA3_DESP_6C
                pst_edma_desc->ui_desp_6c_src_y_stride_1 = pst_edma_desc_info->ui_src_pitch_y_1;
                // 0x70: APU_EDMA3_DESP_70
                pst_edma_desc->ui_desp_70_dst_y_stride_1 = pst_edma_desc_info->ui_dst_pitch_y_1;
                // 0x74: APU_EDMA3_DESP_74
                pst_edma_desc->ui_desp_74_src_x_size_1 = pst_edma_desc_info->ui_src_size_x_1;
                pst_edma_desc->ui_desp_74_dst_x_size_1 = pst_edma_desc_info->ui_dst_size_x_1;
                // 0x78: APU_EDMA3_DESP_78
                pst_edma_desc->ui_desp_78_src_y_size_1 = pst_edma_desc_info->ui_src_size_y_1;
                pst_edma_desc->ui_desp_78_dst_y_size_1 = pst_edma_desc_info->ui_dst_size_y_1;
                // 0x7C: APU_EDMA3_DESP_7C
                pst_edma_desc->ui_desp_7c_src_z_size_1 = pst_edma_desc_info->ui_src_size_z_1;
                pst_edma_desc->ui_desp_7c_dst_z_size_1 = pst_edma_desc_info->ui_dst_size_z_1;
            }
            // OpenCV YUV to RGB
            // U2BF = 2.032f, U2GF = -0.395f, V2GF = -0.581f, V2RF = 1.140f
            // 10-bit precision
            // U2BI = 2081, U2GI = -404, V2GI = -595, V2RI = 1167
            // YUV to RGB
            // r = Y + CV_DESCALE((Cr - delta)*C0, shift);
            // g = Y + CV_DESCALE((Cb - delta)*C2 + (Cr - delta)*C1, shift);
            // b = Y + CV_DESCALE((Cb - delta)*C3, shift);
            // =>
            // R = CV_DESCALE((1024*Y +    0*U + 1167*V), 10) + ((-1167*128)>>10)
            // G = CV_DESCALE((1024*Y + -404*U + -595*V), 10) + ((999*128)>>10)
            // B = CV_DESCALE((1024*Y + 2081*U +    0*V), 10) + ((-2081*128)>>10)
            // =>
            // R = CV_DESCALE((1024*Y +    0*U + 1167*V), 10) + -146
            // G = CV_DESCALE((1024*Y + -404*U + -595*V), 10) + 125
            // B = CV_DESCALE((1024*Y + 2081*U +    0*V), 10) + -260
            // => HW is under 10-bit operation, need to modify vector part
            // R = CV_DESCALE((1024*Y +    0*U + 1167*V), 10) + -584
            // G = CV_DESCALE((1024*Y + -404*U + -595*V), 10) + 500
            // B = CV_DESCALE((1024*Y + 2081*U +    0*V), 10) + -1040
            as_color_mat[0][0] = 1024; as_color_mat[0][1] =    0; as_color_mat[0][2] =  1167;
            as_color_mat[1][0] = 1024; as_color_mat[1][1] = -404; as_color_mat[1][2] =  -595;
            as_color_mat[2][0] = 1024; as_color_mat[2][1] = 2081; as_color_mat[2][2] =     0;
            as_color_vec[0]    = -584; as_color_vec[1]    =  500; as_color_vec[2]    = -1040;
            // 0x44: APU_EDMA3_DESP_44
            pst_edma_desc->ui_desp_44_r2y_y2r_mat_00 = (unsigned int)as_color_mat[0][0];
            pst_edma_desc->ui_desp_44_r2y_y2r_mat_01 = (unsigned int)as_color_mat[0][1];
            // 0x48: APU_EDMA3_DESP_48
            pst_edma_desc->ui_desp_48_r2y_y2r_mat_02 = (unsigned int)as_color_mat[0][2];
            pst_edma_desc->ui_desp_48_r2y_y2r_vec_0 = (unsigned int)as_color_vec[0];
            // 0x4C: APU_EDMA3_DESP_4C
            pst_edma_desc->ui_desp_4c_r2y_y2r_mat_10 = (unsigned int)as_color_mat[1][0];
            pst_edma_desc->ui_desp_4c_r2y_y2r_mat_11 = (unsigned int)as_color_mat[1][1];
            // 0x50: APU_EDMA3_DESP_50
            pst_edma_desc->ui_desp_50_r2y_y2r_mat_12 = (unsigned int)as_color_mat[1][2];
            pst_edma_desc->ui_desp_50_r2y_y2r_vec_1 = (unsigned int)as_color_vec[1];
            // 0x54: APU_EDMA3_DESP_54
            pst_edma_desc->ui_desp_54_r2y_y2r_mat_20 = (unsigned int)as_color_mat[2][0];
            pst_edma_desc->ui_desp_54_r2y_y2r_mat_21 = (unsigned int)as_color_mat[2][1];
            // 0x58: APU_EDMA3_DESP_58
            pst_edma_desc->ui_desp_58_r2y_y2r_mat_22 = (unsigned int)as_color_mat[2][2];
            pst_edma_desc->ui_desp_58_r2y_y2r_vec_2 = (unsigned int)as_color_vec[2];
            break;
        case EDMA_FMT_RGBA_8_TO_YUV420_8:
            // 0x1C: APU_EDMA3_DESP_1C
            pst_edma_desc->ui_desp_1c_src_addr_0 = pst_edma_desc_info->ui_src_addr_0;
            // 0x20: APU_EDMA3_DESP_20
            pst_edma_desc->ui_desp_20_dst_addr_0 = pst_edma_desc_info->ui_dst_addr_1;
            // 0x24: APU_EDMA3_DESP_24
            pst_edma_desc->ui_desp_24_src_x_stride_0 = pst_edma_desc_info->ui_src_pitch_x_0;
            // 0x28: APU_EDMA3_DESP_28
            pst_edma_desc->ui_desp_28_dst_x_stride_0 = pst_edma_desc_info->ui_dst_pitch_x_1;
            // 0x2C: APU_EDMA3_DESP_2C
            pst_edma_desc->ui_desp_2c_src_y_stride_0 = pst_edma_desc_info->ui_src_pitch_y_0;
            // 0x30: APU_EDMA3_DESP_30
            pst_edma_desc->ui_desp_30_dst_y_stride_0 = pst_edma_desc_info->ui_dst_pitch_y_1;
            // 0x34: APU_EDMA3_DESP_34
            pst_edma_desc->ui_desp_34_src_x_size_0 = pst_edma_desc_info->ui_src_size_x_0;
            pst_edma_desc->ui_desp_34_dst_x_size_0 = pst_edma_desc_info->ui_dst_size_x_1;
            // 0x38: APU_EDMA3_DESP_38
            pst_edma_desc->ui_desp_38_src_y_size_0 = pst_edma_desc_info->ui_src_size_y_0;
            pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc_info->ui_dst_size_y_1;
            // 0x3C: APU_EDMA3_DESP_3C
            pst_edma_desc->ui_desp_3c_src_z_size_0 = pst_edma_desc_info->ui_src_size_z_0;
            pst_edma_desc->ui_desp_3c_dst_z_size_0 = pst_edma_desc_info->ui_dst_size_z_1;
            // 0x5C: APU_EDMA3_DESP_5C
            pst_edma_desc->ui_desp_5c_src_addr_1 = pst_edma_desc_info->ui_src_addr_1;
            // 0x60: APU_EDMA3_DESP_60
            pst_edma_desc->ui_desp_60_dst_addr_1 = pst_edma_desc_info->ui_dst_addr_0;
            // 0x64: APU_EDMA3_DESP_64
            pst_edma_desc->ui_desp_64_src_x_stride_1 = pst_edma_desc_info->ui_src_pitch_x_1;
            // 0x68: APU_EDMA3_DESP_68
            pst_edma_desc->ui_desp_68_dst_x_stride_1 = pst_edma_desc_info->ui_dst_pitch_x_0;
            // 0x6C: APU_EDMA3_DESP_6C
            pst_edma_desc->ui_desp_6c_src_y_stride_1 = pst_edma_desc_info->ui_src_pitch_y_1;
            // 0x70: APU_EDMA3_DESP_70
            pst_edma_desc->ui_desp_70_dst_y_stride_1 = pst_edma_desc_info->ui_dst_pitch_y_0;
            // 0x74: APU_EDMA3_DESP_74
            pst_edma_desc->ui_desp_74_src_x_size_1 = pst_edma_desc_info->ui_src_size_x_1;
            pst_edma_desc->ui_desp_74_dst_x_size_1 = pst_edma_desc_info->ui_dst_size_x_0;
            // 0x78: APU_EDMA3_DESP_78
            pst_edma_desc->ui_desp_78_src_y_size_1 = pst_edma_desc_info->ui_src_size_y_1;
            pst_edma_desc->ui_desp_78_dst_y_size_1 = pst_edma_desc_info->ui_dst_size_y_0;
            // 0x7C: APU_EDMA3_DESP_7C
            pst_edma_desc->ui_desp_7c_src_z_size_1 = pst_edma_desc_info->ui_src_size_z_1;
            pst_edma_desc->ui_desp_7c_dst_z_size_1 = pst_edma_desc_info->ui_dst_size_z_0;
            // OpenCV RGB to YUV
            // R2YF = 0.299f, G2YF = 0.587f, B2YF = 0.114f, R2VF = 0.877f, B2UF = 0.492f
            // 10-bit precision
            // R2YI = 306, G2YI = 601, B2YI = 117, R2VI = 898, B2UI = 504
            // RGB to YUV
            // Y  = CV_DESCALE(    R*C0 + G*C1 + B*C2, shift);
            // Cb = CV_DESCALE((B-Y)*C4 + delta, shift);
            // Cr = CV_DESCALE((R-Y)*C3 + delta, shift);
            // =>
            // Y  = CV_DESCALE(   R*C0 + G*C1 + B*C2, shift);
            // Cb = CV_DESCALE((B-R*C0 - G*C1 - B*C2)*C4 + delta, shift);
            // Cr = CV_DESCALE((R-R*C0 - G*C1 - B*C2)*C3 + delta, shift);
            // =>
            // Y  = CV_DESCALE(R*       C0 + G*      C1 + B*       C2, shift);
            // Cb = CV_DESCALE(R* (-C0)*C4 + G*(-C1)*C4 + B*(1-C2)*C4 + delta, shift);
            // Cr = CV_DESCALE(R*(1-C0)*C3 + G*(-C1)*C3 + B* (-C2)*C3 + delta, shift);
            // =>
            // Y  = CV_DESCALE(R*   0.299 + G*   0.587 + B*   0.114, shift);
            // Cb = CV_DESCALE(R*(-0.147) + G*(-0.289) + B*   0.436 + delta, shift);
            // Cr = CV_DESCALE(R*   0.615 + G*(-0.515) + B*(-0.100) + delta, shift);
            // =>
            // Y  = CV_DESCALE( 306*R +  601*G +  117*B, 10);
            // Cb = CV_DESCALE(-151*R + -296*G +  446*B, 10) + 128;
            // Cr = CV_DESCALE( 630*R + -527*G + -102*B, 10) + 128;
            // => HW is under 10-bit operation, need to modify vector part
            // Y  = CV_DESCALE( 306*R +  601*G +  117*B, 10);
            // Cb = CV_DESCALE(-151*R + -296*G +  446*B, 10) + 512;
            // Cr = CV_DESCALE( 630*R + -527*G + -102*B, 10) + 512;
            as_color_mat[0][0] =  306; as_color_mat[0][1] =  601; as_color_mat[0][2] =  117;
            as_color_mat[1][0] = -151; as_color_mat[1][1] = -296; as_color_mat[1][2] =  446;
            as_color_mat[2][0] =  630; as_color_mat[2][1] = -527; as_color_mat[2][2] = -102;
            as_color_vec[0]    =    0; as_color_vec[1]    =  512; as_color_vec[2]    =  512;
            // 0x44: APU_EDMA3_DESP_44
            pst_edma_desc->ui_desp_44_r2y_y2r_mat_00 = (unsigned int)as_color_mat[0][0];
            pst_edma_desc->ui_desp_44_r2y_y2r_mat_01 = (unsigned int)as_color_mat[0][1];
            // 0x48: APU_EDMA3_DESP_48
            pst_edma_desc->ui_desp_48_r2y_y2r_mat_02 = (unsigned int)as_color_mat[0][2];
            pst_edma_desc->ui_desp_48_r2y_y2r_vec_0 = (unsigned int)as_color_vec[0];
            // 0x4C: APU_EDMA3_DESP_4C
            pst_edma_desc->ui_desp_4c_r2y_y2r_mat_10 = (unsigned int)as_color_mat[1][0];
            pst_edma_desc->ui_desp_4c_r2y_y2r_mat_11 = (unsigned int)as_color_mat[1][1];
            // 0x50: APU_EDMA3_DESP_50
            pst_edma_desc->ui_desp_50_r2y_y2r_mat_12 = (unsigned int)as_color_mat[1][2];
            pst_edma_desc->ui_desp_50_r2y_y2r_vec_1 = (unsigned int)as_color_vec[1];
            // 0x54: APU_EDMA3_DESP_54
            pst_edma_desc->ui_desp_54_r2y_y2r_mat_20 = (unsigned int)as_color_mat[2][0];
            pst_edma_desc->ui_desp_54_r2y_y2r_mat_21 = (unsigned int)as_color_mat[2][1];
            // 0x58: APU_EDMA3_DESP_58
            pst_edma_desc->ui_desp_58_r2y_y2r_mat_22 = (unsigned int)as_color_mat[2][2];
            pst_edma_desc->ui_desp_58_r2y_y2r_vec_2 = (unsigned int)as_color_vec[2];
            break;
        default: // others
            // 0x1C: APU_EDMA3_DESP_1C
            pst_edma_desc->ui_desp_1c_src_addr_0 = pst_edma_desc_info->ui_src_addr_0;
            // 0x20: APU_EDMA3_DESP_20
            pst_edma_desc->ui_desp_20_dst_addr_0 = pst_edma_desc_info->ui_dst_addr_0;
            // 0x24: APU_EDMA3_DESP_24
            pst_edma_desc->ui_desp_24_src_x_stride_0 = pst_edma_desc_info->ui_src_pitch_x_0;
            // 0x28: APU_EDMA3_DESP_28
            pst_edma_desc->ui_desp_28_dst_x_stride_0 = pst_edma_desc_info->ui_dst_pitch_x_0;
            // 0x2C: APU_EDMA3_DESP_2C
            pst_edma_desc->ui_desp_2c_src_y_stride_0 = pst_edma_desc_info->ui_src_pitch_y_0;
            // 0x30: APU_EDMA3_DESP_30
            pst_edma_desc->ui_desp_30_dst_y_stride_0 = pst_edma_desc_info->ui_dst_pitch_y_0;
            // 0x34: APU_EDMA3_DESP_34
            pst_edma_desc->ui_desp_34_src_x_size_0 = pst_edma_desc_info->ui_src_size_x_0;
            pst_edma_desc->ui_desp_34_dst_x_size_0 = pst_edma_desc_info->ui_dst_size_x_0;
            // 0x38: APU_EDMA3_DESP_38
            pst_edma_desc->ui_desp_38_src_y_size_0 = pst_edma_desc_info->ui_src_size_y_0;
            pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc_info->ui_dst_size_y_0;
            // 0x3C: APU_EDMA3_DESP_3C
            pst_edma_desc->ui_desp_3c_src_z_size_0 = pst_edma_desc_info->ui_src_size_z_0;
            pst_edma_desc->ui_desp_3c_dst_z_size_0 = pst_edma_desc_info->ui_dst_size_z_0;
            // 0x5C: APU_EDMA3_DESP_5C
            pst_edma_desc->ui_desp_5c_src_addr_1 = pst_edma_desc_info->ui_src_addr_1;
            // 0x60: APU_EDMA3_DESP_60
            pst_edma_desc->ui_desp_60_dst_addr_1 = pst_edma_desc_info->ui_dst_addr_1;
            // 0x64: APU_EDMA3_DESP_64
            pst_edma_desc->ui_desp_64_src_x_stride_1 = pst_edma_desc_info->ui_src_pitch_x_1;
            // 0x68: APU_EDMA3_DESP_68
            pst_edma_desc->ui_desp_68_dst_x_stride_1 = pst_edma_desc_info->ui_dst_pitch_x_1;
            // 0x6C: APU_EDMA3_DESP_6C
            pst_edma_desc->ui_desp_6c_src_y_stride_1 = pst_edma_desc_info->ui_src_pitch_y_1;
            // 0x70: APU_EDMA3_DESP_70
            pst_edma_desc->ui_desp_70_dst_y_stride_1 = pst_edma_desc_info->ui_dst_pitch_y_1;
            // 0x74: APU_EDMA3_DESP_74
            pst_edma_desc->ui_desp_74_src_x_size_1 = pst_edma_desc_info->ui_src_size_x_1;
            pst_edma_desc->ui_desp_74_dst_x_size_1 = pst_edma_desc_info->ui_dst_size_x_1;
            // 0x78: APU_EDMA3_DESP_78
            pst_edma_desc->ui_desp_78_src_y_size_1 = pst_edma_desc_info->ui_src_size_y_1;
            pst_edma_desc->ui_desp_78_dst_y_size_1 = pst_edma_desc_info->ui_dst_size_y_1;
            // 0x7C: APU_EDMA3_DESP_7C
            pst_edma_desc->ui_desp_7c_src_z_size_1 = pst_edma_desc_info->ui_src_size_z_1;
            pst_edma_desc->ui_desp_7c_dst_z_size_1 = pst_edma_desc_info->ui_dst_size_z_1;
            as_color_mat[0][0] = 1024; as_color_mat[0][1] =    0; as_color_mat[0][2] =    0;
            as_color_mat[1][0] =    0; as_color_mat[1][1] = 1024; as_color_mat[1][2] =    0;
            as_color_mat[2][0] =    0; as_color_mat[2][1] =    0; as_color_mat[2][2] = 1024;
            as_color_vec[0]    =    0; as_color_vec[1]    =    0; as_color_vec[2]    =    0;
            // 0x44: APU_EDMA3_DESP_44
            pst_edma_desc->ui_desp_44_r2y_y2r_mat_00 = (unsigned int)as_color_mat[0][0];
            pst_edma_desc->ui_desp_44_r2y_y2r_mat_01 = (unsigned int)as_color_mat[0][1];
            // 0x48: APU_EDMA3_DESP_48
            pst_edma_desc->ui_desp_48_r2y_y2r_mat_02 = (unsigned int)as_color_mat[0][2];
            pst_edma_desc->ui_desp_48_r2y_y2r_vec_0 = (unsigned int)as_color_vec[0];
            // 0x4C: APU_EDMA3_DESP_4C
            pst_edma_desc->ui_desp_4c_r2y_y2r_mat_10 = (unsigned int)as_color_mat[1][0];
            pst_edma_desc->ui_desp_4c_r2y_y2r_mat_11 = (unsigned int)as_color_mat[1][1];
            // 0x50: APU_EDMA3_DESP_50
            pst_edma_desc->ui_desp_50_r2y_y2r_mat_12 = (unsigned int)as_color_mat[1][2];
            pst_edma_desc->ui_desp_50_r2y_y2r_vec_1 = (unsigned int)as_color_vec[1];
            // 0x54: APU_EDMA3_DESP_54
            pst_edma_desc->ui_desp_54_r2y_y2r_mat_20 = (unsigned int)as_color_mat[2][0];
            pst_edma_desc->ui_desp_54_r2y_y2r_mat_21 = (unsigned int)as_color_mat[2][1];
            // 0x58: APU_EDMA3_DESP_58
            pst_edma_desc->ui_desp_58_r2y_y2r_mat_22 = (unsigned int)as_color_mat[2][2];
            pst_edma_desc->ui_desp_58_r2y_y2r_vec_2 = (unsigned int)as_color_vec[2];
            break;
    }

    // 0x40: APU_EDMA3_DESP_40
    pst_edma_desc->ui_desp_40_ufbc_alg_sel = 0; // TBD
    pst_edma_desc->ui_desp_40_ufbc_yuv_transform = 0; // TBD
    pst_edma_desc->ui_desp_40_ufbc_align_payload_offset = 0; // TBD
    pst_edma_desc->ui_desp_40_ufbc_dummy_write = 0; // TBD
    pst_edma_desc->ui_desp_40_ufbc_secure_mode = 0; // TBD
    pst_edma_desc->ui_desp_40_reserved_0 = 0;
    pst_edma_desc->ui_desp_40_act_msb_flip = 0; // TBD
    pst_edma_desc->ui_desp_40_act_dec_asm_left = 0; // TBD
    pst_edma_desc->ui_desp_40_reserved_1 = 0;
    pst_edma_desc->ui_desp_40_down_sampling_drop = 0;
    pst_edma_desc->ui_desp_40_down_sampling_filter = 0;
    pst_edma_desc->ui_desp_40_src_1_addr_cbfc_extension = 0; // TBD
    pst_edma_desc->ui_desp_40_dst_1_addr_cbfc_extension = 0; // TBD

    EDMA_LOG_INFO("fillDescType5 END\n");
}

void fillDescRotOutBufIn(st_edmaDescInfoRotate *pst_edma_desc_info, st_edmaDescType0 *pst_edma_desc, int *pi_errorCode=NULL)
{
    unsigned int tile_x_size = 1, tile_y_size = 1;
    unsigned char pixel_size = pst_edma_desc_info->pixel_size;
    switch (pixel_size) {
    case 1:
        tile_x_size = MAX_EDMA_ROT_1B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_1B_SIZE_Y;
        break;
    case 2:
        tile_x_size = MAX_EDMA_ROT_2B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_2B_SIZE_Y;
        break;
    case 3:
        tile_x_size = MAX_EDMA_ROT_3B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_3B_SIZE_Y;
        break;
    case 4:
        tile_x_size = MAX_EDMA_ROT_4B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_4B_SIZE_Y;
        break;
    default:
        tile_x_size = 1;
        tile_y_size = 1;
        break;
    }

    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = 0;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_handshake_enable = 0; // TBD
    pst_edma_desc->ui_desp_00_reserved_0 = 0;

    // 0x04: APU_EDMA3_DESP_04
    //checkFormat(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_format = EDMA_FMT_NORMAL_MODE;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    //checkRawShift(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 1;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 0;
    pst_edma_desc->ui_desp_04_write_tmp_buf = 1; // Write data to internal buffer.
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0;
    pst_edma_desc->ui_desp_04_reserved_1 = 0;
    pst_edma_desc->ui_desp_04_raw_shift = 0;
    pst_edma_desc->ui_desp_04_reserved_2 = 0;

    // 0x08: APU_EDMA3_DESP_08
    pst_edma_desc->ui_desp_08_fill_const = 0;

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_i2f_min = 0;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_i2f_scale = 0;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_f2i_min = 0;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_f2i_scale = 0;

    // 0x1C: APU_EDMA3_DESP_1C
    pst_edma_desc->ui_desp_1c_src_addr_0 = pst_edma_desc_info->ui_src_addr_0;

    // 0x20: APU_EDMA3_DESP_20
    pst_edma_desc->ui_desp_20_dst_addr_0 = 0;
    EDMA_LOG_DEBUG("DST  addr: 0x%08x\n", pst_edma_desc->ui_desp_20_dst_addr_0);

    pst_edma_desc->ui_desp_34_src_x_size_0 = pst_edma_desc_info->ui_src_size_x;
    pst_edma_desc->ui_desp_38_src_y_size_0 = pst_edma_desc_info->ui_src_size_y;
    pst_edma_desc->ui_desp_3c_src_z_size_0 = 1;

    pst_edma_desc->ui_desp_24_src_x_stride_0 = pst_edma_desc_info->ui_src_pitch_x_0;
    pst_edma_desc->ui_desp_2c_src_y_stride_0 = pst_edma_desc_info->ui_src_pitch_y_0;

    pst_edma_desc->ui_desp_34_dst_x_size_0 = pst_edma_desc->ui_desp_34_src_x_size_0;
    pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc->ui_desp_38_src_y_size_0;
    pst_edma_desc->ui_desp_3c_dst_z_size_0 = 1;
    pst_edma_desc->ui_desp_28_dst_x_stride_0 = tile_x_size;
    pst_edma_desc->ui_desp_30_dst_y_stride_0 = 1;
    EDMA_LOG_DEBUG("SRC  size: x- %.3d, y- %.3d\n", pst_edma_desc_info->ui_dst_size_x, pst_edma_desc_info->ui_dst_size_y);
    EDMA_LOG_DEBUG("ROTA size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_src_x_size_0, pst_edma_desc->ui_desp_38_src_y_size_0, pst_edma_desc->ui_desp_3c_src_z_size_0);
    EDMA_LOG_DEBUG("   stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_24_src_x_stride_0, pst_edma_desc->ui_desp_2c_src_y_stride_0);
    EDMA_LOG_DEBUG("DST  size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_dst_x_size_0, pst_edma_desc->ui_desp_38_dst_y_size_0, pst_edma_desc->ui_desp_3c_dst_z_size_0);
    EDMA_LOG_DEBUG("   stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_28_dst_x_stride_0, pst_edma_desc->ui_desp_30_dst_y_stride_0);
}

void fillDescRotate90(st_edmaDescInfoRotate *pst_edma_desc_info, st_edmaDescType0 *pst_edma_desc, int *pi_errorCode=NULL)
{
    unsigned int tile_x_size = 1, tile_y_size = 1;
    unsigned char pixel_size = pst_edma_desc_info->pixel_size;

    switch (pixel_size) {
    case 1:
        tile_x_size = MAX_EDMA_ROT_1B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_1B_SIZE_Y;
        break;
    case 2:
        tile_x_size = MAX_EDMA_ROT_2B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_2B_SIZE_Y;
        break;
    case 3:
        tile_x_size = MAX_EDMA_ROT_3B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_3B_SIZE_Y;
        break;
    case 4:
        tile_x_size = MAX_EDMA_ROT_4B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_4B_SIZE_Y;
        break;
    default:
        tile_x_size = 1;
        tile_y_size = 1;
        break;
    }
    EDMA_LOG_DEBUG("pix  size: %d\n", pixel_size);

    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = 0;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_handshake_enable = 0; // TBD
    pst_edma_desc->ui_desp_00_reserved_0 = 0;

    // 0x04: APU_EDMA3_DESP_04
    //checkFormat(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_format = EDMA_FMT_NORMAL_MODE;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    //checkRawShift(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 1;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 1;
    pst_edma_desc->ui_desp_04_write_tmp_buf = 1; // Write data to internal buffer.
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0;
    pst_edma_desc->ui_desp_04_reserved_1 = 0;
    pst_edma_desc->ui_desp_04_raw_shift = 0;
    pst_edma_desc->ui_desp_04_reserved_2 = 0;

    // 0x08: APU_EDMA3_DESP_08
    pst_edma_desc->ui_desp_08_fill_const = 0;

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_i2f_min = 0;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_i2f_scale = 0;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_f2i_min = 0;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_f2i_scale = 0;

    // 0x1C: APU_EDMA3_DESP_1C
    pst_edma_desc->ui_desp_1c_src_addr_0 = 0;
    pst_edma_desc->ui_desp_20_dst_addr_0 = tile_x_size*tile_y_size + pst_edma_desc_info->ui_src_size_y*pixel_size - pixel_size;

    pst_edma_desc->ui_desp_34_src_x_size_0 = pixel_size;
    pst_edma_desc->ui_desp_38_src_y_size_0 = (pst_edma_desc_info->ui_src_size_x+pixel_size-1)/pixel_size;
    pst_edma_desc->ui_desp_3c_src_z_size_0 = pst_edma_desc_info->ui_src_size_y;
    pst_edma_desc->ui_desp_24_src_x_stride_0 = pixel_size;
    pst_edma_desc->ui_desp_2c_src_y_stride_0 = tile_x_size;

    EDMA_LOG_DEBUG("SRC  addr: 0x%08x\n", pst_edma_desc->ui_desp_1c_src_addr_0);
    EDMA_LOG_DEBUG("DST  addr: 0x%08x\n", pst_edma_desc->ui_desp_20_dst_addr_0);
    EDMA_LOG_DEBUG("SRC  size: x- %.3d, y- %.3d\n", pst_edma_desc_info->ui_src_size_x, pst_edma_desc_info->ui_src_size_y);
    EDMA_LOG_DEBUG("ROTA size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_src_x_size_0, pst_edma_desc->ui_desp_38_src_y_size_0, pst_edma_desc->ui_desp_3c_src_z_size_0);
    EDMA_LOG_DEBUG("   stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_24_src_x_stride_0, pst_edma_desc->ui_desp_2c_src_y_stride_0);

    pst_edma_desc->ui_desp_34_dst_x_size_0 = pixel_size;
    pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc->ui_desp_38_src_y_size_0;
    pst_edma_desc->ui_desp_3c_dst_z_size_0 = pst_edma_desc->ui_desp_3c_src_z_size_0;
    pst_edma_desc->ui_desp_28_dst_x_stride_0 = tile_y_size*pixel_size;
    pst_edma_desc->ui_desp_30_dst_y_stride_0 = 0 - pixel_size; // dst y pitch: -4

    EDMA_LOG_DEBUG("DST  size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_dst_x_size_0, pst_edma_desc->ui_desp_38_dst_y_size_0, pst_edma_desc->ui_desp_3c_dst_z_size_0);
    EDMA_LOG_DEBUG("   stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_28_dst_x_stride_0, pst_edma_desc->ui_desp_30_dst_y_stride_0);
}

void fillDescRotate270(st_edmaDescInfoRotate *pst_edma_desc_info, st_edmaDescType0 *pst_edma_desc, int *pi_errorCode=NULL)
{
    unsigned int tile_x_size = 1, tile_y_size = 1;
    unsigned char pixel_size = pst_edma_desc_info->pixel_size;

    switch (pixel_size) {
    case 1:
        tile_x_size = MAX_EDMA_ROT_1B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_1B_SIZE_Y;
        break;
    case 2:
        tile_x_size = MAX_EDMA_ROT_2B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_2B_SIZE_Y;
        break;
    case 3:
        tile_x_size = MAX_EDMA_ROT_3B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_3B_SIZE_Y;
        break;
    case 4:
        tile_x_size = MAX_EDMA_ROT_4B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_4B_SIZE_Y;
        break;
    default:
        tile_x_size = 1;
        tile_y_size = 1;
        break;
    }
    EDMA_LOG_DEBUG("pix  size: %d\n", pixel_size);

    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = 0;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_handshake_enable = 0; // TBD
    pst_edma_desc->ui_desp_00_reserved_0 = 0;

    // 0x04: APU_EDMA3_DESP_04
    //checkFormat(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_format = EDMA_FMT_NORMAL_MODE;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    //checkRawShift(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 1;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 1;
    pst_edma_desc->ui_desp_04_write_tmp_buf = 1; // Write data to internal buffer.
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0;
    pst_edma_desc->ui_desp_04_reserved_1 = 0;
    pst_edma_desc->ui_desp_04_raw_shift = 0;
    pst_edma_desc->ui_desp_04_reserved_2 = 0;

    // 0x08: APU_EDMA3_DESP_08
    pst_edma_desc->ui_desp_08_fill_const = 0;

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_i2f_min = 0;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_i2f_scale = 0;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_f2i_min = 0;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_f2i_scale = 0;

    // 0x1C: APU_EDMA3_DESP_1C
    pst_edma_desc->ui_desp_1c_src_addr_0 = 0;
    pst_edma_desc->ui_desp_20_dst_addr_0 = tile_x_size*tile_y_size + tile_y_size*pixel_size*(pst_edma_desc_info->ui_dst_size_y - 1);

    pst_edma_desc->ui_desp_34_src_x_size_0 = pixel_size;
    pst_edma_desc->ui_desp_38_src_y_size_0 = (pst_edma_desc_info->ui_src_size_x+pixel_size-1)/pixel_size;
    pst_edma_desc->ui_desp_3c_src_z_size_0 = pst_edma_desc_info->ui_src_size_y;
    pst_edma_desc->ui_desp_24_src_x_stride_0 = pixel_size;
    pst_edma_desc->ui_desp_2c_src_y_stride_0 = tile_x_size;

    EDMA_LOG_DEBUG("SRC  addr: 0x%08x\n", pst_edma_desc->ui_desp_1c_src_addr_0);
    EDMA_LOG_DEBUG("DST  addr: 0x%08x\n", pst_edma_desc->ui_desp_20_dst_addr_0);
    EDMA_LOG_DEBUG("SRC  size: x- %.3d, y- %.3d\n", pst_edma_desc_info->ui_src_size_x, pst_edma_desc_info->ui_src_size_y);
    EDMA_LOG_DEBUG("ROTA size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_src_x_size_0, pst_edma_desc->ui_desp_38_src_y_size_0, pst_edma_desc->ui_desp_3c_src_z_size_0);
    EDMA_LOG_DEBUG("   stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_24_src_x_stride_0, pst_edma_desc->ui_desp_2c_src_y_stride_0);

    pst_edma_desc->ui_desp_34_dst_x_size_0 = pixel_size;
    pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc->ui_desp_38_src_y_size_0;
    pst_edma_desc->ui_desp_3c_dst_z_size_0 = pst_edma_desc->ui_desp_3c_src_z_size_0;
    pst_edma_desc->ui_desp_28_dst_x_stride_0 = 0 - tile_y_size*pixel_size;
    pst_edma_desc->ui_desp_30_dst_y_stride_0 = pixel_size; // dst y pitch: -4

    EDMA_LOG_DEBUG("DST  size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_dst_x_size_0, pst_edma_desc->ui_desp_38_dst_y_size_0, pst_edma_desc->ui_desp_3c_dst_z_size_0);
    EDMA_LOG_DEBUG("   stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_28_dst_x_stride_0, pst_edma_desc->ui_desp_30_dst_y_stride_0);
}

void fillDescRotate180(st_edmaDescInfoRotate *pst_edma_desc_info, st_edmaDescType15 *pst_edma_desc, int *pi_errorCode=NULL)
{
    unsigned int tile_x_size = 1, tile_y_size = 1, work_size = 0;
    unsigned char pixel_size = pst_edma_desc_info->pixel_size;
    unsigned int work_pixel = 1;

    switch (pixel_size) {
    case 1:
        tile_x_size = MAX_EDMA_ROT_1B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_1B_SIZE_Y;
        if (pst_edma_desc_info->ui_src_size_x % 4 == 0) {
            work_pixel = 4;
            work_size = 4*pixel_size;
        } else {
            work_pixel = 1;
            work_size = pixel_size;
        }
        break;
    case 2:
        tile_x_size = MAX_EDMA_ROT_2B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_2B_SIZE_Y;
        work_pixel = 1;
        work_size = pixel_size;
        break;
    case 3:
        tile_x_size = MAX_EDMA_ROT_3B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_3B_SIZE_Y;
        work_pixel = 1;
        work_size = pixel_size;
        break;
    case 4:
        tile_x_size = MAX_EDMA_ROT_4B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_4B_SIZE_Y;
        work_pixel = 1;
        work_size = pixel_size;
        break;
    default:
        tile_x_size = 1;
        tile_y_size = 1;
        work_pixel = 1;
        work_size = pixel_size;
        break;
    }
    EDMA_LOG_DEBUG("pix  size: %d\n", pixel_size);
    EDMA_LOG_DEBUG("work size: %d\n", work_size);
    EDMA_LOG_DEBUG("work pixl: %d\n", work_pixel);

    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = 15;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_handshake_enable = 0; // TBD
    pst_edma_desc->ui_desp_00_reserved_0 = 0;

    // 0x04: APU_EDMA3_DESP_04
    //checkFormat(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_format = 0;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    //checkRawShift(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 1;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 1;
    pst_edma_desc->ui_desp_04_write_tmp_buf = 1; // Write data to internal buffer.
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0;
    pst_edma_desc->ui_desp_04_reserved_1 = 0;
    pst_edma_desc->ui_desp_04_pad_const = 0;

    // 0x08: APU_EDMA3_DESP_08
    if (work_pixel == 1)
        pst_edma_desc->ui_desp_08_in_swap_mat = 0x8421;
    else if (work_pixel == 4)
        pst_edma_desc->ui_desp_08_in_swap_mat = 0x1248;

    pst_edma_desc->ui_desp_08_out_swap_mat = 0x8421;

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_ufbc_tile_width = 0;
    pst_edma_desc->ui_desp_0c_reserved_0 = 0;
    pst_edma_desc->ui_desp_0c_ufbc_tile_height = 0;
    pst_edma_desc->ui_desp_0c_reserved_1 = 0;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_ufbc_frame_width = 0;
    pst_edma_desc->ui_desp_10_reserved_0 = 0;
    pst_edma_desc->ui_desp_10_ufbc_frame_height = 0;
    pst_edma_desc->ui_desp_10_reserved_1 = 0;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_ufbc_x_offset = 0;
    pst_edma_desc->ui_desp_14_reserved_0 = 0;
    pst_edma_desc->ui_desp_14_ufbc_y_offset = 0;
    pst_edma_desc->ui_desp_14_reserved_1 = 0;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_ufbc_payload_offset = 0;

    // 0x1C: APU_EDMA3_DESP_1C
    pst_edma_desc->ui_desp_1c_src_addr_0 = 0;
    EDMA_LOG_DEBUG("SRC  addr: 0x%08x\n", pst_edma_desc->ui_desp_1c_src_addr_0);

    pst_edma_desc->ui_desp_20_dst_addr_0 = tile_x_size*tile_y_size + tile_x_size*(pst_edma_desc_info->ui_dst_size_y - 1) + pst_edma_desc_info->ui_dst_size_x - work_size;
    pst_edma_desc->ui_desp_34_src_x_size_0 = pst_edma_desc_info->ui_src_size_x;
    pst_edma_desc->ui_desp_38_src_y_size_0 = pst_edma_desc_info->ui_src_size_y;

    pst_edma_desc->ui_desp_3c_src_z_size_0 = 1;
    pst_edma_desc->ui_desp_24_src_x_stride_0 = tile_x_size;
    pst_edma_desc->ui_desp_2c_src_y_stride_0 = tile_y_size;

    EDMA_LOG_DEBUG("DST  addr: 0x%08x\n", pst_edma_desc->ui_desp_20_dst_addr_0);
    EDMA_LOG_DEBUG("SRC  size: x- %.3d, y- %.3d\n", pst_edma_desc_info->ui_src_size_x, pst_edma_desc_info->ui_src_size_y);
    EDMA_LOG_DEBUG("ROTA size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_src_x_size_0, pst_edma_desc->ui_desp_38_src_y_size_0, pst_edma_desc->ui_desp_3c_src_z_size_0);
    EDMA_LOG_DEBUG("   stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_24_src_x_stride_0, pst_edma_desc->ui_desp_2c_src_y_stride_0);

    pst_edma_desc->ui_desp_34_dst_x_size_0 = work_size;
    pst_edma_desc->ui_desp_38_dst_y_size_0 = (pst_edma_desc_info->ui_dst_size_x+work_size-1)/work_size;
    pst_edma_desc->ui_desp_3c_dst_z_size_0 = pst_edma_desc_info->ui_dst_size_y;
    pst_edma_desc->ui_desp_28_dst_x_stride_0 = 0 - work_size;
    pst_edma_desc->ui_desp_30_dst_y_stride_0 = 0 - tile_x_size; // dst y pitch: -4

    EDMA_LOG_DEBUG("DST  size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_dst_x_size_0, pst_edma_desc->ui_desp_38_dst_y_size_0, pst_edma_desc->ui_desp_3c_dst_z_size_0);
    EDMA_LOG_DEBUG("   stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_28_dst_x_stride_0, pst_edma_desc->ui_desp_30_dst_y_stride_0);

    // 0x40: APU_EDMA3_DESP_40
    pst_edma_desc->ui_desp_40_ufbc_alg_sel = 0;
    pst_edma_desc->ui_desp_40_ufbc_yuv_transform = 0;
    pst_edma_desc->ui_desp_40_ufbc_align_payload_offset = 0;
    pst_edma_desc->ui_desp_40_ufbc_dummy_write = 0;
    pst_edma_desc->ui_desp_40_ufbc_secure_mode = 0;
    pst_edma_desc->ui_desp_40_reserved_0 = 0;
    pst_edma_desc->ui_desp_40_act_msb_flip = 0;
    pst_edma_desc->ui_desp_40_act_dec_asm_left = 0;
    pst_edma_desc->ui_desp_40_reserved_1 = 0;
    pst_edma_desc->ui_desp_40_down_sampling_drop = 0;
    pst_edma_desc->ui_desp_40_down_sampling_filter = 0;
    pst_edma_desc->ui_desp_40_reserved_2 = 0;

    // 0x44: APU_EDMA3_DESP_44
    pst_edma_desc->ui_desp_44_r2y_y2r_mat_00 = 0;
    pst_edma_desc->ui_desp_44_r2y_y2r_mat_01 = 0;

    // 0x48: APU_EDMA3_DESP_48
    pst_edma_desc->ui_desp_48_r2y_y2r_mat_02 = 0;
    pst_edma_desc->ui_desp_48_r2y_y2r_vec_0 = 0;

    // 0x4C: APU_EDMA3_DESP_4C
    pst_edma_desc->ui_desp_4c_r2y_y2r_mat_10 = 0;
    pst_edma_desc->ui_desp_4c_r2y_y2r_mat_11 = 0;

    // 0x50: APU_EDMA3_DESP_50
    pst_edma_desc->ui_desp_50_r2y_y2r_mat_12 = 0;
    pst_edma_desc->ui_desp_50_r2y_y2r_vec_1 = 0;

    // 0x54: APU_EDMA3_DESP_54
    pst_edma_desc->ui_desp_54_r2y_y2r_mat_20 = 0;
    pst_edma_desc->ui_desp_54_r2y_y2r_mat_21 = 0;

    // 0x58: APU_EDMA3_DESP_58
    pst_edma_desc->ui_desp_58_r2y_y2r_mat_22 = 0;
    pst_edma_desc->ui_desp_58_r2y_y2r_vec_2 = 0;

    // 0x5C: APU_EDMA3_DESP_5C
    pst_edma_desc->ui_desp_5c_src_addr_1 = 0;

    // 0x60: APU_EDMA3_DESP_60
    pst_edma_desc->ui_desp_60_dst_addr_1 = 0;

    // 0x64: APU_EDMA3_DESP_64
    pst_edma_desc->ui_desp_64_src_x_stride_1 = 0;

    // 0x68: APU_EDMA3_DESP_68
    pst_edma_desc->ui_desp_68_dst_x_stride_1 = 0;

    // 0x6C: APU_EDMA3_DESP_6C
    pst_edma_desc->ui_desp_6c_src_y_stride_1 = 0;

    // 0x70: APU_EDMA3_DESP_70
    pst_edma_desc->ui_desp_70_dst_y_stride_1 = 0;

    // 0x74: APU_EDMA3_DESP_74
    pst_edma_desc->ui_desp_74_src_x_size_1 = 0;
    pst_edma_desc->ui_desp_74_dst_x_size_1 = 0;

    // 0x78: APU_EDMA3_DESP_78
    pst_edma_desc->ui_desp_78_src_y_size_1 = 0;
    pst_edma_desc->ui_desp_78_dst_y_size_1 = 0;

    // 0x7C: APU_EDMA3_DESP_7C
    pst_edma_desc->ui_desp_7c_src_z_size_1 = 0;
    pst_edma_desc->ui_desp_7c_dst_z_size_1 = 0;

    // 0x80: APU_EDMA3_DESP_80
    pst_edma_desc->ui_desp_80_in_extractor_0 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_1 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_2 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_3 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_0 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_1 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_2 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_3 = 0;

    // 0x84: APU_EDMA3_DESP_84
    pst_edma_desc->ui_desp_84_in_zero_padder_0 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_1 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_2 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_3 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_0 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_1 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_2 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_3 = 0;

    // 0x88: APU_EDMA3_DESP_88
    if (work_pixel == 1)
        pst_edma_desc->ui_desp_88_router = 1;
    else if (work_pixel == 4)
        pst_edma_desc->ui_desp_88_router = 0;

    pst_edma_desc->ui_desp_88_merger = 0;
    pst_edma_desc->ui_desp_88_splitter = 0;

    if (work_pixel == 1)
        pst_edma_desc->ui_desp_88_in_swapper = 0;
    else if (work_pixel == 4)
        pst_edma_desc->ui_desp_88_in_swapper = 3;

    pst_edma_desc->ui_desp_88_out_swapper = 0;
    pst_edma_desc->ui_desp_88_extractor = 0;
    pst_edma_desc->ui_desp_88_constant_padder = 0;
    pst_edma_desc->ui_desp_88_converter = 0;

    // 0x8C: APU_EDMA3_DESP_8C
    pst_edma_desc->ui_desp_8c_line_packer = 0;
    pst_edma_desc->ui_desp_8c_up_sampler = 0;
    pst_edma_desc->ui_desp_8c_down_sampler = 0;
    pst_edma_desc->ui_desp_8c_reserved_0 = 0;

    // 0x90: APU_EDMA3_DESP_90
    pst_edma_desc->ui_desp_90_ar_sender_0 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_1 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_2 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_3 = 1;
    pst_edma_desc->ui_desp_90_r_receiver_0 = work_size - 1;
    pst_edma_desc->ui_desp_90_r_receiver_1 = 0;
    pst_edma_desc->ui_desp_90_r_receiver_2 = 0;
    pst_edma_desc->ui_desp_90_r_receiver_3 = 0;

    // 0x94: APU_EDMA3_DESP_94
    pst_edma_desc->ui_desp_94_aw_sender_0 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_1 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_2 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_3 = 2;

    pst_edma_desc->ui_desp_94_w_sender_0 = work_size - 1;
    pst_edma_desc->ui_desp_94_w_sender_1 = 0;
    pst_edma_desc->ui_desp_94_w_sender_2 = 0;
    pst_edma_desc->ui_desp_94_w_sender_3 = 0;

    // 0x98: APU_EDMA3_DESP_98
    pst_edma_desc->ui_desp_98_reader = 1;
    pst_edma_desc->ui_desp_98_writer = 1;
    pst_edma_desc->ui_desp_98_encoder = 0;
    pst_edma_desc->ui_desp_98_decoder = 0;
    pst_edma_desc->ui_desp_98_act_encoder = 0;
    pst_edma_desc->ui_desp_98_act_decoder = 0;
    pst_edma_desc->ui_desp_98_pipe_enable = 0x11;

    // 0x9C: APU_EDMA3_DESP_9C
    pst_edma_desc->ui_desp_9c_src_addr_2 = 0;

    // 0xA0: APU_EDMA3_DESP_A0
    pst_edma_desc->ui_desp_a0_dst_addr_2 = 0;

    // 0xA4: APU_EDMA3_DESP_A4
    pst_edma_desc->ui_desp_a4_src_x_stride_2 = 0;

    // 0xA8: APU_EDMA3_DESP_A8
    pst_edma_desc->ui_desp_a8_dst_x_stride_2 = 0;

    // 0xAC: APU_EDMA3_DESP_AC
    pst_edma_desc->ui_desp_ac_src_y_stride_2 = 0;

    // 0xB0: APU_EDMA3_DESP_B0
    pst_edma_desc->ui_desp_b0_dst_y_stride_2 = 0;

    // 0xB4: APU_EDMA3_DESP_B4
    pst_edma_desc->ui_desp_b4_src_x_size_2 = 0;
    pst_edma_desc->ui_desp_b4_dst_x_size_2 = 0;

    // 0xB8: APU_EDMA3_DESP_B8
    pst_edma_desc->ui_desp_b8_src_y_size_2 = 0;
    pst_edma_desc->ui_desp_b8_dst_y_size_2 = 0;

    // 0xBC: APU_EDMA3_DESP_BC
    pst_edma_desc->ui_desp_bc_src_z_size_2 = 0;
    pst_edma_desc->ui_desp_bc_dst_z_size_2 = 0;

    // 0xC0: APU_EDMA3_DESP_C0
    pst_edma_desc->ui_desp_c0_reserved_0 = 0;

    // 0xC4: APU_EDMA3_DESP_C4
    pst_edma_desc->ui_desp_c4_reserved_0 = 0;

    // 0xC8: APU_EDMA3_DESP_C8
    pst_edma_desc->ui_desp_c8_reserved_0 = 0;

    // 0xCC: APU_EDMA3_DESP_CC
    pst_edma_desc->ui_desp_cc_reserved_0 = 0;

    // 0xD0: APU_EDMA3_DESP_D0
    pst_edma_desc->ui_desp_d0_reserved_0 = 0;

    // 0xD4: APU_EDMA3_DESP_D4
    pst_edma_desc->ui_desp_d4_reserved_0 = 0;

    // 0xD8: APU_EDMA3_DESP_D8
    pst_edma_desc->ui_desp_d8_reserved_0 = 0;

    // 0xDC: APU_EDMA3_DESP_DC
    pst_edma_desc->ui_desp_dc_src_addr_3 = 0;

    // 0xE0: APU_EDMA3_DESP_E0
    pst_edma_desc->ui_desp_e0_dst_addr_3 = 0;

    // 0xE4: APU_EDMA3_DESP_E4
    pst_edma_desc->ui_desp_e4_src_x_stride_3 = 0;

    // 0xE8: APU_EDMA3_DESP_E8
    pst_edma_desc->ui_desp_e8_dst_x_stride_3 = 0;

    // 0xEC: APU_EDMA3_DESP_EC
    pst_edma_desc->ui_desp_ec_src_y_stride_3 = 0;

    // 0xF0: APU_EDMA3_DESP_F0
    pst_edma_desc->ui_desp_f0_dst_y_stride_3 = 0;

    // 0xF4: APU_EDMA3_DESP_F4
    pst_edma_desc->ui_desp_f4_src_x_size_3 = 0;
    pst_edma_desc->ui_desp_f4_dst_x_size_3 = 0;

    // 0xF8: APU_EDMA3_DESP_F8
    pst_edma_desc->ui_desp_f8_src_y_size_3 = 0;
    pst_edma_desc->ui_desp_f8_dst_y_size_3 = 0;

    // 0xFC: APU_EDMA3_DESP_FC
    pst_edma_desc->ui_desp_fc_src_z_size_3 = 0;
    pst_edma_desc->ui_desp_fc_dst_z_size_3 = 0;
}

void fillDescRotInBufOut(st_edmaDescInfoRotate *pst_edma_desc_info, st_edmaDescType0 *pst_edma_desc, int *pi_errorCode=NULL)
{
    unsigned int tile_x_size = 1, tile_y_size = 1;
    unsigned char pixel_size = pst_edma_desc_info->pixel_size;
    switch (pixel_size) {
    case 1:
        tile_x_size = MAX_EDMA_ROT_1B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_1B_SIZE_Y;
        break;
    case 2:
        tile_x_size = MAX_EDMA_ROT_2B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_2B_SIZE_Y;
        break;
    case 3:
        tile_x_size = MAX_EDMA_ROT_3B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_3B_SIZE_Y;
        break;
    case 4:
        tile_x_size = MAX_EDMA_ROT_4B_SIZE_X;
        tile_y_size = MAX_EDMA_ROT_4B_SIZE_Y;
        break;
    default:
        tile_x_size = 1;
        tile_y_size = 1;
        break;
    }

    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = 0;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_handshake_enable = 0; // TBD
    pst_edma_desc->ui_desp_00_reserved_0 = 0;

    // 0x04: APU_EDMA3_DESP_04
    //checkFormat(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_format = EDMA_FMT_NORMAL_MODE;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    //checkRawShift(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 0;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 1;
    pst_edma_desc->ui_desp_04_write_tmp_buf = 0; // Write data to internal buffer.
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0;
    pst_edma_desc->ui_desp_04_reserved_1 = 0;
    pst_edma_desc->ui_desp_04_raw_shift = 0;
    pst_edma_desc->ui_desp_04_reserved_2 = 0;

    // 0x08: APU_EDMA3_DESP_08
    pst_edma_desc->ui_desp_08_fill_const = 0;

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_i2f_min = 0;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_i2f_scale = 0;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_f2i_min = 0;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_f2i_scale = 0;

    // 0x1C: APU_EDMA3_DESP_1C
    pst_edma_desc->ui_desp_1c_src_addr_0 = tile_x_size*tile_y_size;

    // 0x20: APU_EDMA3_DESP_20
    pst_edma_desc->ui_desp_20_dst_addr_0 = pst_edma_desc_info->ui_dst_addr_0;
    EDMA_LOG_DEBUG("DST  addr: 0x%08x\n", pst_edma_desc->ui_desp_20_dst_addr_0);

    pst_edma_desc->ui_desp_34_src_x_size_0 = pst_edma_desc_info->ui_dst_size_x;
    pst_edma_desc->ui_desp_38_src_y_size_0 = pst_edma_desc_info->ui_dst_size_y;
    pst_edma_desc->ui_desp_3c_src_z_size_0 = 1;

    switch(pst_edma_desc_info->uc_fmt) {
    case EDMA_FMT_ROTATE_90:
        pst_edma_desc->ui_desp_24_src_x_stride_0 = tile_y_size*pixel_size;
        break;
    case EDMA_FMT_ROTATE_180:
        pst_edma_desc->ui_desp_24_src_x_stride_0 = tile_x_size;
        break;
    case EDMA_FMT_ROTATE_270:
        pst_edma_desc->ui_desp_24_src_x_stride_0 = tile_y_size*pixel_size;
        break;
    default:
        EDMA_LOG_ERR("unsupported rotate format: %d\n", pst_edma_desc_info->uc_fmt);
        //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
        break;
    }
    //pst_edma_desc->ui_desp_24_src_x_stride_0 = tile_x_size;

    pst_edma_desc->ui_desp_2c_src_y_stride_0 = 1;

    pst_edma_desc->ui_desp_34_dst_x_size_0 = pst_edma_desc_info->ui_dst_size_x;
    pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc_info->ui_dst_size_y;
    pst_edma_desc->ui_desp_3c_dst_z_size_0 = 1;
    pst_edma_desc->ui_desp_28_dst_x_stride_0 = pst_edma_desc_info->ui_dst_pitch_x_0;
    pst_edma_desc->ui_desp_30_dst_y_stride_0 = 1;
    EDMA_LOG_DEBUG("SRC  size: x- %.3d, y- %.3d\n", pst_edma_desc_info->ui_dst_size_x, pst_edma_desc_info->ui_dst_size_y);
    EDMA_LOG_DEBUG("ROTA size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_src_x_size_0, pst_edma_desc->ui_desp_38_src_y_size_0, pst_edma_desc->ui_desp_3c_src_z_size_0);
    EDMA_LOG_DEBUG("   stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_24_src_x_stride_0, pst_edma_desc->ui_desp_2c_src_y_stride_0);
    EDMA_LOG_DEBUG("DST  size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_dst_x_size_0, pst_edma_desc->ui_desp_38_dst_y_size_0, pst_edma_desc->ui_desp_3c_dst_z_size_0);
    EDMA_LOG_DEBUG("   stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_28_dst_x_stride_0, pst_edma_desc->ui_desp_30_dst_y_stride_0);
}

void fillDescSplitNIn(st_edmaDescInfoSplitN *pst_edma_desc_info, st_edmaDescType0 *pst_edma_desc, int *pi_errorCode=NULL)
{
    unsigned int tile_x_size = 1, tile_y_size = 1;
    unsigned char fmt = pst_edma_desc_info->uc_fmt;
    EDMA_LOG_DEBUG("fillDescSplitNIn\n");

    switch (fmt) {
    case EDMA_FMT_SPLIT_2:
        tile_x_size = MAX_EDMA_SPLIT2_PXL_SIZE_X;
        tile_y_size = MAX_EDMA_SPLIT2_SIZE_Y;
        break;
    case EDMA_FMT_SPLIT_3:
        tile_x_size = MAX_EDMA_SPLIT3_PXL_SIZE_X;
        tile_y_size = MAX_EDMA_SPLIT3_SIZE_Y;
        break;;
    case EDMA_FMT_SPLIT_4:
        tile_x_size = MAX_EDMA_SPLIT4_PXL_SIZE_X;
        tile_y_size = MAX_EDMA_SPLIT4_SIZE_Y;
        break;
    default:
        tile_x_size = 1;
        tile_y_size = 1;
        break;
    }
    EDMA_LOG_DEBUG("fmt: %d\n", fmt);

    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = 0;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_handshake_enable = 0; // TBD
    pst_edma_desc->ui_desp_00_reserved_0 = 0;

    // 0x04: APU_EDMA3_DESP_04
    pst_edma_desc->ui_desp_04_format = EDMA_FMT_NORMAL_MODE;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 1;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 0;
    pst_edma_desc->ui_desp_04_write_tmp_buf = 1;
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0;
    pst_edma_desc->ui_desp_04_reserved_1 = 0;
    pst_edma_desc->ui_desp_04_raw_shift = 0;
    pst_edma_desc->ui_desp_04_reserved_2 = 0;

    // 0x08: APU_EDMA3_DESP_08
    pst_edma_desc->ui_desp_08_fill_const = 0;

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_i2f_min = 0;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_i2f_scale = 0;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_f2i_min = 0;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_f2i_scale = 0;

    // 0x1C: APU_EDMA3_DESP_1C
    pst_edma_desc->ui_desp_1c_src_addr_0 = pst_edma_desc_info->ui_src_addr_0;

    // 0x20: APU_EDMA3_DESP_20
    pst_edma_desc->ui_desp_20_dst_addr_0 = 0;

    // 0x24: APU_EDMA3_DESP_24
    pst_edma_desc->ui_desp_24_src_x_stride_0 = pst_edma_desc_info->ui_src_pitch_x_0;

    // 0x28: APU_EDMA3_DESP_28
    pst_edma_desc->ui_desp_28_dst_x_stride_0 = 2*tile_x_size;

    // 0x2C: APU_EDMA3_DESP_2C
    pst_edma_desc->ui_desp_2c_src_y_stride_0 = pst_edma_desc_info->ui_src_pitch_y_0;

    // 0x30: APU_EDMA3_DESP_30
    pst_edma_desc->ui_desp_30_dst_y_stride_0 = tile_y_size;

    // 0x34: APU_EDMA3_DESP_34
    pst_edma_desc->ui_desp_34_src_x_size_0 = pst_edma_desc_info->ui_src_size_x_0;
    pst_edma_desc->ui_desp_34_dst_x_size_0 = pst_edma_desc->ui_desp_34_src_x_size_0;
    EDMA_LOG_DEBUG("src_x_size_0: 0x%08x\n", pst_edma_desc->ui_desp_34_src_x_size_0);
    EDMA_LOG_DEBUG("dst_x_size_0: 0x%08x\n", pst_edma_desc->ui_desp_34_dst_x_size_0);

    // 0x38: APU_EDMA3_DESP_38
    pst_edma_desc->ui_desp_38_src_y_size_0 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc->ui_desp_38_src_y_size_0;
    EDMA_LOG_DEBUG("src_y_size_0: 0x%08x\n", pst_edma_desc->ui_desp_38_src_y_size_0);
    EDMA_LOG_DEBUG("dst_y_size_0: 0x%08x\n", pst_edma_desc->ui_desp_38_dst_y_size_0);

    // 0x3c: APU_EDMA3_DESP_3C
    pst_edma_desc->ui_desp_3c_src_z_size_0 = pst_edma_desc_info->ui_src_size_z_0; // 1
    pst_edma_desc->ui_desp_3c_dst_z_size_0 = pst_edma_desc->ui_desp_3c_src_z_size_0;
}

void fillDescSplitN(st_edmaDescInfoSplitN *pst_edma_desc_info, st_edmaDescType15 *pst_edma_desc, int *pi_errorCode=NULL)
{
    unsigned int tile_x_size = 1, tile_y_size = 1, tile_x_size_out = 1;
    unsigned int chnl_size = 0;
    unsigned int pxl_num = 0;
    unsigned char pixel_size = pst_edma_desc_info->pixel_size;
    unsigned char fmt = pst_edma_desc_info->uc_fmt;
    unsigned char pipe_enable = 0x0;
    unsigned char pipe_enable_num = 0x0;
    unsigned char ch_num = 0x0;
    unsigned char output_mask = pst_edma_desc_info->output_mask & 0x0F;
    EDMA_LOG_DEBUG("fillDescSplitN output_mask: %d\n", output_mask);

    switch (fmt) {
    case EDMA_FMT_SPLIT_2:
        tile_x_size = MAX_EDMA_SPLIT2_PXL_SIZE_X;
        tile_y_size = MAX_EDMA_SPLIT2_SIZE_Y;
        tile_x_size_out = MAX_EDMA_SPLIT2_CH_SIZE_X;

        if(output_mask != 0)
            pipe_enable = (output_mask << 4)|output_mask;
        else
            pipe_enable = 0x33;
        ch_num = 2;
        break;
    case EDMA_FMT_SPLIT_3:
        tile_x_size = MAX_EDMA_SPLIT3_PXL_SIZE_X;
        tile_y_size = MAX_EDMA_SPLIT3_SIZE_Y;
        tile_x_size_out = MAX_EDMA_SPLIT3_CH_SIZE_X;
        tile_x_size_out = MAX_EDMA_SPLIT2_CH_SIZE_X;

        if(output_mask != 0)
            pipe_enable = (output_mask << 4)|output_mask;
        else
            pipe_enable = 0x77;
        ch_num = 3;
        break;
    case EDMA_FMT_SPLIT_4:
        tile_x_size = MAX_EDMA_SPLIT4_PXL_SIZE_X;
        tile_y_size = MAX_EDMA_SPLIT4_SIZE_Y;
        tile_x_size_out = MAX_EDMA_SPLIT4_CH_SIZE_X;
        tile_x_size_out = MAX_EDMA_SPLIT2_CH_SIZE_X;

        if(output_mask != 0)
            pipe_enable = (output_mask << 4)|output_mask;
        else
            pipe_enable = 0xFF;
        ch_num = 4;
        break;
    default:
        tile_x_size = 1;
        tile_y_size = 1;
        tile_x_size_out = 1;
        ch_num = 1;
        break;
    }

    chnl_size = pixel_size/ch_num;
    pxl_num  = pst_edma_desc_info->ui_src_size_x_0/pixel_size;
    EDMA_LOG_DEBUG("pxl  size: %d\n", pixel_size);
    EDMA_LOG_DEBUG("chnl size: %d\n", chnl_size);
    EDMA_LOG_DEBUG("pxl  num : %d\n", pxl_num);

    for (int i = 0; i < 4; i++){
        if (pipe_enable & 1 << i)
            pipe_enable_num++;
    }

    EDMA_LOG_DEBUG("pipe num : %d\n", pipe_enable_num);

    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = 15;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_handshake_enable = 0; // TBD
    pst_edma_desc->ui_desp_00_reserved_0 = 0;

    // 0x04: APU_EDMA3_DESP_04
    //checkFormat(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_format = 0;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    //checkRawShift(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 1;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 1;
    pst_edma_desc->ui_desp_04_write_tmp_buf = 1; // Write data to internal buffer.
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0;
    pst_edma_desc->ui_desp_04_reserved_1 = 0;
    pst_edma_desc->ui_desp_04_pad_const = 0;

    // 0x08: APU_EDMA3_DESP_08
    pst_edma_desc->ui_desp_08_in_swap_mat = 0x8421;
    pst_edma_desc->ui_desp_08_out_swap_mat = 0x8421;

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_ufbc_tile_width = 0;
    pst_edma_desc->ui_desp_0c_reserved_0 = 0;
    pst_edma_desc->ui_desp_0c_ufbc_tile_height = 0;
    pst_edma_desc->ui_desp_0c_reserved_1 = 0;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_ufbc_frame_width = 0;
    pst_edma_desc->ui_desp_10_reserved_0 = 0;
    pst_edma_desc->ui_desp_10_ufbc_frame_height = 0;
    pst_edma_desc->ui_desp_10_reserved_1 = 0;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_ufbc_x_offset = 0;
    pst_edma_desc->ui_desp_14_reserved_0 = 0;
    pst_edma_desc->ui_desp_14_ufbc_y_offset = 0;
    pst_edma_desc->ui_desp_14_reserved_1 = 0;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_ufbc_payload_offset = 0;

    // 0x1C: APU_EDMA3_DESP_1C
    pst_edma_desc->ui_desp_1c_src_addr_0 = chnl_size*0;
    EDMA_LOG_DEBUG("SRC 0 addr: 0x%08x\n", pst_edma_desc->ui_desp_1c_src_addr_0);
    pst_edma_desc->ui_desp_20_dst_addr_0 = tile_x_size + tile_x_size_out*0;
    EDMA_LOG_DEBUG("DST 0 addr: 0x%08x\n", pst_edma_desc->ui_desp_20_dst_addr_0);

    pst_edma_desc->ui_desp_34_src_x_size_0 = chnl_size;
    pst_edma_desc->ui_desp_38_src_y_size_0 = pxl_num;
    pst_edma_desc->ui_desp_3c_src_z_size_0 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_24_src_x_stride_0 = pixel_size;
    pst_edma_desc->ui_desp_2c_src_y_stride_0 = 2*tile_x_size;

    EDMA_LOG_DEBUG("SRC 0 size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_src_x_size_0, pst_edma_desc->ui_desp_38_src_y_size_0, pst_edma_desc->ui_desp_3c_src_z_size_0);
    EDMA_LOG_DEBUG("    stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_24_src_x_stride_0, pst_edma_desc->ui_desp_2c_src_y_stride_0);
    pst_edma_desc->ui_desp_34_dst_x_size_0 = chnl_size;
    pst_edma_desc->ui_desp_38_dst_y_size_0 = pxl_num;
    pst_edma_desc->ui_desp_3c_dst_z_size_0 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_28_dst_x_stride_0 = chnl_size;
    pst_edma_desc->ui_desp_30_dst_y_stride_0 = 2*tile_x_size;

    EDMA_LOG_DEBUG("DST 0 size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_dst_x_size_0, pst_edma_desc->ui_desp_38_dst_y_size_0, pst_edma_desc->ui_desp_3c_dst_z_size_0);
    EDMA_LOG_DEBUG("    stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_28_dst_x_stride_0, pst_edma_desc->ui_desp_30_dst_y_stride_0);

    // 0x40: APU_EDMA3_DESP_40
    pst_edma_desc->ui_desp_40_ufbc_alg_sel = 0;
    pst_edma_desc->ui_desp_40_ufbc_yuv_transform = 0;
    pst_edma_desc->ui_desp_40_ufbc_align_payload_offset = 0;
    pst_edma_desc->ui_desp_40_ufbc_dummy_write = 0;
    pst_edma_desc->ui_desp_40_ufbc_secure_mode = 0;
    pst_edma_desc->ui_desp_40_reserved_0 = 0;
    pst_edma_desc->ui_desp_40_act_msb_flip = 0;
    pst_edma_desc->ui_desp_40_act_dec_asm_left = 0;
    pst_edma_desc->ui_desp_40_reserved_1 = 0;
    pst_edma_desc->ui_desp_40_down_sampling_drop = 0;
    pst_edma_desc->ui_desp_40_down_sampling_filter = 0;
    pst_edma_desc->ui_desp_40_reserved_2 = 0;

    // 0x44: APU_EDMA3_DESP_44
    pst_edma_desc->ui_desp_44_r2y_y2r_mat_00 = 0;
    pst_edma_desc->ui_desp_44_r2y_y2r_mat_01 = 0;

    // 0x48: APU_EDMA3_DESP_48
    pst_edma_desc->ui_desp_48_r2y_y2r_mat_02 = 0;
    pst_edma_desc->ui_desp_48_r2y_y2r_vec_0 = 0;

    // 0x4C: APU_EDMA3_DESP_4C
    pst_edma_desc->ui_desp_4c_r2y_y2r_mat_10 = 0;
    pst_edma_desc->ui_desp_4c_r2y_y2r_mat_11 = 0;

    // 0x50: APU_EDMA3_DESP_50
    pst_edma_desc->ui_desp_50_r2y_y2r_mat_12 = 0;
    pst_edma_desc->ui_desp_50_r2y_y2r_vec_1 = 0;

    // 0x54: APU_EDMA3_DESP_54
    pst_edma_desc->ui_desp_54_r2y_y2r_mat_20 = 0;
    pst_edma_desc->ui_desp_54_r2y_y2r_mat_21 = 0;

    // 0x58: APU_EDMA3_DESP_58
    pst_edma_desc->ui_desp_58_r2y_y2r_mat_22 = 0;
    pst_edma_desc->ui_desp_58_r2y_y2r_vec_2 = 0;

    pst_edma_desc->ui_desp_5c_src_addr_1 = chnl_size*1;
    EDMA_LOG_DEBUG("SRC 1 addr: 0x%08x\n", pst_edma_desc->ui_desp_5c_src_addr_1);
    pst_edma_desc->ui_desp_60_dst_addr_1 = tile_x_size + tile_x_size_out*1;
    EDMA_LOG_DEBUG("DST 1 addr: 0x%08x\n", pst_edma_desc->ui_desp_60_dst_addr_1);

    pst_edma_desc->ui_desp_74_src_x_size_1 = chnl_size;
    pst_edma_desc->ui_desp_78_src_y_size_1 = pxl_num;
    pst_edma_desc->ui_desp_7c_src_z_size_1 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_64_src_x_stride_1 = pixel_size;
    pst_edma_desc->ui_desp_6c_src_y_stride_1 = 2*tile_x_size;

    pst_edma_desc->ui_desp_74_dst_x_size_1 = chnl_size;
    pst_edma_desc->ui_desp_78_dst_y_size_1 = pxl_num;
    pst_edma_desc->ui_desp_7c_dst_z_size_1 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_68_dst_x_stride_1 = chnl_size;
    pst_edma_desc->ui_desp_70_dst_y_stride_1 = 2*tile_x_size;

    // 0x80: APU_EDMA3_DESP_80
    pst_edma_desc->ui_desp_80_in_extractor_0 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_1 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_2 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_3 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_0 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_1 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_2 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_3 = 0;

    // 0x84: APU_EDMA3_DESP_84
    pst_edma_desc->ui_desp_84_in_zero_padder_0 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_1 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_2 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_3 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_0 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_1 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_2 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_3 = 0;

    // 0x88: APU_EDMA3_DESP_88
    pst_edma_desc->ui_desp_88_router = 1;
    pst_edma_desc->ui_desp_88_merger = 0;
    pst_edma_desc->ui_desp_88_splitter = 0;
    pst_edma_desc->ui_desp_88_in_swapper = 0;
    pst_edma_desc->ui_desp_88_out_swapper = 0;
    pst_edma_desc->ui_desp_88_extractor = 0;
    pst_edma_desc->ui_desp_88_constant_padder = 0;
    pst_edma_desc->ui_desp_88_converter = 0;

    // 0x8C: APU_EDMA3_DESP_8C
    pst_edma_desc->ui_desp_8c_line_packer = 0;
    pst_edma_desc->ui_desp_8c_up_sampler = 0;
    pst_edma_desc->ui_desp_8c_down_sampler = 0;
    pst_edma_desc->ui_desp_8c_reserved_0 = 0;

    // 0x90: APU_EDMA3_DESP_90
    pst_edma_desc->ui_desp_90_ar_sender_0 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_1 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_2 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_3 = 1;
    pst_edma_desc->ui_desp_90_r_receiver_0 = chnl_size - 1;
    pst_edma_desc->ui_desp_90_r_receiver_1 = chnl_size - 1;
    pst_edma_desc->ui_desp_90_r_receiver_2 = chnl_size - 1;
    pst_edma_desc->ui_desp_90_r_receiver_3 = chnl_size - 1;

    // 0x94: APU_EDMA3_DESP_94
    pst_edma_desc->ui_desp_94_aw_sender_0 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_1 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_2 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_3 = 2;
    pst_edma_desc->ui_desp_94_w_sender_0 = chnl_size - 1;
    pst_edma_desc->ui_desp_94_w_sender_1 = chnl_size - 1;
    pst_edma_desc->ui_desp_94_w_sender_2 = chnl_size - 1;
    pst_edma_desc->ui_desp_94_w_sender_3 = chnl_size - 1;

    // 0x98: APU_EDMA3_DESP_98
    pst_edma_desc->ui_desp_98_reader = pipe_enable_num;
    pst_edma_desc->ui_desp_98_writer = pipe_enable_num;
    pst_edma_desc->ui_desp_98_encoder = 0;
    pst_edma_desc->ui_desp_98_decoder = 0;
    pst_edma_desc->ui_desp_98_act_encoder = 0;
    pst_edma_desc->ui_desp_98_act_decoder = 0;
    pst_edma_desc->ui_desp_98_pipe_enable = pipe_enable;

    pst_edma_desc->ui_desp_9c_src_addr_2 = chnl_size*2;
    EDMA_LOG_DEBUG("SRC 2 addr: 0x%08x\n", pst_edma_desc->ui_desp_9c_src_addr_2);
    pst_edma_desc->ui_desp_a0_dst_addr_2 = tile_x_size + tile_x_size_out*2;
    EDMA_LOG_DEBUG("DST 2 addr: 0x%08x\n", pst_edma_desc->ui_desp_a0_dst_addr_2);

    pst_edma_desc->ui_desp_b4_src_x_size_2 = chnl_size;
    pst_edma_desc->ui_desp_b8_src_y_size_2 = pxl_num;
    pst_edma_desc->ui_desp_bc_src_z_size_2 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_a4_src_x_stride_2 = pixel_size;
    pst_edma_desc->ui_desp_ac_src_y_stride_2 = 2*tile_x_size;

    pst_edma_desc->ui_desp_b4_dst_x_size_2 = chnl_size;
    pst_edma_desc->ui_desp_b8_dst_y_size_2 = pxl_num;
    pst_edma_desc->ui_desp_bc_dst_z_size_2 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_a8_dst_x_stride_2 = chnl_size;
    pst_edma_desc->ui_desp_b0_dst_y_stride_2 = 2*tile_x_size;

    // 0xC0: APU_EDMA3_DESP_C0
    pst_edma_desc->ui_desp_c0_reserved_0 = 0;

    // 0xC4: APU_EDMA3_DESP_C4
    pst_edma_desc->ui_desp_c4_reserved_0 = 0;

    // 0xC8: APU_EDMA3_DESP_C8
    pst_edma_desc->ui_desp_c8_reserved_0 = 0;

    // 0xCC: APU_EDMA3_DESP_CC
    pst_edma_desc->ui_desp_cc_reserved_0 = 0;

    // 0xD0: APU_EDMA3_DESP_D0
    pst_edma_desc->ui_desp_d0_reserved_0 = 0;

    // 0xD4: APU_EDMA3_DESP_D4
    pst_edma_desc->ui_desp_d4_reserved_0 = 0;

    // 0xD8: APU_EDMA3_DESP_D8
    pst_edma_desc->ui_desp_d8_reserved_0 = 0;

    pst_edma_desc->ui_desp_dc_src_addr_3 = chnl_size*3;
    EDMA_LOG_DEBUG("SRC 3 addr: 0x%08x\n", pst_edma_desc->ui_desp_dc_src_addr_3);
    pst_edma_desc->ui_desp_e0_dst_addr_3 = tile_x_size + tile_x_size_out*3;
    EDMA_LOG_DEBUG("DST 3 addr: 0x%08x\n", pst_edma_desc->ui_desp_e0_dst_addr_3);

    pst_edma_desc->ui_desp_f4_src_x_size_3 = chnl_size;
    pst_edma_desc->ui_desp_f8_src_y_size_3 = pxl_num;
    pst_edma_desc->ui_desp_fc_src_z_size_3 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_e4_src_x_stride_3 = pixel_size;
    pst_edma_desc->ui_desp_ec_src_y_stride_3 = 2*tile_x_size;

    pst_edma_desc->ui_desp_f4_dst_x_size_3 = chnl_size;
    pst_edma_desc->ui_desp_f8_dst_y_size_3 = pxl_num;
    pst_edma_desc->ui_desp_fc_dst_z_size_3 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_e8_dst_x_stride_3 = chnl_size;
    pst_edma_desc->ui_desp_f0_dst_y_stride_3 = 2*tile_x_size;
}


void fillDescSplitNOut(st_edmaDescInfoSplitN *pst_edma_desc_info, st_edmaDescType15 *pst_edma_desc, int *pi_errorCode=NULL)
{
    unsigned int tile_x_size = 1, tile_y_size = 1, tile_x_size_out = 1;
    unsigned int chnl_size = 0;
    unsigned int pxl_num = 0;
    unsigned char pixel_size = pst_edma_desc_info->pixel_size;
    unsigned char fmt = pst_edma_desc_info->uc_fmt;
    unsigned char pipe_enable = 0x0;
    unsigned char pipe_enable_num = 0x0;
    unsigned char ch_num = 0x0;
    unsigned char output_mask = pst_edma_desc_info->output_mask & 0x0F;
    EDMA_LOG_DEBUG("fillDescSplitNOut output_mask: %d\n", output_mask);

    switch (fmt) {
    case EDMA_FMT_SPLIT_2:
        tile_x_size = MAX_EDMA_SPLIT2_PXL_SIZE_X;
        tile_y_size = MAX_EDMA_SPLIT2_SIZE_Y;
        tile_x_size_out = MAX_EDMA_SPLIT2_CH_SIZE_X;

        if(output_mask != 0)
            pipe_enable = (output_mask << 4)|output_mask;
        else
            pipe_enable = 0x33;
        ch_num = 2;
        break;
    case EDMA_FMT_SPLIT_3:
        tile_x_size = MAX_EDMA_SPLIT3_PXL_SIZE_X;
        tile_y_size = MAX_EDMA_SPLIT3_SIZE_Y;
        tile_x_size_out = MAX_EDMA_SPLIT3_CH_SIZE_X;

        if(output_mask != 0)
            pipe_enable = (output_mask << 4)|output_mask;
        else
            pipe_enable = 0x77;
        ch_num = 3;
        break;
    case EDMA_FMT_SPLIT_4:
        tile_x_size = MAX_EDMA_SPLIT4_PXL_SIZE_X;
        tile_y_size = MAX_EDMA_SPLIT4_SIZE_Y;
        tile_x_size_out = MAX_EDMA_SPLIT4_CH_SIZE_X;

        if(output_mask != 0)
            pipe_enable = (output_mask << 4)|output_mask;
        else
            pipe_enable = 0xFF;
        ch_num = 4;
        break;
    default:
        tile_x_size = 1;
        tile_y_size = 1;
        chnl_size = 1;
        ch_num = 1;
        break;
    }

    chnl_size = pixel_size/ch_num;
    pxl_num  = pst_edma_desc_info->ui_dst_size_x_0/chnl_size;
    EDMA_LOG_DEBUG("pxl  size: %d\n", pixel_size);
    EDMA_LOG_DEBUG("chnl size: %d\n", chnl_size);
    EDMA_LOG_DEBUG("pxl  num : %d\n", pxl_num);

    for (int i = 0; i < 4; i++){
        if (pipe_enable & 1 << i)
            pipe_enable_num++;
    }

    EDMA_LOG_DEBUG("pipe num : %d\n", pipe_enable_num);

    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = 15;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_handshake_enable = 0; // TBD
    pst_edma_desc->ui_desp_00_reserved_0 = 0;

    // 0x04: APU_EDMA3_DESP_04
    //checkFormat(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_format = 0;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    //checkRawShift(pst_edma_desc_info, pi_errorCode);
    //EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 0;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 1;
    pst_edma_desc->ui_desp_04_write_tmp_buf = 0; // Write data to internal buffer.
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0;
    pst_edma_desc->ui_desp_04_reserved_1 = 0;
    pst_edma_desc->ui_desp_04_pad_const = 0;

    // 0x08: APU_EDMA3_DESP_08
    pst_edma_desc->ui_desp_08_in_swap_mat = 0x8421;
    pst_edma_desc->ui_desp_08_out_swap_mat = 0x8421;

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_ufbc_tile_width = 0;
    pst_edma_desc->ui_desp_0c_reserved_0 = 0;
    pst_edma_desc->ui_desp_0c_ufbc_tile_height = 0;
    pst_edma_desc->ui_desp_0c_reserved_1 = 0;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_ufbc_frame_width = 0;
    pst_edma_desc->ui_desp_10_reserved_0 = 0;
    pst_edma_desc->ui_desp_10_ufbc_frame_height = 0;
    pst_edma_desc->ui_desp_10_reserved_1 = 0;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_ufbc_x_offset = 0;
    pst_edma_desc->ui_desp_14_reserved_0 = 0;
    pst_edma_desc->ui_desp_14_ufbc_y_offset = 0;
    pst_edma_desc->ui_desp_14_reserved_1 = 0;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_ufbc_payload_offset = 0;

    // 0x1C: APU_EDMA3_DESP_1C
    pst_edma_desc->ui_desp_1c_src_addr_0 = tile_x_size + tile_x_size_out*0;
    EDMA_LOG_DEBUG("SRC 0 addr: 0x%08x\n", pst_edma_desc->ui_desp_1c_src_addr_0);
    pst_edma_desc->ui_desp_20_dst_addr_0 = pst_edma_desc_info->ui_dst_addr_0;
    EDMA_LOG_DEBUG("DST 0 addr: 0x%08x\n", pst_edma_desc->ui_desp_20_dst_addr_0);

    pst_edma_desc->ui_desp_34_src_x_size_0 = pst_edma_desc_info->ui_dst_size_x_0;
    pst_edma_desc->ui_desp_38_src_y_size_0 = pst_edma_desc_info->ui_dst_size_y_0;
    pst_edma_desc->ui_desp_3c_src_z_size_0 = 1;
    pst_edma_desc->ui_desp_24_src_x_stride_0 = 2*tile_x_size;
    pst_edma_desc->ui_desp_2c_src_y_stride_0 = 1;

    EDMA_LOG_DEBUG("SRC 0 size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_src_x_size_0, pst_edma_desc->ui_desp_38_src_y_size_0, pst_edma_desc->ui_desp_3c_src_z_size_0);
    EDMA_LOG_DEBUG("    stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_24_src_x_stride_0, pst_edma_desc->ui_desp_2c_src_y_stride_0);
    pst_edma_desc->ui_desp_34_dst_x_size_0 = pst_edma_desc_info->ui_dst_size_x_0;
    pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc_info->ui_dst_size_y_0;
    pst_edma_desc->ui_desp_3c_dst_z_size_0 = 1;
    pst_edma_desc->ui_desp_28_dst_x_stride_0 = pst_edma_desc_info->ui_dst_pitch_x_0;
    pst_edma_desc->ui_desp_30_dst_y_stride_0 = pst_edma_desc_info->ui_dst_pitch_y_0;

    EDMA_LOG_DEBUG("DST 0 size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_dst_x_size_0, pst_edma_desc->ui_desp_38_dst_y_size_0, pst_edma_desc->ui_desp_3c_dst_z_size_0);
    EDMA_LOG_DEBUG("    stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_28_dst_x_stride_0, pst_edma_desc->ui_desp_30_dst_y_stride_0);

    // 0x40: APU_EDMA3_DESP_40
    pst_edma_desc->ui_desp_40_ufbc_alg_sel = 0;
    pst_edma_desc->ui_desp_40_ufbc_yuv_transform = 0;
    pst_edma_desc->ui_desp_40_ufbc_align_payload_offset = 0;
    pst_edma_desc->ui_desp_40_ufbc_dummy_write = 0;
    pst_edma_desc->ui_desp_40_ufbc_secure_mode = 0;
    pst_edma_desc->ui_desp_40_reserved_0 = 0;
    pst_edma_desc->ui_desp_40_act_msb_flip = 0;
    pst_edma_desc->ui_desp_40_act_dec_asm_left = 0;
    pst_edma_desc->ui_desp_40_reserved_1 = 0;
    pst_edma_desc->ui_desp_40_down_sampling_drop = 0;
    pst_edma_desc->ui_desp_40_down_sampling_filter = 0;
    pst_edma_desc->ui_desp_40_reserved_2 = 0;

    // 0x44: APU_EDMA3_DESP_44
    pst_edma_desc->ui_desp_44_r2y_y2r_mat_00 = 0;
    pst_edma_desc->ui_desp_44_r2y_y2r_mat_01 = 0;

    // 0x48: APU_EDMA3_DESP_48
    pst_edma_desc->ui_desp_48_r2y_y2r_mat_02 = 0;
    pst_edma_desc->ui_desp_48_r2y_y2r_vec_0 = 0;

    // 0x4C: APU_EDMA3_DESP_4C
    pst_edma_desc->ui_desp_4c_r2y_y2r_mat_10 = 0;
    pst_edma_desc->ui_desp_4c_r2y_y2r_mat_11 = 0;

    // 0x50: APU_EDMA3_DESP_50
    pst_edma_desc->ui_desp_50_r2y_y2r_mat_12 = 0;
    pst_edma_desc->ui_desp_50_r2y_y2r_vec_1 = 0;

    // 0x54: APU_EDMA3_DESP_54
    pst_edma_desc->ui_desp_54_r2y_y2r_mat_20 = 0;
    pst_edma_desc->ui_desp_54_r2y_y2r_mat_21 = 0;

    // 0x58: APU_EDMA3_DESP_58
    pst_edma_desc->ui_desp_58_r2y_y2r_mat_22 = 0;
    pst_edma_desc->ui_desp_58_r2y_y2r_vec_2 = 0;

    pst_edma_desc->ui_desp_5c_src_addr_1 = tile_x_size + tile_x_size_out*1;
    EDMA_LOG_DEBUG("SRC 1 addr: 0x%08x\n", pst_edma_desc->ui_desp_5c_src_addr_1);
    pst_edma_desc->ui_desp_60_dst_addr_1 = pst_edma_desc_info->ui_dst_addr_1;
    EDMA_LOG_DEBUG("DST 1 addr: 0x%08x\n", pst_edma_desc->ui_desp_60_dst_addr_1);

    pst_edma_desc->ui_desp_74_src_x_size_1 = pst_edma_desc_info->ui_dst_size_x_1;
    pst_edma_desc->ui_desp_78_src_y_size_1 = pst_edma_desc_info->ui_dst_size_y_1;
    pst_edma_desc->ui_desp_7c_src_z_size_1 = 1;
    pst_edma_desc->ui_desp_64_src_x_stride_1 = 2*tile_x_size;
    pst_edma_desc->ui_desp_6c_src_y_stride_1 = 1;

    pst_edma_desc->ui_desp_74_dst_x_size_1 = pst_edma_desc_info->ui_dst_size_x_1;
    pst_edma_desc->ui_desp_78_dst_y_size_1 = pst_edma_desc_info->ui_dst_size_y_1;
    pst_edma_desc->ui_desp_7c_dst_z_size_1 = 1;
    pst_edma_desc->ui_desp_68_dst_x_stride_1 = pst_edma_desc_info->ui_dst_pitch_x_1;
    pst_edma_desc->ui_desp_70_dst_y_stride_1 = pst_edma_desc_info->ui_dst_pitch_y_1;

    // 0x80: APU_EDMA3_DESP_80
    pst_edma_desc->ui_desp_80_in_extractor_0 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_1 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_2 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_3 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_0 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_1 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_2 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_3 = 0;

    // 0x84: APU_EDMA3_DESP_84
    pst_edma_desc->ui_desp_84_in_zero_padder_0 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_1 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_2 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_3 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_0 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_1 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_2 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_3 = 0;

    // 0x88: APU_EDMA3_DESP_88
    pst_edma_desc->ui_desp_88_router = 1;
    pst_edma_desc->ui_desp_88_merger = 0;
    pst_edma_desc->ui_desp_88_splitter = 0;
    pst_edma_desc->ui_desp_88_in_swapper = 0;
    pst_edma_desc->ui_desp_88_out_swapper = 0;
    pst_edma_desc->ui_desp_88_extractor = 0;
    pst_edma_desc->ui_desp_88_constant_padder = 0;
    pst_edma_desc->ui_desp_88_converter = 0;

    // 0x8C: APU_EDMA3_DESP_8C
    pst_edma_desc->ui_desp_8c_line_packer = 0;
    pst_edma_desc->ui_desp_8c_up_sampler = 0;
    pst_edma_desc->ui_desp_8c_down_sampler = 0;
    pst_edma_desc->ui_desp_8c_reserved_0 = 0;

    // 0x90: APU_EDMA3_DESP_90
    pst_edma_desc->ui_desp_90_ar_sender_0 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_1 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_2 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_3 = 1;
    pst_edma_desc->ui_desp_90_r_receiver_0 = 15;
    pst_edma_desc->ui_desp_90_r_receiver_1 = 15;
    pst_edma_desc->ui_desp_90_r_receiver_2 = 15;
    pst_edma_desc->ui_desp_90_r_receiver_3 = 15;

    // 0x94: APU_EDMA3_DESP_94
    pst_edma_desc->ui_desp_94_aw_sender_0 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_1 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_2 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_3 = 2;
    pst_edma_desc->ui_desp_94_w_sender_0 = 15;
    pst_edma_desc->ui_desp_94_w_sender_1 = 15;
    pst_edma_desc->ui_desp_94_w_sender_2 = 15;
    pst_edma_desc->ui_desp_94_w_sender_3 = 15;

    // 0x98: APU_EDMA3_DESP_98
    pst_edma_desc->ui_desp_98_reader = pipe_enable_num;
    pst_edma_desc->ui_desp_98_writer = pipe_enable_num;
    pst_edma_desc->ui_desp_98_encoder = 0;
    pst_edma_desc->ui_desp_98_decoder = 0;
    pst_edma_desc->ui_desp_98_act_encoder = 0;
    pst_edma_desc->ui_desp_98_act_decoder = 0;
    pst_edma_desc->ui_desp_98_pipe_enable = pipe_enable;

    pst_edma_desc->ui_desp_9c_src_addr_2 = tile_x_size + tile_x_size_out*2;
    EDMA_LOG_DEBUG("SRC 2 addr: 0x%08x\n", pst_edma_desc->ui_desp_9c_src_addr_2);
    pst_edma_desc->ui_desp_a0_dst_addr_2 = pst_edma_desc_info->ui_dst_addr_2;
    EDMA_LOG_DEBUG("DST 2 addr: 0x%08x\n", pst_edma_desc->ui_desp_a0_dst_addr_2);

    pst_edma_desc->ui_desp_b4_src_x_size_2 = pst_edma_desc_info->ui_dst_size_x_2;
    pst_edma_desc->ui_desp_b8_src_y_size_2 = pst_edma_desc_info->ui_dst_size_y_2;
    pst_edma_desc->ui_desp_bc_src_z_size_2 = 1;
    pst_edma_desc->ui_desp_a4_src_x_stride_2 = 2*tile_x_size;
    pst_edma_desc->ui_desp_ac_src_y_stride_2 = 1;

    pst_edma_desc->ui_desp_b4_dst_x_size_2 = pst_edma_desc_info->ui_dst_size_x_2;
    pst_edma_desc->ui_desp_b8_dst_y_size_2 = pst_edma_desc_info->ui_dst_size_y_2;
    pst_edma_desc->ui_desp_bc_dst_z_size_2 = 1;
    pst_edma_desc->ui_desp_a8_dst_x_stride_2 = pst_edma_desc_info->ui_dst_pitch_x_2;
    pst_edma_desc->ui_desp_b0_dst_y_stride_2 = pst_edma_desc_info->ui_dst_pitch_y_2;

    // 0xC0: APU_EDMA3_DESP_C0
    pst_edma_desc->ui_desp_c0_reserved_0 = 0;

    // 0xC4: APU_EDMA3_DESP_C4
    pst_edma_desc->ui_desp_c4_reserved_0 = 0;

    // 0xC8: APU_EDMA3_DESP_C8
    pst_edma_desc->ui_desp_c8_reserved_0 = 0;

    // 0xCC: APU_EDMA3_DESP_CC
    pst_edma_desc->ui_desp_cc_reserved_0 = 0;

    // 0xD0: APU_EDMA3_DESP_D0
    pst_edma_desc->ui_desp_d0_reserved_0 = 0;

    // 0xD4: APU_EDMA3_DESP_D4
    pst_edma_desc->ui_desp_d4_reserved_0 = 0;

    // 0xD8: APU_EDMA3_DESP_D8
    pst_edma_desc->ui_desp_d8_reserved_0 = 0;

    pst_edma_desc->ui_desp_dc_src_addr_3 = tile_x_size + tile_x_size_out*3;
    EDMA_LOG_DEBUG("SRC 3 addr: 0x%08x\n", pst_edma_desc->ui_desp_dc_src_addr_3);
    pst_edma_desc->ui_desp_e0_dst_addr_3 = pst_edma_desc_info->ui_dst_addr_3;
    EDMA_LOG_DEBUG("DST 3 addr: 0x%08x\n", pst_edma_desc->ui_desp_e0_dst_addr_3);

    pst_edma_desc->ui_desp_f4_src_x_size_3 = pst_edma_desc_info->ui_dst_size_x_3;
    pst_edma_desc->ui_desp_f8_src_y_size_3 = pst_edma_desc_info->ui_dst_size_y_3;
    pst_edma_desc->ui_desp_fc_src_z_size_3 = 1;
    pst_edma_desc->ui_desp_e4_src_x_stride_3 = 2*tile_x_size;
    pst_edma_desc->ui_desp_ec_src_y_stride_3 = 1;

    pst_edma_desc->ui_desp_f4_dst_x_size_3 = pst_edma_desc_info->ui_dst_size_x_3;
    pst_edma_desc->ui_desp_f8_dst_y_size_3 = pst_edma_desc_info->ui_dst_size_y_3;
    pst_edma_desc->ui_desp_fc_dst_z_size_3 = 1;
    pst_edma_desc->ui_desp_e8_dst_x_stride_3 = pst_edma_desc_info->ui_dst_pitch_x_3;
    pst_edma_desc->ui_desp_f0_dst_y_stride_3 = pst_edma_desc_info->ui_dst_pitch_y_3;

}

void fillDescMergeNIn(st_edmaDescInfoMergeN *pst_edma_desc_info, st_edmaDescType15 *pst_edma_desc, int *pi_errorCode=NULL)
{
    unsigned int tile_x_size = 1, tile_y_size = 1, tile_x_size_out = 1;
    unsigned int chnl_size = 0;
    unsigned int pxl_num = 0;
    unsigned char pixel_size = pst_edma_desc_info->pixel_size;
    unsigned char fmt = pst_edma_desc_info->uc_fmt;
    unsigned char pipe_enable = 0x0;
    unsigned char ch_num = 0x0;

    switch (fmt) {
    case EDMA_FMT_MERGE_2:
        tile_x_size = MAX_EDMA_MERGE2_CH_SIZE_X;
        tile_y_size = MAX_EDMA_MERGE2_SIZE_Y;
        tile_x_size_out = MAX_EDMA_MERGE2_PXL_SIZE_X;
        pipe_enable = 0x33;
        ch_num = 2;
        break;
    case EDMA_FMT_MERGE_3:
        tile_x_size = MAX_EDMA_MERGE3_CH_SIZE_X;
        tile_y_size = MAX_EDMA_MERGE3_SIZE_Y;
        tile_x_size_out = MAX_EDMA_MERGE3_PXL_SIZE_X;
        pipe_enable = 0x77;
        ch_num = 3;
        break;
    case EDMA_FMT_MERGE_4:
        tile_x_size = MAX_EDMA_MERGE4_CH_SIZE_X;
        tile_y_size = MAX_EDMA_MERGE4_SIZE_Y;
        tile_x_size_out = MAX_EDMA_MERGE4_PXL_SIZE_X;
        pipe_enable = 0xFF;
        ch_num = 4;
        break;
    default:
        tile_x_size = 1;
        tile_y_size = 1;
        tile_x_size_out = 1;
        ch_num = 1;
        break;
    }

    chnl_size = pixel_size;
    pxl_num  = pst_edma_desc_info->ui_dst_size_x_0/chnl_size;
    EDMA_LOG_DEBUG("pxl  size: %d\n", pixel_size);
    EDMA_LOG_DEBUG("chnl size: %d\n", chnl_size);
    EDMA_LOG_DEBUG("pxl  num : %d\n", pxl_num);

    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = 15;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_handshake_enable = 0; // TBD
    pst_edma_desc->ui_desp_00_reserved_0 = 0;

    // 0x04: APU_EDMA3_DESP_04
    //checkFormat(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_format = 0;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    //checkRawShift(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 1;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 0;
    pst_edma_desc->ui_desp_04_write_tmp_buf = 1; // Write data to internal buffer.
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0;
    pst_edma_desc->ui_desp_04_reserved_1 = 0;
    pst_edma_desc->ui_desp_04_pad_const = 0;

    // 0x08: APU_EDMA3_DESP_08
    pst_edma_desc->ui_desp_08_in_swap_mat = 0x8421;
    pst_edma_desc->ui_desp_08_out_swap_mat = 0x8421;

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_ufbc_tile_width = 0;
    pst_edma_desc->ui_desp_0c_reserved_0 = 0;
    pst_edma_desc->ui_desp_0c_ufbc_tile_height = 0;
    pst_edma_desc->ui_desp_0c_reserved_1 = 0;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_ufbc_frame_width = 0;
    pst_edma_desc->ui_desp_10_reserved_0 = 0;
    pst_edma_desc->ui_desp_10_ufbc_frame_height = 0;
    pst_edma_desc->ui_desp_10_reserved_1 = 0;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_ufbc_x_offset = 0;
    pst_edma_desc->ui_desp_14_reserved_0 = 0;
    pst_edma_desc->ui_desp_14_ufbc_y_offset = 0;
    pst_edma_desc->ui_desp_14_reserved_1 = 0;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_ufbc_payload_offset = 0;

    // 0x1C: APU_EDMA3_DESP_1C
    pst_edma_desc->ui_desp_1c_src_addr_0 = pst_edma_desc_info->ui_src_addr_0;
    EDMA_LOG_DEBUG("SRC 0 addr: 0x%08x\n", pst_edma_desc->ui_desp_1c_src_addr_0);
    pst_edma_desc->ui_desp_20_dst_addr_0 = tile_x_size*0;
    EDMA_LOG_DEBUG("DST 0 addr: 0x%08x\n", pst_edma_desc->ui_desp_20_dst_addr_0);

    pst_edma_desc->ui_desp_34_src_x_size_0 = pst_edma_desc_info->ui_src_size_x_0;
    pst_edma_desc->ui_desp_38_src_y_size_0 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_3c_src_z_size_0 = 1;
    pst_edma_desc->ui_desp_24_src_x_stride_0 = pst_edma_desc_info->ui_src_pitch_x_0;
    pst_edma_desc->ui_desp_2c_src_y_stride_0 = pst_edma_desc_info->ui_src_pitch_y_0;

    EDMA_LOG_DEBUG("SRC 0 size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_src_x_size_0, pst_edma_desc->ui_desp_38_src_y_size_0, pst_edma_desc->ui_desp_3c_src_z_size_0);
    EDMA_LOG_DEBUG("    stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_24_src_x_stride_0, pst_edma_desc->ui_desp_2c_src_y_stride_0);
    pst_edma_desc->ui_desp_34_dst_x_size_0 = pst_edma_desc_info->ui_src_size_x_0;
    pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_3c_dst_z_size_0 = 1;
    pst_edma_desc->ui_desp_28_dst_x_stride_0 = 2*tile_x_size_out;
    pst_edma_desc->ui_desp_30_dst_y_stride_0 = 1;

    EDMA_LOG_DEBUG("DST 0 size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_dst_x_size_0, pst_edma_desc->ui_desp_38_dst_y_size_0, pst_edma_desc->ui_desp_3c_dst_z_size_0);
    EDMA_LOG_DEBUG("    stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_28_dst_x_stride_0, pst_edma_desc->ui_desp_30_dst_y_stride_0);

    // 0x40: APU_EDMA3_DESP_40
    pst_edma_desc->ui_desp_40_ufbc_alg_sel = 0;
    pst_edma_desc->ui_desp_40_ufbc_yuv_transform = 0;
    pst_edma_desc->ui_desp_40_ufbc_align_payload_offset = 0;
    pst_edma_desc->ui_desp_40_ufbc_dummy_write = 0;
    pst_edma_desc->ui_desp_40_ufbc_secure_mode = 0;
    pst_edma_desc->ui_desp_40_reserved_0 = 0;
    pst_edma_desc->ui_desp_40_act_msb_flip = 0;
    pst_edma_desc->ui_desp_40_act_dec_asm_left = 0;
    pst_edma_desc->ui_desp_40_reserved_1 = 0;
    pst_edma_desc->ui_desp_40_down_sampling_drop = 0;
    pst_edma_desc->ui_desp_40_down_sampling_filter = 0;
    pst_edma_desc->ui_desp_40_reserved_2 = 0;

    // 0x44: APU_EDMA3_DESP_44
    pst_edma_desc->ui_desp_44_r2y_y2r_mat_00 = 0;
    pst_edma_desc->ui_desp_44_r2y_y2r_mat_01 = 0;

    // 0x48: APU_EDMA3_DESP_48
    pst_edma_desc->ui_desp_48_r2y_y2r_mat_02 = 0;
    pst_edma_desc->ui_desp_48_r2y_y2r_vec_0 = 0;

    // 0x4C: APU_EDMA3_DESP_4C
    pst_edma_desc->ui_desp_4c_r2y_y2r_mat_10 = 0;
    pst_edma_desc->ui_desp_4c_r2y_y2r_mat_11 = 0;

    // 0x50: APU_EDMA3_DESP_50
    pst_edma_desc->ui_desp_50_r2y_y2r_mat_12 = 0;
    pst_edma_desc->ui_desp_50_r2y_y2r_vec_1 = 0;

    // 0x54: APU_EDMA3_DESP_54
    pst_edma_desc->ui_desp_54_r2y_y2r_mat_20 = 0;
    pst_edma_desc->ui_desp_54_r2y_y2r_mat_21 = 0;

    // 0x58: APU_EDMA3_DESP_58
    pst_edma_desc->ui_desp_58_r2y_y2r_mat_22 = 0;
    pst_edma_desc->ui_desp_58_r2y_y2r_vec_2 = 0;

    pst_edma_desc->ui_desp_5c_src_addr_1 = pst_edma_desc_info->ui_src_addr_1;
    EDMA_LOG_DEBUG("SRC 1 addr: 0x%08x\n", pst_edma_desc->ui_desp_5c_src_addr_1);
    pst_edma_desc->ui_desp_60_dst_addr_1 = tile_x_size*1;
    EDMA_LOG_DEBUG("DST 1 addr: 0x%08x\n", pst_edma_desc->ui_desp_60_dst_addr_1);

    pst_edma_desc->ui_desp_74_src_x_size_1 = pst_edma_desc_info->ui_src_size_x_1;
    pst_edma_desc->ui_desp_78_src_y_size_1 = pst_edma_desc_info->ui_src_size_y_1;
    pst_edma_desc->ui_desp_7c_src_z_size_1 = 1;
    pst_edma_desc->ui_desp_64_src_x_stride_1 = pst_edma_desc_info->ui_src_pitch_x_1;
    pst_edma_desc->ui_desp_6c_src_y_stride_1 = pst_edma_desc_info->ui_src_pitch_y_1;

    pst_edma_desc->ui_desp_74_dst_x_size_1 = pst_edma_desc_info->ui_src_size_x_1;
    pst_edma_desc->ui_desp_78_dst_y_size_1 = pst_edma_desc_info->ui_src_size_y_1;
    pst_edma_desc->ui_desp_7c_dst_z_size_1 = 1;
    pst_edma_desc->ui_desp_68_dst_x_stride_1 = 2*tile_x_size_out;
    pst_edma_desc->ui_desp_70_dst_y_stride_1 = 1;

    // 0x80: APU_EDMA3_DESP_80
    pst_edma_desc->ui_desp_80_in_extractor_0 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_1 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_2 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_3 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_0 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_1 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_2 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_3 = 0;

    // 0x84: APU_EDMA3_DESP_84
    pst_edma_desc->ui_desp_84_in_zero_padder_0 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_1 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_2 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_3 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_0 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_1 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_2 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_3 = 0;

    // 0x88: APU_EDMA3_DESP_88
    pst_edma_desc->ui_desp_88_router = 1;
    pst_edma_desc->ui_desp_88_merger = 0;
    pst_edma_desc->ui_desp_88_splitter = 0;
    pst_edma_desc->ui_desp_88_in_swapper = 0;
    pst_edma_desc->ui_desp_88_out_swapper = 0;
    pst_edma_desc->ui_desp_88_extractor = 0;
    pst_edma_desc->ui_desp_88_constant_padder = 0;
    pst_edma_desc->ui_desp_88_converter = 0;

    // 0x8C: APU_EDMA3_DESP_8C
    pst_edma_desc->ui_desp_8c_line_packer = 0;
    pst_edma_desc->ui_desp_8c_up_sampler = 0;
    pst_edma_desc->ui_desp_8c_down_sampler = 0;
    pst_edma_desc->ui_desp_8c_reserved_0 = 0;

    // 0x90: APU_EDMA3_DESP_90
    pst_edma_desc->ui_desp_90_ar_sender_0 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_1 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_2 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_3 = 1;
    pst_edma_desc->ui_desp_90_r_receiver_0 = 15;
    pst_edma_desc->ui_desp_90_r_receiver_1 = 15;
    pst_edma_desc->ui_desp_90_r_receiver_2 = 15;
    pst_edma_desc->ui_desp_90_r_receiver_3 = 15;

    // 0x94: APU_EDMA3_DESP_94
    pst_edma_desc->ui_desp_94_aw_sender_0 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_1 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_2 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_3 = 2;
    pst_edma_desc->ui_desp_94_w_sender_0 = 15;
    pst_edma_desc->ui_desp_94_w_sender_1 = 15;
    pst_edma_desc->ui_desp_94_w_sender_2 = 15;
    pst_edma_desc->ui_desp_94_w_sender_3 = 15;

    // 0x98: APU_EDMA3_DESP_98
    pst_edma_desc->ui_desp_98_reader = ch_num;
    pst_edma_desc->ui_desp_98_writer = ch_num;
    pst_edma_desc->ui_desp_98_encoder = 0;
    pst_edma_desc->ui_desp_98_decoder = 0;
    pst_edma_desc->ui_desp_98_act_encoder = 0;
    pst_edma_desc->ui_desp_98_act_decoder = 0;
    pst_edma_desc->ui_desp_98_pipe_enable = pipe_enable;

    pst_edma_desc->ui_desp_9c_src_addr_2 = pst_edma_desc_info->ui_src_addr_2;
    EDMA_LOG_DEBUG("SRC 2 addr: 0x%08x\n", pst_edma_desc->ui_desp_9c_src_addr_2);
    pst_edma_desc->ui_desp_a0_dst_addr_2 = tile_x_size*2;
    EDMA_LOG_DEBUG("DST 2 addr: 0x%08x\n", pst_edma_desc->ui_desp_a0_dst_addr_2);

    pst_edma_desc->ui_desp_b4_src_x_size_2 = pst_edma_desc_info->ui_src_size_x_2;
    pst_edma_desc->ui_desp_b8_src_y_size_2 = pst_edma_desc_info->ui_src_size_y_2;
    pst_edma_desc->ui_desp_bc_src_z_size_2 = 1;
    pst_edma_desc->ui_desp_a4_src_x_stride_2 = pst_edma_desc_info->ui_src_pitch_x_2;
    pst_edma_desc->ui_desp_ac_src_y_stride_2 = pst_edma_desc_info->ui_src_pitch_x_2;

    pst_edma_desc->ui_desp_b4_dst_x_size_2 = pst_edma_desc_info->ui_src_size_x_2;
    pst_edma_desc->ui_desp_b8_dst_y_size_2 = pst_edma_desc_info->ui_src_size_y_2;
    pst_edma_desc->ui_desp_bc_dst_z_size_2 = 1;
    pst_edma_desc->ui_desp_a8_dst_x_stride_2 = 2*tile_x_size_out;
    pst_edma_desc->ui_desp_b0_dst_y_stride_2 = 1;

    // 0xC0: APU_EDMA3_DESP_C0
    pst_edma_desc->ui_desp_c0_reserved_0 = 0;

    // 0xC4: APU_EDMA3_DESP_C4
    pst_edma_desc->ui_desp_c4_reserved_0 = 0;

    // 0xC8: APU_EDMA3_DESP_C8
    pst_edma_desc->ui_desp_c8_reserved_0 = 0;

    // 0xCC: APU_EDMA3_DESP_CC
    pst_edma_desc->ui_desp_cc_reserved_0 = 0;

    // 0xD0: APU_EDMA3_DESP_D0
    pst_edma_desc->ui_desp_d0_reserved_0 = 0;

    // 0xD4: APU_EDMA3_DESP_D4
    pst_edma_desc->ui_desp_d4_reserved_0 = 0;

    // 0xD8: APU_EDMA3_DESP_D8
    pst_edma_desc->ui_desp_d8_reserved_0 = 0;

    pst_edma_desc->ui_desp_dc_src_addr_3 = pst_edma_desc_info->ui_src_addr_3;
    EDMA_LOG_DEBUG("SRC 3 addr: 0x%08x\n", pst_edma_desc->ui_desp_dc_src_addr_3);
    pst_edma_desc->ui_desp_e0_dst_addr_3 = tile_x_size*3;
    EDMA_LOG_DEBUG("DST 3 addr: 0x%08x\n", pst_edma_desc->ui_desp_e0_dst_addr_3);

    pst_edma_desc->ui_desp_f4_src_x_size_3 = pst_edma_desc_info->ui_src_size_x_3;
    pst_edma_desc->ui_desp_f8_src_y_size_3 = pst_edma_desc_info->ui_src_size_y_3;
    pst_edma_desc->ui_desp_fc_src_z_size_3 = 1;
    pst_edma_desc->ui_desp_e4_src_x_stride_3 = pst_edma_desc_info->ui_src_pitch_x_3;
    pst_edma_desc->ui_desp_ec_src_y_stride_3 = pst_edma_desc_info->ui_src_pitch_y_3;

    pst_edma_desc->ui_desp_f4_dst_x_size_3 = pst_edma_desc_info->ui_src_size_x_3;
    pst_edma_desc->ui_desp_f8_dst_y_size_3 = pst_edma_desc_info->ui_src_size_y_3;
    pst_edma_desc->ui_desp_fc_dst_z_size_3 = 1;
    pst_edma_desc->ui_desp_e8_dst_x_stride_3 = 2*tile_x_size_out;
    pst_edma_desc->ui_desp_f0_dst_y_stride_3 = 1;

}

void fillDescMergeN(st_edmaDescInfoMergeN *pst_edma_desc_info, st_edmaDescType15 *pst_edma_desc, int *pi_errorCode=NULL)
{
    unsigned int tile_x_size = 1, tile_y_size = 1, tile_x_size_out = 1;
    unsigned int chnl_size = 0;
    unsigned int pxl_num = 0;
    unsigned char pixel_size = pst_edma_desc_info->pixel_size;
    unsigned char pixel_size_out = 0;
    unsigned char fmt = pst_edma_desc_info->uc_fmt;
    unsigned char pipe_enable = 0x0;
    unsigned char ch_num = 0x0;

    switch (fmt) {
    case EDMA_FMT_MERGE_2:
        tile_x_size = MAX_EDMA_MERGE2_CH_SIZE_X;
        tile_y_size = MAX_EDMA_MERGE2_SIZE_Y;
        tile_x_size_out = MAX_EDMA_MERGE2_PXL_SIZE_X;
        pipe_enable = 0x33;
        ch_num = 2;
        break;
    case EDMA_FMT_MERGE_3:
        tile_x_size = MAX_EDMA_MERGE3_CH_SIZE_X;
        tile_y_size = MAX_EDMA_MERGE3_SIZE_Y;
        tile_x_size_out = MAX_EDMA_MERGE3_PXL_SIZE_X;
        pipe_enable = 0x77;
        ch_num = 3;
        break;
    case EDMA_FMT_MERGE_4:
        tile_x_size = MAX_EDMA_MERGE4_CH_SIZE_X;
        tile_y_size = MAX_EDMA_MERGE4_SIZE_Y;
        tile_x_size_out = MAX_EDMA_MERGE4_PXL_SIZE_X;
        pipe_enable = 0xFF;
        ch_num = 4;
        break;
    default:
        tile_x_size = 1;
        tile_y_size = 1;
        tile_x_size_out = 1;
        ch_num = 1;
        break;
    }

    chnl_size = pixel_size;
    pixel_size_out = chnl_size*ch_num;
    pxl_num  = pst_edma_desc_info->ui_src_size_x_0/pixel_size;
    EDMA_LOG_DEBUG("pxl  size: %d\n", pixel_size);
    EDMA_LOG_DEBUG("chnl size: %d\n", chnl_size);
    EDMA_LOG_DEBUG("pxl  num : %d\n", pxl_num);

    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = 15;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_handshake_enable = 0; // TBD
    pst_edma_desc->ui_desp_00_reserved_0 = 0;

    // 0x04: APU_EDMA3_DESP_04
    //checkFormat(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_format = 0;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    //checkRawShift(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 1;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 1;
    pst_edma_desc->ui_desp_04_write_tmp_buf = 1; // Write data to internal buffer.
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0;
    pst_edma_desc->ui_desp_04_reserved_1 = 0;
    pst_edma_desc->ui_desp_04_pad_const = 0;

    // 0x08: APU_EDMA3_DESP_08
    pst_edma_desc->ui_desp_08_in_swap_mat = 0x8421;
    pst_edma_desc->ui_desp_08_out_swap_mat = 0x8421;

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_ufbc_tile_width = 0;
    pst_edma_desc->ui_desp_0c_reserved_0 = 0;
    pst_edma_desc->ui_desp_0c_ufbc_tile_height = 0;
    pst_edma_desc->ui_desp_0c_reserved_1 = 0;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_ufbc_frame_width = 0;
    pst_edma_desc->ui_desp_10_reserved_0 = 0;
    pst_edma_desc->ui_desp_10_ufbc_frame_height = 0;
    pst_edma_desc->ui_desp_10_reserved_1 = 0;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_ufbc_x_offset = 0;
    pst_edma_desc->ui_desp_14_reserved_0 = 0;
    pst_edma_desc->ui_desp_14_ufbc_y_offset = 0;
    pst_edma_desc->ui_desp_14_reserved_1 = 0;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_ufbc_payload_offset = 0;

    // 0x1C: APU_EDMA3_DESP_1C
    pst_edma_desc->ui_desp_1c_src_addr_0 = tile_x_size*0;
    EDMA_LOG_DEBUG("SRC 0 addr: 0x%08x\n", pst_edma_desc->ui_desp_1c_src_addr_0);
    pst_edma_desc->ui_desp_20_dst_addr_0 = tile_x_size*ch_num + chnl_size*0;
    EDMA_LOG_DEBUG("DST 0 addr: 0x%08x\n", pst_edma_desc->ui_desp_20_dst_addr_0);

    pst_edma_desc->ui_desp_34_src_x_size_0 = chnl_size;
    pst_edma_desc->ui_desp_38_src_y_size_0 = pxl_num;
    pst_edma_desc->ui_desp_3c_src_z_size_0 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_24_src_x_stride_0 = pixel_size;
    pst_edma_desc->ui_desp_2c_src_y_stride_0 = 2*tile_x_size_out;

    EDMA_LOG_DEBUG("SRC 0 size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_src_x_size_0, pst_edma_desc->ui_desp_38_src_y_size_0, pst_edma_desc->ui_desp_3c_src_z_size_0);
    EDMA_LOG_DEBUG("    stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_24_src_x_stride_0, pst_edma_desc->ui_desp_2c_src_y_stride_0);
    pst_edma_desc->ui_desp_34_dst_x_size_0 = chnl_size;
    pst_edma_desc->ui_desp_38_dst_y_size_0 = pxl_num;
    pst_edma_desc->ui_desp_3c_dst_z_size_0 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_28_dst_x_stride_0 = pixel_size_out;
    pst_edma_desc->ui_desp_30_dst_y_stride_0 = 2*tile_x_size_out;

    EDMA_LOG_DEBUG("DST 0 size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_dst_x_size_0, pst_edma_desc->ui_desp_38_dst_y_size_0, pst_edma_desc->ui_desp_3c_dst_z_size_0);
    EDMA_LOG_DEBUG("    stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_28_dst_x_stride_0, pst_edma_desc->ui_desp_30_dst_y_stride_0);

    // 0x40: APU_EDMA3_DESP_40
    pst_edma_desc->ui_desp_40_ufbc_alg_sel = 0;
    pst_edma_desc->ui_desp_40_ufbc_yuv_transform = 0;
    pst_edma_desc->ui_desp_40_ufbc_align_payload_offset = 0;
    pst_edma_desc->ui_desp_40_ufbc_dummy_write = 0;
    pst_edma_desc->ui_desp_40_ufbc_secure_mode = 0;
    pst_edma_desc->ui_desp_40_reserved_0 = 0;
    pst_edma_desc->ui_desp_40_act_msb_flip = 0;
    pst_edma_desc->ui_desp_40_act_dec_asm_left = 0;
    pst_edma_desc->ui_desp_40_reserved_1 = 0;
    pst_edma_desc->ui_desp_40_down_sampling_drop = 0;
    pst_edma_desc->ui_desp_40_down_sampling_filter = 0;
    pst_edma_desc->ui_desp_40_reserved_2 = 0;

    // 0x44: APU_EDMA3_DESP_44
    pst_edma_desc->ui_desp_44_r2y_y2r_mat_00 = 0;
    pst_edma_desc->ui_desp_44_r2y_y2r_mat_01 = 0;

    // 0x48: APU_EDMA3_DESP_48
    pst_edma_desc->ui_desp_48_r2y_y2r_mat_02 = 0;
    pst_edma_desc->ui_desp_48_r2y_y2r_vec_0 = 0;

    // 0x4C: APU_EDMA3_DESP_4C
    pst_edma_desc->ui_desp_4c_r2y_y2r_mat_10 = 0;
    pst_edma_desc->ui_desp_4c_r2y_y2r_mat_11 = 0;

    // 0x50: APU_EDMA3_DESP_50
    pst_edma_desc->ui_desp_50_r2y_y2r_mat_12 = 0;
    pst_edma_desc->ui_desp_50_r2y_y2r_vec_1 = 0;

    // 0x54: APU_EDMA3_DESP_54
    pst_edma_desc->ui_desp_54_r2y_y2r_mat_20 = 0;
    pst_edma_desc->ui_desp_54_r2y_y2r_mat_21 = 0;

    // 0x58: APU_EDMA3_DESP_58
    pst_edma_desc->ui_desp_58_r2y_y2r_mat_22 = 0;
    pst_edma_desc->ui_desp_58_r2y_y2r_vec_2 = 0;

    pst_edma_desc->ui_desp_5c_src_addr_1 = tile_x_size*1;
    EDMA_LOG_DEBUG("SRC 1 addr: 0x%08x\n", pst_edma_desc->ui_desp_5c_src_addr_1);
    pst_edma_desc->ui_desp_60_dst_addr_1 = tile_x_size*ch_num + chnl_size*1;
    EDMA_LOG_DEBUG("DST 1 addr: 0x%08x\n", pst_edma_desc->ui_desp_60_dst_addr_1);

    pst_edma_desc->ui_desp_74_src_x_size_1 = chnl_size;
    pst_edma_desc->ui_desp_78_src_y_size_1 = pxl_num;
    pst_edma_desc->ui_desp_7c_src_z_size_1 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_64_src_x_stride_1 = pixel_size;
    pst_edma_desc->ui_desp_6c_src_y_stride_1 = 2*tile_x_size_out;

    pst_edma_desc->ui_desp_74_dst_x_size_1 = chnl_size;
    pst_edma_desc->ui_desp_78_dst_y_size_1 = pxl_num;
    pst_edma_desc->ui_desp_7c_dst_z_size_1 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_68_dst_x_stride_1 = pixel_size_out;
    pst_edma_desc->ui_desp_70_dst_y_stride_1 = 2*tile_x_size_out;

    // 0x80: APU_EDMA3_DESP_80
    pst_edma_desc->ui_desp_80_in_extractor_0 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_1 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_2 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_3 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_0 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_1 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_2 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_3 = 0;

    // 0x84: APU_EDMA3_DESP_84
    pst_edma_desc->ui_desp_84_in_zero_padder_0 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_1 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_2 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_3 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_0 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_1 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_2 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_3 = 0;

    // 0x88: APU_EDMA3_DESP_88
    pst_edma_desc->ui_desp_88_router = 1;
    pst_edma_desc->ui_desp_88_merger = 0;
    pst_edma_desc->ui_desp_88_splitter = 0;
    pst_edma_desc->ui_desp_88_in_swapper = 0;
    pst_edma_desc->ui_desp_88_out_swapper = 0;
    pst_edma_desc->ui_desp_88_extractor = 0;
    pst_edma_desc->ui_desp_88_constant_padder = 0;
    pst_edma_desc->ui_desp_88_converter = 0;

    // 0x8C: APU_EDMA3_DESP_8C
    pst_edma_desc->ui_desp_8c_line_packer = 0;
    pst_edma_desc->ui_desp_8c_up_sampler = 0;
    pst_edma_desc->ui_desp_8c_down_sampler = 0;
    pst_edma_desc->ui_desp_8c_reserved_0 = 0;

    // 0x90: APU_EDMA3_DESP_90
    pst_edma_desc->ui_desp_90_ar_sender_0 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_1 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_2 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_3 = 1;
    pst_edma_desc->ui_desp_90_r_receiver_0 = chnl_size - 1;
    pst_edma_desc->ui_desp_90_r_receiver_1 = chnl_size - 1;
    pst_edma_desc->ui_desp_90_r_receiver_2 = chnl_size - 1;
    pst_edma_desc->ui_desp_90_r_receiver_3 = chnl_size - 1;

    // 0x94: APU_EDMA3_DESP_94
    pst_edma_desc->ui_desp_94_aw_sender_0 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_1 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_2 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_3 = 2;
    pst_edma_desc->ui_desp_94_w_sender_0 = chnl_size - 1;
    pst_edma_desc->ui_desp_94_w_sender_1 = chnl_size - 1;
    pst_edma_desc->ui_desp_94_w_sender_2 = chnl_size - 1;
    pst_edma_desc->ui_desp_94_w_sender_3 = chnl_size - 1;

    // 0x98: APU_EDMA3_DESP_98
    pst_edma_desc->ui_desp_98_reader = ch_num;
    pst_edma_desc->ui_desp_98_writer = ch_num;
    pst_edma_desc->ui_desp_98_encoder = 0;
    pst_edma_desc->ui_desp_98_decoder = 0;
    pst_edma_desc->ui_desp_98_act_encoder = 0;
    pst_edma_desc->ui_desp_98_act_decoder = 0;
    pst_edma_desc->ui_desp_98_pipe_enable = pipe_enable;

    pst_edma_desc->ui_desp_9c_src_addr_2 = tile_x_size*2;
    EDMA_LOG_DEBUG("SRC 2 addr: 0x%08x\n", pst_edma_desc->ui_desp_9c_src_addr_2);
    pst_edma_desc->ui_desp_a0_dst_addr_2 = tile_x_size*ch_num + chnl_size*2;
    EDMA_LOG_DEBUG("DST 2 addr: 0x%08x\n", pst_edma_desc->ui_desp_a0_dst_addr_2);

    pst_edma_desc->ui_desp_b4_src_x_size_2 = chnl_size;
    pst_edma_desc->ui_desp_b8_src_y_size_2 = pxl_num;
    pst_edma_desc->ui_desp_bc_src_z_size_2 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_a4_src_x_stride_2 = pixel_size;
    pst_edma_desc->ui_desp_ac_src_y_stride_2 = 2*tile_x_size_out;

    pst_edma_desc->ui_desp_b4_dst_x_size_2 = chnl_size;
    pst_edma_desc->ui_desp_b8_dst_y_size_2 = pxl_num;
    pst_edma_desc->ui_desp_bc_dst_z_size_2 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_a8_dst_x_stride_2 = pixel_size_out;
    pst_edma_desc->ui_desp_b0_dst_y_stride_2 = 2*tile_x_size_out;

    // 0xC0: APU_EDMA3_DESP_C0
    pst_edma_desc->ui_desp_c0_reserved_0 = 0;

    // 0xC4: APU_EDMA3_DESP_C4
    pst_edma_desc->ui_desp_c4_reserved_0 = 0;

    // 0xC8: APU_EDMA3_DESP_C8
    pst_edma_desc->ui_desp_c8_reserved_0 = 0;

    // 0xCC: APU_EDMA3_DESP_CC
    pst_edma_desc->ui_desp_cc_reserved_0 = 0;

    // 0xD0: APU_EDMA3_DESP_D0
    pst_edma_desc->ui_desp_d0_reserved_0 = 0;

    // 0xD4: APU_EDMA3_DESP_D4
    pst_edma_desc->ui_desp_d4_reserved_0 = 0;

    // 0xD8: APU_EDMA3_DESP_D8
    pst_edma_desc->ui_desp_d8_reserved_0 = 0;

    pst_edma_desc->ui_desp_dc_src_addr_3 = tile_x_size*3;
    EDMA_LOG_DEBUG("SRC 3 addr: 0x%08x\n", pst_edma_desc->ui_desp_dc_src_addr_3);
    pst_edma_desc->ui_desp_e0_dst_addr_3 = tile_x_size*ch_num + chnl_size*3;
    EDMA_LOG_DEBUG("DST 3 addr: 0x%08x\n", pst_edma_desc->ui_desp_e0_dst_addr_3);

    pst_edma_desc->ui_desp_f4_src_x_size_3 = chnl_size;
    pst_edma_desc->ui_desp_f8_src_y_size_3 = pxl_num;
    pst_edma_desc->ui_desp_fc_src_z_size_3 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_e4_src_x_stride_3 = pixel_size;
    pst_edma_desc->ui_desp_ec_src_y_stride_3 = 2*tile_x_size_out;

    pst_edma_desc->ui_desp_f4_dst_x_size_3 = chnl_size;
    pst_edma_desc->ui_desp_f8_dst_y_size_3 = pxl_num;
    pst_edma_desc->ui_desp_fc_dst_z_size_3 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_e8_dst_x_stride_3 = pixel_size_out;
    pst_edma_desc->ui_desp_f0_dst_y_stride_3 = 2*tile_x_size_out;
}

void fillDescMergeNOut(st_edmaDescInfoMergeN *pst_edma_desc_info, st_edmaDescType0 *pst_edma_desc, int *pi_errorCode=NULL)
{
    unsigned int tile_x_size = 1, tile_y_size = 1, tile_x_size_out = 1;
    unsigned int chnl_size = 0;
    unsigned char fmt = pst_edma_desc_info->uc_fmt;
    unsigned char pipe_enable = 0x0;
    unsigned char ch_num = 0x0;

    switch (fmt) {
    case EDMA_FMT_MERGE_2:
        tile_x_size = MAX_EDMA_MERGE2_CH_SIZE_X;
        tile_y_size = MAX_EDMA_MERGE2_SIZE_Y;
        tile_x_size_out = MAX_EDMA_MERGE2_PXL_SIZE_X;
        pipe_enable = 0x33;
        ch_num = 2;
        break;
    case EDMA_FMT_MERGE_3:
        tile_x_size = MAX_EDMA_MERGE3_CH_SIZE_X;
        tile_y_size = MAX_EDMA_MERGE3_SIZE_Y;
        tile_x_size_out = MAX_EDMA_MERGE3_PXL_SIZE_X;
        pipe_enable = 0x77;
        ch_num = 3;
        break;
    case EDMA_FMT_MERGE_4:
        tile_x_size = MAX_EDMA_MERGE4_CH_SIZE_X;
        tile_y_size = MAX_EDMA_MERGE4_SIZE_Y;
        tile_x_size_out = MAX_EDMA_MERGE4_PXL_SIZE_X;
        pipe_enable = 0xFF;
        ch_num = 4;
        break;
    default:
        tile_x_size = 1;
        tile_y_size = 1;
        tile_x_size_out = 1;
        chnl_size = 1;
        ch_num = 1;
        break;
    }
    EDMA_LOG_DEBUG("fmt: %d\n", fmt);

    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = 0;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_handshake_enable = 0; // TBD
    pst_edma_desc->ui_desp_00_reserved_0 = 0;

    // 0x04: APU_EDMA3_DESP_04
    pst_edma_desc->ui_desp_04_format = EDMA_FMT_NORMAL_MODE;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 0;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 1;
    pst_edma_desc->ui_desp_04_write_tmp_buf = 0;
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0;
    pst_edma_desc->ui_desp_04_reserved_1 = 0;
    pst_edma_desc->ui_desp_04_raw_shift = 0;
    pst_edma_desc->ui_desp_04_reserved_2 = 0;

    // 0x08: APU_EDMA3_DESP_08
    pst_edma_desc->ui_desp_08_fill_const = 0;

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_i2f_min = 0;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_i2f_scale = 0;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_f2i_min = 0;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_f2i_scale = 0;

    // 0x1C: APU_EDMA3_DESP_1C
    pst_edma_desc->ui_desp_1c_src_addr_0 = tile_x_size*ch_num;

    // 0x20: APU_EDMA3_DESP_20
    pst_edma_desc->ui_desp_20_dst_addr_0 = pst_edma_desc_info->ui_dst_addr_0;

    // 0x24: APU_EDMA3_DESP_24
    pst_edma_desc->ui_desp_24_src_x_stride_0 = 2*tile_x_size_out;

    // 0x28: APU_EDMA3_DESP_28
    pst_edma_desc->ui_desp_28_dst_x_stride_0 = pst_edma_desc_info->ui_dst_pitch_x_0;

    // 0x2C: APU_EDMA3_DESP_2C
    pst_edma_desc->ui_desp_2c_src_y_stride_0 = 1;

    // 0x30: APU_EDMA3_DESP_30
    pst_edma_desc->ui_desp_30_dst_y_stride_0 = pst_edma_desc_info->ui_dst_pitch_y_0;

    // 0x34: APU_EDMA3_DESP_34
    pst_edma_desc->ui_desp_34_src_x_size_0 = pst_edma_desc_info->ui_dst_size_x_0;
    pst_edma_desc->ui_desp_34_dst_x_size_0 = pst_edma_desc->ui_desp_34_src_x_size_0;

    // 0x38: APU_EDMA3_DESP_38
    pst_edma_desc->ui_desp_38_src_y_size_0 = pst_edma_desc_info->ui_dst_size_y_0;
    pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc->ui_desp_38_src_y_size_0;

    // 0x3c: APU_EDMA3_DESP_3C
    pst_edma_desc->ui_desp_3c_src_z_size_0 = pst_edma_desc_info->ui_dst_size_z_0; // 1
    pst_edma_desc->ui_desp_3c_dst_z_size_0 = pst_edma_desc->ui_desp_3c_src_z_size_0;
}

void fillDescUnpack(st_edmaDescInfoUnpack *pst_edma_desc_info, st_edmaDescType0 *pst_edma_desc, int *pi_errorCode=NULL)
{
    unsigned char fmt = pst_edma_desc_info->uc_fmt;
    unsigned char fmt_setting = 0x0;
    unsigned char pad_pos = pst_edma_desc_info->uc_pad_pos;
    unsigned char raw_shift = 0;

    switch (fmt) {
    case EDMA_FMT_UNPACK_8_TO_16:
        fmt_setting = EDMA_FMT_RAW_8_TO_RAW_16;
        if (pad_pos == EDMA_PAD_POS_LOW)
            raw_shift = 8;
        else
            raw_shift = 0;
        break;
    case EDMA_FMT_UNPACK_10_TO_16:
        fmt_setting = EDMA_FMT_RAW_10_TO_RAW_16;
        if (pad_pos == EDMA_PAD_POS_LOW)
            raw_shift = 6;
        else
            raw_shift = 0;
        break;
    case EDMA_FMT_UNPACK_12_TO_16:
        fmt_setting = EDMA_FMT_RAW_12_TO_RAW_16;
        if (pad_pos == EDMA_PAD_POS_LOW)
            raw_shift = 4;
        else
            raw_shift = 0;
        break;
    case EDMA_FMT_UNPACK_14_TO_16:
        fmt_setting = EDMA_FMT_RAW_14_TO_RAW_16;
        if (pad_pos == EDMA_PAD_POS_LOW)
            raw_shift = 2;
        else
            raw_shift = 0;
        break;
    default:
        fmt_setting = EDMA_FMT_RAW_8_TO_RAW_16;
        if (pad_pos == EDMA_PAD_POS_LOW)
            raw_shift = 8;
        else
            raw_shift = 0;
        break;
    }

    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = 0;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_handshake_enable = 0; // TBD
    pst_edma_desc->ui_desp_00_reserved_0 = 0;

    // 0x04: APU_EDMA3_DESP_04
    pst_edma_desc->ui_desp_04_format = fmt_setting;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 0;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 0;
    pst_edma_desc->ui_desp_04_write_tmp_buf = 0;
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0;
    pst_edma_desc->ui_desp_04_reserved_1 = 0;
    pst_edma_desc->ui_desp_04_raw_shift = raw_shift;
    pst_edma_desc->ui_desp_04_reserved_2 = 0;

    // 0x08: APU_EDMA3_DESP_08
    pst_edma_desc->ui_desp_08_fill_const = 0;

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_i2f_min = 0;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_i2f_scale = 0;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_f2i_min = 0;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_f2i_scale = 0;

    // 0x1C: APU_EDMA3_DESP_1C
    pst_edma_desc->ui_desp_1c_src_addr_0 = pst_edma_desc_info->ui_src_addr_0;

    // 0x20: APU_EDMA3_DESP_20
    pst_edma_desc->ui_desp_20_dst_addr_0 = pst_edma_desc_info->ui_dst_addr_0;

    // 0x24: APU_EDMA3_DESP_24
    pst_edma_desc->ui_desp_24_src_x_stride_0 = pst_edma_desc_info->ui_src_pitch_x_0;

    // 0x28: APU_EDMA3_DESP_28
    pst_edma_desc->ui_desp_28_dst_x_stride_0 = pst_edma_desc_info->ui_dst_pitch_x_0;

    // 0x2C: APU_EDMA3_DESP_2C
    pst_edma_desc->ui_desp_2c_src_y_stride_0 = pst_edma_desc_info->ui_src_pitch_y_0;

    // 0x30: APU_EDMA3_DESP_30
    pst_edma_desc->ui_desp_30_dst_y_stride_0 = pst_edma_desc_info->ui_dst_pitch_y_0;

    // 0x34: APU_EDMA3_DESP_34
    pst_edma_desc->ui_desp_34_src_x_size_0 = pst_edma_desc_info->ui_src_size_x_0;
    pst_edma_desc->ui_desp_34_dst_x_size_0 = pst_edma_desc_info->ui_dst_size_x_0;

    // 0x38: APU_EDMA3_DESP_38
    pst_edma_desc->ui_desp_38_src_y_size_0 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc_info->ui_dst_size_y_0;

    // 0x3c: APU_EDMA3_DESP_3C
    pst_edma_desc->ui_desp_3c_src_z_size_0 = pst_edma_desc_info->ui_src_size_z_0;
    pst_edma_desc->ui_desp_3c_dst_z_size_0 = pst_edma_desc_info->ui_dst_size_z_0;
}

void fillDescPack(st_edmaDescInfoPack *pst_edma_desc_info, st_edmaDescType15 *pst_edma_desc, int *pi_errorCode=NULL)
{
    unsigned char uc_fmt = pst_edma_desc_info->uc_fmt;
    unsigned char extract_pos = pst_edma_desc_info->uc_extract_pos;
    unsigned char extract_setting = 0;

    switch (uc_fmt) {
    case EDMA_FMT_PACK_16_TO_10:
        if (extract_pos == EDMA_EXTRACT_POS_LOW)
            extract_setting = 2;
        else
            extract_setting = 3;
        break;
    case EDMA_FMT_PACK_16_TO_12:
        if (extract_pos == EDMA_EXTRACT_POS_LOW)
            extract_setting = 4;
        else
            extract_setting = 5;
        break;
    default:
        break;
    }

    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = 15;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_handshake_enable = 0; // TBD
    pst_edma_desc->ui_desp_00_reserved_0 = 0;

    // 0x04: APU_EDMA3_DESP_04
    //checkFormat(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_format = 0;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    //checkRawShift(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 0;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 0;
    pst_edma_desc->ui_desp_04_write_tmp_buf = 0;
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0;
    pst_edma_desc->ui_desp_04_reserved_1 = 0;
    pst_edma_desc->ui_desp_04_pad_const = 0;

    // 0x08: APU_EDMA3_DESP_08
    pst_edma_desc->ui_desp_08_in_swap_mat = 0x8421;
    pst_edma_desc->ui_desp_08_out_swap_mat = 0x8421;

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_ufbc_tile_width = 0;
    pst_edma_desc->ui_desp_0c_reserved_0 = 0;
    pst_edma_desc->ui_desp_0c_ufbc_tile_height = 0;
    pst_edma_desc->ui_desp_0c_reserved_1 = 0;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_ufbc_frame_width = 0;
    pst_edma_desc->ui_desp_10_reserved_0 = 0;
    pst_edma_desc->ui_desp_10_ufbc_frame_height = 0;
    pst_edma_desc->ui_desp_10_reserved_1 = 0;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_ufbc_x_offset = 0;
    pst_edma_desc->ui_desp_14_reserved_0 = 0;
    pst_edma_desc->ui_desp_14_ufbc_y_offset = 0;
    pst_edma_desc->ui_desp_14_reserved_1 = 0;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_ufbc_payload_offset = 0;

    // 0x1C: APU_EDMA3_DESP_1C
    pst_edma_desc->ui_desp_1c_src_addr_0 = pst_edma_desc_info->ui_src_addr_0;
    EDMA_LOG_DEBUG("SRC 0 addr: 0x%08x\n", pst_edma_desc->ui_desp_1c_src_addr_0);
    pst_edma_desc->ui_desp_20_dst_addr_0 = pst_edma_desc_info->ui_dst_addr_0;
    EDMA_LOG_DEBUG("DST 0 addr: 0x%08x\n", pst_edma_desc->ui_desp_20_dst_addr_0);

    pst_edma_desc->ui_desp_34_src_x_size_0 = pst_edma_desc_info->ui_src_size_x_0;
    pst_edma_desc->ui_desp_38_src_y_size_0 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_3c_src_z_size_0 = pst_edma_desc_info->ui_src_size_z_0;
    pst_edma_desc->ui_desp_24_src_x_stride_0 = pst_edma_desc_info->ui_src_pitch_x_0;
    pst_edma_desc->ui_desp_2c_src_y_stride_0 = pst_edma_desc_info->ui_src_pitch_y_0;

    EDMA_LOG_DEBUG("SRC 0 size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_src_x_size_0, pst_edma_desc->ui_desp_38_src_y_size_0, pst_edma_desc->ui_desp_3c_src_z_size_0);
    EDMA_LOG_DEBUG("    stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_24_src_x_stride_0, pst_edma_desc->ui_desp_2c_src_y_stride_0);
    pst_edma_desc->ui_desp_34_dst_x_size_0 = pst_edma_desc_info->ui_dst_size_x_0;
    pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc_info->ui_dst_size_y_0;
    pst_edma_desc->ui_desp_3c_dst_z_size_0 = pst_edma_desc_info->ui_dst_size_z_0;
    pst_edma_desc->ui_desp_28_dst_x_stride_0 = pst_edma_desc_info->ui_dst_pitch_x_0;
    pst_edma_desc->ui_desp_30_dst_y_stride_0 = pst_edma_desc_info->ui_dst_pitch_y_0;

    EDMA_LOG_DEBUG("DST 0 size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_dst_x_size_0, pst_edma_desc->ui_desp_38_dst_y_size_0, pst_edma_desc->ui_desp_3c_dst_z_size_0);
    EDMA_LOG_DEBUG("    stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_28_dst_x_stride_0, pst_edma_desc->ui_desp_30_dst_y_stride_0);

    // 0x40: APU_EDMA3_DESP_40
    pst_edma_desc->ui_desp_40_ufbc_alg_sel = 0;
    pst_edma_desc->ui_desp_40_ufbc_yuv_transform = 0;
    pst_edma_desc->ui_desp_40_ufbc_align_payload_offset = 0;
    pst_edma_desc->ui_desp_40_ufbc_dummy_write = 0;
    pst_edma_desc->ui_desp_40_ufbc_secure_mode = 0;
    pst_edma_desc->ui_desp_40_reserved_0 = 0;
    pst_edma_desc->ui_desp_40_act_msb_flip = 0;
    pst_edma_desc->ui_desp_40_act_dec_asm_left = 0;
    pst_edma_desc->ui_desp_40_reserved_1 = 0;
    pst_edma_desc->ui_desp_40_down_sampling_drop = 0;
    pst_edma_desc->ui_desp_40_down_sampling_filter = 0;
    pst_edma_desc->ui_desp_40_reserved_2 = 0;

    // 0x44: APU_EDMA3_DESP_44
    pst_edma_desc->ui_desp_44_r2y_y2r_mat_00 = 0;
    pst_edma_desc->ui_desp_44_r2y_y2r_mat_01 = 0;

    // 0x48: APU_EDMA3_DESP_48
    pst_edma_desc->ui_desp_48_r2y_y2r_mat_02 = 0;
    pst_edma_desc->ui_desp_48_r2y_y2r_vec_0 = 0;

    // 0x4C: APU_EDMA3_DESP_4C
    pst_edma_desc->ui_desp_4c_r2y_y2r_mat_10 = 0;
    pst_edma_desc->ui_desp_4c_r2y_y2r_mat_11 = 0;

    // 0x50: APU_EDMA3_DESP_50
    pst_edma_desc->ui_desp_50_r2y_y2r_mat_12 = 0;
    pst_edma_desc->ui_desp_50_r2y_y2r_vec_1 = 0;

    // 0x54: APU_EDMA3_DESP_54
    pst_edma_desc->ui_desp_54_r2y_y2r_mat_20 = 0;
    pst_edma_desc->ui_desp_54_r2y_y2r_mat_21 = 0;

    // 0x58: APU_EDMA3_DESP_58
    pst_edma_desc->ui_desp_58_r2y_y2r_mat_22 = 0;
    pst_edma_desc->ui_desp_58_r2y_y2r_vec_2 = 0;

    pst_edma_desc->ui_desp_5c_src_addr_1 = 0;
    pst_edma_desc->ui_desp_60_dst_addr_1 = 0;

    pst_edma_desc->ui_desp_74_src_x_size_1 = 0;
    pst_edma_desc->ui_desp_78_src_y_size_1 = 0;
    pst_edma_desc->ui_desp_7c_src_z_size_1 = 0;
    pst_edma_desc->ui_desp_64_src_x_stride_1 = 0;
    pst_edma_desc->ui_desp_6c_src_y_stride_1 = 0;

    pst_edma_desc->ui_desp_74_dst_x_size_1 = 0;
    pst_edma_desc->ui_desp_78_dst_y_size_1 = 0;
    pst_edma_desc->ui_desp_7c_dst_z_size_1 = 0;
    pst_edma_desc->ui_desp_68_dst_x_stride_1 = 0;
    pst_edma_desc->ui_desp_70_dst_y_stride_1 = 0;

    // 0x80: APU_EDMA3_DESP_80
    pst_edma_desc->ui_desp_80_in_extractor_0 = extract_setting;
    pst_edma_desc->ui_desp_80_in_extractor_1 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_2 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_3 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_0 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_1 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_2 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_3 = 0;

    // 0x84: APU_EDMA3_DESP_84
    pst_edma_desc->ui_desp_84_in_zero_padder_0 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_1 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_2 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_3 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_0 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_1 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_2 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_3 = 0;

    // 0x88: APU_EDMA3_DESP_88
    pst_edma_desc->ui_desp_88_router = 0;
    pst_edma_desc->ui_desp_88_merger = 0;
    pst_edma_desc->ui_desp_88_splitter = 0;
    pst_edma_desc->ui_desp_88_in_swapper = 0;
    pst_edma_desc->ui_desp_88_out_swapper = 0;
    pst_edma_desc->ui_desp_88_extractor = 0;
    pst_edma_desc->ui_desp_88_constant_padder = 0;
    pst_edma_desc->ui_desp_88_converter = 0;

    // 0x8C: APU_EDMA3_DESP_8C
    pst_edma_desc->ui_desp_8c_line_packer = 0;
    pst_edma_desc->ui_desp_8c_up_sampler = 0;
    pst_edma_desc->ui_desp_8c_down_sampler = 0;
    pst_edma_desc->ui_desp_8c_reserved_0 = 0;

    // 0x90: APU_EDMA3_DESP_90
    pst_edma_desc->ui_desp_90_ar_sender_0 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_1 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_2 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_3 = 1;
    pst_edma_desc->ui_desp_90_r_receiver_0 = 15;
    pst_edma_desc->ui_desp_90_r_receiver_1 = 0;
    pst_edma_desc->ui_desp_90_r_receiver_2 = 0;
    pst_edma_desc->ui_desp_90_r_receiver_3 = 0;

    // 0x94: APU_EDMA3_DESP_94
    pst_edma_desc->ui_desp_94_aw_sender_0 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_1 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_2 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_3 = 2;
    pst_edma_desc->ui_desp_94_w_sender_0 = 15;
    pst_edma_desc->ui_desp_94_w_sender_1 = 0;
    pst_edma_desc->ui_desp_94_w_sender_2 = 0;
    pst_edma_desc->ui_desp_94_w_sender_3 = 0;

    // 0x98: APU_EDMA3_DESP_98
    pst_edma_desc->ui_desp_98_reader = 1;
    pst_edma_desc->ui_desp_98_writer = 1;
    pst_edma_desc->ui_desp_98_encoder = 0;
    pst_edma_desc->ui_desp_98_decoder = 0;
    pst_edma_desc->ui_desp_98_act_encoder = 0;
    pst_edma_desc->ui_desp_98_act_decoder = 0;
    pst_edma_desc->ui_desp_98_pipe_enable = 0x11;

    pst_edma_desc->ui_desp_9c_src_addr_2 = 0;
    pst_edma_desc->ui_desp_a0_dst_addr_2 = 0;

    pst_edma_desc->ui_desp_b4_src_x_size_2 = 0;
    pst_edma_desc->ui_desp_b8_src_y_size_2 = 0;
    pst_edma_desc->ui_desp_bc_src_z_size_2 = 0;
    pst_edma_desc->ui_desp_a4_src_x_stride_2 = 0;
    pst_edma_desc->ui_desp_ac_src_y_stride_2 = 0;

    pst_edma_desc->ui_desp_b4_dst_x_size_2 = 0;
    pst_edma_desc->ui_desp_b8_dst_y_size_2 = 0;
    pst_edma_desc->ui_desp_bc_dst_z_size_2 = 0;
    pst_edma_desc->ui_desp_a8_dst_x_stride_2 = 0;
    pst_edma_desc->ui_desp_b0_dst_y_stride_2 = 0;

    // 0xC0: APU_EDMA3_DESP_C0
    pst_edma_desc->ui_desp_c0_reserved_0 = 0;

    // 0xC4: APU_EDMA3_DESP_C4
    pst_edma_desc->ui_desp_c4_reserved_0 = 0;

    // 0xC8: APU_EDMA3_DESP_C8
    pst_edma_desc->ui_desp_c8_reserved_0 = 0;

    // 0xCC: APU_EDMA3_DESP_CC
    pst_edma_desc->ui_desp_cc_reserved_0 = 0;

    // 0xD0: APU_EDMA3_DESP_D0
    pst_edma_desc->ui_desp_d0_reserved_0 = 0;

    // 0xD4: APU_EDMA3_DESP_D4
    pst_edma_desc->ui_desp_d4_reserved_0 = 0;

    // 0xD8: APU_EDMA3_DESP_D8
    pst_edma_desc->ui_desp_d8_reserved_0 = 0;

    pst_edma_desc->ui_desp_dc_src_addr_3 = 0;
    pst_edma_desc->ui_desp_e0_dst_addr_3 = 0;

    pst_edma_desc->ui_desp_f4_src_x_size_3 = 0;
    pst_edma_desc->ui_desp_f8_src_y_size_3 = 0;
    pst_edma_desc->ui_desp_fc_src_z_size_3 = 0;
    pst_edma_desc->ui_desp_e4_src_x_stride_3 = 0;
    pst_edma_desc->ui_desp_ec_src_y_stride_3 = 0;

    pst_edma_desc->ui_desp_f4_dst_x_size_3 = 0;
    pst_edma_desc->ui_desp_f8_dst_y_size_3 = 0;
    pst_edma_desc->ui_desp_fc_dst_z_size_3 = 0;
    pst_edma_desc->ui_desp_e8_dst_x_stride_3 = 0;
    pst_edma_desc->ui_desp_f0_dst_y_stride_3 = 0;
}


void fillDescSwap2(st_edmaDescInfoSwap2 *pst_edma_desc_info, st_edmaDescType15 *pst_edma_desc, int *pi_errorCode=NULL)
{
    unsigned char swap_size = pst_edma_desc_info->swap_size;
    unsigned int swap_setting = 0;
    if (swap_size == 1)
        swap_setting = 3;
    else
        swap_setting = 5;

    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = 15;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_handshake_enable = 0; // TBD
    pst_edma_desc->ui_desp_00_reserved_0 = 0;

    // 0x04: APU_EDMA3_DESP_04
    //checkFormat(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_format = 0;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    //checkRawShift(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 0;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 0;
    pst_edma_desc->ui_desp_04_write_tmp_buf = 0;
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0;
    pst_edma_desc->ui_desp_04_reserved_1 = 0;
    pst_edma_desc->ui_desp_04_pad_const = 0;

    // 0x08: APU_EDMA3_DESP_08
    pst_edma_desc->ui_desp_08_in_swap_mat = 0x4812;
    pst_edma_desc->ui_desp_08_out_swap_mat = 0x8421;

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_ufbc_tile_width = 0;
    pst_edma_desc->ui_desp_0c_reserved_0 = 0;
    pst_edma_desc->ui_desp_0c_ufbc_tile_height = 0;
    pst_edma_desc->ui_desp_0c_reserved_1 = 0;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_ufbc_frame_width = 0;
    pst_edma_desc->ui_desp_10_reserved_0 = 0;
    pst_edma_desc->ui_desp_10_ufbc_frame_height = 0;
    pst_edma_desc->ui_desp_10_reserved_1 = 0;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_ufbc_x_offset = 0;
    pst_edma_desc->ui_desp_14_reserved_0 = 0;
    pst_edma_desc->ui_desp_14_ufbc_y_offset = 0;
    pst_edma_desc->ui_desp_14_reserved_1 = 0;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_ufbc_payload_offset = 0;

    // 0x1C: APU_EDMA3_DESP_1C
    pst_edma_desc->ui_desp_1c_src_addr_0 = pst_edma_desc_info->ui_src_addr_0;
    EDMA_LOG_DEBUG("SRC 0 addr: 0x%08x\n", pst_edma_desc->ui_desp_1c_src_addr_0);
    pst_edma_desc->ui_desp_20_dst_addr_0 = pst_edma_desc_info->ui_dst_addr_0;
    EDMA_LOG_DEBUG("DST 0 addr: 0x%08x\n", pst_edma_desc->ui_desp_20_dst_addr_0);

    pst_edma_desc->ui_desp_34_src_x_size_0 = pst_edma_desc_info->ui_src_size_x_0;
    pst_edma_desc->ui_desp_38_src_y_size_0 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_3c_src_z_size_0 = pst_edma_desc_info->ui_src_size_z_0;
    pst_edma_desc->ui_desp_24_src_x_stride_0 = pst_edma_desc_info->ui_src_pitch_x_0;
    pst_edma_desc->ui_desp_2c_src_y_stride_0 = pst_edma_desc_info->ui_src_pitch_y_0;

    EDMA_LOG_DEBUG("SRC 0 size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_src_x_size_0, pst_edma_desc->ui_desp_38_src_y_size_0, pst_edma_desc->ui_desp_3c_src_z_size_0);
    EDMA_LOG_DEBUG("    stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_24_src_x_stride_0, pst_edma_desc->ui_desp_2c_src_y_stride_0);
    pst_edma_desc->ui_desp_34_dst_x_size_0 = pst_edma_desc_info->ui_dst_size_x_0;
    pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc_info->ui_dst_size_y_0;
    pst_edma_desc->ui_desp_3c_dst_z_size_0 = pst_edma_desc_info->ui_dst_size_z_0;
    pst_edma_desc->ui_desp_28_dst_x_stride_0 = pst_edma_desc_info->ui_dst_pitch_x_0;
    pst_edma_desc->ui_desp_30_dst_y_stride_0 = pst_edma_desc_info->ui_dst_pitch_y_0;

    EDMA_LOG_DEBUG("DST 0 size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_dst_x_size_0, pst_edma_desc->ui_desp_38_dst_y_size_0, pst_edma_desc->ui_desp_3c_dst_z_size_0);
    EDMA_LOG_DEBUG("    stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_28_dst_x_stride_0, pst_edma_desc->ui_desp_30_dst_y_stride_0);

    // 0x40: APU_EDMA3_DESP_40
    pst_edma_desc->ui_desp_40_ufbc_alg_sel = 0;
    pst_edma_desc->ui_desp_40_ufbc_yuv_transform = 0;
    pst_edma_desc->ui_desp_40_ufbc_align_payload_offset = 0;
    pst_edma_desc->ui_desp_40_ufbc_dummy_write = 0;
    pst_edma_desc->ui_desp_40_ufbc_secure_mode = 0;
    pst_edma_desc->ui_desp_40_reserved_0 = 0;
    pst_edma_desc->ui_desp_40_act_msb_flip = 0;
    pst_edma_desc->ui_desp_40_act_dec_asm_left = 0;
    pst_edma_desc->ui_desp_40_reserved_1 = 0;
    pst_edma_desc->ui_desp_40_down_sampling_drop = 0;
    pst_edma_desc->ui_desp_40_down_sampling_filter = 0;
    pst_edma_desc->ui_desp_40_reserved_2 = 0;

    // 0x44: APU_EDMA3_DESP_44
    pst_edma_desc->ui_desp_44_r2y_y2r_mat_00 = 0;
    pst_edma_desc->ui_desp_44_r2y_y2r_mat_01 = 0;

    // 0x48: APU_EDMA3_DESP_48
    pst_edma_desc->ui_desp_48_r2y_y2r_mat_02 = 0;
    pst_edma_desc->ui_desp_48_r2y_y2r_vec_0 = 0;

    // 0x4C: APU_EDMA3_DESP_4C
    pst_edma_desc->ui_desp_4c_r2y_y2r_mat_10 = 0;
    pst_edma_desc->ui_desp_4c_r2y_y2r_mat_11 = 0;

    // 0x50: APU_EDMA3_DESP_50
    pst_edma_desc->ui_desp_50_r2y_y2r_mat_12 = 0;
    pst_edma_desc->ui_desp_50_r2y_y2r_vec_1 = 0;

    // 0x54: APU_EDMA3_DESP_54
    pst_edma_desc->ui_desp_54_r2y_y2r_mat_20 = 0;
    pst_edma_desc->ui_desp_54_r2y_y2r_mat_21 = 0;

    // 0x58: APU_EDMA3_DESP_58
    pst_edma_desc->ui_desp_58_r2y_y2r_mat_22 = 0;
    pst_edma_desc->ui_desp_58_r2y_y2r_vec_2 = 0;

    pst_edma_desc->ui_desp_5c_src_addr_1 = 0;
    pst_edma_desc->ui_desp_60_dst_addr_1 = 0;

    pst_edma_desc->ui_desp_74_src_x_size_1 = 0;
    pst_edma_desc->ui_desp_78_src_y_size_1 = 0;
    pst_edma_desc->ui_desp_7c_src_z_size_1 = 0;
    pst_edma_desc->ui_desp_64_src_x_stride_1 = 0;
    pst_edma_desc->ui_desp_6c_src_y_stride_1 = 0;

    pst_edma_desc->ui_desp_74_dst_x_size_1 = 0;
    pst_edma_desc->ui_desp_78_dst_y_size_1 = 0;
    pst_edma_desc->ui_desp_7c_dst_z_size_1 = 0;
    pst_edma_desc->ui_desp_68_dst_x_stride_1 = 0;
    pst_edma_desc->ui_desp_70_dst_y_stride_1 = 0;

    // 0x80: APU_EDMA3_DESP_80
    pst_edma_desc->ui_desp_80_in_extractor_0 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_1 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_2 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_3 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_0 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_1 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_2 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_3 = 0;

    // 0x84: APU_EDMA3_DESP_84
    pst_edma_desc->ui_desp_84_in_zero_padder_0 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_1 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_2 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_3 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_0 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_1 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_2 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_3 = 0;

    // 0x88: APU_EDMA3_DESP_88
    pst_edma_desc->ui_desp_88_router = 0;
    pst_edma_desc->ui_desp_88_merger = 0;
    pst_edma_desc->ui_desp_88_splitter = 0;
    pst_edma_desc->ui_desp_88_in_swapper = swap_setting;
    pst_edma_desc->ui_desp_88_out_swapper = 0;
    pst_edma_desc->ui_desp_88_extractor = 0;
    pst_edma_desc->ui_desp_88_constant_padder = 0;
    pst_edma_desc->ui_desp_88_converter = 0;

    // 0x8C: APU_EDMA3_DESP_8C
    pst_edma_desc->ui_desp_8c_line_packer = 0;
    pst_edma_desc->ui_desp_8c_up_sampler = 0;
    pst_edma_desc->ui_desp_8c_down_sampler = 0;
    pst_edma_desc->ui_desp_8c_reserved_0 = 0;

    // 0x90: APU_EDMA3_DESP_90
    pst_edma_desc->ui_desp_90_ar_sender_0 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_1 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_2 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_3 = 1;
    pst_edma_desc->ui_desp_90_r_receiver_0 = 15;
    pst_edma_desc->ui_desp_90_r_receiver_1 = 0;
    pst_edma_desc->ui_desp_90_r_receiver_2 = 0;
    pst_edma_desc->ui_desp_90_r_receiver_3 = 0;

    // 0x94: APU_EDMA3_DESP_94
    pst_edma_desc->ui_desp_94_aw_sender_0 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_1 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_2 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_3 = 2;
    pst_edma_desc->ui_desp_94_w_sender_0 = 15;
    pst_edma_desc->ui_desp_94_w_sender_1 = 0;
    pst_edma_desc->ui_desp_94_w_sender_2 = 0;
    pst_edma_desc->ui_desp_94_w_sender_3 = 0;

    // 0x98: APU_EDMA3_DESP_98
    pst_edma_desc->ui_desp_98_reader = 1;
    pst_edma_desc->ui_desp_98_writer = 1;
    pst_edma_desc->ui_desp_98_encoder = 0;
    pst_edma_desc->ui_desp_98_decoder = 0;
    pst_edma_desc->ui_desp_98_act_encoder = 0;
    pst_edma_desc->ui_desp_98_act_decoder = 0;
    pst_edma_desc->ui_desp_98_pipe_enable = 0x11;

    pst_edma_desc->ui_desp_9c_src_addr_2 = 0;
    pst_edma_desc->ui_desp_a0_dst_addr_2 = 0;

    pst_edma_desc->ui_desp_b4_src_x_size_2 = 0;
    pst_edma_desc->ui_desp_b8_src_y_size_2 = 0;
    pst_edma_desc->ui_desp_bc_src_z_size_2 = 0;
    pst_edma_desc->ui_desp_a4_src_x_stride_2 = 0;
    pst_edma_desc->ui_desp_ac_src_y_stride_2 = 0;

    pst_edma_desc->ui_desp_b4_dst_x_size_2 = 0;
    pst_edma_desc->ui_desp_b8_dst_y_size_2 = 0;
    pst_edma_desc->ui_desp_bc_dst_z_size_2 = 0;
    pst_edma_desc->ui_desp_a8_dst_x_stride_2 = 0;
    pst_edma_desc->ui_desp_b0_dst_y_stride_2 = 0;

    // 0xC0: APU_EDMA3_DESP_C0
    pst_edma_desc->ui_desp_c0_reserved_0 = 0;

    // 0xC4: APU_EDMA3_DESP_C4
    pst_edma_desc->ui_desp_c4_reserved_0 = 0;

    // 0xC8: APU_EDMA3_DESP_C8
    pst_edma_desc->ui_desp_c8_reserved_0 = 0;

    // 0xCC: APU_EDMA3_DESP_CC
    pst_edma_desc->ui_desp_cc_reserved_0 = 0;

    // 0xD0: APU_EDMA3_DESP_D0
    pst_edma_desc->ui_desp_d0_reserved_0 = 0;

    // 0xD4: APU_EDMA3_DESP_D4
    pst_edma_desc->ui_desp_d4_reserved_0 = 0;

    // 0xD8: APU_EDMA3_DESP_D8
    pst_edma_desc->ui_desp_d8_reserved_0 = 0;

    pst_edma_desc->ui_desp_dc_src_addr_3 = 0;
    pst_edma_desc->ui_desp_e0_dst_addr_3 = 0;

    pst_edma_desc->ui_desp_f4_src_x_size_3 = 0;
    pst_edma_desc->ui_desp_f8_src_y_size_3 = 0;
    pst_edma_desc->ui_desp_fc_src_z_size_3 = 0;
    pst_edma_desc->ui_desp_e4_src_x_stride_3 = 0;
    pst_edma_desc->ui_desp_ec_src_y_stride_3 = 0;

    pst_edma_desc->ui_desp_f4_dst_x_size_3 = 0;
    pst_edma_desc->ui_desp_f8_dst_y_size_3 = 0;
    pst_edma_desc->ui_desp_fc_dst_z_size_3 = 0;
    pst_edma_desc->ui_desp_e8_dst_x_stride_3 = 0;
    pst_edma_desc->ui_desp_f0_dst_y_stride_3 = 0;
}

void fillDesc565To888(st_edmaDescInfo565To888 *pst_edma_desc_info, st_edmaDescType15 *pst_edma_desc, int *pi_errorCode=NULL)
{
    unsigned char uc_fmt = pst_edma_desc_info->uc_fmt;

    // eDMA 3.1 descriptor
    // 0x00: APU_EDMA3_DESP_00
    pst_edma_desc->ui_desp_00_type = 15;
    pst_edma_desc->ui_desp_00_aruser = 0; // TBD
    pst_edma_desc->ui_desp_00_awuser = 0; // TBD
    pst_edma_desc->ui_desp_00_des_id = pst_edma_desc_info->uc_desc_id;
    pst_edma_desc->ui_desp_00_handshake_enable = 0; // TBD
    pst_edma_desc->ui_desp_00_reserved_0 = 0;

    // 0x04: APU_EDMA3_DESP_04
    //checkFormat(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_format = 0;
    pst_edma_desc->ui_desp_04_reserved_0 = 0;
    //checkRawShift(pst_edma_desc_info, pi_errorCode);
    ////EDMA_PERROR_CODE_CHECK_RETURN(pi_errorCode);
    pst_edma_desc->ui_desp_04_safe_mode = 1;
    pst_edma_desc->ui_desp_04_atomic_mode = 0;
    pst_edma_desc->ui_desp_04_read_tmp_buf = 0;
    pst_edma_desc->ui_desp_04_write_tmp_buf = 0;
    pst_edma_desc->ui_desp_04_in_rb_uv_swap = 0;
    pst_edma_desc->ui_desp_04_in_a_swap = 0;
    pst_edma_desc->ui_desp_04_reserved_1 = 0;

    if(uc_fmt == EDMA_FMT_565_TO_8888)
        pst_edma_desc->ui_desp_04_pad_const = pst_edma_desc_info->us_alpha;
    else
        pst_edma_desc->ui_desp_04_pad_const = 0;

    // 0x08: APU_EDMA3_DESP_08
    pst_edma_desc->ui_desp_08_in_swap_mat = 0x8421;

    if(uc_fmt == EDMA_FMT_565_TO_8888 && pst_edma_desc_info->uc_dst_alpha_pos == EDMA_RGB2RGBA_ALPHA_POS_LOW) // 1: low part
        pst_edma_desc->ui_desp_08_out_swap_mat = 0x1842;
    else
        pst_edma_desc->ui_desp_08_out_swap_mat = 0x8421;

    // 0x0C: APU_EDMA3_DESP_0C
    pst_edma_desc->ui_desp_0c_ufbc_tile_width = 0;
    pst_edma_desc->ui_desp_0c_reserved_0 = 0;
    pst_edma_desc->ui_desp_0c_ufbc_tile_height = 0;
    pst_edma_desc->ui_desp_0c_reserved_1 = 0;

    // 0x10: APU_EDMA3_DESP_10
    pst_edma_desc->ui_desp_10_ufbc_frame_width = 0;
    pst_edma_desc->ui_desp_10_reserved_0 = 0;
    pst_edma_desc->ui_desp_10_ufbc_frame_height = 0;
    pst_edma_desc->ui_desp_10_reserved_1 = 0;

    // 0x14: APU_EDMA3_DESP_14
    pst_edma_desc->ui_desp_14_ufbc_x_offset = 0;
    pst_edma_desc->ui_desp_14_reserved_0 = 0;
    pst_edma_desc->ui_desp_14_ufbc_y_offset = 0;
    pst_edma_desc->ui_desp_14_reserved_1 = 0;

    // 0x18: APU_EDMA3_DESP_18
    pst_edma_desc->ui_desp_18_ufbc_payload_offset = 0;

    // 0x1C: APU_EDMA3_DESP_1C
    pst_edma_desc->ui_desp_1c_src_addr_0 = pst_edma_desc_info->ui_src_addr_0;
    EDMA_LOG_DEBUG("SRC 0 addr: 0x%08x\n", pst_edma_desc->ui_desp_1c_src_addr_0);
    pst_edma_desc->ui_desp_20_dst_addr_0 = pst_edma_desc_info->ui_dst_addr_0;
    EDMA_LOG_DEBUG("DST 0 addr: 0x%08x\n", pst_edma_desc->ui_desp_20_dst_addr_0);

    pst_edma_desc->ui_desp_34_src_x_size_0 = pst_edma_desc_info->ui_src_size_x_0;
    pst_edma_desc->ui_desp_38_src_y_size_0 = pst_edma_desc_info->ui_src_size_y_0;
    pst_edma_desc->ui_desp_3c_src_z_size_0 = pst_edma_desc_info->ui_src_size_z_0;
    pst_edma_desc->ui_desp_24_src_x_stride_0 = pst_edma_desc_info->ui_src_pitch_x_0;
    pst_edma_desc->ui_desp_2c_src_y_stride_0 = pst_edma_desc_info->ui_src_pitch_y_0;

    EDMA_LOG_DEBUG("SRC 0 size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_src_x_size_0, pst_edma_desc->ui_desp_38_src_y_size_0, pst_edma_desc->ui_desp_3c_src_z_size_0);
    EDMA_LOG_DEBUG("    stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_24_src_x_stride_0, pst_edma_desc->ui_desp_2c_src_y_stride_0);
    pst_edma_desc->ui_desp_34_dst_x_size_0 = pst_edma_desc_info->ui_dst_size_x_0;
    pst_edma_desc->ui_desp_38_dst_y_size_0 = pst_edma_desc_info->ui_dst_size_y_0;
    pst_edma_desc->ui_desp_3c_dst_z_size_0 = pst_edma_desc_info->ui_dst_size_z_0;
    pst_edma_desc->ui_desp_28_dst_x_stride_0 = pst_edma_desc_info->ui_dst_pitch_x_0;
    pst_edma_desc->ui_desp_30_dst_y_stride_0 = pst_edma_desc_info->ui_dst_pitch_y_0;

    EDMA_LOG_DEBUG("DST 0 size: x- %.3d, y- %.3d, z- %.3d\n", pst_edma_desc->ui_desp_34_dst_x_size_0, pst_edma_desc->ui_desp_38_dst_y_size_0, pst_edma_desc->ui_desp_3c_dst_z_size_0);
    EDMA_LOG_DEBUG("    stride: x- %.3d, y- %.3d\n", pst_edma_desc->ui_desp_28_dst_x_stride_0, pst_edma_desc->ui_desp_30_dst_y_stride_0);

    // 0x40: APU_EDMA3_DESP_40
    pst_edma_desc->ui_desp_40_ufbc_alg_sel = 0;
    pst_edma_desc->ui_desp_40_ufbc_yuv_transform = 0;
    pst_edma_desc->ui_desp_40_ufbc_align_payload_offset = 0;
    pst_edma_desc->ui_desp_40_ufbc_dummy_write = 0;
    pst_edma_desc->ui_desp_40_ufbc_secure_mode = 0;
    pst_edma_desc->ui_desp_40_reserved_0 = 0;
    pst_edma_desc->ui_desp_40_act_msb_flip = 0;
    pst_edma_desc->ui_desp_40_act_dec_asm_left = 0;
    pst_edma_desc->ui_desp_40_reserved_1 = 0;
    pst_edma_desc->ui_desp_40_down_sampling_drop = 0;
    pst_edma_desc->ui_desp_40_down_sampling_filter = 0;
    pst_edma_desc->ui_desp_40_reserved_2 = 0;

    // 0x44: APU_EDMA3_DESP_44
    pst_edma_desc->ui_desp_44_r2y_y2r_mat_00 = 0;
    pst_edma_desc->ui_desp_44_r2y_y2r_mat_01 = 0;

    // 0x48: APU_EDMA3_DESP_48
    pst_edma_desc->ui_desp_48_r2y_y2r_mat_02 = 0;
    pst_edma_desc->ui_desp_48_r2y_y2r_vec_0 = 0;

    // 0x4C: APU_EDMA3_DESP_4C
    pst_edma_desc->ui_desp_4c_r2y_y2r_mat_10 = 0;
    pst_edma_desc->ui_desp_4c_r2y_y2r_mat_11 = 0;

    // 0x50: APU_EDMA3_DESP_50
    pst_edma_desc->ui_desp_50_r2y_y2r_mat_12 = 0;
    pst_edma_desc->ui_desp_50_r2y_y2r_vec_1 = 0;

    // 0x54: APU_EDMA3_DESP_54
    pst_edma_desc->ui_desp_54_r2y_y2r_mat_20 = 0;
    pst_edma_desc->ui_desp_54_r2y_y2r_mat_21 = 0;

    // 0x58: APU_EDMA3_DESP_58
    pst_edma_desc->ui_desp_58_r2y_y2r_mat_22 = 0;
    pst_edma_desc->ui_desp_58_r2y_y2r_vec_2 = 0;

    pst_edma_desc->ui_desp_5c_src_addr_1 = 0;
    pst_edma_desc->ui_desp_60_dst_addr_1 = 0;

    pst_edma_desc->ui_desp_74_src_x_size_1 = 0;
    pst_edma_desc->ui_desp_78_src_y_size_1 = 0;
    pst_edma_desc->ui_desp_7c_src_z_size_1 = 0;
    pst_edma_desc->ui_desp_64_src_x_stride_1 = 0;
    pst_edma_desc->ui_desp_6c_src_y_stride_1 = 0;

    pst_edma_desc->ui_desp_74_dst_x_size_1 = 0;
    pst_edma_desc->ui_desp_78_dst_y_size_1 = 0;
    pst_edma_desc->ui_desp_7c_dst_z_size_1 = 0;
    pst_edma_desc->ui_desp_68_dst_x_stride_1 = 0;
    pst_edma_desc->ui_desp_70_dst_y_stride_1 = 0;

    // 0x80: APU_EDMA3_DESP_80
    pst_edma_desc->ui_desp_80_in_extractor_0 = 9;
    pst_edma_desc->ui_desp_80_in_extractor_1 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_2 = 0;
    pst_edma_desc->ui_desp_80_in_extractor_3 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_0 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_1 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_2 = 0;
    pst_edma_desc->ui_desp_80_out_extractor_3 = 0;

    // 0x84: APU_EDMA3_DESP_84
    pst_edma_desc->ui_desp_84_in_zero_padder_0 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_1 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_2 = 0;
    pst_edma_desc->ui_desp_84_in_zero_padder_3 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_0 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_1 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_2 = 0;
    pst_edma_desc->ui_desp_84_out_zero_padder_3 = 0;

    // 0x88: APU_EDMA3_DESP_88
    if(uc_fmt == EDMA_FMT_565_TO_8888)
        pst_edma_desc->ui_desp_88_router = 0;
    else
        pst_edma_desc->ui_desp_88_router = 1;

    pst_edma_desc->ui_desp_88_merger = 0;
    pst_edma_desc->ui_desp_88_splitter = 0;

    pst_edma_desc->ui_desp_88_in_swapper = 0;

    if(uc_fmt == EDMA_FMT_565_TO_8888 && pst_edma_desc_info->uc_dst_alpha_pos == EDMA_RGB2RGBA_ALPHA_POS_LOW)
        pst_edma_desc->ui_desp_88_out_swapper = 3;
    else
        pst_edma_desc->ui_desp_88_out_swapper = 0;

    pst_edma_desc->ui_desp_88_extractor = 0;

    if(uc_fmt == EDMA_FMT_565_TO_8888)
        pst_edma_desc->ui_desp_88_constant_padder = 1;
    else
        pst_edma_desc->ui_desp_88_constant_padder = 0;

    pst_edma_desc->ui_desp_88_converter = 0;

    // 0x8C: APU_EDMA3_DESP_8C
    pst_edma_desc->ui_desp_8c_line_packer = 0;
    pst_edma_desc->ui_desp_8c_up_sampler = 0;
    pst_edma_desc->ui_desp_8c_down_sampler = 0;
    pst_edma_desc->ui_desp_8c_reserved_0 = 0;

    // 0x90: APU_EDMA3_DESP_90
    pst_edma_desc->ui_desp_90_ar_sender_0 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_1 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_2 = 1;
    pst_edma_desc->ui_desp_90_ar_sender_3 = 1;

    if(uc_fmt == EDMA_FMT_565_TO_8888)
        pst_edma_desc->ui_desp_90_r_receiver_0 = 7;
    else
        pst_edma_desc->ui_desp_90_r_receiver_0 = 9;

    pst_edma_desc->ui_desp_90_r_receiver_1 = 0;
    pst_edma_desc->ui_desp_90_r_receiver_2 = 0;
    pst_edma_desc->ui_desp_90_r_receiver_3 = 0;

    // 0x94: APU_EDMA3_DESP_94
    pst_edma_desc->ui_desp_94_aw_sender_0 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_1 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_2 = 2;
    pst_edma_desc->ui_desp_94_aw_sender_3 = 2;

    if(uc_fmt == EDMA_FMT_565_TO_8888)
        pst_edma_desc->ui_desp_94_w_sender_0 = 15;
    else
        pst_edma_desc->ui_desp_94_w_sender_0 = 14;

    pst_edma_desc->ui_desp_94_w_sender_1 = 0;
    pst_edma_desc->ui_desp_94_w_sender_2 = 0;
    pst_edma_desc->ui_desp_94_w_sender_3 = 0;

    // 0x98: APU_EDMA3_DESP_98
    pst_edma_desc->ui_desp_98_reader = 1;
    pst_edma_desc->ui_desp_98_writer = 1;
    pst_edma_desc->ui_desp_98_encoder = 0;
    pst_edma_desc->ui_desp_98_decoder = 0;
    pst_edma_desc->ui_desp_98_act_encoder = 0;
    pst_edma_desc->ui_desp_98_act_decoder = 0;
    pst_edma_desc->ui_desp_98_pipe_enable = 0x11;

    pst_edma_desc->ui_desp_9c_src_addr_2 = 0;
    pst_edma_desc->ui_desp_a0_dst_addr_2 = 0;

    pst_edma_desc->ui_desp_b4_src_x_size_2 = 0;
    pst_edma_desc->ui_desp_b8_src_y_size_2 = 0;
    pst_edma_desc->ui_desp_bc_src_z_size_2 = 0;
    pst_edma_desc->ui_desp_a4_src_x_stride_2 = 0;
    pst_edma_desc->ui_desp_ac_src_y_stride_2 = 0;

    pst_edma_desc->ui_desp_b4_dst_x_size_2 = 0;
    pst_edma_desc->ui_desp_b8_dst_y_size_2 = 0;
    pst_edma_desc->ui_desp_bc_dst_z_size_2 = 0;
    pst_edma_desc->ui_desp_a8_dst_x_stride_2 = 0;
    pst_edma_desc->ui_desp_b0_dst_y_stride_2 = 0;

    // 0xC0: APU_EDMA3_DESP_C0
    pst_edma_desc->ui_desp_c0_reserved_0 = 0;

    // 0xC4: APU_EDMA3_DESP_C4
    pst_edma_desc->ui_desp_c4_reserved_0 = 0;

    // 0xC8: APU_EDMA3_DESP_C8
    pst_edma_desc->ui_desp_c8_reserved_0 = 0;

    // 0xCC: APU_EDMA3_DESP_CC
    pst_edma_desc->ui_desp_cc_reserved_0 = 0;

    // 0xD0: APU_EDMA3_DESP_D0
    pst_edma_desc->ui_desp_d0_reserved_0 = 0;

    // 0xD4: APU_EDMA3_DESP_D4
    pst_edma_desc->ui_desp_d4_reserved_0 = 0;

    // 0xD8: APU_EDMA3_DESP_D8
    pst_edma_desc->ui_desp_d8_reserved_0 = 0;

    pst_edma_desc->ui_desp_dc_src_addr_3 = 0;
    pst_edma_desc->ui_desp_e0_dst_addr_3 = 0;

    pst_edma_desc->ui_desp_f4_src_x_size_3 = 0;
    pst_edma_desc->ui_desp_f8_src_y_size_3 = 0;
    pst_edma_desc->ui_desp_fc_src_z_size_3 = 0;
    pst_edma_desc->ui_desp_e4_src_x_stride_3 = 0;
    pst_edma_desc->ui_desp_ec_src_y_stride_3 = 0;

    pst_edma_desc->ui_desp_f4_dst_x_size_3 = 0;
    pst_edma_desc->ui_desp_f8_dst_y_size_3 = 0;
    pst_edma_desc->ui_desp_fc_dst_z_size_3 = 0;
    pst_edma_desc->ui_desp_e8_dst_x_stride_3 = 0;
    pst_edma_desc->ui_desp_f0_dst_y_stride_3 = 0;
}

