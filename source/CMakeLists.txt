add_library(rgf SHARED "rgf.cpp")
set_property(TARGET rgf PROPERTY CXX_STANDARD 20)
target_include_directories(rgf PUBLIC "${PROJECT_SOURCE_DIR}/include")
