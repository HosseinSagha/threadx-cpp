cmake_minimum_required(VERSION 3.31.12)

# Project variables
set(PROJECT_NAME threadx-cpp-test-app)
set(VERSION_MAJOR 0) # When you make incompatible API or HW changes
set(VERSION_MINOR 1) # When you add functionality in a backwards compatible manner
set(VERSION_PATCH 0) # Post-release version. When you make backwards compatible bug fixes. Must be 0 for release and release candidate versions

set(MCU_FAMILY STM32L4xx) # MCU HAL driver folder
set(MCU_LINE STM32L496xx) # MCU line define macro
set(MCU_DEVICE STM32L496ZG) # MCU device name
set(MCU_TYPE STM32L496ZGTX) # Linker/Startup filenames

set(THREADX_VER v6.4.5.202504_rel)
set(FILEX_VER v6.4.5.202504_rel)
set(LEVELX_VER v6.4.5.202504_rel)

set(MCU_DEVICE ${MCU_DEVICE} CACHE STRING "MCU device name" FORCE)

include(FetchContent)
find_package(Git REQUIRED)

set(GITHUB_SUBDIR ../)
fetchcontent_declare(toolchain-gnu
        GIT_REPOSITORY ${GITHUB_SUBDIR}/toolchain-gnu.git
        GIT_TAG origin/main
        SYSTEM)
fetchcontent_makeavailable(toolchain-gnu)
fetchcontent_getproperties(toolchain-gnu SOURCE_DIR TOOLCHAIN_DIR)

fetchcontent_declare(stm32-svd
        GIT_REPOSITORY ${GITHUB_SUBDIR}/stm32-svd.git
        GIT_TAG origin/main
        SYSTEM
        EXCLUDE_FROM_ALL)
fetchcontent_makeavailable(stm32-svd)

fetchcontent_getproperties(stm32-svd SOURCE_DIR SVD_DIR)
set(SVD_DIR ${SVD_DIR} CACHE PATH "Path to SVD directory" FORCE)

# Set toolchain and project
set(CMAKE_TOOLCHAIN_FILE ${TOOLCHAIN_DIR}/cortex-m4f.cmake)
project(${PROJECT_NAME}
        LANGUAGES C CXX ASM
        VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release) # Default build type is release
endif ()

fetchcontent_declare(utility
        GIT_REPOSITORY ${GITHUB_SUBDIR}/utility.git
        GIT_TAG origin/main
        SYSTEM)

fetchcontent_declare(segger-rtt
        GIT_REPOSITORY ${GITHUB_SUBDIR}/segger-rtt.git
        GIT_TAG origin/main
        SYSTEM)

fetchcontent_makeavailable(utility segger-rtt)

add_compile_definitions(${MCU_LINE}
        SYSTEM_CLOCK=16000000
)

# Link libs and dirs
set(LINK_DIRS ${PROJECT_SOURCE_DIR}/Middlewares/ST/ARM/DSP/Lib)

configure_file(${PROJECT_SOURCE_DIR}/tx_user.h.in ${CMAKE_BINARY_DIR}/tx_user.h)
configure_file(${PROJECT_SOURCE_DIR}/fx_user.h.in ${CMAKE_BINARY_DIR}/fx_user.h)
configure_file(${PROJECT_SOURCE_DIR}/lx_user.h.in ${CMAKE_BINARY_DIR}/lx_user.h)

set(LIBS
        stm32_hal_drivers
        arm_cortexM4lf_math
        threadx-cpp
        utility
        utility
        segger-rtt
)
# Include directories
set(SYS_INC_DIRS
        ${PROJECT_SOURCE_DIR}/Middlewares/ST/ARM/DSP/Inc
)

set(INC_DIRS
        ${PROJECT_SOURCE_DIR}
)

# Find all source files
configure_file(${PROJECT_SOURCE_DIR}/Core/Src/main.c main.cpp)
file(GLOB USER_SRCS CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/*.cpp)
file(GLOB CORE_SRCS CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/Core/Src/*.* ${PROJECT_BINARY_DIR}/*.cpp
        ${CMAKE_SOURCE_DIR}/Core/ThreadSafe/*.c)
list(REMOVE_ITEM CORE_SRCS ${CMAKE_SOURCE_DIR}/Core/Src/main.c)
set_source_files_properties(${CORE_SRCS} PROPERTIES COMPILE_FLAGS -w) # Disable warnings for system sources

# Object library
add_library(SRC_OBJS OBJECT ${USER_SRCS} ${CORE_SRCS})
target_include_directories(SRC_OBJS SYSTEM PUBLIC ${SYS_INC_DIRS})
target_include_directories(SRC_OBJS PUBLIC ${INC_DIRS} ${PROJECT_BINARY_DIR})
target_link_libraries(SRC_OBJS PRIVATE ${LIBS}) # Add dependencies so it sees libs' public properties

set(LINKER_SCRIPT ${PROJECT_SOURCE_DIR}/${MCU_TYPE}_FLASH) # Linker file

# Startup source
string(TOLOWER ${MCU_TYPE} MCU_TYPE)
set(STARTUP_SOURCE ${PROJECT_SOURCE_DIR}/Core/Startup/startup_${MCU_TYPE})

# Add libs
add_subdirectory(${PROJECT_SOURCE_DIR}/Drivers)
add_subdirectory(.. ..) # Pull in the scheduler library from the repo root.

# Read version number
if (CMAKE_BUILD_TYPE STREQUAL Release AND DEFINED ENV{CI_PIPELINE_ID})
    set(VERSION_SUFFIX -${PROJECT_VERSION})
endif ()

configure_file(${PROJECT_SOURCE_DIR}/version.hpp.in version.hpp)
set(EXEC_NAME ${PROJECT_NAME}${VERSION_SUFFIX})

# Add project binaries
set(EXECUTABLE ${EXEC_NAME}.elf)
add_executable(${EXECUTABLE} ${STARTUP_SOURCE}.s)

# Link libraries
target_link_directories(${EXECUTABLE} PRIVATE ${LINK_DIRS})
target_link_libraries(${EXECUTABLE} PRIVATE ${LIBS} SRC_OBJS)

# Target specific link options
target_link_options(${EXECUTABLE} PRIVATE
        ${LINKER_OPTS}
        LINKER:-Map=${CMAKE_BINARY_DIR}/${EXEC_NAME}.map
        -T${LINKER_SCRIPT}${EXEC_SUFFIX}.ld)

# Print executable size and create hex and bin files
add_custom_command(TARGET ${EXECUTABLE}
        POST_BUILD
        COMMAND ${CMAKE_SIZE_UTIL} ${EXECUTABLE}
        COMMAND ${CMAKE_OBJCOPY} -O ihex ${EXECUTABLE} ${EXEC_NAME}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary ${EXECUTABLE} ${EXEC_NAME}.bin
        VERBATIM)
