package LibAPUSYSSAMPLE

import (
    "strings"

    "android/soong/android"
    "android/soong/cc"
)

func libApusysSampleDefaults(ctx android.LoadHookContext) {
    type props struct {
        Srcs []string
        Include_dirs []string
        Cflags []string
    }

    p := &props{}
    vars := ctx.Config().VendorConfig("mtkPlugin")
    mtkPlatformDir := strings.ToLower(vars.String("TARGET_BOARD_PLATFORM"))

    if vars.Bool("MTK_APUSYS_SUPPORT") {
       if mtkPlatformDir == "mt6885" || mtkPlatformDir == "mt6873" || mtkPlatformDir == "mt6853" || mtkPlatformDir == "mt6893" || mtkPlatformDir == "mt6877" {
            p.Srcs = append(p.Srcs, "sampleEngine.cpp")
        }

        ctx.AppendProperties(p)
    }
}

func libApusysSampleFactory() android.Module {
    module := cc.LibraryStaticFactory()
    android.AddLoadHook(module, libApusysSampleDefaults)
    return module
}

func init() {
    android.RegisterModuleType("apusys_sample_cc_library_static", libApusysSampleFactory)
}

