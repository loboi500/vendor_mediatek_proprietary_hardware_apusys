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


int edma_querySDKDNum(unsigned char type, st_edmaUnifyInfo *shapeInfo)
{
	EDMA_LOG_NONE("%s type = %d, info_type = %d \n", __func__, type, shapeInfo->info_type);
	return 1; //not support type
}

//descriptor size per info
int edma_querySDKDSize(unsigned char type, st_edmaUnifyInfo *shapeInfo)
{
	EDMA_LOG_NONE("%s type = %d, info_type = %d \n", __func__, type, shapeInfo->info_type);
	return sizeof(st_edmaDesc20);
}

/*for edma 2.0*/
int fillDescSDK(st_edmaUnifyInfo *pInfo, void *edma_desc)
{
	st_edmaDesc20 *pDesc = (st_edmaDesc20 *)edma_desc;

	st_edma_InfoSDK *pAISDK = &pInfo->st_info.infoSDK;
	st_sdk_typeGnl *pAICAR = (st_sdk_typeGnl *)pAISDK->pSdkInfo;

	memset(pDesc, 0, sizeof(st_edmaDesc20));

	EDMA_LOG_INFO("fillDescSDK st_edmaDesc20 size = %zu\n", sizeof(st_edmaDesc20));
	EDMA_LOG_INFO("sdk_type = %d\n", pAISDK->sdk_type);
	EDMA_LOG_INFO("shape ui_desp_00_type = %d\n", pAICAR->ui_desp_00_type);


	pDesc->DespDmaAruser = 0x2;
	pDesc->DespDmaAwuser = 0x2;
	pDesc->DespIntEnable = 1;
    /* fill related descriptor here
	*/

	return 0;

}


