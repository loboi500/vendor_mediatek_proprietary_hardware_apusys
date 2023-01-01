#ifndef EDMA_CORE_H_
#define EDMA_CORE_H_

#include "edma_info.h"
//#include "edma_core_hw.h"

//int edma_queryDSize(unsigned char type, void *shapeInfo);

int edma_queryDSize(unsigned char type, st_edmaUnifyInfo *shapeInfo);

int edma_queryDNum(unsigned char type, st_edmaUnifyInfo *shapeInfo);

int edma_queryUFBCDSize(st_edmaUnifyInfo *shapeInfo);

int edma_querySliceDSize(st_edmaUnifyInfo *shapeInfo);

int edma_queryPadDNum(st_edmaUnifyInfo *shapeInfo);

int edma_queryPadDSize(st_edmaUnifyInfo *shapeInfo);

int edma_queryBayerToRGGBDSize(st_edmaUnifyInfo *shapeInfo);

int edma_queryRGGBToBayerDSize(st_edmaUnifyInfo *shapeInfo);

int fillDescV0(st_edmaUnifyInfo *pInfo, void *currDesc, void *headDesc);

int fillDescNN(st_edmaUnifyInfo *shape, void *currDesc, void *headDesc);

int fillDescData(st_edmaUnifyInfo *pInfo, void *currDesc, void *headDesc);

//int fillDescType0(st_edmaUnifyInfo *pInfo, void *edma_desc);

//int fillDescType1(st_edmaUnifyInfo *pInfo, void *edma_desc);

int fillDescUFBC(st_edmaUnifyInfo *pInfo, void *currDesc, void *headDesc);

int fillDescSlice(st_edmaUnifyInfo *pInfo, void *currDesc, void *headDesc);

int fillDescPad(st_edmaUnifyInfo *pInfo, void *currDesc, void *headDesc);

int fillDescBayerToRGGB(st_edmaUnifyInfo *pInfo, void *currDesc, void *headDesc);

int fillDescRGGBToBayer(st_edmaUnifyInfo *pInfo, void *currDesc, void *headDesc);

void edma_MVPUtransTaskInfo(st_edmaTaskInfo *pst_old_edma_task_info, st_edmaTaskInfo *pst_new_edma_task_info);

#ifdef MVPU_EN
void fillDescType0(st_edmaDescInfoType0 *pst_edma_desc_info, st_edmaDescType0 *pst_edma_desc, int *pi_errorCode);
void fillDescType1(st_edmaDescInfoType1 *pst_edma_desc_info, st_edmaDescType1 *pst_edma_desc, int *pi_errorCode);
void fillDescType5(st_edmaDescInfoType5 *pst_edma_desc_info, st_edmaDescType5 *pst_edma_desc, int *pi_errorCode);

void fillDescRotOutBufIn(st_edmaDescInfoRotate *pst_edma_desc_info, st_edmaDescType0 *pst_edma_desc, int *pi_errorCode);
void fillDescRotate90(st_edmaDescInfoRotate *pst_edma_desc_info, st_edmaDescType0 *pst_edma_desc, int *pi_errorCode);
void fillDescRotate180(st_edmaDescInfoRotate *pst_edma_desc_info, st_edmaDescType15 *pst_edma_desc, int *pi_errorCode);
void fillDescRotate270(st_edmaDescInfoRotate *pst_edma_desc_info, st_edmaDescType0 *pst_edma_desc, int *pi_errorCode);
void fillDescRotInBufOut(st_edmaDescInfoRotate *pst_edma_desc_info, st_edmaDescType0 *pst_edma_desc, int *pi_errorCode);

void fillDescSplitNIn(st_edmaDescInfoSplitN *pst_edma_desc_info, st_edmaDescType0 *pst_edma_desc, int *pi_errorCode);
void fillDescSplitN(st_edmaDescInfoSplitN *pst_edma_desc_info, st_edmaDescType15 *pst_edma_desc, int *pi_errorCode);
void fillDescSplitNOut(st_edmaDescInfoSplitN *pst_edma_desc_info, st_edmaDescType15 *pst_edma_desc, int *pi_errorCode);

void fillDescMergeNIn(st_edmaDescInfoMergeN *pst_edma_desc_info, st_edmaDescType15 *pst_edma_desc, int *pi_errorCode);
void fillDescMergeN(st_edmaDescInfoMergeN *pst_edma_desc_info, st_edmaDescType15 *pst_edma_desc, int *pi_errorCode);
void fillDescMergeNOut(st_edmaDescInfoMergeN *pst_edma_desc_info, st_edmaDescType0 *pst_edma_desc, int *pi_errorCode);

void fillDescUnpack(st_edmaDescInfoUnpack *pst_edma_desc_info, st_edmaDescType0 *pst_edma_desc, int *pi_errorCode);

void fillDescSwap2(st_edmaDescInfoSwap2 *pst_edma_desc_info, st_edmaDescType15 *pst_edma_desc, int *pi_errorCode);

void fillDesc565To888(st_edmaDescInfo565To888 *pst_edma_desc_info, st_edmaDescType15 *pst_edma_desc, int *pi_errorCode);

void fillDescPack(st_edmaDescInfoPack *pst_edma_desc_info, st_edmaDescType15 *pst_edma_desc, int *pi_errorCode);

EDMA_INFO_TYPE checkMVPUDescrpType(st_edmaUnifyInfo *shapeInfo);

void edma_MVPUtransTaskInfo(st_edmaTaskInfo *pst_old_edma_task_info, st_edmaTaskInfo *pst_new_edma_task_info);

unsigned int edma_queryMVPUDSize(st_edmaUnifyInfo *shapeInfo);

unsigned int edma_queryMVPUDNum(st_edmaUnifyInfo *in_info);

unsigned int edma_queryMVPURPNum(st_edmaUnifyInfo *in_info);
#endif

int edma_querySDKDNum(unsigned char type, st_edmaUnifyInfo *shapeInfo);

//descriptor size per info
int edma_querySDKDSize(unsigned char type, st_edmaUnifyInfo *shapeInfo);

/*for edma 2.0*/
int fillDescSDK(st_edmaUnifyInfo *pInfo, void *edma_desc);

int edmav20_queryDSize(unsigned char type, st_edmaUnifyInfo *shapeInfo);

int edmav20_queryDSize(unsigned char type, st_edmaUnifyInfo *shapeInfo);

int edmav20_queryDNum(unsigned char type, st_edmaUnifyInfo *shapeInfo);

int edmav20_querySliceDSize(st_edmaUnifyInfo *shapeInfo);

//descriptor size per info
int edmav20_queryUFBCDSize(st_edmaUnifyInfo *shapeInfo);

/*for edma 2.0*/
int fillDescV20(st_edmaUnifyInfo *pInfo, void *edma_desc);

int fillDescDataV20(st_edmaUnifyInfo *pInfo, void *edma_desc);

int fillDescUFBCV20(st_edmaUnifyInfo *pInfo, void *edma_desc, void *headDesc);


#endif // EDMA_CORE_H_
