package LibAPU

import (
    "strings"
    "android/soong/android"
    "android/soong/cc"
    "github.com/google/blueprint/proptools"
)

var kApusysLegacyPlatform = []string {"mt6853", "mt6873", "mt6877", "mt6885", "mt6893"}

func IsLegacyPlatformSupportApusys(platform string) bool {
    for _, value := range kApusysLegacyPlatform {
        if platform == value { return true }
    }

    return false
}

func libApuMdwBatchDefaults(ctx android.LoadHookContext) {
    type props struct {
        Srcs []string
        Include_dirs []string
        Cflags []string
        Shared_libs []string
        Enabled *bool
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
        p.Srcs = append(p.Srcs, "apusysBatchEntry.cpp")
    } else {
        // layer decoupling 1.0 chips will go to this branch
        // platform macro is still supported here
        p.Srcs = append(p.Srcs, "apusysBatchEntry.cpp")
        mtkPlatformDir := strings.ToLower(vars.String("TARGET_BOARD_PLATFORM"))
        p.Enabled = proptools.BoolPtr(IsLegacyPlatformSupportApusys(mtkPlatformDir))
    }
    ctx.AppendProperties(p)
}

func libApuMdwBatchFactory() android.Module {
    module := cc.LibrarySharedFactory()
    android.AddLoadHook(module, libApuMdwBatchDefaults)
    return module
}

func init() {
    android.RegisterModuleType("apu_mdw_batch_cc_library_shared", libApuMdwBatchFactory)
}

