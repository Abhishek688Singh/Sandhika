# CMake generated Testfile for 
# Source directory: /home/abhishek/game/hrm
# Build directory: /home/abhishek/game/hrm/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[config_manager_test]=] "/home/abhishek/game/hrm/build/config_manager_test")
set_tests_properties([=[config_manager_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/abhishek/game/hrm/CMakeLists.txt;90;add_test;/home/abhishek/game/hrm/CMakeLists.txt;0;")
add_test([=[reminder_scheduler_test]=] "/home/abhishek/game/hrm/build/reminder_scheduler_test")
set_tests_properties([=[reminder_scheduler_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/abhishek/game/hrm/CMakeLists.txt;91;add_test;/home/abhishek/game/hrm/CMakeLists.txt;0;")
add_test([=[stats_manager_test]=] "/home/abhishek/game/hrm/build/stats_manager_test")
set_tests_properties([=[stats_manager_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/abhishek/game/hrm/CMakeLists.txt;92;add_test;/home/abhishek/game/hrm/CMakeLists.txt;0;")
add_test([=[command_executor_test]=] "/home/abhishek/game/hrm/build/command_executor_test")
set_tests_properties([=[command_executor_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/abhishek/game/hrm/CMakeLists.txt;93;add_test;/home/abhishek/game/hrm/CMakeLists.txt;0;")
add_test([=[idle_detector_test]=] "/home/abhishek/game/hrm/build/idle_detector_test")
set_tests_properties([=[idle_detector_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/abhishek/game/hrm/CMakeLists.txt;94;add_test;/home/abhishek/game/hrm/CMakeLists.txt;0;")
add_test([=[health_score_test]=] "/home/abhishek/game/hrm/build/health_score_test")
set_tests_properties([=[health_score_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/abhishek/game/hrm/CMakeLists.txt;95;add_test;/home/abhishek/game/hrm/CMakeLists.txt;0;")
add_test([=[gui_tests]=] "/home/abhishek/game/hrm/build/gui_tests")
set_tests_properties([=[gui_tests]=] PROPERTIES  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" _BACKTRACE_TRIPLES "/home/abhishek/game/hrm/CMakeLists.txt;96;add_test;/home/abhishek/game/hrm/CMakeLists.txt;0;")
add_test([=[fullscreen_detector_test]=] "/home/abhishek/game/hrm/build/fullscreen_detector_test")
set_tests_properties([=[fullscreen_detector_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/abhishek/game/hrm/CMakeLists.txt;97;add_test;/home/abhishek/game/hrm/CMakeLists.txt;0;")
subdirs("_deps/yaml-cpp-build")
