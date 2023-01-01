package LibAPUSYS_EDMA

import (
    "strings"

    "android/soong/android"
    "android/soong/cc"
)

func libApusys_edmaDefaults(ctx android.LoadHookContext) {
    type props struct {
        Srcs []string
        Include_dirs []string
        Cflags []string
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
            p.Srcs = append(p.Srcs,"2.0/edma_core_v20.cpp")
            p.Srcs = append(p.Srcs,"3.0/edma_core.cpp")
            p.Srcs = append(p.Srcs,"3.0/edma_mvpu_core.cpp")
            p.Srcs = append(p.Srcs,"3.0/edma_sdk_core.cpp")
            p.Srcs = append(p.Srcs, "edmaEngine.cpp")
            p.Srcs = append(p.Srcs, "edmaDesEngine.cpp")
        } else {

         // layer decoupling 1.0 chips will go to this branch
         // platform macro is still supported here
           mtkPlatformDir := strings.ToLower(vars.String("TARGET_BOARD_PLATFORM"))
            if mtkPlatformDir == "mt6885" || mtkPlatformDir == "mt6873" || mtkPlatformDir == "mt6893" || mtkPlatformDir == "mt6853" {
                edmaVersion := "2.0"
                p.Srcs = append(p.Srcs,"2.0/edma_core_v20.cpp")
                p.Srcs = append(p.Srcs,edmaVersion + "/edma_core.cpp")
                p.Srcs = append(p.Srcs,edmaVersion + "/edma_mvpu_core.cpp")
                p.Srcs = append(p.Srcs,edmaVersion + "/edma_sdk_core.cpp")
                p.Srcs = append(p.Srcs, "edmaEngine.cpp")
                p.Srcs = append(p.Srcs, "edmaDesEngine.cpp")
            }
            if mtkPlatformDir == "mt8195" || mtkPlatformDir == "mt6877" {
                edmaVersion := "3.0"
                p.Srcs = append(p.Srcs,"2.0/edma_core_v20.cpp")
                p.Srcs = append(p.Srcs,edmaVersion + "/edma_core.cpp")
                p.Srcs = append(p.Srcs,edmaVersion + "/edma_mvpu_core.cpp")
                p.Srcs = append(p.Srcs,edmaVersion + "/edma_sdk_core.cpp")
                p.Srcs = append(p.Srcs, "edmaEngine.cpp")
                p.Srcs = append(p.Srcs, "edmaDesEngine.cpp")
            }
        }
        ctx.AppendProperties(p)
}

func libApusys_edmaFactory() android.Module {
    module := cc.LibrarySharedFactory()
    android.AddLoadHook(module, libApusys_edmaDefaults)
    return module
}

func init() {
    android.RegisterModuleType("apusys_edma_cc_library_shared", libApusys_edmaFactory)
}

