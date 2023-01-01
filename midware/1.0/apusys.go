package LibAPUSYS

import (
    "strings"
    "android/soong/android"
    "android/soong/cc"
)

func libApusysDefaults(ctx android.LoadHookContext) {
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
        p.Srcs = append(p.Srcs, "apusysEmpty.cpp")

    } else {
        // layer decoupling 1.0 chips will go to this branch
        // platform macro is still supported here
        mtkPlatformDir := strings.ToLower(vars.String("TARGET_BOARD_PLATFORM"))
        if mtkPlatformDir == "mt6885" || mtkPlatformDir == "mt6873" || mtkPlatformDir == "mt6853" || mtkPlatformDir == "mt6893" || mtkPlatformDir == "mt6877" {

            p.Shared_libs = append(p.Shared_libs, "libion")
            p.Shared_libs = append(p.Shared_libs, "libion_mtk")

            p.Cflags = append(p.Cflags, "-DMTK_APSYS_ION_ENABLE")

            p.Srcs = append(p.Srcs, "apusysCmd.cpp")
            p.Srcs = append(p.Srcs, "apusysEngine.cpp")
            p.Srcs = append(p.Srcs, "vlmAllocator.cpp")
            p.Srcs = append(p.Srcs, "ionAllocator.cpp")
            p.Srcs = append(p.Srcs, "ionAospAllocator.cpp")
            p.Srcs = append(p.Srcs, "dmaAllocator.cpp")
        }
    }
    ctx.AppendProperties(p)
}

func libApusysHeaderDefaults(ctx android.LoadHookContext) {
    type props struct {
        Export_include_dirs []string
    }

    p := &props{}

    headerPath := "include"
    p.Export_include_dirs = append(p.Export_include_dirs, headerPath)

    ctx.AppendProperties(p)
}

func libApusysFactory() android.Module {
    module := cc.LibrarySharedFactory()
    android.AddLoadHook(module, libApusysDefaults)
    return module
}

func libApusysHeaderFactory() android.Module {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, libApusysHeaderDefaults)
    return module
}

func init() {
    android.RegisterModuleType("mtk_libapusys_platform_headers", libApusysHeaderFactory)
    android.RegisterModuleType("apusys_cc_library_shared", libApusysFactory)
}

