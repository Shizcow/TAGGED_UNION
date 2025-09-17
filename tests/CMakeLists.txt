# Collect all .cpp files in the current directory
file(GLOB TEST_SOURCES "*.cpp")

# And auto-create tests
foreach(TEST_SRC IN LISTS TEST_SOURCES)
    get_filename_component(TEST_NAME "${TEST_SRC}" NAME_WE)
    add_executable("${TEST_NAME}" "${TEST_SRC}")
    target_link_libraries("${TEST_NAME}" PRIVATE tagged_union)
    add_test(NAME "${TEST_NAME}" COMMAND "${TEST_NAME}")
endforeach()
