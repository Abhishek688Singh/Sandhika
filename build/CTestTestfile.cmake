# CMake generated Testfile for 
# Source directory: /home/abhishek/game/hrm
# Build directory: /home/abhishek/game/hrm/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[config_manager_test]=] "/home/abhishek/game/hrm/build/config_manager_test")
set_tests_properties([=[config_manager_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/abhishek/game/hrm/CMakeLists.txt;91;add_test;/home/abhishek/game/hrm/CMakeLists.txt;0;")
add_test([=[reminder_scheduler_test]=] "/home/abhishek/game/hrm/build/reminder_scheduler_test")
set_tests_properties([=[reminder_scheduler_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/abhishek/game/hrm/CMakeLists.txt;92;add_test;/home/abhishek/game/hrm/CMakeLists.txt;0;")
add_test([=[stats_manager_test]=] "/home/abhishek/game/hrm/build/stats_manager_test")
set_tests_properties([=[stats_manager_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/abhishek/game/hrm/CMakeLists.txt;93;add_test;/home/abhishek/game/hrm/CMakeLists.txt;0;")
subdirs("_deps/yaml-cpp-build")
