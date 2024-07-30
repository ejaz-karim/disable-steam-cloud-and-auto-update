add_rules("mode.debug", "mode.release")

target("disable-steam-cloud-and-auto-update")
    set_kind("binary")
    add_headerfiles("include/disable-steam-cloud-and-auto-update/*.hpp")
    add_files("src/*.cpp")
