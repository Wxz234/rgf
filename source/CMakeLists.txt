add_library(RGF SHARED "RGF.cpp")
set_property(TARGET RGF PROPERTY CXX_STANDARD 20)
target_include_directories(RGF PUBLIC "${PROJECT_SOURCE_DIR}/include")
