# TEST: sub module

add_test_exe    (tNet tNet.c)
test_link_libs  (tNet servkit)
create_test     (tNet)
