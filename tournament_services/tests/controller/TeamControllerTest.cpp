#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Team.hpp"
#include "delegate/ITeamDelegate.hpp"
#include "controller/TeamController.hpp"

class TeamDelegateMock : public ITeamDelegate {
    public:
    MOCK_METHOD((std::expected<std::string, std::string>), CreateTeam, (const std::shared_ptr<domain::Team> team), (override));
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Team>, std::string>), GetTeam, (const std::string_view id), (override));
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Team>>, std::string>), ReadAll, (), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), UpdateTeam, (const std::string_view id, const std::shared_ptr<domain::Team> team), (override));
    MOCK_METHOD((std::expected<void, std::string>), DeleteTeam, (const std::string_view id), (override));
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

//Prueba 1: Crear un equipo exitosamente
TEST_F(TeamControllerTest, CreateTeamSuccessTest) {
    std::shared_ptr<domain::Team> capturedTeam;

    EXPECT_CALL(*teamDelegateMock, CreateTeam(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTeam),
                testing::Return(std::expected<std::string, std::string>("new-id"))
            )
        );

    nlohmann::json teamRequestBody = {{"id", "new-id"}, {"name", "new team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();
    auto response = teamController->SaveTeam(teamRequest);

    testing::Mock::VerifyAndClearExpectations(&teamDelegateMock);

    EXPECT_EQ(capturedTeam->Id, teamRequestBody.at("id").get<std::string>());
    EXPECT_EQ(capturedTeam->Name, teamRequestBody.at("name").get<std::string>());
    EXPECT_EQ(response.code, crow::CREATED);
    EXPECT_EQ(response.get_header_value("location"), "new-id");
}

// Prueba 2: Crear un equipo con error en la base de datos
TEST_F(TeamControllerTest, SaveTeam_DatabaseErrorTest) {
    std::shared_ptr<domain::Team> capturedTeam;

    EXPECT_CALL(*teamDelegateMock, CreateTeam(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTeam),
                testing::Return(std::unexpected<std::string>("Database constraint violation: duplicate key"))
            )
        );

    nlohmann::json teamRequestBody = {
        {"id", "id-5"},
        {"name", "Equipo Mucha Lucha"}
    };

    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->SaveTeam(teamRequest);

    testing::Mock::VerifyAndClearExpectations(&teamDelegateMock);

    EXPECT_EQ(crow::CONFLICT, response.code);
    EXPECT_EQ(teamRequestBody.at("id").get<std::string>(), capturedTeam->Id);
    EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam->Name);
}

// Prueba 3: Crear un equipo con JSON malformado
TEST_F(TeamControllerTest, SaveTeam_MalformedJSONTest) {
    EXPECT_CALL(*teamDelegateMock, CreateTeam(::testing::_))
        .Times(0);

    crow::request teamRequest;
    teamRequest.body = R"({malformed json})";

    crow::response response = teamController->SaveTeam(teamRequest);

    testing::Mock::VerifyAndClearExpectations(&teamDelegateMock);

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
    EXPECT_EQ("Invalid JSON", response.body);
}

// Prueba 4: Búsqueda de equipo por ID
TEST_F(TeamControllerTest, TeamSearchTest) {
    std::shared_ptr<domain::Team> expectedTeam = std::make_shared<domain::Team>(
        domain::Team{"test-team-123", "Team Flare"}
    );

    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(std::string("test-team-123"))))
        .WillOnce(testing::Return(expectedTeam));

    crow::response response = teamController->getTeam("test-team-123");

    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(expectedTeam->Id, jsonResponse["id"]);
    EXPECT_EQ(expectedTeam->Name, jsonResponse["name"]);
}

// Prueba 5: Obtener equipo que no existe
TEST_F(TeamControllerTest, GetTeam_NullTest) {
    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(std::string("no-id"))))
        .WillOnce(testing::Return(std::unexpected<std::string>("Team not found")));

    crow::response response = teamController->getTeam("no-id");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
    EXPECT_EQ("Team not found", response.body);
}

// Prueba 6: Obtener equipo con ID inválido
TEST_F(TeamControllerTest, GetTeam_InvalidIDTest) {
    EXPECT_CALL(*teamDelegateMock, GetTeam(::testing::_))
        .Times(0);

    crow::response response = teamController->getTeam("invalid#id");

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
    EXPECT_EQ("Invalid ID format", response.body);
}

// Prueba 7: Obtener equipo con header JSON
TEST_F(TeamControllerTest, GetTeam_ValidWithHeaderTest) {
    std::shared_ptr<domain::Team> expectedTeam = std::make_shared<domain::Team>(
        domain::Team{"test-id", "Team Rocket"}
    );

    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(std::string("test-id"))))
        .WillOnce(testing::Return(expectedTeam));

    crow::response response = teamController->getTeam("test-id");

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
}

// Prueba 8: Obtener todos los equipos
TEST_F(TeamControllerTest, GetAllTeamsTest) {
    std::vector<std::shared_ptr<domain::Team>> mockTeamsList;
    mockTeamsList.push_back(std::make_shared<domain::Team>(domain::Team{"6-7", "Guardian"}));
    mockTeamsList.push_back(std::make_shared<domain::Team>(domain::Team{"15-00", "Tales"}));
    mockTeamsList.push_back(std::make_shared<domain::Team>(domain::Team{"2-3-4-5-6", "Zona Minecraft"}));

    EXPECT_CALL(*teamDelegateMock, ReadAll())
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

// Prueba 9: Obtener todos los equipos con lista vacía
TEST_F(TeamControllerTest, GetAllTeams_EmptyTest) {
    std::vector<std::shared_ptr<domain::Team>> emptyTeamsList;

    EXPECT_CALL(*teamDelegateMock, ReadAll())
        .WillOnce(testing::Return(emptyTeamsList));

    crow::response response = teamController->getAllTeams();

    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(200, response.code);
    EXPECT_EQ(0, jsonResponse.size());
}

// Prueba 10: Obtener todos los equipos cuando hay error en la base de datos
TEST_F(TeamControllerTest, GetAllTeams_DBErrorTest) {
    EXPECT_CALL(*teamDelegateMock, ReadAll())
        .WillOnce(testing::Return(std::unexpected<std::string>("Database connection failed")));

    crow::response response = teamController->getAllTeams();

    EXPECT_EQ(crow::INTERNAL_SERVER_ERROR, response.code);
    EXPECT_EQ("Database connection failed", response.body);
}

// Prueba 11: Obtener todos los equipos con header JSON
TEST_F(TeamControllerTest, GetAllTeams_ValidWithHeaderTest) {
    std::vector<std::shared_ptr<domain::Team>> mockTeamsList;
    mockTeamsList.push_back(std::make_shared<domain::Team>(domain::Team{"id1", "Team1"}));
    mockTeamsList.push_back(std::make_shared<domain::Team>(domain::Team{"id2", "Team2"}));

    EXPECT_CALL(*teamDelegateMock, ReadAll())
        .WillOnce(testing::Return(mockTeamsList));

    crow::response response = teamController->getAllTeams();

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ("application/json", response.get_header_value("content-type"));
}

// Prueba 12: Actualizar un equipo exitosamente
TEST_F(TeamControllerTest, UpdateTeamTest) {
    std::shared_ptr<domain::Team> capturedTeam;
    std::string_view capturedId;

    EXPECT_CALL(*teamDelegateMock, UpdateTeam(testing::Eq(std::string("id-123456")), testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedId),
                testing::SaveArg<1>(&capturedTeam),
                testing::Return(std::expected<std::string, std::string>("id-123456"))
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
    EXPECT_EQ(teamRequestBody.at("id").get<std::string>(), capturedTeam->Id);
    EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam->Name);
}

// Prueba 13: Actualizar equipo que no existe
TEST_F(TeamControllerTest, UpdateTeam_NotFoundTest) {
    std::shared_ptr<domain::Team> capturedTeam;
    std::string_view capturedId;

    EXPECT_CALL(*teamDelegateMock, UpdateTeam(testing::Eq(std::string("no-id")), testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedId),
                testing::SaveArg<1>(&capturedTeam),
                testing::Return(std::unexpected<std::string>("Team not found"))
            )
        );

    nlohmann::json teamRequestBody = {
        {"id", "no-id"},
        {"name", "Los Bunkers"}
    };

    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->UpdateTeam(teamRequest, "no-id");

    testing::Mock::VerifyAndClearExpectations(&teamDelegateMock);

    EXPECT_EQ(crow::NOT_FOUND, response.code);
    EXPECT_EQ(teamRequestBody.at("id").get<std::string>(), capturedTeam->Id);
    EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam->Name);
}

// Prueba 14: Actualizar equipo con ID inválido
TEST_F(TeamControllerTest, UpdateTeam_InvalidIDTest) {
    EXPECT_CALL(*teamDelegateMock, UpdateTeam(::testing::_, ::testing::_))
        .Times(0);

    nlohmann::json teamRequestBody = {
        {"id", "bad id"},
        {"name", "Bad Team"}
    };

    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->UpdateTeam(teamRequest, "bad id");

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
    EXPECT_EQ("Invalid ID format", response.body);
}

// Prueba 15: Actualizar equipo con JSON malformado
TEST_F(TeamControllerTest, UpdateTeam_MalformedJSONTest) {
    EXPECT_CALL(*teamDelegateMock, UpdateTeam(::testing::_, ::testing::_))
        .Times(0);

    crow::request teamRequest;
    teamRequest.body = R"({malformed json})";

    crow::response response = teamController->UpdateTeam(teamRequest, "id-123");

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
    EXPECT_EQ("Invalid JSON", response.body);
}

// Prueba 16: Actualizar equipo con error en la base de datos
TEST_F(TeamControllerTest, UpdateTeam_DBErrorTest) {
    std::shared_ptr<domain::Team> capturedTeam;

    EXPECT_CALL(*teamDelegateMock, UpdateTeam(testing::Eq(std::string("id-123")), testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<1>(&capturedTeam),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    nlohmann::json teamRequestBody = {
        {"id", "id-123"},
        {"name", "Team"}
    };

    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->UpdateTeam(teamRequest, "id-123");

    EXPECT_EQ(crow::INTERNAL_SERVER_ERROR, response.code);
    EXPECT_EQ("Database connection failed", response.body);
}

// Prueba 17: Eliminar un equipo exitosamente
TEST_F(TeamControllerTest, DeleteTeam_SuccessTest) {
    EXPECT_CALL(*teamDelegateMock, DeleteTeam(testing::Eq(std::string("id-to-delete"))))
        .WillOnce(testing::Return(std::expected<void, std::string>()));

    crow::response response = teamController->DeleteTeam("id-to-delete");

    EXPECT_EQ(crow::NO_CONTENT, response.code);
}

// Prueba 18: Eliminar equipo que no existe
TEST_F(TeamControllerTest, DeleteTeam_NotFoundTest) {
    EXPECT_CALL(*teamDelegateMock, DeleteTeam(testing::Eq(std::string("no-id"))))
        .WillOnce(testing::Return(std::unexpected<std::string>("Team not found")));

    crow::response response = teamController->DeleteTeam("no-id");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
    EXPECT_EQ("Team not found", response.body);
}

// Prueba 19: Eliminar equipo con ID inválido
TEST_F(TeamControllerTest, DeleteTeam_InvalidIDTest) {
    EXPECT_CALL(*teamDelegateMock, DeleteTeam(::testing::_))
        .Times(0);

    crow::response response = teamController->DeleteTeam("invalid#id");

    EXPECT_EQ(crow::BAD_REQUEST, response.code);
    EXPECT_EQ("Invalid ID format", response.body);
}

// Prueba 20: Eliminar equipo con error en la base de datos
TEST_F(TeamControllerTest, DeleteTeam_DBErrorTest) {
    EXPECT_CALL(*teamDelegateMock, DeleteTeam(testing::Eq(std::string("id-123"))))
        .WillOnce(testing::Return(std::unexpected<std::string>("Database connection failed")));

    crow::response response = teamController->DeleteTeam("id-123");

    EXPECT_EQ(crow::INTERNAL_SERVER_ERROR, response.code);
    EXPECT_EQ("Database connection failed", response.body);
}
