#ifndef EDMA_INFO_H_
#define EDMA_INFO_H_

#include "desc_type.h"
#include <linux/types.h>
#include "edma_def.h"
#include "EdmaMemoryBindings.h"

typedef enum {
    EDMA_FMT_NORMAL = 0, //direct copy mode
    EDMA_FMT_FP32 = 1,
    EDMA_FMT_FP16 = 2,
    EDMA_FMT_I8 = 3,
    EDMA_FMT_YUV420_2P_8B = 4,
    EDMA_FMT_YUV420_2P_16B = 5,

    EDMA_FMT_RGB = 8,
    EDMA_FMT_ARGB_8 = 9,
    EDMA_FMT_ARGB_16B = 10,

    EDMA_FMT_YUV422_8B = 29, //only support @ edma 2.1 up

    //only support @ edma 3.0 up
    EDMA_FMT_ROTATE90  = 90, //rotate angle 90
    EDMA_FMT_ROTATE180 = 91, //rotate angle 180
    EDMA_FMT_ROTATE270 = 92, //rotate angle 270

    EDMA_FMT_FILL = 600, // fill mode
    EDMA_FMT_DEPTHTOSPACE, // depth to space
    EDMA_FMT_DEPTHTOSPACE_FAST, // depth to space
    EDMA_FMT_S2D_FAST,

    EDMA_FMT_UVResz2X, //UV 2x resize up sample

    EDMA_FMT_PRODUCER,
    EDMA_FMT_CONSUMER,
    EDMA_FMT_NONE, // fill mode

}EDMA_FMT_E;

typedef enum {
	/* ENCODE */
	EDMA_UFBC_DECODE,
	EDMA_UFBC_ENCODE,

	EDMA_UFBC_RGBA8_TO_RGBA8_CODE = 31,
	EDMA_UFBC_YUV420_8_TO_YUV420_8_CODE = 32,
	EDMA_UFBC_RGB8_TO_YUV420_8_CODE = 33,
	EDMA_UFBC_RGBA102_TO_RGBA10_CODE = 34,
	EDMA_UFBC_YUV420_16_TO_YUV420_10_CODE = 35,
	EDMA_UFBC_RGBA16_TO_YUV420_10_CODE = 36,
	EDMA_UFBC_ACT_TO_ACT_CODE = 37,

	/* DECODE */
	EDMA_UFBC_RGBA8_CODE_TO_RGBA8 = 38,
	EDMA_UFBC_YUV420_8_CODE_TO_YUV420_8 = 39,
	EDMA_UFBC_YUV420_8_CODE_TO_RGBA8 = 40,
	EDMA_UFBC_RGBA10_CODE_TO_RGBA16 = 41,
	EDMA_UFBC_YUV420_10_CODE_TO_YUV420_16 = 42,
	EDMA_UFBC_YUV420_10_CODE_TO_RGBA16 = 43,
	EDMA_UFBC_ACT_CODE_TO_ACT = 44,

}EDMA_UFBC_FMT;

typedef enum {
    EDMA_CONSTANT_PADDING = 0,
    EDMA_EDGE_PADDING = 1,
} EDMA_PADDING_MODE;

struct edma_ext {
	__u64 cmd_handle;
	__u32 count;
	__u32 reg_addr;
	__u32 fill_value;
	__u8	desp_iommu_en;
} __attribute__ ((__packed__));

#define EDMA_DESC_INFO_TYPE_5 EDMA_DESC_INFO_COLORCVT
typedef enum {
	EDMA_DESC_INFO_TYPE_0 = 0, //direct copy mode
	EDMA_DESC_INFO_TYPE_1 = 1,
	EDMA_INFO_GENERAL = 2,
	EDMA_INFO_NN = 3,
	EDMA_INFO_DATA = 4,
	EDMA_DESC_INFO_COLORCVT = 5,
	EDMA_DESC_INFO_ROTATE = 6,
	EDMA_DESC_INFO_SPLITN = 7,
	EDMA_DESC_INFO_MERGEN = 8,
	EDMA_DESC_INFO_UNPACK = 9,
	EDMA_DESC_INFO_SWAP2 = 10,
	EDMA_DESC_INFO_565TO888 = 11,
	EDMA_DESC_INFO_PACK = 12,
	EDMA_INFO_UFBC = 13,
	EDMA_INFO_SDK = 14,
	EDMA_INFO_SLICE = 15,
	EDMA_INFO_PAD = 16,
	EDMA_INFO_BAYER_TO_RGGB = 17,
	EDMA_INFO_RGGB_TO_BAYER = 18,
}EDMA_INFO_TYPE;


typedef struct _edma_nn_ouput {
	void *out_edma_desc;
	unsigned int	desc_num; // return to NN total number of desc
	__u32 des_offset;
}st_edma_nn_ouput;
#pragma pack(1)

typedef struct _edma_info_nn {

	__u8 desc_id;

	EDMA_FMT_E inFormat;	// descriptor type 0: 64 bytes descriptor
	EDMA_FMT_E outFormat;	// descriptor id
// format
	__u32 src_stride_h; // stride h = c*w*h, "pixel" of stride
	__u32 src_stride_w; // stride w	= c*w
	__u32 src_stride_c; // stride c : c

	__u32 dst_stride_h;
	__u32 dst_stride_w;
	__u32 dst_stride_c;

	__u16 shape_6d;
	__u16 shape_5d;
	__u16 shape_n; //n * h = z
	__u16 shape_h; // part of z
	__u16 shape_w; // y
	__u16 shape_c; // x


	__u16 src_type_size; //bytes of pixel, should the same = src in normal mode
	__u16 dst_type_size;

	__u32 src_addr_offset;	// out: offset to codebuf
	__u32 dst_addr_offset;	// out: offset to codebuf
	__u32 dummy_addr_offset;	//out: offset to codebuf Used in YUV420.

	__u32 src_addr2_offset;	// out: offset to codebuf
	__u32 dst_addr2_offset;	// out: offset to codebuf

//case 1
	// N * H, W, C
	// C = 3, W = 20, N =2	, src type size = 1 , format = normal
	// stride w = CxW
	// stride h = CxWxH
	// dst c = 4, W = 20, N =2	, src type size = 1 , format = normal
	//	N * H, W, C ==> Z,Y,X
	//	RGB->RGBA ==> XYZ => 1, N*H, W*3 (RGB => ARGB)

	__u16 block_size; //depth to space factor , n means width = n, height = n


//-----------------------------------
// case 2 x
	// C = 3, W = 20, N = 2 , H = 20, src_type_size = 1, dst_type_size = 1, format = rgb -> argb
	// 1, N*H, W*3 (RGB => ARGB)

}st_edma_info_nn;
#pragma pack()

#pragma pack(1)

typedef struct _edma_request_nn {
	st_edma_info_nn *info_list;
	__u32 info_num;
}st_edma_request_nn;

#pragma pack()

#pragma pack(1)
typedef struct _edma_result_nn {
	__u32 cmd_size;
	__u32 cmd_count;
}st_edma_result_nn;
#pragma pack()

#pragma pack(1)
typedef struct _edma_support_nn {
	bool* support_list;
	__u32 count;
	} st_edma_support_nn;
#pragma pack()


#pragma pack(1)
typedef struct _edma_desBuf {
	__u8 * descBuf;
	__u32 des_num;	// out
}st_edma_desBuf;
#pragma pack()


#pragma pack(1)
typedef struct _eInfo_nnPack {
	st_edma_request_nn	 info;
	st_edma_nn_ouput	*pOut;
}st_edma_info_nnPack;
#pragma pack()

typedef struct _imgCropShape
{
	unsigned int x_offset;	 // x_offset from big img
	unsigned int y_offset;	 // y_offset from big img
	unsigned int x_size;	 // width of img
	unsigned int y_size;	 // height of img
} st_img_corp_shape;

#pragma pack(1)
typedef struct _edma_InputShape
{
	unsigned char uc_desc_type;	 // descriptor type 1: 64 bytes descriptor

	EDMA_FMT_E inFormat;	 // descriptor type 0: 64 bytes descriptor
	EDMA_FMT_E outFormat;	 // descriptor id
	// format
	// 0: normal mode
	// 1: fill mode
	// 2: f16_to_f32
	// 3: f32_to_f16
	// 4: i8_to_f32
	// 5: f32_to_i8
	// 6: i8_to_i8
	// 21: raw_8_to_raw_16
	// 22: raw_10_to_raw_16
	// 23: raw_12_to_raw_16
	// 24: raw_14_to_raw_16
	// only support @ edma 3.0 up
	// 90: rotate angle 90
	// 91: rotate angle 180
	// 92: rotate angle 270

	unsigned int inBuf_addr;	 // source address of channel 0
	unsigned int outBuf_addr;	 // source address of channel 0
	unsigned int size_x;	 // unit: pixel, x axis pixel length per line
	// ui_src_size_x <=536870911
	unsigned int size_y;	 // unit: line
	// ui_src_size_y <= 4294967295
	unsigned int size_z;	 // unit: frame
	// ui_src_size_z <= 4294967295
	unsigned int dst_stride_x;	 // unit: depth
	// ui_src_size_w <= 4294967295
	unsigned int dst_stride_y;	 // unit: pixel

	st_img_corp_shape cropShape;
	unsigned int tileNum;	 // unit: tileNum

	__u32 src_addr_offset;	// out: offset to codebuf
	__u32 dst_addr_offset;	// out: offset to codebuf
} st_edma_InputShape;
#pragma pack()


typedef enum {
    EDMA_SDK_TYPE_GNL = 0, //SDK EDMA general input
	EDMA_SDK_MM02, //SDK MM 02

    EDMA_SDK_NONE, //SDK NONE
}EDMA_SDK_TYPE;

typedef struct _imgbuffer
{
	unsigned int imgIova;	 // sdk customerize info sturcture
	unsigned int imgW;	 // width of img
	unsigned int imgH;	 // height of img
	unsigned int imgSize;	 // img size
} st_img_buf;

typedef struct _sdk_aicar
{
	// sdk customerize info sturcture
	st_img_buf imgInBuf[6];	 // ai car input buffer YUVx2
	st_img_buf imgOutBuf;	 // ai car output buffer
	unsigned int numberImg;
	unsigned int data; //reserved for checking
} st_sdk_aicar;

typedef struct _sdk_typeGnl_
{
	// eDMA 3.1 descriptor
	// 0x00: APU_EDMA3_DESP_00
	unsigned int ui_desp_00_type:4;
	unsigned int ui_desp_00_aruser:2;
	unsigned int ui_desp_00_awuser:2;
	unsigned int ui_desp_00_des_id:8;
	unsigned int ui_desp_00_handshake_enable:8;
	unsigned int ui_desp_00_reserved_0:8;

	// 0x04: APU_EDMA3_DESP_04
	unsigned int ui_desp_04_format:6;
	unsigned int ui_desp_04_reserved_0:2;
	unsigned int ui_desp_04_safe_mode:1;
	unsigned int ui_desp_04_atomic_mode:1;
	unsigned int ui_desp_04_read_tmp_buf:1;
	unsigned int ui_desp_04_write_tmp_buf:1;
	unsigned int ui_desp_04_in_rb_uv_swap:1;
	unsigned int ui_desp_04_in_a_swap:1;
	unsigned int ui_desp_04_reserved_1:2;
	unsigned int ui_desp_04_pad_const:16;

	// 0x08: APU_EDMA3_DESP_08
	unsigned int ui_desp_08_in_swap_mat:16;
	unsigned int ui_desp_08_out_swap_mat:16;

	// 0x0C: APU_EDMA3_DESP_0C
	unsigned int ui_desp_0c_ufbc_tile_width:15;
	unsigned int ui_desp_0c_reserved_0:1;
	unsigned int ui_desp_0c_ufbc_tile_height:15;
	unsigned int ui_desp_0c_reserved_1:1;

	// 0x10: APU_EDMA3_DESP_10
	unsigned int ui_desp_10_ufbc_frame_width:15;
	unsigned int ui_desp_10_reserved_0:1;
	unsigned int ui_desp_10_ufbc_frame_height:15;
	unsigned int ui_desp_10_reserved_1:1;

	// 0x14: APU_EDMA3_DESP_14
	unsigned int ui_desp_14_ufbc_x_offset:15;
	unsigned int ui_desp_14_reserved_0:1;
	unsigned int ui_desp_14_ufbc_y_offset:15;
	unsigned int ui_desp_14_reserved_1:1;

	// 0x18: APU_EDMA3_DESP_18
	unsigned int ui_desp_18_ufbc_payload_offset:32;

	// 0x1C: APU_EDMA3_DESP_1C
	unsigned int ui_desp_1c_src_addr_0:32;

	// 0x20: APU_EDMA3_DESP_20
	unsigned int ui_desp_20_dst_addr_0:32;

	// 0x24: APU_EDMA3_DESP_24
	unsigned int ui_desp_24_src_x_stride_0:32;

	// 0x28: APU_EDMA3_DESP_28
	unsigned int ui_desp_28_dst_x_stride_0:32;

	// 0x2C: APU_EDMA3_DESP_2C
	unsigned int ui_desp_2c_src_y_stride_0:32;

	// 0x30: APU_EDMA3_DESP_30
	unsigned int ui_desp_30_dst_y_stride_0:32;

	// 0x34: APU_EDMA3_DESP_34
	unsigned int ui_desp_34_src_x_size_0:16;
	unsigned int ui_desp_34_dst_x_size_0:16;

	// 0x38: APU_EDMA3_DESP_38
	unsigned int ui_desp_38_src_y_size_0:16;
	unsigned int ui_desp_38_dst_y_size_0:16;

	// 0x3C: APU_EDMA3_DESP_3C
	unsigned int ui_desp_3c_src_z_size_0:16;
	unsigned int ui_desp_3c_dst_z_size_0:16;

	// 0x40: APU_EDMA3_DESP_40
	unsigned int ui_desp_40_ufbc_alg_sel:1;
	unsigned int ui_desp_40_ufbc_yuv_transform:1;
	unsigned int ui_desp_40_ufbc_align_payload_offset:1;
	unsigned int ui_desp_40_ufbc_dummy_write:1;
	unsigned int ui_desp_40_ufbc_secure_mode:1;
	unsigned int ui_desp_40_reserved_0:3;
	unsigned int ui_desp_40_act_msb_flip:2;
	unsigned int ui_desp_40_act_dec_asm_left:2;
	unsigned int ui_desp_40_reserved_1:4;
	unsigned int ui_desp_40_down_sampling_drop:1;
	unsigned int ui_desp_40_down_sampling_filter:1;
	unsigned int ui_desp_40_reserved_2:14;

	// 0x44: APU_EDMA3_DESP_44
	unsigned int ui_desp_44_r2y_y2r_mat_00:16;
	unsigned int ui_desp_44_r2y_y2r_mat_01:16;

	// 0x48: APU_EDMA3_DESP_48
	unsigned int ui_desp_48_r2y_y2r_mat_02:16;
	unsigned int ui_desp_48_r2y_y2r_vec_0:16;

	// 0x4C: APU_EDMA3_DESP_4C
	unsigned int ui_desp_4c_r2y_y2r_mat_10:16;
	unsigned int ui_desp_4c_r2y_y2r_mat_11:16;

	// 0x50: APU_EDMA3_DESP_50
	unsigned int ui_desp_50_r2y_y2r_mat_12:16;
	unsigned int ui_desp_50_r2y_y2r_vec_1:16;

	// 0x54: APU_EDMA3_DESP_54
	unsigned int ui_desp_54_r2y_y2r_mat_20:16;
	unsigned int ui_desp_54_r2y_y2r_mat_21:16;

	// 0x58: APU_EDMA3_DESP_58
	unsigned int ui_desp_58_r2y_y2r_mat_22:16;
	unsigned int ui_desp_58_r2y_y2r_vec_2:16;

	// 0x5C: APU_EDMA3_DESP_5C
	unsigned int ui_desp_5c_src_addr_1:32;

	// 0x60: APU_EDMA3_DESP_60
	unsigned int ui_desp_60_dst_addr_1:32;

	// 0x64: APU_EDMA3_DESP_64
	unsigned int ui_desp_64_src_x_stride_1:32;

	// 0x68: APU_EDMA3_DESP_68
	unsigned int ui_desp_68_dst_x_stride_1:32;

	// 0x6C: APU_EDMA3_DESP_6C
	unsigned int ui_desp_6c_src_y_stride_1:32;

	// 0x70: APU_EDMA3_DESP_70
	unsigned int ui_desp_70_dst_y_stride_1:32;

	// 0x74: APU_EDMA3_DESP_74
	unsigned int ui_desp_74_src_x_size_1:16;
	unsigned int ui_desp_74_dst_x_size_1:16;

	// 0x78: APU_EDMA3_DESP_78
	unsigned int ui_desp_78_src_y_size_1:16;
	unsigned int ui_desp_78_dst_y_size_1:16;

	// 0x7C: APU_EDMA3_DESP_7C
	unsigned int ui_desp_7c_src_z_size_1:16;
	unsigned int ui_desp_7c_dst_z_size_1:16;

	// 0x80: APU_EDMA3_DESP_80
	unsigned int ui_desp_80_in_extractor_0:4;
	unsigned int ui_desp_80_in_extractor_1:4;
	unsigned int ui_desp_80_in_extractor_2:4;
	unsigned int ui_desp_80_in_extractor_3:4;
	unsigned int ui_desp_80_out_extractor_0:4;
	unsigned int ui_desp_80_out_extractor_1:4;
	unsigned int ui_desp_80_out_extractor_2:4;
	unsigned int ui_desp_80_out_extractor_3:4;

	// 0x84: APU_EDMA3_DESP_84
	unsigned int ui_desp_84_in_zero_padder_0:4;
	unsigned int ui_desp_84_in_zero_padder_1:4;
	unsigned int ui_desp_84_in_zero_padder_2:4;
	unsigned int ui_desp_84_in_zero_padder_3:4;
	unsigned int ui_desp_84_out_zero_padder_0:4;
	unsigned int ui_desp_84_out_zero_padder_1:4;
	unsigned int ui_desp_84_out_zero_padder_2:4;
	unsigned int ui_desp_84_out_zero_padder_3:4;

	// 0x88: APU_EDMA3_DESP_88
	unsigned int ui_desp_88_router:4;
	unsigned int ui_desp_88_merger:4;
	unsigned int ui_desp_88_splitter:4;
	unsigned int ui_desp_88_in_swapper:4;
	unsigned int ui_desp_88_out_swapper:4;
	unsigned int ui_desp_88_extractor:4;
	unsigned int ui_desp_88_constant_padder:4;
	unsigned int ui_desp_88_converter:4;

	// 0x8C: APU_EDMA3_DESP_8C
	unsigned int ui_desp_8c_line_packer:4;
	unsigned int ui_desp_8c_up_sampler:4;
	unsigned int ui_desp_8c_down_sampler:8;
	unsigned int ui_desp_8c_reserved_0:16;

	// 0x90: APU_EDMA3_DESP_90
	unsigned int ui_desp_90_ar_sender_0:4;
	unsigned int ui_desp_90_ar_sender_1:4;
	unsigned int ui_desp_90_ar_sender_2:4;
	unsigned int ui_desp_90_ar_sender_3:4;
	unsigned int ui_desp_90_r_receiver_0:4;
	unsigned int ui_desp_90_r_receiver_1:4;
	unsigned int ui_desp_90_r_receiver_2:4;
	unsigned int ui_desp_90_r_receiver_3:4;

	// 0x94: APU_EDMA3_DESP_94
	unsigned int ui_desp_94_aw_sender_0:4;
	unsigned int ui_desp_94_aw_sender_1:4;
	unsigned int ui_desp_94_aw_sender_2:4;
	unsigned int ui_desp_94_aw_sender_3:4;
	unsigned int ui_desp_94_w_sender_0:4;
	unsigned int ui_desp_94_w_sender_1:4;
	unsigned int ui_desp_94_w_sender_2:4;
	unsigned int ui_desp_94_w_sender_3:4;

	// 0x98: APU_EDMA3_DESP_98
	unsigned int ui_desp_98_reader:4;
	unsigned int ui_desp_98_writer:4;
	unsigned int ui_desp_98_encoder:4;
	unsigned int ui_desp_98_decoder:4;
	unsigned int ui_desp_98_act_encoder:4;
	unsigned int ui_desp_98_act_decoder:4;
	unsigned int ui_desp_98_pipe_enable:8;

	// 0x9C: APU_EDMA3_DESP_9C
	unsigned int ui_desp_9c_src_addr_2:32;

	// 0xA0: APU_EDMA3_DESP_A0
	unsigned int ui_desp_a0_dst_addr_2:32;

	// 0xA4: APU_EDMA3_DESP_A4
	unsigned int ui_desp_a4_src_x_stride_2:32;

	// 0xA8: APU_EDMA3_DESP_A8
	unsigned int ui_desp_a8_dst_x_stride_2:32;

	// 0xAC: APU_EDMA3_DESP_AC
	unsigned int ui_desp_ac_src_y_stride_2:32;

	// 0xB0: APU_EDMA3_DESP_B0
	unsigned int ui_desp_b0_dst_y_stride_2:32;

	// 0xB4: APU_EDMA3_DESP_B4
	unsigned int ui_desp_b4_src_x_size_2:16;
	unsigned int ui_desp_b4_dst_x_size_2:16;

	// 0xB8: APU_EDMA3_DESP_B8
	unsigned int ui_desp_b8_src_y_size_2:16;
	unsigned int ui_desp_b8_dst_y_size_2:16;

	// 0xBC: APU_EDMA3_DESP_BC
	unsigned int ui_desp_bc_src_z_size_2:16;
	unsigned int ui_desp_bc_dst_z_size_2:16;

	// 0xC0: APU_EDMA3_DESP_C0
	unsigned int ui_desp_c0_reserved_0:32;

	// 0xC4: APU_EDMA3_DESP_C4
	unsigned int ui_desp_c4_reserved_0:32;

	// 0xC8: APU_EDMA3_DESP_C8
	unsigned int ui_desp_c8_reserved_0:32;

	// 0xCC: APU_EDMA3_DESP_CC
	unsigned int ui_desp_cc_reserved_0:32;

	// 0xD0: APU_EDMA3_DESP_D0
	unsigned int ui_desp_d0_reserved_0:32;

	// 0xD4: APU_EDMA3_DESP_D4
	unsigned int ui_desp_d4_reserved_0:32;

	// 0xD8: APU_EDMA3_DESP_D8
	unsigned int ui_desp_d8_reserved_0:32;

	// 0xDC: APU_EDMA3_DESP_DC
	unsigned int ui_desp_dc_src_addr_3:32;

	// 0xE0: APU_EDMA3_DESP_E0
	unsigned int ui_desp_e0_dst_addr_3:32;

	// 0xE4: APU_EDMA3_DESP_E4
	unsigned int ui_desp_e4_src_x_stride_3:32;

	// 0xE8: APU_EDMA3_DESP_E8
	unsigned int ui_desp_e8_dst_x_stride_3:32;

	// 0xEC: APU_EDMA3_DESP_EC
	unsigned int ui_desp_ec_src_y_stride_3:32;

	// 0xF0: APU_EDMA3_DESP_F0
	unsigned int ui_desp_f0_dst_y_stride_3:32;

	// 0xF4: APU_EDMA3_DESP_F4
	unsigned int ui_desp_f4_src_x_size_3:16;
	unsigned int ui_desp_f4_dst_x_size_3:16;

	// 0xF8: APU_EDMA3_DESP_F8
	unsigned int ui_desp_f8_src_y_size_3:16;
	unsigned int ui_desp_f8_dst_y_size_3:16;

	// 0xFC: APU_EDMA3_DESP_FC
	unsigned int ui_desp_fc_src_z_size_3:16;
	unsigned int ui_desp_fc_dst_z_size_3:16;
} st_sdk_typeGnl;

#pragma pack(1)
typedef struct _edma_InfoSDK
{
	EDMA_SDK_TYPE sdk_type;	 // sdk customerize type

	unsigned char *pSdkInfo;	 // sdk customerize info sturcture
	unsigned int sdkInfoSize;	 // sdk info size
} st_edma_InfoSDK;
#pragma pack()


#pragma pack(1)
typedef struct _edma_InfoData
{
	unsigned char uc_desc_type;	 // descriptor type 1: 64 bytes descriptor

	EDMA_FMT_E inFormat;	// support normal mode copy and fill mode
	EDMA_FMT_E outFormat;	 // descriptor id
	// format
	// 0: normal mode
	// 1: fill mode

	unsigned int inBuf_addr;	 // source address of channel 0
	unsigned int outBuf_addr;	 // source address of channel 0

	unsigned int fillValue;	 // unit: fill value
	unsigned int copy_size;	 // unit: size of copy to dst size (bytes)

	unsigned char pVID;	 // producer VID
	unsigned char cVID;	 // consumber VID
	unsigned char cWait; // consumber waitFlag

	// size < 65535 ok,	size > 65535 might need multi-descriptor

} st_edma_InfoData;
#pragma pack()

#pragma pack(1)
typedef struct _edmaDescInfoType0_
{
	unsigned char uc_desc_type;	 // descriptor type 0: 64 bytes HW descriptor
	unsigned char uc_desc_id;	 // descriptor id
	unsigned char uc_fmt;	 // format
	// 0: normal mode
	// 1: fill mode
	// 2: f16_to_f32
	// 3: f32_to_f16
	// 4: i8_to_f32
	// 5: f32_to_i8
	// 6: i8_to_i8
	// 21: raw_8_to_raw_16
	// 22: raw_10_to_raw_16
	// 23: raw_12_to_raw_16
	// 24: raw_14_to_raw_16

	unsigned int ui_src_addr_0;	 // source address of channel 0
	unsigned int ui_src_size_x;	 // unit: byte
	// ui_src_size_x <=536870911
	unsigned int ui_src_size_y;	 // unit: line
	// ui_src_size_y <= 4294967295
	unsigned int ui_src_size_z;	 // unit: frame
	// ui_src_size_z <= 4294967295
	unsigned int ui_src_size_w;	 // unit: depth
	// ui_src_size_w <= 4294967295
	unsigned int ui_src_pitch_x_0;	// unit: byte
	unsigned int ui_src_pitch_y_0;	// unit: byte
	unsigned int ui_src_pitch_z_0;	// unit: byte

	unsigned int ui_dst_addr_0;	 // destination address of channel 0
	unsigned int ui_dst_size_x;	 // unit: byte
	// ui_dst_size_x <=4294967295
	unsigned int ui_dst_size_y;	 // unit: line
	// ui_dst_size_y <= 4294967295
	unsigned int ui_dst_size_z;	 // unit: frame
	// ui_dst_size_z <= 4294967295
	unsigned int ui_dst_size_w;	 // unit: depth
	// ui_dst_size_w <= 4294967295
	unsigned int ui_dst_pitch_x_0;	// unit: byte
	unsigned int ui_dst_pitch_y_0;	// unit: byte
	unsigned int ui_dst_pitch_z_0;	// unit: byte

	// FIX8 to FP32
	float fl_fix8tofp32_min;	// out = min + in * scale / 255
	float fl_fix8tofp32_scale;

	// FP32 to FIX8
	float fl_fp32tofix8_min;	// out = (in - min) * scale
	float fl_fp32tofix8_scale;

	// raw function
	unsigned char uc_raw_shift;	 // 0: align to lsb
	// 2: align to lsb and left shift 2 bits
	// 4: align to lsb and left shift 4 bits
	// 6: align to lsb and left shift 6 bits
	// 8: align to lsb and left shift 8 bits

	// filling
	unsigned int ui_fill_value;	 // fill value for fill mode

	DescType descType;
} st_edmaDescInfoType0;
#pragma pack()

#pragma pack(1)
typedef struct _edmaDescInfoType1_
{
	unsigned char uc_desc_type;	 // descriptor type 1: 64 bytes descriptor
	unsigned char uc_desc_id;	 // descriptor id
	unsigned char uc_fmt;	 // format
	// 7: rgb_8_to_rgba_8
	// 12: rgba_102_to_rgba_16
	// 20: rgba_f1110_to_rgba_f16

	unsigned int ui_src_addr_0;	 // source address of channel 0
	unsigned int ui_src_size_x;	 // unit: byte
	// ui_src_size_x <= 2147483647
	unsigned int ui_src_size_y;	 // unit: line
	// ui_src_size_y <= 4294967295
	unsigned int ui_src_size_z;	 // unit: frame
	// ui_src_size_z <= 4294967295
	unsigned int ui_src_size_w;	 // unit: depth
	// ui_src_size_w <= 4294967295
	unsigned int ui_src_pitch_x_0;	// unit: byte
	unsigned int ui_src_pitch_y_0;	// unit: byte
	unsigned int ui_src_pitch_z_0;	// unit: byte

	unsigned int ui_dst_addr_0;	 // destination address of channel 0
	unsigned int ui_dst_size_x;	 // unit: byte
	// ui_dst_size_x <=4294967295
	unsigned int ui_dst_size_y;	 // unit: line
	// ui_dst_size_y <= 4294967295
	unsigned int ui_dst_size_z;	 // unit: frame
	// ui_dst_size_z <= 4294967295
	unsigned int ui_dst_size_w;	 // unit: depth
	// ui_dst_size_w <= 4294967295
	unsigned int ui_dst_pitch_x_0;	// unit: byte
	unsigned int ui_dst_pitch_y_0;	// unit: byte
	unsigned int ui_dst_pitch_z_0;	// unit: byte

	// alpha information
	unsigned char uc_src_alpha_pos;	 // 0: high part, 1: low part
	unsigned char uc_dst_alpha_pos;	 // 0: high part, 1: low part
	unsigned short us_alpha;	// alpha value

	DescType descType;
} st_edmaDescInfoType1;
#pragma pack()


#pragma pack(1)
typedef struct _edmaDescInfoType5_
{
	unsigned char uc_desc_type; 		// descriptor type 5: 128 bytes for HW descriptor
	unsigned char uc_desc_id;			// descriptor id

	unsigned char uc_fmt;				// format
	// 8: yuv420_8_to_rgba_8
	// 9: yuv422_8_to_rgba_8
	// 10: rgb_8_to_yuv420_8
	// 11: rgba_8_to_yuv420_8
	// 13: yuv420_16_to_rgba_16
	// 14: yuv420_p010_to_rgba_16 (TBD)
	// 15: yuv420_mtk_to_rgba_16 (TBD)
	// 16: yuv420_p010_to_yuv420_16 (TBD)
	// 17: yuv420_mtk_to_yuv420_16 (TBD)
	// 18: rgba_16_to_yuv420_16
	// 19: rgba_102_to_yuv420_16 (TBD)
	// 31: rgba_8_to_rgba_8_code (TBD)
	// 32: yuv420_8_to_yuv420_8_code (TBD)
	// 33: rgb_8_to_yuv420_8_code (TBD)
	// 34: rgba_102_to_rgba_10_code (TBD)
	// 35: yuv420_16_to_yuv420_10_code (TBD)
	// 36: rgba_16_to_yuv420_10_code (TBD)
	// 38: rgba_8_code_to_rgba_8 (TBD)
	// 39: yuv420_8_code_to_yuv420_8 (TBD)
	// 40: yuv420_8_code_to_rgba_8 (TBD)
	// 41: rgba_10_code_to_rgba_16 (TBD)
	// 42: yuv420_10_code_to_yuv420_16 (TBD)
	// 43: yuv420_10_code_to_rgba_16 (TBD)
	// 45: bayer_10_to_ainr_16 (TBD)
	// 46: bayer_12_to_ainr_16 (TBD)
	// 47: 4cell_10_to_ainr_16 (TBD)
	// 48: 4cell_12_to_ainr_16 (TBD)
	// 51: ainr_16_to_bayer_10 (TBD)
	// 52: ainr_16_to_bayer_12 (TBD)
	// 53: ainr_16_to_bayer_16 (TBD)
	// 55: ainr_16_to_3pfullg_12_g (TBD)
	// 56: ainr_16_to_3pfullg_12_rb (TBD)
	// 128: rgb_8_to_gray_8
	// 129: yuv420_8_to_rgb_8
	// 130: yuv420_8_to_yuv444_8
	// 131: yuv444_8_to_yuv420_8

	unsigned char uc_src_layout;		// 0: interleaved, 1: semi-planar, 2: planar
	unsigned char uc_src_itlv_fmt;		// interleaved format (lsb -> msb)
	// YUV420 - 0: UV, 1: VU
	// YUV422 - 0: YUYV, 1: YVYU
	// RGBA - 0: RGBA, 1: ARGB, 2: BGRA, 3: ABGR
	unsigned int ui_src_addr_0; 		// source address of channel 0
	unsigned int ui_src_size_x_0;		// unit: byte
	// ui_src_size_x_0 <= 536870911
	unsigned int ui_src_size_y_0;		// unit: line
	// ui_src_size_y_0 <= 4294967295
	unsigned int ui_src_size_z_0;		// unit: frame
	// ui_src_size_z_0 <= 4294967295
	unsigned int ui_src_size_w_0;		// unit: depth
	// ui_src_size_w_0 <= 4294967295
	unsigned int ui_src_pitch_x_0;		// unit: byte
	unsigned int ui_src_pitch_y_0;		// unit: byte
	unsigned int ui_src_pitch_z_0;		// unit: byte

	unsigned int ui_src_addr_1; 		// source address of channel 1
	unsigned int ui_src_size_x_1;		// unit: byte
	// ui_src_size_x_1 <= 536870911
	unsigned int ui_src_size_y_1;		// unit: line
	// ui_src_size_y_1 <= 4294967295
	unsigned int ui_src_size_z_1;		// unit: frame
	// ui_src_size_z_1 <= 4294967295
	unsigned int ui_src_size_w_1;		// unit: depth
	// ui_src_size_w_1 <= 4294967295
	unsigned int ui_src_pitch_x_1;		// unit: byte
	unsigned int ui_src_pitch_y_1;		// unit: byte
	unsigned int ui_src_pitch_z_1;		// unit: byte

	unsigned int ui_src_addr_2; 		// source address of channel 2
	unsigned int ui_src_size_x_2;		// unit: byte
	// ui_src_size_x_2 <= 536870911
	unsigned int ui_src_size_y_2;		// unit: line
	// ui_src_size_y_2 <= 4294967295
	unsigned int ui_src_size_z_2;		// unit: frame
	// ui_src_size_z_2 <= 4294967295
	unsigned int ui_src_size_w_2;		// unit: depth
	// ui_src_size_w_2 <= 4294967295
	unsigned int ui_src_pitch_x_2;		// unit: byte
	unsigned int ui_src_pitch_y_2;		// unit: byte
	unsigned int ui_src_pitch_z_2;		// unit: byte

	unsigned int ui_src_addr_3; 		// source address of channel 3
	unsigned int ui_src_size_x_3;		// unit: byte
	// ui_src_size_x_3 <= 536870911
	unsigned int ui_src_size_y_3;		// unit: line
	// ui_src_size_y_3 <= 4294967295
	unsigned int ui_src_size_z_3;		// unit: frame
	// ui_src_size_z_3 <= 4294967295
	unsigned int ui_src_size_w_3;		// unit: depth
	// ui_src_size_w_3 <= 4294967295
	unsigned int ui_src_pitch_x_3;		// unit: byte
	unsigned int ui_src_pitch_y_3;		// unit: byte
	unsigned int ui_src_pitch_z_3;		// unit: byte

	unsigned char uc_dst_layout;		// 0: interleaved, 1: semi-planar, 2: planar
	unsigned char uc_dst_itlv_fmt;		// interleaved format (lsb -> msb)
	// YUV420 - 0: UV, 1: VU
	// YUV422 - 0: YUYV, 1: YVYU
	// RGBA - 0: RGBA, 1: ARGB, 2: BGRA, 3: ABGR
	unsigned int ui_dst_addr_0; 		// destination address of channel 0
	unsigned int ui_dst_size_x_0;		// unit: byte
	// ui_dst_size_x_0 <= 536870911
	unsigned int ui_dst_size_y_0;		// unit: line
	// ui_dst_size_y_0 <= 4294967295
	unsigned int ui_dst_size_z_0;		// unit: frame
	// ui_dst_size_z_0 <= 4294967295
	unsigned int ui_dst_size_w_0;		// unit: depth
	// ui_dst_size_w_0 <= 4294967295
	unsigned int ui_dst_pitch_x_0;		// unit: byte
	unsigned int ui_dst_pitch_y_0;		// unit: byte
	unsigned int ui_dst_pitch_z_0;		// unit: byte

	unsigned int ui_dst_addr_1; 		// destination address of channel 1
	unsigned int ui_dst_size_x_1;		// unit: byte
	// ui_dst_size_x_1 <= 536870911
	unsigned int ui_dst_size_y_1;		// unit: line
	// ui_dst_size_y_1 <= 4294967295
	unsigned int ui_dst_size_z_1;		// unit: frame
	// ui_dst_size_z_1 <= 4294967295
	unsigned int ui_dst_size_w_1;		// unit: depth
	// ui_dst_size_w_1 <= 4294967295
	unsigned int ui_dst_pitch_x_1;		// unit: byte
	unsigned int ui_dst_pitch_y_1;		// unit: byte
	unsigned int ui_dst_pitch_z_1;		// unit: byte

	unsigned int ui_dst_addr_2; 		// destination address of channel 2
	unsigned int ui_dst_size_x_2;		// unit: byte
	// ui_dst_size_x_2 <= 536870911
	unsigned int ui_dst_size_y_2;		// unit: line
	// ui_dst_size_y_2 <= 4294967295
	unsigned int ui_dst_size_z_2;		// unit: frame
	// ui_dst_size_z_2 <= 4294967295
	unsigned int ui_dst_size_w_2;		// unit: depth
	// ui_dst_size_w_2 <= 4294967295
	unsigned int ui_dst_pitch_x_2;		// unit: byte
	unsigned int ui_dst_pitch_y_2;		// unit: byte
	unsigned int ui_dst_pitch_z_2;		// unit: byte

	unsigned int ui_dst_addr_3; 		// destination address of channel 3
	unsigned int ui_dst_size_x_3;		// unit: byte
	// ui_dst_size_x_3 <= 536870911
	unsigned int ui_dst_size_y_3;		// unit: line
	// ui_dst_size_y_3 <= 4294967295
	unsigned int ui_dst_size_z_3;		// unit: frame
	// ui_dst_size_z_3 <= 4294967295
	unsigned int ui_dst_size_w_3;		// unit: depth
	// ui_dst_size_w_3 <= 4294967295
	unsigned int ui_dst_pitch_x_3;		// unit: byte
	unsigned int ui_dst_pitch_y_3;		// unit: byte
	unsigned int ui_dst_pitch_z_3;		// unit: byte

	// alpha information
	unsigned short us_alpha;			// alpha value

	DescType descType;
} st_edmaDescInfoType5, st_edmaDescInfoColorCVT;
#pragma pack()

#pragma pack(1)
typedef struct _edmaDescInfoRotate_ {
	unsigned char uc_desc_type; 		// descriptor type 1: 64 bytes for HW descriptor
	unsigned char uc_desc_id;			// descriptor id
	unsigned char uc_fmt;				// format
	// 90: rotate angle 90
	// 91: rotate angle 180
	// 92: rotate angle 270

	unsigned int ui_src_addr_0; 		// source address of channel 0
	unsigned int ui_src_size_x; 		// unit: byte
	// ui_src_size_x <= 2147483647
	unsigned int ui_src_size_y; 		// unit: line
	// ui_src_size_y <= 4294967295
	unsigned int ui_src_size_z; 		// unit: frame
	// ui_src_size_z <= 4294967295
	unsigned int ui_src_size_w; 		// unit: depth
	// ui_src_size_w <= 4294967295
	unsigned int ui_src_pitch_x_0;		// unit: byte
	unsigned int ui_src_pitch_y_0;		// unit: byte
	unsigned int ui_src_pitch_z_0;		// unit: byte

	unsigned int ui_dst_addr_0; 		// destination address of channel 0
	unsigned int ui_dst_size_x; 		// unit: byte
	// ui_dst_size_x <=4294967295
	unsigned int ui_dst_size_y; 		// unit: line
	// ui_dst_size_y <= 4294967295
	unsigned int ui_dst_size_z; 		// unit: frame
	// ui_dst_size_z <= 4294967295
	unsigned int ui_dst_size_w; 		// unit: depth
	// ui_dst_size_w <= 4294967295
	unsigned int ui_dst_pitch_x_0;		// unit: byte
	unsigned int ui_dst_pitch_y_0;		// unit: byte
	unsigned int ui_dst_pitch_z_0;		// unit: byte

	// Pixel information
	unsigned char pixel_size;			// pixel size 1~4 bytes
} st_edmaDescInfoRotate;
#pragma pack()

#pragma pack(1)
typedef struct _edmaDescInfoSplitN_ {
	unsigned char uc_desc_type; 		// descriptor type 1: 64 bytes for HW descriptor
	unsigned char uc_desc_id;			// descriptor id
	unsigned char uc_fmt;				// format
	// 93: split 2
	// 94: split 3
	// 95: split 4

	unsigned int ui_src_addr_0; 		// source address of channel 0
	unsigned int ui_src_size_x_0;		// unit: byte
	// ui_src_size_x <= 2147483647
	unsigned int ui_src_size_y_0;		// unit: line
	// ui_src_size_y <= 4294967295
	unsigned int ui_src_size_z_0;		// unit: frame
	// ui_src_size_z <= 4294967295
	unsigned int ui_src_size_w_0;		// unit: depth
	// ui_src_size_w <= 4294967295
	unsigned int ui_src_pitch_x_0;		// unit: byte
	unsigned int ui_src_pitch_y_0;		// unit: byte
	unsigned int ui_src_pitch_z_0;		// unit: byte

	unsigned int ui_dst_addr_0; 		// destination address of channel 0
	unsigned int ui_dst_size_x_0;		  // unit: byte
	// ui_dst_size_x <=4294967295
	unsigned int ui_dst_size_y_0;		  // unit: line
	// ui_dst_size_y <= 4294967295
	unsigned int ui_dst_size_z_0;		  // unit: frame
	// ui_dst_size_z <= 4294967295
	unsigned int ui_dst_size_w_0;		  // unit: depth
	// ui_dst_size_w <= 4294967295
	unsigned int ui_dst_pitch_x_0;		// unit: byte
	unsigned int ui_dst_pitch_y_0;		// unit: byte
	unsigned int ui_dst_pitch_z_0;		// unit: byte

	unsigned int ui_dst_addr_1; 		// destination address of channel 0
	unsigned int ui_dst_size_x_1;		  // unit: byte
	// ui_dst_size_x <=4294967295
	unsigned int ui_dst_size_y_1;		  // unit: line
	// ui_dst_size_y <= 4294967295
	unsigned int ui_dst_size_z_1;		  // unit: frame
	// ui_dst_size_z <= 4294967295
	unsigned int ui_dst_size_w_1;		  // unit: depth
	// ui_dst_size_w <= 4294967295
	unsigned int ui_dst_pitch_x_1;		// unit: byte
	unsigned int ui_dst_pitch_y_1;		// unit: byte
	unsigned int ui_dst_pitch_z_1;		// unit: byte

	unsigned int ui_dst_addr_2; 		// destination address of channel 0
	unsigned int ui_dst_size_x_2;		  // unit: byte
	// ui_dst_size_x <=4294967295
	unsigned int ui_dst_size_y_2;		  // unit: line
	// ui_dst_size_y <= 4294967295
	unsigned int ui_dst_size_z_2;		  // unit: frame
	// ui_dst_size_z <= 4294967295
	unsigned int ui_dst_size_w_2;		  // unit: depth
	// ui_dst_size_w <= 4294967295
	unsigned int ui_dst_pitch_x_2;		// unit: byte
	unsigned int ui_dst_pitch_y_2;		// unit: byte
	unsigned int ui_dst_pitch_z_2;		// unit: byte

	unsigned int ui_dst_addr_3; 		// destination address of channel 0
	unsigned int ui_dst_size_x_3;		  // unit: byte
	// ui_dst_size_x <=4294967295
	unsigned int ui_dst_size_y_3;		  // unit: line
	// ui_dst_size_y <= 4294967295
	unsigned int ui_dst_size_z_3;		  // unit: frame
	// ui_dst_size_z <= 4294967295
	unsigned int ui_dst_size_w_3;		  // unit: depth
	// ui_dst_size_w <= 4294967295
	unsigned int ui_dst_pitch_x_3;		// unit: byte
	unsigned int ui_dst_pitch_y_3;		// unit: byte
	unsigned int ui_dst_pitch_z_3;		// unit: byte

	// Pixel information
	unsigned char output_mask;			// bit[3:0] for DST 3 ~ 0, 1: enable, 0: disable
										// bit[3:0] = 0b'0000 or 0b'1111 means all enable
	unsigned char pixel_size;			// pixel size 1~4 bytes
} st_edmaDescInfoSplitN;
#pragma pack()

#pragma pack(1)
typedef struct _edmaDescInfoMergeN_ {
	unsigned char uc_desc_type; 		// descriptor type 1: 64 bytes for HW descriptor
	unsigned char uc_desc_id;			// descriptor id
	unsigned char uc_fmt;				// format
	// 96: merge 2
	// 97: merge 3
	// 98: merge 4

	unsigned int ui_src_addr_0; 		// source address of channel 0
	unsigned int ui_src_size_x_0;		// unit: byte
	// ui_src_size_x <= 2147483647
	unsigned int ui_src_size_y_0;		// unit: line
	// ui_src_size_y <= 4294967295
	unsigned int ui_src_size_z_0;		// unit: frame
	// ui_src_size_z <= 4294967295
	unsigned int ui_src_size_w_0;		// unit: depth
	// ui_src_size_w <= 4294967295
	unsigned int ui_src_pitch_x_0;		// unit: byte
	unsigned int ui_src_pitch_y_0;		// unit: byte
	unsigned int ui_src_pitch_z_0;		// unit: byte

	unsigned int ui_src_addr_1; 		// source address of channel 0
	unsigned int ui_src_size_x_1;		// unit: byte
	// ui_src_size_x <=4294967295
	unsigned int ui_src_size_y_1;		// unit: line
	// ui_src_size_y <= 4294967295
	unsigned int ui_src_size_z_1;		// unit: frame
	// ui_src_size_z <= 4294967295
	unsigned int ui_src_size_w_1;		// unit: depth
	// ui_src_size_w <= 4294967295
	unsigned int ui_src_pitch_x_1;		// unit: byte
	unsigned int ui_src_pitch_y_1;		// unit: byte
	unsigned int ui_src_pitch_z_1;		// unit: byte

	unsigned int ui_src_addr_2; 		// source address of channel 0
	unsigned int ui_src_size_x_2;		// unit: byte
	// ui_src_size_x <=4294967295
	unsigned int ui_src_size_y_2;		// unit: line
	// ui_src_size_y <= 4294967295
	unsigned int ui_src_size_z_2;		// unit: frame
	// ui_src_size_z <= 4294967295
	unsigned int ui_src_size_w_2;		// unit: depth
	// ui_src_size_w <= 4294967295
	unsigned int ui_src_pitch_x_2;		// unit: byte
	unsigned int ui_src_pitch_y_2;		// unit: byte
	unsigned int ui_src_pitch_z_2;		// unit: byte

	unsigned int ui_src_addr_3; 		// source address of channel 0
	unsigned int ui_src_size_x_3;		// unit: byte
	// ui_src_size_x <=4294967295
	unsigned int ui_src_size_y_3;		// unit: line
	// ui_src_size_y <= 4294967295
	unsigned int ui_src_size_z_3;		// unit: frame
	// ui_src_size_z <= 4294967295
	unsigned int ui_src_size_w_3;		// unit: depth
	// ui_src_size_w <= 4294967295
	unsigned int ui_src_pitch_x_3;		// unit: byte
	unsigned int ui_src_pitch_y_3;		// unit: byte
	unsigned int ui_src_pitch_z_3;		// unit: byte

	unsigned int ui_dst_addr_0; 		// destination address of channel 0
	unsigned int ui_dst_size_x_0;		// unit: byte
										// ui_dst_size_x <=4294967295
	unsigned int ui_dst_size_y_0;		// unit: line
										// ui_dst_size_y <= 4294967295
	unsigned int ui_dst_size_z_0;		// unit: frame
										// ui_dst_size_z <= 4294967295
	unsigned int ui_dst_size_w_0;		// unit: depth
										// ui_dst_size_w <= 4294967295
	unsigned int ui_dst_pitch_x_0;		// unit: byte
	unsigned int ui_dst_pitch_y_0;		// unit: byte
	unsigned int ui_dst_pitch_z_0;		// unit: byte

	// Pixel information
	unsigned char pixel_size;			// pixel size 1~4 bytes
} st_edmaDescInfoMergeN;
#pragma pack()

#pragma pack(1)
typedef struct _edmaDescInfoUnpack_ {
	unsigned char uc_desc_type; 		// descriptor type 1: 64 bytes for HW descriptor
	unsigned char uc_desc_id;			// descriptor id
	unsigned char uc_fmt;				// format
	//	99: unpack from  8 bit to 16 bit
	// 100: unpack from 10 bit to 16 bit
	// 101: unpack from 12 bit to 16 bit
	// 102: unpack from 14 bit to 16 bit

	unsigned int ui_src_addr_0; 		// source address of channel 0
	unsigned int ui_src_size_x_0;		  // unit: byte
	// ui_src_size_x <= 2147483647
	unsigned int ui_src_size_y_0;		  // unit: line
	// ui_src_size_y <= 4294967295
	unsigned int ui_src_size_z_0;		  // unit: frame
	// ui_src_size_z <= 4294967295
	unsigned int ui_src_size_w_0;		  // unit: depth
	// ui_src_size_w <= 4294967295
	unsigned int ui_src_pitch_x_0;		// unit: byte
	unsigned int ui_src_pitch_y_0;		// unit: byte
	unsigned int ui_src_pitch_z_0;		// unit: byte

	unsigned int ui_dst_addr_0; 		// destination address of channel 0
	unsigned int ui_dst_size_x_0;		  // unit: byte
	// ui_dst_size_x <=4294967295
	unsigned int ui_dst_size_y_0;		  // unit: line
	// ui_dst_size_y <= 4294967295
	unsigned int ui_dst_size_z_0;		  // unit: frame
	// ui_dst_size_z <= 4294967295
	unsigned int ui_dst_size_w_0;		  // unit: depth
	// ui_dst_size_w <= 4294967295
	unsigned int ui_dst_pitch_x_0;		// unit: byte
	unsigned int ui_dst_pitch_y_0;		// unit: byte
	unsigned int ui_dst_pitch_z_0;		// unit: byte

	unsigned char uc_pad_pos;			// EDMA_PAD_POS_HIGH, EDMA_PAD_POS_LOW
} st_edmaDescInfoUnpack;
#pragma pack()

#pragma pack(1)
typedef struct _edmaDescInfoPack_ {
	unsigned char uc_desc_type; 		// descriptor type 1: 64 bytes for HW descriptor
	unsigned char uc_desc_id;			// descriptor id
	unsigned char uc_fmt;				// format
	// 103: pack from 16 bit to 10 bit
	// 104: pack from 16 bit to 12 bit

	unsigned int ui_src_addr_0; 		// source address of channel 0
	unsigned int ui_src_size_x_0;		// unit: byte
	// ui_src_size_x <= 2147483647
	unsigned int ui_src_size_y_0;		// unit: line
	// ui_src_size_y <= 4294967295
	unsigned int ui_src_size_z_0;		// unit: frame
	// ui_src_size_z <= 4294967295
	unsigned int ui_src_size_w_0;		// unit: depth
	// ui_src_size_w <= 4294967295
	unsigned int ui_src_pitch_x_0;		// unit: byte
	unsigned int ui_src_pitch_y_0;		// unit: byte
	unsigned int ui_src_pitch_z_0;		// unit: byte

	unsigned int ui_dst_addr_0; 		// destination address of channel 0
	unsigned int ui_dst_size_x_0;		// unit: byte
	// ui_dst_size_x <=4294967295
	unsigned int ui_dst_size_y_0;		// unit: line
	// ui_dst_size_y <= 4294967295
	unsigned int ui_dst_size_z_0;		// unit: frame
	// ui_dst_size_z <= 4294967295
	unsigned int ui_dst_size_w_0;		// unit: depth
	// ui_dst_size_w <= 4294967295
	unsigned int ui_dst_pitch_x_0;		// unit: byte
	unsigned int ui_dst_pitch_y_0;		// unit: byte
	unsigned int ui_dst_pitch_z_0;		// unit: byte

	unsigned char uc_extract_pos;		// EDMA_EXTRACT_POS_HIGH, EDMA_EXTRACT_POS_LOW
} st_edmaDescInfoPack;
#pragma pack()

#pragma pack(1)
typedef struct _edmaDescInfoSwap2_ {
	unsigned char uc_desc_type; 		// descriptor type 1: 64 bytes for HW descriptor
	unsigned char uc_desc_id;			// descriptor id
	unsigned char uc_fmt;				// format, NO USED

	unsigned int ui_src_addr_0; 		// source address of channel 0
	unsigned int ui_src_size_x_0;		  // unit: byte
	// ui_src_size_x <= 2147483647
	unsigned int ui_src_size_y_0;		  // unit: line
	// ui_src_size_y <= 4294967295
	unsigned int ui_src_size_z_0;		  // unit: frame
	// ui_src_size_z <= 4294967295
	unsigned int ui_src_size_w_0;		  // unit: depth
	// ui_src_size_w <= 4294967295
	unsigned int ui_src_pitch_x_0;		// unit: byte
	unsigned int ui_src_pitch_y_0;		// unit: byte
	unsigned int ui_src_pitch_z_0;		// unit: byte

	unsigned int ui_dst_addr_0; 		// destination address of channel 0
	unsigned int ui_dst_size_x_0;		  // unit: byte
	// ui_dst_size_x <=4294967295
	unsigned int ui_dst_size_y_0;		  // unit: line
	// ui_dst_size_y <= 4294967295
	unsigned int ui_dst_size_z_0;		  // unit: frame
	// ui_dst_size_z <= 4294967295
	unsigned int ui_dst_size_w_0;		  // unit: depth
	// ui_dst_size_w <= 4294967295
	unsigned int ui_dst_pitch_x_0;		// unit: byte
	unsigned int ui_dst_pitch_y_0;		// unit: byte
	unsigned int ui_dst_pitch_z_0;		// unit: byte

	// swap information
	unsigned char swap_size;		   // swap size 1~2 bytes
} st_edmaDescInfoSwap2;
#pragma pack()

#pragma pack(1)
typedef struct _edmaDescInfo565To888_ {
	unsigned char uc_desc_type; 		// descriptor type 1: 64 bytes for HW descriptor
	unsigned char uc_desc_id;			// descriptor id
	unsigned char uc_fmt;				// format, EDMA_FMT_565_TO_888 and EDMA_FMT_565_TO_8888

	unsigned int ui_src_addr_0; 		// source address of channel 0
	unsigned int ui_src_size_x_0;		  // unit: byte
	// ui_src_size_x <= 2147483647
	unsigned int ui_src_size_y_0;		  // unit: line
	// ui_src_size_y <= 4294967295
	unsigned int ui_src_size_z_0;		  // unit: frame
	// ui_src_size_z <= 4294967295
	unsigned int ui_src_size_w_0;		  // unit: depth
	// ui_src_size_w <= 4294967295
	unsigned int ui_src_pitch_x_0;		// unit: byte
	unsigned int ui_src_pitch_y_0;		// unit: byte
	unsigned int ui_src_pitch_z_0;		// unit: byte

	unsigned int ui_dst_addr_0; 		// destination address of channel 0
	unsigned int ui_dst_size_x_0;		  // unit: byte
	// ui_dst_size_x <=4294967295
	unsigned int ui_dst_size_y_0;		  // unit: line
	// ui_dst_size_y <= 4294967295
	unsigned int ui_dst_size_z_0;		  // unit: frame
	// ui_dst_size_z <= 4294967295
	unsigned int ui_dst_size_w_0;		  // unit: depth
	// ui_dst_size_w <= 4294967295
	unsigned int ui_dst_pitch_x_0;		// unit: byte
	unsigned int ui_dst_pitch_y_0;		// unit: byte
	unsigned int ui_dst_pitch_z_0;		// unit: byte

	// alpha information
	unsigned char uc_dst_alpha_pos; 	// 0: high part, 1: low part
	unsigned short us_alpha;			// alpha value
} st_edmaDescInfo565To888;
#pragma pack()

#pragma pack(1)

typedef struct _edmaDescInfoPad {
    uint8_t uc_mode;                // 0: constant, 1: edge (see EDMA_PADDING_MODE)
    uint8_t uc_desc_id;             // descriptor id
    uint8_t uc_elem_size;           // element size (in bytes)
    uint8_t reserved;

    // input size
    uint32_t ui_src_size_x;         // input X size (in pixels)
    uint32_t ui_src_size_y;         // input Y size (in pixels)

    uint32_t ui_paddings_before_x;  // how many pixels to add before X dimension
    uint32_t ui_paddings_after_x;   // how many pixels to add after X dimension
    uint32_t ui_paddings_before_y;  // how many pixels to add before Y dimension
    uint32_t ui_paddings_after_y;   // how many pixels to add after Y dimension

    uint32_t ui_src_stride_x;       // input X stride (in pixels)
    uint32_t ui_src_stride_y;       // input Y stride (in pixels)
    uint32_t ui_dst_stride_x;       // output X stride (in pixels)
    uint32_t ui_dst_stride_y;       // output Y stride (in pixels)

    uint32_t ui_pad_constant;       // value for pad (used in EDMA_CONSTANT_PADDING only)
    uint32_t ui_src_addr;           // source address
    uint32_t ui_dst_addr;           // destination address
} st_edmaDescInfoPad;

#pragma pack()

#pragma pack(1)

typedef struct _edmaDescInfoBayerTransform {
    uint8_t uc_desc_id;             // descriptor id
    // input and output bits, support bayer10 or bayer12 to RGGB 16
    uint8_t uc_in_bits;
    uint8_t uc_out_bits;
    uint8_t reserved;

    // input shape
    uint32_t ui_src_size_x;         // input X size (in pixels, one RG or GB pair is one pixel)
    uint32_t ui_src_size_y;         // input Y size (in pixels)
    uint32_t ui_src_stride_x;       // input X stride (in bytes)
    uint32_t ui_src_stride_y;       // input Y stride (in bytes)

    // output shape
    uint32_t ui_dst_size_x;         // output X size (in pixels, one RGGB pair is one pixel)
    uint32_t ui_dst_size_y;         // output Y size (in pixels)
    uint32_t ui_dst_stride_x;       // output X stride (in bytes)
    uint32_t ui_dst_stride_y;       // output Y stride (in bytes)

    uint32_t ui_src_addr;           // source address
    uint32_t ui_dst_addr;           // destination address
} st_edmaDescInfoBayer;

#pragma pack()

#pragma pack(4)

typedef struct edmaUnifyInfo
{
    EDMA_INFO_TYPE info_type;               // info type for generating related hw descriptor
    EdmaMemoryBindings* inputBindings;      // binding offsets for input buffer(s) in descriptor(s)
    EdmaMemoryBindings* outputBindings;     // binding offsets for output buffer(s) in descriptor(s)
    union info
    {
        st_edmaDescInfoType0 info0;
        st_edmaDescInfoType1 info1;
        st_edma_InputShape infoShape;
        st_edma_info_nn infoNN;
        st_edma_InfoData infoData;
        st_edma_InfoSDK infoSDK;
        st_edmaDescInfoColorCVT infoColorCVT;
        st_edmaDescInfoType5 info5;
        st_edmaDescInfoRotate infoRotate;
        st_edmaDescInfoSplitN infoSplitN;
        st_edmaDescInfoMergeN infoMergeN;
        st_edmaDescInfoUnpack infoUnpack;
        st_edmaDescInfoSwap2 infoSwap2;
        st_edmaDescInfo565To888 info565To888;
        st_edmaDescInfoPack infoPack;
        st_edmaDescInfoPad infoPad;
        st_edmaDescInfoBayer infoBayer;
    } st_info;
} st_edmaUnifyInfo;

#pragma pack()


#pragma pack(4)

typedef struct _edmaTaskInfo_
{
	unsigned int info_num;	 //	info number == number if edma_info
	st_edmaUnifyInfo *edma_info;	//
#ifdef MVPU_EN
	bool is_binary_record;	 // assigned in PatternEngine, see also: cl_batch_info.h
#endif
} st_edmaTaskInfo;

#pragma pack()

#endif // EDMA_INFO_H_
