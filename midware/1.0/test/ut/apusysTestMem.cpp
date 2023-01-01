#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <random>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ion/ion.h> //system/core/libion/include/ion.h
#include <linux/ion_drv.h>
#include <ion.h> //libion_mtk/include/ion.h
#include <mt_iommu_port.h>
#include <sys/mman.h>
#include <../kernel-headers/linux/ion_4.19.h>  // system/memory/libion/include/

#include "apusys_drv.h"
#include "apusysTest.h"
#include "apusys.h"

#define CONSTRUCT_RUN_CMD_MAX 20
#define UT_MEM_MAX_SIZE 1*1024*1024

#ifdef MTK_APUSYS_IOMMU_LEGACY
#define APUSYS_IOMMU_PORT M4U_PORT_VPU
#else
#define APUSYS_IOMMU_PORT M4U_PORT_L21_APU_FAKE_DATA
#endif

#define PAGE_ALIGN (4096)
#define NONCACHED (0)


enum {
    UT_MEM_ACCESS,
    UT_MEM_IMPORT,
    UT_MEM_RANDOM_SIZE,
    UT_MEM_VLM_ALLOC,

    UT_MEM_MAX,
};


struct outside_memory
{
    unsigned long long va;
    int ionfd;
    int shareFd;
    int devFd;
    unsigned int iova;
    unsigned int iova_size;
    unsigned int size;
    ion_user_handle_t bufHandle = 0;
};

apusysTestMem::apusysTestMem()
{
    DEBUG_TAG;
}

apusysTestMem::~apusysTestMem()
{
    DEBUG_TAG;
}



bool legacy_ion_alloc(struct outside_memory *outMem)
{
    bool ret = true;
    struct ion_mm_data mmData;

    /* allocate ion fd */
    if (outMem->ionfd < 0)
    {
        LOG_ERR("create ion fd fail\n");
        return false;
    }


    /* alloc ion memory non-cached*/
    if(ion_alloc_mm(outMem->ionfd, outMem->size, PAGE_ALIGN, 0, &outMem->bufHandle))
    {
        LOG_ERR("fail to get ion buffer, (devFd=%d, buf_handle = %d, len=%d)\n", outMem->ionfd, outMem->bufHandle, outMem->size);
        return false;
    }
    /* set ion memory shared */
    if(ion_share(outMem->ionfd, outMem->bufHandle, &outMem->shareFd))
    {
        LOG_ERR("fail to get ion buffer handle (devFd=%d, shared_fd = %d, len=%d)\n", outMem->ionfd, outMem->shareFd, outMem->size);
        ret = false;
        goto free_mem;
    }

    /* map user va */
    outMem->va = (unsigned long long)ion_mmap(outMem->ionfd, NULL, (size_t)outMem->size, PROT_READ|PROT_WRITE, MAP_SHARED, outMem->shareFd, 0);
    if ((void *)outMem->va == MAP_FAILED)
    {
        LOG_ERR("get uva failed.\n");
        goto free_sharefd;
    }

    /* config buffer */
    memset(&mmData, 0, sizeof(mmData));
    mmData.mm_cmd = ION_MM_GET_IOVA;
    mmData.get_phys_param.handle      = outMem->bufHandle;
    mmData.get_phys_param.module_id   = APUSYS_IOMMU_PORT;
    mmData.get_phys_param.security    = 0;
    mmData.get_phys_param.coherent    = 1;
    if(ion_custom_ioctl(outMem->ionfd, ION_CMD_MULTIMEDIA, &mmData))
    {
        LOG_ERR("ion_config_buffer: ion_custom_ioctl ION_CMD_MULTIMEDIA Config Buffer failed\n");
        ret = false;
        goto free_mmap;
    }

    outMem->iova = (unsigned int)mmData.get_phys_param.phy_addr;
    LOG_DEBUG("\nsrcMem----\nsize: %08x\niova: %08x\n",
            outMem->size, outMem->iova);

    return ret;
free_mmap:
    if(ion_munmap(outMem->ionfd, (void*)outMem->va, outMem->size))
    {
        LOG_ERR("ion unmap fail\n");
    }
free_sharefd:
    ion_share_close(outMem->ionfd, outMem->shareFd);
free_mem:
    if(ion_free(outMem->ionfd, outMem->bufHandle))
    {
        LOG_ERR("ion free fail, fd = %d, len = %d\n", outMem->ionfd, outMem->size);
    }
    return ret;
}
bool legacy_ion_free(struct outside_memory *outMem)
{
    bool ret = true;

    if(ion_munmap(outMem->ionfd, (void*)outMem->va, outMem->size))
    {
        LOG_ERR("ion unmap fail\n");
    }

    ion_share_close(outMem->ionfd, outMem->shareFd);

    if(ion_free(outMem->ionfd, outMem->bufHandle))
    {
        LOG_ERR("ion free fail, fd = %d, len = %d\n", outMem->ionfd, outMem->size);
    }
    return ret;
}

bool aosp_ion_alloc(struct outside_memory *outMem)
{
    struct apusys_mem ctlmem;
    int shareFd = 0;
    int ret = 0;
    unsigned long long uva;
    struct ion_heap_data *heap_data;
    unsigned int heap_id;
    int i, heap_cnt;
    unsigned int flag = 0; // 0, or ION_FLAG_CACHED
    unsigned int size;
    int devFd;
    unsigned int align;

    align = PAGE_ALIGN;
    size = outMem->size;

    devFd = open("/dev/apusys", O_RDWR | O_SYNC);
    if (devFd < 0)
    {
        LOG_ERR("===========================================\n");
        LOG_ERR("| open apusys device node fail, errno(%3d)|\n", errno);
        LOG_ERR("===========================================\n");
        return false;
    }

    /* query the number of heaps ion driver can support */
    ret = ion_query_heap_cnt(outMem->ionfd, &heap_cnt);
    if (ret < 0) {
        LOG_ERR("ion_query_heap_cnt Failed %s\n", strerror(errno));
        goto err_malloc;
    }

    heap_data = (struct ion_heap_data *) std::malloc(heap_cnt * sizeof(*heap_data));
    if (heap_data == nullptr) {
        LOG_ERR("std::malloc Failed\n");
        goto err_malloc;
    }
    memset(heap_data, 0, heap_cnt * sizeof(*heap_data));
    ret = ion_query_get_heaps(outMem->ionfd, heap_cnt, heap_data);
    if (ret < 0) {
        LOG_ERR("ion_query_get_heaps Failed: %s\n", strerror(errno));
        goto err_query;
    }

    /* get heap id by the heap name */
    for (i = 0; i < heap_cnt; i++)
    {
        if (strcmp(heap_data[i].name, "ion_system_heap") == 0)
        {
            heap_id = heap_data[i].heap_id;
            break;
        }
    }

    if (i >= heap_cnt)
    {
        LOG_ERR("get_ion_heap_id Failed: %s\n", strerror(errno));
        goto err_query;
    }

    ret = ion_alloc_fd(outMem->ionfd, size, 0, 1 << heap_id, flag, &shareFd);
    if (ret < 0) {
        LOG_ERR("ION_IOC_ALLOC Failed: %s\n", strerror(errno));
        goto err_alloc;
    }

    /* mmap buffer */
    uva = (unsigned long long) mmap(NULL, size, PROT_READ|PROT_WRITE,
    MAP_SHARED, shareFd, 0);
    if ((void *) uva == MAP_FAILED) {
        LOG_ERR("ion mmap fail %s\n", strerror(errno));
        goto err_mmap;
    }

    memset(&ctlmem, 0, sizeof(struct apusys_mem));
    /* get iova from kernel */
    ctlmem.uva = uva;
    ctlmem.iova = 0;
    ctlmem.size = size;
    ctlmem.iova_size = 0;
    ctlmem.mem_type = APUSYS_MEM_DRAM_ION;
    ctlmem.align = align;
    ctlmem.fd = shareFd;

    ret = ioctl(devFd, APUSYS_IOCTL_MEM_MAP, &ctlmem);
    if (ret)
    {
        LOG_ERR("APUSYS_IOCTL_MEM_MAP: fail: %s (%d)\n",
            strerror(errno), errno);
        goto err_iovamap;
    }

    outMem->iova = ctlmem.iova;
    outMem->iova_size = ctlmem.iova_size;
    outMem->shareFd = shareFd;
    outMem->va = uva;
    outMem->devFd = devFd;

    std::free((void *)heap_data);

    return true;

err_iovamap:
    /* unmap the buffer properly in the end */
    if (munmap((void*)uva, size))
    {
        LOG_ERR("ion unmap fail\n");
    }
err_mmap:
    close(shareFd);
err_alloc:
err_query:
    std::free((void *)heap_data);
err_malloc:
    close(devFd);

    return false;

}
bool aosp_ion_free(struct outside_memory *outMem)
{
    int ret = 0;
    struct apusys_mem ctlmem;

    memset(&ctlmem, 0, sizeof(struct apusys_mem));
    /* get kva from kernel */

    ctlmem.khandle = 0;
    ctlmem.uva = outMem->va;
    ctlmem.size = outMem->size;
    ctlmem.iova = outMem->iova;
    ctlmem.iova_size = outMem->iova_size;
    ctlmem.align = 0;
    ctlmem.mem_type = APUSYS_MEM_DRAM_ION;
    ctlmem.fd = outMem->shareFd;

    ret = ioctl(outMem->devFd, APUSYS_IOCTL_MEM_UNMAP, &ctlmem);
    if(ret)
    {
        LOG_ERR("APUSYS_IOCTL_MEM_UNMAP: fail: %s (%d)\n",
            strerror(errno), errno);
        return false;
    }

    /* unmap the buffer properly in the end */
    if(munmap((void*)outMem->va, outMem->size))
    {
        LOG_ERR("ion unmap fail\n");
        return false;
    }
    /* close the buffer fd */
    if(close(outMem->shareFd))
    {
        LOG_ERR("ion close share fail\n");
        return false;
    }
    /* close the dev fd */
    if(close(outMem->devFd))
    {
        LOG_ERR("dev close fail\n");
        return false;
    }
    return true;
}

bool ut_access(apusysTestMem * caseInst, unsigned int loopNum)
{
    IApusysMem * testMem;
    char *data;
    int i = 0;
    bool ret = true;
    unsigned int index = 0;

    DEBUG_TAG;

    LOG_INFO("ut_access: loop(%u)\n", loopNum);

    for(index = 0; index < loopNum; index++)
    {
        testMem = caseInst->getEngine()->memAlloc(10);
        if (testMem == nullptr)
        {
            LOG_ERR("alloc memory fail\n");
            return false;
        }

        LOG_DEBUG("\ntestMem----\nsize: %08x\niova: %08x\nuva: %08llx\n",
                testMem->size, testMem->iova, testMem->va);

        data = (char*) testMem->va;
        for(i = 0; i < 10; i++)
        {
            data[i] = i;
        }

        caseInst->getEngine()->memSync(testMem);
        caseInst->getEngine()->memInvalidate(testMem);
        for(i = 0; i < 10; i++)
        {
            if(data[i] != i)
            {
                ret = false;
            }
        }

        if(ret == true)
        {
            LOG_DEBUG("R/W Check Pass\n");
        }
        else
        {
            LOG_DEBUG("R/W Check Fail\n");
        }

        if (caseInst->getEngine()->memFree(testMem) == false)
        {
            LOG_ERR("free memory fail\n");
            break;
        }
    }

    return ret;
}

bool ut_import(apusysTestMem * caseInst, unsigned int loopNum)
{
    IApusysMem *dstMem;
    bool ret = true;
    unsigned char *data;
    struct outside_memory outMem;
    bool legacy;
    int i = 0;

    DEBUG_TAG;

    LOG_INFO("ut_import: loop(%u)\n", loopNum);

    outMem.ionfd = open("/dev/ion", O_RDWR);
    if (outMem.ionfd < 0) {
        LOG_ERR("/dev/ion open fail: %s (%d)\n", strerror(errno), errno);
        return false;
    }
    legacy = ion_is_legacy(outMem.ionfd);
    outMem.size = 10;
    if (legacy)
        ret = legacy_ion_alloc(&outMem);
    else
        ret = aosp_ion_alloc(&outMem);

    if(!ret)
    {
        LOG_ERR("ion_alloc fail\n");
        goto free_fd;
    }

    data = (unsigned char*) outMem.va;
    for(i = 0; i < 10; i++)
    {
        data[i] = i;
    }

    dstMem = caseInst->getEngine()->memImport(outMem.shareFd, outMem.size);


    if (dstMem == nullptr) {
        ret = false;
        LOG_INFO("Import Failed: apusysEngine::memImport() returns null\n");
        goto mem_free;
    } else if (!dstMem->iova) {
        ret = false;
        LOG_INFO("Import Failed: dstMem->iova is NULL\n");
        caseInst->getEngine()->memUnImport(dstMem);
        goto mem_free;
    } else {
        LOG_INFO("Import Pass\n");
    }

    LOG_DEBUG("outMem----\n size: %08x iova: %08x uva: %llx\n",
            outMem.size, outMem.iova, outMem.va);
    LOG_DEBUG("dstMem----\n size: %08x iova: %08x uva: %llx\n",
            dstMem->size, dstMem->iova, dstMem->va);

    caseInst->getEngine()->memUnImport(dstMem);

    if (legacy)
        ret = legacy_ion_free(&outMem);
    else
        ret = aosp_ion_free(&outMem);

    close(outMem.ionfd);

    return ret;

mem_free:
    DEBUG_TAG;
    if (legacy)
        legacy_ion_free(&outMem);
    else
        aosp_ion_free(&outMem);

free_fd:
    close(outMem.ionfd);

    return ret;
}

static bool utRandomSizeAccess(apusysTestMem * caseInst, unsigned int loopNum)
{
    bool ret = true;
    int num = 0;
    unsigned int i = 0;
    std::random_device rd;
    std::default_random_engine gen = std::default_random_engine(rd());
    std::uniform_int_distribution<int> dis(1, UT_MEM_MAX_SIZE);
    IApusysMem * mem = nullptr;
    unsigned long long offset = 0;

    for (i = 0; i < loopNum; i++)
    {
        num = dis(gen);
        num = num - num%4;
        LOG_INFO("randomSizeAccess: loop(%d/%u)\n", i, loopNum);

        mem = caseInst->getEngine()->memAlloc(num);
        if (mem == nullptr)
        {
            return false;
        }
        LOG_INFO("clear mem(0x%llx/0x%x)(0x%x/0x%x/0x%x)\n", mem->va, mem->iova, num, mem->size, mem->iova_size);

        if (mem->size > mem->iova_size) {
            LOG_INFO("Allocate memory size(0x%x) mismatch with mapped iova size(0x%x)\n", mem->size, mem->iova_size);
            ret = false;
        }

        memset((void *)mem->va, 0, num);
        LOG_INFO("write pattern\n");
        memset((void *)mem->va, 0x5A, num);
        LOG_INFO("check pattern\n");
        while (offset < (unsigned long long)(num-4))
        {
            if (*(unsigned int *)(mem->va + offset) != 0x5A5A5A5A)
            {
                LOG_ERR("detect mem(0x%llx/0x%x) offset(0x%llx) pattern error(0x%x/0x5A5A5A5A)\n", mem->va, mem->iova, offset, *(unsigned int *)(mem->va + offset));
                ret = false;
            }
            offset+=4;
        }
        DEBUG_TAG;
        if(caseInst->getEngine()->memFree(mem) == false)
        {
            LOG_ERR("free mem(0x%llx/0x%x) fail\n", mem->va, mem->iova);
            ret = false;
        }

        if (ret == false)
            break;
    }

    return ret;
}

static bool utVlmAllocTest(apusysTestMem * caseInst, unsigned int loopNum)
{
    IApusysMem * mem = nullptr;
    unsigned int count = 0;

    for (count = 0; count < loopNum; count++)
    {
        mem = caseInst->getEngine()->memAlloc(0, 0, 0, APUSYS_USER_MEM_VLM);
        if (mem == nullptr)
        {
            LOG_ERR("allocate vlm mem fail\n");
            return false;
        }

        if (mem->type != APUSYS_USER_MEM_VLM || mem->va != 0 ||  mem->size == 0 || mem->iova == 0)
        {
            LOG_ERR("wrong vlm mem param(%d/0x%llx/0x%x/%u)\n", mem->type, mem->va, mem->iova, mem->size);
            goto runFalse;
        }
        LOG_INFO("vlm allocated iova(0x%x) size(%u)\n", mem->iova, mem->size);

        if (caseInst->getEngine()->memFree(mem) == false)
        {
            LOG_ERR("free vlm mem fail\n");
            return false;
        }
    }

    return true;

runFalse:
    if (caseInst->getEngine()->memFree(mem) == false)
    {
        LOG_ERR("free vlm mem fail 2\n");
    }
    return false;

}

bool apusysTestMem::runCase(unsigned int subCaseIdx, unsigned int loopNum, unsigned int threadNum)
{

    if(loopNum == 0)
    {
        LOG_ERR("case: cmdPath loop number(%d), do nothing\n", loopNum);
        return false;
    }

    LOG_DEBUG("case: caseMem (%d/%d/%d)\n", loopNum, subCaseIdx, threadNum);
    switch(subCaseIdx)
    {
        case UT_MEM_ACCESS:
            return ut_access(this, loopNum);
        case UT_MEM_IMPORT:
            return ut_import(this, loopNum);
        case UT_MEM_RANDOM_SIZE:
            return utRandomSizeAccess(this, loopNum);
        case UT_MEM_VLM_ALLOC:
            return utVlmAllocTest(this, loopNum);
        default:
            LOG_ERR("invalid mem test case (%d/%d)\n", subCaseIdx, UT_MEM_MAX);
    }

    return false;
}

void apusysTestMem::showUsage()
{
    LOG_CON("       <-c case>               [%d] memory test\n", APUSYS_UT_MEM);
    LOG_CON("           <-s subIdx>             [%d] memory access test\n", UT_MEM_ACCESS);
    LOG_CON("                                   [%d] memory import test\n", UT_MEM_IMPORT);
    LOG_CON("                                   [%d] random size access test\n", UT_MEM_RANDOM_SIZE);
    LOG_CON("                                   [%d] vlm alloc test\n", UT_MEM_VLM_ALLOC);
    return;
}

