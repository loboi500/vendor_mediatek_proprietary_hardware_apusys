package LibAPU

import (
    "strings"
    "android/soong/android"
    "android/soong/cc"
)

func libApuMdwDefaults(ctx android.LoadHookContext) {
    type props struct {
        Srcs []string
        Include_dirs []string
        Cflags []string
        Shared_libs []string
    }

    p := &props{}
    vars := ctx.Config().VendorConfig("mtkPlugin")

    // MTK_GENERIC_HAL is defined for layer decoupling 2.0 chips, no matter it uses split build 1.0 or 2.0 or full build
    layerDecouple2_0 := strings.ToLower(vars.String("MTK_GENERIC_HAL"))
    if layerDecouple2_0 == "yes" {
        // layer decoupling 2.0 chips will go to this branch
        // DO NOT USE platform macro in this branch
        // consider to create a "common" folder beside the platform folder
        // and implement single binary for all LD 2.0 chips in common folder
        p.Srcs = append(p.Srcs, "apusysSession.cpp")
        p.Srcs = append(p.Srcs, "apusysEntry.cpp")
        p.Srcs = append(p.Srcs, "apusysCmd.cpp")
        p.Srcs = append(p.Srcs, "apusysExecutor.cpp")
        p.Srcs = append(p.Srcs, "apusysUtil.cpp")
        p.Srcs = append(p.Srcs, "v1_0/apusysExecutorEmpty_v1.cpp")
        p.Srcs = append(p.Srcs, "v2_0/apusysExecutor_v2.cpp")
    } else {
        // layer decoupling 1.0 chips will go to this branch
        // platform macro is still supported here
        mtkPlatformDir := strings.ToLower(vars.String("TARGET_BOARD_PLATFORM"))
        if mtkPlatformDir == "mt6885" || mtkPlatformDir == "mt6873" || mtkPlatformDir == "mt6853" || mtkPlatformDir == "mt6893" || mtkPlatformDir == "mt6877" {

            p.Shared_libs = append(p.Shared_libs, "libion")
            p.Shared_libs = append(p.Shared_libs, "libion_mtk")

            p.Srcs = append(p.Srcs, "apusysSession.cpp")
            p.Srcs = append(p.Srcs, "apusysEntry.cpp")
            p.Srcs = append(p.Srcs, "apusysCmd.cpp")
            p.Srcs = append(p.Srcs, "apusysExecutor.cpp")
            p.Srcs = append(p.Srcs, "apusysUtil.cpp")
            p.Srcs = append(p.Srcs, "v1_0/apusysExecutor_v1.cpp")
            p.Srcs = append(p.Srcs, "v2_0/apusysExecutor_v2.cpp")
        }
    }
    ctx.AppendProperties(p)
}

func libApuMdwHeaderDefaults(ctx android.LoadHookContext) {
    type props struct {
        Export_include_dirs []string
    }

    p := &props{}

    headerPath := "./include"
    p.Export_include_dirs = append(p.Export_include_dirs, headerPath)

    ctx.AppendProperties(p)
}

func libApuMdwFactory() android.Module {
    module := cc.LibrarySharedFactory()
    android.AddLoadHook(module, libApuMdwDefaults)
    return module
}

func libApuMdwHeaderFactory() android.Module {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, libApuMdwHeaderDefaults)
    return module
}

func init() {
    android.RegisterModuleType("mtk_libapu_mdw_platform_headers", libApuMdwHeaderFactory)
    android.RegisterModuleType("apu_mdw_cc_library_shared", libApuMdwFactory)
}

