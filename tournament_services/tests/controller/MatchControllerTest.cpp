#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <expected>

#include "domain/Match.hpp"
#include "delegate/IMatchDelegate.h"
#include "controller/MatchController.hpp"
#include "domain/Utilities.hpp"

class MatchDelegateMock : public IMatchDelegate {
public:
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>), GetMatches, (std::string_view tournamentId, std::optional<std::string> filter), (override));
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Match>, std::string>), GetMatch, (std::string_view tournamentId, std::string_view matchId), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateMatchScore, (std::string_view tournamentId, std::string_view matchId, const domain::Score& score), (override));
};

class MatchControllerTest : public ::testing::Test{
protected:
    std::shared_ptr<MatchDelegateMock> matchDelegateMock;
    std::shared_ptr<MatchController> matchController;

    void SetUp() override {
        matchDelegateMock = std::make_shared<MatchDelegateMock>();
        matchController = std::make_shared<MatchController>(MatchController(matchDelegateMock));
    }

    // TearDown() function
    void TearDown() override {
        // teardown code comes here
    }

};

TEST_F(MatchControllerTest, GetMatchesSuccessTest) {
    std::string capturedTournamentId;
    std::optional<std::string> capturedFilter;

    std::vector<std::shared_ptr<domain::Match>> matches;

    nlohmann::json body1 = {{"id", "match1-id"}, {"tournamentId", "tournament-id"}, {"round", "regular"}, {"home", {{"id", "team1-id"}, {"name", "T1"}}}, {"visitor", {{"id", "team2-id"}, {"name", "T2"}}}};
    matches.push_back(std::make_shared<domain::Match>(body1));

    nlohmann::json body2 = {{"id", "match2-id"}, {"tournamentId", "tournament-id"}, {"round", "regular"}, {"home", {{"id", "team3-id"}, {"name", "T3"}}}, {"visitor", {{"id", "team4-id"}, {"name", "T4"}}}};
    matches.push_back(std::make_shared<domain::Match>(body2));

    EXPECT_CALL(*matchDelegateMock, GetMatches(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournamentId),
                    testing::SaveArg<1>(&capturedFilter),
                    testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(matches))
                )
            );

    crow::request mockRequest;
    mockRequest.url = "/tournaments/tournament-id/matches";
    std::string tournamentId = "tournament-id";
    auto response = matchController->GetMatches(mockRequest, tournamentId);
    auto bodyJson = nlohmann::json::parse(response.body);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);
    
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedFilter, std::nullopt);
    EXPECT_EQ(bodyJson.size(), matches.size());
    EXPECT_EQ(bodyJson[0]["id"], body1.at("id").get<std::string>());
    EXPECT_EQ(bodyJson[0]["tournamentId"], body1.at("tournamentId").get<std::string>());
    EXPECT_EQ(bodyJson[0]["round"], body1.at("round").get<std::string>());
    EXPECT_EQ(bodyJson[0]["home"]["id"], body1.at("home").at("id").get<std::string>());
    EXPECT_EQ(bodyJson[0]["home"]["name"], body1.at("home").at("name").get<std::string>());
    EXPECT_EQ(bodyJson[0]["visitor"]["id"], body1.at("visitor").at("id").get<std::string>());
    EXPECT_EQ(bodyJson[0]["visitor"]["name"], body1.at("visitor").at("name").get<std::string>());
    EXPECT_EQ(bodyJson[1]["id"], body2.at("id").get<std::string>());
    EXPECT_EQ(bodyJson[1]["tournamentId"], body2.at("tournamentId").get<std::string>());
    EXPECT_EQ(bodyJson[1]["round"], body2.at("round").get<std::string>());
    EXPECT_EQ(bodyJson[1]["home"]["id"], body2.at("home").at("id").get<std::string>());
    EXPECT_EQ(bodyJson[1]["home"]["name"], body2.at("home").at("name").get<std::string>());
    EXPECT_EQ(bodyJson[1]["visitor"]["id"], body2.at("visitor").at("id").get<std::string>());
    EXPECT_EQ(bodyJson[1]["visitor"]["name"], body2.at("visitor").at("name").get<std::string>());
    EXPECT_EQ(response.code, crow::OK);
    EXPECT_EQ(response.get_header_value(CONTENT_TYPE_HEADER), JSON_CONTENT_TYPE);
}

TEST_F(MatchControllerTest, GetMatchesEmptyTest) {
    std::string capturedTournamentId;
    std::optional<std::string> capturedFilter;

    std::vector<std::shared_ptr<domain::Match>> matches;

    EXPECT_CALL(*matchDelegateMock, GetMatches(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournamentId),
                    testing::SaveArg<1>(&capturedFilter),
                    testing::Return(std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>(matches))
                )
            );

    crow::request mockRequest;
    mockRequest.url = "/tournaments/tournament-id/matches?showMatches=played";
    mockRequest.url_params = crow::query_string(mockRequest.url);
    std::string tournamentId = "tournament-id";
    auto response = matchController->GetMatches(mockRequest, tournamentId);
    auto bodyJson = nlohmann::json::parse(response.body);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);
    
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_TRUE(capturedFilter.has_value());
    EXPECT_EQ(capturedFilter.value(), "played");
    EXPECT_TRUE(bodyJson.empty());
    EXPECT_EQ(response.code, crow::OK);
    EXPECT_EQ(response.get_header_value(CONTENT_TYPE_HEADER), JSON_CONTENT_TYPE);
}

TEST_F(MatchControllerTest, GetMatchesWrongFilterTest) {
    EXPECT_CALL(*matchDelegateMock, GetMatches(::testing::_, ::testing::_))
        .Times(0);

    crow::request mockRequest;
    mockRequest.url = "/tournaments/tournament-id/matches?showMatches=wrong";
    mockRequest.url_params = crow::query_string(mockRequest.url);
    std::string tournamentId = "tournament-id";
    auto response = matchController->GetMatches(mockRequest, tournamentId);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid showMatches value. Must be 'played' or 'pending'");
}

TEST_F(MatchControllerTest, GetMatchesTournamentNotFoundTest) {
    std::string capturedTournamentId;
    std::optional<std::string> capturedFilter;

    EXPECT_CALL(*matchDelegateMock, GetMatches(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournamentId),
                    testing::SaveArg<1>(&capturedFilter),
                    testing::Return(std::unexpected<std::string>("Tournament not found"))
                )
            );

    crow::request mockRequest;
    mockRequest.url = "/tournaments/tournament-id/matches";
    std::string tournamentId = "tournament-id";
    auto response = matchController->GetMatches(mockRequest, tournamentId);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);
    
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedFilter, std::nullopt);
    
    EXPECT_EQ(response.code, crow::NOT_FOUND);
    EXPECT_EQ(response.body, "Tournament not found");
}

TEST_F(MatchControllerTest, GetMatchesDataBaseFailTest) {
    std::string capturedTournamentId;
    std::optional<std::string> capturedFilter;

    EXPECT_CALL(*matchDelegateMock, GetMatches(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournamentId),
                    testing::SaveArg<1>(&capturedFilter),
                    testing::Return(std::unexpected<std::string>("Selection fail"))
                )
            );

    crow::request mockRequest;
    mockRequest.url = "/tournaments/tournament-id/matches";
    std::string tournamentId = "tournament-id";
    auto response = matchController->GetMatches(mockRequest, tournamentId);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);
    
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedFilter, std::nullopt);
    
    EXPECT_EQ(response.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(response.body, "Selection fail");
}

TEST_F(MatchControllerTest, GetMatchSuccessTest) {
    std::string capturedTournamentId;
    std::string capturedMatchId;

    nlohmann::json body = {{"id", "match-id"}, {"tournamentId", "tournament-id"}, {"round", "regular"}, {"home", {{"id", "team1-id"}, {"name", "T1"}}}, {"visitor", {{"id", "team2-id"}, {"name", "T2"}}}};

    EXPECT_CALL(*matchDelegateMock, GetMatch(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournamentId),
                    testing::SaveArg<1>(&capturedMatchId),
                    testing::Return(std::expected<std::shared_ptr<domain::Match>, std::string>(std::make_shared<domain::Match>(body)))
                )
            );

    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id";
    auto response = matchController->GetMatch(tournamentId, matchId);
    auto bodyJson = nlohmann::json::parse(response.body);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);
    
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedMatchId, matchId);
    EXPECT_EQ(bodyJson["id"], body.at("id").get<std::string>());
    EXPECT_EQ(bodyJson["tournamentId"], body.at("tournamentId").get<std::string>());
    EXPECT_EQ(bodyJson["round"], body.at("round").get<std::string>());
    EXPECT_EQ(bodyJson["home"]["id"], body.at("home").at("id").get<std::string>());
    EXPECT_EQ(bodyJson["home"]["name"], body.at("home").at("name").get<std::string>());
    EXPECT_EQ(bodyJson["visitor"]["id"], body.at("visitor").at("id").get<std::string>());
    EXPECT_EQ(bodyJson["visitor"]["name"], body.at("visitor").at("name").get<std::string>());
    EXPECT_EQ(response.code, crow::OK);
    EXPECT_EQ(response.get_header_value(CONTENT_TYPE_HEADER), JSON_CONTENT_TYPE);
}

TEST_F(MatchControllerTest, GetMatchMatchNotFoundTest) {
    std::string capturedTournamentId;
    std::string capturedMatchId;

    EXPECT_CALL(*matchDelegateMock, GetMatch(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournamentId),
                    testing::SaveArg<1>(&capturedMatchId),
                    testing::Return(std::unexpected<std::string>("Match not found"))
                )
            );

    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id";
    auto response = matchController->GetMatch(tournamentId, matchId);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);
    
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedMatchId, matchId);
    EXPECT_EQ(response.code, crow::NOT_FOUND);
    EXPECT_EQ(response.body, "Match not found");
}

TEST_F(MatchControllerTest, GetMatchTournamentNotFoundTest) {
    std::string capturedTournamentId;
    std::string capturedMatchId;

    EXPECT_CALL(*matchDelegateMock, GetMatch(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournamentId),
                    testing::SaveArg<1>(&capturedMatchId),
                    testing::Return(std::unexpected<std::string>("Tournament not found"))
                )
            );

    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id";
    auto response = matchController->GetMatch(tournamentId, matchId);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);
    
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedMatchId, matchId);
    EXPECT_EQ(response.code, crow::NOT_FOUND);
    EXPECT_EQ(response.body, "Tournament not found");
}

TEST_F(MatchControllerTest, GetMatchDataBaseFailTest) {
    std::string capturedTournamentId;
    std::string capturedMatchId;

    EXPECT_CALL(*matchDelegateMock, GetMatch(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournamentId),
                    testing::SaveArg<1>(&capturedMatchId),
                    testing::Return(std::unexpected<std::string>("Selection failed"))
                )
            );

    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id";
    auto response = matchController->GetMatch(tournamentId, matchId);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);
    
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedMatchId, matchId);
    EXPECT_EQ(response.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(response.body, "Selection failed");
}

TEST_F(MatchControllerTest, UpdateMatchScoreSuccessTest) {
    std::string capturedTournamentId;
    std::string capturedMatchId;
    domain::Score capturedScore;

    EXPECT_CALL(*matchDelegateMock, UpdateMatchScore(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournamentId),
                    testing::SaveArg<1>(&capturedMatchId),
                    testing::SaveArg<2>(&capturedScore),
                    testing::Return(std::expected<void, std::string>())
                )
            );

    nlohmann::json scoreRequestBody = {{"score", {{"home", 6}, {"visitor", 7}}}};
    crow::request scoreRequest;
    scoreRequest.body = scoreRequestBody.dump();
    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id";
    auto response = matchController->UpdateMatchScore(scoreRequest, tournamentId, matchId);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);

    domain::Score expectedScore{6, 7};
    
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedMatchId, matchId);
    EXPECT_EQ(capturedScore.homeTeamScore, expectedScore.homeTeamScore);
    EXPECT_EQ(capturedScore.visitorTeamScore, expectedScore.visitorTeamScore);
    EXPECT_EQ(response.code, crow::NO_CONTENT);
}

TEST_F(MatchControllerTest, UpdateMatchScoreInvalidScoreJsonTest) {
    EXPECT_CALL(*matchDelegateMock, UpdateMatchScore(::testing::_, ::testing::_, ::testing::_))
        .Times(0);

    nlohmann::json scoreRequestBody = {{"spore", {{"home", 6}, {"visitor", 7}}}};
    crow::request scoreRequest;
    scoreRequest.body = scoreRequestBody.dump();
    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id";
    auto response = matchController->UpdateMatchScore(scoreRequest, tournamentId, matchId);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Missing 'score' field");
}

TEST_F(MatchControllerTest, UpdateMatchScoreMatchNotFoundTest) {
    std::string capturedTournamentId;
    std::string capturedMatchId;
    domain::Score capturedScore;

    EXPECT_CALL(*matchDelegateMock, UpdateMatchScore(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournamentId),
                    testing::SaveArg<1>(&capturedMatchId),
                    testing::SaveArg<2>(&capturedScore),
                    testing::Return(std::unexpected<std::string>("Match not found"))
                )
            );

    nlohmann::json scoreRequestBody = {{"score", {{"home", 6}, {"visitor", 7}}}};
    crow::request scoreRequest;
    scoreRequest.body = scoreRequestBody.dump();
    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id";
    auto response = matchController->UpdateMatchScore(scoreRequest, tournamentId, matchId);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);

    domain::Score expectedScore{6, 7};
    
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedMatchId, matchId);
    EXPECT_EQ(capturedScore.homeTeamScore, expectedScore.homeTeamScore);
    EXPECT_EQ(capturedScore.visitorTeamScore, expectedScore.visitorTeamScore);
    EXPECT_EQ(response.code, crow::NOT_FOUND);
    EXPECT_EQ(response.body, "Match not found");
}

TEST_F(MatchControllerTest, UpdateMatchScoreTournamentNotFoundTest) {
    std::string capturedTournamentId;
    std::string capturedMatchId;
    domain::Score capturedScore;

    EXPECT_CALL(*matchDelegateMock, UpdateMatchScore(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournamentId),
                    testing::SaveArg<1>(&capturedMatchId),
                    testing::SaveArg<2>(&capturedScore),
                    testing::Return(std::unexpected<std::string>("Tournament not found"))
                )
            );

    nlohmann::json scoreRequestBody = {{"score", {{"home", 6}, {"visitor", 7}}}};
    crow::request scoreRequest;
    scoreRequest.body = scoreRequestBody.dump();
    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id";
    auto response = matchController->UpdateMatchScore(scoreRequest, tournamentId, matchId);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);

    domain::Score expectedScore{6, 7};
    
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedMatchId, matchId);
    EXPECT_EQ(capturedScore.homeTeamScore, expectedScore.homeTeamScore);
    EXPECT_EQ(capturedScore.visitorTeamScore, expectedScore.visitorTeamScore);
    EXPECT_EQ(response.code, crow::NOT_FOUND);
    EXPECT_EQ(response.body, "Tournament not found");
}

TEST_F(MatchControllerTest, UpdateMatchScoreInvalidScoreTest) {
    std::string capturedTournamentId;
    std::string capturedMatchId;
    domain::Score capturedScore;

    EXPECT_CALL(*matchDelegateMock, UpdateMatchScore(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournamentId),
                    testing::SaveArg<1>(&capturedMatchId),
                    testing::SaveArg<2>(&capturedScore),
                    testing::Return(std::unexpected<std::string>("Invalid score for this tournament format and round"))
                )
            );

    nlohmann::json scoreRequestBody = {{"score", {{"home", 6}, {"visitor", 7}}}};
    crow::request scoreRequest;
    scoreRequest.body = scoreRequestBody.dump();
    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id";
    auto response = matchController->UpdateMatchScore(scoreRequest, tournamentId, matchId);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);

    domain::Score expectedScore{6, 7};
    
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedMatchId, matchId);
    EXPECT_EQ(capturedScore.homeTeamScore, expectedScore.homeTeamScore);
    EXPECT_EQ(capturedScore.visitorTeamScore, expectedScore.visitorTeamScore);
    EXPECT_EQ(response.code, 409);
    EXPECT_EQ(response.body, "Invalid score for this tournament format and round");
}

TEST_F(MatchControllerTest, UpdateMatchScoreInvalidTieTest) {
    std::string capturedTournamentId;
    std::string capturedMatchId;
    domain::Score capturedScore;

    EXPECT_CALL(*matchDelegateMock, UpdateMatchScore(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournamentId),
                    testing::SaveArg<1>(&capturedMatchId),
                    testing::SaveArg<2>(&capturedScore),
                    testing::Return(std::unexpected<std::string>("Tie not allowed"))
                )
            );

    nlohmann::json scoreRequestBody = {{"score", {{"home", 6}, {"visitor", 7}}}};
    crow::request scoreRequest;
    scoreRequest.body = scoreRequestBody.dump();
    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id";
    auto response = matchController->UpdateMatchScore(scoreRequest, tournamentId, matchId);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);

    domain::Score expectedScore{6, 7};
    
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedMatchId, matchId);
    EXPECT_EQ(capturedScore.homeTeamScore, expectedScore.homeTeamScore);
    EXPECT_EQ(capturedScore.visitorTeamScore, expectedScore.visitorTeamScore);
    EXPECT_EQ(response.code, 409);
    EXPECT_EQ(response.body, "Tie not allowed");
}

TEST_F(MatchControllerTest, UpdateMatchScoreDataBaseFailTest) {
    std::string capturedTournamentId;
    std::string capturedMatchId;
    domain::Score capturedScore;

    EXPECT_CALL(*matchDelegateMock, UpdateMatchScore(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedTournamentId),
                    testing::SaveArg<1>(&capturedMatchId),
                    testing::SaveArg<2>(&capturedScore),
                    testing::Return(std::unexpected<std::string>("Database fail"))
                )
            );

    nlohmann::json scoreRequestBody = {{"score", {{"home", 6}, {"visitor", 7}}}};
    crow::request scoreRequest;
    scoreRequest.body = scoreRequestBody.dump();
    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id";
    auto response = matchController->UpdateMatchScore(scoreRequest, tournamentId, matchId);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);

    domain::Score expectedScore{6, 7};
    
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedMatchId, matchId);
    EXPECT_EQ(capturedScore.homeTeamScore, expectedScore.homeTeamScore);
    EXPECT_EQ(capturedScore.visitorTeamScore, expectedScore.visitorTeamScore);
    EXPECT_EQ(response.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(response.body, "Database fail");
}

TEST_F(MatchControllerTest, UpdateMatchScoreInvalidJSONTest) {
    EXPECT_CALL(*matchDelegateMock, UpdateMatchScore(::testing::_, ::testing::_, ::testing::_))
        .Times(0);

    crow::request scoreRequest;
    scoreRequest.body = R"({"score": {"home": 6, INVALID}})";
    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id";
    auto response = matchController->UpdateMatchScore(scoreRequest, tournamentId, matchId);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid JSON");
}

TEST_F(MatchControllerTest, UpdateMatchScoreMissingJSONFieldsTest) {
    EXPECT_CALL(*matchDelegateMock, UpdateMatchScore(::testing::_, ::testing::_, ::testing::_))
        .Times(0);

    crow::request scoreRequest;
    scoreRequest.body = R"({"score": {"home": 6}})";
    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id";
    auto response = matchController->UpdateMatchScore(scoreRequest, tournamentId, matchId);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid JSON");
}

TEST_F(MatchControllerTest, UpdateMatchScoreWrongDataTypesTest) {
    EXPECT_CALL(*matchDelegateMock, UpdateMatchScore(::testing::_, ::testing::_, ::testing::_))
        .Times(0);

    crow::request scoreRequest;
    scoreRequest.body = R"({"score": {"home": "six", "visitor": "seven"}})";
    std::string tournamentId = "tournament-id";
    std::string matchId = "match-id";
    auto response = matchController->UpdateMatchScore(scoreRequest, tournamentId, matchId);

    testing::Mock::VerifyAndClearExpectations(&matchDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid JSON");
}