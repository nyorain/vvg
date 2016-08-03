add_executable(example example.cpp)
target_link_libraries(example vpp vvg)
file(COPY "Roboto-Regular.ttf" ${CMAKE_BINARY_DIR})
file(COPY "Roboto-Regular.ttf" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
