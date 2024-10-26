include (GoogleTest)


add_executable (unittests test_lru.cpp test_iumap.cpp)
setup_target(unittests)
target_include_directories(unittests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")


set (clang_options -Wno-exit-time-destructors -Wno-global-constructors -Wno-used-but-marked-unused)
set (gcc_options)
set (msvc_options -wd4702) # unreachable code.
target_compile_options (
  unittests
  PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${clang_options}>
    $<$<CXX_COMPILER_ID:GNU>:${gcc_options}>
    $<$<CXX_COMPILER_ID:MSVC>:${msvc_options}>
)

gtest_discover_tests (unittests)

if (FUZZTEST)
  target_link_libraries (unittests PRIVATE gtest gmock)
  link_fuzztest (unittests)
else ()
  target_link_libraries (unittests PRIVATE gmock_main)
endif (FUZZTEST)
