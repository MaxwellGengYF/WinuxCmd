set_project("WinuxCmd")
set_xmakever("3.0.0")

includes("scripts/xmake_func.lua")

add_rules("mode.debug", "mode.release")

set_warnings("all")

-- C++ standard version (e.g., cxx17, cxx20, cxx23)
option("lc_cxx_standard", {default = 'cxx20'})
-- C standard version (e.g., c11, clatest)
option("lc_c_standard", {default = 'clatest'})
-- enable sse and sse2 SIMD
option("lc_enable_simd", {default = true})

option("lc_rtti", {default = false})

option("enable_tests")
    set_default(true)
    set_showmenu(true)
    set_description("Enable tests")
option_end()

option("build_ffi")
    set_default(true)
    set_showmenu(true)
    set_description("Build FFI library")
option_end()


-- Core static library (utils, core, container, version, ffi, tools)
target("winuxcmd-core")
    add_rules("lc_basic_settings", {project_kind = "static", enable_exception = true})
    add_files("src/utils/*.cpp", "src/core/*.cpp", "src/container/*.cpp", "src/version/*.cpp", "src/ffi/*.cpp", "src/tools/*.cpp")
    set_pcxxheader("src/pch/pch.h")
    on_load(function(target)
        target:set('pcxxheader', path.join(os.scriptdir(), "src/pch/pch.h"))
    end)
    add_includedirs("src", "$(builddir)/generated")
target_end()

-- Commands static library
target("winuxcmd-commands")
    add_rules("lc_basic_settings", {project_kind = "static", enable_exception = true})
    add_files("src/commands/*.cpp")
    set_pcxxheader("src/pch/pch.h")
    on_load(function(target)
        target:set('pcxxheader', path.join(os.scriptdir(), "src/pch/pch.h"))
    end)
    add_deps("winuxcmd-core")
    add_includedirs("src", "$(builddir)/generated")
target_end()

-- Main executable
target("winuxcmd")
    add_rules("lc_basic_settings", {project_kind = "binary", enable_exception = true})
    add_files("src/Main/*.cpp")
    set_pcxxheader("src/pch/pch.h")
    add_deps("winuxcmd-commands", "winuxcmd-core")
    add_includedirs("src", "$(builddir)/generated")
    if is_plat("windows") then
        add_syslinks("user32", "shell32", "advapi32", "ole32", "oleaut32")
    end
target_end()

-- FFI library (optional)
if has_config("build_ffi") then
    target("winuxcore")
        add_rules("lc_basic_settings", {project_kind = "shared"})
        add_files("src/ffi/ffi.cpp")
        set_pcxxheader("src/pch/pch.h")
        add_deps("winuxcmd-commands")
        add_defines("WINUX_FFI_EXPORTS")
        if is_plat("windows") then
            add_cxflags("/std:c++latest", "/experimental:module", "/interface", {force = true})
        end
        if is_mode("release") then
            add_cxflags("/O2", "/Os", {force = true})
            add_ldflags("/LTCG", "/OPT:REF", "/OPT:ICF", {force = true})
        end
        add_includedirs("src", "$(builddir)/generated")
        add_ldflags("/WHOLEARCHIVE:winuxcmd-commands.lib", {force = true})
    target_end()
end

-- Tests (optional)
if has_config("enable_tests") then
    target("test_main")
        add_rules("lc_basic_settings", {project_kind = "static"})
        add_files("tests/framework/test_main.cpp")
        add_includedirs("tests", "tests/framework")
        if is_plat("windows") then
            add_cxflags("/std:c++latest", {force = true})
        end
    target_end()

    target("winux-test")
        add_rules("lc_basic_settings", {project_kind = "static"})
        add_files("tests/framework/*.cpp")
        remove_files("tests/framework/test_main.cpp")
        add_includedirs("tests", "tests/framework")
        if is_plat("windows") then
            add_cxflags("/std:c++latest", {force = true})
        end
    target_end()

    target("winuxcmd-tests")
        add_rules("lc_basic_settings", {project_kind = "binary"})
        add_files("tests/unit/**/*.cpp")
        add_deps("winux-test", "test_main")
        add_includedirs("tests", "tests/framework", "$(builddir)/generated")
        add_defines("WINUXCMD_BIN_DIR=L\"" .. path.absolute("$(builddir)") .. "\"")
        if is_plat("windows") then
            add_cxflags("/std:c++latest", {force = true})
        end
    target_end()
end


-- Examples
target("container_module")
    add_rules("lc_basic_settings", {project_kind = "static"})
    add_files("src/container/constexpr_map.cpp", "src/container/container.cpp", "src/container/small_vector.cpp")
    add_includedirs("src")
    if is_plat("windows") then
        add_cxflags("/std:c++latest", "/experimental:module", {force = true})
    end
target_end()

for _, example in ipairs({"constexpr_map_example", "small_vector_example"}) do
    local src = "examples/container/" .. example .. ".cpp"
    if os.isfile(src) then
        target(example)
            add_rules("lc_basic_settings", {project_kind = "binary"})
            add_files(src)
            add_deps("container_module")
            add_includedirs("src", "$(builddir)/generated")
            if is_plat("windows") then
                add_cxflags("/std:c++latest", "/experimental:module", {force = true})
            end
        target_end()
    end
end

if has_config("build_ffi") then
    target("ffi_example")
        add_rules("lc_basic_settings", {project_kind = "binary"})
        add_files("examples/ffi/ffi_example.c")
        add_deps("winuxcore")
        add_includedirs("src/ffi")
        if is_plat("windows") then
            add_cflags("/std:c11", {force = true})
        end
    target_end()
end