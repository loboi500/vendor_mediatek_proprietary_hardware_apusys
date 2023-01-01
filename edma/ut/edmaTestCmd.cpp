#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <random>
#include <thread>
#include <unistd.h>

#include "edmaTest.h"
#include "apusys.h"
#include "edma_info.h"


#define UT_SUBCMD_MAX 10
//static int g_channel = 1024;
static int g_width = 1024;
static int g_height = 1024;

#define MULT_DESC_SRC_NUM 8
#define MULT_DESC_SRC_IDX_MASK (MULT_DESC_SRC_NUM-1)
#define TEST_DATA_VALUE 0x77
#define TEST_DATA_VALUE2 0xAA
#define TEST_COPY_SIZE 1024*1024

int edmaTestCmd::directlyRun(edmaTestCmd * caseInst, unsigned int loopNum, unsigned int caseID)
{
	std::string name;
	unsigned int k = 0;
	unsigned int i = 0, j = 0;
	int ret = 0;
	unsigned char *ptr;
	unsigned int *ptr32;

	std::vector<int> vec_mem_size;
	std::vector<SubCmdInfo *> vec_subcmd;
	std::vector<void *> vec_edma_task_info;

	st_edmaTaskInfo st_edma_task_info;
	void *pst_curr_desc_info;

	st_edma_InputShape *shape = NULL;

	edmaMem *am_src_buff = NULL, *am_dst_buff = NULL, *dsc_buff = NULL;
	edmaMem *am_mid_buff = NULL;
	st_edma_InfoData *edma_info_copy = NULL;

	edmaCmd *apusys_cmd = NULL;
	std::vector<int> apusys_sub_cmd_dependency;


	unsigned char *ptr8 = NULL, test_value;

	unsigned int ui_src_buff_size, ui_src_buff_offset = 0, ui_dst_buff_size = 0;

	unsigned int ui_desc_num, ui_desc_idx;

	ui_src_buff_size = g_width * g_height;
	ui_dst_buff_size = ui_src_buff_size;

	printf("directlyRun, case id = %d, test size = %d bytes\n",caseID, ui_dst_buff_size);


	ui_desc_num = 1;
	ui_desc_idx = 0;
	st_edma_task_info.edma_info = NULL;

	am_src_buff = caseInst->getEngine()->memAlloc(ui_src_buff_size*sizeof(unsigned char));
	am_dst_buff = caseInst->getEngine()->memAlloc(ui_dst_buff_size*sizeof(unsigned char));

	if (am_src_buff == NULL) {
        	ret = -1;
		goto EXIT_PROCESS;
    }

	if (am_dst_buff == NULL)
		goto EXIT_PROCESS;


	st_edma_task_info.info_num = ui_desc_num;

	//mem_ext = caseInst->getEngine()->memAlloc(EDMA_EXT_MODE_SIZE);
	ptr = (unsigned char *)am_dst_buff->va;
	ptr32 = (unsigned int *)am_dst_buff->va;

	//printf("directlyRun #2, ptr = 0x%x\n", ptr);

	switch (caseID) {
		case 1:
		case 2:
		case 5:
			/*type array & info array for rquest total size*/
			if (loopNum%10)
				test_value = TEST_DATA_VALUE;
			else
				test_value = TEST_DATA_VALUE2;
			ptr8 = (unsigned char *)am_src_buff->va;
			memset(ptr8, test_value, ui_src_buff_size); //init src memory
			memset(ptr, 0x00, ui_dst_buff_size); //init dst memory

			//st_edma_task_info.puc_desc_type = (unsigned char *)calloc(st_edma_task_info.ui_desc_num, sizeof(unsigned char));
			st_edma_task_info.edma_info = (st_edmaUnifyInfo *)calloc(st_edma_task_info.info_num, sizeof(st_edmaUnifyInfo));

            if (st_edma_task_info.edma_info == NULL)
                goto EXIT_PROCESS;

			st_edma_task_info.edma_info[0].info_type = EDMA_INFO_GENERAL;

			pst_curr_desc_info = &st_edma_task_info.edma_info[0].st_info.infoShape;
			//st_edma_task_info.puc_desc_type[ui_desc_idx] = 2;
			shape = (st_edma_InputShape *)pst_curr_desc_info;
			shape->uc_desc_type = EDMA_INFO_GENERAL;
			shape->inBuf_addr = (unsigned int)(am_src_buff->iova+ui_src_buff_offset*(ui_desc_idx&MULT_DESC_SRC_IDX_MASK)); //2^7 = 128 bytes
			shape->outBuf_addr = (unsigned int)am_dst_buff->iova;
			shape->inFormat = EDMA_FMT_NORMAL;
			shape->outFormat = EDMA_FMT_NORMAL;
			shape->size_x = g_width;
			shape->size_y = g_height;
			shape->size_z = 1;

			shape->dst_stride_x = g_width;
			shape->dst_stride_y = g_height;

			break;
		case 3:
			am_mid_buff = caseInst->getEngine()->memAllocVLM(ui_dst_buff_size*sizeof(unsigned char));
			if (am_mid_buff == NULL)
				goto EXIT_PROCESS;

			printf("caseID = %d, test vlm, iova = 0x%llx\n",caseID, am_mid_buff->iova);
			if (loopNum%10)
				test_value = TEST_DATA_VALUE;
			else
				test_value = TEST_DATA_VALUE2;

			ui_desc_num = 2;
			ui_desc_idx = 0;
			st_edma_task_info.info_num = ui_desc_num;
			/*type array & info array for rquest total size*/
			ptr8 = (unsigned char *)am_src_buff->va;
			memset(ptr8, test_value, ui_src_buff_size); //init src memory
			memset(ptr, 0x00, ui_dst_buff_size); //init dst memory

			//st_edma_task_info.puc_desc_type = (unsigned char *)calloc(st_edma_task_info.ui_desc_num, sizeof(unsigned char));
			st_edma_task_info.edma_info = (st_edmaUnifyInfo *)calloc(st_edma_task_info.info_num, sizeof(st_edmaUnifyInfo));

            if (st_edma_task_info.edma_info == NULL)
                goto EXIT_PROCESS;
			st_edma_task_info.edma_info[0].info_type = EDMA_INFO_DATA;
			st_edma_task_info.edma_info[1].info_type = EDMA_INFO_DATA;

			pst_curr_desc_info = &st_edma_task_info.edma_info[0].st_info.infoData;

			edma_info_copy = (st_edma_InfoData *)pst_curr_desc_info;
			edma_info_copy->uc_desc_type = EDMA_INFO_DATA;
			edma_info_copy->inBuf_addr = (unsigned int)(am_src_buff->iova); //2^7 = 128 bytes
			edma_info_copy->outBuf_addr = (unsigned int)(am_mid_buff->iova);
			edma_info_copy->inFormat = EDMA_FMT_NORMAL;
			edma_info_copy->outFormat = EDMA_FMT_NORMAL;
			edma_info_copy->copy_size = TEST_COPY_SIZE;

			pst_curr_desc_info = &st_edma_task_info.edma_info[1].st_info.infoData;

			edma_info_copy = (st_edma_InfoData *)pst_curr_desc_info;
			edma_info_copy->uc_desc_type = EDMA_INFO_DATA;
			edma_info_copy->inBuf_addr = (unsigned int)(am_mid_buff->iova); //2^7 = 128 bytes
			edma_info_copy->outBuf_addr = (unsigned int)(am_dst_buff->iova);
			edma_info_copy->inFormat = EDMA_FMT_NORMAL;
			edma_info_copy->outFormat = EDMA_FMT_NORMAL;
			edma_info_copy->copy_size = TEST_COPY_SIZE;

			break;
		default:
			printf("caseID = %d, not support\n",caseID);
			return -1;
		}

	apusys_cmd = caseInst->getEngine()->initCmd();

	if (apusys_cmd == NULL)
		goto EXIT_PROCESS;

	// initialize apusys sub-command buffer
	vec_edma_task_info.push_back((void *)&st_edma_task_info);
	/*size[0] = request cmd size, size[1] = total descriptor size*/
	caseInst->getEngine()->querySubCmdInfo(vec_edma_task_info, vec_mem_size);

	//printf("vec_mem_size.size() = %d\n", vec_mem_size.size());

	for (i=0; i<vec_mem_size.size(); i++) {
		SubCmdInfo *p_subcmd_info = new SubCmdInfo;
		//printf("cmd[%d] size = %d\n", i, vec_mem_size[i] );
		p_subcmd_info->descMem = caseInst->getEngine()->memAllocCmd(vec_mem_size[i]);
		vec_subcmd.push_back(p_subcmd_info);
	}

	caseInst->getEngine()->fillDesc(vec_edma_task_info, vec_subcmd);

    if (caseID == 3)
		apusys_cmd->addSubCmd(vec_subcmd, SC_PARAM_VLM_USAGE, 1024*1024); //use tcm
	else
		apusys_cmd->addSubCmd(vec_subcmd);
#if 0
	sub_cmd_buff = vec_subcmd[0]->descMem;
	apusys_sub_cmd_dependency.clear();
	apusys_cmd->addSubCmd(sub_cmd_buff, APUSYS_DEVICE_EDMA, apusys_sub_cmd_dependency);
	apusys_sub_cmd_dependency.clear();

	apusys_cmd->addSubCmd(dsc_buff, APUSYS_DEVICE_EDMA, apusys_sub_cmd_dependency);
#endif

	dsc_buff = vec_subcmd[1]->descMem;

	ptr32 =  (unsigned int *)dsc_buff->va;
    if (caseInst->getConfig() & EDMA_TEST_FORCE_ERROR) {
		printf("EDMA_TEST_FORCE_ERROR...\n");
        for (k=0; k < dsc_buff->size/4; k++)
            ptr32[k] = 0;
    }

	EDMA_LOG_INFO("directlyRun #2\n");

	if (caseInst->getConfig() & EDMA_TEST_SHOW_DESC) {
		printf("EDMA_TEST_SHOW_DESC...\n");

		for (k=0; k < dsc_buff->size/4; k++)
			printf("dsc_buff[%x] = 0x%x\n", k*4, ptr32[k] );
	}
	for (i = 0; i < loopNum; i++) {
		// start execution
		caseInst->getEngine()->runCmd(apusys_cmd);

		if (caseID == 1 || caseID == 3) {
			printf("check results, dst[0] = 0x%x:\n", *(ptr));
			for (j = 0; j < ui_dst_buff_size; j++) {
				if (*(ptr + j) != test_value)
					break;
			}

			if (j == ui_dst_buff_size) {
				printf("%d times Pass, ptr[0] = 0x%x\n", i+1, ptr[0]);
				memset(ptr, 0x00, ui_dst_buff_size); //init dst memory for next test
			} else {
				printf("%d times Fail, data error byte = p[%d] = 0x%x\n", i+1,j, *(ptr + j));
				ret = -6;
				goto EXIT_PROCESS;
			}
		}
	}

EXIT_PROCESS:

	for (i=0; i<vec_mem_size.size(); i++) {
		SubCmdInfo *p_subcmd_info = vec_subcmd[i];

		if (p_subcmd_info != NULL && p_subcmd_info->descMem != NULL) {
			caseInst->getEngine()->memFree(p_subcmd_info->descMem);
			printf("free p_subcmd_info[%d]\n", i);
			delete p_subcmd_info;
		}
	}
    // free apusys memory
    if (am_src_buff != NULL)
        caseInst->getEngine()->memFree(am_src_buff);
    if (am_dst_buff != NULL)
        caseInst->getEngine()->memFree(am_dst_buff);

    if (am_mid_buff != NULL)
        caseInst->getEngine()->memFree(am_mid_buff);

    if (st_edma_task_info.edma_info != NULL)
        free(st_edma_task_info.edma_info);

    if (apusys_cmd != NULL) {
    	if (caseInst->getEngine()->destroyCmd(apusys_cmd) == false)
    	{
    		printf("delete apusys command fail\n");
    	}
    }

	return ret;
}

edmaTestCmd::edmaTestCmd()
{
	printf("new edmaTestCmd\n");
}

edmaTestCmd::~edmaTestCmd()
{
	printf("del edmaTestCmd\n");
}

int edmaTestCmd::runCase(unsigned int loopNum, unsigned int caseID)
{
	if (loopNum == 0) {
		printf("cmdTest: loop number(%u), do nothing\n", loopNum);
		return 0;
	}
	std::unique_lock<std::mutex> mutexLock(apusys_mtx);

	return directlyRun(this, loopNum, caseID);
}
