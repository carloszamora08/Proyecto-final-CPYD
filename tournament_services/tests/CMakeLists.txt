project(tournament_tests)

set(TEST_SOURCES
        controller/TeamControllerTest.cpp
        controller/TournamentControllerTest.cpp
        controller/GroupControllerTest.cpp
        controller/MatchControllerTest.cpp
        delegate/TournamentDelegateTest.cpp
        delegate/TeamDelegateTest.cpp
        delegate/GroupDelegateTest.cpp
        delegate/MatchDelegateTest.cpp
        delegate/MatchDelegate2Test.cpp
        cms/GroupAddTeamListenerTest.cpp
        cms/ScoreUpdateListenerTest.cpp
        ../src/controller/TeamController.cpp
        ../src/controller/TournamentController.cpp
        ../include/controller/GroupController.hpp
        ../include/controller/MatchController.hpp
        ../src/delegate/TournamentDelegate.cpp
        ../src/delegate/TeamDelegate.cpp
        ../include/delegate/GroupDelegate.hpp
        ../include/delegate/MatchDelegate.hpp
        ../../tournament_consumer/include/delegate/MatchDelegate2.hpp
        ../../tournament_common/include/domain/NFLStrategy.hpp
        ../../tournament_common/src/persistence/repository/MatchRepository.cpp
        ../../tournament_consumer/include/cms/MatchCreationListener.hpp
        ../include/delegate/IMatchDelegate.h
        ../src/delegate/MatchDelegate.cpp
        ../src/controller/MatchController.cpp
        ../../tournament_consumer/include/cms/GroupAddTeamListener.hpp
        ../../tournament_consumer/include/cms/ScoreUpdateListener.hpp
)

set(SOURCES ${TEST_SOURCES})
include_directories(
        ../include
        ../../tournament_consumer/include)

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