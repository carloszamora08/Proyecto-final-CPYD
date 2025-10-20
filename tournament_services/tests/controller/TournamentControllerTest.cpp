#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Tournament.hpp"
#include "delegate/ITournamentDelegate.hpp"
#include "controller/TournamentController.hpp"
#include "domain/Utilities.hpp"

class TournamentDelegateMock : public ITournamentDelegate {
public:
    MOCK_METHOD((std::expected<std::string, std::string>), CreateTournament, (const std::shared_ptr<domain::Tournament> tournament), (override));
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Tournament>, std::string>), GetTournament, (const std::string_view id), (override));
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string>), ReadAll, (), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), UpdateTournament, (const std::string_view id, const std::shared_ptr<domain::Tournament> tournament), (override));
    MOCK_METHOD((std::expected<void, std::string>), DeleteTournament, (const std::string_view id), (override));
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

TEST_F(TournamentControllerTest, CreateTournamentSucessTest) {
    std::shared_ptr<domain::Tournament> capturedTournament;

    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournament),
                testing::Return(std::expected<std::string, std::string>("new-id"))
            )
        );

    nlohmann::json tournamentRequestBody = {{"id", "new-id"}, {"name", "new tournament"}, {"year", 2025}};
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();
    auto response = tournamentController->CreateTournament(tournamentRequest);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(capturedTournament->Id(), tournamentRequestBody.at("id").get<std::string>());
    EXPECT_EQ(capturedTournament->Name(), tournamentRequestBody.at("name").get<std::string>());
    EXPECT_EQ(capturedTournament->Year(), tournamentRequestBody.at("year").get<int>());
    EXPECT_EQ(response.code, crow::CREATED);
    EXPECT_EQ(response.get_header_value("location"), "new-id");
}

TEST_F(TournamentControllerTest, CreateTournamentDBInsertionFailTest) {
    std::shared_ptr<domain::Tournament> capturedTournament;

    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournament),
                testing::Return(std::unexpected<std::string>("Tournament insertion failed"))
            )
        );
    
    nlohmann::json tournamentRequestBody = {{"id", "existing-id"}, {"name", "existing tournament"}, {"year", 2025}};
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();
    auto response = tournamentController->CreateTournament(tournamentRequest);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(capturedTournament->Id(), tournamentRequestBody.at("id").get<std::string>());
    EXPECT_EQ(capturedTournament->Name(), tournamentRequestBody.at("name").get<std::string>());
    EXPECT_EQ(capturedTournament->Year(), tournamentRequestBody.at("year").get<int>());
    EXPECT_EQ(response.code, crow::CONFLICT);
    EXPECT_EQ(response.body, "Tournament insertion failed");
}

TEST_F(TournamentControllerTest, CreateTournamentMalformedJSONTest) {
    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(::testing::_))
        .Times(0);
   
    crow::request tournamentRequest;
    tournamentRequest.body = R"({malformed json})";
    auto response = tournamentController->CreateTournament(tournamentRequest);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid JSON");
}

TEST_F(TournamentControllerTest, CreateTournamentInvalidDataTest) {
    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(::testing::_))
        .Times(0);
    
    crow::request tournamentRequest;
    tournamentRequest.body = R"({"id": 123, "name": 456, "year": "789"})";
    auto response = tournamentController->CreateTournament(tournamentRequest);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid JSON");
}

TEST_F(TournamentControllerTest, ReadTournamentSuccessTest) {
    std::string id;

    nlohmann::json body = {{"id", "read-id"}, {"name", "read tournament"}, {"year", 2025}};
    auto tournament = std::make_shared<domain::Tournament>(body);

    EXPECT_CALL(*tournamentDelegateMock, GetTournament(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&id),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );
    
    std::string tournamentId = "read-id";
    auto response = tournamentController->ReadTournament(tournamentId);
    auto bodyJson = nlohmann::json::parse(response.body);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(id, tournamentId);
    EXPECT_EQ(bodyJson["id"], body.at("id").get<std::string>());
    EXPECT_EQ(bodyJson["name"], body.at("name").get<std::string>());
    EXPECT_EQ(bodyJson["year"], body.at("year").get<int>());
    EXPECT_EQ(response.code, crow::OK);
    EXPECT_EQ(response.get_header_value(CONTENT_TYPE_HEADER), JSON_CONTENT_TYPE);
}

TEST_F(TournamentControllerTest, ReadTournamentDBSelectionFailTest) {
    std::string id;

    EXPECT_CALL(*tournamentDelegateMock, GetTournament(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&id),
                testing::Return(std::unexpected<std::string>("Tournament not found"))
            )
        );
    
    std::string tournamentId = "non-existing-id";
    auto response = tournamentController->ReadTournament(tournamentId);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(id, tournamentId);
    EXPECT_EQ(response.code, crow::NOT_FOUND);
    EXPECT_EQ(response.body, "Tournament not found");
}

TEST_F(TournamentControllerTest, ReadTournamentInvalidIDTest) {
    EXPECT_CALL(*tournamentDelegateMock, GetTournament(::testing::_))
        .Times(0);

    std::string tournamentId = "bad id";
    auto response = tournamentController->ReadTournament(tournamentId);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid ID format");
}

TEST_F(TournamentControllerTest, ReadAllTournamentsSuccessTest) {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments;

    nlohmann::json body1 = {{"id", "first-id"}, {"name", "first tournament"}, {"year", 2024}};
    tournaments.push_back(std::make_shared<domain::Tournament>(body1));

    nlohmann::json body2 = {{"id", "second-id"}, {"name", "second tournament"}, {"year", 2026}};
    tournaments.push_back(std::make_shared<domain::Tournament>(body2));

    EXPECT_CALL(*tournamentDelegateMock, ReadAll())
        .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string>(tournaments)));

    auto response = tournamentController->ReadAll();
    auto bodyJson = nlohmann::json::parse(response.body);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);
    
    EXPECT_EQ(bodyJson.size(), tournaments.size());
    EXPECT_EQ(bodyJson[0]["id"], body1.at("id").get<std::string>());
    EXPECT_EQ(bodyJson[0]["name"], body1.at("name").get<std::string>());
    EXPECT_EQ(bodyJson[0]["year"], body1.at("year").get<int>());
    EXPECT_EQ(bodyJson[1]["id"], body2.at("id").get<std::string>());
    EXPECT_EQ(bodyJson[1]["name"], body2.at("name").get<std::string>());
    EXPECT_EQ(bodyJson[1]["year"], body2.at("year").get<int>());
    EXPECT_EQ(response.code, crow::OK);
    EXPECT_EQ(response.get_header_value(CONTENT_TYPE_HEADER), JSON_CONTENT_TYPE);
}

TEST_F(TournamentControllerTest, ReadAllTournamentsEmptyTest) {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments;

    EXPECT_CALL(*tournamentDelegateMock, ReadAll())
        .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string>(tournaments)));

    auto response = tournamentController->ReadAll();
    auto bodyJson = nlohmann::json::parse(response.body);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);
    
    EXPECT_EQ(bodyJson.size(), 0);
    EXPECT_EQ(response.body, "[]");
    EXPECT_EQ(response.code, crow::OK);
    EXPECT_EQ(response.get_header_value(CONTENT_TYPE_HEADER), JSON_CONTENT_TYPE);
}

TEST_F(TournamentControllerTest, ReadAllTournamentsDBFailTest) {
    EXPECT_CALL(*tournamentDelegateMock, ReadAll())
        .WillOnce(testing::Return(std::unexpected<std::string>("Database connection failed")));

    auto response = tournamentController->ReadAll();

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);
    
    EXPECT_EQ(response.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(response.body, "Database connection failed");
}

TEST_F(TournamentControllerTest, UpdateTournamentSuccessTest) {
    std::string id;
    std::shared_ptr<domain::Tournament> capturedTournament;

    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&id),
                testing::SaveArg<1>(&capturedTournament),
                testing::Return(std::expected<std::string, std::string>("updated-id"))
            )
        );

    nlohmann::json tournamentRequestBody = {{"id", "updated-id"}, {"name", "updated tournament"}, {"year", 2025}};
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();
    std::string tournamentId = "updated-id";
    auto response = tournamentController->UpdateTournament(tournamentRequest, tournamentId);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(id, tournamentId);
    EXPECT_EQ(capturedTournament->Id(), tournamentRequestBody.at("id").get<std::string>());
    EXPECT_EQ(capturedTournament->Name(), tournamentRequestBody.at("name").get<std::string>());
    EXPECT_EQ(capturedTournament->Year(), tournamentRequestBody.at("year").get<int>());
    EXPECT_EQ(response.code, crow::NO_CONTENT);
}

TEST_F(TournamentControllerTest, UpdateTournamentFailTest) {
    std::string id;
    std::shared_ptr<domain::Tournament> capturedTournament;

    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&id),
                testing::SaveArg<1>(&capturedTournament),
                testing::Return(std::unexpected<std::string>("Tournament not found"))
            )
        );

    nlohmann::json tournamentRequestBody = {{"id", "non-existing-id"}, {"name", "updated tournament"}, {"year", 2025}};
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();
    std::string tournamentId = "non-existing-id";
    auto response = tournamentController->UpdateTournament(tournamentRequest, tournamentId);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(id, tournamentId);
    EXPECT_EQ(capturedTournament->Id(), tournamentRequestBody.at("id").get<std::string>());
    EXPECT_EQ(capturedTournament->Name(), tournamentRequestBody.at("name").get<std::string>());
    EXPECT_EQ(capturedTournament->Year(), tournamentRequestBody.at("year").get<int>());
    EXPECT_EQ(response.code, crow::NOT_FOUND);
    EXPECT_EQ(response.body, "Tournament not found");
}

TEST_F(TournamentControllerTest, UpdateTournamentInvalidIDTest) {
    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(::testing::_, ::testing::_))
        .Times(0);

    nlohmann::json tournamentRequestBody = {{"id", "bad id"}, {"name", "bad tournament"}, {"year", 2025}};
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();
    std::string tournamentId = "bad id";
    auto response = tournamentController->UpdateTournament(tournamentRequest, tournamentId);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid ID format");
}

TEST_F(TournamentControllerTest, UpdateTournamentDBFailTest) {
    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Database connection failed")));

    nlohmann::json tournamentRequestBody = {{"id", "id"}, {"name", "tournament"}, {"year", 2025}};
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();
    std::string tournamentId = "id";
    auto response = tournamentController->UpdateTournament(tournamentRequest, tournamentId);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);
    
    EXPECT_EQ(response.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(response.body, "Database connection failed");
}

TEST_F(TournamentControllerTest, UpdateTournamentMalformedJSONTest) {
    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(::testing::_, ::testing::_))
        .Times(0);
   
    crow::request tournamentRequest;
    tournamentRequest.body = R"({malformed json})";
    std::string tournamentId = "id";
    auto response = tournamentController->UpdateTournament(tournamentRequest, tournamentId);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid JSON");
}

TEST_F(TournamentControllerTest, UpdateTournamentInvalidDataTest) {
    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(::testing::_, ::testing::_))
        .Times(0);
    
    crow::request tournamentRequest;
    tournamentRequest.body = R"({"id": 123, "name": 456, "year": "789"})";
    std::string tournamentId = "id";
    auto response = tournamentController->UpdateTournament(tournamentRequest, tournamentId);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid JSON");
}

TEST_F(TournamentControllerTest, DeleteTournamentSuccessTest) {
    std::string id;

    EXPECT_CALL(*tournamentDelegateMock, DeleteTournament(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&id),
                testing::Return(std::expected<void, std::string>())
            )
        );
    
    std::string tournamentId = "read-id";
    auto response = tournamentController->DeleteTournament(tournamentId);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(id, tournamentId);
    EXPECT_EQ(response.code, crow::NO_CONTENT);
}

TEST_F(TournamentControllerTest, DeleteTournamentDBDeletionFailTest) {
    std::string id;

    EXPECT_CALL(*tournamentDelegateMock, DeleteTournament(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&id),
                testing::Return(std::unexpected<std::string>("Tournament not found"))
            )
        );
    
    std::string tournamentId = "non-existing-id";
    auto response = tournamentController->DeleteTournament(tournamentId);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(id, tournamentId);
    EXPECT_EQ(response.code, crow::NOT_FOUND);
    EXPECT_EQ(response.body, "Tournament not found");
}

TEST_F(TournamentControllerTest, DeleteTournamentInvalidIDTest) {
    EXPECT_CALL(*tournamentDelegateMock, DeleteTournament(::testing::_))
        .Times(0);

    std::string tournamentId = "bad id";
    auto response = tournamentController->DeleteTournament(tournamentId);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid ID format");
}

TEST_F(TournamentControllerTest, DeleteTournamentDBFailTest) {
    EXPECT_CALL(*tournamentDelegateMock, DeleteTournament(::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Database connection failed")));

    std::string tournamentId = "id";
    auto response = tournamentController->DeleteTournament(tournamentId);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);
    
    EXPECT_EQ(response.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(response.body, "Database connection failed");
}