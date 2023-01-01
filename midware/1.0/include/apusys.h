/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2019. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#pragma once

#include <cstdint>
#include <vector>
#include <string>

typedef enum {
    APUSYS_DEVICE_NONE,
    APUSYS_DEVICE_SAMPLE,

    APUSYS_DEVICE_MDLA,
    APUSYS_DEVICE_VPU,
    APUSYS_DEVICE_EDMA,
    APUSYS_DEVICE_WAIT,

    APUSYS_DEVICE_MAX,
}APUSYS_DEVICE_E;

typedef enum {
    APUSYS_USER_MEM_DRAM,
    APUSYS_USER_MEM_VLM,

    APUSYS_USER_MEM_MAX,
}APUSYS_USER_MEM_E;

/* enumuratio priority for apusys cmd */
typedef enum {
    APUSYS_PRIORITY_ULTRA = 0,
    APUSYS_PRIORITY_HIGH = 10,
    APUSYS_PRIORITY_NORMAL = 20,

    APUSYS_PRIORITY_MAX = 32,
}APUSYS_PRIORITY_E;

struct apusysSubCmdRunInfo {
    unsigned int bandwidth;  // (MB/s)execution bandwidth of indicated device
    unsigned int tcmUsage;  // (B)how many tcm this cmd usage
    unsigned int execTime;  // (us)execution time of driver turn around
    unsigned int ipTime;  // (us)hardware execution time
};

struct apusysSubCmdTuneParam {
    unsigned char boostVal; // (0~100) percent of opp, default 0xFF
    unsigned int estimateTime; // (us)expected turnaround time for this subcmd, default 0
    unsigned int suggestTime; // (ms)user suggest execution time for this subcmd, default 0
    unsigned int bandwidth; // (MB/s)user expected bandwidth for this subcmd, default 0
    bool tcmForce; // force this subcmd occupied fast memory to execute, default false
};

struct apusysCmdProp {
    unsigned char priority;  // priority, should refer to APUSYS_PRIORITY_E
    unsigned short softLimit;  // (ms)soft limit
    unsigned short hardLimit;  // (ms)hard limit, abort if command overtime
    bool powerSave;  // allow apusys upgrade opp, true->yes, false->no
};

class IApusysMem {
 public:
    virtual ~IApusysMem() {}
    virtual int getMemFd() = 0;

    unsigned long long va;
    unsigned int iova;
    unsigned int iova_size;

    unsigned int offset;
    unsigned int size;

    unsigned int align;
    unsigned long long khandle;

    int type;
};

class IApusysDev {
 public:
    virtual ~IApusysDev() {}
};

class IApusysFirmware {
 public:
    virtual ~IApusysFirmware() {}
};

class IApusysSubCmd {
 public:
     virtual ~IApusysSubCmd() {}

     /*
      * @getDeviceType:
      *  description:
      *      get device type of this subcmd which was set when IApusysCmd::addSubCmd() or IApusysCmd::packSubCmd()
      *  return:
      *      success: device type refer to APUSYS_DEVICE_E
      *      fail: -1
      */
     virtual int getDeviceType() = 0;

     /*
      * @getIdx:
      *  description:
      *      get this subcmd's idx at parent apusyscmd
      *  return:
      *      success: this subcmd's idx at parent apusyscmd
      *      fail: -1
      */
     virtual int getIdx() = 0;

     /*
      * @setCtxId:
      *  description:
      *      set memory ctx id, should be smaller than subcmd's idx
      *  arguement:
      *      <ctxId>: memory context id
      *  return:
      *      success: true
      *      fail: false
      */
     virtual bool setCtxId(unsigned int ctxId) = 0;

     /*
      * @getCtxId:
      *  description:
      *      get memory ctx id from this subcmd
      *  return:
      *      success: this subcmd's memory context id
      *      fail:0xFFFFFFFF
      */
     virtual unsigned int getCtxId() = 0;

     /*
      * @getRunInfo:
      *  description:
      *      get this subcmd runtime information, should be called after execution
      *  arguement:
      *      <info>: runtime information of this subcmd
      *         bandwidth:(MB/s)execution bandwidth of indicated device
      *         tcmUsage:(B)how many tcm this cmd usage
      *         execTime:(us)execution time of driver trun around
      *         ipTime:(us)execution time of hardware
      *  return:
      *      success: true
      *      fail: false
      */
     virtual bool getRunInfo(struct apusysSubCmdRunInfo& info) = 0;

     /*
      * @getTuneParam:
      *  description:
      *      get this subcmd tunning parameters
      *  return:
      *      refer to apusysSubCmdTunParam
      */
     virtual struct apusysSubCmdTuneParam& getTuneParam() = 0;

     /*
      * @setTuneParam:
      *  description:
      *      set tunning parameters for performance
      *  arguement:
      *      <tune>: refer to apusysSubCmdTunParam
      *  return:
      *      success: true
      *      fail: false
      */
     virtual bool setTuneParam(struct apusysSubCmdTuneParam& tune) = 0;
};

class IApusysCmd {
 public:
    virtual ~IApusysCmd() {}

    /* cmd functions */

    /*
     * @size:
     *  description:
     *      get number of subcmd in this apusys cmd
     *  return:
     *      number of subcmd in this apusys cmd
     */
    virtual unsigned int size() = 0;

    /*
     * @getCmdProperty: config apusys information
     *  description:
     *      get config parameters of IApusysCmd which may impact schduler working
     *  return:
     *      refer to apusysCmdProp
     *      <prop>: parameters user can config for IApusysCmd
     *          priority: priority
     *          vlmForce: force scheduler provide real vlm, if no vlm, blocking until vlm release and occuiped it (1/0)
     *          targetMs: deadline, command should be done in indicated time duration, if no, return false (0: no effect)
     *          estimateMs: time cost of this command on trail run (0: no effect)
     */
    virtual struct apusysCmdProp& getCmdProperty() = 0;

    /*
     * @setCmdProperty: config apusys information
     *  description:
     *      config parameters of IApusysCmd which may impact schduler working
     *  arguement:
     *      <prop>: parameters user can config for IApusysCmd
     *          priority: priority
     *          vlmForce: force scheduler provide real vlm, if no vlm, blocking until vlm release and occuiped it (1/0)
     *          targetMs: deadline, command should be done in indicated time duration, if no, return false (0: no effect)
     *          estimateMs: time cost of this command on trail run (0: no effect)
     *  return:
     *      success: true
     *      fail: false
     */
    virtual bool setCmdProperty(const struct apusysCmdProp &prop) = 0;

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
    virtual int addSubCmd(IApusysMem * subCmdBuf, APUSYS_DEVICE_E deviceType,
                          std::vector<int> & dependency) = 0;
    virtual int addSubCmd(IApusysMem * subCmdBuf, APUSYS_DEVICE_E deviceType,
                          std::vector<int> & dependency,
                          unsigned int ctxId) = 0;
    /*
     * @getSubCmd: get IApusysSubCmd by idx
     *  description:
     *      getIApusysSubCmd from this apusys cmd by idx
     *  arguement:
     *      <idx>: subcmd's idx user want to get
     *  return:
     *      success: IApusysSubCmd*
     *      fail: nullptr
     */
    virtual IApusysSubCmd * getSubCmd(int idx) = 0;

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
    virtual int packSubCmd(IApusysMem * subCmdBuf, APUSYS_DEVICE_E deviceType,
                           int subCmdIdx) = 0;

    /*
     * @constructCmd: construct apusys command format buffer
     *  description:
     *      construct apusys command format buffer according IApusysCmd detail information
     *      user call this function to avoid the construct overhead in first inference
     *  return:
     *      success: true
     *      fail: false
     */
    virtual bool constructCmd() = 0;
};

class IApusysEngine {
 public:
    virtual ~IApusysEngine() {}

    /* inst fnuctions */

    /*
     * @createInstance: apusys engine init function
     *  description:
     *      init apusys engine class for processing
     *      user should call this function to get IApusysEngine * first
     *  arguement:
     *      <userName>: name which user provided, can be used to identify user
     *  return:
     *      success: IApusysEngine * (libapusys entry)
     *      fail: nullptr
     */
    static IApusysEngine * createInstance(const char * userName);

    /*
     * @deleteInstance: apusys engine delete function
     *  description:
     *      delete apusys engine class which get by IApusysEngine::createInstance()
     *      shouldn't delete IApusysEngine * directly byself
     *  arguement:
     *      <engine>: apusys engine inst which init by IApusysEngine::createInstance()
     *  return:
     *      success: true
     *      fail: false
     */
    static bool deleteInstance(IApusysEngine * engine);

    /* cmd functions */

    /*
     * @initCmd: apusys cmd init function
     *  description:
     *      init apusys cmd class to user, and user can construct this cmd by class member functions (refer to IApusysCmd)
     *      all apusys cmd should be init by this function first
     *  return:
     *      success: IApusysCmd * (handle for user)
     *      fail: nullptr
     */
    virtual IApusysCmd * initCmd() = 0;

    /*
     * @destroyCmd: apusys cmd destroy function
     *  description:
     *      destroy apusys cmd which inited by IApusysEngine::initCmd() safely
     *      shouldn't delete IApusysCmd * directly byself
     *  arguement:
     *      <cmd>: the apusys cmd which get by IApusysEngine::initCmd()
     *  return:
     *      success: true
     *      fail: false
     */
    virtual bool destroyCmd(IApusysCmd * cmd) = 0;

    /*
     * @runCmd: run apusys cmd synchronous
     *  description:
     *      execute apusys cmd synchronous, blocking until cmd completed
     *  arguement:
     *      <cmd>: the apusys cmd which get by IApusysEngine::initCmd()
     *  return:
     *      success: true
     *      fail: false
     */
    virtual bool runCmd(IApusysCmd * cmd) = 0;

    /*
     * @runCmdAsync: run apusys cmd asynchronous
     *  description:
     *      execute apusys cmd asynchronous, no blocking and return immediately
     *      should call IApusysEngine::waitCmd() to check cmd completed
     *  arguement:
     *      <cmd>: the apusys cmd which get by IApusysEngine::initCmd()
     *  return:
     *      success: true
     *      fail: false
     */
    virtual bool runCmdAsync(IApusysCmd * cmd) = 0;

    /*
     * @waitCmd: wait indicate cmd which was executed by IApusysEngine::runCmdAsync()
     *  description:
     *      wait indicate cmd which was executed by IApusysEngine::runCmdAsync()
     *      block user thread until cmd completed
     *  arguement:
     *      <cmd>: the apusys cmd which get by IApusysEngine::initCmd()
     *  return:
     *      success: true
     *      fail: false
     */
    virtual bool waitCmd(IApusysCmd * cmd) = 0;

    /*
     * @runCmd: run apusys cmd synchronous
     *  description:
     *      execute apusys cmd synchronous with command buffer directly, blocking until cmd completed
     *      ***should be called by superuser who can construct apusys command format byself
     *  arguement:
     *      <cmdBuf>: memory buffer contain apusys command format
     *  return:
     *      success: true
     *      fail: false
     */
    virtual bool runCmd(IApusysMem * cmdBuf) = 0;

    /*
     * @runCmdAsync: run apusys cmd asynchronous
     *  description:
     *      execute apusys cmd asynchronous, no blocking and return immediately
     *      should call IApusysEngine::waitCmd() to check cmd completed
     *      ***should be called by superuser who can construct apusys command format byself
     *  arguement:
     *      <cmdBuf>: memory buffer contain apusys command format
     *  return:
     *      success: true
     *      fail: false
     */
    virtual bool runCmdAsync(IApusysMem * cmdBuf) = 0;

    /*
     * @waitCmd: wait indicate cmd which was executed by IApusysEngine::runCmdAsync()
     *  description:
     *      wait indicate cmd which was executed by IApusysEngine::runCmdAsync()
     *      block user thread until cmd completed
     *      ***should be called by superuser who can construct apusys command format byself
     *  arguement:
     *      <cmdBuf>: memory buffer contain apusys command format
     *  return:
     *      success: true
     *      fail: false
     */
    virtual bool waitCmd(IApusysMem * cmdBuf) = 0;

    /*
     * @checkCmdValid: check command buffer is valid with apusys command format or not
     *  description:
     *      check command buffer is valid for apusys command format
     *      used to check command version to avoid version mismatch
     *      ***should be called by superuser who can construct apusys command format byself
     *  arguement:
     *      <cmdBuf>: memory buffer contain apusys command format
     *  return:
     *      success: true
     *      fail: false
     */
    virtual bool checkCmdValid(IApusysMem * cmdBuf) = 0;

    /*
     * @sendUserCmdBuf:
     *  description:
     *      send user defined cmd by memory buffer format to indicated device
     *      driver should support this operations
     *  arguement:
     *      <cmdBuf>: command buffer
     *      <deviceType>: device type, refer to APUSYS_DEVICE_E
     *      <idx>: device idx
     *  return:
     *      success: true
     *      fail: false
     */
    virtual bool sendUserCmdBuf(IApusysMem * cmdBuf, APUSYS_DEVICE_E deviceType) = 0;
    virtual bool sendUserCmdBuf(IApusysMem * cmdBuf, APUSYS_DEVICE_E deviceType, int idx) = 0;

    /* memory functions*/

    /*
     * @memAlloc: memory allocate
     *  description:
     *      allocate dram memory with size, and va(va), kernel va(kva) and device va(iova) are mapped already
     *  arguement:
     *      <size>: the type of device user want to allocate
     *  return:
     *      success: IApusysMem * (handle for user)
     *      fail: nullptr
     */
    virtual IApusysMem * memAlloc(size_t size) = 0;

    /*
     * @memAlloc: memory allocate
     *  description:
     *      allocate dram memory with size and align, and va(va), kernel va(kva) and device va(iova) are mapped already
     *  arguement:
     *      <size>: the type of device user want to allocate
     *      <align>: memory alignment (should be factors of 4K)
     *  return:
     *      success: IApusysMem * (handle for user)
     *      fail: nullptr
     */
    virtual IApusysMem * memAlloc(size_t size, unsigned int align) = 0;

    /*
     * @memAlloc: memory allocate
     *  description:
     *      allocate memory with size, align and type, and va(va), kernel va(kva) and device va(iova) are mapped already
     *      if type = APUSYS_USER_MEM_VLM, always return start address(iova only, no va and kva)
     *  arguement:
     *      <size>: the type of device user want to allocate
     *      <align>: memory alignment (should be factors of 4K)
     *      <type>: type of memory
     *          APUSYS_USER_MEM_DRAM: dram
     *          APUSYS_USER_MEM_VLM: fast memory, if this type set, only return start address(iova) only to user
     *  return:
     *      success: IApusysMem * (handle for user)
     *      fail: nullptr
     */
    virtual IApusysMem * memAlloc(size_t size, unsigned int align,
                                  unsigned int cache) = 0;
    virtual IApusysMem * memAlloc(size_t size, unsigned int align,
                                  unsigned int cache,
                                  APUSYS_USER_MEM_E type) = 0;
    /*
     * @memFree: memory free
     *  description:
     *      free memory which allocated by IApusysEngine::memAlloc()
     *  arguement:
     *      <mem>: the handle return from IApusysEngine::memAlloc()
     *  return:
     *      success: true
     *      fail: false
     */
    virtual bool memFree(IApusysMem * mem) = 0;

    /*
     * @memSync: memory sync(flush)
     *  description:
     *      sync memory which allocated by IApusysEngine::memAlloc()
     *  arguement:
     *      <mem>: the handle return from IApusysEngine::memAlloc()
     *  return:
     *      success: true
     *      fail: false
     */
    virtual bool memSync(IApusysMem * mem) = 0;

    /*
     * @memInvalidate: memory invalidate
     *  description:
     *      invalidate memory which allocated by IApusysEngine::memAlloc()
     *  arguement:
     *      <mem>: the handle return from IApusysEngine::memAlloc()
     *  return:
     *      success: true
     *      fail: false
     */
    virtual bool memInvalidate(IApusysMem * mem) = 0;

    /*
     * @memImport: memory import
     *  description:
     *      import memory by ion fd, and get apusys memory handle
     *  arguement:
     *      <shareFd>: ion shared fd which allocate by ion function
     *      <size>: ion shared fd which allocate by ion function
     *  return:
     *      success: IApusysMem * (handle for user)
     *      fail: nullptr
     */
    virtual IApusysMem * memImport(int shareFd, unsigned int size) = 0;

    /*
     * @memUnImport: memory unimport
     *  description:
     *      unimport memory with IApusysMem
     *  arguement:
     *      <mem>: the handle return from IApusysEngine::memImport()
     *  return:
     *      success: true
     *      fail: false
     */
    virtual bool memUnImport(IApusysMem * mem) = 0;

    /* device functions */

    /*
     * @getDeviceNum: query # type device support
     *  description:
     *      query how many core the device type support this platform
     *  arguement:
     *      <deviceType>: the type of device user want to allocate
     *  return:
     *      success: > 0 (# type device exist this platform)
     *      fail: <=0 (no type device support)
     */
    virtual int getDeviceNum(APUSYS_DEVICE_E type) = 0;

    /*
     * @devAlloc: allocate indicate type device
     *  description:
     *      user can allocate type of device for specific using
     *      apusys doesn't schedule any cmd to this device until user process free or crash
     *  arguement:
     *      <deviceType>: the type of device user want to allocate
     *  return:
     *      success: IApusysDev * (handle for user)
     *      fail: nullptr
     */
    virtual IApusysDev * devAlloc(APUSYS_DEVICE_E deviceType) = 0;

    /*
     * @devFree: free indicate device
     *  description:
     *      free indicate device which allocated from IApusysEngine::devAlloc()
     *      user has responsibility for free device
     *  arguement:
     *      <idev>: device handle which return from IApusysEngine::devAlloc()
     *  return:
     *      success: true
     *      fail: false
     */
    virtual bool devFree(IApusysDev * idev) = 0;

    /* power functions */

    /*
     * @setPower: power on indicate device for hiding overhead
     *  description:
     *      power on device of indicate type, user can hiding power on latency byself
     *  arguement:
     *      <deviceType>: the type of device user want to allocate
     *      <idx>: device core index
     *      <boostVal>: linear value of opp step, max:100, min:0 [0,100]
     *          ex. boostVal=100, bootup device by its highest clk
     *  return:
     *    success: true
     *    fail: false
     */
    virtual bool setPower(APUSYS_DEVICE_E deviceType, unsigned int idx,
                          unsigned int boostVal) = 0;

    /* firmware functions */

    /*
     * @loadFirmware: load firmware to indicate device
     *  description:
     *      bypass one valid memory buffer which contain indicate deivce's firmware to kernel driver
     *  arguement:
     *      <deviceType>: the type of device user want to allocate
     *      <magic>: magic number, will be check in hardware driver
     *      <idx>: device core index
     *      <name>: firmware name
     *      <fwBuf>: IApusysMem type, one memory buffer user can copy firmware for passing
     *  return:
     *    success: IApusysFirmware * handle
     *    fail: 0
     */
    virtual IApusysFirmware * loadFirmware(APUSYS_DEVICE_E deviceType,
                                           unsigned int magic, unsigned int idx,
                                           std::string name, IApusysMem * fwBuf) = 0;

    /*
     * @unloadFirmware: unload firmware from indicate device
     *  description:
     *      send unload firmware command to indicate device kernel driver
     *  arguement:
     *      <hnd>:apusys firmware handle from loadFirmware
     *  return:
     *    success: true
     *    fail: false
     */
    virtual bool unloadFirmware(IApusysFirmware * hnd) = 0;
};

