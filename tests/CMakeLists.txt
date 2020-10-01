add_custom_target(tests ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.bash $<TARGET_FILE:compiler>)
add_dependencies(tests compiler)
