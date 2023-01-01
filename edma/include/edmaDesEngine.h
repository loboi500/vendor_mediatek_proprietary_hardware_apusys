#pragma once

#include <string>
#include <vector>
#include "edma_info.h"
#include "edma_cmn.h"
#include <map>

using namespace std;

class EdmaDescGentor
{
	private:
		void *pHeadDesc = NULL;

	protected:
		void *getHeadDesc(void) {return pHeadDesc;};
	public:
		void setHeadDesc(void *pHead) { pHeadDesc = pHead; };
		virtual ~EdmaDescGentor(void) { EDMA_LOG_INFO("EdmaDescGentor be deleted!!!\n");};
	/*total descriptor size of single info*/
		virtual unsigned int queryDSize(st_edmaUnifyInfo *in_info) = 0;
	/*total descriptor number of single info, default = 1*/
		virtual unsigned int queryDNum(st_edmaUnifyInfo *in_info) { if(in_info){return 1;} return -1;};
#ifdef MVPU_EN
	/*total num number of values which need to be replaced */
		virtual unsigned int queryRPNum(st_edmaUnifyInfo *in_info) { if(in_info){return 1;} return -1;};
		virtual void fillRPOffset(st_edmaTaskInfo *task_info, unsigned int *id, EDMA_INFO_TYPE type, unsigned int * rp_offset, unsigned int desc_offset) { if(task_info){return;} return;};
#endif
		virtual int fillDesc(st_edmaUnifyInfo *shape, void *edma_desc) = 0;
	/*DescGentor designer needs to convert user info to new multi-info, default user info not change */
		virtual void transTaskInfo(st_edmaTaskInfo *oldInfo, st_edmaTaskInfo *newInfo) {
			memcpy(newInfo, oldInfo, sizeof(st_edmaTaskInfo));
			EDMA_LOG_INFO("EdmaDescGentor no need allocate new info for this gentor\n");
		};
};

class EdmaDesGGeneralV20 : public EdmaDescGentor {
	unsigned int queryDSize(st_edmaUnifyInfo *in_info) override;
	unsigned int queryDNum(st_edmaUnifyInfo *in_info)  override;
	int fillDesc(st_edmaUnifyInfo *shape, void *edma_desc) override;
};

class EdmaDesGDataV20 : public EdmaDescGentor {
	unsigned int queryDSize(st_edmaUnifyInfo *in_info) override;
	int fillDesc(st_edmaUnifyInfo *shape, void *edma_desc) override;
};

class EdmaDesGUFBCV20 : public EdmaDescGentor {
	unsigned int queryDSize(st_edmaUnifyInfo *in_info) override;
	int fillDesc(st_edmaUnifyInfo *shape, void *edma_desc) override;
};

class EdmaDesGGeneral : public EdmaDescGentor {
	unsigned int queryDSize(st_edmaUnifyInfo *in_info) override;
	unsigned int queryDNum(st_edmaUnifyInfo *in_info)  override;
	int fillDesc(st_edmaUnifyInfo *shape, void *edma_desc) override;
};

class EdmaDesGNN : public EdmaDescGentor {
	unsigned int queryDSize(st_edmaUnifyInfo *in_info) override;
	unsigned int queryDNum(st_edmaUnifyInfo *in_info)  override;
	int fillDesc(st_edmaUnifyInfo *shape, void *edma_desc) override;
};

class EdmaDesGData : public EdmaDescGentor {

	unsigned int queryDSize(st_edmaUnifyInfo *in_info) override;
	int fillDesc(st_edmaUnifyInfo *shape, void *edma_desc) override;
};



#ifdef MVPU_EN
class EdmaDesGMVPU : public EdmaDescGentor {
	unsigned int queryDSize(st_edmaUnifyInfo *in_info) override;
	unsigned int queryDNum(st_edmaUnifyInfo *in_info)  override;
	unsigned int queryRPNum(st_edmaUnifyInfo *in_info) override;
	void fillRPOffset(st_edmaTaskInfo *task_info, unsigned int *id, EDMA_INFO_TYPE type, unsigned int * rp_offset, unsigned int desc_offset) override;
	int fillDesc(st_edmaUnifyInfo *shape, void *edma_desc) override;
	void transTaskInfo(st_edmaTaskInfo *oldInfo, st_edmaTaskInfo *newInfo) override;
};
#endif

class EdmaDesGUFBC : public EdmaDescGentor {
	unsigned int queryDSize(st_edmaUnifyInfo *in_info) override;
	int fillDesc(st_edmaUnifyInfo *shape, void *edma_desc) override;
};

class EdmaDesGSDK : public EdmaDescGentor {
	unsigned int queryDSize(st_edmaUnifyInfo *in_info) override;
	int fillDesc(st_edmaUnifyInfo *shape, void *edma_desc) override;
};

class EdmaDesGSlice : public EdmaDescGentor {
    unsigned int queryDSize(st_edmaUnifyInfo *in_info) override;
    int fillDesc(st_edmaUnifyInfo *shape, void *edma_desc) override;
};

class EdmaDesGPad : public EdmaDescGentor {
    unsigned int queryDSize(st_edmaUnifyInfo *in_info) override;
    unsigned int queryDNum(st_edmaUnifyInfo *in_info) override;
    int fillDesc(st_edmaUnifyInfo *shape, void *edma_desc) override;
};

class EdmaDesGBayerToRGGB : public EdmaDescGentor {
    unsigned int queryDSize(st_edmaUnifyInfo *in_info) override;
    int fillDesc(st_edmaUnifyInfo *shape, void *edma_desc) override;
};

class EdmaDesGRGGBToBayer : public EdmaDescGentor {
    unsigned int queryDSize(st_edmaUnifyInfo *in_info) override;
    int fillDesc(st_edmaUnifyInfo *shape, void *edma_desc) override;
};

class EdmaDescEngine
{
	private:
		//Descriptor generators map
		map<EDMA_INFO_TYPE, EdmaDescGentor*> mDesGentor;

		//query descriptor size (internal use)
		unsigned int queryDescSize(void *pst_in_edma_desc_info, unsigned int *pui_new_desc_num=NULL);

		void transTaskInfo(st_edmaTaskInfo *pst_old_edma_task_info, st_edmaTaskInfo *pst_new_edma_task_info);

		// for single-descriptor translation
		int fillDesc(st_edmaUnifyInfo *pst_in_edma_desc_info, void *pst_out_edma_desc);
		unsigned char hwVer; //hw version

	public:
		EdmaDescEngine(void);
		~EdmaDescEngine(void);

		int isSupport(st_edma_request_nn* request, st_edma_support_nn* support);

		void setHWver(unsigned char ver);

		unsigned char getHWver(void) { return hwVer;};

		// query descriptor size & number
		// in: request out: result
		// Description:
		//  This function will fill total hw descriptor buffer size and support multi info_list
		//  User can use this buffer size to allcate buffer
		//  and fill descriptor number in edma-apusys command
		int queryDescSize(st_edma_request_nn* request, st_edma_result_nn* result);

		// fillDesc for nn request
		// in: request, out: codebuf (descriptor buffer)
		// Description:
		//  This function will fill src_addr_offset & dst_addr_offset in each info_list
		//  (all offset from start descriptor point) , and fill related hw command in codebuf
		int fillDesc(st_edma_request_nn* request, unsigned char* codebuf);


		// query descriptor size & number for genernal request
		// in: pst_in_edma_task_info out: pui_new_desc_num (descriptor number)
		// return: descriptor size
		// Description:
		//  This function will calculate total hw descriptor buffer size and number
		//  User can use this buffer size to allcate buffer
		//  and fill descriptor number in edma-apusys command
		unsigned int queryDescSize(st_edmaTaskInfo *pst_in_edma_task_info, unsigned int *pui_new_desc_num=NULL);


		// fillDesc for genernal request
		// in: pst_in_edma_task_info, out: pst_out_edma_desc (descriptor buffer)
		// Description:
		//  This function will fill fill related hw command in pst_out_edma_desc depend on
		//  the intput task info and support multi edma info
		int fillDesc(st_edmaTaskInfo *pst_in_edma_task_info, void *pst_out_edma_desc);

#ifdef MVPU_EN
		int fillDesc(st_edmaTaskInfo *pst_in_edma_task_info, void *pst_out_edma_desc, unsigned int* rp_offset);

		//query the number of values which need to be replaced
		unsigned int queryReplaceNum(st_edmaTaskInfo *pst_old_edma_task_info);
#endif

		void queryTransDescSize(st_edmaTaskInfo *pst_old_edma_task_info,
					unsigned int *ui_new_desc_num,
					unsigned int *ui_new_desc_info_buff_size,
					unsigned int *ui_new_desc_buff_size);
};

