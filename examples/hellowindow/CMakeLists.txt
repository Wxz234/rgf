add_executable(example-hellowindow WIN32 "hellowindow.cpp")

target_link_libraries(example-hellowindow PRIVATE rgf nvrhi nvrhi_d3d12)