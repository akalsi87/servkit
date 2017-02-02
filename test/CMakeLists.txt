# TEST: sub module

add_test_exe    (tAsserts tAsserts.c)
test_link_libs  (tAsserts servkit)
create_test     (tAsserts)

# pthread tests
add_definitions(-pthread)

add_test_exe    (tConnection tConnection.c)
test_link_libs  (tConnection servkit)
add_link_flag   (tConnection -pthread)
create_test     (tConnection)

add_test_exe    (tBgTaskManager tBgTaskManager.c)
test_link_libs  (tBgTaskManager servkit)
create_test     (tBgTaskManager)
