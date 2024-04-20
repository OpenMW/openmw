
yojimbo_version = "1.0"

solution "Yojimbo"
    kind "ConsoleApp"
    language "C++"
    configurations { "Debug", "Release" }
    includedirs { ".", "include", "sodium", "tlsf", "netcode", "reliable", "serialize" }
    if not os.istarget "windows" then
        targetdir "bin/"  
    end
    rtti "Off"
    warnings "Extra"
    flags { "FatalWarnings" }
    floatingpoint "Fast"
    filter "configurations:Debug"
        symbols "On"
        defines { "YOJIMBO_DEBUG", "NETCODE_DEBUG", "RELIABLE_DEBUG" }
    filter "configurations:Release"
        symbols "Off"
        optimize "Speed"
        defines { "YOJIMBO_RELEASE", "NETCODE_RELEASE", "RELIABLE_RELEASE" }

project "sodium-builtin"
    kind "StaticLib"
    language "C"
    files { "sodium/dummy.c" }
    filter "system:windows"
            files {
                "sodium/**.c",
                "sodium/**.h",
            }
        filter { "system:not windows", "platforms:*x64 or *avx or *avx2" }
            files {
                "sodium/**.S"
            }
        filter { "action:gmake*" }
            buildoptions { "-Wno-unused-parameter", "-Wno-unused-function", "-Wno-unknown-pragmas", "-Wno-unused-variable", "-Wno-type-limits" }

project "netcode"
    kind "StaticLib"
    language "C"
    defines { "NETCODE_ENABLE_TESTS=1" }
    files { "netcode/netcode.c", "netcode/netcode.h" }

project "reliable"
    kind "StaticLib"
    language "C"
    defines { "RELIABLE_ENABLE_TESTS=1" }
    files { "reliable/reliable.c", "reliable/reliable.h" }

project "tlsf"
    kind "StaticLib"
    language "C"
    files { "tlsf/tlsf.c", "tlsf/tlsf.h" }

project "yojimbo"
    kind "StaticLib"
    files { "include/*.h", "source/*.cpp" }

project "client"
    files { "client.cpp", "shared.h" }
    filter "system:windows"
        links { "yojimbo", "sodium-builtin", "tlsf", "netcode", "reliable" }
    filter "system:not windows"
        links { "yojimbo", "sodium", "tlsf", "netcode", "reliable" }

project "server"
    files { "server.cpp", "shared.h" }
    filter "system:windows"
        links { "yojimbo", "sodium-builtin", "tlsf", "netcode", "reliable" }
    filter "system:not windows"
        links { "yojimbo", "sodium", "tlsf", "netcode", "reliable" }

project "loopback"
    files { "loopback.cpp", "shared.h" }
    filter "system:windows"
        links { "yojimbo", "sodium-builtin", "tlsf", "netcode", "reliable" }
    filter "system:not windows"
        links { "yojimbo", "sodium", "tlsf", "netcode", "reliable" }

project "soak"
    files { "soak.cpp", "shared.h" }
    filter "system:windows"
        links { "yojimbo", "sodium-builtin", "tlsf", "netcode", "reliable" }
    filter "system:not windows"
        links { "yojimbo", "sodium", "tlsf", "netcode", "reliable" }

project "test"
    files { "test.cpp" }
    defines { "SERIALIZE_ENABLE_TESTS=1" }
    filter "system:windows"
        links { "yojimbo", "sodium-builtin", "tlsf", "netcode", "reliable" }
    filter "system:not windows"
        links { "yojimbo", "sodium", "tlsf", "netcode", "reliable" }

newaction
{
    trigger     = "clean",

    description = "Clean all build files and output",

    execute = function ()

        files_to_delete = 
        {
            "Makefile",
            "*.make",
            "*.txt",
            "*.zip",
            "*.tar.gz",
            "*.db",
            "*.opendb",
            "*.vcproj",
            "*.vcxproj",
            "*.vcxproj.user",
            "*.vcxproj.filters",
            "*.sln",
            "*.xcodeproj",
            "*.xcworkspace"
        }

        directories_to_delete = 
        {
            "obj",
            "ipch",
            "bin",
            ".vs",
            "Debug",
            "Release",
            "release",
            "cov-int",
            "docker/yojimbo",
            "valgrind/yojimbo",
            "docs",
            "xml"
        }

        for i,v in ipairs( directories_to_delete ) do
          os.rmdir( v )
        end

        if not os.istarget "windows" then
            os.execute "find . -name .DS_Store -delete"
            for i,v in ipairs( files_to_delete ) do
              os.execute( "rm -f " .. v )
            end
        else
            for i,v in ipairs( files_to_delete ) do
              os.execute( "del /F /Q  " .. v )
            end
        end

    end
}
