cmake_minimum_required(VERSION 3.27.9)

if(GEN_DOCS MATCHES ON)
    set(DOXYGEN_FOUND "NO")
    set(DOXYGEN_VERSION "0")
    find_package(Doxygen
                 COMPONENTS dot
                 OPTIONAL_COMPONENTS mscgen dia)

    if(DOXYGEN_FOUND AND ${DOXYGEN_VERSION} STRGREATER_EQUAL "1.9.8")
        set(DOXYGEN_PROJECT_NAME "ThreadX Library")
        set(DOXYGEN_PROJECT_BRIEF "")
        set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/doc/doxygen)
        set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
        set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
        set(DOXYGEN_EXTRACT_ALL YES)
        set(DOXYGEN_GENERATE_TREEVIEW YES)

        file(GLOB HEADER_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/*.hpp)
        doxygen_add_docs(docs ${HEADER_FILES} ALL)
    endif()
    return()
endif()

#azure rtos variables
string(TOLOWER ${CMAKE_C_COMPILER_ID} COMPILER_ID)
set(THREADX_TOOLCHAIN ${COMPILER_ID})

if(MCU_ARCH STREQUAL cortex-m0 OR MCU_ARCH STREQUAL cortex-m0plus)
    set(THREADX_ARCH "cortex_m0")
    set(FILEX_ARCH ${THREADX_ARCH})
elseif(MCU_ARCH STREQUAL cortex-m3)
    set(THREADX_ARCH "cortex_m3")
    set(FILEX_ARCH ${THREADX_ARCH})
elseif(MCU_ARCH STREQUAL cortex-m4)
    set(THREADX_ARCH "cortex_m4")
    set(FILEX_ARCH ${THREADX_ARCH})
elseif(MCU_ARCH STREQUAL cortex-m7)
    set(THREADX_ARCH "cortex_m7")
    set(FILEX_ARCH ${THREADX_ARCH})
elseif(MCU_ARCH STREQUAL cortex-m33)
    set(THREADX_ARCH "cortex_m33")
    set(FILEX_ARCH "cortex_m4")
else()
    message(FATAL_ERROR "Unknown architecture")
endif()

set(THREADX_SUBDIR https://github.com/eclipse-threadx)
FetchContent_Declare(threadx
                     GIT_REPOSITORY ${THREADX_SUBDIR}/threadx.git
                     GIT_TAG ${THREADX_VER}
                     SYSTEM)
FetchContent_Declare(filex
                     GIT_REPOSITORY ${THREADX_SUBDIR}/filex.git
                     GIT_TAG ${FILEX_VER}
                     SYSTEM)
FetchContent_Declare(levelx
                     GIT_REPOSITORY ${THREADX_SUBDIR}/levelx.git
                     GIT_TAG ${LEVELX_VER}
                     SYSTEM)

set(TX_USER_FILE "../tx_user.h")
set(FX_USER_FILE "../fx_user.h")
set(LX_USER_FILE "../lx_user.h")

FetchContent_MakeAvailable(threadx levelx)
set(THREADX_ARCH ${FILEX_ARCH})
FetchContent_MakeAvailable(filex)

target_compile_definitions(threadx PRIVATE TX_INCLUDE_USER_DEFINE_FILE)
target_compile_definitions(filex PRIVATE FX_INCLUDE_USER_DEFINE_FILE)
target_compile_definitions(levelx PRIVATE LX_INCLUDE_USER_DEFINE_FILE)
set_target_properties(threadx filex levelx PROPERTIES COMPILE_FLAGS -w)

get_filename_component(LIB_ID ${CMAKE_CURRENT_LIST_DIR} NAME)
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
add_library(${LIB_ID} STATIC ${LIB_SOURCES})

target_include_directories(${LIB_ID} INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(${LIB_ID} PUBLIC threadx filex levelx)
