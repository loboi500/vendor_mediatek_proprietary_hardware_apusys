/**
 * Mediatek APUSys batch API
 * ---
 * The utility APIs allow setting multiple APUSys cmds / APUSys sub-cmds parameters at once.
 */

#ifndef __MTK_APUSYS_BATCH_H__
#define __MTK_APUSYS_BATCH_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set multiple APUsys command parameters.
 * The number of params and vals should be the same as the number of APUSys commands.
 *
 * arguments:
 *   subCmds: APUSys commands user want to set parameters.
 *   numCmds: Number of APUSys commands
 *   params: Parameter types.
 *   vals: Parmeter values.
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysCmd_setParams(apusys_cmd_t* const* cmds, uint32_t numCmds,
                        enum apusys_cmd_param_type const* params,
                        uint64_t const* vals);
typedef int (*FnApusysCmd_setParams)(apusys_cmd_t* const* cmds, uint32_t numCmds,
                                     enum apusys_cmd_param_type const* types,
                                     uint64_t const* vals);

/**
 * Set multiple APUsys sub-command parameters.
 * The number of params and vals should be the same as the number of APUSys sub-commands.
 *
 * arguments:
 *   subCmds: APUSys sub-commands created by apusysCmd_createSubcmd().
 *   numSubCmds: Number of APUSys sub-commands
 *   params: Parameter types.
 *   vals: Parmeter values.
 * return value:
 *   success: 0.
 *   fail: linux errno.
 */
int apusysSubCmd_setParams(apusys_subcmd_t* const* subCmds, uint32_t numSubCmds,
                           enum apusys_subcmd_param_type const* params,
                           uint64_t const* vals);
typedef int (*FnApusysSubCmd_setParams)(apusys_subcmd_t* const* subCmds, uint32_t numCmds,
                                        enum apusys_subcmd_param_type const* types,
                                        uint64_t const* vals);

#ifdef __cplusplus
}
#endif

#endif // __MTK_APUSYS_BATCH_H__