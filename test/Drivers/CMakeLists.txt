set(LIB stm32_hal_drivers)

file(GLOB LIB_SRCS CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${MCU_FAMILY}_HAL_Driver/Src/*.c)

add_library(${LIB} STATIC ${LIB_SRCS})
set_target_properties(${LIB} PROPERTIES COMPILE_FLAGS -w)

target_compile_definitions(${LIB} PRIVATE USE_HAL_DRIVER)

target_include_directories(${LIB}
    SYSTEM PUBLIC
    ${PROJECT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_LIST_DIR}/${MCU_FAMILY}_HAL_Driver/Inc
    ${CMAKE_CURRENT_LIST_DIR}/CMSIS/Device/ST/${MCU_FAMILY}/Include
    ${CMAKE_CURRENT_LIST_DIR}/CMSIS/Include
    )
