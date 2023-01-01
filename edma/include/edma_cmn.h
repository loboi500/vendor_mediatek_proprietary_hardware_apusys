#ifndef __APUSYS_EDMA_COMMON_H__
#define __APUSYS_EDMA_COMMON_H__

#ifdef __EDMA_KERNEL_DRV__
#include <linux/printk.h>
#else
#include <stdio.h>
#endif // __EDMA_KERNEL_DRV__

extern unsigned int gEdmaLogLv;

enum {
	EDMA_LOG_BITMAP_WARN,
	EDMA_LOG_BITMAP_INFO,
	EDMA_LOG_BITMAP_DEBUG,

	EDMA_LOG_BITMAP_MAX = 30,
	EDMA_LOG_BITMAP_NONE,
};

#define EDMA_LOG_PREFIX "[edma]"
#ifdef __ANDROID__
#define PROPERTY_DEBUG_EDMA_LEVEL "debug.edma.loglevel"
#else
#define PROPERTY_DEBUG_EDMA_LEVEL "DEBUG_EDMA_LEVEL"
#endif

#ifdef __EDMA_KERNEL_DRV__
#define EDMA_LOG_ERR(x, args...) pr_err(EDMA_LOG_PREFIX "[error] %s " x, __func__, ##args)
#define EDMA_LOG_WARN(x, args...) pr_warn(EDMA_LOG_PREFIX "[warn] " x, ##args)
#define EDMA_LOG_INFO(x, args...) pr_info(EDMA_LOG_PREFIX x, ##args)
#define EDMA_LOG_DEBUG(x, args...) pr_info(EDMA_LOG_PREFIX "[debug] %s " x, __func__, ##args)
#else

#define LOG_PRINT_HELPER(type, x, ...) printf(EDMA_LOG_PREFIX"[%s]%s: " x "%s", type, __func__, __VA_ARGS__)

#define EDMA_LOG_ERR(...) LOG_PRINT_HELPER("error", __VA_ARGS__ ,"")



#define EDMA_LOG_DEBUG_HELPER(mask, x, ...) \
    { \
        if (gEdmaLogLv & (1 << mask)) \
           printf(EDMA_LOG_PREFIX"[debug]%s/%d: " x "%s", __func__, __LINE__, __VA_ARGS__); \
    }

#define EDMA_LOG_DEBUG(...)  EDMA_LOG_DEBUG_HELPER(EDMA_LOG_BITMAP_DEBUG, __VA_ARGS__, "")

#define EDMA_LOG_WARN(...)  EDMA_LOG_DEBUG_HELPER(EDMA_LOG_BITMAP_WARN, __VA_ARGS__, "")
#define EDMA_LOG_INFO(...)  EDMA_LOG_DEBUG_HELPER(EDMA_LOG_BITMAP_INFO, __VA_ARGS__, "")

#define EDMA_LOG_NONE(...) EDMA_LOG_DEBUG_HELPER(EDMA_LOG_BITMAP_NONE, __VA_ARGS__, "")

#endif // __EDMA_KERNEL_DRV__

#define EDMA_DEBUG_TAG EDMA_LOG_DEBUG("[%d] \n", __LINE__)

#endif
