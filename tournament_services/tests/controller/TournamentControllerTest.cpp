#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Tournament.hpp"
#include "delegate/ITournamentDelegate.hpp"
#include "controller/TournamentController.hpp"
#include "domain/Utilities.hpp"

class TournamentDelegateMock : public ITournamentDelegate {
public:
    MOCK_METHOD(std::string, CreateTournament, (const std::shared_ptr<domain::Tournament> tournament), (override));
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, GetTournament, (const std::string_view id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(std::string, UpdateTournament, (const std::string_view id, const std::shared_ptr<domain::Tournament> tournament), (override));
    MOCK_METHOD(void, DeleteTournament, (const std::string_view id), (override));
};

class TournamentControllerTest : public ::testing::Test{
protected:
    std::shared_ptr<TournamentDelegateMock> tournamentDelegateMock;
    std::shared_ptr<TournamentController> tournamentController;

    void SetUp() override {
        tournamentDelegateMock = std::make_shared<TournamentDelegateMock>();
        tournamentController = std::make_shared<TournamentController>(TournamentController(tournamentDelegateMock));
    }

    // TearDown() function
    void TearDown() override {
        // teardown code comes here
    }

};

TEST_F(TournamentControllerTest, CreateTournamentJSONTransformationTest) {
    std::shared_ptr<domain::Tournament> capturedTournament;

    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournament),
                testing::Return("new-id")
            )
        );

    nlohmann::json tournamentRequestBody = {{"id", "new-id"}, {"name", "new tournament"}};
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->CreateTournament(tournamentRequest);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(tournamentRequestBody.at("id").get<std::string>(), capturedTournament->Id());
    EXPECT_EQ(tournamentRequestBody.at("name").get<std::string>(), capturedTournament->Name());
    EXPECT_EQ(crow::CREATED, response.code);
}

TEST_F(TournamentControllerTest, CreateTournamentDBInsertionErrorTest) {
    std::shared_ptr<domain::Tournament> capturedTournament;

    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournament),
                testing::Return("")
            )
        );
    
    nlohmann::json tournamentRequestBody = {{"id", "dup-id"}, {"name", "existing tournament"}};
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->CreateTournament(tournamentRequest);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(tournamentRequestBody.at("id").get<std::string>(), capturedTournament->Id());
    EXPECT_EQ(tournamentRequestBody.at("name").get<std::string>(), capturedTournament->Name());
    EXPECT_EQ(crow::CONFLICT, response.code);
}

TEST_F(TournamentControllerTest, ReadTournamentSuccessfulTest) {
    std::string_view id;

    nlohmann::json body = {{"id", "read-id"}, {"name", "read tournament"}};
    auto tournament = std::make_shared<domain::Tournament>(body);
    tournament->Id() = body["id"];

    EXPECT_CALL(*tournamentDelegateMock, GetTournament(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&id),
                testing::Return(tournament)
            )
        );
    
    std::string tournamentId = "read-id";

    crow::response response = tournamentController->ReadTournament(tournamentId);
    auto bodyJson = nlohmann::json::parse(response.body);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(bodyJson["id"], "read-id");
    EXPECT_EQ(bodyJson["name"], "read tournament");
    EXPECT_EQ(tournamentId, id);
    EXPECT_EQ(crow::OK, response.code);
}

TEST_F(TournamentControllerTest, ReadTournamentFailTest) {
    std::string_view id;

    EXPECT_CALL(*tournamentDelegateMock, GetTournament(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&id),
                testing::Return(nullptr)
            )
        );
    
    std::string tournamentId = "non-existing-id";

    crow::response response = tournamentController->ReadTournament(tournamentId);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(tournamentId, id);
    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

TEST_F(TournamentControllerTest, ReadAllTournamentsSuccessfulTest) {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments;

    nlohmann::json body = {{"id", "first-id"}, {"name", "first tournament"}};
    auto tournament = std::make_shared<domain::Tournament>(body);
    tournament->Id() = body["id"];
    tournaments.push_back(tournament);

    body = {{"id", "second-id"}, {"name", "second tournament"}};
    tournament = std::make_shared<domain::Tournament>(body);
    tournament->Id() = body["id"];
    tournaments.push_back(tournament);

    EXPECT_CALL(*tournamentDelegateMock, ReadAll())
        .WillOnce(testing::Return(tournaments));

    crow::response response = tournamentController->ReadAll();
    auto bodyJson = nlohmann::json::parse(response.body);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);
    
    EXPECT_EQ(bodyJson.size(), 2);
    EXPECT_EQ(bodyJson[0]["id"], "first-id");
    EXPECT_EQ(bodyJson[0]["name"], "first tournament");
    EXPECT_EQ(bodyJson[1]["id"], "second-id");
    EXPECT_EQ(bodyJson[1]["name"], "second tournament");
    EXPECT_EQ(crow::OK, response.code);
}

TEST_F(TournamentControllerTest, ReadAllTournamentsFailTest) {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments;

    EXPECT_CALL(*tournamentDelegateMock, ReadAll())
        .WillOnce(testing::Return(tournaments));

    crow::response response = tournamentController->ReadAll();

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);
    
    EXPECT_EQ(response.body.size(), 0);
    EXPECT_EQ(crow::OK, response.code);
}

TEST_F(TournamentControllerTest, UpdateTournamentSuccessfulTest) {
    std::string_view id;
    std::shared_ptr<domain::Tournament> capturedTournament;

    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&id),
                testing::SaveArg<1>(&capturedTournament),
                testing::Return("updated-id")
            )
        );

    nlohmann::json tournamentRequestBody = {{"id", "updated-id"}, {"name", "updated tournament"}};
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    std::string tournamentId = "updated-id";

    crow::response response = tournamentController->UpdateTournament(tournamentRequest, tournamentId);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(tournamentRequestBody.at("id").get<std::string>(), capturedTournament->Id());
    EXPECT_EQ(tournamentRequestBody.at("name").get<std::string>(), capturedTournament->Name());
    EXPECT_EQ(crow::NO_CONTENT, response.code);
}

TEST_F(TournamentControllerTest, UpdateTournamentFailTest) {
    std::string_view id;
    std::shared_ptr<domain::Tournament> capturedTournament;

    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&id),
                testing::SaveArg<1>(&capturedTournament),
                testing::Return("")
            )
        );

    nlohmann::json tournamentRequestBody = {{"id", "non-existing-id"}, {"name", "updated tournament"}};
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    std::string tournamentId = "non-existing-id";

    crow::response response = tournamentController->UpdateTournament(tournamentRequest, tournamentId);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(tournamentRequestBody.at("id").get<std::string>(), capturedTournament->Id());
    EXPECT_EQ(tournamentRequestBody.at("name").get<std::string>(), capturedTournament->Name());
    EXPECT_EQ(crow::NOT_FOUND, response.code);
}
