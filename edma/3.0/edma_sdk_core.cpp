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
	return sizeof(st_edmaDescType15);
}

/*for edma 3.0*/
int fillDescSDK(st_edmaUnifyInfo *pInfo, void *edma_desc)
{
    st_edmaDescType15 *pDesc = (st_edmaDescType15 *)edma_desc;

    st_edma_InfoSDK *pAISDK = &pInfo->st_info.infoSDK;
    st_sdk_typeGnl *pAICAR = (st_sdk_typeGnl *)pAISDK->pSdkInfo;

    EDMA_LOG_INFO("fillDescSDK st_edmaDescType15 size = %zu\n", sizeof(st_edmaDescType15));

    if (pAISDK->sdk_type == EDMA_SDK_TYPE_GNL) {
            if (pAISDK->pSdkInfo != NULL)
                memcpy(pDesc, pAISDK->pSdkInfo, sizeof(st_edmaDescType15));
            else {
                    EDMA_LOG_ERR("pAISDK->pSdkInfo = null!!\n");
                    return -1;
            }
    }
    EDMA_LOG_INFO("sdk_type = %d\n", pAISDK->sdk_type);
    EDMA_LOG_INFO("shape ui_desp_00_type = %d\n", pAICAR->ui_desp_00_type);


#ifdef DISABLE_IOMMU
    pDesc->ui_desp_00_aruser = 0;
    pDesc->ui_desp_00_awuser = 0;
#else
    pDesc->ui_desp_00_aruser = 0x2;
    pDesc->ui_desp_00_awuser = 0x2;
#endif
    /* fill related descriptor here
    */
    pDesc->ui_desp_00_type = 0xF;

	return 0;

}


