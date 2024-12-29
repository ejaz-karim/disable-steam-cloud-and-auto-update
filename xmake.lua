target("disable-steam-cloud-and-auto-update")
    set_kind("binary")
    add_includedirs("include")
    add_headerfiles("include/**.hpp")
    add_files("src/**.cpp")

    if is_plat("mingw") then
        set_toolchains("mingw")
        add_ldflags("-static-libgcc", "-static-libstdc++", "-static", {force = true})
    end
