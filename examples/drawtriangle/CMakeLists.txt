add_executable(example-drawtriangle WIN32 "drawtriangle.cpp")
target_link_libraries(example-drawtriangle PRIVATE rgf nvrhi nvrhi_d3d12)