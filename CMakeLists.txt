cmake_minimum_required(VERSION 3.21)

project(sandhika VERSION 0.1.0 LANGUAGES CXX)

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

add_library(sandhika_config STATIC src/config/config_manager.cpp include/sandhika/config/config_manager.h)
add_library(sandhika_scheduler STATIC src/scheduler/reminder_scheduler.cpp include/sandhika/scheduler/reminder_scheduler.h)
add_library(sandhika_stats STATIC src/stats/stats_manager.cpp include/sandhika/stats/stats_manager.h)
add_library(sandhika_brightness STATIC src/brightness/brightness_controller.cpp include/sandhika/brightness/brightness_controller.h)
add_library(sandhika_commands STATIC src/commands/command_executor.cpp include/sandhika/commands/command_executor.h)
add_library(sandhika_suppression STATIC src/suppression/suppression_manager.cpp include/sandhika/suppression/suppression_manager.h)
add_library(sandhika_health STATIC src/health/health_score_calculator.cpp include/sandhika/health/health_score_calculator.h)
add_library(sandhika_idle STATIC src/idle/idle_detector.cpp include/sandhika/idle/idle_detector.h)
add_library(sandhika_notifications STATIC src/notifications/notification_manager.cpp src/notifications/notification_window.cpp include/sandhika/notifications/notification_manager.h include/sandhika/notifications/notification_window.h)
add_library(sandhika_strict STATIC src/strict/strict_break_window.cpp include/sandhika/strict/strict_break_window.h src/strict/strict_break_dialog.h)
add_library(sandhika_tray STATIC src/tray/system_tray_manager.cpp include/sandhika/tray/system_tray_manager.h)
add_library(sandhika_dashboard STATIC src/dashboard/dashboard_window.cpp include/sandhika/dashboard/dashboard_window.h)
add_library(sandhika_app STATIC src/app/application_controller.cpp include/sandhika/app/application_controller.h)

set(ALL_LIBS
    sandhika_app
    sandhika_dashboard
    sandhika_tray
    sandhika_strict
    sandhika_notifications
    sandhika_idle
    sandhika_health
    sandhika_suppression
    sandhika_commands
    sandhika_brightness
    sandhika_stats
    sandhika_scheduler
    sandhika_config
)

foreach(lib ${ALL_LIBS})
    target_include_directories(${lib} PUBLIC ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(${lib} PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Charts Qt6::Svg Qt6::Network yaml-cpp)
endforeach()

add_executable(sandhika src/main.cpp)
target_include_directories(sandhika PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(sandhika PRIVATE ${ALL_LIBS})

if(BUILD_TESTING)
    enable_testing()

    add_executable(config_manager_test tests/config_manager_test.cpp)
    add_executable(reminder_scheduler_test tests/reminder_scheduler_test.cpp)
    add_executable(stats_manager_test tests/stats_manager_test.cpp)
    add_executable(command_executor_test tests/command_executor_test.cpp)
    add_executable(idle_detector_test tests/idle_detector_test.cpp)
    add_executable(health_score_test tests/health_score_test.cpp)
    add_executable(gui_tests tests/gui_tests.cpp)
    add_executable(suppression_manager_test tests/suppression_manager_test.cpp)

    target_link_libraries(config_manager_test PRIVATE sandhika_config Qt6::Core)
    target_link_libraries(reminder_scheduler_test PRIVATE sandhika_scheduler Qt6::Core)
    target_link_libraries(stats_manager_test PRIVATE sandhika_stats Qt6::Core)
    target_link_libraries(command_executor_test PRIVATE sandhika_commands Qt6::Core)
    target_link_libraries(idle_detector_test PRIVATE sandhika_idle Qt6::Core)
    target_link_libraries(health_score_test PRIVATE sandhika_health Qt6::Core)
    target_link_libraries(gui_tests PRIVATE sandhika_notifications sandhika_strict sandhika_dashboard sandhika_stats sandhika_brightness sandhika_suppression sandhika_config Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Charts)
    target_link_libraries(suppression_manager_test PRIVATE sandhika_suppression sandhika_config Qt6::Core)

    add_test(NAME config_manager_test COMMAND config_manager_test)
    add_test(NAME reminder_scheduler_test COMMAND reminder_scheduler_test)
    add_test(NAME stats_manager_test COMMAND stats_manager_test)
    add_test(NAME command_executor_test COMMAND command_executor_test)
    add_test(NAME idle_detector_test COMMAND idle_detector_test)
    add_test(NAME health_score_test COMMAND health_score_test)
    add_test(NAME gui_tests COMMAND gui_tests)
    add_test(NAME suppression_manager_test COMMAND suppression_manager_test)

    set_tests_properties(gui_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

include(GNUInstallDirs)
install(TARGETS sandhika DESTINATION ${CMAKE_INSTALL_BINDIR})

set(CPACK_PACKAGE_NAME "sandhika")
set(CPACK_PACKAGE_VENDOR "Sandhika")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Sandhika Health Companion")
set(CPACK_PACKAGE_VERSION "0.1.0")
include(CPack)
