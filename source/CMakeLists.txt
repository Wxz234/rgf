add_library(D3D12MA SHARED IMPORTED)
set_property(TARGET D3D12MA PROPERTY IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/third/bin/D3D12MA.dll")
set_property(TARGET D3D12MA PROPERTY IMPORTED_IMPLIB "${PROJECT_SOURCE_DIR}/third/lib/D3D12MA.lib")

add_library(rgf SHARED 
	"rgf.cpp"
	"rwindow.cpp"
	"rdevice.cpp"
	"rpass.cpp"
	"descriptor.h"
	"cmdList.h"
)
set_property(TARGET rgf PROPERTY CXX_STANDARD 20)
target_include_directories(rgf PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(rgf PRIVATE "${PROJECT_SOURCE_DIR}/third/include")
target_compile_definitions(rgf PUBLIC UNICODE)
target_compile_definitions(rgf PRIVATE RGF_LIBRARY)
target_link_libraries(rgf PRIVATE d3d12.lib dxgi.lib dxguid.lib d3dcompiler.lib D3D12MA)
