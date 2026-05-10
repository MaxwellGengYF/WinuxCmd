rule("lc_basic_settings")
on_config(function(target)
    -- Linux-specific: Use libc++ with Clang
    -- See: https://github.com/LuisaGroup/LuisaCompute/issues/58
    if target:is_plat("linux") then
        if target:has_tool("cxx", "clang", "clangxx") then
            target:add("cxflags", "-stdlib=libc++", {
                force = true
            })
            target:add("syslinks", "c++")
        end
    end
    -- Note: LTO is disabled by default
end)

on_load(function(target)
    -- Helper function to get configuration value from multiple sources
    local function _get_or(name, default_value)
        local v = target:extraconf("rules", "lc_basic_settings", name)
        name = 'lc_' .. name
        if v == nil then
            v = target:values(name)
        end
        if v == nil then
            v = get_config(name)
        end
        if v then
            return v
        end
        return default_value or false
    end

    local function empty_str(value)
        return type(value) == 'string' and #value == 0
    end

    -- Apply toolchain configuration
    local toolchain = _get_or("toolchain")
    if toolchain and not empty_str(toolchain) then
        target:set("toolchains", toolchain)
    end

    -- Apply project type (static/shared library, executable, etc.)
    local project_kind = _get_or("project_kind")
    if project_kind and not empty_str(project_kind) then
        target:set("kind", project_kind)
    end

    -- Linux: Position independent code for static libraries
    if target:is_plat("linux") then
        if project_kind == "static" or project_kind == "object" then
            target:add("cxflags", "-fPIC")
        end
    end

    -- macOS-specific flags
    if target:is_plat("macosx") then
        target:add("cxflags", "-no-pie")
        target:add("cxflags", "-Wno-invalid-specialization", {
            tools = {"clang"}
        })
    end

    -- Enable FMA (Fused Multiply-Add) on x64 platforms
    if target:is_arch("x64", "x86_64") then
        target:add("cxflags", "-mfma", {
            tools = {"clang", "gcc"}
        })
    end

    -- Set C/C++ language standards
    local c_standard = _get_or("c_standard")
    local cxx_standard = _get_or("cxx_standard")
    if c_standard and not empty_str(c_standard) then
        target:set("languages", c_standard, {
            public = true
        })
    end
    if c_standard and not empty_str(cxx_standard) then
        target:set("languages", cxx_standard, {
            public = true
        })
    end

    -- Configure exception handling
    local enable_exception = _get_or("enable_exception")
    if not empty_str(enable_exception) then
        if enable_exception then
            target:set("exceptions", "cxx")
        else
            target:set("exceptions", "no-cxx")
        end
    end

    -- Mode-specific configurations
    local win_runtime
    local opt
    if is_mode("debug") then
        win_runtime = _get_or('win_runtime', 'MDd')
        opt = _get_or("optimize", "none")
        target:add("cxflags", "/GS", "/Gd", {
            tools = {"clang_cl", "cl"},
            public = true
        })
    elseif is_mode("releasedbg") then
        win_runtime = _get_or('win_runtime', 'MD')
        opt = _get_or("optimize", "none")
        target:add("cxflags", "/GS-", "/Gd", {
            tools = {"clang_cl", "cl"},
            public = true
        })
    else
        win_runtime = _get_or('win_runtime', 'MD')
        opt = _get_or("optimize", "aggressive")
        target:add("cxflags", "/GS-", "/Gd", {
            tools = {"clang_cl", "cl"},
            public = true
        })
    end

    if not empty_str(opt) then
        target:set("optimize", opt)
    end

    local warnings = _get_or("warnings", "none")
    if not empty_str(warnings) then
        target:set("warnings", warnings)
    end

    if not empty_str(win_runtime) then
        target:set("runtimes", win_runtime, {
            public = true
        })
    end

    -- MSVC-specific preprocessor settings
    target:add("cxflags", "/Zc:preprocessor", "/wd4244", {
        tools = "cl",
        public = true
    });

    -- SIMD extensions configuration
    if _get_or("enable_simd") then
        if is_arch("arm64") then
            -- NEON is always available on aarch64; vectorexts "neon" emits
            -- -mfpu=neon which is ARM32-only and errors on ARM64 macOS/Linux.
            if not target:is_plat("macosx", "linux") then
                target:add("vectorexts", "neon", {
                    public = true
                })
            end
        else
            target:add("vectorexts", "avx", "avx2", {
                public = true
            })
        end
    end

    -- Link Time Optimization (LTO) configuration
    local use_lto = _get_or("lto", false)
    if not empty_str(use_lto) then
        target:set("policy", "build.optimization.lto", use_lto)
        if use_lto then
            -- Use LLVM tools when using Clang toolchain with LTO
            if toolchain:find("clang") or toolchain:find("llvm") then
                target:set("toolset", "ld", "lld-link")
                target:set("toolset", "ar", "llvm-ar")
            end
        end

    end
    -- RTTI (Run-Time Type Information) configuration
    local use_rtti = _get_or("rtti", false)
    if not empty_str(use_rtti) then
        if use_rtti or has_config("_lc_enable_py") then
            -- Enable RTTI
            target:add("cxflags", "/GR", {
                tools = {"clang_cl", "cl"}
            })
        else
            -- Disable RTTI
            target:add("cxflags", "/GR-", {
                tools = {"clang_cl", "cl"}
            })
            target:add("cxflags", "-fno-rtti", "-fno-rtti-data", {
                tools = {"clang"}
            })
            target:add("cxflags", "-fno-rtti", {
                tools = {"gcc"}
            })
        end
    end
end)
rule_end()
