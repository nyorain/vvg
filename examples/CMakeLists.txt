add_executable(basic basic-windows.cpp)
target_link_libraries(basic vpp vvg)

add_executable(offscreen offscreen.cpp)
target_link_libraries(offscreen vpp vvg)

file(COPY "Roboto-Regular.ttf" DESTINATION ${CMAKE_BINARY_DIR})
file(COPY "Roboto-Regular.ttf" DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
