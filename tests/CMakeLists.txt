enable_testing()

add_executable(
    imc-ut
    "${CMAKE_CURRENT_SOURCE_DIR}/include/tests/framework.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/framework.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/InterMcuCommunicationModuleTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MetaTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TripleBufferTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/UnorderedRingBufferTests.cpp"
)

target_link_libraries(imc-ut stm32-imc)
target_include_directories(imc-ut PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_test(
    NAME imc-ut
    COMMAND imc-ut
)

set_tests_properties(imc-ut PROPERTIES
  FAIL_REGULAR_EXPRESSION "FAILED"
)