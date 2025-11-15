#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Match.hpp"
#include "persistence/repository/MatchRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "delegate/MatchDelegate.hpp"
#include "domain/Utilities.hpp"

class MatchRepositoryMock : public MatchRepository {
public:
    MatchRepositoryMock() : MatchRepository(nullptr) {}

    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>), FindByTournamentId, (const std::string_view& tournamentId), (override));
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>), FindPlayedMatchesByTournamentId, (const std::string_view& tournamentId), (override));
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>), FindPendingMatchesByTournamentId, (const std::string_view& tournamentId), (override));
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Match>, std::string>), ReadById, (const std::string& id), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), Update, (const std::string& id, const domain::Match& match), (override));
};

class TournamentRepositoryMock3 : public TournamentRepository {
public:
    TournamentRepositoryMock3() : TournamentRepository(nullptr) {}

    MOCK_METHOD((std::expected<std::shared_ptr<domain::Tournament>, std::string>), ReadById, (std::string id), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), Update, (std::string id, const domain::Tournament & entity), (override));
};

class GroupRepositoryMock2 : public GroupRepository {
public:
    GroupRepositoryMock2() : GroupRepository(nullptr) {}

    
};

class QueueMessageProducerMock2 : public QueueMessageProducer {
public:
    QueueMessageProducerMock2(): QueueMessageProducer(nullptr) {}

    MOCK_METHOD(void, SendMessage, (const std::string_view& message, const std::string_view& queue), (override));
};

class MatchDelegateTest : public ::testing::Test{
protected:
    std::shared_ptr<MatchRepositoryMock> matchRepositoryMock;
    std::shared_ptr<TournamentRepositoryMock3> tournamentRepositoryMock3;
    std::shared_ptr<GroupRepositoryMock2> groupRepositoryMock2;
    std::shared_ptr<QueueMessageProducerMock2> producerMock2;
    std::shared_ptr<MatchDelegate> matchDelegate;

    void SetUp() override {
        matchRepositoryMock = std::make_shared<MatchRepositoryMock>();
        tournamentRepositoryMock3 = std::make_shared<TournamentRepositoryMock3>();
        groupRepositoryMock2 = std::make_shared<GroupRepositoryMock2>();
        producerMock2 = std::make_shared<QueueMessageProducerMock2>();
        matchDelegate = std::make_shared<MatchDelegate>(MatchDelegate(matchRepositoryMock, tournamentRepositoryMock3, groupRepositoryMock2, producerMock2));
    }

    // TearDown() function
    void TearDown() override {
        // teardown code comes here
    }

};

TEST_F(MatchDelegateTest, GetMatchesSuccessTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, FindPlayedMatchesByTournamentId(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock, FindPendingMatchesByTournamentId(::testing::_))
        .Times(0);

    std::string capturedTournamentId;

    std::vector<std::shared_ptr<domain::Match>> existingMatches;
    for (int i = 0; i < 2; i++) {
        nlohmann::json matchData = {
            {"id", "match-id-" + std::to_string(i)},
            {"round", "regular"},
            {"tournamentId", "tournament-id-" + std::to_string(i)},
            {"home", {
                {"id", "team-" + std::to_string(i * 2) + "-id"},
                {"name", "Team " + std::to_string(i * 2)}
            }},
            {"visitor", {
                {"id", "team-" + std::to_string(i * 2 + 1) + "-id"},
                {"name", "Team " + std::to_string(i * 2 + 1)}
            }}
        };
        auto match = std::make_shared<domain::Match>(matchData);
        existingMatches.push_back(match);
    }

    EXPECT_CALL(*matchRepositoryMock, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(existingMatches))
            )
        );

    std::string_view tournamentId = "tournament-id";
    std::optional<std::string> filter;
    auto response = matchDelegate->GetMatches(tournamentId, filter);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(response.value().size(), 2);
    EXPECT_EQ(response.value()[0]->Id(), "match-id-0");
    EXPECT_EQ(response.value()[0]->Round(), domain::RoundType::REGULAR);
    EXPECT_EQ(response.value()[0]->TournamentId(), "tournament-id-0");
    EXPECT_EQ(response.value()[0]->getHome().id, "team-0-id");
    EXPECT_EQ(response.value()[0]->getHome().name, "Team 0");
    EXPECT_EQ(response.value()[0]->getVisitor().id, "team-1-id");
    EXPECT_EQ(response.value()[0]->getVisitor().name, "Team 1");
    EXPECT_EQ(response.value()[1]->Id(), "match-id-1");
    EXPECT_EQ(response.value()[1]->Round(), domain::RoundType::REGULAR);
    EXPECT_EQ(response.value()[1]->TournamentId(), "tournament-id-1");
    EXPECT_EQ(response.value()[1]->getHome().id, "team-2-id");
    EXPECT_EQ(response.value()[1]->getHome().name, "Team 2");
    EXPECT_EQ(response.value()[1]->getVisitor().id, "team-3-id");
    EXPECT_EQ(response.value()[1]->getVisitor().name, "Team 3");
}

TEST_F(MatchDelegateTest, GetMatchesEmptyTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, FindPlayedMatchesByTournamentId(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock, FindPendingMatchesByTournamentId(::testing::_))
        .Times(0);

    std::string capturedTournamentId;

    std::vector<std::shared_ptr<domain::Match>> existingMatches;

    EXPECT_CALL(*matchRepositoryMock, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(existingMatches))
            )
        );

    std::string_view tournamentId = "tournament-id";
    std::optional<std::string> filter;
    auto response = matchDelegate->GetMatches(tournamentId, filter);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_TRUE(response.value().empty());
}

TEST_F(MatchDelegateTest, GetMatchesPlayedFilterTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedTournamentId;

    std::vector<std::shared_ptr<domain::Match>> existingMatches;
    for (int i = 0; i < 2; i++) {
        nlohmann::json matchData = {
            {"id", "match-id-" + std::to_string(i)},
            {"round", "regular"},
            {"tournamentId", "tournament-id-" + std::to_string(i)},
            {"home", {
                {"id", "team-" + std::to_string(i * 2) + "-id"},
                {"name", "Team " + std::to_string(i * 2)}
            }},
            {"visitor", {
                {"id", "team-" + std::to_string(i * 2 + 1) + "-id"},
                {"name", "Team " + std::to_string(i * 2 + 1)}
            }}
        };
        auto match = std::make_shared<domain::Match>(matchData);
        existingMatches.push_back(match);
    }

    EXPECT_CALL(*matchRepositoryMock, FindPlayedMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(existingMatches))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, FindPendingMatchesByTournamentId(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock, FindByTournamentId(::testing::_))
        .Times(0);

    std::string_view tournamentId = "tournament-id";
    std::optional<std::string> filter = "played";
    auto response = matchDelegate->GetMatches(tournamentId, filter);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(response.value().size(), 2);
    EXPECT_EQ(response.value()[0]->Id(), "match-id-0");
    EXPECT_EQ(response.value()[0]->Round(), domain::RoundType::REGULAR);
    EXPECT_EQ(response.value()[0]->TournamentId(), "tournament-id-0");
    EXPECT_EQ(response.value()[0]->getHome().id, "team-0-id");
    EXPECT_EQ(response.value()[0]->getHome().name, "Team 0");
    EXPECT_EQ(response.value()[0]->getVisitor().id, "team-1-id");
    EXPECT_EQ(response.value()[0]->getVisitor().name, "Team 1");
    EXPECT_EQ(response.value()[1]->Id(), "match-id-1");
    EXPECT_EQ(response.value()[1]->Round(), domain::RoundType::REGULAR);
    EXPECT_EQ(response.value()[1]->TournamentId(), "tournament-id-1");
    EXPECT_EQ(response.value()[1]->getHome().id, "team-2-id");
    EXPECT_EQ(response.value()[1]->getHome().name, "Team 2");
    EXPECT_EQ(response.value()[1]->getVisitor().id, "team-3-id");
    EXPECT_EQ(response.value()[1]->getVisitor().name, "Team 3");
}

TEST_F(MatchDelegateTest, GetMatchesPendingFilterTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, FindPlayedMatchesByTournamentId(::testing::_))
        .Times(0);

    std::string capturedTournamentId;

    std::vector<std::shared_ptr<domain::Match>> existingMatches;
    for (int i = 0; i < 2; i++) {
        nlohmann::json matchData = {
            {"id", "match-id-" + std::to_string(i)},
            {"round", "regular"},
            {"tournamentId", "tournament-id-" + std::to_string(i)},
            {"home", {
                {"id", "team-" + std::to_string(i * 2) + "-id"},
                {"name", "Team " + std::to_string(i * 2)}
            }},
            {"visitor", {
                {"id", "team-" + std::to_string(i * 2 + 1) + "-id"},
                {"name", "Team " + std::to_string(i * 2 + 1)}
            }}
        };
        auto match = std::make_shared<domain::Match>(matchData);
        existingMatches.push_back(match);
    }

    EXPECT_CALL(*matchRepositoryMock, FindPendingMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(existingMatches))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, FindByTournamentId(::testing::_))
        .Times(0);

    std::string_view tournamentId = "tournament-id";
    std::optional<std::string> filter = "pending";
    auto response = matchDelegate->GetMatches(tournamentId, filter);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(response.value().size(), 2);
    EXPECT_EQ(response.value()[0]->Id(), "match-id-0");
    EXPECT_EQ(response.value()[0]->Round(), domain::RoundType::REGULAR);
    EXPECT_EQ(response.value()[0]->TournamentId(), "tournament-id-0");
    EXPECT_EQ(response.value()[0]->getHome().id, "team-0-id");
    EXPECT_EQ(response.value()[0]->getHome().name, "Team 0");
    EXPECT_EQ(response.value()[0]->getVisitor().id, "team-1-id");
    EXPECT_EQ(response.value()[0]->getVisitor().name, "Team 1");
    EXPECT_EQ(response.value()[1]->Id(), "match-id-1");
    EXPECT_EQ(response.value()[1]->Round(), domain::RoundType::REGULAR);
    EXPECT_EQ(response.value()[1]->TournamentId(), "tournament-id-1");
    EXPECT_EQ(response.value()[1]->getHome().id, "team-2-id");
    EXPECT_EQ(response.value()[1]->getHome().name, "Team 2");
    EXPECT_EQ(response.value()[1]->getVisitor().id, "team-3-id");
    EXPECT_EQ(response.value()[1]->getVisitor().name, "Team 3");
}

TEST_F(MatchDelegateTest, GetMatchesFailTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, FindPlayedMatchesByTournamentId(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock, FindPendingMatchesByTournamentId(::testing::_))
        .Times(0);

    std::string capturedTournamentId;
    EXPECT_CALL(*matchRepositoryMock, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    std::string_view tournamentId = "tournament-id";
    std::optional<std::string> filter;
    auto response = matchDelegate->GetMatches(tournamentId, filter);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Database connection failed");
}

TEST_F(MatchDelegateTest, GetMatchesPlayedFilterFailTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedTournamentId;
    EXPECT_CALL(*matchRepositoryMock, FindPlayedMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, FindPendingMatchesByTournamentId(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock, FindByTournamentId(::testing::_))
        .Times(0);

    std::string_view tournamentId = "tournament-id";
    std::optional<std::string> filter = "played";
    auto response = matchDelegate->GetMatches(tournamentId, filter);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Database connection failed");
}

TEST_F(MatchDelegateTest, GetMatchesPendingFilterFailTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, FindPlayedMatchesByTournamentId(::testing::_))
        .Times(0);

    std::string capturedTournamentId;
    EXPECT_CALL(*matchRepositoryMock, FindPendingMatchesByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, FindByTournamentId(::testing::_))
        .Times(0);

    std::string_view tournamentId = "tournament-id";
    std::optional<std::string> filter = "pending";
    auto response = matchDelegate->GetMatches(tournamentId, filter);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Database connection failed");
}

TEST_F(MatchDelegateTest, GetMatchesTournamentNotFoundTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::unexpected<std::string>("Tournament not found"))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, FindPlayedMatchesByTournamentId(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock, FindPendingMatchesByTournamentId(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock, FindByTournamentId(::testing::_))
        .Times(0);

    std::string_view tournamentId = "tournament-id";
    std::optional<std::string> filter;
    auto response = matchDelegate->GetMatches(tournamentId, filter);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Tournament not found");
}

TEST_F(MatchDelegateTest, GetMatchSuccessTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedMatchId;
    nlohmann::json matchData = {
        {"id", "match-id-0"},
        {"round", "regular"},
        {"tournamentId", "tournament-id"},
        {"home", {
            {"id", "team-0-id"},
            {"name", "Team 0"}
        }},
        {"visitor", {
            {"id", "team-1-id"},
            {"name", "Team 1"}
        }}
    };
    auto match = std::make_shared<domain::Match>(matchData);

    EXPECT_CALL(*matchRepositoryMock, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchId),
                testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(match))
            )
        );

    std::string_view tournamentId = "tournament-id";
    std::string_view matchId = "match-id-0";
    auto response = matchDelegate->GetMatch(tournamentId, matchId);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedMatchId, matchId);
    EXPECT_TRUE(response.has_value());
    EXPECT_EQ(response.value()->Id(), "match-id-0");
    EXPECT_EQ(response.value()->Round(), domain::RoundType::REGULAR);
    EXPECT_EQ(response.value()->TournamentId(), "tournament-id");
    EXPECT_EQ(response.value()->getHome().id, "team-0-id");
    EXPECT_EQ(response.value()->getHome().name, "Team 0");
    EXPECT_EQ(response.value()->getVisitor().id, "team-1-id");
    EXPECT_EQ(response.value()->getVisitor().name, "Team 1");
}

TEST_F(MatchDelegateTest, GetMatchFailTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedMatchId;
    EXPECT_CALL(*matchRepositoryMock, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchId),
                testing::Return(std::unexpected<std::string>("Selection fail"))
            )
        );

    std::string_view tournamentId = "tournament-id";
    std::string_view matchId = "match-id-0";
    auto response = matchDelegate->GetMatch(tournamentId, matchId);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedMatchId, matchId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Selection fail");
}

TEST_F(MatchDelegateTest, GetMatchTournamentNotFoundTest) {
    std::string capturedTournamentIdTournamentRepo;
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::unexpected<std::string>("Tournament not found"))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, ReadById(::testing::_))
        .Times(0);

    std::string_view tournamentId = "tournament-id";
    std::string_view matchId = "match-id-0";
    auto response = matchDelegate->GetMatch(tournamentId, matchId);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Tournament not found");
}

TEST_F(MatchDelegateTest, GetMatchWrongMatchTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedMatchId;
    nlohmann::json matchData = {
        {"id", "match-id-0"},
        {"round", "regular"},
        {"tournamentId", "tournament2-id"},
        {"home", {
            {"id", "team-0-id"},
            {"name", "Team 0"}
        }},
        {"visitor", {
            {"id", "team-1-id"},
            {"name", "Team 1"}
        }}
    };
    auto match = std::make_shared<domain::Match>(matchData);

    EXPECT_CALL(*matchRepositoryMock, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchId),
                testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(match))
            )
        );

    std::string_view tournamentId = "tournament-id";
    std::string_view matchId = "match-id-0";
    auto response = matchDelegate->GetMatch(tournamentId, matchId);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedMatchId, matchId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Match does not belong to the specified tournament");
}

TEST_F(MatchDelegateTest, UpdateMatchScoreSuccessTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedMatchIdRead;
    nlohmann::json matchData = {
        {"id", "match-id-0"},
        {"round", "regular"},
        {"tournamentId", "tournament-id"},
        {"home", {
            {"id", "team-0-id"},
            {"name", "Team 0"}
        }},
        {"visitor", {
            {"id", "team-1-id"},
            {"name", "Team 1"}
        }}
    };
    auto match = std::make_shared<domain::Match>(matchData);

    EXPECT_CALL(*matchRepositoryMock, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchIdRead),
                testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(match))
            )
        );

    std::string capturedMatchIdUpdate;
    domain::Match capturedMatch;
    EXPECT_CALL(*matchRepositoryMock, Update(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchIdUpdate),
                testing::SaveArg<1>(&capturedMatch),
                testing::Return(std::expected<std::string, std::string>("match-id-0"))
            )
        );

    std::string capturedMessage;
    std::string capturedQueue;
    EXPECT_CALL(*producerMock2, SendMessage(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMessage),
                testing::SaveArg<1>(&capturedQueue)
            )
        );

    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id-0";
    domain::Score score{6, 7};
    auto response = matchDelegate->UpdateMatchScore(tournamentId, matchId, score);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&producerMock2);

    nlohmann::json messageJson = nlohmann::json::parse(capturedMessage);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedMatchIdRead, matchId);
    EXPECT_EQ(capturedMatchIdUpdate, matchId);
    EXPECT_EQ(capturedMatch.Id(), "match-id-0");
    EXPECT_EQ(capturedMatch.Round(), domain::RoundType::REGULAR);
    EXPECT_EQ(capturedMatch.TournamentId(), "tournament-id");
    EXPECT_EQ(capturedMatch.getHome().id, "team-0-id");
    EXPECT_EQ(capturedMatch.getHome().name, "Team 0");
    EXPECT_EQ(capturedMatch.getVisitor().id, "team-1-id");
    EXPECT_EQ(capturedMatch.getVisitor().name, "Team 1");
    EXPECT_EQ(capturedMatch.MatchScore().value().homeTeamScore, score.homeTeamScore);
    EXPECT_EQ(capturedMatch.MatchScore().value().visitorTeamScore, score.visitorTeamScore);
    EXPECT_EQ(messageJson["tournamentId"], tournamentId);
    EXPECT_EQ(messageJson["matchId"], matchId);
    EXPECT_EQ(messageJson["homeTeamId"], "team-0-id");
    EXPECT_EQ(messageJson["visitorTeamId"], "team-1-id");
    EXPECT_EQ(messageJson["homeScore"], score.homeTeamScore);
    EXPECT_EQ(messageJson["visitorScore"], score.visitorTeamScore);
    EXPECT_EQ(capturedQueue, "match.score-updated");
    EXPECT_TRUE(response.has_value());
}

TEST_F(MatchDelegateTest, UpdateMatchScorePlayoffSuccessTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedMatchIdRead;
    nlohmann::json matchData = {
        {"id", "match-id-0"},
        {"round", "championship"},
        {"tournamentId", "tournament-id"},
        {"home", {
            {"id", "team-0-id"},
            {"name", "Team 0"}
        }},
        {"visitor", {
            {"id", "team-1-id"},
            {"name", "Team 1"}
        }}
    };
    auto match = std::make_shared<domain::Match>(matchData);

    EXPECT_CALL(*matchRepositoryMock, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchIdRead),
                testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(match))
            )
        );

    std::string capturedMatchIdUpdate;
    domain::Match capturedMatch;
    EXPECT_CALL(*matchRepositoryMock, Update(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchIdUpdate),
                testing::SaveArg<1>(&capturedMatch),
                testing::Return(std::expected<std::string, std::string>("match-id-0"))
            )
        );

    std::string capturedMessage;
    std::string capturedQueue;
    EXPECT_CALL(*producerMock2, SendMessage(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMessage),
                testing::SaveArg<1>(&capturedQueue)
            )
        );

    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id-0";
    domain::Score score{6, 7};
    auto response = matchDelegate->UpdateMatchScore(tournamentId, matchId, score);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&producerMock2);

    nlohmann::json messageJson = nlohmann::json::parse(capturedMessage);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedMatchIdRead, matchId);
    EXPECT_EQ(capturedMatchIdUpdate, matchId);
    EXPECT_EQ(capturedMatch.Id(), "match-id-0");
    EXPECT_EQ(capturedMatch.Round(), domain::RoundType::CHAMPIONSHIP);
    EXPECT_EQ(capturedMatch.TournamentId(), "tournament-id");
    EXPECT_EQ(capturedMatch.getHome().id, "team-0-id");
    EXPECT_EQ(capturedMatch.getHome().name, "Team 0");
    EXPECT_EQ(capturedMatch.getVisitor().id, "team-1-id");
    EXPECT_EQ(capturedMatch.getVisitor().name, "Team 1");
    EXPECT_EQ(capturedMatch.MatchScore().value().homeTeamScore, score.homeTeamScore);
    EXPECT_EQ(capturedMatch.MatchScore().value().visitorTeamScore, score.visitorTeamScore);
    EXPECT_EQ(messageJson["tournamentId"], tournamentId);
    EXPECT_EQ(messageJson["matchId"], matchId);
    EXPECT_EQ(messageJson["homeTeamId"], "team-0-id");
    EXPECT_EQ(messageJson["visitorTeamId"], "team-1-id");
    EXPECT_EQ(messageJson["homeScore"], score.homeTeamScore);
    EXPECT_EQ(messageJson["visitorScore"], score.visitorTeamScore);
    EXPECT_EQ(capturedQueue, "match.score-updated");
    EXPECT_TRUE(response.has_value());
}

TEST_F(MatchDelegateTest, UpdateMatchScoreInvalidScoreTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedMatchIdRead;
    nlohmann::json matchData = {
        {"id", "match-id-0"},
        {"round", "regular"},
        {"tournamentId", "tournament-id"},
        {"home", {
            {"id", "team-0-id"},
            {"name", "Team 0"}
        }},
        {"visitor", {
            {"id", "team-1-id"},
            {"name", "Team 1"}
        }}
    };
    auto match = std::make_shared<domain::Match>(matchData);

    EXPECT_CALL(*matchRepositoryMock, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchIdRead),
                testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(match))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, Update(::testing::_, ::testing::_))
        .Times(0);

    EXPECT_CALL(*producerMock2, SendMessage(::testing::_, ::testing::_))
        .Times(0);

    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id-0";
    domain::Score score{6, 11};
    auto response = matchDelegate->UpdateMatchScore(tournamentId, matchId, score);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&producerMock2);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedMatchIdRead, matchId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Invalid score for this tournament format and round");
}

TEST_F(MatchDelegateTest, UpdateMatchScoreInvalidTieTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedMatchIdRead;
    nlohmann::json matchData = {
        {"id", "match-id-0"},
        {"round", "championship"},
        {"tournamentId", "tournament-id"},
        {"home", {
            {"id", "team-0-id"},
            {"name", "Team 0"}
        }},
        {"visitor", {
            {"id", "team-1-id"},
            {"name", "Team 1"}
        }}
    };
    auto match = std::make_shared<domain::Match>(matchData);

    EXPECT_CALL(*matchRepositoryMock, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchIdRead),
                testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(match))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, Update(::testing::_, ::testing::_))
        .Times(0);

    EXPECT_CALL(*producerMock2, SendMessage(::testing::_, ::testing::_))
        .Times(0);

    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id-0";
    domain::Score score{6, 6};
    auto response = matchDelegate->UpdateMatchScore(tournamentId, matchId, score);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&producerMock2);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedMatchIdRead, matchId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Invalid score for this tournament format and round");
}

TEST_F(MatchDelegateTest, UpdateMatchScoreFinalizedTournamentTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedMatchIdRead;
    nlohmann::json matchData = {
        {"id", "match-id-0"},
        {"round", "super bowl"},
        {"tournamentId", "tournament-id"},
        {"home", {
            {"id", "team-0-id"},
            {"name", "Team 0"}
        }},
        {"visitor", {
            {"id", "team-1-id"},
            {"name", "Team 1"}
        }}
    };
    auto match = std::make_shared<domain::Match>(matchData);

    EXPECT_CALL(*matchRepositoryMock, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchIdRead),
                testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(match))
            )
        );

    std::string capturedMatchIdUpdate;
    domain::Match capturedMatch;
    EXPECT_CALL(*matchRepositoryMock, Update(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchIdUpdate),
                testing::SaveArg<1>(&capturedMatch),
                testing::Return(std::expected<std::string, std::string>("match-id-0"))
            )
        );

    std::string capturedMessage;
    std::string capturedQueue;
    EXPECT_CALL(*producerMock2, SendMessage(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMessage),
                testing::SaveArg<1>(&capturedQueue)
            )
        );

    std::string capturedTournamentIdTournamentRepo2;
    domain::Tournament capturedTournament;
    EXPECT_CALL(*tournamentRepositoryMock3, Update(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo2),
                testing::SaveArg<1>(&capturedTournament),
                testing::Return(std::expected<std::string, std::string>("tournament-id"))
            )
        );

    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id-0";
    domain::Score score{6, 7};
    auto response = matchDelegate->UpdateMatchScore(tournamentId, matchId, score);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&producerMock2);

    nlohmann::json messageJson = nlohmann::json::parse(capturedMessage);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedMatchIdRead, matchId);
    EXPECT_EQ(capturedMatchIdUpdate, matchId);
    EXPECT_EQ(capturedMatch.Id(), "match-id-0");
    EXPECT_EQ(capturedMatch.Round(), domain::RoundType::SUPERBOWL);
    EXPECT_EQ(capturedMatch.TournamentId(), "tournament-id");
    EXPECT_EQ(capturedMatch.getHome().id, "team-0-id");
    EXPECT_EQ(capturedMatch.getHome().name, "Team 0");
    EXPECT_EQ(capturedMatch.getVisitor().id, "team-1-id");
    EXPECT_EQ(capturedMatch.getVisitor().name, "Team 1");
    EXPECT_EQ(capturedMatch.MatchScore().value().homeTeamScore, score.homeTeamScore);
    EXPECT_EQ(capturedMatch.MatchScore().value().visitorTeamScore, score.visitorTeamScore);
    EXPECT_EQ(messageJson["tournamentId"], tournamentId);
    EXPECT_EQ(messageJson["matchId"], matchId);
    EXPECT_EQ(messageJson["homeTeamId"], "team-0-id");
    EXPECT_EQ(messageJson["visitorTeamId"], "team-1-id");
    EXPECT_EQ(messageJson["homeScore"], score.homeTeamScore);
    EXPECT_EQ(messageJson["visitorScore"], score.visitorTeamScore);
    EXPECT_EQ(capturedQueue, "match.score-updated");
    EXPECT_EQ(capturedTournamentIdTournamentRepo2, tournamentId);
    EXPECT_EQ(capturedTournament.Id(), tournament->Id());
    EXPECT_EQ(capturedTournament.Name(), tournament->Name());
    EXPECT_EQ(capturedTournament.Year(), tournament->Year());
    EXPECT_EQ(capturedTournament.Finished(), tournament->Finished());
    EXPECT_EQ(capturedTournament.Finished(), "yes");
    EXPECT_TRUE(response.has_value());
}

TEST_F(MatchDelegateTest, UpdateMatchScoreFinalizedTournamentFailTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedMatchIdRead;
    nlohmann::json matchData = {
        {"id", "match-id-0"},
        {"round", "super bowl"},
        {"tournamentId", "tournament-id"},
        {"home", {
            {"id", "team-0-id"},
            {"name", "Team 0"}
        }},
        {"visitor", {
            {"id", "team-1-id"},
            {"name", "Team 1"}
        }}
    };
    auto match = std::make_shared<domain::Match>(matchData);

    EXPECT_CALL(*matchRepositoryMock, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchIdRead),
                testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(match))
            )
        );

    std::string capturedMatchIdUpdate;
    domain::Match capturedMatch;
    EXPECT_CALL(*matchRepositoryMock, Update(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchIdUpdate),
                testing::SaveArg<1>(&capturedMatch),
                testing::Return(std::expected<std::string, std::string>("match-id-0"))
            )
        );

    std::string capturedMessage;
    std::string capturedQueue;
    EXPECT_CALL(*producerMock2, SendMessage(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMessage),
                testing::SaveArg<1>(&capturedQueue)
            )
        );

    std::string capturedTournamentIdTournamentRepo2;
    domain::Tournament capturedTournament;
    EXPECT_CALL(*tournamentRepositoryMock3, Update(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo2),
                testing::SaveArg<1>(&capturedTournament),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id-0";
    domain::Score score{6, 7};
    auto response = matchDelegate->UpdateMatchScore(tournamentId, matchId, score);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&producerMock2);

    nlohmann::json messageJson = nlohmann::json::parse(capturedMessage);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedMatchIdRead, matchId);
    EXPECT_EQ(capturedMatchIdUpdate, matchId);
    EXPECT_EQ(capturedMatch.Id(), "match-id-0");
    EXPECT_EQ(capturedMatch.Round(), domain::RoundType::SUPERBOWL);
    EXPECT_EQ(capturedMatch.TournamentId(), "tournament-id");
    EXPECT_EQ(capturedMatch.getHome().id, "team-0-id");
    EXPECT_EQ(capturedMatch.getHome().name, "Team 0");
    EXPECT_EQ(capturedMatch.getVisitor().id, "team-1-id");
    EXPECT_EQ(capturedMatch.getVisitor().name, "Team 1");
    EXPECT_EQ(capturedMatch.MatchScore().value().homeTeamScore, score.homeTeamScore);
    EXPECT_EQ(capturedMatch.MatchScore().value().visitorTeamScore, score.visitorTeamScore);
    EXPECT_EQ(messageJson["tournamentId"], tournamentId);
    EXPECT_EQ(messageJson["matchId"], matchId);
    EXPECT_EQ(messageJson["homeTeamId"], "team-0-id");
    EXPECT_EQ(messageJson["visitorTeamId"], "team-1-id");
    EXPECT_EQ(messageJson["homeScore"], score.homeTeamScore);
    EXPECT_EQ(messageJson["visitorScore"], score.visitorTeamScore);
    EXPECT_EQ(capturedQueue, "match.score-updated");
    EXPECT_EQ(capturedTournamentIdTournamentRepo2, tournamentId);
    EXPECT_EQ(capturedTournament.Id(), tournament->Id());
    EXPECT_EQ(capturedTournament.Name(), tournament->Name());
    EXPECT_EQ(capturedTournament.Year(), tournament->Year());
    EXPECT_EQ(capturedTournament.Finished(), tournament->Finished());
    EXPECT_EQ(capturedTournament.Finished(), "yes");
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Database connection failed");
}

TEST_F(MatchDelegateTest, UpdateMatchScoreFailTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedMatchIdRead;
    nlohmann::json matchData = {
        {"id", "match-id-0"},
        {"round", "regular"},
        {"tournamentId", "tournament-id"},
        {"home", {
            {"id", "team-0-id"},
            {"name", "Team 0"}
        }},
        {"visitor", {
            {"id", "team-1-id"},
            {"name", "Team 1"}
        }}
    };
    auto match = std::make_shared<domain::Match>(matchData);

    EXPECT_CALL(*matchRepositoryMock, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchIdRead),
                testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(match))
            )
        );

    std::string capturedMatchIdUpdate;
    domain::Match capturedMatch;
    EXPECT_CALL(*matchRepositoryMock, Update(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchIdUpdate),
                testing::SaveArg<1>(&capturedMatch),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    EXPECT_CALL(*producerMock2, SendMessage(::testing::_, ::testing::_))
        .Times(0);

    EXPECT_CALL(*tournamentRepositoryMock3, Update(::testing::_, ::testing::_))
        .Times(0);

    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id-0";
    domain::Score score{6, 7};
    auto response = matchDelegate->UpdateMatchScore(tournamentId, matchId, score);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&producerMock2);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedMatchIdRead, matchId);
    EXPECT_EQ(capturedMatchIdUpdate, matchId);
    EXPECT_EQ(capturedMatch.Id(), "match-id-0");
    EXPECT_EQ(capturedMatch.Round(), domain::RoundType::REGULAR);
    EXPECT_EQ(capturedMatch.TournamentId(), "tournament-id");
    EXPECT_EQ(capturedMatch.getHome().id, "team-0-id");
    EXPECT_EQ(capturedMatch.getHome().name, "Team 0");
    EXPECT_EQ(capturedMatch.getVisitor().id, "team-1-id");
    EXPECT_EQ(capturedMatch.getVisitor().name, "Team 1");
    EXPECT_EQ(capturedMatch.MatchScore().value().homeTeamScore, score.homeTeamScore);
    EXPECT_EQ(capturedMatch.MatchScore().value().visitorTeamScore, score.visitorTeamScore);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Database connection failed");
}

TEST_F(MatchDelegateTest, UpdateMatchScoreAlreadyPlayedPlayoffTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedMatchIdRead;
    nlohmann::json matchData = {
        {"id", "match-id-0"},
        {"round", "divisional"},
        {"tournamentId", "tournament-id"},
        {"home", {
            {"id", "team-0-id"},
            {"name", "Team 0"}
        }},
        {"visitor", {
            {"id", "team-1-id"},
            {"name", "Team 1"}
        }},
        {"score", {
            {"home", 5},
            {"visitor", 4}
        }}
    };
    auto match = std::make_shared<domain::Match>(matchData);

    EXPECT_CALL(*matchRepositoryMock, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchIdRead),
                testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(match))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, Update(::testing::_, ::testing::_))
        .Times(0);

    EXPECT_CALL(*producerMock2, SendMessage(::testing::_, ::testing::_))
        .Times(0);

    EXPECT_CALL(*tournamentRepositoryMock3, Update(::testing::_, ::testing::_))
        .Times(0);

    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id-0";
    domain::Score score{6, 7};
    auto response = matchDelegate->UpdateMatchScore(tournamentId, matchId, score);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&producerMock2);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedMatchIdRead, matchId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Cannot modify an already played playoff game");
}

TEST_F(MatchDelegateTest, UpdateMatchScoreMissingTeamTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedMatchIdRead;
    nlohmann::json matchData = {
        {"id", "match-id-0"},
        {"round", "regular"},
        {"tournamentId", "tournament-id"},
        {"home", {
            {"id", "team-0-id"},
            {"name", "Team 0"}
        }},
        {"visitor", {
            {"id", ""},
            {"name", ""}
        }}
    };
    auto match = std::make_shared<domain::Match>(matchData);

    EXPECT_CALL(*matchRepositoryMock, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchIdRead),
                testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(match))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, Update(::testing::_, ::testing::_))
        .Times(0);

    EXPECT_CALL(*producerMock2, SendMessage(::testing::_, ::testing::_))
        .Times(0);

    EXPECT_CALL(*tournamentRepositoryMock3, Update(::testing::_, ::testing::_))
        .Times(0);

    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id-0";
    domain::Score score{6, 7};
    auto response = matchDelegate->UpdateMatchScore(tournamentId, matchId, score);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&producerMock2);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedMatchIdRead, matchId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Match teams are not ready");
}

TEST_F(MatchDelegateTest, UpdateMatchScoreForeignMatchTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedMatchIdRead;
    nlohmann::json matchData = {
        {"id", "match-id-0"},
        {"round", "regular"},
        {"tournamentId", "tournament2-id"},
        {"home", {
            {"id", "team-0-id"},
            {"name", "Team 0"}
        }},
        {"visitor", {
            {"id", "team-1-id"},
            {"name", "team-1-id"}
        }}
    };
    auto match = std::make_shared<domain::Match>(matchData);

    EXPECT_CALL(*matchRepositoryMock, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchIdRead),
                testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(match))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, Update(::testing::_, ::testing::_))
        .Times(0);

    EXPECT_CALL(*producerMock2, SendMessage(::testing::_, ::testing::_))
        .Times(0);

    EXPECT_CALL(*tournamentRepositoryMock3, Update(::testing::_, ::testing::_))
        .Times(0);

    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id-0";
    domain::Score score{6, 7};
    auto response = matchDelegate->UpdateMatchScore(tournamentId, matchId, score);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&producerMock2);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedMatchIdRead, matchId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Match does not belong to the specified tournament");
}

TEST_F(MatchDelegateTest, UpdateMatchScoreMatchSelectionFailTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string capturedMatchIdRead;

    EXPECT_CALL(*matchRepositoryMock, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedMatchIdRead),
                testing::Return(std::unexpected<std::string>("Selection failed"))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, Update(::testing::_, ::testing::_))
        .Times(0);

    EXPECT_CALL(*producerMock2, SendMessage(::testing::_, ::testing::_))
        .Times(0);

    EXPECT_CALL(*tournamentRepositoryMock3, Update(::testing::_, ::testing::_))
        .Times(0);

    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id-0";
    domain::Score score{6, 7};
    auto response = matchDelegate->UpdateMatchScore(tournamentId, matchId, score);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&producerMock2);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_EQ(capturedMatchIdRead, matchId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Selection failed");
}

TEST_F(MatchDelegateTest, UpdateMatchScoreTournamentNotFoundTest) {
    std::string capturedTournamentIdTournamentRepo;
    EXPECT_CALL(*tournamentRepositoryMock3, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::unexpected<std::string>("Tournament not found"))
            )
        );

    EXPECT_CALL(*matchRepositoryMock, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*matchRepositoryMock, Update(::testing::_, ::testing::_))
        .Times(0);

    EXPECT_CALL(*producerMock2, SendMessage(::testing::_, ::testing::_))
        .Times(0);

    EXPECT_CALL(*tournamentRepositoryMock3, Update(::testing::_, ::testing::_))
        .Times(0);

    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id-0";
    domain::Score score{6, 7};
    auto response = matchDelegate->UpdateMatchScore(tournamentId, matchId, score);

    testing::Mock::VerifyAndClearExpectations(&matchRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&producerMock2);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Tournament not found");
}