#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>

#include "edma_cmn.h"
#include "edma_def.h"

#include "edma_info.h"
#include "edma_core_hw.h"



int edma_queryMVPUDNum(unsigned char type, st_edmaUnifyInfo *shapeInfo)
{
	EDMA_LOG_NONE("%s type = %d, info_type = %d \n", __func__, type, shapeInfo->info_type);
	return 1;
}

int edma_queryMVPUDSize(unsigned char type, st_edmaUnifyInfo *shapeInfo)
{
	EDMA_LOG_NONE("%s type = %d, info_type = %d \n", __func__, type, shapeInfo->info_type);
	return -1; //not support type
}

/*for edma 3.0*/


void edma_MVPUtransTaskInfo(st_edmaTaskInfo *pst_old_edma_task_info, st_edmaTaskInfo *pst_new_edma_task_info)
{
	EDMA_LOG_NONE("%s pst_old_edma_task_info = %p, pst_new_edma_task_info = %p\n",
		__func__, (void *)pst_old_edma_task_info, (void *)pst_new_edma_task_info);
}


int fillDescType0(st_edmaUnifyInfo *pInfo, void *edma_desc)
{
	EDMA_LOG_NONE("%s info_type = %d, edma_desc = %p\n",
		__func__, pInfo->info_type, edma_desc);
	return 0;
}

int fillDescType1(st_edmaUnifyInfo *pInfo, void *edma_desc)
{

	EDMA_LOG_NONE("%s info_type = %d, edma_desc = %p\n",
		__func__, pInfo->info_type, edma_desc);
	return 0;
}




