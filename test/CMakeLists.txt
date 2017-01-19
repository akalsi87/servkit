# TEST: sub module

add_test_exe    (tNet tNet.c)
test_link_libs  (tNet servkit)
create_test     (tNet)

add_test_exe    (tAsserts tAsserts.c)
test_link_libs  (tAsserts servkit)
create_test     (tAsserts)
