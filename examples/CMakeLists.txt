add_executable(example-00-helloworld WIN32 "helloworld.cpp")
set_property(TARGET example-00-helloworld PROPERTY CXX_STANDARD 20)
target_link_libraries(example-00-helloworld PRIVATE rgf)
add_custom_command(TARGET example-00-helloworld POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:example-00-helloworld> $<TARGET_FILE_DIR:example-00-helloworld>
  COMMAND_EXPAND_LISTS
)