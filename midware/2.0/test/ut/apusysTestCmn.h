#pragma once

#define UNUSED(x) (void)(x)

#define LOG_PREFIX "[apusys_test]"

extern int gUtLogLevel;
#define LOG_PRINT_HELPER(type, x, ...) printf(LOG_PREFIX"[%s]%s: " x "%s", type, __func__, __VA_ARGS__)
#define LOG_ERR(...)      LOG_PRINT_HELPER("error", __VA_ARGS__ ,"")
#define LOG_WARN(...)     LOG_PRINT_HELPER("warn", __VA_ARGS__ ,"")
#define LOG_INFO(...)     LOG_PRINT_HELPER("info", __VA_ARGS__ ,"")

#define LOG_DEBUG_HELPER(x, ...) \
    { \
        if (gUtLogLevel) \
           printf(LOG_PREFIX"[debug]%s/%d: " x "%s", __func__, __LINE__ , __VA_ARGS__); \
    }
#define LOG_DEBUG(...)  LOG_DEBUG_HELPER(__VA_ARGS__, "")
#define DEBUG_TAG LOG_DEBUG("\n")
#define LOG_CON(...)  printf(__VA_ARGS__)
