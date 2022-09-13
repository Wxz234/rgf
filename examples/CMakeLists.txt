add_executable(example-00-helloworld WIN32 "helloworld.cpp")
set_property(TARGET example-00-helloworld PROPERTY CXX_STANDARD 20)
target_include_directories(example-00-helloworld PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_directories(example-00-helloworld PUBLIC rgf)