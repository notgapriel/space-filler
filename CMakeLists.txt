cmake_minimum_required (VERSION 4.0.0)
set(TARGET "space-filler")
project (${TARGET})
message(NOTICE "Compiler: ${CMAKE_CXX_COMPILER_ID} v${CMAKE_CXX_COMPILER_VERSION}, OS: ${CMAKE_SYSTEM_NAME}")

set(EXECUTABLE_OUTPUT_PATH "out")

# appears recursive wildcard (**) not behaving as it's meant to. `src/**/` should include `src/`
file(
  GLOB_RECURSE
  SRC_FILES
  CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_SOURCE_DIR}/src/**/*.cpp"
)

add_executable(${TARGET} ${SRC_FILES})

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(${TARGET} PRIVATE -Wno-literal-suffix -fconstexpr-ops-limit=1000000000)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  # ideally, in future, able to remove `-Wno-undefined-var-template` and `-Wno-uninitialized`
  target_compile_options(${TARGET} PRIVATE -Wno-user-defined-literals -Wno-reserved-user-defined-literal -Wno-undefined-var-template -Wno-uninitialized -fbracket-depth=65536 -ftemplate-backtrace-limit=0 -fconstexpr-steps=1000000000)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  target_compile_options(${TARGET} PRIVATE -mmacosx-version-min=14.5)
endif()

target_compile_options(${TARGET} PRIVATE -Wno-deprecated-declarations -Werror -Wno-error=cpp -ftemplate-depth=20000 -Wswitch)

include_directories(
  "${CMAKE_SOURCE_DIR}/include/"
)

execute_process(COMMAND GraphicsMagick++-config --cppflags --cxxflags OUTPUT_VARIABLE COMPILER_FLAGS)
execute_process(COMMAND GraphicsMagick++-config --ldflags --libs OUTPUT_VARIABLE LINKING_FLAGS)

string(REGEX REPLACE "[ \n]" ";" COMPILER_FLAGS_LIST ${COMPILER_FLAGS})
string(REGEX REPLACE "[ \n]" ";" LINKING_FLAGS_LIST ${LINKING_FLAGS})

target_compile_options(${TARGET} PRIVATE ${COMPILER_FLAGS_LIST})
target_link_options(${TARGET} PRIVATE ${LINKING_FLAGS_LIST})

set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD 26)
