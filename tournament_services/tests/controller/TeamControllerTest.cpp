#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Team.hpp"
#include "delegate/ITeamDelegate.hpp"
#include "controller/TeamController.hpp"

class TeamDelegateMock : public ITeamDelegate {
    public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, GetTeam, (const std::string_view id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, GetAllTeams, (), (override));
    MOCK_METHOD(std::string, UpdateTeam, (std::string_view id, std::shared_ptr<domain::Team> team), (override));
    MOCK_METHOD(void, DeleteTeam, (std::string_view id), (override));
    MOCK_METHOD(std::string_view, SaveTeam, (const domain::Team&), (override));
};

class TeamControllerTest : public ::testing::Test{
protected:
    std::shared_ptr<TeamDelegateMock> teamDelegateMock;
    std::shared_ptr<TeamController> teamController;

    void SetUp() override {
        teamDelegateMock = std::make_shared<TeamDelegateMock>();
        teamController = std::make_shared<TeamController>(TeamController(teamDelegateMock));
    }

    // TearDown() function
    void TearDown() override {
        // teardown code comes here
    }

};

//Prueba 1
TEST_F(TeamControllerTest, SaveTeamTest) {
    domain::Team capturedTeam;
    EXPECT_CALL(*teamDelegateMock, SaveTeam(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTeam),
                testing::Return("new-id")
            )
        );

    nlohmann::json teamRequestBody = {{"id", "new-id"}, {"name", "new team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->SaveTeam(teamRequest);

    testing::Mock::VerifyAndClearExpectations(&teamDelegateMock);

    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ(teamRequestBody.at("id").get<std::string>(), capturedTeam.Id);
    EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam.Name);
}

//Prueba 2
TEST_F(TeamControllerTest, SaveTeam_DatabaseErrorTest) {
    domain::Team capturedTeam;

    EXPECT_CALL(*teamDelegateMock, SaveTeam(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTeam),
                testing::Throw(std::runtime_error("Database constraint violation: duplicate key"))
            )
        );

    nlohmann::json teamRequestBody = {
        {"id", "duplicate-id"},
        {"name", "Duplicate Team"}
    };

    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->SaveTeam(teamRequest);

    testing::Mock::VerifyAndClearExpectations(&teamDelegateMock);

    EXPECT_EQ(crow::CONFLICT, response.code);

    EXPECT_EQ(teamRequestBody.at("id").get<std::string>(), capturedTeam.Id);
    EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam.Name);

    EXPECT_STREQ("duplicate-id", capturedTeam.Id.c_str());
    EXPECT_STREQ("Duplicate Team", capturedTeam.Name.c_str());
}

//Prueba 3
TEST_F(TeamControllerTest, TeamSearchTest) {
    std::shared_ptr<domain::Team> expectedTeam = std::make_shared<domain::Team>(
        domain::Team{"test-team-123", "Test Team Name"}
    );

    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(std::string("test-team-123"))))
        .WillOnce(testing::Return(expectedTeam));

    crow::response response = teamController->getTeam("test-team-123");

    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(expectedTeam->Id, jsonResponse["id"]);
    EXPECT_EQ(expectedTeam->Name, jsonResponse["name"]);
}

//Prueba 4
TEST_F(TeamControllerTest, GetTeam_NullTest) {
    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(std::string("no-id"))))
        .WillOnce(testing::Return(nullptr));

    crow::response response = teamController->getTeam("no-id");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
    EXPECT_EQ("team not found", response.body);
}

//Prueba 5
TEST_F(TeamControllerTest, GetAllTeamsTest) {
    std::vector<std::shared_ptr<domain::Team>> mockTeamsList;
    mockTeamsList.push_back(std::make_shared<domain::Team>(domain::Team{"6-7", "Guardian"}));
    mockTeamsList.push_back(std::make_shared<domain::Team>(domain::Team{"15-00", "Tales"}));
    mockTeamsList.push_back(std::make_shared<domain::Team>(domain::Team{"2-3-4-5-6", "Zona Minecraft"}));

    EXPECT_CALL(*teamDelegateMock, GetAllTeams())
        .WillOnce(testing::Return(mockTeamsList));

    crow::response response = teamController->getAllTeams();

    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(200, response.code);

    EXPECT_EQ(3, jsonResponse.size());

    EXPECT_EQ("6-7", jsonResponse[0]["id"]);
    EXPECT_EQ("Guardian", jsonResponse[0]["name"]);
    EXPECT_EQ("15-00", jsonResponse[1]["id"]);
    EXPECT_EQ("Tales", jsonResponse[1]["name"]);
    EXPECT_EQ("2-3-4-5-6", jsonResponse[2]["id"]);
    EXPECT_EQ("Zona Minecraft", jsonResponse[2]["name"]);
}

//Prueba 6
TEST_F(TeamControllerTest, GetAllTeams_EmptyTest) {
    std::vector<std::shared_ptr<domain::Team>> emptyTeamsList;

    EXPECT_CALL(*teamDelegateMock, GetAllTeams())
        .WillOnce(testing::Return(emptyTeamsList));

    crow::response response = teamController->getAllTeams();

    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(200, response.code);

    EXPECT_EQ(0, jsonResponse.size());
}

//Prueba 7
TEST_F(TeamControllerTest, UpdateTeamTest) {
    std::shared_ptr<domain::Team> capturedTeam;
    std::string capturedId;

    EXPECT_CALL(*teamDelegateMock, UpdateTeam(testing::Eq(std::string("id-123456")), testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedId),
                testing::SaveArg<1>(&capturedTeam),
                testing::Return("id-123456")
            )
        );

    nlohmann::json teamRequestBody = {
        {"id", "id-123456"},
        {"name", "Tortas de Jamon"}
    };

    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->UpdateTeam(teamRequest, "id-123456");

    testing::Mock::VerifyAndClearExpectations(&teamDelegateMock);

    EXPECT_EQ(crow::NO_CONTENT, response.code);

    EXPECT_EQ("id-123456", capturedId);

    EXPECT_EQ(teamRequestBody.at("id").get<std::string>(), capturedTeam->Id);
    EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam->Name);
}

// Prueba 8
TEST_F(TeamControllerTest, UpdateTeam_NotFoundTest) {
    std::shared_ptr<domain::Team> capturedTeam;
    std::string capturedId;

    EXPECT_CALL(*teamDelegateMock, UpdateTeam(testing::Eq(std::string("no-id")), testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedId),
                testing::SaveArg<1>(&capturedTeam),
                testing::Return("")
            )
        );

    nlohmann::json teamRequestBody = {
        {"id", "no-id"},
        {"name", "Updated Team Name"}
    };

    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->UpdateTeam(teamRequest, "no-id");

    testing::Mock::VerifyAndClearExpectations(&teamDelegateMock);

    EXPECT_EQ(crow::NOT_FOUND, response.code);

    EXPECT_EQ("no-id", capturedId);

    EXPECT_EQ(teamRequestBody.at("id").get<std::string>(), capturedTeam->Id);
    EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam->Name);
}

//Pruebas extra
TEST_F(TeamControllerTest, GetTeamById_ErrorFormat) {
    crow::response badRequest = teamController->getTeam("");

    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);

    badRequest = teamController->getTeam("mfasd#*");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);
}

TEST_F(TeamControllerTest, GetTeamById) {

    std::shared_ptr<domain::Team> expectedTeam = std::make_shared<domain::Team>(domain::Team{"my-id",  "Team Name"});

    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(std::string("my-id"))))
        .WillOnce(testing::Return(expectedTeam));

    crow::response response = teamController->getTeam("my-id");
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(expectedTeam->Id, jsonResponse["id"]);
    EXPECT_EQ(expectedTeam->Name, jsonResponse["name"]);
}

TEST_F(TeamControllerTest, GetTeamNotFound) {
    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(std::string("my-id"))))
        .WillOnce(testing::Return(nullptr));

    crow::response response = teamController->getTeam("my-id");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}
