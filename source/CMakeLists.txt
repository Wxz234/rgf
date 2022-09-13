add_library(rgf SHARED 
	"rgf.cpp"
	"rwindow.cpp"
	"rdevice.cpp"
)
set_property(TARGET rgf PROPERTY CXX_STANDARD 20)
target_include_directories(rgf PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_definitions(rgf PUBLIC UNICODE)
target_compile_definitions(rgf PRIVATE RGF_LIBRARY)
target_link_libraries(rgf PRIVATE d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib)
