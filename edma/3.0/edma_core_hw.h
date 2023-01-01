#ifndef __APUSYS_EDMA_CORE_HW_H__
#define __APUSYS_EDMA_CORE_HW_H__


/*edma 30 type 0*/

#pragma pack(1)
typedef struct _edmaDescType0_
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
	unsigned int ui_desp_04_raw_shift:4;
	unsigned int ui_desp_04_reserved_2:12;

	// 0x08: APU_EDMA3_DESP_08
	unsigned int ui_desp_08_fill_const:32;

	// 0x0C: APU_EDMA3_DESP_0C
	unsigned int ui_desp_0c_i2f_min:32;

	// 0x10: APU_EDMA3_DESP_10
	unsigned int ui_desp_10_i2f_scale:32;

	// 0x14: APU_EDMA3_DESP_14
	unsigned int ui_desp_14_f2i_min:32;

	// 0x18: APU_EDMA3_DESP_18
	unsigned int ui_desp_18_f2i_scale:32;

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
} st_edmaDescType0;
#pragma pack()


#pragma pack(1)
typedef struct _edmaDescType1_
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
	unsigned int ui_desp_0c_reserved_0:32;

	// 0x10: APU_EDMA3_DESP_10
	unsigned int ui_desp_10_reserved_0:32;

	// 0x14: APU_EDMA3_DESP_14
	unsigned int ui_desp_14_reserved_0:32;

	// 0x18: APU_EDMA3_DESP_18
	unsigned int ui_desp_18_reserved_0:32;

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
} st_edmaDescType1;
#pragma pack()



#pragma pack(1)

typedef struct _edmaDescType2_
{
	// eDMA 3.5 descriptor
	unsigned int ui_desp_00_type                      : 4;
	unsigned int ui_desp_00_aruser                    : 2;
	unsigned int ui_desp_00_awuser                    : 2;
	unsigned int ui_desp_00_des_id                    : 8;
	unsigned int ui_desp_00_rsv_16                    : 2;
	unsigned int ui_desp_00_src_0_addr_cbfc_extension : 7;
	unsigned int ui_desp_00_dst_0_addr_cbfc_extension : 7;


	unsigned int ui_desp_04_format                    : 6;
	unsigned int ui_desp_04_rsv_6                     : 2;
	unsigned int ui_desp_04_safe_mode                 : 1;
	unsigned int ui_desp_04_atomic_mode               : 1;
	unsigned int ui_desp_04_read_tmp_buf              : 1;
	unsigned int ui_desp_04_write_tmp_buf             : 1;
	unsigned int ui_desp_04_in_rb_uv_swap             : 1;
	unsigned int ui_desp_04_in_a_swap                 : 1;
	unsigned int ui_desp_04_rsv_14                    : 18;

	// 0x08: APU_EDMA3_DESP_08
	unsigned int descp08_reserved_0:32;

	// 0x0C: APU_EDMA3_DESP_0C
	unsigned int descp0c_reserved_0:32;


	unsigned int ui_desp_10_hse_pro_enroll_vid        : 5;
	unsigned int ui_desp_10_rsv_5                     : 11;
	unsigned int ui_desp_10_hse_pro_enroll_iteration  : 4;
	unsigned int ui_desp_10_rsv_20                    : 12;



	unsigned int ui_desp_14_hse_con_enroll_vid        : 5;
	unsigned int ui_desp_14_rsv_5                     : 11;
	unsigned int ui_desp_14_hse_con_enroll_iteration  : 4;
	unsigned int ui_desp_14_rsv_20                    : 12;


	unsigned int ui_desp_18_hse_con_wait              : 1;
	unsigned int ui_desp_18_rsv_1                     : 31;

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

}st_edmaDescType2;

#pragma pack()

#pragma pack(1)
typedef struct _edmaDescType4_
{
	// eDMA 3.0 descriptor type 4
	// 0x00: APU_EDMA3_DESP_00
	unsigned int descp00_type:4;
	unsigned int descp00_aruser:2;
	unsigned int descp00_awuser:2;
	unsigned int descp00_des_id:8;
	unsigned int descp00_handshake_enable:8;
	unsigned int descp00_pmu_enable:1;
	unsigned int descp00_reserved_0:7;

	// 0x04: APU_EDMA3_DESP_04
	unsigned int descp04_format:6;
	unsigned int descp04_rsv_6:2;
	unsigned int descp04_safe_mode:1;
	unsigned int descp04_atomic_mode:1;
	unsigned int descp04_read_tmp_buf:1;
	unsigned int descp04_write_tmp_buf:1;
	unsigned int descp04_in_rb_uv_swap:1;
	unsigned int descp04_in_a_swap:1;
	unsigned int descp04_rsv_14:2;
	unsigned int descp04_raw_shift:4;
	unsigned int descp04_rsv_20:12;

	// 0x08: APU_EDMA3_DESP_08
	unsigned int descp08_reserved_0:32;

	// 0x0C: APU_EDMA3_DESP_0C
	unsigned int descp0c_reserved_0:32;

	// 0x10: APU_EDMA3_DESP_10
	unsigned int descp10_reserved_0:32;

	// 0x14: APU_EDMA3_DESP_14
	unsigned int descp14_reserved_0:32;

	// 0x18: APU_EDMA3_DESP_18
	unsigned int descp18_reserved_0:32;

	// 0x1C: APU_EDMA3_DESP_1C
	unsigned int descp1c_src_addr_0:32;

	// 0x20: APU_EDMA3_DESP_20
	unsigned int descp20_dst_addr_0:32;

	// 0x24: APU_EDMA3_DESP_24
	unsigned int descp24_src_x_stride_0:32;

	// 0x28: APU_EDMA3_DESP_28
	unsigned int descp28_dst_x_stride_0:32;

	// 0x2C: APU_EDMA3_DESP_2C
	unsigned int descp2c_src_y_stride_0:32;

	// 0x30: APU_EDMA3_DESP_30
	unsigned int descp30_dst_y_stride_0:32;

	// 0x34: APU_EDMA3_DESP_34
	unsigned int descp34_src_x_size_0:16;
	unsigned int descp34_dst_x_size_0:16;

	// 0x38: APU_EDMA3_DESP_38
	unsigned int descp38_src_y_size_0:16;
	unsigned int descp38_dst_y_size_0:16;

	// 0x3c: APU_EDMA3_DESP_3C
	unsigned int descp3c_src_z_size_0:16;
	unsigned int descp3c_dst_z_size_0:16;

	// 0x40: APU_EDMA3_DESP_08
	unsigned int descp40_reserved_0:32;

	// 0x44: APU_EDMA3_DESP_0C
	unsigned int descp44_reserved_0:32;

	// 0x48: APU_EDMA3_DESP_10
	unsigned int descp48_reserved_0:32;

	// 0x4C: APU_EDMA3_DESP_14
	unsigned int descp4c_reserved_0:32;

	// 0x50: APU_EDMA3_DESP_08
	unsigned int descp50_reserved_0:32;

	// 0x54: APU_EDMA3_DESP_0C
	unsigned int descp54_reserved_0:32;

	// 0x58: APU_EDMA3_DESP_10
	unsigned int descp58_reserved_0:32;

	// 0x5C: APU_EDMA3_DESP_1C
	unsigned int descp5c_src_addr_0:32;

	// 0x60: APU_EDMA3_DESP_20
	unsigned int descp60_dst_addr_0:32;

	// 0x64: APU_EDMA3_DESP_24
	unsigned int descp64_src_x_stride_0:32;

	// 0x68: APU_EDMA3_DESP_28
	unsigned int descp68_dst_x_stride_0:32;

	// 0x6C: APU_EDMA3_DESP_2C
	unsigned int descp6c_src_y_stride_0:32;

	// 0x70: APU_EDMA3_DESP_30
	unsigned int descp70_dst_y_stride_0:32;

	// 0x74: APU_EDMA3_DESP_34
	unsigned int descp74_src_x_size_0:16;
	unsigned int descp74_dst_x_size_0:16;

	// 0x78: APU_EDMA3_DESP_38
	unsigned int descp78_src_y_size_0:16;
	unsigned int descp78_dst_y_size_0:16;

	// 0x7c: APU_EDMA3_DESP_3C
	unsigned int descp7c_src_z_size_0:16;
	unsigned int descp7c_dst_z_size_0:16;
} st_edmaDescType4;
#pragma pack()


#pragma pack(1)
typedef struct _edmaDescType5_
{
	// eDMA 3.1 descriptor
	// 0x00: APU_EDMA3_DESP_00
	unsigned int ui_desp_00_type:4;
	unsigned int ui_desp_00_aruser:2;
	unsigned int ui_desp_00_awuser:2;
	unsigned int ui_desp_00_des_id:8;
	unsigned int ui_desp_00_reserved_0:2;
	unsigned int ui_desp_00_src_0_addr_cbfc_extension:7;
	unsigned int ui_desp_00_dst_0_addr_cbfc_extension:7;

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
	unsigned int ui_desp_40_src_1_addr_cbfc_extension:7;
	unsigned int ui_desp_40_dst_1_addr_cbfc_extension:7;

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
} st_edmaDescType5;
#pragma pack()


#pragma pack(1)

typedef struct _edmaDescType15_
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
} st_edmaDescType15;
#pragma pack()

#endif
