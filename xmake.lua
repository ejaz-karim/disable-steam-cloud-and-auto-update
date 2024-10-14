target("disable-steam-cloud-and-auto-update")
    set_kind("binary")
    add_includedirs("include")
    add_headerfiles("include/**.hpp")
    add_files("src/**.cpp")

    if is_plat("windows") and is_tool("gcc") then
        add_ldflags("-static", "-static-libgcc", "-static-libstdc++", {force = true})
    end
