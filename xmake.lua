add_rules("mode.debug", "mode.release")

target("disable-steam-cloud-and-auto-update")
    set_kind("binary")
    add_includedirs("include/disable-steam-cloud-and-auto-update")
    add_files("src/*.cpp")
