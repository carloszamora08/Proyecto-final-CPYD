project(tournament_tests)

set(TEST_SOURCES
        controller/TeamControllerTest.cpp
        controller/TournamentControllerTest.cpp
        controller/GroupControllerTest.cpp
        delegate/TournamentDelegateTest.cpp
        delegate/GroupDelegateTest.cpp
        ../src/controller/TeamController.cpp
        ../src/controller/TournamentController.cpp
        ../include/controller/GroupController.hpp
        ../src/delegate/TournamentDelegate.cpp
        ../include/delegate/GroupDelegate.hpp
)

set(SOURCES ${TEST_SOURCES})
include_directories(../include)

find_package(GTest CONFIG REQUIRED)



add_executable(${PROJECT_NAME}_runner2
    ${TEST_SOURCES}
)

target_link_libraries(${PROJECT_NAME}_runner2 PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main
        tournament_common)

add_test(AllTestsInMain ${PROJECT_NAME}_runner2)