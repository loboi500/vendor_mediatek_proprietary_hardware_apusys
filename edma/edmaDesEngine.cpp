
//#define NO_APUSYS_ENGINE

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>

#include "edma_cmn.h"
//#include "edmaError.h"
#include "edma_core.h"
#include "edmaDesEngine.h"
#include "edma_def.h"



unsigned int EdmaDesGGeneralV20::queryDSize(st_edmaUnifyInfo *in_info)
{
    EDMA_LOG_INFO("test v20 descriptor generator");

    return edmav20_queryDSize(EDMA_INFO_GENERAL,in_info);
}

unsigned int EdmaDesGGeneralV20::queryDNum(st_edmaUnifyInfo *in_info)
{
    EDMA_LOG_INFO("test v20 queryDNum");

    return edmav20_queryDNum(EDMA_INFO_GENERAL,in_info);
}

int EdmaDesGGeneralV20::fillDesc(st_edmaUnifyInfo *shape, void *edma_desc)
{
    return fillDescV20(shape,edma_desc);
}

unsigned int EdmaDesGDataV20::queryDSize(st_edmaUnifyInfo *in_info)
{
    EDMA_LOG_INFO("test jack #3 v20 descriptor generator");

    return edmav20_queryDSize(EDMA_INFO_DATA,in_info);
}
int EdmaDesGDataV20::fillDesc(st_edmaUnifyInfo *shape, void *edma_desc)
{
    return fillDescDataV20(shape,edma_desc);
}

int EdmaDesGUFBCV20::fillDesc(st_edmaUnifyInfo *shape, void *edma_desc)
{
    return fillDescUFBCV20(shape,edma_desc,getHeadDesc());
}

unsigned int EdmaDesGUFBCV20::queryDSize(st_edmaUnifyInfo *in_info)
{
    return edmav20_queryUFBCDSize(in_info);
}


unsigned int EdmaDesGGeneral::queryDSize(st_edmaUnifyInfo *in_info)
{
    EDMA_LOG_INFO("test jack #1 descriptor generator");

    return edma_queryDSize(EDMA_INFO_GENERAL,in_info);
}

unsigned int EdmaDesGSDK::queryDSize(st_edmaUnifyInfo *in_info)
{
    EDMA_LOG_INFO("test sdk descriptor generator");

    return edma_querySDKDSize(EDMA_INFO_SDK,in_info);
}

int EdmaDesGSDK::fillDesc(st_edmaUnifyInfo *shape, void *edma_desc)
{
    return fillDescSDK(shape,edma_desc);
}

unsigned int EdmaDesGGeneral::queryDNum(st_edmaUnifyInfo *in_info)
{
    EDMA_LOG_INFO("test jack #1 queryDNum");

    return edma_queryDNum(EDMA_INFO_GENERAL,in_info);
}

int EdmaDesGGeneral::fillDesc(st_edmaUnifyInfo *shape, void *edma_desc)
{
    return fillDescV0(shape,edma_desc,getHeadDesc());
}

unsigned int EdmaDesGData::queryDSize(st_edmaUnifyInfo *in_info)
{
    EDMA_LOG_INFO("test jack #3 descriptor generator");

    return edma_queryDSize(EDMA_INFO_DATA,in_info);
}
int EdmaDesGData::fillDesc(st_edmaUnifyInfo *shape, void *edma_desc)
{
    return fillDescData(shape,edma_desc,getHeadDesc());
}

unsigned int EdmaDesGNN::queryDSize(st_edmaUnifyInfo *in_info)
{
    EDMA_LOG_INFO("test jack #2 descriptor generator");
    return edma_queryDSize(EDMA_INFO_NN,in_info);
}

unsigned int EdmaDesGNN::queryDNum(st_edmaUnifyInfo *in_info)
{
    EDMA_LOG_INFO("NN queryDNum");

    return edma_queryDNum(EDMA_INFO_NN,in_info);
}

int EdmaDesGNN::fillDesc(st_edmaUnifyInfo *shape, void *edma_desc)
{
    return fillDescNN(shape,edma_desc,getHeadDesc());
}

int EdmaDesGUFBC::fillDesc(st_edmaUnifyInfo *shape, void *edma_desc)
{
    return fillDescUFBC(shape,edma_desc,getHeadDesc());
}

unsigned int EdmaDesGUFBC::queryDSize(st_edmaUnifyInfo *in_info)
{
    return edma_queryUFBCDSize(in_info);
}

int EdmaDesGSlice::fillDesc(st_edmaUnifyInfo *shape , void *edma_desc)
{
    return fillDescSlice(shape, edma_desc, getHeadDesc());
}

unsigned int EdmaDesGSlice::queryDSize(st_edmaUnifyInfo *in_info)
{
    return edma_querySliceDSize(in_info);
}

int EdmaDesGPad::fillDesc(st_edmaUnifyInfo *shape , void *edma_desc)
{
    return fillDescPad(shape, edma_desc, getHeadDesc());
}

unsigned int EdmaDesGPad::queryDNum(st_edmaUnifyInfo *in_info)
{
    return edma_queryPadDNum(in_info);
}

unsigned int EdmaDesGPad::queryDSize(st_edmaUnifyInfo *in_info)
{
    return edma_queryPadDSize(in_info);
}

unsigned int EdmaDesGBayerToRGGB::queryDSize(st_edmaUnifyInfo *in_info)
{
    return edma_queryBayerToRGGBDSize(in_info);
}

int EdmaDesGBayerToRGGB::fillDesc(st_edmaUnifyInfo *shape, void *edma_desc)
{
    return fillDescBayerToRGGB(shape, edma_desc, getHeadDesc());
}

unsigned int EdmaDesGRGGBToBayer::queryDSize(st_edmaUnifyInfo *in_info)
{
    return edma_queryRGGBToBayerDSize(in_info);
}

int EdmaDesGRGGBToBayer::fillDesc(st_edmaUnifyInfo *shape, void *edma_desc)
{
    return fillDescRGGBToBayer(shape, edma_desc, getHeadDesc());
}

#ifdef MVPU_EN
unsigned int EdmaDesGMVPU::queryDSize(st_edmaUnifyInfo *in_info)
{
    return edma_queryMVPUDSize(in_info);
}

unsigned int EdmaDesGMVPU::queryDNum(st_edmaUnifyInfo *in_info)
{
    //EDMA_LOG_INFO("test jack #1 queryDNum");

    return edma_queryMVPUDNum(in_info);
}

unsigned int EdmaDesGMVPU::queryRPNum(st_edmaUnifyInfo *in_info)
{
    return edma_queryMVPURPNum(in_info);
}

void EdmaDesGMVPU::fillRPOffset(st_edmaTaskInfo *task_info, unsigned int *id, EDMA_INFO_TYPE type, unsigned int * rp_offset, unsigned int desc_offset)
{
    EDMA_LOG_INFO("fillRPOffset id %d, type %d, desc_offset 0x%08x\n", *id, type, desc_offset);
    switch(type){
    case EDMA_DESC_INFO_TYPE_0:
    case EDMA_DESC_INFO_TYPE_1:
        rp_offset[*id] = desc_offset + 0x1C;
        (*id) += 1;
        rp_offset[*id] = desc_offset + 0x20;
        (*id) += 1;
        break;
    case EDMA_DESC_INFO_COLORCVT:
        rp_offset[*id] = desc_offset + 0x1C;
        (*id) += 1;
        rp_offset[*id] = desc_offset + 0x20;
        (*id) += 1;
        rp_offset[*id] = desc_offset + 0x5C;
        (*id) += 1;
        rp_offset[*id] = desc_offset + 0x60;
        (*id) += 1;
        break;
    case EDMA_DESC_INFO_ROTATE:
    {
        st_edmaDescInfoRotate *desc_info = &task_info->edma_info[0].st_info.infoRotate;
        unsigned int fmt = 0;
        fmt = desc_info->uc_fmt;
        //#1
        rp_offset[*id] = desc_offset + 0x1C;
        (*id) += 1;

        //#2
        desc_offset = desc_offset + 0x40;

        //#3
        switch(fmt) {
        case EDMA_FMT_ROTATE_90:
        case EDMA_FMT_ROTATE_270:
            desc_offset = desc_offset + 0x40;
            break;
        case EDMA_FMT_ROTATE_180:
            desc_offset = desc_offset + 0x100;
            break;
        default:
            break;
        }
        rp_offset[*id] = desc_offset + 0x20;
        (*id) += 1;
        break;
    }
    case EDMA_DESC_INFO_SPLITN:
        //#1
        rp_offset[*id] = desc_offset + 0x1C;
        (*id) += 1;

        //#2
        desc_offset = desc_offset + 0x40;

        //#3
        desc_offset = desc_offset + 0x100;
        rp_offset[*id] = desc_offset + 0x20;
        (*id) += 1;
        rp_offset[*id] = desc_offset + 0x60;
        (*id) += 1;
        rp_offset[*id] = desc_offset + 0xA0;
        (*id) += 1;
        rp_offset[*id] = desc_offset + 0xE0;
        (*id) += 1;
        break;
    case EDMA_DESC_INFO_MERGEN:
        //#1
        rp_offset[*id] = desc_offset + 0x1C;
        (*id) += 1;
        rp_offset[*id] = desc_offset + 0x5C;
        (*id) += 1;
        rp_offset[*id] = desc_offset + 0x9C;
        (*id) += 1;
        rp_offset[*id] = desc_offset + 0xDC;
        (*id) += 1;

        //#2
        desc_offset = desc_offset + 0x100;

        //#3
        desc_offset = desc_offset + 0x100;
        rp_offset[*id] = desc_offset + 0x20;
        (*id) += 1;
        break;
    case EDMA_DESC_INFO_UNPACK:
        rp_offset[*id] = desc_offset + 0x1C;
        (*id) += 1;
        rp_offset[*id] = desc_offset + 0x20;
        (*id) += 1;
        break;
    case EDMA_DESC_INFO_SWAP2:
    case EDMA_DESC_INFO_565TO888:
        rp_offset[*id] = desc_offset + 0x1C;
        (*id) += 1;
        rp_offset[*id] = desc_offset + 0x20;
        (*id) += 1;
        break;
    case EDMA_DESC_INFO_PACK:
        rp_offset[*id] = desc_offset + 0x1C;
        (*id) += 1;
        rp_offset[*id] = desc_offset + 0x20;
        (*id) += 1;
        break;
    default:
        break;
    }

    return;
}

int EdmaDesGMVPU::fillDesc(st_edmaUnifyInfo *shape, void *edma_desc)
{
    int *pi_errorCode=NULL;
    EDMA_INFO_TYPE type_real;

    EDMA_LOG_INFO("=== EdmaDesGMVPU info type: %d\n", shape->info_type);
    type_real = checkMVPUDescrpType(shape);

    switch (type_real){
    case EDMA_DESC_INFO_TYPE_0:
        fillDescType0(&shape->st_info.info0, (st_edmaDescType0 *)edma_desc, pi_errorCode);
        break;
    case EDMA_DESC_INFO_TYPE_1:
        fillDescType1(&shape->st_info.info1, (st_edmaDescType1 *)edma_desc, pi_errorCode);
        break;
    case EDMA_DESC_INFO_TYPE_5:
        fillDescType5(&shape->st_info.info5, (st_edmaDescType5 *)edma_desc, pi_errorCode);
        break;
    case EDMA_DESC_INFO_ROTATE:
    {
        st_edmaDescInfoRotate *desc_info = &shape->st_info.infoRotate;
        unsigned int fmt = 0;
        fmt = desc_info->uc_fmt;

        st_edmaDescType0  *pst_desc_0 = (st_edmaDescType0  *)edma_desc;
        st_edmaDescType0  *pst_desc_1 = (st_edmaDescType0  *)((uintptr_t)edma_desc+(uintptr_t)sizeof(st_edmaDescType0));
        st_edmaDescType0  *pst_desc_2 = (st_edmaDescType0  *)((uintptr_t)edma_desc+(uintptr_t)sizeof(st_edmaDescType0)+(uintptr_t)sizeof(st_edmaDescType0));
        st_edmaDescType15 *pst_desc_3 = (st_edmaDescType15 *)((uintptr_t)edma_desc+(uintptr_t)sizeof(st_edmaDescType0));
        st_edmaDescType0  *pst_desc_4 = (st_edmaDescType0  *)((uintptr_t)edma_desc+(uintptr_t)sizeof(st_edmaDescType15)+(uintptr_t)sizeof(st_edmaDescType0));
        switch(fmt) {
        case EDMA_FMT_ROTATE_90:
            EDMA_LOG_DEBUG("--- fillDesc - 90 1st ---\n");
            fillDescRotOutBufIn(desc_info, pst_desc_0, pi_errorCode);
            EDMA_LOG_DEBUG("--- fillDesc - 90 2nd ---\n");
            fillDescRotate90(desc_info, pst_desc_1, pi_errorCode);
            EDMA_LOG_DEBUG("--- fillDesc - 90 3rd ---\n");
            fillDescRotInBufOut(desc_info, pst_desc_2, pi_errorCode);
            break;
        case EDMA_FMT_ROTATE_180:
            EDMA_LOG_DEBUG("--- fillDesc - 180 1st ---\n");
            fillDescRotOutBufIn(desc_info, pst_desc_0, pi_errorCode);
            EDMA_LOG_DEBUG("--- fillDesc - 180 2nd ---\n");
            fillDescRotate180(desc_info, pst_desc_3, pi_errorCode);
            EDMA_LOG_DEBUG("--- fillDesc - 180 3rd ---\n");
            fillDescRotInBufOut(desc_info, pst_desc_4, pi_errorCode);
            break;
        case EDMA_FMT_ROTATE_270:
            EDMA_LOG_DEBUG("--- fillDesc - 270 1st ---\n");
            fillDescRotOutBufIn(desc_info, pst_desc_0, pi_errorCode);
            EDMA_LOG_DEBUG("--- fillDesc - 270 2nd ---\n");
            fillDescRotate270(desc_info, pst_desc_1, pi_errorCode);
            EDMA_LOG_DEBUG("--- fillDesc - 270 3rd ---\n");
            fillDescRotInBufOut(desc_info, pst_desc_2, pi_errorCode);
            break;
        default:
            EDMA_LOG_ERR("unsupported rotate format: %d\n", fmt);
            //EDMA_PERROR_CODE_RETURN(pi_errorCode, EDMA_DRV_ERR_INVALID_FMT);
            break;
        }
        break;
    }
    case EDMA_DESC_INFO_SPLITN:
    {
        st_edmaDescInfoSplitN *desc_info = &shape->st_info.infoSplitN;
        st_edmaDescType0  *pst_desc_1 = (st_edmaDescType0  *)edma_desc;
        st_edmaDescType15 *pst_desc_2 = (st_edmaDescType15 *)((uintptr_t)edma_desc+(uintptr_t)sizeof(st_edmaDescType0));
        st_edmaDescType15 *pst_desc_3 = (st_edmaDescType15 *)((uintptr_t)edma_desc+(uintptr_t)sizeof(st_edmaDescType0)+(uintptr_t)sizeof(st_edmaDescType15));

        EDMA_LOG_DEBUG("--- Split fillDesc - 1st ---\n");
        fillDescSplitNIn(desc_info, pst_desc_1, pi_errorCode);

        EDMA_LOG_DEBUG("--- Split fillDesc - 2nd ---\n");
        fillDescSplitN(desc_info, pst_desc_2, pi_errorCode);

        EDMA_LOG_DEBUG("--- Split fillDesc - 3rd ---\n");
        fillDescSplitNOut(desc_info, pst_desc_3, pi_errorCode);
        break;
    }
    case EDMA_DESC_INFO_MERGEN:
    {
        st_edmaDescInfoMergeN *desc_info = &shape->st_info.infoMergeN;
        st_edmaDescType15 *pst_desc_1 = (st_edmaDescType15 *)edma_desc;
        st_edmaDescType15 *pst_desc_2 = (st_edmaDescType15 *)((uintptr_t)edma_desc+(uintptr_t)sizeof(st_edmaDescType15));
        st_edmaDescType0  *pst_desc_3 = (st_edmaDescType0  *)((uintptr_t)edma_desc+(uintptr_t)sizeof(st_edmaDescType15)+(uintptr_t)sizeof(st_edmaDescType15));

        EDMA_LOG_DEBUG("--- Merge fillDesc - 1st ---\n");
        fillDescMergeNIn(desc_info, pst_desc_1, pi_errorCode);

        EDMA_LOG_DEBUG("--- Merge fillDesc - 2nd ---\n");
        fillDescMergeN(desc_info, pst_desc_2, pi_errorCode);

        EDMA_LOG_DEBUG("--- Merge fillDesc - 3rd ---\n");
        fillDescMergeNOut(desc_info, pst_desc_3, pi_errorCode);
        break;
    }
    case EDMA_DESC_INFO_UNPACK:
    {
        st_edmaDescInfoUnpack *pst_desc_info = &shape->st_info.infoUnpack;
        st_edmaDescType0 *pst_desc = (st_edmaDescType0 *)edma_desc;

        fillDescUnpack(pst_desc_info, pst_desc, pi_errorCode);
        break;
    }
    case EDMA_DESC_INFO_SWAP2:
    {
        st_edmaDescInfoSwap2 *pst_desc_info = &shape->st_info.infoSwap2;
        st_edmaDescType15 *pst_desc = (st_edmaDescType15 *)edma_desc;

        fillDescSwap2(pst_desc_info, pst_desc, pi_errorCode);
        break;
    }
    case EDMA_DESC_INFO_565TO888:
    {
        st_edmaDescInfo565To888 *pst_desc_info = &shape->st_info.info565To888;
        st_edmaDescType15 *pst_desc = (st_edmaDescType15 *)edma_desc;

        fillDesc565To888(pst_desc_info, pst_desc, pi_errorCode);
        break;
    }
    case EDMA_DESC_INFO_PACK:
    {
        st_edmaDescInfoPack *pst_desc_info = &shape->st_info.infoPack;
        st_edmaDescType15 *pst_desc = (st_edmaDescType15 *)edma_desc;

        fillDescPack(pst_desc_info, pst_desc, pi_errorCode);
        break;
    }
    default:
        EDMA_LOG_INFO("Type %d NOT support fill single Desc function", type_real);
        break;
    }

    EDMA_LOG_INFO("=== EdmaDesGMVPU done\n\n");
    return 0;
}

void EdmaDesGMVPU::transTaskInfo(st_edmaTaskInfo *oldInfo, st_edmaTaskInfo *newInfo)
{
    edma_MVPUtransTaskInfo(oldInfo, newInfo);
}
#endif //MVPU_EN

EdmaDescEngine::EdmaDescEngine(void)
{
    mDesGentor[EDMA_INFO_NN] = (EdmaDescGentor*) new EdmaDesGNN();
    mDesGentor[EDMA_INFO_GENERAL] = (EdmaDescGentor*) new EdmaDesGGeneral();
    mDesGentor[EDMA_INFO_DATA] = (EdmaDescGentor*) new EdmaDesGData();
    mDesGentor[EDMA_INFO_UFBC] = (EdmaDesGUFBC*) new EdmaDesGUFBC();
    mDesGentor[EDMA_INFO_SDK] = (EdmaDesGUFBC*) new EdmaDesGSDK();
    mDesGentor[EDMA_INFO_SLICE] = (EdmaDesGSlice*) new EdmaDesGSlice();
    mDesGentor[EDMA_INFO_PAD] = (EdmaDesGPad*) new EdmaDesGPad();
    mDesGentor[EDMA_INFO_BAYER_TO_RGGB] = (EdmaDescGentor*) new EdmaDesGBayerToRGGB();
    mDesGentor[EDMA_INFO_RGGB_TO_BAYER] = (EdmaDescGentor*) new EdmaDesGRGGBToBayer();

#ifdef MVPU_EN
    mDesGentor[EDMA_DESC_INFO_TYPE_0] = (EdmaDescGentor*) new EdmaDesGMVPU();
    mDesGentor[EDMA_DESC_INFO_TYPE_1] = (EdmaDescGentor*) new EdmaDesGMVPU();
    mDesGentor[EDMA_DESC_INFO_TYPE_5] = (EdmaDescGentor*) new EdmaDesGMVPU();
    mDesGentor[EDMA_DESC_INFO_ROTATE] = (EdmaDescGentor*) new EdmaDesGMVPU();
    mDesGentor[EDMA_DESC_INFO_SPLITN] = (EdmaDescGentor*) new EdmaDesGMVPU();
    mDesGentor[EDMA_DESC_INFO_MERGEN] = (EdmaDescGentor*) new EdmaDesGMVPU();
    mDesGentor[EDMA_DESC_INFO_UNPACK] = (EdmaDescGentor*) new EdmaDesGMVPU();
    mDesGentor[EDMA_DESC_INFO_SWAP2] = (EdmaDescGentor*) new EdmaDesGMVPU();
    mDesGentor[EDMA_DESC_INFO_565TO888] = (EdmaDescGentor*) new EdmaDesGMVPU();
    mDesGentor[EDMA_DESC_INFO_PACK] = (EdmaDescGentor*) new EdmaDesGMVPU();
#endif
    hwVer = 30; //default hw version = 30
    return;
}
void EdmaDescEngine::setHWver(unsigned char ver)
{
    hwVer = ver;
    if (hwVer == 20) {
        EDMA_LOG_INFO("replace v20 DesGentors\n");

        if (mDesGentor[EDMA_INFO_GENERAL] != NULL)
            delete mDesGentor[EDMA_INFO_GENERAL];
        if (mDesGentor[EDMA_INFO_DATA] != NULL)
            delete mDesGentor[EDMA_INFO_DATA];
        if (mDesGentor[EDMA_INFO_UFBC] != NULL)
            delete mDesGentor[EDMA_INFO_UFBC];

        mDesGentor[EDMA_INFO_GENERAL] = (EdmaDescGentor*) new EdmaDesGGeneralV20();
        mDesGentor[EDMA_INFO_DATA] = (EdmaDescGentor*) new EdmaDesGDataV20();
        mDesGentor[EDMA_INFO_UFBC] = (EdmaDesGUFBC*) new EdmaDesGUFBCV20();
    }
};

EdmaDescEngine::~EdmaDescEngine()
{
    map<EDMA_INFO_TYPE, EdmaDescGentor*>::iterator iter;

    int i = 0;

    for(iter = mDesGentor.begin(); iter != mDesGentor.end(); iter++)
    {
        delete (EdmaDescGentor *)iter->second;
        EDMA_LOG_INFO("first = %d, deleted second = 0x%p\r\n", iter->first, (void*)iter->second);
        i++;
    }


    EDMA_LOG_INFO("mDesGentor delete #%d, xx\n", i);

    mDesGentor.clear();
    return;
}


void EdmaDescEngine::queryTransDescSize(st_edmaTaskInfo *pst_old_edma_task_info,
                                   unsigned int *ui_new_desc_num,
                                   unsigned int *ui_new_desc_info_buff_size,
                                   unsigned int *ui_new_desc_buff_size)
{
    unsigned int ui_old_desc_info_size, ui_old_desc_idx;

    // check input emda task info
    if ((pst_old_edma_task_info==NULL) || (pst_old_edma_task_info->info_num==0) ||
        (pst_old_edma_task_info->edma_info==NULL))
    {
        *ui_new_desc_num = *ui_new_desc_info_buff_size = *ui_new_desc_buff_size = 0;
        return;
    }

    // count number of descriptors for new task info
    ui_old_desc_info_size = *ui_new_desc_info_buff_size = *ui_new_desc_buff_size = 0;
    *ui_new_desc_num = 0;

    EDMA_LOG_INFO("old info num = %d\n", pst_old_edma_task_info->info_num);

    for (ui_old_desc_idx=0; ui_old_desc_idx<pst_old_edma_task_info->info_num; ui_old_desc_idx++)
    {
        if (mDesGentor.find(pst_old_edma_task_info->edma_info[ui_old_desc_idx].info_type) !=  mDesGentor.end()) {
            st_edmaUnifyInfo *pDesc_info = (st_edmaUnifyInfo *) &pst_old_edma_task_info->edma_info[ui_old_desc_idx];
            int descNNum = mDesGentor[pst_old_edma_task_info->edma_info[ui_old_desc_idx].info_type]->queryDNum(pDesc_info);

            (*ui_new_desc_num)+=
                descNNum;
            *ui_new_desc_buff_size +=
                mDesGentor[pst_old_edma_task_info->edma_info[ui_old_desc_idx].info_type]->queryDSize(pDesc_info);
            EDMA_LOG_INFO("mDesGentor num = %d, new_desc_num = %d\r\n", descNNum, (*ui_new_desc_num));

        }else
            EDMA_LOG_ERR("unsupported descriptor type: %d\n", pst_old_edma_task_info->edma_info[ui_old_desc_idx].info_type);
    }
    EDMA_LOG_INFO("mDesGentor get new desc num = %d, size = %d\n", (*ui_new_desc_num), *ui_new_desc_buff_size);

}

void EdmaDescEngine::transTaskInfo(st_edmaTaskInfo *pst_old_edma_task_info, st_edmaTaskInfo *pst_new_edma_task_info)
{

    EDMA_INFO_TYPE type;
    // check input emda task info
    if ((pst_old_edma_task_info==NULL) || (pst_old_edma_task_info->info_num==0) ||
        (pst_old_edma_task_info->edma_info==NULL))
    {
        pst_new_edma_task_info->info_num = 0;
        pst_new_edma_task_info->edma_info = NULL;
        return;
    }
    type = pst_old_edma_task_info->edma_info[0].info_type;
    if (mDesGentor.find(type) !=  mDesGentor.end()) {
        mDesGentor[type]->transTaskInfo(pst_old_edma_task_info, pst_new_edma_task_info);
        return;
    }
    EDMA_LOG_ERR("transTaskInfo error!!\n");
}


int EdmaDescEngine::queryDescSize(st_edma_request_nn* request, st_edma_result_nn* result)
{
    st_edmaTaskInfo tInfo;
    unsigned int i;
    unsigned int desNum = 0;

    tInfo.info_num = request->info_num;
    tInfo.edma_info = (st_edmaUnifyInfo *)calloc(tInfo.info_num, sizeof(st_edmaUnifyInfo));

    if (tInfo.edma_info == NULL)
        return 1;

    for (i=0; i< request->info_num; i++ )
    {
        memcpy(&tInfo.edma_info[i].st_info.infoNN, &request->info_list[i], sizeof(st_edma_info_nn));
        tInfo.edma_info[i].info_type = EDMA_INFO_NN;
    }
    result->cmd_size = queryDescSize(&tInfo, &desNum);

    result->cmd_count = desNum;

    //mDesGentor[EDMA_INFO_NN]->queryDSize(NULL);

    free(tInfo.edma_info);
    return 0;

}

unsigned int EdmaDescEngine::queryDescSize(void *pst_in_edma_desc_info, unsigned int *pui_new_desc_num)
{
    unsigned int ui_new_desc_info_buff_size, ui_new_desc_buff_size, ui_new_desc_num;
    st_edmaTaskInfo st_old_edma_task_info;

    st_old_edma_task_info.info_num = 1;
    //st_old_edma_task_info.puc_desc_type = (unsigned char *)pst_in_edma_desc_info;
    st_old_edma_task_info.edma_info = (st_edmaUnifyInfo *)pst_in_edma_desc_info;

    queryTransDescSize(&st_old_edma_task_info, &ui_new_desc_num, &ui_new_desc_info_buff_size, &ui_new_desc_buff_size);

    if (pui_new_desc_num != NULL)
    {
        *pui_new_desc_num = ui_new_desc_num;
    }

    return ui_new_desc_buff_size;
}

unsigned int EdmaDescEngine::queryDescSize(st_edmaTaskInfo *pst_in_edma_task_info, unsigned int *pui_new_desc_num)
{
    unsigned int ui_new_desc_info_buff_size, ui_new_desc_buff_size, ui_new_desc_num;

    queryTransDescSize(pst_in_edma_task_info, &ui_new_desc_num, &ui_new_desc_info_buff_size, &ui_new_desc_buff_size);

    if (pui_new_desc_num != NULL)
    {
        *pui_new_desc_num = ui_new_desc_num;
    }

    return ui_new_desc_buff_size;
}

int EdmaDescEngine::fillDesc(st_edmaUnifyInfo *pst_in_edma_desc_info, void *pst_out_edma_desc)
{
    //unsigned char uc_desc_type = ((unsigned char *)pst_in_edma_desc_info)[0];

    EDMA_INFO_TYPE uc_desc_type = pst_in_edma_desc_info->info_type;

    EDMA_LOG_INFO("EdmaDescEngine::fillDesc type: %d\n", uc_desc_type);

    EDMA_LOG_INFO("EdmaDescEngine::fillDesc use mDesGentor\n");

    if (mDesGentor.find(uc_desc_type) ==  mDesGentor.end()) {
        EDMA_LOG_ERR("unsupported descriptor type: %d\n", uc_desc_type);
        return -1;
    } else
        mDesGentor[uc_desc_type]->fillDesc((st_edmaUnifyInfo *) pst_in_edma_desc_info, pst_out_edma_desc);
    return 0;
}

int EdmaDescEngine::fillDesc(st_edma_request_nn* request, unsigned char* codebuf)
{
    st_edmaTaskInfo tInfo;
    unsigned int i;

    tInfo.info_num = request->info_num;
    tInfo.edma_info = (st_edmaUnifyInfo *)calloc(tInfo.info_num, sizeof(st_edmaUnifyInfo));

	if (tInfo.edma_info != NULL) {
	    for (i=0; i< request->info_num; i++ )
	    {
	        memcpy(&tInfo.edma_info[i].st_info.infoNN, &request->info_list[i], sizeof(st_edma_info_nn));
	        tInfo.edma_info[i].info_type = EDMA_INFO_NN;
	    }

	    fillDesc(&tInfo, codebuf);

	    for (i=0; i< request->info_num; i++ )
	    {
	        memcpy(&request->info_list[i], &tInfo.edma_info[i].st_info.infoNN, sizeof(st_edma_info_nn));
	    }

	    free(tInfo.edma_info);
	} else {
		EDMA_LOG_ERR("fillDesc no enough memory !!");
	}
    return 0;

}

int EdmaDescEngine::fillDesc(st_edmaTaskInfo *pst_in_edma_task_info, void *pst_out_edma_desc)
{
    void *pst_desc;
    st_edmaUnifyInfo *pst_desc_info;
    unsigned int ui_desc_info_size, ui_desc_size, ui_desc_idx;

    st_edmaTaskInfo st_new_edma_task_info;
    st_new_edma_task_info.info_num = 0;
    st_new_edma_task_info.edma_info = NULL;
    //st_new_edma_task_info.pst_desc_info = NULL;

    // transform old edma task info to new edma task info
    transTaskInfo(pst_in_edma_task_info, &st_new_edma_task_info);

    //pst_desc_info = st_new_edma_task_info.edma_info;
    pst_desc = pst_out_edma_desc;
    ui_desc_info_size = ui_desc_size = 0;
    EDMA_LOG_INFO("ui_desc_num (info_num) = %d\r\n", st_new_edma_task_info.info_num);
    for (ui_desc_idx=0; ui_desc_idx<st_new_edma_task_info.info_num; ui_desc_idx++)
    {
        pst_desc_info = &st_new_edma_task_info.edma_info[ui_desc_idx];

        EDMA_LOG_INFO("fillDesc type %d\n", st_new_edma_task_info.edma_info[ui_desc_idx].info_type);
        if (mDesGentor.find(st_new_edma_task_info.edma_info[ui_desc_idx].info_type) !=  mDesGentor.end()) {
            mDesGentor[st_new_edma_task_info.edma_info[ui_desc_idx].info_type]->setHeadDesc(pst_out_edma_desc);  // due to support return addr-offset for nn request
            ui_desc_size = mDesGentor[st_new_edma_task_info.edma_info[ui_desc_idx].info_type]->queryDSize(pst_desc_info);
            EDMA_LOG_INFO("type = %d , ui_desc_size = %d\r\n",
                st_new_edma_task_info.edma_info[ui_desc_idx].info_type, ui_desc_size);
        }
        else {
            EDMA_LOG_ERR("unsupported descriptor type: %d\n", st_new_edma_task_info.edma_info[ui_desc_idx].info_type);
            return -1;
        }

        fillDesc(pst_desc_info, pst_desc);

        //pst_desc_info = (void *)((uintptr_t)pst_desc_info+(uintptr_t)ui_desc_info_size);
        pst_desc = (void *)((uintptr_t)pst_desc+(uintptr_t)ui_desc_size);
    }

    // release new edma task info
    if (pst_in_edma_task_info->edma_info != st_new_edma_task_info.edma_info) {

        if (st_new_edma_task_info.edma_info != NULL)
        {
            EDMA_LOG_INFO("need free edma_info\n");
            free(st_new_edma_task_info.edma_info);
            st_new_edma_task_info.edma_info = NULL;
        }
    } else
    {

        EDMA_LOG_INFO("no need free if old=new\n");
    }
    st_new_edma_task_info.info_num = 0;
    return 0;
}

#ifdef MVPU_EN
int EdmaDescEngine::fillDesc(st_edmaTaskInfo *pst_in_edma_task_info, void *pst_out_edma_desc, unsigned int* rp_offset)
{
    void *pst_desc;
    st_edmaUnifyInfo *pst_desc_info;
    unsigned int ui_desc_info_size, ui_desc_size, ui_desc_idx;
    EDMA_INFO_TYPE curr_type = EDMA_DESC_INFO_TYPE_0;
    unsigned int desc_offset = 0;
    unsigned int ui_desc_idx_old = 0;

    st_edmaTaskInfo st_new_edma_task_info;
    st_new_edma_task_info.info_num = 0;
    st_new_edma_task_info.edma_info = NULL;
    st_new_edma_task_info.is_binary_record = false;

    //st_new_edma_task_info.pst_desc_info = NULL;

    // transform old edma task info to new edma task info
    transTaskInfo(pst_in_edma_task_info, &st_new_edma_task_info);

    //pst_desc_info = st_new_edma_task_info.edma_info;
    pst_desc = pst_out_edma_desc;
    ui_desc_info_size = ui_desc_size = 0;
    EDMA_LOG_INFO("ui_desc_num (info_num) = %d\r\n", st_new_edma_task_info.info_num);

    for (ui_desc_idx=0; ui_desc_idx<st_new_edma_task_info.info_num; ui_desc_idx++)
    {
        curr_type = st_new_edma_task_info.edma_info[ui_desc_idx].info_type;
        pst_desc_info = &st_new_edma_task_info.edma_info[ui_desc_idx];

        EDMA_LOG_INFO("fillDesc type %d\n", curr_type);
        if (mDesGentor.find(curr_type) !=  mDesGentor.end()) {
            ui_desc_size = mDesGentor[curr_type]->queryDSize(pst_desc_info);
            EDMA_LOG_INFO("type = %d , ui_desc_size = %d\r\n",
                curr_type, ui_desc_size);

            if (pst_in_edma_task_info->is_binary_record) {
                mDesGentor[curr_type]->fillRPOffset(pst_in_edma_task_info, &ui_desc_idx_old, curr_type, rp_offset, desc_offset);
                desc_offset += ui_desc_size;
            }
        }
        else {
            EDMA_LOG_ERR("unsupported descriptor type: %d\n", curr_type);
            return -1;
        }

        fillDesc(pst_desc_info, pst_desc);

        //pst_desc_info = (void *)((uintptr_t)pst_desc_info+(uintptr_t)ui_desc_info_size);

        pst_desc = (void *)((uintptr_t)pst_desc+(uintptr_t)ui_desc_size);
    }

    if (pst_in_edma_task_info->is_binary_record) {
        EDMA_LOG_INFO("=== print rp_offset ===\n");
        EDMA_LOG_INFO("ui_desc_idx_old %d, desc_offset 0x%08x\n", ui_desc_idx_old, desc_offset);
        for (ui_desc_idx=0; ui_desc_idx<ui_desc_idx_old; ui_desc_idx++)
        {
            EDMA_LOG_INFO("Fill rp_offset #[%d] to 0x%08x\n", ui_desc_idx, rp_offset[ui_desc_idx]);
        }
        EDMA_LOG_INFO("=== print rp_offset ===\n");
    }

    // release new edma task info
    if (pst_in_edma_task_info->edma_info != st_new_edma_task_info.edma_info) {

        if (st_new_edma_task_info.edma_info != NULL)
        {
            EDMA_LOG_INFO("need free edma_info\n");
            free(st_new_edma_task_info.edma_info);
            st_new_edma_task_info.edma_info = NULL;
        }
    } else
    {

        EDMA_LOG_INFO("no need free if old=new\n");
    }
    st_new_edma_task_info.info_num = 0;
    return 0;
}

unsigned int EdmaDescEngine::queryReplaceNum(st_edmaTaskInfo *pst_old_edma_task_info)
{
    unsigned int ui_old_desc_idx = 0;
    unsigned int ui_rp_num = 0;

    for (ui_old_desc_idx=0; ui_old_desc_idx<pst_old_edma_task_info->info_num; ui_old_desc_idx++)
    {
        if (mDesGentor.find(pst_old_edma_task_info->edma_info[ui_old_desc_idx].info_type) !=  mDesGentor.end()) {
            st_edmaUnifyInfo *pDesc_info = (st_edmaUnifyInfo *) &pst_old_edma_task_info->edma_info[ui_old_desc_idx];
            int rpnum = mDesGentor[pst_old_edma_task_info->edma_info[ui_old_desc_idx].info_type]->queryRPNum(pDesc_info);

            ui_rp_num += rpnum;

        } else {
            EDMA_LOG_ERR("unsupported descriptor type: %d\n", pst_old_edma_task_info->edma_info[ui_old_desc_idx].info_type);
        }
    }
    EDMA_LOG_INFO("mDesGentor need to replaced num = %d\n", ui_rp_num);

    return ui_rp_num;
}
#endif

