#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <cstdint>
#include <getopt.h>
#include <unistd.h>

#include "edma_info.h"
#include "edma_test_data.h"
#include "edma_cmn.h"

#include "DeviceEngine.h"
#include "edmaEngine.h"

#ifdef __ANDROID__
#include <cutils/properties.h>
#endif

#include "edmaTest.h"

#define APUSYS_UT_LOOP_MAX 1000000000
#define APUSYS_UT_THREAD_MAX 10

edmaTestCmd * gTestCmd = nullptr;
static void showHelp(void)
{
	printf("==========================================================================\n");
	printf("  edma sample test \n");
	printf("==========================================================================\n");
	printf(" prototype: edma_test64 [-l loop]\n\n");
	printf("--------------------------------------------------------------------------\n");
	printf("  -l --loop            (optional) number of loop count, default=0 \n");
	printf("  -s --stress          (optional) stress test, with sleep time (us) \n");
	printf("  -c --compress        (optional) compress test support 2,3\n");
	printf("  -d --show descriptor (optional) show descriptor content\n");
	printf("  -h --help            (optional) help \n");
	printf("==========================================================================\n");
	return;
}

static struct option longOptions[] =
{
	{"loop", required_argument, 0, 'l'},
	{"stress", no_argument, 0, 's'},
	{"compress", required_argument, 0, 'c'},
	{"sleep time", required_argument, 0, 't'},
	{"show descriptor", no_argument, 0, 'd'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};
#if 1


#define MULT_DESC_SRC_NUM 8
#define MULT_DESC_SRC_IDX_MASK (MULT_DESC_SRC_NUM-1)

enum SCN_LOC_IDX
{
	SCN_LOC_D2D = 0,
	SCN_LOC_D2T2D,
	SCN_LOC_D2L2D,
	SCN_LOC_D2I2D,
	SCN_LOC_END
};

enum MODE_IDX
{
	MODE_NORMAL = 0,
	MODE_FILL,
	MODE_RGB2ARGB,
	MODE_END
};

enum SIZE_IDX
{
	SIZE_X = 0,
	SIZE_Y,
	SIZE_Z,
	SIZE_W,
	SIZE_END
};

enum PITCH_IDX
{
	PITCH_ROW = 0,
	PITCH_SLICE,
	PITCH_CUBE,
	PITCH_END
};

enum RGB2ARGB_ALPHA_POS_IDX
{
	RGB2ARGB_ALPHA_POS_HIGH = 0,
	RGB2ARGB_ALPHA_POS_LOW,
	RGB2ARGB_ALPHA_POS_END
};

typedef enum
{
	DEV_DRAM_S0 = 0,
	DEV_DRAM_S1,
	DEV_SYSRAM
} DEV_MEM_TYPE;

void dumpDescriptor(unsigned int *ptr32, unsigned int size)
{
	if (gTestCmd->getConfig() & EDMA_TEST_SHOW_DESC) {
		unsigned int k;
		printf("EDMA_TEST_SHOW_DESC...\n");
		for (k=0; k<size/4; k++)
		{
			printf("dsc_buff[%x] = 0x%x\n", k*4, ptr32[k]);
		}
	}
}

void writeBufToFile(char *fileName, unsigned char *pBuf, int size)
{
	FILE *pFile;
	int wlens;

	pFile = fopen(fileName, "wb");
	if (pFile == NULL) {
		printf("Open %s fail!!\n", fileName);
	} else {
		wlens = fwrite(pBuf, sizeof(char), size, pFile);
		if (wlens != size)
			printf("fwrite fail!!\n");

		if (fclose(pFile) != 0)
			printf("fclose %s fail!!\n", fileName);
	}
}

void readFileToBuf(char const *fileName, unsigned char *pBuf, int size)
{
	FILE *pFile;
	unsigned int lens, i;
	pFile = fopen(fileName, "r");
	if (pFile == NULL) {
		printf("Open %s fail!!\n", fileName);
		return;
	} else {
		//fwrite(ptr8, sizeof(char), 512*512*4, pFile);
		lens = fread(pBuf, 1, size, pFile);
        if (lens == 0)
    		printf("error fread, 0 \n");
        else {
    		printf("Read %s, lens = %d, size = %d \n", fileName, lens, size);            
    		for (i=0; i< 10; i++)
    			printf("pBuf[%d] = 0x%x \n", i, pBuf[i]);
        }
		if (fclose(pFile) != 0)
			printf("fclose %s fail!!\n", fileName);

	}
}

void test_edma_engine_nn_FP(void)
{
	DeviceEngine *edma_engine = DeviceEngine::createInstance("edma_engine");
	EdmaDescEngine* edmaDesc_engine = new EdmaDescEngine();
	std::vector<int> vec_mem_size;
	std::vector<SubCmdInfo *> vec_subcmd;
	std::vector<void *> vec_edma_task_info;

	st_edmaTaskInfo st_edma_task_info;

	st_edma_info_nn *pst_info_nn = NULL;

	edmaMem *am_src_buff = NULL, *am_dst_buff = NULL, *sub_cmd_buff = NULL, *dsc_buff = NULL;
	edmaCmd *apusys_cmd = NULL;
	std::vector<int> apusys_sub_cmd_dependency;

	int j;
	unsigned int i;
	unsigned char *ptr8 = NULL;
	unsigned int *ptr32 = NULL;

	unsigned char uc_mode;
	unsigned int ui_src_data_size, ui_src_buff_size, ui_dst_data_size, ui_dst_buff_size;
	unsigned char *puc_golden_data = NULL, *puc_revised_data = NULL;

	unsigned int ui_desc_num, ui_desc_idx;
	void *pOut_edma_desc = NULL;
	unsigned int ui_desc_buff_size, desc_num;

	st_edmaUnifyInfo *pEdmaInfo = NULL;

	if (edma_engine == NULL) {
		printf("edma_engine = NULL\n");
		delete edmaDesc_engine;
		return;
	}
	st_edma_task_info.edma_info = NULL;


	printf("test_edma_engine_nn_FP #2\n");

	printf("size of st_edmaUnifyInfo = %d #2\n", (int)sizeof(st_edmaUnifyInfo));

	//EDMA_DESC_NN

	ui_desc_num = 1;
	ui_desc_idx = 0;


	uc_mode = MODE_NORMAL;
	ui_src_data_size = 2000;
	ui_dst_data_size = 4000;
	ui_src_buff_size = ui_src_data_size;
	ui_dst_buff_size = ui_dst_data_size;

	// memory allocation
	if ((puc_golden_data = (unsigned char *)malloc(ui_dst_data_size*sizeof(unsigned char))) == NULL)
	{
		printf("allocate memory buffer fail: puc_golden_data\n");
		goto EXIT_PROCESS;
	}
	if ((puc_revised_data = (unsigned char *)malloc(ui_dst_data_size*sizeof(unsigned char))) == NULL)
	{
		printf("allocate memory buffer fail: puc_revised_data\n");
		goto EXIT_PROCESS;
	}

	printf("test_edma_engine malloc\n");

	// allocate apusys memory
	am_src_buff = edma_engine->memAlloc(ui_src_buff_size*sizeof(unsigned char));
	am_dst_buff = edma_engine->memAlloc(ui_dst_buff_size*sizeof(unsigned char));

	if (am_src_buff == NULL || am_dst_buff == NULL )
		goto EXIT_PROCESS;

	st_edma_task_info.info_num = ui_desc_num;

	// output buffer address
	printf("am_src_buff->va: 0x%p\n", am_src_buff->va);
	//printf("am_src_buff->kva: 0x%X\n", (unsigned int)am_src_buff->kva);
	printf("am_src_buff->iova: 0x%u\n", (unsigned int)am_src_buff->iova);
	printf("\n");
	printf("am_dst_buff->va: 0x%p\n", am_dst_buff->va);
	//printf("am_dst_buff->kva: 0x%X\n", (unsigned int)am_dst_buff->kva);
	printf("am_dst_buff->iova: 0x%u\n", (unsigned int)am_dst_buff->iova);
	printf("\n");

	memset((unsigned char *)am_src_buff->va, 0x69, ui_src_buff_size);
	memset((unsigned char *)am_dst_buff->va, 0x3, ui_dst_buff_size);

	/*type array & info array for rquest total size*/
	// st_edma_task_info.puc_desc_type = (unsigned char *)calloc(st_edma_task_info.ui_desc_num, sizeof(unsigned char));

	pEdmaInfo = (st_edmaUnifyInfo *)calloc(st_edma_task_info.info_num, sizeof(st_edmaUnifyInfo));
	st_edma_task_info.edma_info = (st_edmaUnifyInfo *)calloc(st_edma_task_info.info_num, sizeof(st_edmaUnifyInfo));

	if (pEdmaInfo == NULL || st_edma_task_info.edma_info == NULL)
		goto EXIT_PROCESS;

	ui_desc_idx = 0;

	//pst_curr_desc_info = (void *)((uintptr_t)st_edma_task_info.pst_desc_info + (uintptr_t)(ui_desc_idx*sizeof(st_edma_info_nn)));
	st_edma_task_info.edma_info[ui_desc_idx].info_type = EDMA_INFO_NN;
	pst_info_nn = (st_edma_info_nn *)&st_edma_task_info.edma_info[ui_desc_idx].st_info.infoNN;
	//pst_info_nn->uc_desc_type = EDMA_INFO_NN;
	pst_info_nn->inFormat = EDMA_FMT_FP16;
	pst_info_nn->outFormat = EDMA_FMT_FP32;
	pst_info_nn->shape_c = 1000;
	pst_info_nn->shape_w = 1;
	pst_info_nn->shape_h = 1;
	pst_info_nn->shape_n = 1;

	pst_info_nn->src_stride_c = 1000;
	pst_info_nn->dst_stride_c = 1000;
	pst_info_nn->src_stride_w = 1000;
	pst_info_nn->dst_stride_w = 1000;

	pst_info_nn->src_stride_h = 1; //don't care?
	pst_info_nn->dst_stride_h = 1;

	pst_info_nn->dst_type_size = 4;
	pst_info_nn->src_type_size = 2;

	printf("test_edma_engine_nn_FP #2-1\n");

	//memcpy(pEdmaInfo, st_edma_task_info.edma_info, sizeof(st_edmaUnifyInfo));
	memcpy(&pEdmaInfo->st_info.infoNN, pst_info_nn, sizeof(st_edma_info_nn));

	printf("edmaInfo uc_desc_type =  %d, shape_c = %d\n", pEdmaInfo->info_type, pEdmaInfo->st_info.infoNN.shape_c);
	printf("edmaInfo pEdmaInfo->st_info.info1.descType =  %d\n", pEdmaInfo->st_info.info1.descType);

	//pst_curr_desc_info = (void *)((uintptr_t)pst_curr_desc_info + (uintptr_t)sizeof(_edma20_InputShape));


	//apusys_cmd = edma_engine->initCmd();

	// initialize apusys sub-command buffer
	//vec_edma_task_info.push_back((void *)&st_edma_task_info);
	/*size[0] = request cmd size, size[1] = total descriptor size*/
	//edma_engine->querySubCmdInfo(vec_edma_task_info, vec_mem_size);
	//edma_engine->queryDescSize();


	// get info of edma task
	//st_edmaTaskInfo *pst_edma_task_info = (st_edmaTaskInfo *)data[0];

	ui_desc_buff_size = edmaDesc_engine->queryDescSize(&st_edma_task_info, &desc_num);

	printf("ui_new_desc_buff_size = %d, desc_num = %d\r\n", ui_desc_buff_size, desc_num);

	if (ui_desc_buff_size == 0)
		goto EXIT_PROCESS;

	pOut_edma_desc = (void *)malloc(ui_desc_buff_size);

	if (pOut_edma_desc == NULL)
		goto EXIT_PROCESS;

	edmaDesc_engine->fillDesc(&st_edma_task_info, pOut_edma_desc); //auto memset 0 before set descriptor
	ptr32 = (unsigned int *)pOut_edma_desc;

	//ptr8 = (unsigned char *)pOut_edma_desc;
	for (ui_desc_idx = 0; ui_desc_idx < st_edma_task_info.info_num; ui_desc_idx++) {
		//pst_curr_desc_info = (void *)((uintptr_t)st_edma_task_info.pst_desc_info + (uintptr_t)(ui_desc_idx*sizeof(st_edma_info_nn)));
		pst_info_nn = (st_edma_info_nn *) &st_edma_task_info.edma_info[ui_desc_idx].st_info.infoNN;
		printf("desc_id = %d src_addr_offset = %d, dst_addr_offset = %d, shape_c = %d\r\n",
			pst_info_nn->desc_id,
			pst_info_nn->src_addr_offset,
			pst_info_nn->dst_addr_offset,
			pst_info_nn->shape_c);

		if (ui_desc_idx == 0) {
			//(unsigned int *)(ptr32 + (pst_info_nn->src_addr_offset)/4) = (unsigned int)(am_src_buff->iova);
			*(ptr32 + (pst_info_nn->src_addr_offset)/4) = (unsigned int)(am_src_buff->iova);
			*(ptr32 + (pst_info_nn->dst_addr_offset)/4) = (unsigned int)(am_dst_buff->iova);
		}
	}

	for (i = 0; i < ui_desc_buff_size/4; i++)
		printf("pOut_edma_desc[0x%x] = 0x%x\r\n", i*4, ptr32[i]);


//Test send scriptror
	apusys_cmd = edma_engine->initCmd();
	if (apusys_cmd == NULL)
		goto EXIT_PROCESS;

	// initialize apusys sub-command buffer
	vec_edma_task_info.push_back((void *)&st_edma_task_info);
	/*size[0] = request cmd size, size[1] = total descriptor size*/
	edma_engine->querySubCmdInfo(vec_edma_task_info, vec_mem_size);

	printf("vec_mem_size.size() = %d\n", (int)vec_mem_size.size());

	for (i=0; i<vec_mem_size.size(); i++) {
		SubCmdInfo *p_subcmd_info = new SubCmdInfo;
		printf("cmd[%d] size = %d\n", i, vec_mem_size[i] );
		p_subcmd_info->descMem = edma_engine->memAlloc(vec_mem_size[i]);
		vec_subcmd.push_back(p_subcmd_info);
	}
	dsc_buff = vec_subcmd[1]->descMem;

	ptr32 =  (unsigned int *)dsc_buff->va;
	ptr8 = (unsigned char *)pOut_edma_desc;

	edma_engine->fillDesc(vec_edma_task_info, vec_subcmd);

	memcpy((unsigned char *)dsc_buff->va, ptr8, vec_mem_size[1]);

	for (j=0; j<vec_mem_size[1]/4; j++)
		printf("dsc_buff[%x] = 0x%x\n", j*4, ptr32[j]);

	sub_cmd_buff = vec_subcmd[0]->descMem;
	apusys_sub_cmd_dependency.clear();
	apusys_cmd->addSubCmd(sub_cmd_buff, APUSYS_DEVICE_EDMA, apusys_sub_cmd_dependency);

	// start execution
	edma_engine->runCmd(apusys_cmd);

	// comparison
	ptr8 = (unsigned char *)am_dst_buff->va;

	for(i=0; i<16; i++)
		printf("am_dst_buff data ptr8[%d] = 0x%x\n", i , *(ptr8+i));

#endif
EXIT_PROCESS:
	// destroy apusys command
	if (apusys_cmd != NULL)
		edma_engine->destroyCmd(apusys_cmd);

	// free apusys memory
	if (am_src_buff != NULL)
		edma_engine->memFree(am_src_buff);
	if (am_dst_buff != NULL)
		edma_engine->memFree(am_dst_buff);

	// free memory
	if (st_edma_task_info.edma_info != NULL)
		free(st_edma_task_info.edma_info);

	if (pEdmaInfo != NULL)
		free(pEdmaInfo);	

	if (puc_golden_data != NULL)
		free(puc_golden_data);
	if (puc_revised_data != NULL)
		free(puc_revised_data);

	if (pOut_edma_desc != NULL)
		free(pOut_edma_desc);
		
	delete edmaDesc_engine;
	DeviceEngine::deleteInstance(edma_engine);

}

#define TEST_SRC_C 5
#define TEST_DST_C 7

void test_edma_engine_nn_gxp(unsigned int g, unsigned int p)
{
	DeviceEngine *edma_engine = DeviceEngine::createInstance("edma_engine");
	EdmaDescEngine* edmaDesc_engine = new EdmaDescEngine();

	std::vector<int> vec_mem_size;
	std::vector<SubCmdInfo *> vec_subcmd;
	std::vector<void *> vec_edma_task_info;

	st_edmaTaskInfo st_edma_task_info;

	st_edma_info_nn *pst_info_nn = NULL;

	st_edma_info_nn *ptrNN = NULL;


	edmaMem *am_src_buff = NULL, *am_dst_buff = NULL, *am_dmy_buff = NULL, *sub_cmd_buff = NULL, *dsc_buff = NULL;
	edmaCmd *apusys_cmd = NULL;
	std::vector<int> apusys_sub_cmd_dependency;

	int i;

	unsigned char *ptr8 = NULL;

	unsigned int *ptr32 = NULL;


	unsigned char uc_mode;
	unsigned int ui_src_data_size, ui_src_buff_size, ui_dst_data_size, ui_dst_buff_size;

	unsigned int ui_desc_num, ui_desc_idx, j;

	unsigned char *pOut_edma_desc = NULL;

	unsigned int ui_desc_buff_size;

	st_edma_request_nn request;
	st_edma_result_nn result;

	if (edma_engine == NULL) {
		printf("edma_engine = NULL\n");
		delete edmaDesc_engine;
		return;
	}

	printf("test_edma_engine_nn_%dX%d \n",g,p);

	printf("size of st_edmaUnifyInfo = %d #2\n", (int)sizeof(st_edmaUnifyInfo));

	//EDMA_DESC_NN
	st_edma_task_info.edma_info = NULL;
	request.info_list = NULL;
	result.cmd_size = 0;
	result.cmd_count = 0;

	ui_desc_num = 1;

	ui_desc_idx = 0;


	uc_mode = MODE_NORMAL;

	ui_src_data_size = 1000;

	ui_dst_data_size = 1000;

	ui_src_buff_size = ui_src_data_size;

	ui_dst_buff_size = ui_dst_data_size;



	printf("test_edma_engine malloc\n");

	// allocate apusys memory
	am_src_buff = edma_engine->memAlloc(ui_src_buff_size*sizeof(unsigned char));
	am_dst_buff = edma_engine->memAlloc(ui_dst_buff_size*sizeof(unsigned char));

	am_dmy_buff = edma_engine->memAlloc(ui_src_buff_size*sizeof(unsigned char));

	if (am_src_buff == NULL || am_dst_buff == NULL || am_dmy_buff == NULL)
		goto EXIT_PROCESS;

	// output buffer address
	printf("am_src_buff->va: 0x%p\n", am_src_buff->va);
	//printf("am_src_buff->kva: 0x%X\n", (unsigned int)am_src_buff->kva);
	printf("am_src_buff->iova: 0x%u\n", (unsigned int)am_src_buff->iova);

	printf("am_dst_buff->va: 0x%p\n", am_dst_buff->va);
	//printf("am_dst_buff->kva: 0x%X\n", (unsigned int)am_dst_buff->kva);
	printf("am_dst_buff->iova: 0x%u\n", (unsigned int)am_dst_buff->iova);
	printf("\n");

	memset((unsigned char *)am_src_buff->va, 0x69, ui_src_buff_size);
	memset((unsigned char *)am_dst_buff->va, 0x3, ui_dst_buff_size);

	memset((unsigned char *)am_dmy_buff->va, 0, ui_src_buff_size);


	/*type array & info array for rquest total size*/
	//st_edma_task_info.puc_desc_type = (unsigned char *)calloc(st_edma_task_info.ui_desc_num, sizeof(unsigned char));

	pst_info_nn = (st_edma_info_nn *)calloc(ui_desc_num, sizeof(st_edma_info_nn));

	if (pst_info_nn == NULL)
		goto EXIT_PROCESS;


	for (ui_desc_idx = 0; ui_desc_idx < ui_desc_num; ui_desc_idx++) {

		//pst_curr_desc_info = (void *)((uintptr_t)&st_edma_task_info.edma_info[ui_desc_idx].info.infoNN;
		ptrNN = &pst_info_nn[ui_desc_idx];

		//pst_info_nn->uc_desc_type = EDMA_INFO_NN;
		ptrNN->inFormat = EDMA_FMT_NORMAL;
		ptrNN->outFormat = EDMA_FMT_NORMAL;
		ptrNN->shape_c = g;
		ptrNN->shape_w = 32;
		ptrNN->shape_h = 2;
		ptrNN->shape_n = 2;

		ptrNN->src_stride_c = g;
		ptrNN->dst_stride_c = p;		
		ptrNN->src_stride_w = 32*g;
		ptrNN->dst_stride_w = 32*p;		

		ptrNN->src_stride_h = 2*32*g; 
		ptrNN->dst_stride_h = 2*32*p;

		ptrNN->dst_type_size = 1;
		ptrNN->src_type_size = 1;

	}

	request.info_list = pst_info_nn;
	request.info_num = ui_desc_num;

	edmaDesc_engine->queryDescSize(&request, &result);

	ui_desc_buff_size = result.cmd_size;


	printf("cmd_size = %d, cmd_count = %d\r\n", result.cmd_size, result.cmd_count);

	if (result.cmd_size == 0)
		goto EXIT_PROCESS;

	pOut_edma_desc = (unsigned char *)malloc(result.cmd_size);

	if (pOut_edma_desc == NULL)
		goto EXIT_PROCESS;

	edmaDesc_engine->fillDesc(&request, pOut_edma_desc); //need to fill edma request (edma_ext)

	ptr32 = (unsigned int *)pOut_edma_desc;

	//ptr8 = (unsigned char *)pOut_edma_desc;
 	for (ui_desc_idx = 0; ui_desc_idx < ui_desc_num; ui_desc_idx++) {
		pst_info_nn =  &pst_info_nn[ui_desc_idx];
			printf("desc_id = %d src_addr_offset = 0x%x, dst_addr_offset = 0x%x, dummy offset = %d, shape_c = %d#2\r\n", 
				pst_info_nn->desc_id,
				pst_info_nn->src_addr_offset,
				pst_info_nn->dst_addr_offset,
				pst_info_nn->dummy_addr_offset,
				pst_info_nn->shape_c);

		if (ui_desc_idx == 0){
			//(unsigned int *)(ptr32 + (pst_info_nn->src_addr_offset)/4) = (unsigned int)(am_src_buff->iova);
			*(ptr32 + (pst_info_nn->src_addr_offset)/4) = (unsigned int)(am_src_buff->iova);
			*(ptr32 + (pst_info_nn->dst_addr_offset)/4) = (unsigned int)(am_dst_buff->iova);
			if (pst_info_nn->dummy_addr_offset != 0)
					*(ptr32 + (pst_info_nn->dummy_addr_offset)/4) = (unsigned int)(am_dmy_buff->iova);
		}
	}


	printf("check pOut_edma_desc\r\n");

	for (j = 0; j < ui_desc_buff_size/4; j++)
		printf("pOut_edma_desc[0x%x] = 0x%x\r\n", j*4, ptr32[j]);


//Test send scriptror
	apusys_cmd = edma_engine->initCmd();
	if (apusys_cmd == NULL)
		goto EXIT_PROCESS;

	st_edma_task_info.info_num = request.info_num;
	st_edma_task_info.edma_info = (st_edmaUnifyInfo *)calloc(st_edma_task_info.info_num, sizeof(st_edmaUnifyInfo));

	if (st_edma_task_info.edma_info == NULL)
		goto EXIT_PROCESS;

	for (j=0; j< request.info_num; j++ )
	{
		memcpy(&st_edma_task_info.edma_info[j].st_info.infoNN, &request.info_list[j], sizeof(st_edma_info_nn));
		st_edma_task_info.edma_info[j].info_type = EDMA_INFO_NN;
	}

	// initialize apusys sub-command buffer
	vec_edma_task_info.push_back((void *)&st_edma_task_info);
	/*size[0] = request cmd size, size[1] = total descriptor size*/
	edma_engine->querySubCmdInfo(vec_edma_task_info, vec_mem_size);

	printf("vec_mem_size.size() = %d\n", (int)vec_mem_size.size());

	for (j=0; j<vec_mem_size.size(); j++)
	{
		SubCmdInfo *p_subcmd_info = new SubCmdInfo;
		printf("cmd[%d] size = %d\n", j, vec_mem_size[j] );
		p_subcmd_info->descMem = edma_engine->memAlloc(vec_mem_size[j]);
		vec_subcmd.push_back(p_subcmd_info);
	}
	dsc_buff = vec_subcmd[1]->descMem;

	ptr32 = (unsigned int *)dsc_buff->va;
	ptr8 = (unsigned char *)pOut_edma_desc;

	edma_engine->fillDesc(vec_edma_task_info, vec_subcmd);

	memcpy((unsigned char *)dsc_buff->va, ptr8, vec_mem_size[1]);

	for (i=0; i<vec_mem_size[1]/4; i++)
	{
		printf("dsc_buff[%x] = 0x%x\n", i*4, ptr32[i] );
	}

	sub_cmd_buff = vec_subcmd[0]->descMem;
	apusys_sub_cmd_dependency.clear();
	apusys_cmd->addSubCmd(sub_cmd_buff, APUSYS_DEVICE_EDMA, apusys_sub_cmd_dependency);


	// start execution
	edma_engine->runCmd(apusys_cmd);

	// comparison

	// comparison
	ptr8 = (unsigned char *)am_dst_buff->va;

	for(i=0; i<200; i++)
		printf("am_dst_buff data ptr8[%d] = 0x%x\n", i, *(ptr8+i));

EXIT_PROCESS:
		// destroy apusys command
		if (apusys_cmd != NULL)
			edma_engine->destroyCmd(apusys_cmd);

		// free apusys memory
		if (am_src_buff != NULL)
			edma_engine->memFree(am_src_buff);
		if (am_dst_buff != NULL)
			edma_engine->memFree(am_dst_buff);
		if (am_dmy_buff != NULL)
			edma_engine->memFree(am_dmy_buff);
		// free memory
		if (st_edma_task_info.edma_info != NULL)
			free(st_edma_task_info.edma_info);

		if (pOut_edma_desc != NULL)
			free(pOut_edma_desc);
		if (request.info_list != NULL) //=pst_info_nn
			free(request.info_list);

		delete edmaDesc_engine;
		DeviceEngine::deleteInstance(edma_engine);


}

#define TEST_D2S_NORMAL 0
#define DATA_TYPE2 1
unsigned char TEST_S2D = 1;

void test_edma_engine_nn_DepToSpace(unsigned int c, unsigned int block_sz, unsigned int width)
{
	DeviceEngine *edma_engine = DeviceEngine::createInstance("edma_engine");
	EdmaDescEngine* edmaDesc_engine = new EdmaDescEngine();

	std::vector<int> vec_mem_size;
	std::vector<SubCmdInfo *> vec_subcmd;
	std::vector<void *> vec_edma_task_info;

	st_edmaTaskInfo st_edma_task_info;

	st_edma_info_nn *pst_info_nn = NULL;

	st_edma_info_nn *ptrNN = NULL;


	edmaMem *am_src_buff = NULL, *am_dst_buff = NULL, *am_dmy_buff = NULL, *dsc_buff = NULL;
	edmaCmd *apusys_cmd = NULL;
	std::vector<int> apusys_sub_cmd_dependency;

	int i;

	unsigned char *ptr8 = NULL;

	unsigned int *ptr32 = NULL;
    unsigned int height = width;

	unsigned char uc_mode;
	unsigned int ui_src_data_size, ui_src_buff_size, ui_dst_data_size, ui_dst_buff_size;

	unsigned int ui_desc_num, ui_desc_idx, j;

	unsigned char *pOut_edma_desc = NULL;

	unsigned int ui_desc_buff_size;

	st_edma_request_nn request;
	st_edma_result_nn result;

	if (edma_engine == NULL) {
		printf("edma_engine = NULL\n");
		delete edmaDesc_engine;
		return;
	}

    if (block_sz ==  0){
		printf("edma_engine = NULL\n");
		delete edmaDesc_engine;
		DeviceEngine::deleteInstance(edma_engine);
		return;
	}
    if (c ==  0 || width == 0){
		printf("edma_engine = NULL\n");
		delete edmaDesc_engine;
		DeviceEngine::deleteInstance(edma_engine);
		return;
	}

	printf("%s_ c = %d, block sz = %d, width = %d \n", __func__,c,block_sz, width);


	//EDMA_DESC_NN
	st_edma_task_info.edma_info = NULL;
	request.info_list = NULL;

	result.cmd_size = 0;
	result.cmd_count = 0;

	ui_desc_num = 1;

	ui_desc_idx = 0;


	uc_mode = MODE_NORMAL;

    if (width*width*c < 100000) {
	  ui_src_data_size = 100000;
	  ui_dst_data_size = 100000;
    } else if (width == 720) {
        height = 1280;
        ui_src_data_size = height*width*c;
        ui_dst_data_size = height*width*c;
    } else if (width == 1440) {
        height = 2560;
        ui_src_data_size = height*width*c;
        ui_dst_data_size = height*width*c;
    } else if (width == 962) {
        height = 962;
        width = 6;
        ui_src_data_size = height*width*c;
        ui_dst_data_size = height*width*c;
    } else {
      ui_src_data_size = width*width*c;
      ui_dst_data_size = width*width*c;
    }

	ui_src_buff_size = ui_src_data_size;
	ui_dst_buff_size = ui_dst_data_size;

	printf("test_edma_engine S2D malloc, size = %d\n", ui_src_data_size);

	// allocate apusys memory
	am_src_buff = edma_engine->memAlloc(ui_src_buff_size*sizeof(unsigned char));
	am_dst_buff = edma_engine->memAlloc(ui_dst_buff_size*sizeof(unsigned char));

	am_dmy_buff = edma_engine->memAlloc(ui_src_buff_size*sizeof(unsigned char));

	if (am_src_buff == NULL)
		goto EXIT_PROCESS;

	if (am_dst_buff == NULL)
		goto EXIT_PROCESS;

	if (am_dmy_buff == NULL)
		goto EXIT_PROCESS;

	// output buffer address
	printf("am_src_buff->va: 0x%p\n", am_src_buff->va);
	//printf("am_src_buff->kva: 0x%X\n", (unsigned int)am_src_buff->kva);
	printf("am_src_buff->iova: 0x%u\n", (unsigned int)am_src_buff->iova);

	printf("am_dst_buff->va: 0x%p\n", am_dst_buff->va);
	//printf("am_dst_buff->kva: 0x%X\n", (unsigned int)am_dst_buff->kva);
	printf("am_dst_buff->iova: 0x%u\n", (unsigned int)am_dst_buff->iova);
	printf("\n");

	memset((unsigned char *)am_src_buff->va, 0x69, ui_src_buff_size);
	memset((unsigned char *)am_dst_buff->va, 0x3, ui_dst_buff_size);

	memset((unsigned char *)am_dmy_buff->va, 0, ui_src_buff_size);


	/*type array & info array for rquest total size*/
	//st_edma_task_info.puc_desc_type = (unsigned char *)calloc(st_edma_task_info.ui_desc_num, sizeof(unsigned char));

	pst_info_nn = (st_edma_info_nn *)calloc(ui_desc_num, sizeof(st_edma_info_nn));

	if (pst_info_nn == NULL)
		goto EXIT_PROCESS;

#if (TEST_D2S_NORMAL == 1)
	printf("test_edma_engine TEST_D2S_NORMAL\n");

	for (ui_desc_idx = 0; ui_desc_idx < ui_desc_num; ui_desc_idx++) {

		//pst_curr_desc_info = (void *)((uintptr_t)&st_edma_task_info.edma_info[ui_desc_idx].info.infoNN;
		ptrNN = &pst_info_nn[ui_desc_idx];

		//pst_info_nn->uc_desc_type = EDMA_INFO_NN;
		ptrNN->inFormat = EDMA_FMT_DEPTHTOSPACE;
		ptrNN->outFormat = EDMA_FMT_DEPTHTOSPACE;
		ptrNN->shape_c = c;
		ptrNN->shape_w = width;
		ptrNN->shape_h = height;
		ptrNN->shape_n = 1;

		ptrNN->src_stride_c = c;
		ptrNN->dst_stride_c = c/(block_sz*block_sz);
		ptrNN->src_stride_w = ptrNN->shape_w*c;
		ptrNN->dst_stride_w = (ptrNN->shape_w*c)/block_sz;

		ptrNN->src_stride_h = ptrNN->shape_w*ptrNN->shape_h*c;
		ptrNN->dst_stride_h = ptrNN->shape_w*ptrNN->shape_h*c;

		ptrNN->dst_type_size = 1;
		ptrNN->src_type_size = 1;
        ptrNN->block_size = block_sz;

	}
#else

	for (ui_desc_idx = 0; ui_desc_idx < ui_desc_num; ui_desc_idx++) {

    if (TEST_S2D == 1) {

        ptrNN = &pst_info_nn[ui_desc_idx];

        //pst_info_nn->uc_desc_type = EDMA_INFO_NN;
        ptrNN->inFormat = EDMA_FMT_S2D_FAST;
        ptrNN->outFormat = EDMA_FMT_S2D_FAST;
        ptrNN->shape_c = c;
        ptrNN->shape_w = width;
        ptrNN->shape_h = height;
        ptrNN->shape_n = 1;

        ptrNN->src_stride_c = c;
        ptrNN->dst_stride_c = c/(block_sz*block_sz);
        ptrNN->src_stride_w = ptrNN->shape_w*c;
        ptrNN->dst_stride_w = (ptrNN->shape_w*c)/block_sz;

        ptrNN->src_stride_h = ptrNN->shape_w*ptrNN->shape_h*c;
        ptrNN->dst_stride_h = ptrNN->shape_w*ptrNN->shape_h*c;

        ptrNN->dst_type_size = 1;
        ptrNN->src_type_size = 1;
        ptrNN->block_size = block_sz;


        } else  {
		//pst_curr_desc_info = (void *)((uintptr_t)&st_edma_task_info.edma_info[ui_desc_idx].info.infoNN;
		ptrNN = &pst_info_nn[ui_desc_idx];

		//pst_info_nn->uc_desc_type = EDMA_INFO_NN;
		ptrNN->inFormat = EDMA_FMT_DEPTHTOSPACE_FAST;
		ptrNN->outFormat = EDMA_FMT_DEPTHTOSPACE_FAST;
		ptrNN->shape_c = c;
		ptrNN->shape_w = width;
		ptrNN->shape_h = height;
		ptrNN->shape_n = 1;

		ptrNN->src_stride_c = c;
		ptrNN->dst_stride_c = c/(block_sz*block_sz);
		ptrNN->src_stride_w = ptrNN->shape_w*c;
		ptrNN->dst_stride_w = (ptrNN->shape_w*c)/block_sz;

		ptrNN->src_stride_h = ptrNN->shape_w*ptrNN->shape_h*c;
		ptrNN->dst_stride_h = ptrNN->shape_w*ptrNN->shape_h*c;

		ptrNN->dst_type_size = 1;
		ptrNN->src_type_size = 1;
        ptrNN->block_size = block_sz;
}
#if (DATA_TYPE2 == 2)
        ptrNN->dst_type_size = 2;
        ptrNN->src_type_size = 2;
#endif


	}

#endif
    ptr8 = (unsigned char *)am_src_buff->va;
    for (i = 0 ; i < (ptrNN->shape_c*ptrNN->shape_w*ptrNN->shape_h*ptrNN->dst_type_size); i++)
        *(ptr8+i) = i%(0xFF);

	request.info_list = pst_info_nn;
	request.info_num = ui_desc_num;

	edmaDesc_engine->queryDescSize(&request, &result);

	ui_desc_buff_size = result.cmd_size;

	printf("cmd_size = %d, cmd_count = %d\r\n", result.cmd_size, result.cmd_count);

    ui_desc_num = result.cmd_count;

	if (result.cmd_size == 0)
		goto EXIT_PROCESS;

	pOut_edma_desc = (unsigned char *)malloc(result.cmd_size);

	if (pOut_edma_desc == NULL)
		goto EXIT_PROCESS;

	edmaDesc_engine->fillDesc(&request, pOut_edma_desc); //need to fill edma request (edma_ext)

	ptr32 = (unsigned int *)pOut_edma_desc;

	//ptr8 = (unsigned char *)pOut_edma_desc;
 	for (ui_desc_idx = 0; ui_desc_idx < ui_desc_num; ui_desc_idx++) {
        unsigned int singleDes_size = result.cmd_size/result.cmd_count;

        printf("singleDes_size #2 = %d\r\n", singleDes_size);

		if (ui_desc_idx == 0){

		pst_info_nn =  &pst_info_nn[ui_desc_idx];
			printf("desc_id = %d src_addr_offset = 0x%x, dst_addr_offset = 0x%x, dummy offset = %d, shape_c = %d #2\r\n", 
				pst_info_nn->desc_id,
				pst_info_nn->src_addr_offset,
				pst_info_nn->dst_addr_offset,
				pst_info_nn->dummy_addr_offset,
				pst_info_nn->shape_c);

            if (pst_info_nn->src_addr_offset == 0)
                printf("pst_info_nn->src_addr_offset = 0 ERROR!!!!!!!!\r\n");

            *(ptr32 + (ui_desc_idx * singleDes_size/4) + (pst_info_nn->src_addr_offset)/4) = (unsigned int)(am_src_buff->iova);
            *(ptr32 + (ui_desc_idx * singleDes_size/4) + (pst_info_nn->dst_addr_offset)/4) = (unsigned int)(am_dst_buff->iova);


			if (pst_info_nn->dummy_addr_offset != 0)
					*(ptr32 + (pst_info_nn->dummy_addr_offset)/4) = (unsigned int)(am_dmy_buff->iova);
            if (pst_info_nn->src_addr2_offset != 0) {
                printf("src_addr2_offset != 0 for fast!!!!!!!!\r\n");
                if (ptrNN->inFormat == EDMA_FMT_S2D_FAST) {
                    *(ptr32 + (ui_desc_idx * singleDes_size/4) + (pst_info_nn->src_addr2_offset)/4) = (unsigned int)((unsigned int)am_src_buff->iova + (unsigned int)(ptrNN->shape_w * ptrNN->src_type_size));

                } else {
                *(ptr32 + (ui_desc_idx * singleDes_size/4) + (pst_info_nn->src_addr2_offset)/4) = (unsigned int)(am_src_buff->iova + block_sz*block_sz);
                *(ptr32 + (ui_desc_idx * singleDes_size/4) + (pst_info_nn->dst_addr2_offset)/4) = (unsigned int)(am_dst_buff->iova + ptrNN->shape_w*(ptrNN->shape_c/block_sz)*ptrNN->src_type_size);
                }

            }
         }         else if (ui_desc_idx > 0) {

         *(ptr32 + (ui_desc_idx * singleDes_size/4) + (pst_info_nn->src_addr_offset)/4) = (unsigned int)(am_src_buff->iova + ui_desc_idx * ptrNN->shape_w * block_sz * block_sz);
         *(ptr32 + (ui_desc_idx * singleDes_size/4) + (pst_info_nn->dst_addr_offset)/4) = (unsigned int)(am_dst_buff->iova + ui_desc_idx * ptrNN->shape_w * block_sz * block_sz);


         }


		//(unsigned int *)(ptr32 + (pst_info_nn->src_addr_offset)/4) = (unsigned int)(am_src_buff->iova);




	}


	printf("check pOut_edma_desc\r\n");


    dumpDescriptor((unsigned int *)pOut_edma_desc, ui_desc_buff_size);


//Test send scriptror
	apusys_cmd = edma_engine->initCmd();
	if (apusys_cmd == NULL)
		goto EXIT_PROCESS;

	st_edma_task_info.info_num = request.info_num;
	st_edma_task_info.edma_info = (st_edmaUnifyInfo *)calloc(st_edma_task_info.info_num, sizeof(st_edmaUnifyInfo));

	if (st_edma_task_info.edma_info == NULL)
		goto EXIT_PROCESS;

	for (j=0; j< request.info_num; j++ )
	{
		memcpy(&st_edma_task_info.edma_info[j].st_info.infoNN, &request.info_list[j], sizeof(st_edma_info_nn));
		st_edma_task_info.edma_info[j].info_type = EDMA_INFO_NN;
	}

	// initialize apusys sub-command buffer
	vec_edma_task_info.push_back((void *)&st_edma_task_info);
	/*size[0] = request cmd size, size[1] = total descriptor size*/
	edma_engine->querySubCmdInfo(vec_edma_task_info, vec_mem_size);

	printf("vec_mem_size.size() = %d\n", (int)vec_mem_size.size());

	for (j=0; j<vec_mem_size.size(); j++)
	{
		SubCmdInfo *p_subcmd_info = new SubCmdInfo;
		printf("cmd[%d] size = %d\n", j, vec_mem_size[j] );
		p_subcmd_info->descMem = edma_engine->memAllocCmd(vec_mem_size[j]);
		vec_subcmd.push_back(p_subcmd_info);
	}
	dsc_buff = vec_subcmd[1]->descMem;

	ptr32 = (unsigned int *)dsc_buff->va;
	ptr8 = (unsigned char *)pOut_edma_desc;

	edma_engine->fillDesc(vec_edma_task_info, vec_subcmd);

	memcpy((unsigned char *)dsc_buff->va, ptr8, vec_mem_size[1]);

    printf("vec_mem_size[1] = %d\n", vec_mem_size[1]);
	for (i=0; i<vec_mem_size[1]/4; i++)
	{
		printf("dsc_buff[%x] = 0x%x\n", i*4, ptr32[i] );
	}
    apusys_cmd->addSubCmd(vec_subcmd);

	// start execution
	edma_engine->runCmd(apusys_cmd);

	// comparison
	ptr8 = (unsigned char *)am_dst_buff->va;

    for(j=0; j<4; j++) {
        printf("[%2d]: ", j);
        if (TEST_S2D != 1) {
                for(i=(int)(j*width*ptrNN->dst_type_size*c/2); i<(int)((j+1)*width*ptrNN->dst_type_size*c/2); i++)
        		    printf("%3d,", *(ptr8+i));
          } else {
                for(i=(int)(j*width*ptrNN->dst_type_size*c*2); i<(int)((j+1)*width*ptrNN->dst_type_size*c*2); i++)
		printf("%3d,", *(ptr8+i));
        }
          printf("\n");          
    }
    if (TEST_S2D == 1) {

    for(j=(height-1); j<height; j++) {
        printf("[%2d]: ", j);
        for(i=(int)(j*width); i<(int)((j+1)*width); i++) {
		printf("%3d,", *(ptr8+i));
        }
        printf("\n");
    }
    }else {
    if (height * 2 > 4) {
    for(j=(height-1)*2; j<height*2; j++) {
        printf("[%2d]: ", j);
            for(i=(int)(j*width*c/2); i<(int)((j+1)*width*c/2); i++) {
		printf("%3d,", *(ptr8+i));
        }
        printf("\n");
    }
    }
    }
EXIT_PROCESS:
		// destroy apusys command


        printf("EXIT_PROCESS in \n");
		if (apusys_cmd != NULL)
			edma_engine->destroyCmd(apusys_cmd);

		// free apusys memory
		if (am_src_buff != NULL)
			edma_engine->memFree(am_src_buff);
		if (am_dst_buff != NULL)
			edma_engine->memFree(am_dst_buff);
		if (am_dmy_buff != NULL)
			edma_engine->memFree(am_dmy_buff);


		// free memory
		if (st_edma_task_info.edma_info != NULL)
			free(st_edma_task_info.edma_info);

		if (pOut_edma_desc != NULL)
			free(pOut_edma_desc);
		if (request.info_list != NULL) //=pst_info_nn
			free(request.info_list);

		delete edmaDesc_engine;
		DeviceEngine::deleteInstance(edma_engine);


}

void test_edma_engine_nn_new(void)
{
	DeviceEngine *edma_engine = DeviceEngine::createInstance("edma_engine");
	EdmaDescEngine* edmaDesc_engine = new EdmaDescEngine();

	std::vector<int> vec_mem_size;
	std::vector<SubCmdInfo *> vec_subcmd;
	std::vector<void *> vec_edma_task_info;

	st_edmaTaskInfo st_edma_task_info;

	st_edma_info_nn *pst_info_nn = NULL;

	st_edma_info_nn *ptrNN = NULL;

	edmaMem *am_src_buff = NULL, *am_dst_buff = NULL, *sub_cmd_buff = NULL, *dsc_buff = NULL, *am_mid_buff = NULL;
	edmaCmd *apusys_cmd = NULL;
	std::vector<int> apusys_sub_cmd_dependency;

	int i;

	unsigned char *ptr8 = NULL;

	unsigned int *ptr32 = NULL;

	unsigned char uc_mode;
	unsigned int ui_src_data_size, ui_src_buff_size, ui_dst_data_size, ui_dst_buff_size;

	unsigned int ui_desc_num, ui_desc_idx;

	unsigned char *pOut_edma_desc = NULL;
	unsigned int ui_desc_buff_size, j;

	st_edma_request_nn request;
	st_edma_result_nn result;


	if (edma_engine == NULL) {
	    printf("edma_engine = NULL\n");
		delete edmaDesc_engine;
		return;
	}

	printf("test_edma_engine_nn_new\n");

	printf("size of st_edmaUnifyInfo = %d #2\n", (int)sizeof(st_edmaUnifyInfo));

	st_edma_task_info.edma_info = NULL;
	request.info_list = NULL;
	result.cmd_size = 0;
	result.cmd_count = 0;

	//EDMA_DESC_NN


	ui_desc_num = 2;

	ui_desc_idx = 0;


	uc_mode = MODE_NORMAL;

	ui_src_data_size = 1000;

	ui_dst_data_size = 1000;

	ui_src_buff_size = ui_src_data_size;

	ui_dst_buff_size = ui_dst_data_size;



	printf("test_edma_engine malloc\n");

	// allocate apusys memory
	am_src_buff = edma_engine->memAlloc(ui_src_buff_size*sizeof(unsigned char));
	am_dst_buff = edma_engine->memAlloc(ui_dst_buff_size*sizeof(unsigned char));
	am_mid_buff = edma_engine->memAlloc(ui_dst_buff_size*sizeof(unsigned char));

	if (am_src_buff == NULL || am_dst_buff == NULL || am_mid_buff == NULL)
		goto EXIT_PROCESS;

	// output buffer address
	printf("am_src_buff->va: 0x%p\n", am_src_buff->va);
	//printf("am_src_buff->kva: 0x%X\n", (unsigned int)am_src_buff->kva);
	printf("am_src_buff->iova: 0x%u\n", (unsigned int)am_src_buff->iova);
	printf("\n");
	printf("am_dst_buff->va: 0x%p\n", am_dst_buff->va);
	//printf("am_dst_buff->kva: 0x%X\n", (unsigned int)am_dst_buff->kva);
	printf("am_dst_buff->iova: 0x%u\n", (unsigned int)am_dst_buff->iova);
	printf("\n");

	memset((unsigned char *)am_src_buff->va, 0x69, ui_src_buff_size);
	memset((unsigned char *)am_dst_buff->va, 0x3, ui_dst_buff_size);
	memset((unsigned char *)am_mid_buff->va, 0x77, ui_dst_buff_size);

	/*type array & info array for rquest total size*/
	//st_edma_task_info.puc_desc_type = (unsigned char *)calloc(st_edma_task_info.ui_desc_num, sizeof(unsigned char));

	pst_info_nn = (st_edma_info_nn *)calloc(ui_desc_num, sizeof(st_edma_info_nn));

	if (pst_info_nn == NULL)
		goto EXIT_PROCESS;

	for (ui_desc_idx = 0; ui_desc_idx < ui_desc_num; ui_desc_idx++) {
		//pst_curr_desc_info = (void *)((uintptr_t)&st_edma_task_info.edma_info[ui_desc_idx].info.infoNN;
		ptrNN = &pst_info_nn[ui_desc_idx];

		//pst_info_nn->uc_desc_type = EDMA_INFO_NN;
		ptrNN->inFormat = EDMA_FMT_NORMAL;
		ptrNN->outFormat = EDMA_FMT_NORMAL;
		ptrNN->shape_c = 1000;
		ptrNN->shape_w = 1;
		ptrNN->shape_h = 1;
		ptrNN->shape_n = 1;

		ptrNN->src_stride_c = 1000;
		ptrNN->dst_stride_c = 1000;
		ptrNN->src_stride_w = 1000;
		ptrNN->dst_stride_w = 1000;

		ptrNN->src_stride_h = 1; //don't care?
		ptrNN->dst_stride_h = 1;

		ptrNN->dst_type_size = 1;
		ptrNN->src_type_size = 1;
	}

	request.info_list = pst_info_nn;
	request.info_num = ui_desc_num;

	edmaDesc_engine->queryDescSize(&request, &result);

	ui_desc_buff_size = result.cmd_size;

	printf("cmd_size = %d, cmd_count = %d\r\n", result.cmd_size, result.cmd_count);

	if (result.cmd_size == 0)
		goto EXIT_PROCESS;

	pOut_edma_desc = (unsigned char *)malloc(result.cmd_size);

	if (pOut_edma_desc == NULL)
		goto EXIT_PROCESS;

	edmaDesc_engine->fillDesc(&request, pOut_edma_desc); //need to fill edma request (edma_ext)

	ptr32 = (unsigned int *)pOut_edma_desc;

	//ptr8 = (unsigned char *)pOut_edma_desc;
 	for (ui_desc_idx = 0; ui_desc_idx < ui_desc_num; ui_desc_idx++) {

		ptrNN =  &pst_info_nn[ui_desc_idx];
		printf("desc_id = %d src_addr_offset = %d, dst_addr_offset = %d, shape_c = %d\r\n",
			ptrNN->desc_id,
			ptrNN->src_addr_offset,
			ptrNN->dst_addr_offset,
			ptrNN->shape_c);

		printf("am_src_buff = 0x%llu, am_mid_buff = 0x%llu\r\n",
			am_src_buff->iova,
			am_mid_buff->iova);
		if (ui_desc_idx == 0) {
			//(unsigned int *)(ptr32 + (pst_info_nn->src_addr_offset)/4) = (unsigned int)(am_src_buff->iova);
			*(ptr32 + (ptrNN->src_addr_offset)/4) = (unsigned int)(am_src_buff->iova);

			*(ptr32 + (ptrNN->dst_addr_offset)/4) = (unsigned int)(am_mid_buff->iova);
		} else {
			*(ptr32 + 64/4 + (ptrNN->src_addr_offset)/4) = (unsigned int)(am_mid_buff->iova);
			*(ptr32 + 64/4 + (ptrNN->dst_addr_offset)/4) = (unsigned int)(am_dst_buff->iova);
		}
	}

	for (j = 0; j < ui_desc_buff_size/4; j++)
		printf("pOut_edma_desc[0x%x] = 0x%x\r\n", j*4, ptr32[j]);

//Test send scriptror
	apusys_cmd = edma_engine->initCmd();
	if (apusys_cmd == NULL)
		goto EXIT_PROCESS;

	st_edma_task_info.info_num = request.info_num;
	st_edma_task_info.edma_info = (st_edmaUnifyInfo *)calloc(st_edma_task_info.info_num, sizeof(st_edmaUnifyInfo));

	if (st_edma_task_info.edma_info == NULL)
		goto EXIT_PROCESS;

	for (j=0; j< request.info_num; j++ )
	{
		memcpy(&st_edma_task_info.edma_info[j].st_info.infoNN, &request.info_list[j], sizeof(st_edma_info_nn));
		st_edma_task_info.edma_info[j].info_type = EDMA_INFO_NN;
	}

	// initialize apusys sub-command buffer
	vec_edma_task_info.push_back((void *)&st_edma_task_info);
	/*size[0] = request cmd size, size[1] = total descriptor size*/
	edma_engine->querySubCmdInfo(vec_edma_task_info, vec_mem_size);

	printf("vec_mem_size.size() = %d\n", (int)vec_mem_size.size());

	for (j=0; j<vec_mem_size.size(); j++) {
		SubCmdInfo *p_subcmd_info = new SubCmdInfo;
		printf("cmd[%d] size = %d\n", j, vec_mem_size[j]);
		p_subcmd_info->descMem = edma_engine->memAlloc(vec_mem_size[j]);
		vec_subcmd.push_back(p_subcmd_info);
	}
	dsc_buff = vec_subcmd[1]->descMem;

	ptr32 = (unsigned int *)dsc_buff->va;
	ptr8 = (unsigned char *)pOut_edma_desc;

	edma_engine->fillDesc(vec_edma_task_info, vec_subcmd);

	memcpy((unsigned char *)dsc_buff->va, ptr8, vec_mem_size[1]);

	for (i=0; i<vec_mem_size[1]/4; i++)
		printf("dsc_buff[%x] = 0x%x\n", i*4, ptr32[i] );

	sub_cmd_buff = vec_subcmd[0]->descMem;
	apusys_sub_cmd_dependency.clear();
	apusys_cmd->addSubCmd(sub_cmd_buff, APUSYS_DEVICE_EDMA, apusys_sub_cmd_dependency);


	// start execution
	edma_engine->runCmd(apusys_cmd);

	// comparison
	ptr8 = (unsigned char *)am_mid_buff->va;

	for(i=0; i<5; i++)
		printf("am_mid_buff data ptr8[%d] = 0x%x\n", i , *(ptr8+i));

	// comparison
	ptr8 = (unsigned char *)am_dst_buff->va;
	for(i=0; i<5; i++)
		printf("am_dst_buff data ptr8[%d] = 0x%x\n", i , *(ptr8+i));

EXIT_PROCESS:
		// destroy apusys command
		if (apusys_cmd != NULL)
			edma_engine->destroyCmd(apusys_cmd);

		// free apusys memory
		if (am_src_buff != NULL)
			edma_engine->memFree(am_src_buff);
		if (am_dst_buff != NULL)
			edma_engine->memFree(am_dst_buff);
		if (am_mid_buff != NULL)
			edma_engine->memFree(am_mid_buff);

		if (pOut_edma_desc != NULL)
			free(pOut_edma_desc);

		// free memory
		if (st_edma_task_info.edma_info != NULL)
			free(st_edma_task_info.edma_info);

		if (request.info_list != NULL)
			free(request.info_list);

		delete edmaDesc_engine;
		DeviceEngine::deleteInstance(edma_engine);
}

#define TEST_COPY_SIZE 1024*1024
void test_edma_engineCopy(void)
{
	DeviceEngine *edma_engine = DeviceEngine::createInstance("edma_engine");
	std::vector<int> vec_mem_size;
	std::vector<SubCmdInfo *> vec_subcmd;
	std::vector<void *> vec_edma_task_info;

	st_edmaTaskInfo st_edma_task_info;
	void *pst_curr_desc_info = NULL;

	st_edma_InfoData *edma_info_copy = NULL;

	edmaMem *am_src_buff = NULL, *am_dst_buff = NULL, *sub_cmd_buff = NULL, *dsc_buff = NULL;
	edmaCmd *apusys_cmd = NULL;
	std::vector<int> apusys_sub_cmd_dependency;

	int i;
	unsigned char *ptr8 = NULL;
	unsigned int *ptr32 = NULL, j;

	unsigned char uc_mode;
	unsigned int ui_src_data_size, ui_src_buff_size, ui_src_buff_offset = 0, ui_dst_data_size, ui_dst_buff_size;

	unsigned int ui_desc_num, ui_desc_idx;

	if (edma_engine == NULL) {
		printf("edma_engine = NULL\n");
		return;
	}

	printf("test_edma_engineCopy\n");

	ui_desc_num = 1;
	ui_desc_idx = 0;
	st_edma_task_info.edma_info = NULL;


	uc_mode = MODE_NORMAL;
	ui_src_data_size = TEST_COPY_SIZE;
	ui_dst_data_size = TEST_COPY_SIZE;
	ui_src_buff_size = ui_src_data_size;
	ui_dst_buff_size = ui_dst_data_size;

	// memory allocation

	printf("test_edma_engineCopy malloc\n");

	// allocate apusys memory
	am_src_buff = edma_engine->memAlloc(ui_src_buff_size*sizeof(unsigned char));
	am_dst_buff = edma_engine->memAlloc(ui_dst_buff_size*sizeof(unsigned char));

	if (am_src_buff == NULL || am_dst_buff == NULL )
		goto EXIT_PROCESS;

	st_edma_task_info.info_num = ui_desc_num;

	// output buffer address
	printf("am_src_buff->va: 0x%p\n", am_src_buff->va);
	//printf("am_src_buff->kva: 0x%X\n", am_src_buff->kva);
	printf("am_src_buff->iova: 0x%X\n", (unsigned int)am_src_buff->iova);
	printf("\n");
	printf("am_dst_buff->va: 0x%p\n", am_dst_buff->va);
	//printf("am_dst_buff->kva: 0x%X\n", (unsigned int)am_dst_buff->kva);
	printf("am_dst_buff->iova: 0x%X\n", (unsigned int)am_dst_buff->iova);
	printf("\n");

	memset((unsigned char *)am_src_buff->va, 0x69, ui_src_buff_size);
	memset((unsigned char *)am_dst_buff->va, 0x3, ui_dst_buff_size);

	/*type array & info array for rquest total size*/
	//st_edma_task_info.puc_desc_type = (unsigned char *)calloc(st_edma_task_info.ui_desc_num, sizeof(unsigned char));
	st_edma_task_info.edma_info = (st_edmaUnifyInfo *)calloc(st_edma_task_info.info_num, sizeof(st_edmaUnifyInfo));

	if (st_edma_task_info.edma_info == NULL)
		goto EXIT_PROCESS;

	pst_curr_desc_info = &st_edma_task_info.edma_info->st_info.infoData;
	st_edma_task_info.edma_info[ui_desc_idx].info_type = EDMA_INFO_DATA;
	edma_info_copy = (st_edma_InfoData *)pst_curr_desc_info;
	edma_info_copy->uc_desc_type = EDMA_INFO_DATA;
	edma_info_copy->inBuf_addr = (unsigned int)(am_src_buff->iova+ui_src_buff_offset*(ui_desc_idx&MULT_DESC_SRC_IDX_MASK)); //2^7 = 128 bytes
	edma_info_copy->outBuf_addr = (unsigned int)am_dst_buff->iova;
	edma_info_copy->inFormat = EDMA_FMT_NORMAL;
	edma_info_copy->outFormat = EDMA_FMT_NORMAL;
	edma_info_copy->copy_size = TEST_COPY_SIZE;

	//pst_curr_desc_info = (void *)((uintptr_t)pst_curr_desc_info + (uintptr_t)sizeof(_edma20_InputShape));
	apusys_cmd = edma_engine->initCmd();
	if (apusys_cmd == NULL)
		goto EXIT_PROCESS;

	// initialize apusys sub-command buffer
	vec_edma_task_info.push_back((void *)&st_edma_task_info);
	/*size[0] = request cmd size, size[1] = total descriptor size*/
	edma_engine->querySubCmdInfo(vec_edma_task_info, vec_mem_size);

	printf("vec_mem_size.size() = %d\n", (int)vec_mem_size.size());

	for (j=0; j<vec_mem_size.size(); j++) {
		SubCmdInfo *p_subcmd_info = new SubCmdInfo;
		printf("cmd[%d] size = %d\n", j, vec_mem_size[j] );
		p_subcmd_info->descMem = edma_engine->memAlloc(vec_mem_size[j]);
		vec_subcmd.push_back(p_subcmd_info);
	}

	edma_engine->fillDesc(vec_edma_task_info, vec_subcmd);

	sub_cmd_buff = vec_subcmd[0]->descMem;
	apusys_sub_cmd_dependency.clear();
	apusys_cmd->addSubCmd(sub_cmd_buff, APUSYS_DEVICE_EDMA, apusys_sub_cmd_dependency);

	dsc_buff = vec_subcmd[1]->descMem;

	ptr32 = (unsigned int *)dsc_buff->va;
	ptr8 = (unsigned char *)dsc_buff->va;

	for (i=0; i<30; i++)
		printf("dsc_buff[%x] = 0x%x\n", i*4, ptr32[i] );

		
	// start execution
	edma_engine->runCmd(apusys_cmd);

	// comparison
	ptr8 = (unsigned char *)am_dst_buff->va;

	for(i=0; i<30; i++)
		printf("Golden data ptr8[%d] = 0x%x\n", i , *(ptr8+i));

	for (i=0; i<TEST_COPY_SIZE; i++) {
		if (ptr8[i] != 0x69) {
			printf("Golden data ptr8[%d] = 0x%x, error !!!!!\n", i , *(ptr8+i));
			break;
		}
	}
	if (i == TEST_COPY_SIZE)
		printf("test total size [%d] Pass!!\n", i);

EXIT_PROCESS:
	// destroy apusys command
	if (apusys_cmd != NULL)
		edma_engine->destroyCmd(apusys_cmd);

	// free apusys memory
	if (am_src_buff != NULL)
		edma_engine->memFree(am_src_buff);
	if (am_dst_buff != NULL)
		edma_engine->memFree(am_dst_buff);
	for (j=0; j<vec_mem_size.size(); j++) {
		SubCmdInfo *p_subcmd_info = vec_subcmd[j];
		edma_engine->memFree(p_subcmd_info->descMem);
		delete p_subcmd_info;
		p_subcmd_info = NULL;
	}
	vec_mem_size.clear();
	vec_subcmd.clear();
	vec_edma_task_info.clear();

	DeviceEngine::deleteInstance(edma_engine);
	// free memory
	if (st_edma_task_info.edma_info != NULL)
		free(st_edma_task_info.edma_info);

	printf("test_edma_engineCopy free done\n");
}

void test_edma_engineMM_DYN2(char *yuvFile, unsigned int yuvWidth, EDMA_FMT_E angle)
{
	DeviceEngine *edma_engine = DeviceEngine::createInstance("edma_engine");
	std::vector<int> vec_mem_size;
	std::vector<SubCmdInfo *> vec_subcmd;
	std::vector<void *> vec_edma_task_info;

	st_edmaTaskInfo st_edma_task_info;
	void *pst_curr_desc_info = NULL;

	st_edma_InputShape *pInputShape;

	edmaMem *am_src_buff = NULL, *am_dst_buff = NULL, *sub_cmd_buff = NULL, *dsc_buff = NULL;
	edmaCmd *apusys_cmd = NULL;
	std::vector<int> apusys_sub_cmd_dependency;
	edmaMem *am_dst2_buff = NULL;

	int i;

	unsigned char *ptr8 = NULL;

	unsigned int wlens, j;

	FILE *pFile = NULL;

	unsigned char uc_mode;
	unsigned int ui_src_data_size, ui_src_buff_size, ui_dst_data_size, ui_dst_buff_size;

	unsigned int ui_desc_num, info_index = 0;

	char RfileName[50];

	if (edma_engine == NULL) {
		printf("edma_engine = NULL\n");
		return;
	}

	printf("test_edma_engineMM_DYN2 yuvFile = %s, yuvWidth = %d, angle = %d\n", yuvFile, yuvWidth, angle);

	st_edma_task_info.edma_info = NULL;

	if (angle != EDMA_FMT_NORMAL)
		ui_desc_num = 2;
	else
		ui_desc_num = 1;

	info_index = 0;


	uc_mode = MODE_NORMAL;
	ui_src_data_size = yuvWidth*yuvWidth*2; //yuv = w*h*2
	ui_dst_data_size = yuvWidth*yuvWidth*4;
	ui_src_buff_size = ui_src_data_size;
	ui_dst_buff_size = ui_dst_data_size;
	// memory allocation

	// allocate apusys memory
	am_src_buff = edma_engine->memAlloc(ui_src_buff_size*sizeof(unsigned char));
	am_dst_buff = edma_engine->memAlloc(ui_dst_buff_size*sizeof(unsigned char));

	am_dst2_buff = edma_engine->memAlloc(ui_dst_buff_size*sizeof(unsigned char));

	if (am_src_buff == NULL || am_dst_buff == NULL || am_dst2_buff == NULL)
		goto EXIT_PROCESS;

	st_edma_task_info.info_num = ui_desc_num;

	// output buffer address
	printf("am_src_buff->va: 0x%p\n", am_src_buff->va);
	//printf("am_src_buff->kva: 0x%X\n", (unsigned int)am_src_buff->kva);
	printf("am_src_buff->iova: 0x%X\n", (unsigned int)am_src_buff->iova);
	printf("\n");
	printf("am_dst_buff->va: 0x%p\n", am_dst_buff->va);
	//printf("am_dst_buff->kva: 0x%X\n", (unsigned int)am_dst_buff->kva);
	printf("am_dst_buff->iova: 0x%X\n", (unsigned int)am_dst_buff->iova);
	printf("\n");

	//memset((unsigned char *)am_src_buff->va, 0x69, ui_src_buff_size);
	//memcpy((unsigned char *)am_src_buff->va, yuv_data_256, ui_src_buff_size);

	ptr8 = (unsigned char *)am_src_buff->va;

	if (yuvWidth <= 512) {
		readFileToBuf(yuvFile, ptr8, yuvWidth*yuvWidth*2);
	} else {
		printf("YUV_WIDTH = %d not support !!\n", yuvWidth);
		goto EXIT_PROCESS;
	}
	memset((unsigned char *)am_dst_buff->va, 0x3, ui_dst_buff_size);

	/*type array & info array for rquest total size*/
	//st_edma_task_info.puc_desc_type = (unsigned char *)calloc(st_edma_task_info.ui_desc_num, sizeof(unsigned char));
	st_edma_task_info.edma_info = (st_edmaUnifyInfo *)calloc(st_edma_task_info.info_num, sizeof(st_edmaUnifyInfo));

	if (st_edma_task_info.edma_info == NULL)
		goto EXIT_PROCESS;

	pst_curr_desc_info = &st_edma_task_info.edma_info[info_index].st_info.infoShape;
	st_edma_task_info.edma_info[info_index].info_type = EDMA_INFO_GENERAL;
	pInputShape = (st_edma_InputShape *)pst_curr_desc_info;
	pInputShape->uc_desc_type = EDMA_INFO_GENERAL;
	pInputShape->inBuf_addr = (unsigned int)(am_src_buff->iova); //2^7 = 128 bytes
	pInputShape->outBuf_addr = (unsigned int)am_dst_buff->iova;
	pInputShape->inFormat = EDMA_FMT_YUV422_8B;
	pInputShape->outFormat = angle;
	pInputShape->size_x = yuvWidth*2;
	pInputShape->size_y = yuvWidth;
	pInputShape->size_z = 1;

	pInputShape->dst_stride_x = yuvWidth*2;
	pInputShape->dst_stride_y = 1;


	info_index++;

	if (ui_desc_num > 1) {
		pst_curr_desc_info = &st_edma_task_info.edma_info[info_index].st_info.infoShape;
		st_edma_task_info.edma_info[info_index].info_type = EDMA_INFO_GENERAL;
		pInputShape = (st_edma_InputShape *)pst_curr_desc_info;
		pInputShape->uc_desc_type = EDMA_INFO_GENERAL;
		pInputShape->inBuf_addr = (unsigned int)(am_dst_buff->iova); //2^7 = 128 bytes
		pInputShape->outBuf_addr = (unsigned int)am_dst2_buff->iova;
		pInputShape->inFormat = EDMA_FMT_YUV422_8B;
		pInputShape->outFormat = EDMA_FMT_ARGB_8;
		pInputShape->size_x = yuvWidth*2;
		pInputShape->size_y = yuvWidth;
		pInputShape->size_z = 1;

		pInputShape->dst_stride_x = yuvWidth*4;
		pInputShape->dst_stride_y = yuvWidth;  // do not set 1 !!!?

	}
	apusys_cmd = edma_engine->initCmd();
	if (apusys_cmd == NULL)
		goto EXIT_PROCESS;

	// initialize apusys sub-command buffer
	vec_edma_task_info.push_back((void *)&st_edma_task_info);
	/*size[0] = request cmd size, size[1] = total descriptor size*/
	edma_engine->querySubCmdInfo(vec_edma_task_info, vec_mem_size);

	printf("vec_mem_size.size() = %d\n", (int)vec_mem_size.size());

	for (j=0; j<vec_mem_size.size(); j++) {
		SubCmdInfo *p_subcmd_info = new SubCmdInfo;
		printf("cmd[%d] size = %d\n", j, vec_mem_size[j] );
		p_subcmd_info->descMem = edma_engine->memAlloc(vec_mem_size[j]);
		vec_subcmd.push_back(p_subcmd_info);
	}

	edma_engine->fillDesc(vec_edma_task_info, vec_subcmd);

	sub_cmd_buff = vec_subcmd[0]->descMem;
	apusys_sub_cmd_dependency.clear();
	apusys_cmd->addSubCmd(sub_cmd_buff, APUSYS_DEVICE_EDMA, apusys_sub_cmd_dependency);

	dsc_buff = vec_subcmd[1]->descMem;

	dumpDescriptor((unsigned int *)dsc_buff->va, vec_mem_size[1]);

	// start execution
	edma_engine->runCmd(apusys_cmd);

	// comparison
	ptr8 = (unsigned char *)am_dst_buff->va;

	for(i=0; i<10; i++)
		printf("Convert data ptr8[%d] = 0x%x\n", i , *(ptr8+i));


	pFile = fopen("/mnt/yuv_rotate.raw", "wb");
	if (pFile == NULL) {
		printf("Open /mnt/yuv_rotate.raw fail!!\n");
	} else {
		wlens = fwrite(ptr8, sizeof(char), yuvWidth*yuvWidth*2, pFile);
		if (wlens != yuvWidth*yuvWidth*2)
			printf("fwrite fail!!\n");

		if (fclose(pFile) != 0)
			printf("fclose fail!!\n");
	}

	// comparison
	ptr8 = (unsigned char *)am_dst2_buff->va;


	if (angle == EDMA_FMT_ROTATE90)
		strncpy(RfileName,"/mnt/rgba_r90.raw",50);
	else if (angle == EDMA_FMT_ROTATE180)
		strncpy(RfileName,"/mnt/rgba_r180.raw",50);
	else
		strncpy(RfileName,"/mnt/rgba_r270.raw",50);

	pFile = fopen(RfileName, "wb");

	if (pFile == NULL) {
		printf("Open %s fail!! = NULL\n", RfileName);
	} else {
		wlens = fwrite(ptr8, sizeof(char), yuvWidth*yuvWidth*4, pFile);

		if (wlens != yuvWidth*yuvWidth*4)
			printf("fwrite fail!!\n");
		if (fclose(pFile) != 0)
			printf("fclose %s fail!!\n", RfileName);
	}

EXIT_PROCESS:
	// destroy apusys command
	if (apusys_cmd != NULL)
		edma_engine->destroyCmd(apusys_cmd);

	// free apusys memory
	if (am_src_buff != NULL)
		edma_engine->memFree(am_src_buff);
	if (am_dst_buff != NULL)
		edma_engine->memFree(am_dst_buff);
	for (j=0; j<vec_mem_size.size(); j++) {
		SubCmdInfo *p_subcmd_info = vec_subcmd[j];
		edma_engine->memFree(p_subcmd_info->descMem);
		delete p_subcmd_info;
		p_subcmd_info = NULL;
	}
	vec_mem_size.clear();
	vec_subcmd.clear();
	vec_edma_task_info.clear();

	DeviceEngine::deleteInstance(edma_engine);

	// free memory
	if (st_edma_task_info.edma_info != NULL)
		free(st_edma_task_info.edma_info);
}

#define UV2X
#define EXCEED_SIZE_CHECK 100
void test_edma_engineMM_YUV(unsigned int Width, unsigned int Height, unsigned int factor)
{

	DeviceEngine *edma_engine = DeviceEngine::createInstance("edma_engine");
	std::vector<int> vec_mem_size;
	std::vector<SubCmdInfo *> vec_subcmd;
	std::vector<void *> vec_edma_task_info;

	st_edmaTaskInfo st_edma_task_info;
	void *pst_curr_desc_info = NULL;

	st_edma_InputShape *pInputShape = NULL;

	edmaMem *am_src_buff = NULL, *am_dst_buff = NULL, *sub_cmd_buff = NULL, *dsc_buff = NULL;
	edmaCmd *apusys_cmd = NULL;
	std::vector<int> apusys_sub_cmd_dependency;

	int i;
	unsigned char *ptr8 = NULL;
	unsigned int wlens, j;

	FILE *pFile = NULL;

	unsigned char uc_mode;
	unsigned int ui_src_data_size, ui_src_buff_size, ui_src_buff_offset = 0, ui_dst_data_size, ui_dst_buff_size;

	unsigned int ui_desc_num, ui_desc_idx;

	if (edma_engine == NULL) {
		printf("edma_engine = NULL\n");
		return;
	}

	printf("test_edma_engineMM_EDMA_FMT_UVResz2X, factor = %d\n", factor);

	st_edma_task_info.edma_info = NULL;

	ui_desc_num = 1;
	ui_desc_idx = 0;

	uc_mode = MODE_NORMAL;
	//ui_src_data_size = sizeof(yuv_data_256); //yuv = w*h*2
	ui_src_data_size = 2*Width/2*Height/2;
	ui_dst_data_size = 2*Width*Height+EXCEED_SIZE_CHECK;
	ui_src_buff_size = ui_src_data_size;
	ui_dst_buff_size = ui_dst_data_size;

	// memory allocation
	printf("test_edma_engine UV2X, src size = %d, dst size = %d\n", ui_src_data_size, ui_dst_data_size);


	// allocate apusys memory
	am_src_buff = edma_engine->memAlloc(ui_src_buff_size*sizeof(unsigned char));
	am_dst_buff = edma_engine->memAlloc(ui_dst_buff_size*sizeof(unsigned char));

	if (am_src_buff == NULL || am_dst_buff == NULL )
		goto EXIT_PROCESS;


	st_edma_task_info.info_num = ui_desc_num;

	// output buffer address
	printf("am_src_buff->va: 0x%p\n", am_src_buff->va);
	//printf("am_src_buff->kva: 0x%X\n", (unsigned int)am_src_buff->kva);
	printf("am_src_buff->iova: 0x%X\n", (unsigned int)am_src_buff->iova);
	printf("\n");
	printf("am_dst_buff->va: 0x%p\n", am_dst_buff->va);
	//printf("am_dst_buff->kva: 0x%X\n", (unsigned int)am_dst_buff->kva);
	printf("am_dst_buff->iova: 0x%X\n", (unsigned int)am_dst_buff->iova);
	printf("\n");
#ifndef UV2X
	//memset((unsigned char *)am_src_buff->va, 0x69, ui_src_buff_size);
	memcpy((unsigned char *)am_src_buff->va, yuv_data_256, ui_src_buff_size);
	memset((unsigned char *)am_dst_buff->va, 0x3, ui_dst_buff_size);
#else
	memset((unsigned char *)am_dst_buff->va, 0x3, ui_dst_buff_size);
    ptr8 = (unsigned char *)am_src_buff->va;
    readFileToBuf("/mnt/UV.bin", ptr8, ui_src_data_size);
#endif
	/*type array & info array for rquest total size*/
	//st_edma_task_info.puc_desc_type = (unsigned char *)calloc(st_edma_task_info.ui_desc_num, sizeof(unsigned char));
	st_edma_task_info.edma_info = (st_edmaUnifyInfo *)calloc(st_edma_task_info.info_num, sizeof(st_edmaUnifyInfo));

	if (st_edma_task_info.edma_info == NULL)
		goto EXIT_PROCESS;

	pst_curr_desc_info = &st_edma_task_info.edma_info->st_info.infoShape;
	st_edma_task_info.edma_info[ui_desc_idx].info_type = EDMA_INFO_GENERAL;
	pInputShape = (st_edma_InputShape *)pst_curr_desc_info;
	pInputShape->uc_desc_type = EDMA_INFO_GENERAL;
	pInputShape->inBuf_addr = (unsigned int)(am_src_buff->iova+ui_src_buff_offset*(ui_desc_idx&MULT_DESC_SRC_IDX_MASK)); //2^7 = 128 bytes
	pInputShape->outBuf_addr = (unsigned int)am_dst_buff->iova;
	pInputShape->inFormat = EDMA_FMT_UVResz2X;
	pInputShape->outFormat = EDMA_FMT_UVResz2X;
	pInputShape->size_x = Width;
	pInputShape->size_y = Height;
	pInputShape->size_z = 1;
    pInputShape->tileNum = factor;

	pInputShape->dst_stride_x = Width*2;
	pInputShape->dst_stride_y = Height*2;  // do not set 1 !!!?

	apusys_cmd = edma_engine->initCmd();
	if (apusys_cmd == NULL)
		goto EXIT_PROCESS;

	// initialize apusys sub-command buffer
	vec_edma_task_info.push_back((void *)&st_edma_task_info);
	/*size[0] = request cmd size, size[1] = total descriptor size*/
	edma_engine->querySubCmdInfo(vec_edma_task_info, vec_mem_size);

	printf("vec_mem_size.size() = %d\n", (int)vec_mem_size.size());

	for (j=0; j<vec_mem_size.size(); j++) {
		SubCmdInfo *p_subcmd_info = new SubCmdInfo;
		printf("cmd[%d] size = %d\n", j, vec_mem_size[j] );
		p_subcmd_info->descMem = edma_engine->memAllocCmd(vec_mem_size[j]);
		vec_subcmd.push_back(p_subcmd_info);
	}


	edma_engine->fillDesc(vec_edma_task_info, vec_subcmd);

	sub_cmd_buff = vec_subcmd[0]->descMem;
	apusys_sub_cmd_dependency.clear();
    apusys_cmd->addSubCmd(vec_subcmd);


	dsc_buff = vec_subcmd[1]->descMem;

	printf("dumpDescriptor\n");

	dumpDescriptor((unsigned int *)dsc_buff->va, vec_mem_size[1]);

	// start execution
	edma_engine->runCmd(apusys_cmd);

	// comparison
	ptr8 = (unsigned char *)am_dst_buff->va;

	for(i=0; i<100; i++)
		printf("Convert data ptr8[%d] = 0x%x\n", i , *(ptr8+i));

	pFile = fopen("/mnt/uv2x.bin", "wb");
	if (pFile == NULL) {
		printf("Open /mnt/uv2x.bin fail!! = NULL\n");
	} else {
		wlens = fwrite(ptr8, sizeof(char), Width*Height*2+EXCEED_SIZE_CHECK, pFile);

		if (wlens != Width*Height*2)
			printf("fwrite fail!!\n");

		if (fclose(pFile) != 0)
			printf("fclose fail!!\n");
	}

EXIT_PROCESS:
	// destroy apusys command
	if (apusys_cmd != NULL)
			edma_engine->destroyCmd(apusys_cmd);

	// free apusys memory
	if (am_src_buff != NULL)
		edma_engine->memFree(am_src_buff);
	if (am_dst_buff != NULL)
		edma_engine->memFree(am_dst_buff);
	for (j=0; j<vec_mem_size.size(); j++) {
		SubCmdInfo *p_subcmd_info = vec_subcmd[j];
		edma_engine->memFree(p_subcmd_info->descMem);
		delete p_subcmd_info;
		p_subcmd_info = NULL;
	}
	vec_mem_size.clear();
	vec_subcmd.clear();
	vec_edma_task_info.clear();


	DeviceEngine::deleteInstance(edma_engine);

	// free memory
	if (st_edma_task_info.edma_info != NULL)
		free(st_edma_task_info.edma_info);
}

void test_edma_engineMM_SDK(void)
{

	DeviceEngine *edma_engine = DeviceEngine::createInstance("edma_engine");
	std::vector<int> vec_mem_size;
	std::vector<SubCmdInfo *> vec_subcmd;
	std::vector<void *> vec_edma_task_info;

	st_edmaTaskInfo st_edma_task_info;
	void *pst_curr_desc_info = NULL;

	st_edma_InfoSDK *pInputSdk = NULL;

	st_sdk_typeGnl aicarSdk_Gnl;
	st_sdk_typeGnl aicarSdk_Gnl_2;

	edmaMem *am_src_buff = NULL, *am_dst_buff = NULL, *sub_cmd_buff = NULL, *dsc_buff = NULL;
	edmaCmd *apusys_cmd = NULL;
	std::vector<int> apusys_sub_cmd_dependency;

	unsigned int i;

	unsigned char uc_mode;
	unsigned int ui_src_data_size, ui_src_buff_size, ui_dst_data_size, ui_dst_buff_size;

	unsigned int ui_desc_num, ui_desc_idx;

	if (edma_engine == NULL) {
		printf("edma_engine = NULL\n");
		return;
	}

	printf("test_edma_engineMM_SDK\n");

	st_edma_task_info.edma_info = NULL;

	ui_desc_num = 2;
	ui_desc_idx = 0;

	uc_mode = MODE_NORMAL;
	ui_src_data_size = sizeof(yuv_data_256); //yuv = w*h*2
	ui_dst_data_size = 256*256*4;
	ui_src_buff_size = ui_src_data_size;
	ui_dst_buff_size = ui_dst_data_size;

	// memory allocation

	// allocate apusys memory
	am_src_buff = edma_engine->memAlloc(ui_src_buff_size*sizeof(unsigned char));
	am_dst_buff = edma_engine->memAlloc(ui_dst_buff_size*sizeof(unsigned char));

	if (am_src_buff == NULL || am_dst_buff == NULL )
		goto EXIT_PROCESS;

	st_edma_task_info.info_num = ui_desc_num;

	// output buffer address
	printf("am_src_buff->va: 0x%p\n", am_src_buff->va);
	//printf("am_src_buff->kva: 0x%X\n", (unsigned int)am_src_buff->kva);
	printf("am_src_buff->iova: 0x%X\n", (unsigned int)am_src_buff->iova);
	printf("\n");
	printf("am_dst_buff->va: 0x%p\n", am_dst_buff->va);
	//printf("am_dst_buff->kva: 0x%X\n", (unsigned int)am_dst_buff->kva);
	printf("am_dst_buff->iova: 0x%X\n", (unsigned int)am_dst_buff->iova);
	printf("\n");

	//memset((unsigned char *)am_src_buff->va, 0x69, ui_src_buff_size);
	memcpy((unsigned char *)am_src_buff->va, yuv_data_256, ui_src_buff_size);
	memset((unsigned char *)am_dst_buff->va, 0x3, ui_dst_buff_size);

	/*type array & info array for rquest total size*/
	//st_edma_task_info.puc_desc_type = (unsigned char *)calloc(st_edma_task_info.ui_desc_num, sizeof(unsigned char));
	st_edma_task_info.edma_info = (st_edmaUnifyInfo *)calloc(st_edma_task_info.info_num, sizeof(st_edmaUnifyInfo));

	if (st_edma_task_info.edma_info == NULL)
		goto EXIT_PROCESS;

	pst_curr_desc_info = &st_edma_task_info.edma_info->st_info.infoSDK;
	st_edma_task_info.edma_info[ui_desc_idx].info_type = EDMA_INFO_SDK;
	pInputSdk = (st_edma_InfoSDK *)pst_curr_desc_info;

	aicarSdk_Gnl.ui_desp_00_type = 0;
	/*.....
	  designe edma function related command here
	*/

	pInputSdk->sdk_type = EDMA_SDK_TYPE_GNL;
	pInputSdk->pSdkInfo = (unsigned char *)&aicarSdk_Gnl;
	pInputSdk->sdkInfoSize = sizeof(aicarSdk_Gnl);

	if (ui_desc_num == 2) {
		ui_desc_idx++;
		pst_curr_desc_info = &st_edma_task_info.edma_info[ui_desc_idx].st_info.infoSDK;
		st_edma_task_info.edma_info[ui_desc_idx].info_type = EDMA_INFO_SDK;
		pInputSdk = (st_edma_InfoSDK *)pst_curr_desc_info;

		aicarSdk_Gnl_2.ui_desp_00_type = 0;
		/*.....
		  designe edma function related command
        */
		pInputSdk->sdk_type = EDMA_SDK_TYPE_GNL;
		pInputSdk->pSdkInfo = (unsigned char *)&aicarSdk_Gnl_2;
		pInputSdk->sdkInfoSize = sizeof(aicarSdk_Gnl_2);

	}
		
	apusys_cmd = edma_engine->initCmd();
	if (apusys_cmd == NULL)
		goto EXIT_PROCESS;

	// initialize apusys sub-command buffer
	vec_edma_task_info.push_back((void *)&st_edma_task_info);
	/*size[0] = request cmd size, size[1] = total descriptor size*/
	edma_engine->querySubCmdInfo(vec_edma_task_info, vec_mem_size);

	printf("vec_mem_size.size() = %d\n", (int)vec_mem_size.size());

	for (i=0; i<vec_mem_size.size(); i++) {
		SubCmdInfo *p_subcmd_info = new SubCmdInfo;
		printf("cmd[%d] size = %d\n", i, vec_mem_size[i] );
		p_subcmd_info->descMem = edma_engine->memAlloc(vec_mem_size[i]);
		vec_subcmd.push_back(p_subcmd_info);
	}

	edma_engine->fillDesc(vec_edma_task_info, vec_subcmd);

	sub_cmd_buff = vec_subcmd[0]->descMem;
	apusys_sub_cmd_dependency.clear();
	apusys_cmd->addSubCmd(sub_cmd_buff, APUSYS_DEVICE_EDMA, apusys_sub_cmd_dependency);

	dsc_buff = vec_subcmd[1]->descMem;

	dumpDescriptor((unsigned int *)dsc_buff->va, vec_mem_size[1]);

	// start execution
	//edma_engine->runCmd(apusys_cmd);

#if 0
	// comparison
	ptr8 = (unsigned char *)am_dst_buff->va;

	for(i=0; i<100; i++)
		printf("Convert data ptr8[%d] = 0x%x\n", i , *(ptr8+i));

	pFile = fopen("/mnt/rgba.bin", "wb");
	if (pFile == NULL) {
		printf("Open /mnt/rgba.raw fail!! = NULL\n");
	} else {
		wlens = fwrite(ptr8, sizeof(char), 256*256*4, pFile);

		if (wlens != 256*256*4)
			printf("fwrite fail!!\n");

		if (fclose(pFile) != 0)
			printf("fclose fail!!\n");
	}
#endif

EXIT_PROCESS:
	// destroy apusys command
	if (apusys_cmd != NULL)
			edma_engine->destroyCmd(apusys_cmd);

	// free apusys memory
	if (am_src_buff != NULL)
		edma_engine->memFree(am_src_buff);
	if (am_dst_buff != NULL)
		edma_engine->memFree(am_dst_buff);
	for (i=0; i<vec_mem_size.size(); i++) {
		SubCmdInfo *p_subcmd_info = vec_subcmd[i];
		edma_engine->memFree(p_subcmd_info->descMem);
		delete p_subcmd_info;
		p_subcmd_info = NULL;
	}
	vec_mem_size.clear();
	vec_subcmd.clear();
	vec_edma_task_info.clear();


	DeviceEngine::deleteInstance(edma_engine);

	// free memory
	if (st_edma_task_info.edma_info != NULL)
		free(st_edma_task_info.edma_info);
}

//#define ROTATE_ONLY

void test_edma_engineRotate(EDMA_FMT_E fmt)
{

	DeviceEngine *edma_engine = DeviceEngine::createInstance("edma_engine");
	std::vector<int> vec_mem_size;
	std::vector<SubCmdInfo *> vec_subcmd;
	std::vector<void *> vec_edma_task_info;

	st_edmaTaskInfo st_edma_task_info;
	void *pst_curr_desc_info = NULL;

	st_edma_InputShape *pInputShape = NULL;

	edmaMem *am_src_buff = NULL, *am_dst_buff = NULL, *am_dst2_buff = NULL, *sub_cmd_buff = NULL, *dsc_buff = NULL;
	edmaCmd *apusys_cmd = NULL;
	std::vector<int> apusys_sub_cmd_dependency;

	int i, info_index = 0;
	unsigned char *ptr8 = NULL;
	unsigned int wlens;

	unsigned char uc_mode;
	unsigned int ui_src_data_size, ui_src_buff_size, ui_dst_data_size, ui_dst_buff_size;

	unsigned int ui_desc_num, ui_desc_idx;
	FILE *pFile = NULL;
	char RfileName[50];

	if (edma_engine == NULL) {
		printf("edma_engine = NULL\n");
		return;
	}

	printf("test_edma_engine convert and Rotate = %d\n", fmt);

	st_edma_task_info.edma_info = NULL;

#ifndef ROTATE_ONLY
	ui_desc_num = 2;
#else
	ui_desc_num = 1;
#endif

	ui_desc_idx = 0;

	uc_mode = MODE_NORMAL;
	ui_src_data_size = sizeof(yuv_data_256); //yuv = w*h*2
	ui_dst_data_size = 512*1024;
	ui_src_buff_size = ui_src_data_size;
	ui_dst_buff_size = ui_dst_data_size;
	// memory allocation

	printf("test_edma_engine Rotatae fmt = %d\n",fmt);

	// allocate apusys memory
	am_src_buff = edma_engine->memAlloc(ui_src_buff_size*sizeof(unsigned char));
	am_dst_buff = edma_engine->memAlloc(ui_dst_buff_size*sizeof(unsigned char));
	am_dst2_buff = edma_engine->memAlloc(ui_dst_buff_size*sizeof(unsigned char));

	if (am_src_buff == NULL || am_dst_buff == NULL || am_dst2_buff == NULL)
		goto EXIT_PROCESS;

	st_edma_task_info.info_num = ui_desc_num;

	// output buffer address
	printf("am_src_buff->va: 0x%p\n", am_src_buff->va);
	//printf("am_src_buff->kva: 0x%X\n", (unsigned int)am_src_buff->kva);
	printf("am_src_buff->iova: 0x%X\n", (unsigned int)am_src_buff->iova);
	printf("\n");
	printf("am_dst_buff->va: 0x%p\n", am_dst_buff->va);
	//printf("am_dst_buff->kva: 0x%X\n", (unsigned int)am_dst_buff->kva);
	printf("am_dst_buff->iova: 0x%X\n", (unsigned int)am_dst_buff->iova);
	printf("\n");

	memcpy((unsigned char *)am_src_buff->va, yuv_data_256, ui_src_buff_size);
	memset((unsigned char *)am_dst_buff->va, 0x3, ui_dst_buff_size);
	memset((unsigned char *)am_dst2_buff->va, 0x77, ui_dst_buff_size);

	st_edma_task_info.edma_info = (st_edmaUnifyInfo *)calloc(st_edma_task_info.info_num, sizeof(st_edmaUnifyInfo));


	if (st_edma_task_info.edma_info == NULL)
		goto EXIT_PROCESS;

#ifndef ROTATE_ONLY

	pst_curr_desc_info = &st_edma_task_info.edma_info[info_index].st_info.infoShape;
	st_edma_task_info.edma_info[info_index].info_type = EDMA_INFO_GENERAL;
	pInputShape = (st_edma_InputShape *)pst_curr_desc_info;
	pInputShape->uc_desc_type = EDMA_INFO_GENERAL;
	pInputShape->inBuf_addr = (unsigned int)(am_src_buff->iova); //2^7 = 128 bytes
	pInputShape->outBuf_addr = (unsigned int)am_dst_buff->iova;
	pInputShape->inFormat = EDMA_FMT_YUV422_8B;
	pInputShape->outFormat = EDMA_FMT_ARGB_8;
	pInputShape->size_x = 256*2;
	pInputShape->size_y = 256;
	pInputShape->size_z = 1;

	pInputShape->dst_stride_x = 256*4;
	pInputShape->dst_stride_y = 256;  // do not set 1 !!!?

	info_index++;

#endif

	/*type array & info array for rquest total size*/
	//st_edma_task_info.puc_desc_type = (unsigned char *)calloc(st_edma_task_info.ui_desc_num, sizeof(unsigned char));

	pst_curr_desc_info = &st_edma_task_info.edma_info[info_index].st_info.infoShape;
	st_edma_task_info.edma_info[info_index].info_type = EDMA_INFO_GENERAL;
	pInputShape = (st_edma_InputShape *)pst_curr_desc_info;
	pInputShape->uc_desc_type = EDMA_INFO_GENERAL;
	pInputShape->inBuf_addr = (unsigned int)(am_dst_buff->iova); //2^7 = 128 bytes
	pInputShape->outBuf_addr = (unsigned int)am_dst2_buff->iova;
	pInputShape->inFormat = EDMA_FMT_ARGB_8;
	pInputShape->outFormat = fmt;
	pInputShape->size_x = 256*4;
	pInputShape->size_y = 256;
	pInputShape->size_z = 1;

	pInputShape->dst_stride_x = 256*4;
	pInputShape->dst_stride_y = 1;


	//pst_curr_desc_info = (void *)((uintptr_t)pst_curr_desc_info + (uintptr_t)sizeof(_edma20_InputShape));

	apusys_cmd = edma_engine->initCmd();

	if (apusys_cmd == NULL)
		goto EXIT_PROCESS;

	// initialize apusys sub-command buffer
	vec_edma_task_info.push_back((void *)&st_edma_task_info);
	/*size[0] = request cmd size, size[1] = total descriptor size*/
	edma_engine->querySubCmdInfo(vec_edma_task_info, vec_mem_size);

	printf("vec_mem_size.size() = %d\n", (int)vec_mem_size.size());

	for (i=0; i<(int)vec_mem_size.size(); i++) {
		SubCmdInfo *p_subcmd_info = new SubCmdInfo;
		printf("cmd[%d] size = %d\n", i, vec_mem_size[i] );
		p_subcmd_info->descMem = edma_engine->memAllocCmd(vec_mem_size[i]);
		vec_subcmd.push_back(p_subcmd_info);
	}

	edma_engine->fillDesc(vec_edma_task_info, vec_subcmd);

	sub_cmd_buff = vec_subcmd[0]->descMem;
	apusys_sub_cmd_dependency.clear();
	apusys_cmd->addSubCmd(vec_subcmd);

	dsc_buff = vec_subcmd[1]->descMem;

	dumpDescriptor((unsigned int *)dsc_buff->va, vec_mem_size[1]);

#ifdef ROTATE_ONLY
	ptr8 = (unsigned char *)am_dst_buff->va;

	readFileToBuf("/mnt/071645411-0087-0000_256x256_rgba8888.bin", ptr8, 256*1024);
#endif

	// start execution
	edma_engine->runCmd(apusys_cmd);

	// comparison
	ptr8 = (unsigned char *)am_dst2_buff->va;

	for(i=0; i<10; i++)
		printf("Rotated data ptr8[%d] = 0x%x\n", i , *(ptr8+i));


	if (fmt == EDMA_FMT_ROTATE90)
		strncpy(RfileName,"/mnt/rgba_r90.bin",50);
	else if (fmt == EDMA_FMT_ROTATE180)
		strncpy(RfileName,"/mnt/rgba_r180.bin",50);
	else
		strncpy(RfileName,"/mnt/rgba_r270.bin",50);

	pFile = fopen(RfileName, "wb");

	if (pFile == NULL) {
		printf("Open %s fail!! = NULL\n", RfileName);
	} else {
		wlens = fwrite(ptr8, sizeof(char), 256*256*4, pFile);

		if (wlens != 256*256*4)
			printf("fwrite fail!!\n");

		if (fclose(pFile) != 0)
			printf("fclose %s fail!!\n", RfileName);
	}

EXIT_PROCESS:
	// destroy apusys command
	if (apusys_cmd != NULL)
		edma_engine->destroyCmd(apusys_cmd);
	
	// free apusys memory
	if (am_src_buff != NULL)
		edma_engine->memFree(am_src_buff);
	if (am_dst_buff != NULL)
		edma_engine->memFree(am_dst_buff);
	for (i=0; i<(int)vec_mem_size.size(); i++) {
		SubCmdInfo *p_subcmd_info = vec_subcmd[i];
		edma_engine->memFree(p_subcmd_info->descMem);
		delete p_subcmd_info;
		p_subcmd_info = NULL;
	}
	vec_mem_size.clear();
	vec_subcmd.clear();
	vec_edma_task_info.clear();

	DeviceEngine::deleteInstance(edma_engine);

	// free memory
	if (st_edma_task_info.edma_info != NULL)
		free(st_edma_task_info.edma_info);
}

void test_edma_engine(void)
{
	DeviceEngine *edma_engine = DeviceEngine::createInstance("edma_engine");
	std::vector<int> vec_mem_size;
	std::vector<SubCmdInfo *> vec_subcmd;
	std::vector<void *> vec_edma_task_info;

	st_edmaTaskInfo st_edma_task_info;
	void *pst_curr_desc_info = NULL;
	unsigned char *puc_golden_data = NULL, *puc_revised_data = NULL;

	st_edma_InputShape *pInputShape = NULL;

	edmaMem *am_src_buff = NULL, *am_dst_buff = NULL, *sub_cmd_buff = NULL, *dsc_buff = NULL;
	edmaCmd *apusys_cmd = NULL;
	std::vector<int> apusys_sub_cmd_dependency;

	unsigned int i;

	unsigned char *ptr8 = NULL;
	unsigned int *ptr32 = NULL;

	unsigned char uc_mode;
	unsigned int ui_src_data_size, ui_src_buff_size, ui_src_buff_offset = 0, ui_dst_data_size, ui_dst_buff_size;

	unsigned int ui_desc_num, ui_desc_idx;

	if (edma_engine == NULL) {
		printf("edma_engine = NULL\n");
		return;
	}

	printf("test_edma_engine\n");

	ui_desc_num = 1;
	ui_desc_idx = 0;
	st_edma_task_info.edma_info = NULL;

	uc_mode = MODE_NORMAL;
	ui_src_data_size = 1000;
	ui_dst_data_size = 1000;
	ui_src_buff_size = ui_src_data_size;
	ui_dst_buff_size = ui_dst_data_size;

	// memory allocation
	if ((puc_golden_data = (unsigned char *)malloc(ui_dst_data_size*sizeof(unsigned char))) == NULL) {
		printf("allocate memory buffer fail: puc_golden_data\n");
		goto EXIT_PROCESS;
	}
	if ((puc_revised_data = (unsigned char *)malloc(ui_dst_data_size*sizeof(unsigned char))) == NULL) {
		printf("allocate memory buffer fail: puc_revised_data\n");
		goto EXIT_PROCESS;
	}

	printf("test_edma_engine malloc\n");

	// allocate apusys memory
	am_src_buff = edma_engine->memAlloc(ui_src_buff_size*sizeof(unsigned char));
	am_dst_buff = edma_engine->memAlloc(ui_dst_buff_size*sizeof(unsigned char));


	if (am_src_buff == NULL || am_dst_buff == NULL )
		goto EXIT_PROCESS;

	st_edma_task_info.info_num = ui_desc_num;

	// output buffer address
	printf("am_src_buff->va: 0x%p\n", am_src_buff->va);
	//printf("am_src_buff->kva: 0x%X\n", (unsigned int)am_src_buff->kva);
	printf("am_src_buff->iova: 0x%X\n", (unsigned int)am_src_buff->iova);
	printf("\n");
	printf("am_dst_buff->va: 0x%p\n", am_dst_buff->va);
	//printf("am_dst_buff->kva: 0x%X\n", (unsigned int)am_dst_buff->kva);
	printf("am_dst_buff->iova: 0x%X\n", (unsigned int)am_dst_buff->iova);
	printf("\n");

	memset((unsigned char *)am_src_buff->va, 0x69, ui_src_buff_size);
	memset((unsigned char *)am_dst_buff->va, 0x3, ui_dst_buff_size);

	/*type array & info array for rquest total size*/
	//st_edma_task_info.puc_desc_type = (unsigned char *)calloc(st_edma_task_info.ui_desc_num, sizeof(unsigned char));
	st_edma_task_info.edma_info = (st_edmaUnifyInfo *)calloc(st_edma_task_info.info_num, sizeof(st_edmaUnifyInfo));

	if (st_edma_task_info.edma_info == NULL)
		goto EXIT_PROCESS;

	pst_curr_desc_info = &st_edma_task_info.edma_info->st_info.infoShape;
	st_edma_task_info.edma_info[ui_desc_idx].info_type = EDMA_INFO_GENERAL;
	pInputShape = (st_edma_InputShape *)pst_curr_desc_info;
	pInputShape->uc_desc_type = EDMA_INFO_GENERAL;
	pInputShape->inBuf_addr = (unsigned int)(am_src_buff->iova+ui_src_buff_offset*(ui_desc_idx&MULT_DESC_SRC_IDX_MASK)); //2^7 = 128 bytes
	pInputShape->outBuf_addr = (unsigned int)am_dst_buff->iova;
	pInputShape->inFormat = EDMA_FMT_NORMAL;
	pInputShape->outFormat = EDMA_FMT_NORMAL;
	pInputShape->size_x = 1000;
	pInputShape->size_y = 1;
	pInputShape->size_z = 1;

	pInputShape->dst_stride_x = 1000;
	pInputShape->dst_stride_y = 1;

	//pst_curr_desc_info = (void *)((uintptr_t)pst_curr_desc_info + (uintptr_t)sizeof(_edma20_InputShape));

	apusys_cmd = edma_engine->initCmd();

	if (apusys_cmd == NULL)
		goto EXIT_PROCESS;

	// initialize apusys sub-command buffer
	vec_edma_task_info.push_back((void *)&st_edma_task_info);
	/*size[0] = request cmd size, size[1] = total descriptor size*/
	edma_engine->querySubCmdInfo(vec_edma_task_info, vec_mem_size);

	printf("vec_mem_size.size() = %d\n", (int)vec_mem_size.size());

	for (i=0; i<vec_mem_size.size(); i++) {
		SubCmdInfo *p_subcmd_info = new SubCmdInfo;
		printf("cmd[%d] size = %d\n", i, vec_mem_size[i] );
		p_subcmd_info->descMem = edma_engine->memAlloc(vec_mem_size[i]);
		vec_subcmd.push_back(p_subcmd_info);
	}

	edma_engine->fillDesc(vec_edma_task_info, vec_subcmd);

	sub_cmd_buff = vec_subcmd[0]->descMem;
	apusys_sub_cmd_dependency.clear();
	apusys_cmd->addSubCmd(sub_cmd_buff, APUSYS_DEVICE_EDMA, apusys_sub_cmd_dependency);

	dsc_buff = vec_subcmd[1]->descMem;

	ptr32 =  (unsigned int *)dsc_buff->va;
	for (i=0; i<20; i++)
		printf("dsc_buff[%x] = 0x%x\n", i*4, ptr32[i] );

	// start execution
	edma_engine->runCmd(apusys_cmd);

	// comparison
	ptr8 = (unsigned char *)am_dst_buff->va;

	for(i=0; i<10; i++)
		printf("Golden data ptr8[%d] = 0x%x\n", i , *(ptr8+i));


EXIT_PROCESS:
	// destroy apusys command
	if (apusys_cmd != NULL)
		edma_engine->destroyCmd(apusys_cmd);

	// free apusys memory
	if (am_src_buff != NULL)
		edma_engine->memFree(am_src_buff);
	if (am_dst_buff != NULL)
		edma_engine->memFree(am_dst_buff);
	for (i=0; i<vec_mem_size.size(); i++) {
		SubCmdInfo *p_subcmd_info = vec_subcmd[i];
		edma_engine->memFree(p_subcmd_info->descMem);
		delete p_subcmd_info;
		p_subcmd_info = NULL;
	}
	vec_mem_size.clear();
	vec_subcmd.clear();
	vec_edma_task_info.clear();

	DeviceEngine::deleteInstance(edma_engine);

	// free memory
	if (st_edma_task_info.edma_info != NULL)
		free(st_edma_task_info.edma_info);

	if (puc_golden_data != NULL)
		free(puc_golden_data);
	if (puc_revised_data != NULL)
		free(puc_revised_data);
}

// EDMA UFBC feature test
// factor = 0, ufbc encode only
//   input file formate = argb
// facotr = 1, ufbc decode only
//   input file formate = the encoded file of argb
// factor = 2, ufbc encode and decode
// image size should be square (w x w)
void test_edma_engine_UFBC(char const *File, unsigned int Width, unsigned int Height, unsigned int encode, 
	unsigned int tileNumX, unsigned int tileNumY)
{

	DeviceEngine *edma_engine = DeviceEngine::createInstance("edma_engine");
	std::vector<int> vec_mem_size;
	std::vector<SubCmdInfo *> vec_subcmd;
	std::vector<void *> vec_edma_task_info;

	st_edmaTaskInfo st_edma_task_info;
	void *pst_curr_desc_info;

	st_edma_InputShape *pInputShape;

	edmaMem *am_src_buff, *am_dst_buff, *dsc_buff;
	edmaCmd *apusys_cmd = NULL;
	std::vector<int> apusys_sub_cmd_dependency;
	edmaMem *am_dst2_buff;

	unsigned int i;

	unsigned char *ptr8 = NULL;
	unsigned char *ptr8_src;

	unsigned int *ptr32, x, y;

	FILE *pFile;

	unsigned char uc_mode;
	unsigned int ui_src_data_size, ui_src_buff_size, ui_dst_data_size, ui_dst_buff_size;

	unsigned int ui_desc_num, info_index = 0;

	char RfileName[50];
	unsigned int dump_size;

	if (edma_engine == NULL) {
		printf("edma_engine = NULL\n");
		return;
	}



	printf("%s UFBC Width = %d, Height = %d, encode = %d\n",__func__, Width, Height, encode);

	printf("%s rgb tileNumX = %d, tileNumY = %d\n",__func__, tileNumX, tileNumY);

	if (encode == 2)
		ui_desc_num = 2;
	else
		ui_desc_num = 1;
	info_index = 0;

	st_edma_task_info.edma_info = NULL;
	st_edma_task_info.info_num = ui_desc_num;

	uc_mode = MODE_NORMAL;
	ui_src_data_size = Width*Height*4;
	ui_dst_data_size = Width*Height*4;
	ui_src_buff_size = ui_src_data_size;
	ui_dst_buff_size = ui_dst_data_size + Width*Height / 16; // hw max write length

	// memory allocation

	// allocate apusys memory
	am_src_buff = edma_engine->memAlloc(ui_src_buff_size*sizeof(unsigned char));
	am_dst_buff = edma_engine->memAlloc(2*ui_dst_buff_size*sizeof(unsigned char)); //reserved haeder size
	am_dst2_buff = edma_engine->memAlloc(ui_src_buff_size*sizeof(unsigned char));

	if (am_src_buff == NULL)
		goto EXIT_PROCESS;

	if (am_dst_buff == NULL)
		goto EXIT_PROCESS;

	if (am_dst2_buff == NULL)
		goto EXIT_PROCESS;


	// output buffer address
	printf("am_src_buff->va: 0x%p\n", am_src_buff->va);
	//printf("am_src_buff->kva: 0x%X\n", (unsigned int)am_src_buff->kva);
	printf("am_src_buff->iova: 0x%X\n", (unsigned int)am_src_buff->iova);
	printf("\n");
	printf("am_dst_buff->va: 0x%p\n", am_dst_buff->va);
	//printf("am_dst_buff->kva: 0x%X\n", (unsigned int)am_dst_buff->kva);
	printf("am_dst_buff->iova: 0x%X\n", (unsigned int)am_dst_buff->iova);
	printf("\n");
	printf("am_dst2_buff->va: 0x%p\n", am_dst2_buff->va);

	//memset((unsigned char *)am_src_buff->va, 0x69, ui_src_buff_size);
	//memcpy((unsigned char *)am_src_buff->va, yuv_data_256, ui_src_buff_size);

	if (encode == 1 || encode == 5) {
		ptr8 = (unsigned char *)am_dst_buff->va;
		readFileToBuf(File, ptr8, ui_dst_buff_size);
	} else {
		ptr8 = (unsigned char *)am_src_buff->va;
	    if (encode != 6) {

			if (Width <= 512) {
				readFileToBuf(File, ptr8, ui_src_buff_size);
			} else {
				printf("RGBA_WIDTH = %d not support !!\n", Width);
				goto EXIT_PROCESS;
			}
	    } else { //encode = 6
				memset((unsigned char *)ptr8, 0x0, ui_src_buff_size);
		}
		memset((unsigned char *)am_dst_buff->va, 0x0, ui_dst_buff_size);
	}

	memset((unsigned char *)am_dst2_buff->va, 0x0, ui_src_buff_size);
	printf("%s #2\r\n", __func__);

	/*type array & info array for rquest total size*/
	//st_edma_task_info.puc_desc_type = (unsigned char *)calloc(st_edma_task_info.ui_desc_num, sizeof(unsigned char));
	st_edma_task_info.edma_info = (st_edmaUnifyInfo *)calloc(st_edma_task_info.info_num, sizeof(st_edmaUnifyInfo));
	if (st_edma_task_info.edma_info == NULL)
		goto EXIT_PROCESS;


	if (encode == 0 || encode == 2 || encode == 6 ) {
		pst_curr_desc_info = &st_edma_task_info.edma_info->st_info.infoShape;
		st_edma_task_info.edma_info[info_index].info_type = EDMA_INFO_UFBC;
		pInputShape = (st_edma_InputShape *)pst_curr_desc_info;
		pInputShape->uc_desc_type = EDMA_UFBC_ENCODE;
		pInputShape->inBuf_addr = (unsigned int)am_src_buff->iova; //2^7 = 128 bytes
		pInputShape->outBuf_addr = (unsigned int)am_dst_buff->iova;
		pInputShape->inFormat = EDMA_FMT_ARGB_8;
		pInputShape->outFormat = EDMA_FMT_ARGB_8;
		pInputShape->size_x = Width;
		pInputShape->size_y = Height;
		pInputShape->size_z = 1;
		pInputShape->dst_stride_x = Width;
		pInputShape->dst_stride_y = 0;
		pInputShape->cropShape.x_offset = 0;
		pInputShape->cropShape.y_offset = 0;
		pInputShape->cropShape.x_size = Width;
		pInputShape->cropShape.y_size = Height;

		pInputShape->dst_stride_x = Width;
		pInputShape->dst_stride_y = Height;  // do not set 1 !!!?

		info_index++;
	}
	if (encode == 5 || encode == 1 || ui_desc_num > 1) {
		pst_curr_desc_info = &st_edma_task_info.edma_info[info_index].st_info.infoShape;
		st_edma_task_info.edma_info[info_index].info_type = EDMA_INFO_UFBC;
		pInputShape = (st_edma_InputShape *)pst_curr_desc_info;
		pInputShape->uc_desc_type = EDMA_UFBC_DECODE;
		pInputShape->inBuf_addr = (unsigned int)am_dst_buff->iova; //2^7 = 128 bytes
		pInputShape->outBuf_addr = (unsigned int)am_dst2_buff->iova;
		pInputShape->inFormat = EDMA_FMT_ARGB_8;
		pInputShape->outFormat = EDMA_FMT_ARGB_8;
		pInputShape->size_x = Width;
		pInputShape->size_y = Height;
		pInputShape->size_z = 1;
		pInputShape->dst_stride_x = Width;
		pInputShape->dst_stride_y = 0;
		pInputShape->cropShape.x_offset = 0;
		pInputShape->cropShape.y_offset = 0;
		pInputShape->cropShape.x_size = Width;
		pInputShape->cropShape.y_size = Height;

		pInputShape->dst_stride_x = Width;
		pInputShape->dst_stride_y = Height;
	}
	printf("%s #3\r\n", __func__);

	if (encode == 5 || encode == 6) {


	for (x=0; x < tileNumX; x++)
		for (y=0; y < tileNumY; y++)
			{
	            //tile decode
				if (encode == 5 || encode == 6) {
					pInputShape->cropShape.x_offset = x * (Width/tileNumX);
					pInputShape->cropShape.y_offset = y * (Height/tileNumY);
					pInputShape->cropShape.x_size = (Width/tileNumX);
					pInputShape->cropShape.y_size = (Height/tileNumY);
					pInputShape->dst_stride_x = (Width/tileNumX);
					pInputShape->dst_stride_y = 0;
				}

				if (encode == 6) {
					ptr8 = (unsigned char *)am_src_buff->va;
					if (sprintf(RfileName, "/mnt/rgba_decode_%d.raw", (x+y*tileNumX)) < 0)
                        printf("sprintf error");
					printf("read %s for encode \n", RfileName);
					readFileToBuf(RfileName, ptr8, ui_src_buff_size/(tileNumX*tileNumY));
				}
				if (x==0 && y==0) {

				// initialize apusys sub-command buffer
				vec_edma_task_info.push_back((void *)&st_edma_task_info);
				/*size[0] = request cmd size, size[1] = total descriptor size*/
				edma_engine->querySubCmdInfo(vec_edma_task_info, vec_mem_size);

				printf("vec_mem_size.size() = %d\n", (int)vec_mem_size.size());

					for (i=0; i<vec_mem_size.size(); i++) {
						SubCmdInfo *p_subcmd_info = new SubCmdInfo();
						printf("cmd[%d] size = %d\n", i, vec_mem_size[i] );
						p_subcmd_info->descMem = edma_engine->memAllocCmd(vec_mem_size[i]);
						vec_subcmd.push_back(p_subcmd_info);
					}
				}
				printf("tile x = %d, y = %d\n", x, y);

				printf("fillDesc #5\r\n");

				edma_engine->fillDesc(vec_edma_task_info, vec_subcmd);

				dsc_buff = vec_subcmd[1]->descMem;
				apusys_cmd = edma_engine->initCmd();

                if (apusys_cmd == NULL)
                    goto EXIT_PROCESS;

				dumpDescriptor((unsigned int *)dsc_buff->va, vec_mem_size[1]);

				apusys_cmd->addSubCmd(vec_subcmd);

				// start execution
				edma_engine->runCmd(apusys_cmd);

				//strcpy(RfileName,"/mnt/rgba_decode.raw");
				dump_size = ui_src_buff_size;

				if (encode == 5) {
					ptr8 = (unsigned char *)am_dst2_buff->va;
					if (sprintf(RfileName, "/mnt/rgba_decode_%d.raw", (x+y*tileNumX)) < 0)
                        printf("sprintf error");
				    printf("save %s\n", RfileName);

					pFile = fopen(RfileName, "wb");
					if (pFile == NULL)
						printf("Open %s fail!!\n", RfileName);
					else {
						if (fwrite(ptr8, sizeof(char), dump_size, pFile) == 0)
                            printf("fwrite fail!!\n");
                        if (fclose(pFile) != 0)
                            printf("fclose fail!!\n");
					}
				}
				if (encode == 6) {
					ptr8 = (unsigned char *)am_dst_buff->va;
					printf("encode == 6 save big encode[%d] #5\r\n", (x+y*tileNumX));
					if (sprintf(RfileName, "/mnt/rgba_encode_big_%d.raw", (x+y*tileNumX)) < 0)
                        printf("sprintf error");                        

					pFile = fopen(RfileName, "wb");
					if (pFile == NULL)
						printf("Open %s fail!!\n", RfileName);
					else {
						if (fwrite(ptr8, sizeof(char), ui_dst_buff_size, pFile) == 0)
                            printf("fwrite fail!!\n");

                        if (fclose(pFile) != 0)
                            printf("fclose fail!!\n");
					}

				}

				edma_engine->destroyCmd(apusys_cmd);
				apusys_cmd = NULL;
	    }


		if (encode == 6) {
			ptr8 = (unsigned char *)am_dst_buff->va;
			printf("encode == 6 save big encode #6\r\n");

			pFile = fopen("/mnt/rgba_encode_big.raw", "wb");
			if (pFile == NULL)
				printf("Open %s fail!!\n", RfileName);
			else {
				if (fwrite(ptr8, sizeof(char), ui_dst_buff_size, pFile) == 0)
                    printf("fwrite fail!!\n");
                    
                if (fclose(pFile) != 0)
                    printf("fclose fail!!\n");
			}

		}
	}else {

		// initialize apusys sub-command buffer
		vec_edma_task_info.push_back((void *)&st_edma_task_info);
		/*size[0] = request cmd size, size[1] = total descriptor size*/
		edma_engine->querySubCmdInfo(vec_edma_task_info, vec_mem_size);

		printf("vec_mem_size.size() = %d #?\n", (int)vec_mem_size.size());

		for (i=0; i<vec_mem_size.size(); i++) {
			SubCmdInfo *p_subcmd_info = new SubCmdInfo();
			printf("cmd[%d] size = %d\n", i, vec_mem_size[i] );
			p_subcmd_info->descMem = edma_engine->memAllocCmd(vec_mem_size[i]);

			vec_subcmd.push_back(p_subcmd_info);
		}


		edma_engine->fillDesc(vec_edma_task_info, vec_subcmd);

		dsc_buff = vec_subcmd[1]->descMem;
		apusys_cmd = edma_engine->initCmd();
        if (apusys_cmd == NULL)
            goto EXIT_PROCESS;

		dumpDescriptor((unsigned int *)dsc_buff->va, vec_mem_size[1]);

		apusys_cmd->addSubCmd(vec_subcmd);

		ptr32 = (unsigned int *)dsc_buff->va;

		// start execution
		edma_engine->runCmd(apusys_cmd);

		// comparison
		if (encode == 2) {
			ptr8 = (unsigned char *)am_dst2_buff->va;
			ptr8_src = (unsigned char *)am_src_buff->va;

			for(i=0; i<10; i++)
				printf("decompress data ptr8[%d] = 0x%x\n", i , *(ptr8+i));

			for (i = 0; i < ui_src_buff_size; i++) {
				if (*(ptr8 + i) != *(ptr8_src + i))
					break;
			}

			if (i == ui_src_buff_size)
				printf("UFBC Pass, ptr[0] = 0x%x\n", ptr8[0]);
			else
				printf("UFBC Fail, ptr[%d] = 0x%x, src[%d] = 0x%x\n", i, ptr8[i], i, ptr8_src[i]);
		}

		if (encode == 0) {
			ptr8 = (unsigned char *)am_dst_buff->va;
			strcpy(RfileName,"/mnt/rgba_encode.raw");
			dump_size = ui_dst_buff_size;

			pFile = fopen(RfileName, "wb");
			if (pFile == NULL)
				printf("Open %s fail!!\n", RfileName);
			else {
				if (fwrite(ptr8, sizeof(char), dump_size, pFile) == 0)
                    printf("fwrite fail!!\n");
                if (fclose(pFile) != 0)
                    printf("fclose fail!!\n");
			}
		}
		if (encode == 1) {
			ptr8 = (unsigned char *)am_dst2_buff->va;
			strcpy(RfileName,"/mnt/rgba_decode.raw");
			dump_size = ui_src_buff_size;

			pFile = fopen(RfileName, "wb");
			if (pFile == NULL)
				printf("Open %s fail!!\n", RfileName);
			else {
				if (fwrite(ptr8, sizeof(char), dump_size, pFile) == 0)
                    printf("fwrite fail!!\n");
                    
                if (fclose(pFile) != 0)
                    printf("fclose fail!!\n");
            
			}
		}
	}
EXIT_PROCESS:
	// destroy apusys command


	printf("%s #6\r\n", __func__);
	if (apusys_cmd != NULL)
		edma_engine->destroyCmd(apusys_cmd);

	// free apusys memory
	if (am_src_buff != NULL)
		edma_engine->memFree(am_src_buff);
	if (am_dst_buff != NULL)
		edma_engine->memFree(am_dst_buff);
	if (am_dst2_buff != NULL)
		edma_engine->memFree(am_dst2_buff);

	printf("%s #7\r\n", __func__);

	for (i=0; i<vec_mem_size.size(); i++) {
		SubCmdInfo *p_subcmd_info = vec_subcmd[i];
		printf("%s #7.1\r\n", __func__);
		if (p_subcmd_info->descMem != NULL) {
			edma_engine->memFree(p_subcmd_info->descMem);
			printf("%s #7.2\r\n", __func__);
		}
		delete p_subcmd_info;
		p_subcmd_info = NULL;
	}
	printf("%s #7.5\r\n", __func__);

	vec_mem_size.clear();
	vec_subcmd.clear();
	vec_edma_task_info.clear();
	printf("%s #8\r\n", __func__);

	DeviceEngine::deleteInstance(edma_engine);

	// free memory
	if (st_edma_task_info.edma_info != NULL)
		free(st_edma_task_info.edma_info);
}

void compare_ufbc_result(char *File, unsigned int Width, unsigned int Height)
{
	DeviceEngine *edma_engine = DeviceEngine::createInstance("edma_engine");
	edmaMem *am_src_buff, *am_dst_buff;

	unsigned int i;

	unsigned char *ptr8;
	unsigned char *ptr8_src;

	unsigned int ui_src_buff_size;

	if (edma_engine == NULL) {
		printf("edma_engine = NULL\n");
		return;
	}

	printf("compare_ufbc_result File = %s, rgbWidth = %d, Height = %d\n", File, Width, Height);

	ui_src_buff_size = Width*Height*4;

	// memory allocation

	// allocate apusys memory
	am_src_buff = edma_engine->memAlloc(ui_src_buff_size*sizeof(unsigned char));
	am_dst_buff = edma_engine->memAlloc(ui_src_buff_size*sizeof(unsigned char));

	if (am_src_buff == NULL)
		goto EXIT_PROCESS;

	if (am_dst_buff == NULL)
		goto EXIT_PROCESS;

	ptr8_src = (unsigned char *)am_src_buff->va;
	readFileToBuf(File, ptr8_src, ui_src_buff_size);

	ptr8 = (unsigned char *)am_dst_buff->va;
	readFileToBuf("/mnt/rgba_decode.raw", ptr8, ui_src_buff_size);

	for(i=0; i<10; i++)
		printf("decompress data ptr8[%d] = 0x%x\n", i , *(ptr8+i));

	for (i = 0; i < ui_src_buff_size; i++) {
		if (*(ptr8 + i) != *(ptr8_src + i))
			break;
	}

	if (i == ui_src_buff_size)
		printf("UFBC Pass, ptr[0] = 0x%x\n", ptr8[0]);
	else
		printf("UFBC Fail, ptr[%d] = 0x%x, src[%d] = 0x%x\n",
				i, ptr8[i], i, ptr8_src[i]);

EXIT_PROCESS:

	// free apusys memory
	if (am_src_buff != NULL)
		edma_engine->memFree(am_src_buff);
	if (am_dst_buff != NULL)
		edma_engine->memFree(am_dst_buff);

	DeviceEngine::deleteInstance(edma_engine);
}

void test_edma_engineSlice(void)
{
    DeviceEngine* edmaEngine = DeviceEngine::createInstance("edma_engine");
    if (edmaEngine == nullptr) {
        printf("test_edma_engineSlice: edma_engine = nullptr\n");
        return;
    }

    printf("Start test_edma_engineSlice\n");
    bool success = false;
    const uint32_t inputH = 30;
    const uint32_t inputW = 40;
    const uint32_t inputC = 1;  // support one channel only
    const uint32_t outputH = 20;
    const uint32_t outputW = 30;
    const uint32_t outputC = 1;  // support one channel only
    const uint32_t offsetH = 5;  // slice offset of input height
    const uint32_t offsetW = 10;  // slice offset of input width
    const uint32_t offsetC = 0;  // slice offset of input channel
    const uint32_t elemSize = sizeof(uint8_t);  // support data type U8 only
    const uint32_t inputSize = inputH * inputW * inputC * elemSize;
    const uint32_t outputSize = outputH * outputW * outputC * elemSize;
    const uint32_t inputOffset = (offsetH * inputW * inputC + offsetW * inputC + offsetC) * elemSize;
    std::vector<void *> taskInfos;
    std::vector<SubCmdInfo *> subCommands;
    std::vector<int32_t> descSizes;
    st_edmaDescInfoType0* sliceInfo = nullptr;
    edmaMem* srcBuffer = nullptr;
    edmaMem* dstBuffer = nullptr;
    edmaCmd* cmd = nullptr;
    uint8_t* data = nullptr;

    st_edmaTaskInfo sliceTask;
    sliceTask.info_num = 1;
    sliceTask.edma_info = static_cast<st_edmaUnifyInfo *>(calloc(sliceTask.info_num, sizeof(st_edmaUnifyInfo)));

    if (sliceTask.edma_info == nullptr) {
        printf("test_edma_engineSlice: edma_info = nullptr\n");
        goto EXIT_PROCESS;
    }

    sliceTask.edma_info[0].info_type = EDMA_INFO_SLICE;
    sliceInfo = static_cast<st_edmaDescInfoType0 *>(&sliceTask.edma_info[0].st_info.info0);

    if (inputC != outputC) {
        printf("test_edma_engineSlice: Do not support slicing over C %u != %u\n", inputC, outputC);
        goto EXIT_PROCESS;
    }

    sliceInfo->uc_desc_id = 0;
    sliceInfo->ui_src_pitch_x_0 = inputW;
    sliceInfo->ui_dst_pitch_x_0 = outputW;
    sliceInfo->ui_src_pitch_y_0 = inputH;
    sliceInfo->ui_dst_pitch_y_0 = outputH;
    sliceInfo->ui_src_pitch_z_0 = inputC;
    sliceInfo->ui_dst_pitch_z_0 = outputC;
    sliceInfo->ui_src_size_x = outputW;
    sliceInfo->ui_dst_size_x = outputW;
    sliceInfo->ui_src_size_y = outputH;
    sliceInfo->ui_dst_size_y = outputH;
    sliceInfo->ui_src_size_z = outputC;
    sliceInfo->ui_dst_size_z = outputC;

    // Allocate apusys memory (assume src/dst data types are both U8)
    srcBuffer = edmaEngine->memAlloc(inputSize);
    dstBuffer = edmaEngine->memAlloc(outputSize);

    if (srcBuffer == nullptr || dstBuffer == nullptr) {
        printf("test_edma_engineSlice: failed to allocate apusys memory for input/output\n");
        goto EXIT_PROCESS;
    }

    // Assign input values
    memset(reinterpret_cast<uint8_t*>(srcBuffer->va), 0x0, inputSize);
    memset(reinterpret_cast<uint8_t*>(dstBuffer->va), 0x0, outputSize);
    data = reinterpret_cast<uint8_t*>(srcBuffer->va);
    for (uint32_t pos = 0; pos < inputSize; pos++) {
        *(data + pos) = static_cast<uint8_t>(pos / inputW + pos % inputW);
    }

    // Shift input offset to the slice begin
    sliceInfo->ui_src_addr_0 = srcBuffer->iova + inputOffset;
    sliceInfo->ui_dst_addr_0 = dstBuffer->iova;
    printf("src addr %u, dst addr %u\n", sliceInfo->ui_src_addr_0, sliceInfo->ui_dst_addr_0);

    // Prepare apusys command
    cmd = edmaEngine->initCmd();
    if (cmd == nullptr) {
        printf("test_edma_engineSlice: failed to create apusys command\n");
        goto EXIT_PROCESS;
    }

    // Initialize apusys sub-command buffer
    taskInfos.push_back(static_cast<void*>(&sliceTask));
    success = edmaEngine->querySubCmdInfo(taskInfos, descSizes);
    if (!success) {
        printf("test_edma_engineSlice: failed to query sub command info\n");
        goto EXIT_PROCESS;
    }
    printf("test_edma_engineSlice: need %zu descriptors\n", descSizes.size());

    for (const auto dSize : descSizes) {
        SubCmdInfo* subCmd = static_cast<SubCmdInfo *>(calloc(1, sizeof(SubCmdInfo)));
        if (subCmd == nullptr) {
            goto EXIT_PROCESS;
        }
        subCmd->descMem = edmaEngine->memAllocCmd(dSize);
        subCommands.push_back(subCmd);
    }
    success = edmaEngine->fillDesc(taskInfos, subCommands);
    if (!success) {
        printf("test_edma_engineSlice: failed to fill sub command descriptors\n");
        goto EXIT_PROCESS;
    }

    cmd->addSubCmd(subCommands);

    printf("--- src data %u x %u ---\n", inputW, inputH);
    data = reinterpret_cast<uint8_t*>(srcBuffer->va);
    for (uint32_t h = 0; h < inputH; h++) {
        for (uint32_t w = 0; w < inputW; w++) {
            printf("%u ", static_cast<uint32_t>(*(data + w + h * inputW)));
        }
        printf("\n");
    }
    printf("----------------\n");

    success = edmaEngine->runCmd(cmd);

    printf("\nCrop from offset %u x %u\n\n", offsetW, offsetH);

    printf("--- dst data %u x %u ---\n", outputW, outputH);
    data = reinterpret_cast<uint8_t*>(dstBuffer->va);
    for (uint32_t h = 0; h < outputH; h++) {
        for (uint32_t w = 0; w < outputW; w++) {
            printf("%u ", static_cast<uint32_t>(*(data + w + h * outputW)));
        }
        printf("\n");
    }
    printf("----------------\n");

EXIT_PROCESS:
    for (size_t i = 0; i < subCommands.size(); i++) {
        SubCmdInfo* subCmd = subCommands[i];
        edmaEngine->memFree(subCmd->descMem);
        free(subCmd);
        subCommands[i] = nullptr;
    }

    if (cmd != nullptr) {
        edmaEngine->destroyCmd(cmd);
    }

    if (srcBuffer != nullptr) {
        edmaEngine->memFree(srcBuffer);
    }

    if (dstBuffer != nullptr) {
        edmaEngine->memFree(dstBuffer);
    }

    if (sliceTask.edma_info != nullptr) {
        free(sliceTask.edma_info);
    }

    DeviceEngine::deleteInstance(edmaEngine);
    printf("Done test_edma_engineSlice: %s\n", (success ? "success" : "fail"));
}


void test_edma_engineCopyHSE(void)
{
	DeviceEngine *edma_engine = DeviceEngine::createInstance("edma_engine");
	std::vector<int> vec_mem_size;
	std::vector<SubCmdInfo *> vec_subcmd;
	std::vector<void *> vec_edma_task_info;

	std::vector<int> vec2_mem_size;
	std::vector<SubCmdInfo *> vec2_subcmd;
	std::vector<void *> vec2_edma_task_info;

	st_edmaTaskInfo st_edma_task_info;
	st_edmaTaskInfo st_edma_task_info2;

	void *pst_curr_desc_info = NULL;

	st_edma_InfoData *edma_info_copy = NULL;

	edmaMem *am_src_buff = NULL, *am_dst_buff = NULL, *sub_cmd_buff = NULL, *dsc_buff = NULL;
	edmaMem *am_dst2_buff = NULL, *sub_cmd2_buff = NULL;

	edmaCmd *apusys_cmd = NULL;


	unsigned int i;
	unsigned char *ptr8 = NULL;

	unsigned char uc_mode;
	unsigned int ui_src_data_size, ui_src_buff_size, ui_dst_data_size, ui_dst_buff_size;


	unsigned int ui_desc_num, ui_desc_idx;

	if (edma_engine == NULL) {
		printf("edma_engine = NULL\n");
		return;
	}

	printf("test_edma_engineCopyHSE\n");

	ui_desc_num = 2;
	ui_desc_idx = 0;
	st_edma_task_info.edma_info = NULL;
	st_edma_task_info2.edma_info = NULL;


	uc_mode = MODE_NORMAL;
	ui_src_data_size = TEST_COPY_SIZE;
	ui_dst_data_size = TEST_COPY_SIZE;
	ui_src_buff_size = ui_src_data_size;
	ui_dst_buff_size = ui_dst_data_size;

	// memory allocation

	printf("test_edma_engineCopyHSE malloc\n");

	// allocate apusys memory
	am_src_buff = edma_engine->memAlloc(ui_src_buff_size*sizeof(unsigned char));
	am_dst_buff = edma_engine->memAlloc(ui_dst_buff_size*sizeof(unsigned char));
	am_dst2_buff = edma_engine->memAlloc(ui_dst_buff_size*sizeof(unsigned char));

	if (am_src_buff == NULL || am_dst_buff == NULL || am_dst2_buff == NULL )
		goto EXIT_PROCESS;

	st_edma_task_info.info_num = ui_desc_num;
	st_edma_task_info2.info_num = ui_desc_num;


	memset((unsigned char *)am_src_buff->va, 0x69, ui_src_buff_size);
	memset((unsigned char *)am_dst_buff->va, 0x3, ui_dst_buff_size);
	memset((unsigned char *)am_dst2_buff->va, 0x3, ui_dst_buff_size);

	/*type array & info array for rquest total size*/
	//st_edma_task_info.puc_desc_type = (unsigned char *)calloc(st_edma_task_info.ui_desc_num, sizeof(unsigned char));
	st_edma_task_info.edma_info = (st_edmaUnifyInfo *)calloc(st_edma_task_info.info_num, sizeof(st_edmaUnifyInfo));

	if (st_edma_task_info.edma_info == NULL)
		goto EXIT_PROCESS;

	ui_desc_idx = 0;

	pst_curr_desc_info = &st_edma_task_info.edma_info[ui_desc_idx].st_info.infoData;
	st_edma_task_info.edma_info[ui_desc_idx].info_type = EDMA_INFO_DATA;
	edma_info_copy = (st_edma_InfoData *)pst_curr_desc_info;
	edma_info_copy->uc_desc_type = EDMA_INFO_DATA;

	edma_info_copy->inFormat = EDMA_FMT_CONSUMER;
	edma_info_copy->outFormat = EDMA_FMT_NONE;
	edma_info_copy->cVID = 7;
	edma_info_copy->cWait = 1;

	ui_desc_idx++;

	pst_curr_desc_info = &st_edma_task_info.edma_info[ui_desc_idx].st_info.infoData;
	st_edma_task_info.edma_info[ui_desc_idx].info_type = EDMA_INFO_DATA;
	edma_info_copy = (st_edma_InfoData *)pst_curr_desc_info;
	edma_info_copy->uc_desc_type = EDMA_INFO_DATA;
	edma_info_copy->inBuf_addr = (unsigned int)(am_dst_buff->iova); //2^7 = 128 bytes
	edma_info_copy->outBuf_addr = (unsigned int)am_dst2_buff->iova;
	edma_info_copy->inFormat = EDMA_FMT_NORMAL;
	edma_info_copy->outFormat = EDMA_FMT_NORMAL;
	edma_info_copy->copy_size = TEST_COPY_SIZE;


    // HSE Producer
	st_edma_task_info2.edma_info = (st_edmaUnifyInfo *)calloc(st_edma_task_info2.info_num, sizeof(st_edmaUnifyInfo));

	if (st_edma_task_info2.edma_info == NULL)
		goto EXIT_PROCESS;

	ui_desc_idx = 0;

	pst_curr_desc_info = &st_edma_task_info2.edma_info[ui_desc_idx].st_info.infoData;
	st_edma_task_info2.edma_info[ui_desc_idx].info_type = EDMA_INFO_DATA;
	edma_info_copy = (st_edma_InfoData *)pst_curr_desc_info;
	edma_info_copy->uc_desc_type = EDMA_INFO_DATA;
	edma_info_copy->inBuf_addr = (unsigned int)(am_src_buff->iova);
	edma_info_copy->outBuf_addr = (unsigned int)am_dst_buff->iova;
	edma_info_copy->inFormat = EDMA_FMT_NORMAL;
	edma_info_copy->outFormat = EDMA_FMT_NORMAL;
	edma_info_copy->copy_size = TEST_COPY_SIZE;

	ui_desc_idx++;

	pst_curr_desc_info = &st_edma_task_info2.edma_info[ui_desc_idx].st_info.infoData;
	st_edma_task_info2.edma_info[ui_desc_idx].info_type = EDMA_INFO_DATA;
	edma_info_copy = (st_edma_InfoData *)pst_curr_desc_info;
	edma_info_copy->uc_desc_type = EDMA_INFO_DATA;

	edma_info_copy->inFormat = EDMA_FMT_PRODUCER;
	edma_info_copy->outFormat = EDMA_FMT_NONE;
	edma_info_copy->pVID = 7;
	//edma_info_copy->cWait = 1;


	//pst_curr_desc_info = (void *)((uintptr_t)pst_curr_desc_info + (uintptr_t)sizeof(_edma20_InputShape));
	apusys_cmd = edma_engine->initCmd();
	if (apusys_cmd == NULL)
		goto EXIT_PROCESS;

	// initialize apusys sub-command buffer
	vec_edma_task_info.push_back((void *)&st_edma_task_info);
	/*size[0] = request cmd size, size[1] = total descriptor size*/
	edma_engine->querySubCmdInfo(vec_edma_task_info, vec_mem_size);

	printf("vec_mem_size.size() = %d\n", (int)vec_mem_size.size());

    for (const auto dSize : vec_mem_size) {
        SubCmdInfo* subCmd = static_cast<SubCmdInfo *>(calloc(1, sizeof(SubCmdInfo)));
        if (subCmd == nullptr) {
            goto EXIT_PROCESS;
        }
        subCmd->descMem = edma_engine->memAllocCmd(dSize);
        vec_subcmd.push_back(subCmd);
    }

	edma_engine->fillDesc(vec_edma_task_info, vec_subcmd);

	sub_cmd_buff = vec_subcmd[0]->descMem;
	apusys_cmd->addSubCmd(vec_subcmd, SC_PARAM_VLM_CTX, 5);


	dsc_buff = vec_subcmd[1]->descMem;

	dumpDescriptor((unsigned int *)dsc_buff->va, vec_mem_size[1]);


	vec2_edma_task_info.push_back((void *)&st_edma_task_info2);
	/*size[0] = request cmd size, size[1] = total descriptor size*/
	edma_engine->querySubCmdInfo(vec2_edma_task_info, vec2_mem_size);

	printf("vec_mem_size.size() = %d\n", (int)vec2_mem_size.size());

    for (const auto dSize : vec2_mem_size) {
        SubCmdInfo* subCmd = static_cast<SubCmdInfo *>(calloc(1, sizeof(SubCmdInfo)));
        if (subCmd == nullptr) {
            goto EXIT_PROCESS;
        }
        subCmd->descMem = edma_engine->memAllocCmd(dSize);
        vec2_subcmd.push_back(subCmd);
    }

	edma_engine->fillDesc(vec2_edma_task_info, vec2_subcmd);

	sub_cmd2_buff = vec2_subcmd[0]->descMem;

	apusys_cmd->addSubCmd(vec2_subcmd, SC_PARAM_VLM_CTX, 5);

	dsc_buff = vec2_subcmd[1]->descMem;

	dumpDescriptor((unsigned int *)dsc_buff->va, vec2_mem_size[1]);


	// start execution
	edma_engine->runCmdAsync(apusys_cmd);
	printf("edma_engine->runCmdAsync #1\n");

	usleep(30*1000); //sleep 30ms

	edma_engine->waitCmd(apusys_cmd);
	printf("edma_engine->runCmdAsync done\n");

	// comparison

	printf("comparing dst data ...\n");

	ptr8 = (unsigned char *)am_dst_buff->va;
	if (gTestCmd->getConfig() & EDMA_TEST_SHOW_DESC)
		for(i=0; i<30; i++)
			printf("Golden data ptr8[%d] = 0x%x\n", i , *(ptr8+i));

	for (i=0; i<TEST_COPY_SIZE; i++) {
		if (ptr8[i] != 0x69) {
			printf("Golden data ptr8[%d] = 0x%x, error !!!!!\n", i , *(ptr8+i));
			break;
		}
	}

	printf("comparing dst2 data ...\n");
	// comparison
	ptr8 = (unsigned char *)am_dst2_buff->va;

	if (gTestCmd->getConfig() & EDMA_TEST_SHOW_DESC)
		for(i=0; i<30; i++)
			printf("Golden2 data ptr8[%d] = 0x%x\n", i , *(ptr8+i));

	for (i=0; i<TEST_COPY_SIZE; i++) {
		if (ptr8[i] != 0x69) {
			printf("Golden2 data ptr8[%d] = 0x%x, error !!!!!\n", i , *(ptr8+i));
			break;
		}
	}

	if (i == TEST_COPY_SIZE)
		printf("test HSE total size [%d] copy verify Pass!!\n", i);

EXIT_PROCESS:
	// destroy apusys command
	if (apusys_cmd != NULL)
		edma_engine->destroyCmd(apusys_cmd);

	// free apusys memory
	if (am_src_buff != NULL)
		edma_engine->memFree(am_src_buff);
	if (am_dst_buff != NULL)
		edma_engine->memFree(am_dst_buff);

	if (am_dst2_buff != NULL)
		edma_engine->memFree(am_dst2_buff);

	for (i=0; i<vec_mem_size.size(); i++) {
		SubCmdInfo *p_subcmd_info = vec_subcmd[i];
		edma_engine->memFree(p_subcmd_info->descMem);
		delete p_subcmd_info;
		p_subcmd_info = NULL;
	}
	for (i=0; i<vec2_mem_size.size(); i++) {
		SubCmdInfo *p_subcmd_info = vec2_subcmd[i];
		edma_engine->memFree(p_subcmd_info->descMem);
		delete p_subcmd_info;
		p_subcmd_info = NULL;
	}
	vec_mem_size.clear();
	vec_subcmd.clear();
	vec2_mem_size.clear();
	vec2_subcmd.clear();


	DeviceEngine::deleteInstance(edma_engine);
	// free memory
	if (st_edma_task_info.edma_info != NULL)
		free(st_edma_task_info.edma_info);

	if (st_edma_task_info2.edma_info != NULL)
		free(st_edma_task_info2.edma_info);

	printf("test_edma_engineCopy free done\n");
}


void test_edma_enginePad(int32_t mode)
{
    DeviceEngine* edmaEngine = DeviceEngine::createInstance("edma_engine");
    if (edmaEngine == nullptr) {
        printf("test_edma_enginePad: edma_engine = nullptr\n");
        return;
    }

    printf("Start test_edma_enginePad\n");
    bool success = false;
    const uint32_t inputH = 5;
    const uint32_t inputW = 10;
    const uint32_t padBeforeH = 1;
    const uint32_t padAfterH = 2;
    const uint32_t padBeforeW = 3;
    const uint32_t padAfterW = 4;
    const uint32_t elemSize = sizeof(uint8_t);
    const uint32_t padConstant = 99;
    const uint32_t outputH = padBeforeH + inputH + padAfterH;
    const uint32_t outputW = padBeforeW + inputW + padAfterW;
    const uint32_t inStrideH = inputH;
    const uint32_t inStrideW = inputW;
    const uint32_t outStrideH = outputH;
    const uint32_t outStrideW = outputW;
    const uint32_t inputSize = inStrideH * inStrideW * elemSize;
    const uint32_t outputSize = outStrideH * outStrideW * elemSize;

    std::vector<void *> taskInfos;
    std::vector<SubCmdInfo *> subCommands;
    std::vector<int32_t> descSizes;
    st_edmaDescInfoPad* padInfo = nullptr;
    edmaMem* srcBuffer = nullptr;
    edmaMem* dstBuffer = nullptr;
    edmaCmd* cmd = nullptr;
    uint8_t* data = nullptr;

    st_edmaTaskInfo padTask;
    padTask.info_num = 1;
    padTask.edma_info = static_cast<st_edmaUnifyInfo *>(calloc(padTask.info_num, sizeof(st_edmaUnifyInfo)));

    if (padTask.edma_info == nullptr) {
        printf("test_edma_enginePad: edma_info = nullptr\n");
        goto EXIT_PROCESS;
    }

    padTask.edma_info[0].info_type = EDMA_INFO_PAD;
    padInfo = static_cast<st_edmaDescInfoPad *>(&padTask.edma_info[0].st_info.infoPad);

    padInfo->uc_mode = static_cast<uint8_t>(mode);
    padInfo->uc_desc_id = 0;
    padInfo->uc_elem_size = static_cast<uint8_t>(elemSize);
    padInfo->ui_src_size_x = inputW;
    padInfo->ui_src_size_y = inputH;
    padInfo->ui_paddings_before_x = padBeforeW;
    padInfo->ui_paddings_after_x = padAfterW;
    padInfo->ui_paddings_before_y = padBeforeH;
    padInfo->ui_paddings_after_y = padAfterH;
    padInfo->ui_src_stride_x = inStrideW;
    padInfo->ui_src_stride_y = inStrideH;
    padInfo->ui_dst_stride_x = outStrideW;
    padInfo->ui_dst_stride_y = outStrideH;
    padInfo->ui_pad_constant = padConstant;

    // Allocate apusys memory (assume src/dst data types are both U8)
    srcBuffer = edmaEngine->memAlloc(inputSize);
    dstBuffer = edmaEngine->memAlloc(outputSize);

    if (srcBuffer == nullptr || dstBuffer == nullptr) {
        printf("test_edma_enginePad: failed to allocate apusys memory for input/output\n");
        goto EXIT_PROCESS;
    }

    // Assign input values
    memset(reinterpret_cast<uint8_t*>(srcBuffer->va), 0x0, inputSize);
    memset(reinterpret_cast<uint8_t*>(dstBuffer->va), 0x0, outputSize);
    data = reinterpret_cast<uint8_t*>(srcBuffer->va);
    for (uint32_t pos = 0; pos < inputSize; pos++) {
        *(data + pos) = static_cast<uint8_t>(10 + pos / inputW + pos % inputW);
    }

    padInfo->ui_src_addr = srcBuffer->iova;
    padInfo->ui_dst_addr = dstBuffer->iova;
    printf("src addr %u, dst addr %u\n", padInfo->ui_src_addr, padInfo->ui_dst_addr);

    // Prepare apusys command
    cmd = edmaEngine->initCmd();
    if (cmd == nullptr) {
        printf("test_edma_enginePad: failed to create apusys command\n");
        goto EXIT_PROCESS;
    }

    // Initialize apusys sub-command buffer
    taskInfos.push_back(static_cast<void*>(&padTask));
    success = edmaEngine->querySubCmdInfo(taskInfos, descSizes);
    if (!success) {
        printf("test_edma_enginePad: failed to query sub command info\n");
        goto EXIT_PROCESS;
    }

    for (const auto dSize : descSizes) {
        SubCmdInfo* subCmd = static_cast<SubCmdInfo *>(calloc(1, sizeof(SubCmdInfo)));
        if (subCmd == nullptr) {
            goto EXIT_PROCESS;
        }
        subCmd->descMem = edmaEngine->memAllocCmd(dSize);
        subCommands.push_back(subCmd);
    }

    success = edmaEngine->fillDesc(taskInfos, subCommands);
    if (!success) {
        printf("test_edma_enginePad: failed to fill sub command descriptors\n");
        goto EXIT_PROCESS;
    }

    cmd->addSubCmd(subCommands);

    printf("--- src data %u x %u (stride %u x %u) ---\n", inputW, inputH, inStrideW, inStrideH);
    data = reinterpret_cast<uint8_t*>(srcBuffer->va);
    for (uint32_t h = 0; h < inStrideH; h++) {
        for (uint32_t w = 0; w < inStrideW; w++) {
            printf("%u ", static_cast<uint32_t>(*(data + w + h * inStrideW)));
        }
        printf("\n");
    }
    printf("----------------\n");

    success = edmaEngine->runCmd(cmd);

    printf("--- dst data %u x %u (stride %u x %u) ---\n", outputW, outputH, outStrideW, outStrideH);
    data = reinterpret_cast<uint8_t*>(dstBuffer->va);
    for (uint32_t h = 0; h < outStrideH; h++) {
        for (uint32_t w = 0; w < outStrideW; w++) {
            printf("%u ", static_cast<uint32_t>(*(data + w + h * outStrideW)));
        }
        printf("\n");
    }
    printf("----------------\n");

EXIT_PROCESS:
    for (size_t i = 0; i < subCommands.size(); i++) {
        SubCmdInfo* subCmd = subCommands[i];
        edmaEngine->memFree(subCmd->descMem);
        free(subCmd);
        subCommands[i] = nullptr;
    }

    if (cmd != nullptr) {
        edmaEngine->destroyCmd(cmd);
    }

    if (srcBuffer != nullptr) {
        edmaEngine->memFree(srcBuffer);
    }

    if (dstBuffer != nullptr) {
        edmaEngine->memFree(dstBuffer);
    }

    if (padTask.edma_info != nullptr) {
        free(padTask.edma_info);
    }

    DeviceEngine::deleteInstance(edmaEngine);
    printf("Done test_edma_enginePad: %s\n", (success ? "success" : "fail"));
}

int main(int argc, char * argv[])
{
	unsigned int loopNum = 0, test_count = 0;
	unsigned int sleep_time = 0;
	unsigned int test_width = 0;
	unsigned int test_height = 0;
	unsigned int test_id = 0, factor = 0;
	char filename[256] = { 0 };
	int ret = 0;
	EDMA_FMT_E outFormat = EDMA_FMT_NORMAL;
	edmaTestCmd * testInst = nullptr;

	printf("edma testing ...\n");
	testInst = new edmaTestCmd;

	gTestCmd = testInst;

	while (1) {
		int c;
		int optionIdx;
		c = getopt_long(argc, argv, "vedl:s:tc:f:i:w:h:",
						longOptions, &optionIdx);

		if (c == -1)
			break;

		switch (c) {
		case 'l':
			loopNum = (unsigned int)atoi(optarg);
			break;

		case 's':
			test_id = 1;
			sleep_time = (unsigned int)atoi(optarg);
			printf("sleep_time = %d us\n",sleep_time);
			break;

		case 't':
			test_id = 1;
			sleep_time = 0;
			printf("sleep_time = %d us\n",sleep_time);
			break;

		case 'v':
			test_id = 3;
			break;

		case 'c':
			test_id = (unsigned int)atoi(optarg);
			if (test_id < 2)
				test_id = 2;

			printf("test_id = %d \n",test_id);
			break;

		case 'f':
			factor = (unsigned int)atoi(optarg);

		if (factor == 0)
			outFormat = EDMA_FMT_ROTATE90;
		else if (factor == 1)
			outFormat = EDMA_FMT_ROTATE180;
		else
			outFormat = EDMA_FMT_ROTATE270;

			printf("factor = %d \n",factor);
			break;

		case 'd':
			testInst->setConfig(EDMA_TEST_SHOW_DESC);
			break;

        case 'e':
            testInst->setConfig(EDMA_TEST_FORCE_ERROR);
            break;

		case 'i':
			printf ("file name: '%s'\n", optarg);
			strncpy(filename, optarg, sizeof(filename) - 1);
			filename[sizeof(filename) - 1] = '\0';
			break;

		case 'w':
			test_width = (unsigned int)atoi(optarg);
			printf("test_width = %d\n",test_width);
			break;

		case 'h':
			test_height = (unsigned int)atoi(optarg);
			printf("test_height = %d\n",test_height);
			break;

		case '?':
			showHelp();
			return 0;
		default:
			break;
		}
	}

	/* check argument */
	if (loopNum > APUSYS_UT_LOOP_MAX)
		loopNum = APUSYS_UT_LOOP_MAX;

	if (testInst == nullptr) {
		printf("get ut inst fail\n");
		return -1;
	}

	if (test_id == 1) {
		while (1) {
			printf("run testInst->runCase(10,1)\n");

			ret = testInst->runCase(10,1);
			if (ret < 0) {
				printf("stress test(%d) fail, error(%d)\n",test_count,ret);
				break;
			} else {
				printf("stress test(%d) all pass\n", test_count);
			}
			usleep(sleep_time);
			test_count++;
			if (test_count > APUSYS_UT_LOOP_MAX)
				test_count = 0;
		}
	} else if (test_id == 2) {
		printf("compress test loop(%d)... \n",loopNum);
		if (loopNum > 0) {
			ret = testInst->runCase(loopNum,2);
			if (ret < 0)
				printf("loopNum(%d) fail, error(%d)\n",loopNum, ret);
			else
				printf("loopNum(%d) all pass\n", loopNum);
		}
		else
			ret = testInst->runCase(1,2);
	} else if (test_id == 3) {
		printf("vlm test 2 loop(%d)... \n",loopNum);

		if (loopNum > 0) {
			ret = testInst->runCase(loopNum,3);
			if (ret < 0)
				printf("loopNum(%d) fail, error(%d)\n",loopNum, ret);
			else
				printf("loopNum(%d) all pass\n", loopNum);
		}
		else
			ret = testInst->runCase(1,3);
	} else if (test_id == 5 ) {
		ret = testInst->runCase(1,test_id);
	} else if (test_id == 6) {
		//test_edma_engine_nn_FP();
		//test_edma_engineMVPU();
		test_edma_engineRotate(outFormat);
	} else if (test_id == 7) {
		//test_edma_engine_nn_FP();
		//test_edma_engine_nn();
		test_edma_engineMM_YUV(test_width, test_height, factor);
		//test_edma_engineCopyHSE();
		//test_edma_engineMM_DYN2(filename, test_width, outFormat);
		//test_edma_engineMM_SDK();
	} else if (test_id == 8) {
		//test_edma_engine();
		//test_edma_engine_nn_new();
		test_edma_engineCopy();
		//test_edma_engineMM_DYN(filename, test_width, outFormat);
	} else if (test_id == 9) {
		//test_edma_engine2();
		//test_edma_engine_nn_3x4();
		//test_edma_engine_nn_4x3();
		if (loopNum != 0)
			TEST_S2D = 1;
		test_edma_engine_nn_DepToSpace(test_width, test_height, factor);
	} else if (test_id == 10) {
		//test_edma_engine_nn_FP();
		test_edma_engine_nn_gxp(1,4);
	} else if (test_id == 11) {
		if (factor == 2) {
			test_edma_engine_UFBC(filename,test_width, test_height, 0, 1, 1);
			test_edma_engine_UFBC("/mnt/rgba_encode.raw",test_width, test_height, 1, 1, 1);
			compare_ufbc_result(filename,test_width, test_height);
		} else {
			test_edma_engine_UFBC(filename,test_width, test_height, factor, 2, 2);
		}
	} else if (test_id == 12) {
			test_edma_engine_UFBC(filename,test_width, test_height, factor, 1, 1);

		//test_edma_engineCopyHSE();
    } else if (test_id == 13) {
        test_edma_engineSlice();
    } else if (test_id == 14) {
        test_edma_enginePad(EDMA_CONSTANT_PADDING);
    } else if (test_id == 15) {
        test_edma_enginePad(EDMA_EDGE_PADDING);
	} else if(loopNum != 0) {
		printf("run edma test loop %d times\n", loopNum);
		ret = testInst->runCase(loopNum,1);
		if (ret < 0)
			printf("loopNum(%d) fail, error(%d)\n",loopNum, ret);
		else
			printf("loopNum(%d) all pass, ret = %d\n", loopNum, ret);
	}
	delete testInst;

	return ret;
}

