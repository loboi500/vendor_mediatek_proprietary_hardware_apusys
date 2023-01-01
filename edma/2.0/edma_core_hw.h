#ifndef __APUSYS_EDMA_CORE_HW_H__
#define __APUSYS_EDMA_CORE_HW_H__

#define EDMA_REQUEST_NAME_SIZE 32

#pragma pack(1)
typedef struct _edma20Desc_
{
	// eDMA 2.0 descriptor
	//  0x800
	unsigned int DespSrcXSizeM1:16;
	unsigned int DespSrcYSizeM1:16;

	// 0x804: APU_EDMA3_DESP_04
	unsigned int DespSrcZSizeM1:16;
	unsigned int Desp4Reserved:16; //24-31

	// 0x808: APU_EDMA3_DESP_08
	unsigned int DespSrcXStride0:32;

	// 0x80C: APU_EDMA3_DESP_0C
	unsigned int DespSrcYStride0:32;

	// 0x10: APU_EDMA3_DESP_10
	unsigned int DespSrcXStride1:32;

	// 0x14: APU_EDMA3_DESP_14
	unsigned int DespSrcYStride1:32;

	// 0x18: APU_EDMA3_DESP_18
	unsigned int DespDstXSizeM1:16;
	unsigned int DespDstYSizeM1:16;

	// 0x1C: APU_EDMA3_DESP_1C
	unsigned int DespDstXStride0:32;

	// 0x20: APU_EDMA3_DESP_20
	unsigned int DespDstYStride0:32;

	// 0x24: APU_EDMA3_DESP_24
	unsigned int DespDstXStride1:32;

	// 0x28: APU_EDMA3_DESP_28
	unsigned int DespDstYStride1:32;

	// 0x2C: APU_EDMA3_DESP_2C
	unsigned int DespSrcAddr0:32;

	// 0x30: APU_EDMA3_DESP_30
	unsigned int DespSrcAddr1:32;

	// 0x34: APU_EDMA3_DESP_34
	unsigned int DespDstAddr0:32;

	// 0x38: APU_EDMA3_DESP_38
	unsigned int DespDstAddr1:32;

	// 0x3c: APU_EDMA3_DESP_3C
	unsigned int DespCmprsSrcPxl:16;
	unsigned int DespCmprsDstPxl:16; //16-27

	// 0x40: APU_EDMA3_DESP_40
	unsigned int DespDmaAruser:5;
	unsigned int DespDmaAwuser:5;
	unsigned int DespDmaArdomain:5;
	unsigned int DespDmaAwdomain:5;

	unsigned int DespPmuEnable:1;
	unsigned int DespPmuCntIndex:2;
	unsigned int D40Reserved:5;
	unsigned int DespIntEnable:4; //28


	// 0x44: APU_EDMA3_DESP_44cl3
	unsigned int DespInFormat:4;
	unsigned int DespInSwap:1;
	unsigned int DespInOnlyYuv:2;
	unsigned int DespInCmprsSel:1;
	unsigned int DespColrConvtFilter:1;
	unsigned int DespDropPt:1;
	unsigned int Desp0InASwap:1;
	unsigned int Desp0RawUnpackRule:1;
	unsigned int Desp0RawUnpackShift:2;
	unsigned int D44Reserved:2;	
	unsigned int DespOutFormat:4;
	unsigned int DespOutFillMode:1;
	unsigned int UfbcYuvTransform:1;
	unsigned int DespRawEnable:1;
	unsigned int DespRawPlaneNum:1;
	unsigned int DespId:8;

	// 0x48: APU_EDMA3_DESP_44
	unsigned int DespParamA:32;

	// 0x4C: APU_EDMA3_DESP_38
	unsigned int DespParamM:32;

	// 0x50: APU_EDMA3_DESP_38
	unsigned int DespSrcXStridePxl:15;
	unsigned int D50Reserved0:1;
	unsigned int DespSrcYStridePxl:15;
	unsigned int D50Reserved1:1;

	// 0x54: APU_EDMA3_DESP_38
	unsigned int DespSrcXOffsetM1:15;
	unsigned int D54Reserved0:1;
	unsigned int DespSrcYOffsetM1:15;
	unsigned int D54Reserved1:1;

	// 0x58: APU_EDMA3_DESP_38
	unsigned int DespDstXStridePxl:15;
	unsigned int D58Reserved0:1;
	unsigned int DespDstYStridePxl:15;
	unsigned int D58Reserved1:1;

	// 0x5C: APU_EDMA3_DESP_38
	unsigned int DespDstXOffsetM1:15;
	unsigned int D5CReserved0:1;
	unsigned int DespDstYOffsetM1:15;
	unsigned int D5CReserved1:1;

} st_edmaDesc20;
#pragma pack()


#endif
