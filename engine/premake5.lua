ROOT_DIR = (path.getabsolute(".") .. "/")
BUILD_DIR = ROOT_DIR .. ".build/"

THIRD_PARTY = ROOT_DIR .."3rdparty/"
THIRD_PARTY_BUILD = THIRD_PARTY .."build/"
THIRD_PARTY_LIB = THIRD_PARTY_BUILD .."lib/"
THIRD_PARTY_INCLUDE = THIRD_PARTY_BUILD .. "include/"
THIRD_PARTY_INCLUDE_ARCH_DEP = THIRD_PARTY_INCLUDE

function dispatch_third_party_path()
  local arch = os.is64bit() and '64/' or '32/'
  local os_arch = _OS .. arch

  THIRD_PARTY_LIB = THIRD_PARTY_LIB .. os_arch
  THIRD_PARTY_INCLUDE_ARCH_DEP =  THIRD_PARTY_INCLUDE_ARCH_DEP .. os_arch
end

function toolchain(build_dir)
  dispatch_third_party_path()
  
    location (build_dir .. "projects/" .. _ACTION)

    floatingpoint "Fast"
    warnings "Extra"

    includedirs {
      THIRD_PARTY_INCLUDE,
      THIRD_PARTY_INCLUDE_ARCH_DEP,
      ROOT_DIR .. "src"
    }
  
    linkoptions {
	  THIRD_PARTY_LIB .. "libSDL2.a"
    }
      
    filter "Debug"
        defines {"DEBUG"}
        flags {"Symbols"}
        targetsuffix '_debug'
        optimize "Off"

    filter "Release"
        optimize "Full"
        defines {"NDEBUG"}

    filter "system:linux"
        buildoptions {"-std=c++0x", "-fPIC", "-msse2"}
	
	links {
	    "m",
	}

    filter {"system:linux", "platforms:x32"}
        targetdir (build_dir .. "linux32" .. "/bin")
        objdir (build_dir .. "linux32" .. "/obj")

    filter {"system:linux", "platforms:x64"}
        targetdir (build_dir .. "linux64" .. "/bin")
        objdir (build_dir .. "linux64" .. "/obj")

    filter {}
end
--------------------------------------------------------------------------------
solution "cyberego.org tech1"
    configurations {"Debug", "Release"}
    platforms {"native", "x32", "x64"}

    toolchain (BUILD_DIR)
--------------------------------------------------------------------------------
project "tech1"
    kind "ConsoleApp"
    language "C++"

    files {
        ROOT_DIR .. "src/**.cc",
        ROOT_DIR .. "src/**.h",
    }

    filter "Debug"
        defines {"CETECH1_DEBUG"}

    filter {}
--------------------------------------------------------------------------------
project "tech1_test"
    kind "ConsoleApp"
    language "C++"

    files {
        ROOT_DIR .. "src/**.cc",
        ROOT_DIR .. "src/**.h",

        ROOT_DIR .. "tests/**.cc",
        ROOT_DIR .. "tests/**.h",
    }

    excludes {
             ROOT_DIR .. "src/runtime/main.cc",
    }

    filter "Debug"
        defines {"CETECH1_DEBUG"}

    filter {}
