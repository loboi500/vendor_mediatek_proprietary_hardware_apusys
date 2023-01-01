
//#define NO_APUSYS_ENGINE

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>

#include "apusys.h"
#include "edmaEngine.h"

#include "edma_cmn.h"
#include "edma_def.h"
//#include "edmaError.h"
#include "edma_core.h"
#ifdef __ANDROID__
#include <cutils/properties.h>
#endif




DeviceEngine::DeviceEngine(const char * userName)
{
    EDMA_LOG_INFO("DeviceEngine(%s)", userName);
    return;
}

DeviceEngine::~DeviceEngine()
{
    return;
}

DeviceEngine * DeviceEngine::createInstance(const char * userName)
{
    DeviceEngine* engine = NULL;

    if (strcmp(EdmaEngine::NAME, userName) == 0)
    {   try {
        engine = (DeviceEngine*) new EdmaEngine(userName);
       } catch (const std::system_error& e){
            EDMA_LOG_ERR("%s system_error occurred, terminate\n", __func__);
        }
    }
    return engine;
}

bool DeviceEngine::deleteInstance(DeviceEngine * engine)
{
    if (engine == nullptr)
    {
        EDMA_LOG_ERR("invalid argument\n");
        return false;
    }

    delete engine;

    return true;
}
#if 0 //delete cmd @ engine destroyCmd

edmaCmd::~edmaCmd()
{

    apusysSession_deleteCmd(mSession, mCmd);

    if (mSubCmd != nullptr)
        apusysSession_deleteCmd(mSession, mSubCmd);

    EDMA_LOG_INFO("delete all cmd ~edmaCmd\n");

    return;
}
#endif


edmaCmd::edmaCmd(apusys_session_t *session)
{
    mSession = session;
    mIsCmdBuildDone = false;
    mSubCmd = nullptr;
    mCmd = apusysSession_createCmd(mSession);
    if (mCmd == nullptr) {
      EDMA_LOG_ERR("%s:mCmd = NULL createCmd fail!!\n",__func__);
      return;
    }
    apusysCmd_setParam(mCmd, CMD_PARAM_PRIORITY, APUSYS_PRIORITY_HIGH);
    EDMA_LOG_INFO("mCmd = 0x%p  #x\n", (void *)mCmd);

    return;
}

int edmaCmd::addSubCmd(edmaMem * subCmdBuf, apusys_device_type deviceType,
                          std::vector<int> & dependency)
{
    EDMA_LOG_INFO("add subCmd va = 0x%p deviceType = %d, mCmd = 0x%p  #x\n", subCmdBuf->va, deviceType, (void *)mCmd);
    mSubCmd = apusysCmd_createSubcmd(mCmd, APUSYS_DEVICE_EDMA);
    EDMA_LOG_INFO("mSubCmd = 0x%p, dependency[0] = %d\n", (void *)mSubCmd, dependency[0]);
    if (mSubCmd != nullptr) {
        int ret = 0;
        ret |= apusysSubCmd_addCmdBuf(mSubCmd, subCmdBuf->va, APUSYS_CB_IN);
        ret |= apusysSubCmd_setParam(mSubCmd, SC_PARAM_BOOST_VALUE, 85);
        if (ret != 0) {
            EDMA_LOG_ERR("addCmdBuf & setParam fail\n");
            return -1;
        }
        mIsCmdBuildDone = false;
    } else {
        EDMA_LOG_ERR("mSubCmd = NULL!!\n");
        return -1;
    }
    EDMA_LOG_INFO("addSubCmd done\n");

    return 1;
}

 int edmaCmd::addSubCmd(std::vector<SubCmdInfo *> subcmd)
 {
    unsigned int i;

    mSubCmd = apusysCmd_createSubcmd(mCmd, APUSYS_DEVICE_EDMA);

    EDMA_LOG_INFO("mSubCmd = 0x%p  #x\n", (void *)mSubCmd);
    for (i=0; i<subcmd.size(); i++) {
      if (mSubCmd != nullptr) {
          int ret = 0;
          EDMA_LOG_INFO("addSubCmd subcmd[%d], va = 0x%p\n", i, subcmd[i]->descMem->va);

          ret |= apusysSubCmd_addCmdBuf(mSubCmd, subcmd[i]->descMem->va, APUSYS_CB_IN);
          ret |= apusysSubCmd_setParam(mSubCmd, SC_PARAM_BOOST_VALUE, 85);
          if (ret != 0) {
              EDMA_LOG_ERR("addCmdBuf & setParam fail\n");
              return -1;
          }
          mIsCmdBuildDone = false;
      }
    }
    return 1;
 }


 int edmaCmd::addSubCmd(std::vector<SubCmdInfo *> subcmd, int type, uint64_t val)
 {
    unsigned int i;

    mSubCmd = apusysCmd_createSubcmd(mCmd, APUSYS_DEVICE_EDMA);

    EDMA_LOG_INFO("mSubCmd = 0x%p  #x\n", (void *)mSubCmd);

    for (i=0; i<subcmd.size(); i++) {
      if (mSubCmd != nullptr) {
          int ret = 0;
          EDMA_LOG_INFO("addSubCmd subcmd[%d], va = 0x%p\n", i, subcmd[i]->descMem->va);
          EDMA_LOG_INFO("type[%d], val = %d\n", type, (int)val);
          ret |= apusysSubCmd_addCmdBuf(mSubCmd, subcmd[i]->descMem->va, APUSYS_CB_IN);
          ret |= apusysSubCmd_setParam(mSubCmd, (enum apusys_subcmd_param_type) type, val);
          if (ret != 0) {
              EDMA_LOG_ERR("addCmdBuf & setParam fail\n");
              return -1;
          }
          mIsCmdBuildDone = false;
      }
    }
    return 1;
 }

const char * EdmaEngine::NAME = "edma_engine";


unsigned int gEdmaLogLv = 0;

static void getLogLevel()
{
#ifdef __ANDROID__
    char prop[100];
    memset((void *)prop, 0, sizeof(prop));
    property_get(PROPERTY_DEBUG_EDMA_LEVEL, prop, "0");
    gEdmaLogLv = atoi(prop);
#else
    char *prop;

    prop = std::getenv(PROPERTY_DEBUG_EDMA_LEVEL);
    if (prop != nullptr)
        gEdmaLogLv = atoi(prop);
#endif

    EDMA_LOG_DEBUG("debug loglevel = %d\n", gEdmaLogLv);
    return;
}


EdmaEngine::EdmaEngine(const char * userName):DeviceEngine(userName)
{
    uint64_t deviceNum = 0;
    char meta_data[32];
    int ret = 0;

    EDMA_DEBUG_TAG;
    mRequestList.clear();
    mDesEngine = new EdmaDescEngine();
    errorCode = 0;
    mSession = apusysSession_createInstance();

    getLogLevel();

    if (mSession != nullptr) {
        deviceNum = apusysSession_queryDeviceNum(mSession, APUSYS_DEVICE_EDMA);
        if (deviceNum == 0)
            EDMA_LOG_ERR("APUSYS_DEVICE_EDMA = 0, not support edma!!!\n");
        else
            EDMA_LOG_DEBUG("EDMA device number = %d\n", (int)deviceNum);

        ret = apusysSession_queryDeviceMetaData(mSession, APUSYS_DEVICE_EDMA, (void *)meta_data);
        EDMA_LOG_ERR("meta_data[0] = %d, ret = %d\n", meta_data[0], ret);
        mDesEngine->setHWver(meta_data[0]);

    } else
        EDMA_LOG_ERR("%s apusys session get fail!!!\n", __func__);

    return;
}

EdmaEngine::~EdmaEngine()
{
    std::vector<edmaCmd *>::iterator iter;
    EDMA_DEBUG_TAG;

    /* free all memory */
    std::unique_lock<std::mutex> mutexLock(mListMtx);

    for (iter = mRequestList.begin(); iter != mRequestList.end(); iter++)
    {
        mRequestList.erase(iter);
        delete (*iter);
    }

    if (!mRequestList.empty())
    {
        EDMA_LOG_ERR("mRequestList delete fail!!\n");
    }
    EDMA_DEBUG_TAG;
    mRequestList.clear();

    delete mDesEngine;
    if (apusysSession_deleteInstance(mSession) != 0)
        EDMA_LOG_ERR("apusysSession_deleteInstance fail!!\n");

    return;
}

edmaMem * EdmaEngine::memAlloc(size_t size)
{
    edmaMem *pApu_mem = NULL;
    void *va = NULL;

    if (mSession == NULL) {
        EDMA_LOG_ERR("mSession = NULL!\r\n");        
        return nullptr;
    }
    va = apusysSession_memAlloc(mSession, size, 32, APUSYS_MEM_TYPE_DRAM, F_APUSYS_MEM_32BIT);
    pApu_mem = new edmaMem();
    pApu_mem->va = va;
    if (pApu_mem->va ==  nullptr) {
        EDMA_LOG_ERR("allocte error!");
        delete pApu_mem;
        return nullptr;
    }
    pApu_mem->iova = apusysSession_memGetInfoFromHostPtr(mSession, pApu_mem->va, APUSYS_MEM_INFO_GET_DEVICE_VA);
    pApu_mem->type = EDMA_MEM_TYPE_NORMAL;
    pApu_mem->iova_size = size;
    return pApu_mem;
}

edmaMem * EdmaEngine::memAllocVLM(size_t size)
{
    edmaMem *pApu_mem = NULL;
    void *va = NULL;
    va = apusysSession_memAlloc(mSession, 0, 0, APUSYS_MEM_TYPE_VLM, 0); //get full start address only
    pApu_mem = new edmaMem();
    pApu_mem->va = va;
    if (pApu_mem->va ==  nullptr) {
        EDMA_LOG_ERR("allocte error!");
        delete pApu_mem;
        return nullptr;
    }
    pApu_mem->iova = apusysSession_memGetInfoFromHostPtr(mSession, pApu_mem->va, APUSYS_MEM_INFO_GET_DEVICE_VA);
    pApu_mem->type = EDMA_MEM_TYPE_NORMAL;
    pApu_mem->iova_size = size;
    return pApu_mem;
}

edmaMem * EdmaEngine::memAllocCmd(size_t size)
{
    edmaMem *pApu_mem = NULL;
    void *va = NULL;

    EDMA_LOG_INFO("memAllocCmd size = %zu\n", size);
    va = apusysSession_cmdBufAlloc(mSession, size, 128);
    pApu_mem = new edmaMem();
    pApu_mem->va = va;
    if (pApu_mem->va ==  nullptr) {
        EDMA_LOG_ERR("allocte error!");
        delete pApu_mem;
        return nullptr;
    }
    pApu_mem->size = size;
    pApu_mem->iova = 0;
    pApu_mem->iova_size = size;
    pApu_mem->type = EDMA_MEM_TYPE_CMDBUF;

    EDMA_LOG_INFO("pApu_mem->va = 0x%p\n", pApu_mem->va);

    return pApu_mem;
}


bool EdmaEngine::memFree(edmaMem * mem)
{
    if (mem->type == EDMA_MEM_TYPE_CMDBUF)
        apusysSession_cmdBufFree(mSession, mem->va);
    else
        apusysSession_memFree(mSession, mem->va);

    delete mem;
    return true;
}

bool EdmaEngine::runCmd(edmaCmd * cmd)
{
    /* build cmd */
    if (cmd->isCmdBuildDone()) {

        EDMA_LOG_INFO("no need rebuild, direct run\n");
        if (apusysCmd_run(cmd->mCmd) != 0)
            EDMA_LOG_ERR("apusysCmd_run fail!!\n");
    } else {
    if (apusysCmd_build(cmd->mCmd) == 0) {
        /* execution */
        cmd->setCmdBuildDone();
        if (apusysCmd_run(cmd->mCmd) != 0)
            EDMA_LOG_ERR("apusysCmd_run fail!!\n");
    } else
        EDMA_LOG_ERR("apusysCmd_build fail!!\n");
    }
    return true;
}

edmaCmd * EdmaEngine::initCmd()
{
    edmaCmd *apusysCmd = new edmaCmd(mSession);
    return apusysCmd;
}



bool EdmaEngine::destroyCmd(edmaCmd * cmd)
{
    apusysSession_deleteCmd(mSession, cmd->mCmd);
    delete cmd;
    return true;
}

bool EdmaEngine::runCmdAsync(edmaCmd * cmd)
{
    if (cmd->isCmdBuildDone()) {

        EDMA_LOG_INFO("no need rebuild, direct run\n");
        if (apusysCmd_runAsync(cmd->mCmd) != 0)
            EDMA_LOG_ERR("apusysCmd_runAsync fail!!\n");
    } else {
    if (apusysCmd_build(cmd->mCmd) == 0) {
        /* execution */
        cmd->setCmdBuildDone();
        if (apusysCmd_runAsync(cmd->mCmd) != 0)
            EDMA_LOG_ERR("apusysCmd_runAsync fail!!\n");
    } else
        EDMA_LOG_ERR("apusysCmd_runAsync fail!!\n");
    }
    return true;
}

bool EdmaEngine::waitCmd(edmaCmd * cmd)
{
    int ret = 0;
    EDMA_LOG_INFO("%s mCmd = 0x%p\n",__func__ , (void *)cmd->mCmd);
	ret = apusysCmd_wait(cmd->mCmd);
	if (ret < 0)
        EDMA_LOG_ERR("apusysCmd_wait(%d) fail!!\n", ret);

    return true;

}

edmaCmd* EdmaEngine::getCmd()
{
    EDMA_DEBUG_TAG;
    edmaCmd * cmd = new edmaCmd(mSession);
    EDMA_DEBUG_TAG;

    if (cmd == nullptr)
    {
        EDMA_LOG_ERR("allocate edma cmd fail\n");
        return nullptr;
    }

    mRequestList.push_back(cmd);

    return (edmaCmd *)cmd;
}

bool EdmaEngine::releaseCmd(edmaCmd* ICmd) {
    std::vector<edmaCmd *>::iterator iter;

    EDMA_DEBUG_TAG;

    std::unique_lock<std::mutex> mutexLock(mListMtx);
    for (iter = mRequestList.begin(); iter != mRequestList.end(); iter++)
    {
        if (*iter == ICmd)
        {
            EDMA_LOG_DEBUG("subcmd = %p\n", (void *)*iter);
            mRequestList.erase(iter);
            EDMA_DEBUG_TAG;
            delete (*iter);
            EDMA_DEBUG_TAG;
            return true;
        }
    }

    return false;
}

bool EdmaEngine::runSync(edmaCmd* ICmd)
{
    EDMA_DEBUG_TAG;

    /* run cmd */
    if(runCmd(ICmd) == false)
    {
        EDMA_DEBUG_TAG;
        return false;
    }
    EDMA_DEBUG_TAG;

    return true;
}

edmaMem* EdmaEngine::getSubCmdBuf(edmaCmd* ICmd)
{
    edmaCmd * cmd = (edmaCmd *)ICmd;
    std::vector<edmaCmd *>::iterator iter;
    EDMA_DEBUG_TAG;

    if (cmd == nullptr)
    {
        EDMA_LOG_ERR("invalid argument\n");
        return nullptr;
    }
    EDMA_DEBUG_TAG;

    std::unique_lock<std::mutex> mutexLock(mListMtx);
    for (iter = mRequestList.begin(); iter != mRequestList.end(); iter++)
    {
        if (*iter == ICmd)
        {
            EDMA_DEBUG_TAG;
            return cmd->getCmdBuf();
        }
    }

    EDMA_LOG_ERR("can't find this edma cmd\n");
    EDMA_DEBUG_TAG;

    return nullptr;
}


void EdmaEngine::checkDescNum(unsigned int ui_desc_num)
{
    // ui_desc_num <= MAX_EDMA_DESC_NUM
    if (ui_desc_num > MAX_EDMA_DESC_NUM)
    {
        EDMA_LOG_ERR("ui_desc_num <= %d\n", MAX_EDMA_DESC_NUM);
    }
}

//all use orignal edma cmd format

bool EdmaEngine::querySubCmdInfo(std::vector<void*> data, std::vector<int>& size)
{
    unsigned int ui_new_desc_info_buff_size, ui_new_desc_buff_size, ui_new_desc_num;

    // get info of edma task
    st_edmaTaskInfo *pst_edma_task_info = (st_edmaTaskInfo *)data[0];

    mDesEngine->queryTransDescSize(pst_edma_task_info, &ui_new_desc_num, &ui_new_desc_info_buff_size, &ui_new_desc_buff_size);

    // return size of memories
    size.push_back(sizeof(edma_ext));
    checkDescNum(ui_new_desc_num);
    size.push_back(ui_new_desc_buff_size);

    return true;
}
/*Fill sub command request and descriptor*/
bool EdmaEngine::fillDesc(std::vector<void *> data, std::vector<SubCmdInfo *>& devDesc)
{
    // get info of edma task
    st_edmaTaskInfo *pst_edma_task_info = (st_edmaTaskInfo *)data[0];

    // get apusys_subcmd_buff
    SubCmdInfo* apusys_subcmd_info = devDesc[0];
    apusys_subcmd_info->name = "edma_req";
    struct edma_ext *req_ext = (struct edma_ext *)apusys_subcmd_info->descMem->va;

    SubCmdInfo* edma_desc_subcmd_info;

    static uint32_t ui_algo_id = 0;
    unsigned int ui_new_desc_info_buff_size, ui_new_desc_buff_size, ui_new_desc_num;

    mDesEngine->queryTransDescSize(pst_edma_task_info, &ui_new_desc_num, &ui_new_desc_info_buff_size, &ui_new_desc_buff_size);

    edma_desc_subcmd_info = devDesc[1]; // change to two input is bettter?

    void *pst_desc = (void *)edma_desc_subcmd_info->descMem->va;
    EDMA_LOG_INFO("set edma_engine for all\n");
    req_ext->cmd_handle = 0;
    req_ext->fill_value = 0;
#ifdef DISABLE_IOMMU
    req_ext->desp_iommu_en = 0;
    EDMA_LOG_INFO("DISABLE IOMMU for CA!!\n");

#else
    req_ext->desp_iommu_en = 1;
#endif
    //req_ext->count = pst_edma_task_info->info_num;
    checkDescNum(ui_new_desc_num);
    req_ext->count = ui_new_desc_num;
    EDMA_LOG_INFO("set ex_count = %d\n",req_ext->count);

    req_ext->reg_addr = edma_desc_subcmd_info->descMem->iova;

    EDMA_LOG_INFO("set edma_engine for EDMA_INFO_GENERAL || EDMA_INFO_NN, req_ext->count = %d done\n", req_ext->count );
    mDesEngine->fillDesc(pst_edma_task_info, pst_desc);
#ifdef MVPU_EN
    unsigned int ui_idx = 0;
    unsigned int rp_num = 0;
    unsigned int* rp_offset;
    rp_num = mDesEngine->queryReplaceNum(pst_edma_task_info);
    rp_offset = (unsigned int *)calloc(rp_num, sizeof(unsigned int));

    // fill edma descriptors
    if(rp_offset != NULL)
        mDesEngine->fillDesc(pst_edma_task_info, pst_desc, rp_offset);
    else
        mDesEngine->fillDesc(pst_edma_task_info, pst_desc);

    if(pst_edma_task_info->is_binary_record) {
        EDMA_LOG_INFO("=== Fill replace info to PTN ===\n");
        apusys_subcmd_info->ptnAddrInfo.emplace_back(std::make_pair(offsetof(edma_ext, reg_addr), 0));
        for (ui_idx = 0; ui_idx < rp_num; ui_idx++) {
            edma_desc_subcmd_info->ptnAddrInfo.emplace_back(std::make_pair(rp_offset[ui_idx], 0));
        }
    }

    if(rp_offset != NULL)
        free(rp_offset);
#endif

    ui_algo_id++;

    return true;
}

unsigned int EdmaEngine::getError(void)
{
    return errorCode;
}

const char* EdmaEngine::getErrorString(int err)
{
    switch (err) {
    case 0:              return "SUCCESS";
    default: return "UNKNOWN";
    }
}

#if 0
bool EdmaEngine::memFree(void * mem)
{
    return ((mvpuAllocator*)mDramAllocator)->free((edmaMem *)mem);
}
#endif

//---------------------------------------------
#if 0
EdmaDescEngine * EdmaDescEngine::createInstance(const char * userName)
{
    EdmaEngine * engine = new EdmaEngine(userName);

    EDMA_DEBUG_TAG;

    return (EdmaDescEngine*) engine;
}

bool EdmaDescEngine::deleteInstance(EdmaDescEngine * engine)
{
    if (engine == nullptr)
    {
        EDMA_LOG_ERR("invalid argument\n");
        return false;
    }

    delete (EdmaEngine *)engine;
    return true;
}
#endif



