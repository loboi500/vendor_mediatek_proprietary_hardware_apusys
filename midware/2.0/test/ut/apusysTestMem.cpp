#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <random>
#include <thread>
#include <unistd.h>
#include <fcntl.h>

#include <linux/ioctl.h>
#include <sys/mman.h>
#ifdef __ANDROID__
#include <BufferAllocator/BufferAllocatorWrapper.h>
#endif

#include "apusysTest.h"

#define MDW_UT_MEM_MAX (100*1024*1024) //100MB

enum {
    UT_MEM_ALLOC,
    UT_MEM_IMPORT,
    UT_MEM_HANDLE,
    UT_MEM_UPPERSIZE,
    UT_MEM_ADDRESSING,
    UT_MEM_APUMEM,
    UT_MEM_IMPORTDELAY,

    UT_MEM_MAX,
};

apusysTestMem::apusysTestMem()
{
    return;
}

apusysTestMem::~apusysTestMem()
{
    return;
}

static bool memAllocTest(apusysTestMem *inst, struct testCaseParam &param)
{
    apusys_session_t *session = nullptr;
    void *mem = nullptr, *cmdbuf = nullptr;
    unsigned int memPtn = 0x5a5a5a5a, size = 0, i = 0, per = 0;
    bool ret = false;
    uint64_t flags = 0;

    /* create session */
    session = inst->getSession();
    if (session == nullptr)
        goto out;

    // Use delay num to control mem_flags (enum apusys_mem_flag)
    flags = param.c.delayMs;

    for (i = 0; i < param.loopCnt; i++) {
        _SHOW_EXEC_PERCENT(i, param.loopCnt, per);

        /* allocate memory */
        if (param.c.fastMemSize)
            size = param.c.fastMemSize;
        else
            size = rand() % MDW_UT_MEM_MAX;

        //LOG_INFO("flags (%llx)\n", static_cast<unsigned long long>(flags));

        mem = apusysSession_memAlloc(session, size, 0, APUSYS_MEM_TYPE_DRAM, flags);
        if (mem == nullptr) {
            LOG_ERR("alloc memory(%u) fail\n", size);
            goto out;
        }

        /* write memory */
        memset(mem, memPtn, size);

        /* memory cache operation */
        if (apusysSession_memFlush(session, mem)) {
            LOG_ERR("flush memory(%u) fail\n", size);
            goto failCacheOp;
        }
        if (apusysSession_memInvalidate(session, mem)) {
            LOG_ERR("invalidate memory(%u) fail\n", size);
            goto failCacheOp;
        }

        /* allocate cmdbuf */
        size = rand() % MDW_UT_MEM_MAX;
        cmdbuf = apusysSession_cmdBufAlloc(session, size, 0);
        if (cmdbuf == nullptr) {
            LOG_ERR("alloc cmdbuf(%u) fail\n", size);
            goto failAllocCmdBuf;
        }

        /* write cmdbuf */
        memset(cmdbuf, memPtn, size);

        /* free cmdbuf */
        apusysSession_cmdBufFree(session, cmdbuf);
        /* free mem */
        apusysSession_memFree(session, mem);
    }

    ret = true;
    goto out;

failAllocCmdBuf:
failCacheOp:
    apusysSession_memFree(session, mem);
out:
    return ret;
}

static void memImportThreadRun(int fd, uint32_t cnt, int *ret)
{
    apusys_session_t *session = nullptr;
    void *apuVa = nullptr;
    uint64_t apuDeviceVa = 0;
    uint32_t i = 0;

    session = apusysSession_createInstance();
    if (session == nullptr) {
        LOG_ERR("create session fail\n");
        *ret = false;
        return;
    }

    for (i = 0; i < cnt; i++) {
        apuVa = nullptr;
        apuDeviceVa = 0;
        LOG_DEBUG("import test(%u/%u)\n", i, cnt);

        apuVa = apusysSession_memImport(session, fd, MDW_UT_MEM_MAX);
        if (apuVa == nullptr) {
            LOG_ERR("import mem to apu fail\n");
            *ret = false;
            break;
        }

        apuDeviceVa = apusysSession_memGetInfoFromHostPtr(session, apuVa, APUSYS_MEM_INFO_GET_DEVICE_VA);
        if (!apuDeviceVa) {
            LOG_ERR("get device va fail\n");
            *ret = false;
            break;
        }

        /* memory cache operation */
        if (apusysSession_memFlush(session, apuVa)) {
            LOG_ERR("flush memory(%d) fail\n", MDW_UT_MEM_MAX);
            *ret = false;
            break;
        }
        if (apusysSession_memInvalidate(session, apuVa)) {
            LOG_ERR("invalidate memory(%d) fail\n", MDW_UT_MEM_MAX);
            *ret = false;
            break;
        }

        if (apusysSession_memUnImport(session, apuVa)) {
            LOG_ERR("unimport fail\n");
            *ret = false;
            break;
        }
    }

    LOG_DEBUG("import test done, ret(%d)\n", *ret);

    apusysSession_deleteInstance(session);
}

static bool memImportTest(apusysTestMem *inst, struct testCaseParam &param)
{
#ifndef __ANDROID__
    UNUSED(inst);
    UNUSED(param);
    return false;
#else
    int shareFd = 0;
    apusys_session_t *session = nullptr;
    void *va = nullptr;
    BufferAllocator *allocator = nullptr;
    bool ret = false;
    std::vector<int> retVec;
    uint32_t thdCnt = 2, i = 0;
    std::vector<std::thread> threadVec;

    session = inst->getSession();

    allocator = CreateDmabufHeapBufferAllocator();
    if (allocator == nullptr) {
        LOG_ERR("create allocator fail\n");
        goto out;
    }

    /* android, alloc from dmabufheap lib */
    shareFd = DmabufHeapAlloc(allocator, "mtk_mm-uncached", MDW_UT_MEM_MAX, 0, 0);
    if (shareFd <= 0) {
        LOG_ERR("alloc dmabuf from dmabufheap fail\n");
        goto freeAllocator;
    }

    /* map user va */
    va = mmap(NULL, MDW_UT_MEM_MAX, PROT_READ|PROT_WRITE, MAP_SHARED, shareFd, 0);
    if (va == MAP_FAILED || va == nullptr) {
        LOG_ERR("get uva fail %s\n", strerror(errno));
        goto freeMem;
    }

    retVec.resize(thdCnt);
    threadVec.clear();
    for (i = 0; i < retVec.size(); i++)
        retVec.at(i) = true;

    for (i = 0; i < thdCnt; i++)
        threadVec.push_back(std::thread(memImportThreadRun, shareFd, param.loopCnt, &retVec[i]));

    for (i = 0; i < threadVec.size(); i ++)
        threadVec.at(i).join();

    for (i = 0; i < retVec.size(); i ++) {
        if (retVec.at(i) != true) {
            LOG_ERR("thread(%u) fail\n", i);
            ret = false;
            goto unMapMem;
        }
    }

    LOG_INFO("import test ok(%d/%p/%d)\n", shareFd, va, MDW_UT_MEM_MAX);

    ret = true;

unMapMem:
    munmap(va, MDW_UT_MEM_MAX);
freeMem:
    close(shareFd);
freeAllocator:
    FreeDmabufHeapBufferAllocator(allocator);
out:
    return ret;

#endif
}

static bool memHandleTest(apusysTestMem *inst, struct testCaseParam &param)
{
    apusys_session_t *session = nullptr;
    void *mem = nullptr, *cb= nullptr;
    unsigned int memPtn = 0x5a5a5a5a, size = 0;
    uint64_t val = 0;
    bool ret = false;
    UNUSED(param);

    size = 1*1024*1024; //1MB

    /* create session */
    session = inst->getSession();
    if (session == nullptr)
        goto out;

    /* mem related test */
    mem = apusysSession_memAlloc(session, size, 0, APUSYS_MEM_TYPE_DRAM, 0);
    if (mem == nullptr) {
        LOG_ERR("alloc mem fail\n");
        goto out;
    }
    memset(mem, memPtn, size);

    /* get device va */
    val = apusysSession_memGetInfoFromHostPtr(session, mem, APUSYS_MEM_INFO_GET_DEVICE_VA);
    if (!val) {
        LOG_ERR("get device va from mem fail\n");
        goto free_mem;
    }
    /* get size */
    val = apusysSession_memGetInfoFromHostPtr(session, mem, APUSYS_MEM_INFO_GET_SIZE);
    if (val != size) {
        LOG_ERR("get size from mem fail(%u/%llu)\n", size, static_cast<unsigned long long>(val));
        goto free_mem;
    }
    /* get hanlde */
    val = apusysSession_memGetInfoFromHostPtr(session, mem, APUSYS_MEM_INFO_GET_HANDLE);
    if (!val) {
        LOG_ERR("get handle from mem fail\n");
        goto free_mem;
    }

    /* cmdbuf related test */
    cb = apusysSession_cmdBufAlloc(session, size, 0);
    if (cb == nullptr) {
        LOG_ERR("alloc cb fail\n");
        goto free_mem;
    }

    /* get device va */
    val = apusysSession_memGetInfoFromHostPtr(session, cb, APUSYS_MEM_INFO_GET_DEVICE_VA);
    if (val) {
        LOG_ERR("get device va from cb ok, but error(0x%llx)\n", static_cast<unsigned long long>(val));
        goto free_cmdbuf;
    }
    /* get size */
    val = apusysSession_memGetInfoFromHostPtr(session, cb, APUSYS_MEM_INFO_GET_SIZE);
    if (val != size) {
        LOG_ERR("get size from cb fail(%u/%llu)\n", size, static_cast<unsigned long long>(val));
        goto free_cmdbuf;
    }
    /* get hanlde */
    val = apusysSession_memGetInfoFromHostPtr(session, cb, APUSYS_MEM_INFO_GET_HANDLE);
    if (!val) {
        LOG_ERR("get handle cb mem fail\n");
        goto free_cmdbuf;
    }

    ret = true;

free_cmdbuf:
    apusysSession_cmdBufFree(session, cb);
free_mem:
    apusysSession_memFree(session, mem);
out:
    return ret;
}

static bool memUpperSizeTest(apusysTestMem *inst, struct testCaseParam &param)
{
    apusys_session_t *session = nullptr;
    void *mem = nullptr;
    unsigned int memPtn = 0x5a5a5a5a, size = 0, sizeStep = 0;
    bool ret = true;
    uint64_t val = 0;

    if (param.c.fastMemSize)
        sizeStep = param.c.fastMemSize;
    else
        sizeStep = 100 * 1024 * 1024; //100MB

    size = sizeStep;

    /* create session */
    session = inst->getSession();
    if (session == nullptr)
        goto out;

    do {
        LOG_INFO("alloc mem size(0x%x)\n", size);
        mem = apusysSession_memAlloc(session, size, 0, APUSYS_MEM_TYPE_DRAM, 0);
        if (mem == nullptr) {
            LOG_ERR("alloc mem size(0x%x) fail\n", size);
            break;
        }
        /* write mem */
        memset(mem, memPtn, size);

        /* get device va */
        val = apusysSession_memGetInfoFromHostPtr(session, mem, APUSYS_MEM_INFO_GET_DEVICE_VA);
        if (!val) {
            LOG_ERR("get device va from mem size(0x%x) fail\n", size);
            break;
        }
        LOG_DEBUG("mem size(0x%x) device va(0x%llx)\n", size, static_cast<unsigned long long>(val));

        /* get size */
        val = apusysSession_memGetInfoFromHostPtr(session, mem, APUSYS_MEM_INFO_GET_SIZE);
        if (val != size) {
            LOG_ERR("get size from mem size(0x%x) fail(%llu)\n", size, static_cast<unsigned long long>(val));
            break;
        }
        LOG_DEBUG("mem size(0x%x) get size(0x%llx)\n", size, static_cast<unsigned long long>(val));

        /* get hanlde */
        val = apusysSession_memGetInfoFromHostPtr(session, mem, APUSYS_MEM_INFO_GET_HANDLE);
        if (!val) {
            LOG_ERR("get handle from mem size(0x%x) fail\n", size);
            break;
        }
        LOG_DEBUG("mem size(0x%x) handle(%llu)\n", size, static_cast<unsigned long long>(val));

        /* free memory */
        if (apusysSession_memFree(session, mem)) {
            LOG_ERR("free mem size(0x%x) fail\n", size);
            break;
        }

        size += sizeStep;
    } while (mem != nullptr);

    LOG_INFO("upper size = 0x%x\n", size - sizeStep);

    if (mem != nullptr)
        apusysSession_memFree(session, mem);
out:
    return ret;
}

static bool memAddressingTest(apusysTestMem *inst, struct testCaseParam &param)
{
    apusys_session_t *session = nullptr;
    void *defaultMem = nullptr, *highMem = nullptr;
    unsigned int memPtn = 0x5a5a5a5a, size = 0;
    uint64_t val = 0;
    bool ret = false;
    UNUSED(param);

    size = 1*1024*1024; //1MB

    /* create session */
    session = inst->getSession();
    if (session == nullptr)
        goto out;

    /* default mem addressing */
    defaultMem = apusysSession_memAlloc(session, size, 0, APUSYS_MEM_TYPE_DRAM, 0);
    if (defaultMem == nullptr) {
        LOG_ERR("alloc default mem fail\n");
        goto out;
    }
    memset(defaultMem, memPtn, size);

    /* get device va */
    val = apusysSession_memGetInfoFromHostPtr(session, defaultMem, APUSYS_MEM_INFO_GET_DEVICE_VA);
    if (!val) {
        LOG_ERR("get device va from default mem fail\n");
        goto freeDefaultMem;
    }
    if (val & 0xffffffff00000000)
        LOG_WARN("default mem over 32bit(0x%llx)\n", static_cast<unsigned long long>(val));

    /* highaddr mem addressing */
    highMem = apusysSession_memAlloc(session, size, 0, APUSYS_MEM_TYPE_DRAM, F_APUSYS_MEM_HIGHADDR);
    if (highMem == nullptr) {
        LOG_ERR("alloc highaddr mem fail\n");
        goto freeDefaultMem;
    }
    memset(highMem, memPtn, size);

    /* get device va */
    val = apusysSession_memGetInfoFromHostPtr(session, highMem, APUSYS_MEM_INFO_GET_DEVICE_VA);
    if (!val) {
        LOG_ERR("get device va from highaddr mem fail\n");
        goto freeDefaultMem;
    }
    if (!(val & 0xffffffff00000000))
        LOG_WARN("highaddr mem only 32bit(0x%llx)\n", static_cast<unsigned long long>(val));

    ret = true;

    apusysSession_memFree(session, highMem);

freeDefaultMem:
    apusysSession_memFree(session, defaultMem);
out:
    return ret;
}

static bool importApuMemTest(enum apusys_mem_type memType)
{
    apusys_session_t *sessionA = nullptr, *sessionB = nullptr;
    uint64_t sizeMem = 0, sessionA_memDva = 0, sessionA_memHandle = 0, sessionB_memDva = 0;
    void *memEmpty = nullptr, *sessionA_memPtr = nullptr, *sessionB_memPtr = nullptr, *sessionB2_memPtr = nullptr;
    bool ret = false;

    LOG_INFO("APU Test: %s, Mem Type: %d\n", __func__, memType);

    /* create session */
    sessionA = apusysSession_createInstance();
    if (sessionA == nullptr)
        goto out;

    sessionB = apusysSession_createInstance();
    if (sessionB == nullptr)
        goto out;

    /* session A: get size of mem */
    memEmpty = apusysSession_memAlloc(sessionA, 0, 0, memType, 0);
    if (memEmpty == nullptr) {
        LOG_ERR("A: alloc empty mem(%d) fail\n", memType);
        goto out;
    }
    sizeMem = apusysSession_memGetInfoFromHostPtr(sessionA, memEmpty, APUSYS_MEM_INFO_GET_SIZE);
    if (!sizeMem) {
        LOG_ERR("A: not support mem(%d), size 0\n", memType);
        goto out;
    }
    LOG_INFO("A: empty mem(%d) ptr(%p) size(%llu)\n",
        memType, memEmpty, static_cast<unsigned long long>(sizeMem));

    /* session A: alloc mem */
    sessionA_memPtr = apusysSession_memAlloc(sessionA, static_cast<unsigned int>(sizeMem), 0, memType, 0);
    if (sessionA_memPtr == nullptr) {
        LOG_ERR("A: alloc mem(%d/%u) fail\n", memType, static_cast<unsigned int>(sizeMem));
        goto out;
    }

    /* session A: get device va */
    sessionA_memDva = apusysSession_memGetInfoFromHostPtr(sessionA, sessionA_memPtr, APUSYS_MEM_INFO_GET_DEVICE_VA);
    if (!sessionA_memDva) {
        LOG_ERR("A: get mem(%d) va fail\n", memType);
        goto out;
    }

    /* session A: get handle */
    sessionA_memHandle = apusysSession_memGetInfoFromHostPtr(sessionA, sessionA_memPtr, APUSYS_MEM_INFO_GET_HANDLE);
    if (!sessionA_memHandle) {
        LOG_ERR("A: get mem(%d) handle fail\n", memType);
        goto out;
    }
    LOG_INFO("A: mem(%d) ptr(%p) size(%llu) device va(0x%llx) handle(%llu)\n",
        memType,
        sessionA_memPtr, static_cast<unsigned long long>(sizeMem),
        static_cast<unsigned long long>(sessionA_memDva),
        static_cast<unsigned long long>(sessionA_memHandle));

    /* session B: import mem */
    sessionB_memPtr = apusysSession_memImport(sessionB, sessionA_memHandle, sizeMem);
    if (sessionB_memPtr == nullptr) {
        LOG_ERR("B: import mem(%d) fail\n", memType);
        goto out;
    }
    sessionB2_memPtr = apusysSession_memImport(sessionB, sessionA_memHandle, sizeMem);
    if (sessionB2_memPtr == nullptr) {
        LOG_ERR("B: import mem(%d) twice fail\n", memType);
        goto out;
    }

    /* session B: get device va */
    sessionB_memDva = apusysSession_memGetInfoFromHostPtr(sessionB, sessionB_memPtr, APUSYS_MEM_INFO_GET_DEVICE_VA);
    if (!sessionB_memDva) {
        LOG_ERR("B: get mem(%d) device va fail\n", memType);
        goto out;
    }
    LOG_INFO("B: mem(%d) ptr(%p) size(%llu) device va(0x%llx)\n",
        memType,
        sessionB_memPtr, static_cast<unsigned long long>(sizeMem),
        static_cast<unsigned long long>(sessionB_memDva));

    if (sessionA_memDva != sessionB_memDva) {
        LOG_ERR("A/B(%d): device va not the same(0x%llx/0x%llx)\n",
            memType,
            static_cast<unsigned long long>(sessionA_memDva),
            static_cast<unsigned long long>(sessionB_memDva));
    } else {
        LOG_INFO("A/B(%d): same device va\n", memType);
        ret = true;
    }

out:
    /* unimport mem B */
    if (sessionB2_memPtr != nullptr) {
        apusysSession_memUnImport(sessionB, sessionB2_memPtr);
        sessionB2_memPtr = nullptr;
    }
    if (sessionB_memPtr != nullptr) {
        apusysSession_memUnImport(sessionB, sessionB_memPtr);
        sessionB_memPtr = nullptr;
    }

    /* free mem A */
    if (sessionA_memPtr != nullptr) {
        apusysSession_memFree(sessionA, sessionA_memPtr);
        sessionA_memPtr = nullptr;
    }

    /* free empty for size */
    if (memEmpty != nullptr) {
        apusysSession_memFree(sessionA, memEmpty);
        memEmpty = nullptr;
    }

    if (sessionB != nullptr) {
        apusysSession_deleteInstance(sessionB);
        sessionB = nullptr;
    }

    if (sessionA != nullptr) {
        apusysSession_deleteInstance(sessionA);
        sessionA = nullptr;
    }

    return ret;
}

static bool memApuMemTest(apusysTestMem *inst, struct testCaseParam &param)
{
    bool ret = true;
    UNUSED(inst);
    UNUSED(param);

    /* system mem */
    if (importApuMemTest(APUSYS_MEM_TYPE_SYSTEM) == true) {
        LOG_ERR("system memory test success but should be fail\n");
        ret = false;
    }

    /* vlm alloc */
    if (importApuMemTest(APUSYS_MEM_TYPE_VLM) == false) {
        LOG_WARN("vlm test fail\n");
        ret = false;
    }

    /* local mem */
    if (importApuMemTest(APUSYS_MEM_TYPE_LOCAL) == false) {
        LOG_WARN("local memory test fail\n");
        ret = false;
    }

    /* system-isp mem */
    if (importApuMemTest(APUSYS_MEM_TYPE_SYSTEM_ISP) == false) {
        LOG_WARN("system isp memory test fail\n");
        ret = false;
    }

    /* system-apu mem */
    if (importApuMemTest(APUSYS_MEM_TYPE_SYSTEM_APU) == false) {
        LOG_WARN("system apu memory test fail\n");
        ret = false;
    }

    return ret;
}

static bool memImportDelayTest(apusysTestMem *inst, struct testCaseParam &param)
{
#ifndef __ANDROID__
    UNUSED(inst);
    UNUSED(param);
    return false;
#else
    int shareFd = 0;
    UNUSED(param);
    apusys_session_t *session = nullptr;
    void *va = nullptr, *apuVa = nullptr;
    uint64_t apuDeviceVa = 0;
    BufferAllocator *allocator = nullptr;
    bool ret = false;

    session = inst->getSession();

    allocator = CreateDmabufHeapBufferAllocator();
    if (allocator == nullptr) {
        LOG_ERR("create allocator fail\n");
        goto out;
    }

    /* android, alloc from dmabufheap lib */
    shareFd = DmabufHeapAlloc(allocator, "mtk_mm-uncached", MDW_UT_MEM_MAX, 0, 0);
    if (shareFd <= 0) {
        LOG_ERR("alloc dmabuf from dmabufheap fail\n");
        goto freeAllocator;
    }

    /* map user va */
    va = mmap(NULL, MDW_UT_MEM_MAX, PROT_READ|PROT_WRITE, MAP_SHARED, shareFd, 0);
    if (va == MAP_FAILED || va == nullptr) {
        LOG_ERR("get uva fail %s\n", strerror(errno));
        goto freeMem;
    }

    apuVa = apusysSession_memImport(session, shareFd, MDW_UT_MEM_MAX);
    if (apuVa == nullptr) {
        LOG_ERR("import mem to apu fail\n");
        ret = false;
        goto unMapMem;
    }

    apuDeviceVa = apusysSession_memGetInfoFromHostPtr(session, apuVa, APUSYS_MEM_INFO_GET_DEVICE_VA);
    if (!apuDeviceVa) {
        LOG_ERR("get device va fail\n");
        ret = false;
        goto unMapMem;
    }

    LOG_INFO("delay 2s...\n");
    sleep(2);
    LOG_INFO("import test ok(%d/%p/%d)\n", shareFd, va, MDW_UT_MEM_MAX);

    ret = true;

unMapMem:
    munmap(va, MDW_UT_MEM_MAX);
freeMem:
    close(shareFd);
freeAllocator:
    FreeDmabufHeapBufferAllocator(allocator);
out:
    return ret;
#endif
}

static bool(*testFunc[UT_MEM_MAX])(apusysTestMem *inst, struct testCaseParam &param) = {
    memAllocTest,
    memImportTest,
    memHandleTest,
    memUpperSizeTest,
    memAddressingTest,
    memApuMemTest,
    memImportDelayTest,
};

bool apusysTestMem::runCase(unsigned int subCaseIdx, struct testCaseParam &param)
{
    if (subCaseIdx >= UT_MEM_MAX) {
        LOG_ERR("utMisc(%u) undefined\n", subCaseIdx);
        return false;
    }

    return testFunc[subCaseIdx](this, param);
}

void apusysTestMem::showUsage()
{
    LOG_CON("       <-c case>               [%d] mem test\n", APU_MDW_UT_MEM);
    LOG_CON("           <-s subIdx>             [%d] alloc test\n", UT_MEM_ALLOC);
    LOG_CON("           <-s subIdx>             [%d] import test\n", UT_MEM_IMPORT);
    LOG_CON("           <-s subIdx>             [%d] handle test\n", UT_MEM_HANDLE);
    LOG_CON("           <-s subIdx>             [%d] upper size limit test\n", UT_MEM_UPPERSIZE);
    LOG_CON("           <-s subIdx>             [%d] default addresssing test\n", UT_MEM_ADDRESSING);
    LOG_CON("           <-s subIdx>             [%d] apu mem test\n", UT_MEM_APUMEM);
    LOG_CON("           <-s subIdx>             [%d] apu mem import with delay test\n", UT_MEM_IMPORTDELAY);

    return;
}
