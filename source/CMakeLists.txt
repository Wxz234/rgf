add_library(rgf STATIC "rgf.cpp" "window.cpp" "device.cpp" )
target_include_directories(rgf PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(rgf PUBLIC dxgi d3d12 dxguid)