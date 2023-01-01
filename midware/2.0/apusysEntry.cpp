#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <errno.h>

#include "apusysCmn.h"
#include "apusysSession.h"
#include "apusys.h"

//----------------------------------------------
// command buffer functions
void *apusysSession_cmdBufAlloc(apusys_session_t *session, unsigned int size, unsigned int align)
{
    class apusysSession *e = reinterpret_cast<class apusysSession *>(session);

    return e->cmdBufAlloc(size, align);
}

int apusysSession_cmdBufFree(apusys_session_t *session, void *vaddr)
{
    class apusysSession *e = reinterpret_cast<class apusysSession *>(session);

    return e->cmdBufFree(vaddr);
}

//----------------------------------------------
// memory functions
void *apusysSession_memAlloc(apusys_session_t *session, unsigned int size, unsigned int align,
    enum apusys_mem_type type, uint64_t flags)
{
    class apusysSession *e = reinterpret_cast<class apusysSession *>(session);

    return e->memAlloc(size, align, type, flags);
}

int apusysSession_memFree(apusys_session_t *session, void *vaddr)
{
    class apusysSession *e = reinterpret_cast<class apusysSession *>(session);

    return e->memFree(vaddr);
}

void *apusysSession_memImport(apusys_session_t *session, int handle, unsigned int size)
{
    class apusysSession *e = reinterpret_cast<class apusysSession *>(session);

    return e->memImport(handle, size);
}

int apusysSession_memUnImport(apusys_session_t *session, void *vaddr)
{
    class apusysSession *e = reinterpret_cast<class apusysSession *>(session);

    return e->memUnImport(vaddr);
}

int apusysSession_memFlush(apusys_session_t *session, void *vaddr)
{
    class apusysSession *e = reinterpret_cast<class apusysSession *>(session);

    return e->memFlush(vaddr);
}

int apusysSession_memInvalidate(apusys_session_t *session, void *vaddr)
{
    class apusysSession *e = reinterpret_cast<class apusysSession *>(session);

    return e->memInvalidate(vaddr);
}

uint64_t apusysSession_memGetInfoFromHostPtr(apusys_session_t *session, void *vaddr, enum apusys_mem_info type)
{
    class apusysSession *e = reinterpret_cast<class apusysSession *>(session);

    return e->memGetInfoFromHostPtr(vaddr, type);
}

int apusysSession_memSetParamViaHostPtr(apusys_session_t *session, void *vaddr, enum apusys_mem_info type, uint64_t val)
{
    class apusysSession *e = reinterpret_cast<class apusysSession *>(session);

    return e->memSetParamViaHostPtr(vaddr, type, val);
}

//----------------------------------------------
// cmd functions
apusys_cmd_t *apusysSession_createCmd(apusys_session_t *session)
{
    class apusysSession *e = reinterpret_cast<class apusysSession *>(session);
    class apusysCmd *c = nullptr;

    c = e->createCmd();
    if (c == nullptr) {
        LOG_ERR("create cmd fail\n");
        return nullptr;
    }

    return reinterpret_cast<apusys_cmd_t *>(c);
}

int apusysSession_deleteCmd(apusys_session_t *session, apusys_cmd_t *cmd)
{
    class apusysSession *e = reinterpret_cast<class apusysSession *>(session);
    class apusysCmd *c = reinterpret_cast<class apusysCmd *>(cmd);

    return e->deleteCmd(c);
}

int apusysCmd_setParam(apusys_cmd_t *cmd, enum apusys_cmd_param_type type, uint64_t val)
{
    class apusysCmd *c = reinterpret_cast<class apusysCmd *>(cmd);

    return c->setParam(type, val);
}

uint64_t apusysCmd_getRunInfo(apusys_cmd_t *cmd, enum apusys_cmd_runinfo_type type)
{
    class apusysCmd *c = reinterpret_cast<class apusysCmd *>(cmd);

    return c->getRunInfo(type);
}

int apusysCmd_setDependencyEdge(apusys_cmd_t *cmd, apusys_subcmd_t *predecessor, apusys_subcmd_t *successor)
{
    class apusysCmd *c = reinterpret_cast<class apusysCmd *>(cmd);
    class apusysSubCmd *pdr = reinterpret_cast<class apusysSubCmd *>(predecessor);
    class apusysSubCmd *scr = reinterpret_cast<class apusysSubCmd *>(successor);

    return c->setDependencyEdge(pdr, scr);
}

int apusysCmd_setDependencyTightly(apusys_cmd_t *cmd, apusys_subcmd_t *predecessor, apusys_cmd_t *successor, void *mem)
{
    UNUSED(cmd);
    UNUSED(predecessor);
    UNUSED(successor);
    UNUSED(mem);

    LOG_ERR("not support\n");

    return -EINVAL;
}

int apusysCmd_setDependencyPack(apusys_cmd_t *cmd, apusys_subcmd_t *main, apusys_subcmd_t *appendix)
{
    class apusysCmd *c = reinterpret_cast<class apusysCmd *>(cmd);
    class apusysSubCmd *m = reinterpret_cast<class apusysSubCmd *>(main);
    class apusysSubCmd *adx = reinterpret_cast<class apusysSubCmd *>(appendix);

    return c->setDependencyPack(m, adx);
}

int apusysCmd_build(apusys_cmd_t *cmd)
{
    class apusysCmd *c = reinterpret_cast<class apusysCmd *>(cmd);

    return c->build();
}

int apusysCmd_run(apusys_cmd_t *cmd)
{
    class apusysCmd *c = reinterpret_cast<class apusysCmd *>(cmd);

    return c->run();
}

int apusysCmd_runAsync(apusys_cmd_t *cmd)
{
    class apusysCmd *c = reinterpret_cast<class apusysCmd *>(cmd);

    return c->runAsync();
}

int apusysCmd_wait(apusys_cmd_t *cmd)
{
    class apusysCmd *c = reinterpret_cast<class apusysCmd *>(cmd);

    return c->wait();
}

int apusysCmd_runFence(apusys_cmd_t *cmd, int fence, uint64_t flag)
{
    class apusysCmd *c = reinterpret_cast<class apusysCmd *>(cmd);

    return c->runFence(fence, flag);
}

//----------------------------------------------
// subcmd functions
apusys_subcmd_t *apusysCmd_createSubcmd(apusys_cmd_t *cmd, enum apusys_device_type type)
{
    class apusysCmd *c = reinterpret_cast<class apusysCmd *>(cmd);
    class apusysSubCmd *sc = nullptr;

    sc =  c->createSubCmd(type);
    if (sc == nullptr) {
        LOG_ERR("create subcmd fail\n");
        return nullptr;
    }

    return reinterpret_cast<apusys_subcmd_t *>(sc);
}

int apusysSubCmd_setParam(apusys_subcmd_t *subcmd, enum apusys_subcmd_param_type type, uint64_t val)
{
    class apusysSubCmd *sc = reinterpret_cast<class apusysSubCmd *>(subcmd);

    return sc->setParam(type, val);
}

int apusysSubCmd_addCmdBuf(apusys_subcmd_t *subcmd, void *cmdbuf, enum apusys_cb_direction dir)
{
    class apusysSubCmd *sc = reinterpret_cast<class apusysSubCmd *>(subcmd);

    return sc->addCmdBuf(cmdbuf, dir);
}

uint64_t apusysSubCmd_getRunInfo(apusys_subcmd_t *subcmd, enum apusys_subcmd_runinfo_type type)
{
    class apusysSubCmd *sc = reinterpret_cast<class apusysSubCmd *>(subcmd);

    return sc->getRunInfo(type);
}

//----------------------------------------------
// instance functions
apusys_session_t *apusysSession_createInstance(void)
{
    class apusysSession *e = nullptr;
    int fd = 0;

    /* open device node */
    fd = open("/dev/apusys", O_RDWR | O_SYNC);
    if (fd < 0)
    {
        LOG_ERR("==============================================\n");
        LOG_ERR("| open apusys device node fail, errno(%d/%s)|\n", errno, strerror(errno));
        LOG_ERR("==============================================\n");
        return nullptr;
    }

    e = new apusysSession(fd);

    return reinterpret_cast<apusys_session_t *>(e);
}

int apusysSession_deleteInstance(apusys_session_t *session)
{
    class apusysSession *e = nullptr;

    if (session == nullptr)
        return -EINVAL;

    e = reinterpret_cast<class apusysSession *>(session);

    delete e;
    return 0;
}

uint64_t apusysSession_queryDeviceNum(apusys_session_t *session, enum apusys_device_type type)
{
    class apusysSession *e = reinterpret_cast<class apusysSession *>(session);

    return e->queryDeviceNum(type);
}

uint64_t apusysSession_queryInfo(apusys_session_t *session, enum apusys_session_info type)
{
    class apusysSession *e = reinterpret_cast<class apusysSession *>(session);

    return e->queryInfo(type);
}

int apusysSession_queryDeviceMetaData(apusys_session_t *session, enum apusys_device_type type, void *meta)
{
    class apusysSession *e = reinterpret_cast<class apusysSession *>(session);

    return e->queryDeviceMetaData(type, meta);
}

int apusysSession_setDevicePower(apusys_session_t *session, enum apusys_device_type type, unsigned int idx, unsigned int boost)
{
    class apusysSession *e = reinterpret_cast<class apusysSession *>(session);

    return e->setDevicePower(type, idx, boost);
}

int apusysSession_sendUserCmd(apusys_session_t *session, enum apusys_device_type type, void *cmdbuf)
{
    class apusysSession *e = reinterpret_cast<class apusysSession *>(session);

    return e->sendUserCmd(type, cmdbuf);
}
