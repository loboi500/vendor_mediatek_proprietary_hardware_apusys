package LibAPUSYSSAMPLE

import (
    "android/soong/android"
    "android/soong/cc"
)

func libApuSampleDefaults(ctx android.LoadHookContext) {
    type props struct {
        Srcs []string
        Include_dirs []string
        Cflags []string
    }

    p := &props{}

    p.Srcs = append(p.Srcs, "sampleEngine.cpp")

    ctx.AppendProperties(p)
}

func libApuSampleFactory() android.Module {
    module := cc.LibraryStaticFactory()
    android.AddLoadHook(module, libApuSampleDefaults)
    return module
}

func init() {
    android.RegisterModuleType("apu_sample_cc_library_static", libApuSampleFactory)
}

