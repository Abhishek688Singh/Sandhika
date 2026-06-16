cmake_minimum_required(VERSION 3.21)

project(health-reminder VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

add_library(health_reminder_config STATIC
    src/config/config_manager.cpp
)

add_library(health_reminder_scheduler STATIC
    src/scheduler/reminder_scheduler.cpp
)

add_library(health_reminder_stats STATIC
    src/stats/stats_manager.cpp
)

target_include_directories(health_reminder_config
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_include_directories(health_reminder_scheduler
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_include_directories(health_reminder_stats
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(health_reminder_config
    PUBLIC
        yaml-cpp
)

target_link_libraries(health_reminder_stats
    PUBLIC
        yaml-cpp
)

if(BUILD_TESTING)
    enable_testing()

    add_executable(config_manager_test
        tests/config_manager_test.cpp
    )

    add_executable(reminder_scheduler_test
        tests/reminder_scheduler_test.cpp
    )

    add_executable(stats_manager_test
        tests/stats_manager_test.cpp
    )

    target_link_libraries(config_manager_test
        PRIVATE
            health_reminder_config
    )

    target_link_libraries(reminder_scheduler_test
        PRIVATE
            health_reminder_scheduler
    )

    target_link_libraries(stats_manager_test
        PRIVATE
            health_reminder_stats
    )

    add_test(NAME config_manager_test COMMAND config_manager_test)
    add_test(NAME reminder_scheduler_test COMMAND reminder_scheduler_test)
    add_test(NAME stats_manager_test COMMAND stats_manager_test)
endif()
