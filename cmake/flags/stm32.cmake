# it would be nicer to use add_compile_options and add_definitions - but they are not working properly if not set from parent directory of compiled target

set(FLAGS "-mthumb -mcpu=cortex-m3 -specs=nosys.specs -fstack-usage -ffunction-sections -fdata-sections -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS} -fno-rtti -fno-exceptions -Wno-register" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2" CACHE STRING "" FORCE)
set(CMAKE_ASM_FLAGS "-mthumb -mcpu=cortex-m3" CACHE STRING "")

set(CMAKE_EXE_LINKER_FLAGS "-static -Wl,-cref,-u,Reset_Handler -Wl,--gc-sections -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group" CACHE INTERNAL "")
