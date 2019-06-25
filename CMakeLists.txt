

add_executable(allocator "allocator.c")
add_executable(test_simple "allocator.c" "tests/test_simple_alloc.c")
add_executable(test_mult "allocator.c" "tests/test_multipe_allocs.c")