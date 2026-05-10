set_project("WinuxCmd")
set_xmakever("3.0.0")

includes("scripts/xmake_func.lua")

add_rules("mode.debug", "mode.release")

set_warnings("all")

-- C++ standard version (e.g., cxx17, cxx20, cxx23)
option("lc_cxx_standard", {default = 'cxx20'})
-- C standard version (e.g., c11, clatest)
option("lc_c_standard", {default = 'clatest'})

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
