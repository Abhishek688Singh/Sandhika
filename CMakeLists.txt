cmake_minimum_required(VERSION 3.21)

project(health-reminder VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts Svg Network)

option(BUILD_TESTING "Build unit tests" ON)
option(HRM_FETCH_YAML_CPP "Fetch yaml-cpp if system package is unavailable" ON)

if(HRM_FETCH_YAML_CPP)
    include(FetchContent)
    FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG yaml-cpp-0.9.0
    )
    FetchContent_MakeAvailable(yaml-cpp)
else()
    find_package(yaml-cpp REQUIRED)
endif()

add_library(health_reminder_config STATIC src/config/config_manager.cpp include/health_reminder/config/config_manager.h)
add_library(health_reminder_scheduler STATIC src/scheduler/reminder_scheduler.cpp include/health_reminder/scheduler/reminder_scheduler.h)
add_library(health_reminder_stats STATIC src/stats/stats_manager.cpp include/health_reminder/stats/stats_manager.h)
add_library(health_reminder_brightness STATIC src/brightness/brightness_controller.cpp include/health_reminder/brightness/brightness_controller.h)
add_library(health_reminder_commands STATIC src/commands/command_executor.cpp include/health_reminder/commands/command_executor.h)
add_library(health_reminder_fullscreen STATIC src/fullscreen/fullscreen_detector.cpp include/health_reminder/fullscreen/fullscreen_detector.h)
add_library(health_reminder_health STATIC src/health/health_score_calculator.cpp include/health_reminder/health/health_score_calculator.h)
add_library(health_reminder_idle STATIC src/idle/idle_detector.cpp include/health_reminder/idle/idle_detector.h)
add_library(health_reminder_notifications STATIC src/notifications/notification_manager.cpp src/notifications/notification_window.cpp include/health_reminder/notifications/notification_manager.h include/health_reminder/notifications/notification_window.h)
add_library(health_reminder_strict STATIC src/strict/strict_break_window.cpp include/health_reminder/strict/strict_break_window.h src/strict/strict_break_dialog.h)
add_library(health_reminder_tray STATIC src/tray/system_tray_manager.cpp include/health_reminder/tray/system_tray_manager.h)
add_library(health_reminder_dashboard STATIC src/dashboard/dashboard_window.cpp include/health_reminder/dashboard/dashboard_window.h)
add_library(health_reminder_app STATIC src/app/application_controller.cpp include/health_reminder/app/application_controller.h)

set(ALL_LIBS
    health_reminder_app
    health_reminder_dashboard
    health_reminder_tray
    health_reminder_strict
    health_reminder_notifications
    health_reminder_idle
    health_reminder_health
    health_reminder_fullscreen
    health_reminder_commands
    health_reminder_brightness
    health_reminder_stats
    health_reminder_scheduler
    health_reminder_config
)

foreach(lib ${ALL_LIBS})
    target_include_directories(${lib} PUBLIC ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(${lib} PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Charts Qt6::Svg Qt6::Network yaml-cpp)
endforeach()

add_executable(health-reminder src/main.cpp)
target_include_directories(health-reminder PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(health-reminder PRIVATE ${ALL_LIBS})

if(BUILD_TESTING)
    enable_testing()

    add_executable(config_manager_test tests/config_manager_test.cpp)
    add_executable(reminder_scheduler_test tests/reminder_scheduler_test.cpp)
    add_executable(stats_manager_test tests/stats_manager_test.cpp)
    add_executable(command_executor_test tests/command_executor_test.cpp)
    add_executable(idle_detector_test tests/idle_detector_test.cpp)
    add_executable(health_score_test tests/health_score_test.cpp)
    add_executable(gui_tests tests/gui_tests.cpp)
    add_executable(fullscreen_detector_test tests/fullscreen_detector_test.cpp)

    target_link_libraries(config_manager_test PRIVATE health_reminder_config Qt6::Core)
    target_link_libraries(reminder_scheduler_test PRIVATE health_reminder_scheduler Qt6::Core)
    target_link_libraries(stats_manager_test PRIVATE health_reminder_stats Qt6::Core)
    target_link_libraries(command_executor_test PRIVATE health_reminder_commands Qt6::Core)
    target_link_libraries(idle_detector_test PRIVATE health_reminder_idle Qt6::Core)
    target_link_libraries(health_score_test PRIVATE health_reminder_health Qt6::Core)
    target_link_libraries(gui_tests PRIVATE health_reminder_notifications health_reminder_strict health_reminder_dashboard health_reminder_stats health_reminder_brightness Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Charts)
    target_link_libraries(fullscreen_detector_test PRIVATE health_reminder_fullscreen Qt6::Core)

    add_test(NAME config_manager_test COMMAND config_manager_test)
    add_test(NAME reminder_scheduler_test COMMAND reminder_scheduler_test)
    add_test(NAME stats_manager_test COMMAND stats_manager_test)
    add_test(NAME command_executor_test COMMAND command_executor_test)
    add_test(NAME idle_detector_test COMMAND idle_detector_test)
    add_test(NAME health_score_test COMMAND health_score_test)
    add_test(NAME gui_tests COMMAND gui_tests)
    add_test(NAME fullscreen_detector_test COMMAND fullscreen_detector_test)

    set_tests_properties(gui_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
