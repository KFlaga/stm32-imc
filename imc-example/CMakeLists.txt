set(CMAKE_EXE_LINKER_FLAGS "-T${CMAKE_CURRENT_SOURCE_DIR}/stm32_flash.ld ${CMAKE_EXE_LINKER_FLAGS}")

add_executable(
    imc-example
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/startup_stm32f10x_md.s"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/stm32f1xx_it.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/stm32f1xx_it.h"
)

target_link_libraries(imc-example stm32-imc stm32-peripherals cmsis-stm32f10x)

ensure_is_one_of(IMC_DEVICE_ROLE "Master;Slave" "Master")
if(${IMC_DEVICE_ROLE} STREQUAL "Slave")
    target_compile_definitions(imc-example PUBLIC BUILD_SLAVE_DEVICE)
endif()

add_custom_target(imc-example.hex DEPENDS imc-example COMMAND ${CMAKE_OBJCOPY} -Oihex imc-example.elf imc-example.hex WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
add_custom_target(imc-example.bin DEPENDS imc-example COMMAND ${CMAKE_OBJCOPY} -Obinary imc-example.elf imc-example.bin WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
