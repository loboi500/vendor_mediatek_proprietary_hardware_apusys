#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <random>
#include <thread>
#include <unistd.h>

#include "apusys.h"
#include "edma_info.h"
#include "reviserTest.h"
#include "apusys.h"
#include "edmaEngine.h"

#define UT_SUBCMD_MAX 10

enum {
	UT_VLM_FULL_BANK,
	UT_TCM_FULL_BANK,
	UT_VLM_ONE_BANK,
	UT_TCM_ONE_BANK,
	UT_MAX,
};

reviserTestCmd::reviserTestCmd()
{
    return;
}

reviserTestCmd::~reviserTestCmd()
{
    return;
}

void reviserTestCmd::showUsage()
{
	printf("           <-s subIdx>             [%d] VLM FULL BANK test\n", UT_VLM_FULL_BANK);
	printf("                                   [%d] TCM FULL BANK test\n", UT_TCM_FULL_BANK);
	printf("                                   [%d] VLM 1 BANK test\n", UT_VLM_ONE_BANK);
	printf("                                   [%d] TCM 1 BANK test\n", UT_TCM_ONE_BANK);

	return;
}

static int g_channel = 1;
static int g_width = 1024;
static int g_height = 256;


static bool syncRun(reviserTestCmd * caseInst, unsigned int loopNum, unsigned int set_num, bool tcm, unsigned int vlm_size)
{
	unsigned int ui_src_buff_size, ui_dst_buff_size = 0;
	edmaMem *am_src_buff = NULL, *am_dst_buff = NULL, *dsc_buff = NULL, *am_mid_buff = NULL;
	st_edmaTaskInfo st_edma_task_info;
	unsigned char *dst, *src;
	st_edma_InputShape *shape = NULL;
	edmaCmd *apusys_cmd = NULL;
	std::vector<int> vec_mem_size;
	std::vector<SubCmdInfo *> vec_subcmd;
	std::vector<void *> vec_edma_task_info;
	unsigned int i, j;
	EdmaEngine *edmaEng;
	bool ret = false;
	void *pst_curr_desc_info;
	unsigned int ui_desc_num, ui_desc_idx, ui_src_buff_offset = 0;
	unsigned int vlm_addr = 0;
 	std::random_device rd;
 	std::default_random_engine gen = std::default_random_engine(rd());
 	std::uniform_int_distribution<int> dis(1, 255);
 	bool random_enable = false;

	if (tcm)
		vlm_addr = 0x1D000000;
	else
		vlm_addr = 0x1D800000;

	if (!set_num)
		random_enable = true;

	edmaEng = new EdmaEngine("edma_engine");

	apusys_cmd = edmaEng->initCmd();
	if (apusys_cmd == nullptr)
	{
		RVR_ERR("apusys_cmd null\n");
		return false;
	}

	ui_src_buff_size = g_width * g_height;
	ui_dst_buff_size = ui_src_buff_size;
	ui_desc_num = 1;
	ui_desc_idx = 0;

	RVR_INFO("loopNum %u, set_num %u tcm %u vlm_size: 0x%x\n", loopNum, set_num, tcm, vlm_size);

	am_src_buff = edmaEng->memAlloc(ui_src_buff_size*sizeof(unsigned char));
	if (am_src_buff == nullptr) {
		RVR_ERR("malloc null\n");
		return false;
	}
	am_dst_buff = edmaEng->memAlloc(ui_dst_buff_size*sizeof(unsigned char));
	if (am_dst_buff == nullptr) {
		RVR_ERR("malloc null\n");
		return false;
	}

	st_edma_task_info.info_num = 2;

	dst = (unsigned char *)am_dst_buff->va;
	src = (unsigned char *)am_src_buff->va;

	st_edma_task_info.edma_info = (st_edmaUnifyInfo *)calloc(st_edma_task_info.info_num, sizeof(st_edmaUnifyInfo));
	if (st_edma_task_info.edma_info == nullptr)
	{
		RVR_ERR("calloc null\n");
		goto free_mem;
	}

	st_edma_task_info.edma_info[0].info_type = EDMA_INFO_GENERAL;

	pst_curr_desc_info = &st_edma_task_info.edma_info[0].st_info.infoShape;
	shape = (st_edma_InputShape *)pst_curr_desc_info;
	shape->uc_desc_type = EDMA_INFO_GENERAL;
	shape->inBuf_addr = (unsigned int)(am_src_buff->iova);
	//shape->outBuf_addr = (unsigned int)(am_mid_buff->iova);
	shape->outBuf_addr = vlm_addr;
	shape->inFormat = EDMA_FMT_NORMAL;
	shape->outFormat = EDMA_FMT_NORMAL;
	shape->size_x = g_width;
	shape->size_y = g_height;
	shape->size_z = 1;

	shape->dst_stride_x = g_width;
	shape->dst_stride_y = g_height;

	st_edma_task_info.edma_info[1].info_type = EDMA_INFO_GENERAL;
	pst_curr_desc_info = &st_edma_task_info.edma_info[1].st_info.infoShape;
	//st_edma_task_info.puc_desc_type[ui_desc_idx] = 2;
	shape = (st_edma_InputShape *)pst_curr_desc_info;
	shape->uc_desc_type = EDMA_INFO_GENERAL;
	//shape->inBuf_addr = (unsigned int)(am_mid_buff->iova);
	shape->inBuf_addr = vlm_addr;
	shape->outBuf_addr = (unsigned int)(am_dst_buff->iova);
	shape->inFormat = EDMA_FMT_NORMAL;
	shape->outFormat = EDMA_FMT_NORMAL;
	shape->size_x = g_width;
	shape->size_y = g_height;
	shape->size_z = 1;

	printf("am_mid_buff 0x%x:\n", shape->inBuf_addr);

	shape->dst_stride_x = g_width;
	shape->dst_stride_y = g_height;



	// initialize apusys sub-command buffer
	vec_edma_task_info.push_back((void *)&st_edma_task_info);
	/*size[0] = request cmd size, size[1] = total descriptor size*/
	edmaEng->querySubCmdInfo(vec_edma_task_info, vec_mem_size);

	//printf("vec_mem_size.size() = %d\n", vec_mem_size.size());

	for (i=0; i<vec_mem_size.size(); i++) {
		SubCmdInfo *p_subcmd_info = new SubCmdInfo;
		//printf("cmd[%d] size = %d\n", i, vec_mem_size[i] );
		p_subcmd_info->descMem = edmaEng->memAllocCmd(vec_mem_size[i]);
		vec_subcmd.push_back(p_subcmd_info);
	}

	edmaEng->fillDesc(vec_edma_task_info, vec_subcmd);

	apusys_cmd->addSubCmd(vec_subcmd, SC_PARAM_VLM_USAGE, vlm_size);

	dsc_buff = vec_subcmd[1]->descMem;

	for (i = 0; i < loopNum; i++) {

		if (random_enable)
		{
			set_num = dis(gen);
			//printf("set_num random %x\n", set_num);
		}
		memset(src, set_num, ui_src_buff_size); //init src memory
		memset(dst, 0x00, ui_dst_buff_size); //init dst memory

		edmaEng->runCmd(apusys_cmd);
		//printf("check results, dst[0] = 0x%x:\n", *(dst));
		for (j = 0; j < ui_dst_buff_size; j++) {
			if (*(dst) != set_num)
				break;
		}
		if (j == ui_dst_buff_size) {
			printf("Pass %u times, dst[0] = 0x%x/0x%x\n", i, dst[0], set_num);
		} else {
			printf("%d times Fail, data error byte = p[%d] = 0x%x / pattern: 0x%x\n", i+1,j, *(dst + j), set_num);
			ret = -6;
			goto out;
		}
	}


	ret = true;

    RVR_INFO("Done\n");

out:

	for (i=0; i<vec_mem_size.size(); i++) {
		SubCmdInfo *p_subcmd_info = vec_subcmd[i];

		if (p_subcmd_info != NULL && p_subcmd_info->descMem != NULL) {
			printf("free p_subcmd_info[%d] va 0x%p\n", i, p_subcmd_info->descMem->va);
			edmaEng->memFree(p_subcmd_info->descMem);
			delete p_subcmd_info;
		}
	}
free_mem:
	printf("Free src/dst buffer\n");
	edmaEng->memFree(am_src_buff);
	edmaEng->memFree(am_dst_buff);
	if (edmaEng->destroyCmd(apusys_cmd) == false)
	{
		printf("delete apusys command fail\n");
	}

	delete edmaEng;

	return ret;
}


bool reviserTestCmd::runCase(unsigned int subCaseIdx)
{
	bool ret = false;
	if (mLoopNum == 0) {
		printf("cmdTest: loop number(%u), do nothing\n", mLoopNum);
		return false;
	}
	printf("subCaseIdx %u, set_num %u, mLoopNum %u vlm_size %u\n", subCaseIdx, mSetnum, mLoopNum, mVlmSize);


	switch(subCaseIdx)
	{
		case UT_VLM_FULL_BANK:
			printf("VLM FULL <<<<<\n");
			g_channel = 1;
			g_width = 1024;
			g_height = 1024;
			ret = syncRun(this, mLoopNum, mSetnum, false, mVlmSize);
			break;
		case UT_TCM_FULL_BANK:
			printf("TCM FULL\n");
			g_channel = 1;
			g_width = 1024;
			g_height = 1024;
			ret = syncRun(this, mLoopNum, mSetnum, true, mVlmSize);
			break;
		case UT_TCM_ONE_BANK:
			printf("TCM ONE BANK\n");
			g_channel = 1;
			g_width = 1024;
			g_height = 256;

			ret = syncRun(this, mLoopNum, mSetnum, true, mVlmSize);
			break;
		case UT_VLM_ONE_BANK:
			printf("VLM ONE BANK\n");
			g_channel = 1;
			g_width = 1024;
			g_height = 256;

			ret = syncRun(this, mLoopNum, mSetnum, false, mVlmSize);
			break;
		default:
			printf("Undefine SubCommand\n");
			break;
	}


	return ret;
	//return directlyRun(this, mLoopNum);
}
