#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include <unordered_set>

#include "event/TeamAddEvent.hpp"
#include "event/ScoreUpdateEvent.hpp"
#include "domain/Match.hpp"
#include "persistence/repository/MatchRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "delegate/MatchDelegate2.hpp"
#include "domain/Utilities.hpp"

class MatchRepositoryMock2 : public MatchRepository {
public:
    MatchRepositoryMock2() : MatchRepository(nullptr) {}

    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>), FindByTournamentId, (const std::string_view& tournamentId), (override));
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>), FindPendingMatchesByTournamentId, (const std::string_view& tournamentId), (override));
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>), FindByTournamentIdAndRound, (const std::string_view& tournamentId, domain::RoundType round), (override));
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Match>, std::string>), ReadById, (const std::string& id), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), Create, (const domain::Match& match), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), Update, (const std::string& id, const domain::Match& match), (override));
};

class TournamentRepositoryMock4 : public TournamentRepository {
public:
    TournamentRepositoryMock4() : TournamentRepository(nullptr) {}

    MOCK_METHOD((std::expected<std::shared_ptr<domain::Tournament>, std::string>), ReadById, (std::string id), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), Update, (std::string id, const domain::Tournament & entity), (override));
};

class GroupRepositoryMock3 : public GroupRepository {
public:
    GroupRepositoryMock3() : GroupRepository(nullptr) {}

    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>), FindByTournamentId, (const std::string_view& tournamentId), (override));
};

class MatchDelegate2Test : public ::testing::Test{
protected:
    std::shared_ptr<MatchRepositoryMock2> matchRepositoryMock2;
    std::shared_ptr<GroupRepositoryMock3> groupRepositoryMock3;
    std::shared_ptr<TournamentRepositoryMock4> tournamentRepositoryMock4;
    std::shared_ptr<MatchDelegate2> matchDelegate2;

    void SetUp() override {
        matchRepositoryMock2 = std::make_shared<MatchRepositoryMock2>();
        groupRepositoryMock3 = std::make_shared<GroupRepositoryMock3>();
        tournamentRepositoryMock4 = std::make_shared<TournamentRepositoryMock4>();
        matchDelegate2 = std::make_shared<MatchDelegate2>(MatchDelegate2(matchRepositoryMock2, groupRepositoryMock3, tournamentRepositoryMock4));
    }

    // TearDown() function
    void TearDown() override {
        // teardown code comes here
    }

    // Helper function to create a complete tournament with groups and teams
    std::vector<std::shared_ptr<domain::Group>> CreateCompleteGroups() {
        std::vector<std::shared_ptr<domain::Group>> groups;
        
        // Create 8 groups with 4 teams each (NFL format)
        std::vector<std::string> conferences = {"AFC", "AFC", "AFC", "AFC", "NFC", "NFC", "NFC", "NFC"};
        
        for (int i = 0; i < 8; i++) {
            nlohmann::json groupData = {
                {"id", "group-id-" + std::to_string(i)},
                {"name", "Group " + std::to_string(i)},
                {"region", "Region " + std::to_string(i)},
                {"conference", conferences[i]},
                {"teams", nlohmann::json::array()}
            };
            
            // Add 4 teams to each group
            for (int j = 0; j < 4; j++) {
                int teamNum = i * 4 + j;
                groupData["teams"].push_back({
                    {"id", "team-id-" + std::to_string(teamNum)},
                    {"name", "Team " + std::to_string(teamNum)}
                });
            }
            
            auto group = std::make_shared<domain::Group>(groupData);
            groups.push_back(group);
        }
        
        return groups;
    }

    // Helper function to create incomplete groups
    std::vector<std::shared_ptr<domain::Group>> CreateIncompleteGroups() {
        std::vector<std::shared_ptr<domain::Group>> groups;
        
        for (int i = 0; i < 6; i++) { // Only 6 groups instead of 8
            nlohmann::json groupData = {
                {"id", "group-id-" + std::to_string(i)},
                {"name", "Group " + std::to_string(i)},
                {"region", "Region " + std::to_string(i)},
                {"conference", "AFC"},
                {"teams", nlohmann::json::array()}
            };
            
            // Add only 3 teams instead of 4
            for (int j = 0; j < 3; j++) {
                int teamNum = i * 3 + j;
                groupData["teams"].push_back({
                    {"id", "team-id-" + std::to_string(teamNum)},
                    {"name", "Team " + std::to_string(teamNum)}
                });
            }
            
            auto group = std::make_shared<domain::Group>(groupData);
            groups.push_back(group);
        }
        
        return groups;
    }

    // Helper function to create regular season matches with scores
    std::vector<std::shared_ptr<domain::Match>> CreateRegularSeasonMatches() {
        std::vector<std::shared_ptr<domain::Match>> matches;
        
        // Create 272 regular season matches (32 teams × 17 games each ÷ 2)
        // Each team plays 17 games, so total = 32 × 17 / 2 = 272 unique matches
        
        int matchId = 0;
        
        // Generate matches ensuring each team plays exactly 17 games
        for (int teamA = 0; teamA < 32; teamA++) {
            for (int teamB = teamA + 1; teamB < 32; teamB++) {
                // Create enough matches to reach 272 total
                if (matchId >= 272) break;
                
                nlohmann::json matchData = {
                    {"id", "regular-match-" + std::to_string(matchId)},
                    {"round", "regular"},
                    {"tournamentId", "tournament-id"},
                    {"home", {
                        {"id", "team-id-" + std::to_string(teamA)},
                        {"name", "Team " + std::to_string(teamA)}
                    }},
                    {"visitor", {
                        {"id", "team-id-" + std::to_string(teamB)},
                        {"name", "Team " + std::to_string(teamB)}
                    }},
                    {"score", {
                        {"home", 1 + (matchId % 5)},  // Varying scores
                        {"visitor", 7 + (matchId % 2)}
                    }}
                };
                
                matches.push_back(std::make_shared<domain::Match>(matchData));
                matchId++;
            }
            if (matchId >= 272) break;
        }
        
        return matches;
    }
};

TEST_F(MatchDelegate2Test, ProcessTeamAdditionSuccessTournamentCompleteTest) {   
    std::string capturedTournamentIdGroup1;
    std::string capturedTournamentIdGroup2;
    auto groups = CreateCompleteGroups();
    EXPECT_CALL(*groupRepositoryMock3, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroup1),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(groups))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroup2),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(groups))
            )
        );

    std::string capturedTournamentIdTournament1;
    std::string capturedTournamentIdTournament2;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament1),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament2),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::vector<domain::Match> capturedMatches;
    EXPECT_CALL(*matchRepositoryMock2, Create(::testing::_))
        .Times(272)
        .WillRepeatedly(testing::DoAll(
            testing::Invoke([&capturedMatches](const domain::Match& match) {
                capturedMatches.push_back(match);
            }),
            testing::Return(std::expected<std::string, std::string>("generated-match-id"))
        ));
    
    TeamAddEvent teamAddEvent{"tournament-id", "group-id", "team-id"};
    matchDelegate2->ProcessTeamAddition(teamAddEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    auto getMatchKey = [](const domain::Match& match) -> std::string {
        return match.getHome().id + "-vs-" + match.getVisitor().id;
    };

    std::unordered_set<std::string> uniqueMatches;
    std::vector<std::string> duplicates;

    for (size_t i = 0; i < capturedMatches.size(); ++i) {
        std::string key = getMatchKey(capturedMatches[i]);
        
        if (uniqueMatches.find(key) != uniqueMatches.end()) {
            duplicates.push_back("Match " + std::to_string(i) + ": " + key);
        } else {
            uniqueMatches.insert(key);
        }
    }

    EXPECT_EQ(capturedTournamentIdGroup1, teamAddEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdGroup2, teamAddEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament1, teamAddEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament2, teamAddEvent.tournamentId);
    EXPECT_EQ(capturedMatches.size(), 272);
    EXPECT_TRUE(duplicates.empty());
    EXPECT_EQ(uniqueMatches.size(), 272);
}

TEST_F(MatchDelegate2Test, ProcessTeamAdditionStrategyIncompleteGroupTest) {   
    std::string capturedTournamentIdGroup1;
    std::string capturedTournamentIdGroup2;
    auto groups = CreateCompleteGroups();
    auto badGroups = CreateIncompleteGroups();
    EXPECT_CALL(*groupRepositoryMock3, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroup1),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(groups))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroup2),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(badGroups))
            )
        );

    std::string capturedTournamentIdTournament1;
    std::string capturedTournamentIdTournament2;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament1),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament2),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    EXPECT_CALL(*matchRepositoryMock2, Create(::testing::_))
        .Times(0);
    
    TeamAddEvent teamAddEvent{"tournament-id", "group-id", "team-id"};
    matchDelegate2->ProcessTeamAddition(teamAddEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdGroup1, teamAddEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdGroup2, teamAddEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament1, teamAddEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament2, teamAddEvent.tournamentId);
}

TEST_F(MatchDelegate2Test, ProcessTeamAdditionSecondGroupSelectionFailTest) {   
    std::string capturedTournamentIdGroup1;
    std::string capturedTournamentIdGroup2;
    auto groups = CreateCompleteGroups();
    EXPECT_CALL(*groupRepositoryMock3, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroup1),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(groups))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroup2),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    std::string capturedTournamentIdTournament1;
    std::string capturedTournamentIdTournament2;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament1),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament2),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    EXPECT_CALL(*matchRepositoryMock2, Create(::testing::_))
        .Times(0);
    
    TeamAddEvent teamAddEvent{"tournament-id", "group-id", "team-id"};
    matchDelegate2->ProcessTeamAddition(teamAddEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdGroup1, teamAddEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdGroup2, teamAddEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament1, teamAddEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament2, teamAddEvent.tournamentId);
}

TEST_F(MatchDelegate2Test, ProcessTeamAdditionSecondTournamentSelectionFailTest) {   
    std::string capturedTournamentIdGroup;
    auto groups = CreateCompleteGroups();
    EXPECT_CALL(*groupRepositoryMock3, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroup),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(groups))
            )
        );

    std::string capturedTournamentIdTournament1;
    std::string capturedTournamentIdTournament2;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament1),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament2),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    EXPECT_CALL(*matchRepositoryMock2, Create(::testing::_))
        .Times(0);
    
    TeamAddEvent teamAddEvent{"tournament-id", "group-id", "team-id"};
    matchDelegate2->ProcessTeamAddition(teamAddEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdGroup, teamAddEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament1, teamAddEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament2, teamAddEvent.tournamentId);
}

TEST_F(MatchDelegate2Test, ProcessTeamAdditionIncompleteTournamentTest) {   
    std::string capturedTournamentIdGroup;
    auto groups = CreateIncompleteGroups();
    EXPECT_CALL(*groupRepositoryMock3, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroup),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(groups))
            )
        );

    std::string capturedTournamentIdTournament;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    EXPECT_CALL(*matchRepositoryMock2, Create(::testing::_))
        .Times(0);
    
    TeamAddEvent teamAddEvent{"tournament-id", "group-id", "team-id"};
    matchDelegate2->ProcessTeamAddition(teamAddEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdGroup, teamAddEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament, teamAddEvent.tournamentId);
}

TEST_F(MatchDelegate2Test, ProcessTeamAdditionTournamentSelectionFailTest) {   
    std::string capturedTournamentIdGroup;
    auto groups = CreateIncompleteGroups();
    EXPECT_CALL(*groupRepositoryMock3, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroup),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(groups))
            )
        );

    std::string capturedTournamentIdTournament;
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    EXPECT_CALL(*matchRepositoryMock2, Create(::testing::_))
        .Times(0);
    
    TeamAddEvent teamAddEvent{"tournament-id", "group-id", "team-id"};
    matchDelegate2->ProcessTeamAddition(teamAddEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdGroup, teamAddEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament, teamAddEvent.tournamentId);
}

TEST_F(MatchDelegate2Test, ProcessTeamAdditionGroupSelectionFailTest) {   
    std::string capturedTournamentIdGroup;
    auto groups = CreateIncompleteGroups();
    EXPECT_CALL(*groupRepositoryMock3, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroup),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, Create(::testing::_))
        .Times(0);
    
    TeamAddEvent teamAddEvent{"tournament-id", "group-id", "team-id"};
    matchDelegate2->ProcessTeamAddition(teamAddEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdGroup, teamAddEvent.tournamentId);
}

TEST_F(MatchDelegate2Test, ProcessScoreUpdateSuccessTest) {
    std::string capturedTournamentIdMatchPending;
    std::vector<std::shared_ptr<domain::Match>> pendingMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindPendingMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchPending),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(pendingMatches))
            )
        );

    std::string capturedTournamentIdTournament1;
    std::string capturedTournamentIdTournament2;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament1),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament2),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedTournamentIdMatchRound;
    domain::RoundType capturedRoundType;
    std::vector<std::shared_ptr<domain::Match>> regularMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentIdAndRound(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchRound),
                testing::SaveArg<1>(&capturedRoundType),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(regularMatches))
            )
        );

    std::string capturedTournamentIdGroup;
    auto groups = CreateCompleteGroups();
    EXPECT_CALL(*groupRepositoryMock3, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroup),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(groups))
            )
        );

    std::string capturedTournamentIdMatch;
    auto matches = CreateRegularSeasonMatches();
    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatch),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(matches))
            )
        );

    std::vector<domain::Match> capturedMatches;
    EXPECT_CALL(*matchRepositoryMock2, Create(::testing::_))
        .Times(13)
        .WillRepeatedly(testing::DoAll(
            testing::Invoke([&capturedMatches](const domain::Match& match) {
                capturedMatches.push_back(match);
            }),
            testing::Return(std::expected<std::string, std::string>("generated-match-id"))
        ));

    std::vector<std::string> capturedMatchesIds;
    std::vector<std::shared_ptr<domain::Match>> playoffMatches;

    auto make = [&](const std::string& id, domain::RoundType round) {
        domain::Home h{ id + "-home", "Home " + id };
        domain::Visitor v{ id + "-visitor", "Visitor " + id };

        auto m = std::make_shared<domain::Match>("tournament-id", h, v, round);
        m->Id() = id;
        return m;
    };

    playoffMatches.push_back(make("wildcard-1", domain::RoundType::WILDCARD));
    playoffMatches.push_back(make("wildcard-2", domain::RoundType::WILDCARD));
    playoffMatches.push_back(make("wildcard-3", domain::RoundType::WILDCARD));
    playoffMatches.push_back(make("wildcard-4", domain::RoundType::WILDCARD));
    playoffMatches.push_back(make("wildcard-5", domain::RoundType::WILDCARD));
    playoffMatches.push_back(make("wildcard-6", domain::RoundType::WILDCARD));
    playoffMatches.push_back(make("divisional-1", domain::RoundType::DIVISIONAL));
    playoffMatches.push_back(make("divisional-2", domain::RoundType::DIVISIONAL));
    playoffMatches.push_back(make("divisional-3", domain::RoundType::DIVISIONAL));
    playoffMatches.push_back(make("divisional-4", domain::RoundType::DIVISIONAL));
    playoffMatches.push_back(make("championship-1", domain::RoundType::CHAMPIONSHIP));
    playoffMatches.push_back(make("championship-2", domain::RoundType::CHAMPIONSHIP));
    playoffMatches.push_back(make("superbowl-1", domain::RoundType::SUPERBOWL));
    
    size_t readIndex = 0;
    EXPECT_CALL(*matchRepositoryMock2, ReadById(::testing::_))
        .Times(13)
        .WillRepeatedly(testing::Invoke(
            [&](const std::string& matchId)
                -> std::expected<std::shared_ptr<domain::Match>, std::string>
            {
                capturedMatchesIds.push_back(matchId);

                return playoffMatches[readIndex++];
            }
        ));

    std::vector<std::string> capturedPlayoffMatchesIds;
    std::vector<domain::Match> capturedPlayoffMatches;
    EXPECT_CALL(*matchRepositoryMock2, Update(::testing::_, ::testing::_))
        .Times(13)
        .WillRepeatedly(testing::Invoke(
            [&](const std::string& id, const domain::Match& match)
                -> std::expected<std::string, std::string>
            {
                capturedPlayoffMatchesIds.push_back(id);
                capturedPlayoffMatches.push_back(match);

                return "generated-match-id";
            }
        ));

    ScoreUpdateEvent scoreUpdateEvent{"tournament-id", "match-id"};
    matchDelegate2->ProcessScoreUpdate(scoreUpdateEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdMatchPending, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament1, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament2, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdMatchRound, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedRoundType, domain::RoundType::WILDCARD);
    EXPECT_EQ(capturedTournamentIdGroup, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdMatch, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedMatches.size(), 13);
    EXPECT_EQ(capturedMatchesIds.size(), 13);
    EXPECT_EQ(capturedPlayoffMatchesIds.size(), 13);
    EXPECT_EQ(capturedPlayoffMatches.size(), 13);
}

TEST_F(MatchDelegate2Test, ProcessScoreUpdateFinalMatchUpdateFailTest) {
    std::string capturedTournamentIdMatchPending;
    std::vector<std::shared_ptr<domain::Match>> pendingMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindPendingMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchPending),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(pendingMatches))
            )
        );

    std::string capturedTournamentIdTournament1;
    std::string capturedTournamentIdTournament2;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament1),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament2),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedTournamentIdMatchRound;
    domain::RoundType capturedRoundType;
    std::vector<std::shared_ptr<domain::Match>> regularMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentIdAndRound(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchRound),
                testing::SaveArg<1>(&capturedRoundType),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(regularMatches))
            )
        );

    std::string capturedTournamentIdGroup;
    auto groups = CreateCompleteGroups();
    EXPECT_CALL(*groupRepositoryMock3, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroup),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(groups))
            )
        );

    std::string capturedTournamentIdMatch;
    auto matches = CreateRegularSeasonMatches();
    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatch),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(matches))
            )
        );

    std::vector<domain::Match> capturedMatches;
    EXPECT_CALL(*matchRepositoryMock2, Create(::testing::_))
        .Times(13)
        .WillRepeatedly(testing::DoAll(
            testing::Invoke([&capturedMatches](const domain::Match& match) {
                capturedMatches.push_back(match);
            }),
            testing::Return(std::expected<std::string, std::string>("generated-match-id"))
        ));

    std::vector<std::string> capturedMatchesIds;
    std::vector<std::shared_ptr<domain::Match>> playoffMatches;

    auto make = [&](const std::string& id, domain::RoundType round) {
        domain::Home h{ id + "-home", "Home " + id };
        domain::Visitor v{ id + "-visitor", "Visitor " + id };

        auto m = std::make_shared<domain::Match>("tournament-id", h, v, round);
        m->Id() = id;
        return m;
    };

    playoffMatches.push_back(make("wildcard-1", domain::RoundType::WILDCARD));
    playoffMatches.push_back(make("wildcard-2", domain::RoundType::WILDCARD));
    playoffMatches.push_back(make("wildcard-3", domain::RoundType::WILDCARD));
    playoffMatches.push_back(make("wildcard-4", domain::RoundType::WILDCARD));
    playoffMatches.push_back(make("wildcard-5", domain::RoundType::WILDCARD));
    playoffMatches.push_back(make("wildcard-6", domain::RoundType::WILDCARD));
    playoffMatches.push_back(make("divisional-1", domain::RoundType::DIVISIONAL));
    playoffMatches.push_back(make("divisional-2", domain::RoundType::DIVISIONAL));
    playoffMatches.push_back(make("divisional-3", domain::RoundType::DIVISIONAL));
    playoffMatches.push_back(make("divisional-4", domain::RoundType::DIVISIONAL));
    playoffMatches.push_back(make("championship-1", domain::RoundType::CHAMPIONSHIP));
    playoffMatches.push_back(make("championship-2", domain::RoundType::CHAMPIONSHIP));
    playoffMatches.push_back(make("superbowl-1", domain::RoundType::SUPERBOWL));
    
    size_t readIndex = 0;
    EXPECT_CALL(*matchRepositoryMock2, ReadById(::testing::_))
        .Times(13)
        .WillRepeatedly(testing::Invoke(
            [&](const std::string& matchId)
                -> std::expected<std::shared_ptr<domain::Match>, std::string>
            {
                capturedMatchesIds.push_back(matchId);

                return playoffMatches[readIndex++];
            }
        ));

    std::string capturedPlayoffMatchId;
    domain::Match capturedPlayoffMatch;
    EXPECT_CALL(*matchRepositoryMock2, Update(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedPlayoffMatchId),
                testing::SaveArg<1>(&capturedPlayoffMatch),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    ScoreUpdateEvent scoreUpdateEvent{"tournament-id", "match-id"};
    matchDelegate2->ProcessScoreUpdate(scoreUpdateEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdMatchPending, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament1, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament2, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdMatchRound, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedRoundType, domain::RoundType::WILDCARD);
    EXPECT_EQ(capturedTournamentIdGroup, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdMatch, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedMatches.size(), 13);
    EXPECT_EQ(capturedMatchesIds.size(), 13);
    EXPECT_EQ(capturedPlayoffMatchId, playoffMatches[0]->Id());
    EXPECT_EQ(capturedPlayoffMatch.Id(), playoffMatches[0]->Id());
    EXPECT_EQ(capturedPlayoffMatch.Round(), playoffMatches[0]->Round());
    EXPECT_EQ(capturedPlayoffMatch.TournamentId(), playoffMatches[0]->TournamentId());
    EXPECT_EQ(capturedPlayoffMatch.getHome().id, playoffMatches[0]->getHome().id);
    EXPECT_EQ(capturedPlayoffMatch.getVisitor().id, playoffMatches[0]->getVisitor().id);
}

TEST_F(MatchDelegate2Test, ProcessScoreUpdateFinalMatchReadFailTest) {
    std::string capturedTournamentIdMatchPending;
    std::vector<std::shared_ptr<domain::Match>> pendingMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindPendingMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchPending),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(pendingMatches))
            )
        );

    std::string capturedTournamentIdTournament1;
    std::string capturedTournamentIdTournament2;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament1),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament2),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedTournamentIdMatchRound;
    domain::RoundType capturedRoundType;
    std::vector<std::shared_ptr<domain::Match>> regularMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentIdAndRound(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchRound),
                testing::SaveArg<1>(&capturedRoundType),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(regularMatches))
            )
        );

    std::string capturedTournamentIdGroup;
    auto groups = CreateCompleteGroups();
    EXPECT_CALL(*groupRepositoryMock3, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroup),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(groups))
            )
        );

    std::string capturedTournamentIdMatch;
    auto matches = CreateRegularSeasonMatches();
    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatch),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(matches))
            )
        );

    domain::Match capturedMatch;
    EXPECT_CALL(*matchRepositoryMock2, Create(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatch),
                testing::Return(std::expected<std::string, std::string>("generated-match-id"))
            )
        );

    std::string capturedMatchesIds;    
    EXPECT_CALL(*matchRepositoryMock2, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchesIds),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    EXPECT_CALL(*matchRepositoryMock2, Update(::testing::_, ::testing::_))
        .Times(0);

    ScoreUpdateEvent scoreUpdateEvent{"tournament-id", "match-id"};
    matchDelegate2->ProcessScoreUpdate(scoreUpdateEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdMatchPending, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament1, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament2, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdMatchRound, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedRoundType, domain::RoundType::WILDCARD);
    EXPECT_EQ(capturedTournamentIdGroup, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdMatch, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedMatch.Round(), domain::RoundType::WILDCARD);
    EXPECT_NE(capturedMatchesIds, "");
}

TEST_F(MatchDelegate2Test, ProcessScoreUpdateFinalMatchCreateFailTest) {
    std::string capturedTournamentIdMatchPending;
    std::vector<std::shared_ptr<domain::Match>> pendingMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindPendingMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchPending),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(pendingMatches))
            )
        );

    std::string capturedTournamentIdTournament1;
    std::string capturedTournamentIdTournament2;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament1),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament2),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedTournamentIdMatchRound;
    domain::RoundType capturedRoundType;
    std::vector<std::shared_ptr<domain::Match>> regularMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentIdAndRound(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchRound),
                testing::SaveArg<1>(&capturedRoundType),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(regularMatches))
            )
        );

    std::string capturedTournamentIdGroup;
    auto groups = CreateCompleteGroups();
    EXPECT_CALL(*groupRepositoryMock3, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroup),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(groups))
            )
        );

    std::string capturedTournamentIdMatch;
    auto matches = CreateRegularSeasonMatches();
    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatch),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(matches))
            )
        );

    domain::Match capturedMatch;
    EXPECT_CALL(*matchRepositoryMock2, Create(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatch),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    EXPECT_CALL(*matchRepositoryMock2, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, Update(::testing::_, ::testing::_))
        .Times(0);

    ScoreUpdateEvent scoreUpdateEvent{"tournament-id", "match-id"};
    matchDelegate2->ProcessScoreUpdate(scoreUpdateEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdMatchPending, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament1, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament2, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdMatchRound, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedRoundType, domain::RoundType::WILDCARD);
    EXPECT_EQ(capturedTournamentIdGroup, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdMatch, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedMatch.Round(), domain::RoundType::WILDCARD);
}

TEST_F(MatchDelegate2Test, ProcessScoreUpdateRegularMatchesReadFailTest) {
    std::string capturedTournamentIdMatchPending;
    std::vector<std::shared_ptr<domain::Match>> pendingMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindPendingMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchPending),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(pendingMatches))
            )
        );

    std::string capturedTournamentIdTournament1;
    std::string capturedTournamentIdTournament2;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament1),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament2),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedTournamentIdMatchRound;
    domain::RoundType capturedRoundType;
    std::vector<std::shared_ptr<domain::Match>> regularMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentIdAndRound(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchRound),
                testing::SaveArg<1>(&capturedRoundType),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(regularMatches))
            )
        );

    std::string capturedTournamentIdGroup;
    auto groups = CreateCompleteGroups();
    EXPECT_CALL(*groupRepositoryMock3, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroup),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(groups))
            )
        );

    std::string capturedTournamentIdMatch;
    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatch),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    EXPECT_CALL(*matchRepositoryMock2, Create(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, Update(::testing::_, ::testing::_))
        .Times(0);

    ScoreUpdateEvent scoreUpdateEvent{"tournament-id", "match-id"};
    matchDelegate2->ProcessScoreUpdate(scoreUpdateEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdMatchPending, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament1, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament2, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdMatchRound, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedRoundType, domain::RoundType::WILDCARD);
    EXPECT_EQ(capturedTournamentIdGroup, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdMatch, scoreUpdateEvent.tournamentId);
}

TEST_F(MatchDelegate2Test, ProcessScoreUpdateGroupsReadFailTest) {
    std::string capturedTournamentIdMatchPending;
    std::vector<std::shared_ptr<domain::Match>> pendingMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindPendingMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchPending),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(pendingMatches))
            )
        );

    std::string capturedTournamentIdTournament1;
    std::string capturedTournamentIdTournament2;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament1),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament2),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedTournamentIdMatchRound;
    domain::RoundType capturedRoundType;
    std::vector<std::shared_ptr<domain::Match>> regularMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentIdAndRound(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchRound),
                testing::SaveArg<1>(&capturedRoundType),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(regularMatches))
            )
        );

    std::string capturedTournamentIdGroup;
    EXPECT_CALL(*groupRepositoryMock3, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroup),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentId(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, Create(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, Update(::testing::_, ::testing::_))
        .Times(0);

    ScoreUpdateEvent scoreUpdateEvent{"tournament-id", "match-id"};
    matchDelegate2->ProcessScoreUpdate(scoreUpdateEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdMatchPending, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament1, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament2, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdMatchRound, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedRoundType, domain::RoundType::WILDCARD);
    EXPECT_EQ(capturedTournamentIdGroup, scoreUpdateEvent.tournamentId);
}

TEST_F(MatchDelegate2Test, ProcessScoreUpdateSecondTournamentReadFailTest) {
    std::string capturedTournamentIdMatchPending;
    std::vector<std::shared_ptr<domain::Match>> pendingMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindPendingMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchPending),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(pendingMatches))
            )
        );

    std::string capturedTournamentIdTournament1;
    std::string capturedTournamentIdTournament2;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament1),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament2),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    std::string capturedTournamentIdMatchRound;
    domain::RoundType capturedRoundType;
    std::vector<std::shared_ptr<domain::Match>> regularMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentIdAndRound(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchRound),
                testing::SaveArg<1>(&capturedRoundType),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(regularMatches))
            )
        );

    EXPECT_CALL(*groupRepositoryMock3, FindByTournamentId(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentId(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, Create(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, Update(::testing::_, ::testing::_))
        .Times(0);

    ScoreUpdateEvent scoreUpdateEvent{"tournament-id", "match-id"};
    matchDelegate2->ProcessScoreUpdate(scoreUpdateEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdMatchPending, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament1, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament2, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdMatchRound, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedRoundType, domain::RoundType::WILDCARD);
}

TEST_F(MatchDelegate2Test, ProcessScoreUpdateWilcardMatchesReadFailTest) {
    std::string capturedTournamentIdMatchPending;
    std::vector<std::shared_ptr<domain::Match>> pendingMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindPendingMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchPending),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(pendingMatches))
            )
        );

    std::string capturedTournamentIdTournament;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedTournamentIdMatchRound;
    domain::RoundType capturedRoundType;
    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentIdAndRound(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchRound),
                testing::SaveArg<1>(&capturedRoundType),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    EXPECT_CALL(*groupRepositoryMock3, FindByTournamentId(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentId(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, Create(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, Update(::testing::_, ::testing::_))
        .Times(0);

    ScoreUpdateEvent scoreUpdateEvent{"tournament-id", "match-id"};
    matchDelegate2->ProcessScoreUpdate(scoreUpdateEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdMatchPending, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdMatchRound, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedRoundType, domain::RoundType::WILDCARD);
}

TEST_F(MatchDelegate2Test, ProcessScoreUpdateFirstTournamentReadFailTest) {
    std::string capturedTournamentIdMatchPending;
    std::vector<std::shared_ptr<domain::Match>> pendingMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindPendingMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchPending),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(pendingMatches))
            )
        );

    std::string capturedTournamentIdTournament;
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentIdAndRound(::testing::_, ::testing::_))
        .Times(0);

    EXPECT_CALL(*groupRepositoryMock3, FindByTournamentId(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentId(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, Create(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, Update(::testing::_, ::testing::_))
        .Times(0);

    ScoreUpdateEvent scoreUpdateEvent{"tournament-id", "match-id"};
    matchDelegate2->ProcessScoreUpdate(scoreUpdateEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdMatchPending, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament, scoreUpdateEvent.tournamentId);
}

TEST_F(MatchDelegate2Test, ProcessScoreUpdatePendingMatchesFailTest) {
    std::string capturedTournamentIdMatchPending;
    EXPECT_CALL(*matchRepositoryMock2, FindPendingMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchPending),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    std::string capturedTournamentIdTournament;
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentIdAndRound(::testing::_, ::testing::_))
        .Times(0);

    EXPECT_CALL(*groupRepositoryMock3, FindByTournamentId(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentId(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, Create(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock2, Update(::testing::_, ::testing::_))
        .Times(0);

    ScoreUpdateEvent scoreUpdateEvent{"tournament-id", "match-id"};
    matchDelegate2->ProcessScoreUpdate(scoreUpdateEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdMatchPending, scoreUpdateEvent.tournamentId);
}

TEST_F(MatchDelegate2Test, ProcessScoreUpdateAdvanceSuccessTest) {
    std::string capturedTournamentIdMatchPending;
    std::vector<std::shared_ptr<domain::Match>> pendingMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindPendingMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchPending),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(pendingMatches))
            )
        );

    std::string capturedTournamentIdTournament;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedTournamentIdMatchRound;
    domain::RoundType capturedRoundType;
    std::vector<std::shared_ptr<domain::Match>> wildcardMatches;

    auto make = [&](const std::string& id, domain::RoundType round) {
        domain::Home h{ id + "-home", "Home " + id };
        domain::Visitor v{ id + "-visitor", "Visitor " + id };

        auto m = std::make_shared<domain::Match>("tournament-id", h, v, round);
        m->Id() = id;
        return m;
    };

    wildcardMatches.push_back(make("wildcard-1", domain::RoundType::WILDCARD));
    wildcardMatches[0]->WinnerNextMatchId() = "divisional-1";
    wildcardMatches.push_back(make("wildcard-2", domain::RoundType::WILDCARD));
    wildcardMatches[1]->WinnerNextMatchId() = "divisional-2";
    wildcardMatches.push_back(make("wildcard-3", domain::RoundType::WILDCARD));
    wildcardMatches[2]->WinnerNextMatchId() = "divisional-2";
    wildcardMatches.push_back(make("wildcard-4", domain::RoundType::WILDCARD));
    wildcardMatches[3]->WinnerNextMatchId() = "divisional-3";
    wildcardMatches.push_back(make("wildcard-5", domain::RoundType::WILDCARD));
    wildcardMatches[4]->WinnerNextMatchId() = "divisional-4";
    wildcardMatches.push_back(make("wildcard-6", domain::RoundType::WILDCARD));
    wildcardMatches[5]->WinnerNextMatchId() = "divisional-4";
    
    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentIdAndRound(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchRound),
                testing::SaveArg<1>(&capturedRoundType),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(wildcardMatches))
            )
        );

    std::string capturedMatchId1;
    std::string capturedMatchId2;
    auto advancingMatch = make("wildcard-1", domain::RoundType::WILDCARD);
    advancingMatch->WinnerNextMatchId() = "divisional-1";
    advancingMatch->MatchScore() = domain::Score{8, 4};

    domain::Home h{ "", "" };
    domain::Visitor v{ "", "" };
    auto m = std::make_shared<domain::Match>("tournament-id", h, v, domain::RoundType::DIVISIONAL);
    m->Id() = "divisional-1";
    auto nextMatch = m;
    EXPECT_CALL(*matchRepositoryMock2, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchId1),
                testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(advancingMatch))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchId2),
                testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(nextMatch))
            )
        );

    std::string capturedUpdatedMatchId;
    domain::Match capturedUpdatedMatch;
    EXPECT_CALL(*matchRepositoryMock2, Update(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedUpdatedMatchId),
                testing::SaveArg<1>(&capturedUpdatedMatch),
                testing::Return(std::expected<std::string, std::string>("divisional-1"))
            )
        );

    ScoreUpdateEvent scoreUpdateEvent{"tournament-id", "wildcard-1"};
    matchDelegate2->ProcessScoreUpdate(scoreUpdateEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdMatchPending, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdMatchRound, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedRoundType, domain::RoundType::WILDCARD);
    EXPECT_EQ(capturedMatchId1, scoreUpdateEvent.matchId);
    EXPECT_EQ(capturedMatchId2, "divisional-1");
    EXPECT_EQ(capturedUpdatedMatchId, "divisional-1");
    EXPECT_EQ(capturedUpdatedMatch.getHome().id, "wildcard-1-home");
}

TEST_F(MatchDelegate2Test, ProcessScoreUpdateAdvanceUpdateFailTest) {
    std::string capturedTournamentIdMatchPending;
    std::vector<std::shared_ptr<domain::Match>> pendingMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindPendingMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchPending),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(pendingMatches))
            )
        );

    std::string capturedTournamentIdTournament;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedTournamentIdMatchRound;
    domain::RoundType capturedRoundType;
    std::vector<std::shared_ptr<domain::Match>> wildcardMatches;

    auto make = [&](const std::string& id, domain::RoundType round) {
        domain::Home h{ id + "-home", "Home " + id };
        domain::Visitor v{ id + "-visitor", "Visitor " + id };

        auto m = std::make_shared<domain::Match>("tournament-id", h, v, round);
        m->Id() = id;
        return m;
    };

    wildcardMatches.push_back(make("wildcard-1", domain::RoundType::WILDCARD));
    wildcardMatches[0]->WinnerNextMatchId() = "divisional-1";
    wildcardMatches.push_back(make("wildcard-2", domain::RoundType::WILDCARD));
    wildcardMatches[1]->WinnerNextMatchId() = "divisional-2";
    wildcardMatches.push_back(make("wildcard-3", domain::RoundType::WILDCARD));
    wildcardMatches[2]->WinnerNextMatchId() = "divisional-2";
    wildcardMatches.push_back(make("wildcard-4", domain::RoundType::WILDCARD));
    wildcardMatches[3]->WinnerNextMatchId() = "divisional-3";
    wildcardMatches.push_back(make("wildcard-5", domain::RoundType::WILDCARD));
    wildcardMatches[4]->WinnerNextMatchId() = "divisional-4";
    wildcardMatches.push_back(make("wildcard-6", domain::RoundType::WILDCARD));
    wildcardMatches[5]->WinnerNextMatchId() = "divisional-4";
    
    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentIdAndRound(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchRound),
                testing::SaveArg<1>(&capturedRoundType),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(wildcardMatches))
            )
        );

    std::string capturedMatchId1;
    std::string capturedMatchId2;
    auto advancingMatch = make("wildcard-1", domain::RoundType::WILDCARD);
    advancingMatch->WinnerNextMatchId() = "divisional-1";
    advancingMatch->MatchScore() = domain::Score{8, 4};

    domain::Home h{ "", "" };
    domain::Visitor v{ "", "" };
    auto m = std::make_shared<domain::Match>("tournament-id", h, v, domain::RoundType::DIVISIONAL);
    m->Id() = "divisional-1";
    auto nextMatch = m;
    EXPECT_CALL(*matchRepositoryMock2, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchId1),
                testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(advancingMatch))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchId2),
                testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(nextMatch))
            )
        );

    std::string capturedUpdatedMatchId;
    domain::Match capturedUpdatedMatch;
    EXPECT_CALL(*matchRepositoryMock2, Update(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedUpdatedMatchId),
                testing::SaveArg<1>(&capturedUpdatedMatch),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    ScoreUpdateEvent scoreUpdateEvent{"tournament-id", "wildcard-1"};
    matchDelegate2->ProcessScoreUpdate(scoreUpdateEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdMatchPending, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdMatchRound, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedRoundType, domain::RoundType::WILDCARD);
    EXPECT_EQ(capturedMatchId1, scoreUpdateEvent.matchId);
    EXPECT_EQ(capturedMatchId2, "divisional-1");
    EXPECT_EQ(capturedUpdatedMatchId, "divisional-1");
    EXPECT_EQ(capturedUpdatedMatch.getHome().id, "wildcard-1-home");
}

TEST_F(MatchDelegate2Test, ProcessScoreUpdateAdvanceSecondMatchReadFailTest) {
    std::string capturedTournamentIdMatchPending;
    std::vector<std::shared_ptr<domain::Match>> pendingMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindPendingMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchPending),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(pendingMatches))
            )
        );

    std::string capturedTournamentIdTournament;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedTournamentIdMatchRound;
    domain::RoundType capturedRoundType;
    std::vector<std::shared_ptr<domain::Match>> wildcardMatches;

    auto make = [&](const std::string& id, domain::RoundType round) {
        domain::Home h{ id + "-home", "Home " + id };
        domain::Visitor v{ id + "-visitor", "Visitor " + id };

        auto m = std::make_shared<domain::Match>("tournament-id", h, v, round);
        m->Id() = id;
        return m;
    };

    wildcardMatches.push_back(make("wildcard-1", domain::RoundType::WILDCARD));
    wildcardMatches[0]->WinnerNextMatchId() = "divisional-1";
    wildcardMatches.push_back(make("wildcard-2", domain::RoundType::WILDCARD));
    wildcardMatches[1]->WinnerNextMatchId() = "divisional-2";
    wildcardMatches.push_back(make("wildcard-3", domain::RoundType::WILDCARD));
    wildcardMatches[2]->WinnerNextMatchId() = "divisional-2";
    wildcardMatches.push_back(make("wildcard-4", domain::RoundType::WILDCARD));
    wildcardMatches[3]->WinnerNextMatchId() = "divisional-3";
    wildcardMatches.push_back(make("wildcard-5", domain::RoundType::WILDCARD));
    wildcardMatches[4]->WinnerNextMatchId() = "divisional-4";
    wildcardMatches.push_back(make("wildcard-6", domain::RoundType::WILDCARD));
    wildcardMatches[5]->WinnerNextMatchId() = "divisional-4";
    
    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentIdAndRound(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchRound),
                testing::SaveArg<1>(&capturedRoundType),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(wildcardMatches))
            )
        );

    std::string capturedMatchId1;
    std::string capturedMatchId2;
    auto advancingMatch = make("wildcard-1", domain::RoundType::WILDCARD);
    advancingMatch->WinnerNextMatchId() = "divisional-1";
    advancingMatch->MatchScore() = domain::Score{8, 4};

    EXPECT_CALL(*matchRepositoryMock2, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchId1),
                testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(advancingMatch))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchId2),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    EXPECT_CALL(*matchRepositoryMock2, Update(::testing::_, ::testing::_))
        .Times(0);

    ScoreUpdateEvent scoreUpdateEvent{"tournament-id", "wildcard-1"};
    matchDelegate2->ProcessScoreUpdate(scoreUpdateEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdMatchPending, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdMatchRound, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedRoundType, domain::RoundType::WILDCARD);
    EXPECT_EQ(capturedMatchId1, scoreUpdateEvent.matchId);
    EXPECT_EQ(capturedMatchId2, "divisional-1");
}

TEST_F(MatchDelegate2Test, ProcessScoreUpdateAdvanceFirstMatchReadFailTest) {
    std::string capturedTournamentIdMatchPending;
    std::vector<std::shared_ptr<domain::Match>> pendingMatches;
    EXPECT_CALL(*matchRepositoryMock2, FindPendingMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchPending),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(pendingMatches))
            )
        );

    std::string capturedTournamentIdTournament;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock4, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournament),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedTournamentIdMatchRound;
    domain::RoundType capturedRoundType;
    std::vector<std::shared_ptr<domain::Match>> wildcardMatches;

    auto make = [&](const std::string& id, domain::RoundType round) {
        domain::Home h{ id + "-home", "Home " + id };
        domain::Visitor v{ id + "-visitor", "Visitor " + id };

        auto m = std::make_shared<domain::Match>("tournament-id", h, v, round);
        m->Id() = id;
        return m;
    };

    wildcardMatches.push_back(make("wildcard-1", domain::RoundType::WILDCARD));
    wildcardMatches[0]->WinnerNextMatchId() = "divisional-1";
    wildcardMatches.push_back(make("wildcard-2", domain::RoundType::WILDCARD));
    wildcardMatches[1]->WinnerNextMatchId() = "divisional-2";
    wildcardMatches.push_back(make("wildcard-3", domain::RoundType::WILDCARD));
    wildcardMatches[2]->WinnerNextMatchId() = "divisional-2";
    wildcardMatches.push_back(make("wildcard-4", domain::RoundType::WILDCARD));
    wildcardMatches[3]->WinnerNextMatchId() = "divisional-3";
    wildcardMatches.push_back(make("wildcard-5", domain::RoundType::WILDCARD));
    wildcardMatches[4]->WinnerNextMatchId() = "divisional-4";
    wildcardMatches.push_back(make("wildcard-6", domain::RoundType::WILDCARD));
    wildcardMatches[5]->WinnerNextMatchId() = "divisional-4";
    
    EXPECT_CALL(*matchRepositoryMock2, FindByTournamentIdAndRound(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdMatchRound),
                testing::SaveArg<1>(&capturedRoundType),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(wildcardMatches))
            )
        );

    std::string capturedMatchId;
    EXPECT_CALL(*matchRepositoryMock2, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchId),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    EXPECT_CALL(*matchRepositoryMock2, Update(::testing::_, ::testing::_))
        .Times(0);

    ScoreUpdateEvent scoreUpdateEvent{"tournament-id", "wildcard-1"};
    matchDelegate2->ProcessScoreUpdate(scoreUpdateEvent);
    
    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock3);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock4);
    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdMatchPending, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdTournament, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedTournamentIdMatchRound, scoreUpdateEvent.tournamentId);
    EXPECT_EQ(capturedRoundType, domain::RoundType::WILDCARD);
    EXPECT_EQ(capturedMatchId, scoreUpdateEvent.matchId);
}