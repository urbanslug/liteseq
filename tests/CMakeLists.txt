# Create the test executable
add_executable(test_liteseq
  main_tests.cc  # Replace this with your actual test source files
)

target_link_libraries(test_liteseq
  PRIVATE
  gtest_main
  liteseq  # Replace with your actual target
)

# Discover GoogleTest tests
include(GoogleTest)
gtest_discover_tests(test_liteseq)
