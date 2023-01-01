/**
 * Mediatek APUSys API
 * ---
 * Mediatek APUSys provide query, command and memory interface
 * for setup APUSys execution environment.
 *
 * APUSys execution hierarchy:
 *   Session -> Cmd -> Subcmd
 *
 * APUSys memory related lifecycle:
 * 1. Command buffer: belong to cmd which will be duplicated in lower level software
 * 2. Memory(Main memory): belong to session, allocated from session can be shared different cmd/subcmd but separated with other session
 * 3. Virtual local memory(VLM): belong to cmd, can be shared between subcmds by user setting
 * 4. System level buffer(SLB): belong to session, allocated with session and extend local memory size for execution
 */

#ifndef __MTK_APUSYS_H__
#define __MTK_APUSYS_H__

#ifdef __cplusplus
extern "C" {
#endif

enum apusys_device_type {
    APUSYS_DEVICE_NONE, //No use.
    APUSYS_DEVICE_SAMPLE, //Dummy driver for unit test.
    APUSYS_DEVICE_MDLA,
    APUSYS_DEVICE_VPU,
    APUSYS_DEVICE_EDMA,
    APUSYS_DEVICE_EDMA_LITE,
    APUSYS_DEVICE_MVPU,
    APUSYS_DEVICE_UP, //uP backend for pre-defined control.
};

enum apusys_session_info {
    APUSYS_SESSION_INFO_VERSION,
    APUSYS_SESSION_INFO_METADATA_SIZE,
};

enum apusys_cb_direction {
    APUSYS_CB_BIDIRECTIONAL, //cmdbuf copy in before execution and copy out after exection.
    APUSYS_CB_IN, //cmdbuf copy in before execution.
    APUSYS_CB_OUT, //cmdbuf copy out after execution.
};

enum apusys_mem_type {
    APUSYS_MEM_TYPE_DRAM = 0, //Main memory.
    APUSYS_MEM_TYPE_VLM = 1, //Virtual local memory, not exclusive allocation and maybe located in faster memory or main memory by current environment.
    APUSYS_MEM_TYPE_LOCAL = 2, //APU local memory, will be occupied for session scope
    APUSYS_MEM_TYPE_SYSTEM = 3, //System share memory, will be occupied for session scope
    APUSYS_MEM_TYPE_SYSTEM_ISP = 4, //System share memory, will be occupied for session scope
    APUSYS_MEM_TYPE_SYSTEM_APU = 5, //System share memory, will be occupied for session scope
};

/**
 * APUSys memory flags, *bit definition and set by bitmask
 */
enum apusys_mem_flag {
    APUSYS_MEM_CACHEABLE, //Enable cpu cacheable if this bit set
    APUSYS_MEM_32BIT, //32-bit memory, default
    APUSYS_MEM_HIGHADDR, //High addr memory on current platform
};
#define F_APUSYS_MEM_CACHEABLE (1ULL << APUSYS_MEM_CACHEABLE)
#define F_APUSYS_MEM_32BIT (1ULL << APUSYS_MEM_32BIT)
#define F_APUSYS_MEM_HIGHADDR (1ULL << APUSYS_MEM_HIGHADDR)

enum apusys_mem_info {
    APUSYS_MEM_INFO_GET_SIZE = 0, //Get buffer's size, return total size if buffer allocated with size 0.
    APUSYS_MEM_INFO_GET_DEVICE_VA = 1, //Get device virtual address of memory.
    APUSYS_MEM_INFO_GET_HANDLE = 2,
};

/**
 * APUSys cmd parameter enumeration.
 * User setup cmd paramter by apusysCmd_setParam() with these definition.
 */
enum apusys_cmd_param_type {
    CMD_PARAM_PRIORITY, //[0~31] Cmd priority, only valid without softlimit setting.
    CMD_PARAM_HARDLIMIT, //Hard limit, cmd will be timeout if this cmd not completed with time expired.
    CMD_PARAM_SOFTLIMIT, //Soft limit, cmd execution time user expected and scheduler will refers to.
    CMD_PARAM_USRID, //Process pid set by user.
    CMD_PARAM_POWERSAVE, //Allow scheduler shut off power between cmd execution without any delay timer.
    CMD_PARAM_POWERPOLICY, //support different power policy
    CMD_PARAM_DELAY_POWER_OFF_MS, //Set delay power off time of a subcmd
    CMD_PARAM_APPTYPE, //application type hint
};

/**
 * APUSys command priority.
 * User setup cmd priority by apusysCmdSetParam() with CMD_PARAM_PRIORITY enum.
 * Lower number means higher priority.
 */
enum {
    APUSYS_PRIORITY_HIGH = 0, //Highest priority of cmd.
    APUSYS_PRIORITY_MEDIUM = 10,
    APUSYS_PRIORITY_LOW = 20,

    APUSYS_PRIORITY_MIN = 32, //Lowest number of cmd priority.
};

/**
 * APUSys command power policy.
 */
enum {
    APUSYS_POWERPOLICY_DEFAULT = 0, //do nothing
    APUSYS_POWERPOLICY_SUSTAINABLE = 1,
    APUSYS_POWERPOLICY_PERFORMANCE = 2, //set perf mode without considering cpu power consumption
    APUSYS_POWERPOLICY_POWERSAVING = 3,
};

/**
 * APUSys command application type hint
 */
enum {
    APUSYS_APPTYPE_DEFAULT = 0, //do nothing
    APUSYS_APPTYPE_ONESHOT = 1, //TODO, shorter delay power off time
    APUSYS_APPTYPE_STREAMING = 2, //TODO, longer delay power off time
};

#define DEFAULT_CMD_PARAM_PRIORITY (APUSYS_PRIORITY_LOW)
#define DEFAULT_CMD_PARAM_HARDLIMIT (0)
#define DEFAULT_CMD_PARAM_SOFTLIMIT (0)
#define DEFAULT_CMD_PARAM_USRID (0)
#define DEFAULT_CMD_PARAM_POWERSAVE (0)
#define DEFAULT_CMD_PARAM_POWERPOLICY (APUSYS_POWERPOLICY_DEFAULT)
#define DEFAULT_CMD_PARAM_DELAY_POWER_OFF_TIME (0)
#define DEFAULT_CMD_PARAM_APPTYPE (APUSYS_APPTYPE_DEFAULT)

/**
 * APUSys cmd execution information.
 * Cmd exeuction information user can get by apusysCmd_getRunInfo() after inference done.
 */
enum apusys_cmd_runinfo_type {
    CMD_RUNINFO_STATUS, //ref to enum apusys_cmd_errno
    CMD_RUNINFO_TOTALTIME_US, //from cmd signal to cmd end(us)
};
enum apusys_cmd_errno {
    CMD_ERRNO_SUCCESS,
    CMD_ERRNO_HARDLIMIT_EXCEED, //timeout caused by hardlimit exceed
    CMD_ERRNO_HW_TIMEOUT, //hw timeout
};

/**
 * APUSys subcmd parameter enumeration.
 * User setup subcmd paramter by apusysSubcmd_setParam() with these definition.
 */
enum apusys_subcmd_param_type {
    SC_PARAM_BOOST_VALUE, //[0~100] performance value, will be interpolated and mapping to current platform frequency.
    SC_PARAM_EXPECTED_MS, //Execution time user expected, usually get from previous cmd(ms).
    SC_PARAM_SUGGEST_MS, //Not used.
    SC_PARAM_VLM_USAGE, //Expected local memory size used in subcmd(byte).
    SC_PARAM_VLM_FORCE, //[0~1] Force subcmd allocate real local memory before execution, bubble maybe appear caused by wait local memory release from other subcmd.
    SC_PARAM_VLM_CTX, //[0~63] Virtual context for virtual memory, user can setup subcmd's local memory view for shared data in local memory.
    SC_PARAM_TURBO_BOOST, //[0~1] Use turbo opp, this will cause extra power
    SC_PARAM_MIN_BOOST, //[0~100] Min Boost value for power adjust
    SC_PARAM_MAX_BOOST, //[0~100] Max Boost value for power adjust
    SC_PARAM_HSE_ENABLE, //Enable Hardware semaphore
};
#define DEFAULT_SC_PARAM_BOOST_VALUE (100)
#define DEFAULT_SC_PARAM_TURBO_BOOST_VALUE (0)
#define DEFAULT_SC_PARAM_MIN_BOOST_VALUE (0)
#define DEFAULT_SC_PARAM_MAX_BOOST_VALUE (100)
#define DEFAULT_SC_PARAM_EXPECTED_MS (0)
#define DEFAULT_SC_PARAM_SUGGEST_MS (0)
#define DEFAULT_SC_PARAM_VLM_USAGE (0)
#define DEFAULT_SC_PARAM_VLM_FORCE (0)
#define DEFAULT_SC_PARAM_VLM_CTX (0)
#define DEFAULT_SC_PARAM_HSE_ENABLE (0)

enum hse_enable_type {
    APUSYS_HSE_DISABLE = 0,
    APUSYS_HSE_APU = 1,
    APUSYS_HSE_ISP = 2,
    APUSYS_HSE_DISP = 3,
};

/**
 * APUSys subcmd execution information.
 * Subcmd exeuction information user can get by apusysSubCmd_getRunInfo() after inference done.
 */
enum apusys_subcmd_runinfo_type {
    SC_RUNINFO_IPTIME, //Hardware execution time, return by ms.
    SC_RUNINFO_DRIVERTIME, //Driver time include ip time, return by ms.
    SC_RUNINFO_TIMESTAMP_START, //Timestamp at latest hardware trigger.
    SC_RUNINFO_TIMESTAMP_END, //Timestamp at latest hardware done.
    SC_RUNINFO_BANDWIDTH, //Bandwidth consumption of this subcmd.
    SC_RUNINFO_BOOST,
    SC_RUNINFO_VLMUSAGE,
};

/**
 * APUSys session instance.
 * Should be create before any other APUSys related operation.
 * One session means one execution context, and memory will be separated between different session.
 */
struct apusys_session;
typedef struct apusys_session apusys_session_t;

/**
 * APUSys command instance.
 * Execution unit and belong to one APUSys session.
 * One APUSys command may include multiple APUSys sub-commands and user can adjust.
 *   subcmds' execution dependency and order through cmd APIs.
 */
struct apusys_cmd;
typedef struct apusys_cmd apusys_cmd_t;

/**
 * APUSys sub-command instance.
 * Hardware driver execution unit and belong to one APUSys command.
 */
struct apusys_subcmd;
typedef struct apusys_subcmd apusys_subcmd_t;

/* Session functions */
/**
 * Create APUSys session instance for execution environment setting
 * Should be called for session before other operations
 *
 * arguments:
 *   none.
 * return value:
 *   apusys_session_t: session instance.
 */
apusys_session_t *apusysSession_createInstance(void);
typedef apusys_session_t *(*FnApusysSession_createInstance)(void);
/**
 * Delete APUSys session instance
 * Also free all session related resource include cmdbuf, mem, cmd, subcmd
 *
 * arguments:
 *   session: session user want to delete.
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysSession_deleteInstance(apusys_session_t *session);
typedef int (*FnApusysSession_deleteInstance)(apusys_session_t *session);
/**
 * Query information from APUSys session.
 *
 * arguments:
 *   session: APUSys session created by apusysSession_createInstance().
 *   type: target information type.
 * return value:
 *   value.
 */
uint64_t apusysSession_queryInfo(apusys_session_t *session, enum apusys_session_info type);
typedef uint64_t (*FnApusysSession_queryInfo)(apusys_session_t *session, enum apusys_session_info type);
/**
 * Query number of target device type support on current platform
 *
 * arguments:
 *   session: APUSys session created by apusysSession_createInstance().
 *   type: target device type.
 * return value:
 *   success: number of target device type support on current platform.
 *   fail: 0, means no target device type exist.
 */
uint64_t apusysSession_queryDeviceNum(apusys_session_t *session, enum apusys_device_type type);
typedef uint64_t (*FnApusysSession_queryDeviceNum)(apusys_session_t *session, enum apusys_device_type type);
/**
 * Query device meta data.
 *
 * arguments:
 *   session: APUSys session created by apusysSession_createInstance()
 *   type: target device type
 *   meta: buffer given by caller, APUSys fill meta data to this buffer.
 *     the buffer size should be equal or larger than meta data size get
 *     from apusysSession_queryInfo().
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysSession_queryDeviceMetaData(apusys_session_t *session, enum apusys_device_type type, void *meta);
typedef int (*FnApusysSession_queryDeviceMetaData)(apusys_session_t *session, enum apusys_device_type type, void *meta);
/**
 * Set device power.
 *
 * arguments:
 *   session: APUSys session created by apusysSession_createInstance()
 *   type: target device type
 *   idx: target device idx
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysSession_setDevicePower(apusys_session_t *session, enum apusys_device_type type, unsigned int idx, unsigned int boost);
typedef int (*FnApusysSession_setDevicePower)(apusys_session_t *session, enum apusys_device_type type, unsigned int idx, unsigned int boost);
/**
 * Send cmd to target device for query information.
 *
 * arguments:
 *   session: APUSys session created by apusysSession_createInstance()
 *   type: target device type
 *   cmdbuf: cmdbuf return by apusysSession_cmdBufAlloc()
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysSession_sendUserCmd(apusys_session_t *session, enum apusys_device_type type, void *cmdbuf);
typedef int (*FnApusysSession_sendUserCmd)(apusys_session_t *session, enum apusys_device_type type, void *cmdbuf);

/* Cmdbuf functions */
/**
 * Allocate command buffer for control flow.
 * Command buffer will be duplicated in low level software.
 *
 * arguments:
 *   session: APUSys session created by apusysSession_createInstance()
 *   size: size of command buffer.
 *   align: align of command buffer, 0 means no care.
 * return value:
 *   success: user pointer of command buffer.
 *   fail: nullptr.
 */
void *apusysSession_cmdBufAlloc(apusys_session_t *session, uint32_t size, uint32_t align);
typedef void *(*FnApusysSession_cmdBufAlloc)(apusys_session_t *session, uint32_t size, uint32_t align);
/**
 * Free command buffer.
 *
 * arguments:
 *   session: APUSys session created by apusysSession_createInstance()
 *   vaddr: user potiner get from apusysSession_cmdBufAlloc();
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysSession_cmdBufFree(apusys_session_t *session, void *vaddr);
typedef int (*FnApusysSession_cmdBufFree)(apusys_session_t *session, void *vaddr);

/* Memory functions */
/**
 * Allocate memory for data buffer
 * This buffer won't be duplicated in lower software, should be used for data not control
 *
 * arguments:
 *   session: APUSys session created by apusysSession_createInstance()
 *   size: size of command buffer.
 *   align: align of command buffer, 0 means no care.
 *   type: memory type.
 *     DRAM: main memory on platform.
 *     VLM: virtual local memory, this memory's location will be determined
 *       main memory or local faster memory in execution phase.
 *       return whole VLM instance if allocated with size 0.
 *     SLB: system level buffer, allocate share system level buffer to enlarge VLM size
 *   flags: memory parameter by bitmask, refer to enum apusys_mem_flag or APUSYS_MEM_XXX
 * return value:
 *   success: user pointer of command buffer.
 *   fail: nullptr.
 */
void *apusysSession_memAlloc(apusys_session_t *session, unsigned int size, unsigned int align,
    enum apusys_mem_type type, uint64_t flags);
typedef void *(*FnApusysSession_memAlloc)(apusys_session_t *session, unsigned int size, unsigned int align,
    enum apusys_mem_type type, uint64_t flags);
/**
 * Free memory.
 *
 * arguments:
 *   session: APUSys session created by apusysSession_createInstance()
 *   vaddr: user potiner get from apusysSession_memAlloc();
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysSession_memFree(apusys_session_t *session, void *vaddr);
typedef int (*FnApusysSession_memFree)(apusys_session_t *session, void *vaddr);
/**
 * Import memory as APUSys memory.
 * Memory's reference count maybe increased, user should guarentee import/unimport paired.
 *
 * arguments:
 *   session: APUSys session created by apusysSession_createInstance()
 *   handle: buffer handle
 *   size: buffer size
 * return value:
 *   success: user pointer of command buffer.
 *   fail: nullptr.
 */
void *apusysSession_memImport(apusys_session_t *session, int handle, unsigned int size);
typedef void *(*FnApusysSession_memImport)(apusys_session_t *session, int handle, unsigned int size);
/**
 * Unimport memory.
 * Reduce memory reference count
 *
 * arguments:
 *   session: APUSys session created by apusysSession_createInstance()
 *   vaddr: user potiner get from apusysSession_memImport();
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysSession_memUnImport(apusys_session_t *session, void *vaddr);
typedef int (*FnApusysSession_memUnImport)(apusys_session_t *session, void *vaddr);
/**
 * Flush memory.
 * User should flush memory for data buffer before execution to guarentee all data flush to DRAM.
 *
 * arguments:
 *   session: APUSys session created by apusysSession_createInstance()
 *   vaddr: user potiner get from apusysSession_memAlloc() or apusysSession_Import;
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysSession_memFlush(apusys_session_t *session, void *vaddr);
typedef int (*FnApusysSession_memFlush)(apusys_session_t *session, void *vaddr);
/**
 * Invalidate memory.
 * User should invalidate memory after execution to avoid get memory from CPU cache.
 *
 * arguments:
 *   session: APUSys session created by apusysSession_createInstance()
 *   vaddr: user potiner get from apusysSession_memAlloc() or apusysSession_Import;
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysSession_memInvalidate(apusys_session_t *session, void *vaddr);
typedef int (*FnApusysSession_memInvalidate)(apusys_session_t *session, void *vaddr);
/**
 * Get memory information.
 *
 * arguments:
 *   session: APUSys session created by apusysSession_createInstance()
 *   vaddr: user potiner get from apusysSession_memAlloc() or apusysSession_Import;
 *   type: information type
 * return value:
 *   value
 */
uint64_t apusysSession_memGetInfoFromHostPtr(apusys_session_t *session, void *vaddr, enum apusys_mem_info type);
typedef uint64_t (*FnApusysSession_memGetInfoFromHostPtr)(apusys_session_t *session, void *vaddr, enum apusys_mem_info type);
/**
 * Set memory parameter.
 *
 * arguments:
 *   session: APUSys session created by apusysSession_createInstance()
 *   vaddr: user potiner get from apusysSession_memAlloc() or apusysSession_Import;
 *   type: information type
 *   val: seting value
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysSession_memSetParamViaHostPtr(apusys_session_t *session, void *vaddr, enum apusys_mem_info type, uint64_t val);
typedef int (*FnApusysSession_memSetParamViaHostPtr)(apusys_session_t *session, void *vaddr, enum apusys_mem_info type, uint64_t val);

/* Cmd functions */
/**
 * Create APUSys command from one APUSys session.
 * APUSys command is exeuction unit in APUSys, usually as one model inference
 *
 * arguments:
 *   session: APUSys session created by apusysSession_createInstance()
 * return value:
 *   success: APUSys command instance.
 *   fail: nullptr.
 */
apusys_cmd_t *apusysSession_createCmd(apusys_session_t *session);
typedef apusys_cmd_t *(*FnApusysSession_createCmd)(apusys_session_t *session);
/**
 * Delete APUSys command from one APUSys session.
 *
 * arguments:
 *   session: APUSys session created by apusysSession_createInstance()
 *   cmd: APUSys command user want to delete
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysSession_deleteCmd(apusys_session_t *session, apusys_cmd_t *cmd);
typedef int (*FnApusysSession_deleteCmd)(apusys_session_t *session, apusys_cmd_t *cmd);
/**
 * Set APUsys command parameters.
 *
 * arguments:
 *   cmd: APUSys command user want to delete
 *   type: parameter type
 *   val: parmeter value
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysCmd_setParam(apusys_cmd_t *cmd, enum apusys_cmd_param_type type, uint64_t val);
typedef int (*FnApusysCmd_setParam)(apusys_cmd_t *cmd, enum apusys_cmd_param_type type, uint64_t val);

/**
 * Get APUsys command execution information of current inference.
 *
 * arguments:
 *   cmd: APUSys command created by apusysSession_createCmd().
 *   type: information type.
 * return value:
 *   value.
 */
uint64_t apusysCmd_getRunInfo(apusys_cmd_t *cmd, enum apusys_cmd_runinfo_type type);
typedef uint64_t (*FnApusysCmd_getRunInfo)(apusys_cmd_t *cmd, enum apusys_cmd_runinfo_type type);

/**
 * Set APUSys subcmds' dependency as edge in one APUSys command
 * Used to decript subcmd's execution order
 *
 * arguments:
 *   cmd: APUSys command created by apusysSession_createCmd().
 *   predecessor: subcmd should be executed before successor trigger.
 *   successor: subcmd should be pending until predecessor done.
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysCmd_setDependencyEdge(apusys_cmd_t *cmd, apusys_subcmd_t *predecessor, apusys_subcmd_t *successor);
typedef int (*FnApusysCmd_setDependencyEdge)(apusys_cmd_t *cmd, apusys_subcmd_t *predecessor, apusys_subcmd_t *successor);
int apusysCmd_setDependencyTightly(apusys_cmd_t *cmd, apusys_subcmd_t *predecessor, apusys_cmd_t *successor, void *mem);
typedef int (*FnApusysCmd_setDependencyTightly)(apusys_cmd_t *cmd, apusys_subcmd_t *predecessor, apusys_cmd_t *successor, void *mem);
/**
 * Set APUSys subcmds' dependency as pack in one APUSys command
 * Used to pack subcmd and low level software will trigger packed subcmds at the same time.
 *
 * arguments:
 *   cmd: APUSys command created by apusysSession_createCmd().
 *   main: subcmd used reference
 *   appendix: subcmd will be triggered with main subcmd.
 *     appendix subcmd shouldn't be set dependency before this API,
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysCmd_setDependencyPack(apusys_cmd_t *cmd, apusys_subcmd_t *main, apusys_subcmd_t *appendix);
typedef int (*FnApusysCmd_setDependencyPack)(apusys_cmd_t *cmd, apusys_subcmd_t *main, apusys_subcmd_t *appendix);

/**
 * Build APUSys command
 *
 * arguments:
 *   cmd: APUSys command created by apusysSession_createCmd().
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysCmd_build(apusys_cmd_t *cmd);
typedef int (*FnApusysCmd_build)(apusys_cmd_t *cmd);
/**
 * Run APUSys command with sync mode, return until cmd done
 *
 * arguments:
 *   cmd: APUSys command created by apusysSession_createCmd().
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysCmd_run(apusys_cmd_t *cmd);
typedef int (*FnApusysCmd_run)(apusys_cmd_t *cmd);
/**
 * Run APUSys command with async mode, return directly without cmd done.
 *   Should call apusysCmd_wait() for wait cmd done.
 *
 * arguments:
 *   cmd: APUSys command created by apusysSession_createCmd().
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysCmd_runAsync(apusys_cmd_t *cmd);
typedef int (*FnApusysCmd_runAsync)(apusys_cmd_t *cmd);
/**
 * Wait APUSys command which run with apusysCmd_runAsync()
 *
 * arguments:
 *   cmd: APUSys command created by apusysSession_createCmd().
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysCmd_wait(apusys_cmd_t *cmd);
typedef int (*FnApusysCmd_wait)(apusys_cmd_t *cmd);
/**
 * Run APUSys command with fence fd, and return APUSys' fence fd
 *   the cmd will be trigger at input fence signal.
 *   The fence fd return from this funciton will be signaled after cmd done,
 *   and User should poll return fd byself.
 *
 * arguments:
 *   cmd: APUSys command created by apusysSession_createCmd().
 *   fence: fence from other module, cmd will be trigger at this fence signal
 *   flags: no used.
 * return value:
 *   success: fence fd.
 *   fail: <= 0, linux errno.
 */
int apusysCmd_runFence(apusys_cmd_t *cmd, int fence, uint64_t flag);
typedef int (*FnApusysCmd_runFence)(apusys_cmd_t *cmd, int fence, uint64_t flag);

/* Subcmd functions */
/**
 * Create APUSys sub-command from one APUSys command.
 * APUSys sub-command is device execution unit, one command can include
 *   multiple sub-command.
 *
 * arguments:
 *   cmd: APUSys command created by apusysSession_createCmd().
 *   type: target device type.
 * return value:
 *   success: APUSys sub-command instance.
 *   fail: nullptr.
 */
apusys_subcmd_t *apusysCmd_createSubcmd(apusys_cmd_t *cmd, enum apusys_device_type type);
typedef apusys_subcmd_t *(*FnApusysCmd_createSubcmd)(apusys_cmd_t *cmd, enum apusys_device_type type);
/**
 * Set APUsys sub-command parameters.
 *
 * arguments:
 *   subcmd: APUSys sub-command created by apusysCmd_createSubcmd().
 *   type: parameter type.
 *   val: parmeter value.
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysSubCmd_setParam(apusys_subcmd_t *subcmd, enum apusys_subcmd_param_type type, uint64_t val);
typedef int (*FnApusysSubCmd_setParam)(apusys_subcmd_t *subcmd, enum apusys_subcmd_param_type type, uint64_t val);
/**
 * Add cmd buffer to APUSys subcmd.
 * User can add multiple cmdbufs to one subcmd with direction, low level software will
 *   duplicated by user hint
 *
 * arguments:
 *   subcmd: APUSys sub-command created by apusysCmd_createSubcmd().
 *   cmdbuf: cmdbuf return by apusysSession_cmdBufAlloc() and used to descript control cmd
 *   dir: cmdbuf direction, hint low level software how to duplicate this buffer
 *     APU_CB_BIDIRECTIONAL: cmdbuf copy in before execution and copy out after exection.
 *     APU_CB_IN: cmdbuf copy in before execution.
 *     APU_CB_OUT: cmdbuf copy out after execution.
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysSubCmd_addCmdBuf(apusys_subcmd_t *subcmd, void *cmdbuf, enum apusys_cb_direction dir);
typedef int (*FnApusysSubCmd_addCmdBuf)(apusys_subcmd_t *subcmd, void *cmdbuf, enum apusys_cb_direction dir);
/**
 * Get APUsys sub-command execution information of current inference.
 *
 * arguments:
 *   subcmd: APUSys sub-command created by apusysCmd_createSubcmd().
 *   type: information type.
 * return value:
 *   value.
 */
uint64_t apusysSubCmd_getRunInfo(apusys_subcmd_t *subcmd, enum apusys_subcmd_runinfo_type type);
typedef uint64_t (*FnApusysSubCmd_getRunInfo)(apusys_subcmd_t *subcmd, enum apusys_subcmd_runinfo_type type);

#ifdef __cplusplus
}
#endif

#endif
