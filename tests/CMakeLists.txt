include(GoogleTest)

add_executable(archon_tests
        EntityManagerTests.cpp)
target_link_libraries(archon_tests PRIVATE Archon::Archon GTest::gtest_main)
set_target_properties(archon_tests PROPERTIES CXX_EXTENSIONS OFF)
gtest_discover_tests(archon_tests DISCOVERY_MODE PRE_TEST)

add_executable(archon_template_api_compile_tests
        TemplateApiCompileTests.cpp)
target_link_libraries(archon_template_api_compile_tests PRIVATE Archon::Archon)
set_target_properties(archon_template_api_compile_tests PROPERTIES CXX_EXTENSIONS OFF)
