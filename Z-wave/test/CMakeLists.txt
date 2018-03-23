add_executable(wavingz-test wavingz-test.cpp)
target_link_libraries(wavingz-test ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES} wavingz)
add_test(NAME test-wavingz COMMAND $<TARGET_FILE:wavingz-test>)
