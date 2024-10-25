cmake_minimum_required(VERSION 3.22)
project (cache)

include (CheckCXXCompilerFlag)
include (CheckLinkerFlag)


option (FUZZ_TEST "Enable fuzz testing" No)

function (setup_target target)
  cmake_parse_arguments (
    arg # prefix
    "" # options
    "PEDANTIC" # one-value-keywords
    "" # multi-value-keywords
    ${ARGN}
  )
  # PEDANTIC defaults to "Yes".
  if ("${arg_PEDANTIC}" STREQUAL "")
    set (arg_PEDANTIC "Yes")
  endif ()

  set (clang_warning_options
    -Weverything
    -Wno-c++14-extensions
    -Wno-c++20-compat
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-c99-extensions
    -Wno-covered-switch-default
    -Wno-ctad-maybe-unsupported
    -Wno-documentation-unknown-command
    -Wno-packed
    -Wno-padded
    -Wno-switch-default
    -Wno-switch-enum
    -Wno-weak-vtables
  )
  # Some clang warning switches are not available in all versions of the compiler.
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang$")
    check_cxx_compiler_flag (-Wno-unsafe-buffer-usage CLANG_W_UNSAFE_BUFFER_USAGE)
    list (APPEND clang_warning_options $<$<BOOL:${CLANG_W_UNSAFE_BUFFER_USAGE}>:-Wno-unsafe-buffer-usage>)
  endif ()

  set (gcc_warning_options
    -Wall
    -Wextra
    -Wtrampolines
    -pedantic
  )
  if (CMAKE_CXX_COMPILER_ID MATCHES "GCC")
    check_cxx_compiler_flag (-Wbidi-chars=any GCC_W_BIDI_CHARS)
    list (APPEND gcc_warning_options $<$<BOOL:${GCC_W_BIDI_CHARS}>:-Wbidi-chars=any)
  endif()

  set (msvc_warning_options
    -W4
    -wd4324 # 4324: structure was padded due to alignment specifier
    -wd4068 # 4068: unknown pragma
  )

  set (clang_options
    -fstack-protector-strong
  )
  set (gcc_options
    -fstack-clash-protection
    -fstack-protector-strong
    -fPIE
    -pie
    -U_FORTIFY_SOURCE
    -D_FORTIFY_SOURCE=3
  )
  set (msvc_options /bigobj)

  if (LINUX)
    # TODO: On AArch64 use -mbranch-protection=standard?
    list (APPEND clang_options -fstack-clash-protection -fcf-protection=full)
    list (APPEND gcc_options  -fstack-clash-protection -fcf-protection=full)
  endif (LINUX)

  if (MIDI2_WERROR)
    list (APPEND clang_options -Werror)
    list (APPEND gcc_options -Werror)
    list (APPEND msvc_options /WX)
  endif ()
  if (MIDI2_COVERAGE)
    list (APPEND clang_options -fprofile-instr-generate -fcoverage-mapping)
    list (APPEND gcc_options -fprofile-arcs -ftest-coverage)
  endif ()
  if (MIDI2_SANITIZE)
    list (APPEND clang_options -fsanitize=undefined -fsanitize=address)
    list (APPEND gcc_options -fsanitize=undefined -fsanitize=address -fno-sanitize-recover)
  endif ()

  list (APPEND clang_options $<$<BOOL:${arg_PEDANTIC}>:${clang_warning_options}>)
  list (APPEND gcc_options   $<$<BOOL:${arg_PEDANTIC}>:${gcc_warning_options}>)
  list (APPEND msvc_options  $<$<BOOL:${arg_PEDANTIC}>:${msvc_warning_options}>)

  set (clang_definitions
    _GLIBCXX_ASSERTIONS
    _LIB_CPP_ASSERT
    _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG
  )
  set (gcc_definitions
    _GLIBCXX_ASSERTIONS
    _LIB_CPP_ASSERT
    _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG
  )
  set(msvc_definitions )

  target_compile_options (${target} PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:IntelLLVM>>:${clang_options}>
    $<$<CXX_COMPILER_ID:GNU>:${gcc_options}>
    $<$<CXX_COMPILER_ID:MSVC>:${msvc_options}>
  )

  target_compile_definitions (${target} PRIVATE MIDI2_FUZZTEST=$<BOOL:${MIDI2_FUZZTEST}>)
  target_compile_definitions (${target} PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${clang_definitions}>
    $<$<CXX_COMPILER_ID:GNU>:${gcc_definitions}>
    $<$<CXX_COMPILER_ID:MSVC>:${msvc_definitions}>
  )

  target_compile_features (${target} PUBLIC cxx_std_20)
  target_link_options (${target} PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:IntelLLVM>>:${clang_options}>
    $<$<CXX_COMPILER_ID:GNU>:${gcc_options}>
    $<$<CXX_COMPILER_ID:MSVC>:>
  )

  # If the link step supports -Wl,-Map generate a map file from the link.
  check_linker_flag (CXX "-Wl,-Map=map.txt" map_supported)
  if (map_supported)
    target_link_options (${target} PRIVATE "-Wl,-Map=$<TARGET_FILE_DIR:${target}>/map.txt")
  endif ()

  # Restrict dlopen(3) calls to shared objects. Supported since binutils 2.10.
  check_linker_flag (CXX "-Wl,-z,nodlopen" nodlopen_supported)
  if (nodlopen_supported)
    target_link_options (${target} PRIVATE "-Wl,-z,nodlopen")
  endif ()
  # Enable data execution prevention by marking stack memory as non-executable. Supported since binutils 2.14.
  check_linker_flag (CXX "-Wl,-z,noexecstack" noexecstack_supported)
  if (noexecstack_supported)
      target_link_options (${target} PRIVATE "-Wl,-z,noexecstack")
  endif()
  # Mark relocation table entries resolved at load- time as read-only. Supported since binutils 2.15.
  check_linker_flag (CXX "-Wl,-z,relro" relro_supported)
  if (relro_supported)
      target_link_options (${target} PRIVATE "-Wl,-z,relro")
  endif ()
  # Mark relocation table entries resolved at load- time as read-only. Disable lazy binding. Supported since binutils 2.15.
  check_linker_flag (CXX "-Wl,-z,now" now_supported)
  if (now_supported)
      target_link_options (${target} PRIVATE "-Wl,-z,now")
  endif()
endfunction (setup_target)









add_executable(cache main.cpp cache.hpp iumap.hpp lru_list.hpp)
target_compile_features(cache PUBLIC cxx_std_20)
target_include_directories(cache PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
setup_target(cache)



if (FUZZTEST)
  set (FUZZTEST_FUZZING_MODE On)
  include (FetchContent)
  set (FUZZTEST_REPO_BRANCH "main" CACHE STRING "FuzzTest repository branch.")
  message ("Building fuzztest at tag " ${FUZZTEST_REPO_BRANCH})
  FetchContent_Declare (
    fuzztest
    GIT_REPOSITORY https://github.com/google/fuzztest.git
    GIT_TAG ${FUZZTEST_REPO_BRANCH}
  )
  FetchContent_MakeAvailable (fuzztest)
  include (GoogleTest)
  fuzztest_setup_fuzzing_flags ()

  add_subdirectory(unittests)
  enable_testing ()
else ()
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/googletest/CMakeLists.txt")
    # Tell gtest to link against the "Multi-threaded Debug DLL runtime library"
    # on Windows.
    set (gtest_force_shared_crt On CACHE BOOL "Always use msvcrt.dll")
    # We don't want to install either gtest or gmock.
    set (INSTALL_GTEST Off CACHE BOOL "Disable gtest install")
    set (INSTALL_GMOCK Off CACHE BOOL "Disable gmock install")
    add_subdirectory ("${CMAKE_CURRENT_SOURCE_DIR}/googletest")

    add_subdirectory (unittests)
    enable_testing ()
  endif()
endif(FUZZTEST)


foreach (target gtest gmock gmock_main gtest_main)
  if (TARGET ${target})
      target_compile_options (${target} PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wno-deprecated-declarations -Wno-covered-switch-default>
      )
      target_compile_features(${target} PUBLIC cxx_std_20)
    target_compile_definitions(${target} PUBLIC GTEST_REMOVE_LEGACY_TEST_CASEAPI_=1)
  endif()
endforeach()

