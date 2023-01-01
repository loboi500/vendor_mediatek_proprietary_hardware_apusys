#pragma once

#include <string>
#include <vector>
//#include "apusysEngine.h"
#include "edma_info.h"
#include "apusys.h"
#include "DeviceEngine.h"
#include "edmaDesEngine.h"
#include <iostream>       // std::cout
#include <thread>         // std::thread
#include <mutex>          // std::mutex, std::unique_lock
#include <map>

using namespace std;

enum edma_mem_type {
    EDMA_MEM_TYPE_NORMAL = 0, //normal ion memory.
    EDMA_MEM_TYPE_CMDBUF = 1, //cmd buffer memory, cmd data, HW descriptor buffer...
    EDMA_MEM_TYPE_NONE = 2,
};


class edmaMem {
 public:

    void *va;
    unsigned long long iova;
    unsigned int iova_size;

    unsigned int size;

    edma_mem_type type;
};

typedef struct _SubCmdInfo {
	std::string name;
	edmaMem* descMem;
#ifdef MVPU_EN
	std::vector<std::pair<unsigned int, unsigned int>> ptnAddrInfo;
#endif

	//Record real usage amount in descMem
	uint32_t memUsageAmount;
} SubCmdInfo;


class edmaCmd {
	apusys_session_t *mSession;
 private:
 	bool mIsCmdBuildDone;
 public:
	 apusys_cmd_t *mCmd;
	 apusys_subcmd_t *mSubCmd;
 
 	edmaCmd(apusys_session_t *session);
    virtual ~edmaCmd() {}

    /*
     * @addSubCmd: add subcmd(subgraph) into apusys cmd
     *  description:
     *      add subcmd into apusys cmd
     *  arguement:
     *      <subCmdBuf>: subcmd buffer contain communicate driver structure, device driver will get this buffer at scheduler dispatching
     *      <deviceType>: device type
     *      <dependency>: idx vector array, this subcmd should be dispatched after all dependency idx subcmd finished
     *      <ctxId>: memory context id, user descript vlm memory map by this param
     *  return:
     *      success: idx(>=0)
     *      fail: -1
     */
    virtual int addSubCmd(edmaMem * subCmdBuf, apusys_device_type deviceType,
                          std::vector<int> & dependency);

	virtual int addSubCmd(std::vector<SubCmdInfo *> subcmd);
	virtual int addSubCmd(std::vector<SubCmdInfo *> subcmd, int type, uint64_t val);
    /*
     * @packSubCmd: add subcmd and pack indicate subcmd
     *  description:
     *      add subcmd and pack indicate subcmd, the dependency will follow target subcmd
     *      apusys is responsible to trigger all packed subcmd at the same time
     *      used to user multicore application
     *  arguement:
     *      <subCmdBuf>: subcmd buffer contain communicate driver structure, device driver will get this buffer at scheduler dispatching
     *      <deviceType>: device type
     *      <subCmdIdx>: target subcmd user want to packed with
     *  return:
     *      success: idx(>=0)
     *      fail: -1
     */
    //virtual int packSubCmd(edmaMem * subCmdBuf, apusys_device_type deviceType,
                         //  int subCmdIdx) { return 1;};

	virtual edmaMem * getCmdBuf() { return 0; };


 	void setCmdBuildDone() {mIsCmdBuildDone = true;}

	bool isCmdBuildDone() { return mIsCmdBuildDone;}
};





typedef struct _edmaCmdInfo {
	int version;
	std::vector<void *> cmds;
	st_edmaTaskInfo *cmdInfo;
	void *request_buf;
	void *pst_desc_list;

} edmaCmdInfo;

class DeviceEngine {
public:
	DeviceEngine(const char * userName);
	virtual ~DeviceEngine();

	/* new interface to support runtime free DrvMem */
	//virtual bool memFree(void * mem) = 0;

	/* device dependent interface to acquire descriptor  */
	// virtual edmaMem * initSubCmdBuf(void * data) = 0;

	/*size[0] = request cmd size, size[1] = total descriptor size*/
	virtual bool querySubCmdInfo(std::vector<void*> data, std::vector<int>& size) = 0;
	virtual bool fillDesc(std::vector<void*> data, std::vector<SubCmdInfo*>& devDesc) = 0;
	virtual edmaMem * memAlloc(size_t size) = 0;
	virtual edmaMem * memAllocCmd(size_t size) = 0;
	virtual edmaMem * memAllocVLM(size_t size) = 0;
	virtual bool memFree(edmaMem * mem) = 0;
	virtual bool runCmd(edmaCmd * cmd) = 0;
	virtual edmaCmd * initCmd() = 0;
	virtual bool destroyCmd(edmaCmd * cmd) = 0;

	virtual bool runCmdAsync(edmaCmd * cmd) = 0;
	virtual bool waitCmd(edmaCmd * cmd) = 0;

	static DeviceEngine * createInstance(const char * userName);
	static bool deleteInstance(DeviceEngine * engine);

};

class EdmaEngine : public DeviceEngine {
private:
	std::vector<edmaCmd *> mRequestList;
	std::mutex mListMtx;
	unsigned int errorCode;
	EdmaDescEngine *mDesEngine;
    apusys_session_t *mSession;	
	void checkDescNum(unsigned int ui_desc_num);

public:
	EdmaEngine(const char*);
	~EdmaEngine();

	//bool memFree(void * mem);

	/* for normal user trigger directly */
	edmaCmd * getCmd();
	bool releaseCmd(edmaCmd * ICmd);
	bool runSync(edmaCmd * ICmd);

	/* for user who want to construct big apusys cmd */
	edmaMem * getSubCmdBuf(edmaCmd * ICmd);

	//query sub command info which include apusys command size and descriptor buffer size
	bool querySubCmdInfo(std::vector<void*> data, std::vector<int>& size);

	//fill edma descriptor & apusys cmd dependent sub cmd infos
	bool fillDesc(std::vector<void *> data, std::vector<SubCmdInfo *>& devDesc);
	static const char*  getErrorString(int);

	/* for error handle */
	unsigned int getError();

	edmaMem * memAlloc(size_t size) override;
	edmaMem * memAllocVLM(size_t size) override;    
	edmaMem * memAllocCmd(size_t size) override;    

	bool memFree(edmaMem * mem) override;
	bool runCmd(edmaCmd * cmd) override;
	edmaCmd * initCmd() override;	
	bool destroyCmd(edmaCmd * cmd) override;

	bool runCmdAsync(edmaCmd * cmd) override;
	bool waitCmd(edmaCmd * cmd) override;


	static const char * NAME;
};


