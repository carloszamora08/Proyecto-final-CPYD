//
// Created by root on 10/19/25.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <expected>

#include "domain/Team.hpp"
#include "persistence/repository/IRepository.hpp"
#include "delegate/TeamDelegate.hpp"

// Mock del repositorio
class TeamRepositoryMock : public IRepository<domain::Team, std::string, std::expected<std::string, std::string>> {
public:
    MOCK_METHOD((std::expected<std::string, std::string>), Create, (const domain::Team& entity), (override));
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Team>>, std::string>), ReadAll, (), (override));
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Team>, std::string>), ReadById, (std::string id), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), Update, (std::string id, const domain::Team& entity), (override));
    MOCK_METHOD((std::expected<void, std::string>), Delete, (std::string id), (override));
};

class TeamDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<TeamRepositoryMock> teamRepositoryMock;
    std::shared_ptr<TeamDelegate> teamDelegate;

    void SetUp() override {
        teamRepositoryMock = std::make_shared<TeamRepositoryMock>();
        teamDelegate = std::make_shared<TeamDelegate>(teamRepositoryMock);
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};


// Test 1
TEST_F(TeamDelegateTest, CreateTeamSuccessTest) {
    domain::Team capturedTeam;

    EXPECT_CALL(*teamRepositoryMock, Create(::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&capturedTeam),
            ::testing::Return(std::expected<std::string, std::string>("generated-team-id"))
        ));

    auto team = std::make_shared<domain::Team>();
    team->Id = "";
    team->Name = "Test Team";

    auto result = teamDelegate->CreateTeam(team);

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "generated-team-id");
    EXPECT_EQ(capturedTeam.Name, "Test Team");
}

// Test 2
TEST_F(TeamDelegateTest, CreateTeamFailureTest) {
    domain::Team capturedTeam;

    EXPECT_CALL(*teamRepositoryMock, Create(::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&capturedTeam),
            ::testing::Return(std::unexpected<std::string>("Team insertion failed"))
        ));

    auto team = std::make_shared<domain::Team>();
    team->Id = "";
    team->Name = "Duplicate Team";

    auto result = teamDelegate->CreateTeam(team);

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Team insertion failed");
    EXPECT_EQ(capturedTeam.Name, "Duplicate Team");
}

// Test 3
TEST_F(TeamDelegateTest, GetTeamSuccessTest) {
    std::string capturedId;
    auto expectedTeam = std::make_shared<domain::Team>();
    expectedTeam->Id = "team-123";
    expectedTeam->Name = "Found Team";

    EXPECT_CALL(*teamRepositoryMock, ReadById(::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&capturedId),
            ::testing::Return(std::expected<std::shared_ptr<domain::Team>, std::string>(expectedTeam))
        ));

    auto result = teamDelegate->GetTeam("team-123");

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(capturedId, "team-123");
    EXPECT_EQ(result.value()->Id, "team-123");
    EXPECT_EQ(result.value()->Name, "Found Team");
}

// Test 4
TEST_F(TeamDelegateTest, GetTeamNotFoundTest) {
    std::string capturedId;

    EXPECT_CALL(*teamRepositoryMock, ReadById(::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&capturedId),
            ::testing::Return(std::unexpected<std::string>("Team not found"))
        ));

    auto result = teamDelegate->GetTeam("non-existent-id");

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(capturedId, "non-existent-id");
    EXPECT_EQ(result.error(), "Team not found");
}

// Test 5
TEST_F(TeamDelegateTest, ReadAllTeamsWithResultsTest) {
    std::vector<std::shared_ptr<domain::Team>> expectedTeams;

    auto team1 = std::make_shared<domain::Team>();
    team1->Id = "team-1";
    team1->Name = "Team One";

    auto team2 = std::make_shared<domain::Team>();
    team2->Id = "team-2";
    team2->Name = "Team Two";

    expectedTeams.push_back(team1);
    expectedTeams.push_back(team2);

    EXPECT_CALL(*teamRepositoryMock, ReadAll())
        .WillOnce(::testing::Return(
            std::expected<std::vector<std::shared_ptr<domain::Team>>, std::string>(expectedTeams)
        ));

    auto result = teamDelegate->ReadAll();

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 2);
    EXPECT_EQ(result.value()[0]->Id, "team-1");
    EXPECT_EQ(result.value()[0]->Name, "Team One");
    EXPECT_EQ(result.value()[1]->Id, "team-2");
    EXPECT_EQ(result.value()[1]->Name, "Team Two");
}

// Test 6
TEST_F(TeamDelegateTest, ReadAllTeamsEmptyListTest) {
    std::vector<std::shared_ptr<domain::Team>> emptyList;

    EXPECT_CALL(*teamRepositoryMock, ReadAll())
        .WillOnce(::testing::Return(
            std::expected<std::vector<std::shared_ptr<domain::Team>>, std::string>(emptyList)
        ));

    auto result = teamDelegate->ReadAll();

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

// Test 7
TEST_F(TeamDelegateTest, UpdateTeamSuccessTest) {
    std::string capturedId;
    domain::Team capturedTeam;

    EXPECT_CALL(*teamRepositoryMock, Update(::testing::_, ::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&capturedId),
            ::testing::SaveArg<1>(&capturedTeam),
            ::testing::Return(std::expected<std::string, std::string>("team-456"))
        ));

    auto team = std::make_shared<domain::Team>();
    team->Id = "team-456";
    team->Name = "Updated Team Name";

    auto result = teamDelegate->UpdateTeam("team-456", team);

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "team-456");
    EXPECT_EQ(capturedId, "team-456");
    EXPECT_EQ(capturedTeam.Name, "Updated Team Name");
}

// Test 8
TEST_F(TeamDelegateTest, UpdateTeamNotFoundTest) {
    std::string capturedId;
    domain::Team capturedTeam;

    EXPECT_CALL(*teamRepositoryMock, Update(::testing::_, ::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&capturedId),
            ::testing::SaveArg<1>(&capturedTeam),
            ::testing::Return(std::unexpected<std::string>("Team not found"))
        ));

    auto team = std::make_shared<domain::Team>();
    team->Id = "non-existent-id";
    team->Name = "Updated Team Name";

    auto result = teamDelegate->UpdateTeam("non-existent-id", team);

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Team not found");
    EXPECT_EQ(capturedId, "non-existent-id");
    EXPECT_EQ(capturedTeam.Name, "Updated Team Name");
}



// Test 9: Creación fallida - error SQL por clave duplicada
TEST_F(TeamDelegateTest, CreateTeamDuplicateKeyErrorTest) {
    domain::Team capturedTeam;

    EXPECT_CALL(*teamRepositoryMock, Create(::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&capturedTeam),
            ::testing::Return(std::unexpected<std::string>("SQL error: duplicate key value violates unique constraint"))
        ));

    auto team = std::make_shared<domain::Team>();
    team->Id = "";
    team->Name = "Duplicate Team";

    auto result = teamDelegate->CreateTeam(team);

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "SQL error: duplicate key value violates unique constraint");
}

// Test 10: Creación fallida - error de conexión a base de datos
TEST_F(TeamDelegateTest, CreateTeamDatabaseConnectionErrorTest) {
    domain::Team capturedTeam;

    EXPECT_CALL(*teamRepositoryMock, Create(::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&capturedTeam),
            ::testing::Return(std::unexpected<std::string>("Database error: connection lost"))
        ));

    auto team = std::make_shared<domain::Team>();
    team->Id = "";
    team->Name = "Test Team";

    auto result = teamDelegate->CreateTeam(team);

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Database error: connection lost");
}

// Test 11: Creación fallida - error de timeout en base de datos
TEST_F(TeamDelegateTest, CreateTeamDatabaseTimeoutErrorTest) {
    EXPECT_CALL(*teamRepositoryMock, Create(::testing::_))
        .WillOnce(::testing::Return(std::unexpected<std::string>("Database error: connection timeout")));

    auto team = std::make_shared<domain::Team>();
    team->Id = "";
    team->Name = "Timeout Team";

    auto result = teamDelegate->CreateTeam(team);

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Database error: connection timeout");
}

// Test 12: Búsqueda por ID - error SQL en consulta
TEST_F(TeamDelegateTest, GetTeamSQLErrorTest) {
    std::string capturedId;

    EXPECT_CALL(*teamRepositoryMock, ReadById(::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&capturedId),
            ::testing::Return(std::unexpected<std::string>("SQL error: invalid query syntax"))
        ));

    auto result = teamDelegate->GetTeam("team-invalid");

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "SQL error: invalid query syntax");
}

// Test 13: Búsqueda por ID - error de conexión a base de datos
TEST_F(TeamDelegateTest, GetTeamDatabaseConnectionErrorTest) {
    EXPECT_CALL(*teamRepositoryMock, ReadById(::testing::_))
        .WillOnce(::testing::Return(std::unexpected<std::string>("Database error: connection failed")));

    auto result = teamDelegate->GetTeam("team-123");

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Database error: connection failed");
}

// Test 14: Búsqueda de todos - lista con múltiples equipos
TEST_F(TeamDelegateTest, ReadAllTeamsMultipleResultsTest) {
    std::vector<std::shared_ptr<domain::Team>> expectedTeams;

    for (int i = 1; i <= 5; i++) {
        auto team = std::make_shared<domain::Team>();
        team->Id = "team-" + std::to_string(i);
        team->Name = "Team " + std::to_string(i);
        expectedTeams.push_back(team);
    }

    EXPECT_CALL(*teamRepositoryMock, ReadAll())
        .WillOnce(::testing::Return(
            std::expected<std::vector<std::shared_ptr<domain::Team>>, std::string>(expectedTeams)
        ));

    auto result = teamDelegate->ReadAll();

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 5);
    EXPECT_EQ(result.value()[0]->Id, "team-1");
    EXPECT_EQ(result.value()[4]->Id, "team-5");
}

// Test 15: Búsqueda de todos - error de timeout en base de datos
TEST_F(TeamDelegateTest, ReadAllTeamsDatabaseTimeoutTest) {
    EXPECT_CALL(*teamRepositoryMock, ReadAll())
        .WillOnce(::testing::Return(
            std::unexpected<std::string>("Database error: connection timeout")
        ));

    auto result = teamDelegate->ReadAll();

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Database error: connection timeout");
}

// Test 16: Búsqueda de todos - error SQL (tabla no existe)
TEST_F(TeamDelegateTest, ReadAllTeamsSQLTableNotExistsTest) {
    EXPECT_CALL(*teamRepositoryMock, ReadAll())
        .WillOnce(::testing::Return(
            std::unexpected<std::string>("SQL error: table 'teams' does not exist")
        ));

    auto result = teamDelegate->ReadAll();

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "SQL error: table 'teams' does not exist");
}

// Test 17: Búsqueda de todos - error de permisos
TEST_F(TeamDelegateTest, ReadAllTeamsPermissionDeniedTest) {
    EXPECT_CALL(*teamRepositoryMock, ReadAll())
        .WillOnce(::testing::Return(
            std::unexpected<std::string>("Database error: permission denied")
        ));

    auto result = teamDelegate->ReadAll();

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Database error: permission denied");
}

// Test 18: Actualización - error SQL por violación de constraint
TEST_F(TeamDelegateTest, UpdateTeamSQLConstraintViolationTest) {
    std::string capturedId;
    domain::Team capturedTeam;

    EXPECT_CALL(*teamRepositoryMock, Update(::testing::_, ::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&capturedId),
            ::testing::SaveArg<1>(&capturedTeam),
            ::testing::Return(std::unexpected<std::string>("SQL error: constraint violation"))
        ));

    auto team = std::make_shared<domain::Team>();
    team->Id = "team-789";
    team->Name = "Invalid Name";

    auto result = teamDelegate->UpdateTeam("team-789", team);

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "SQL error: constraint violation");
}

// Test 19: Actualización - error de rollback de transacción
TEST_F(TeamDelegateTest, UpdateTeamTransactionRollbackErrorTest) {
    EXPECT_CALL(*teamRepositoryMock, Update(::testing::_, ::testing::_))
        .WillOnce(::testing::Return(
            std::unexpected<std::string>("Database error: transaction rollback")
        ));

    auto team = std::make_shared<domain::Team>();
    team->Id = "team-123";
    team->Name = "Team Name";

    auto result = teamDelegate->UpdateTeam("team-123", team);

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Database error: transaction rollback");
}

// Test 20: Actualización - error de conexión perdida
TEST_F(TeamDelegateTest, UpdateTeamConnectionLostErrorTest) {
    EXPECT_CALL(*teamRepositoryMock, Update(::testing::_, ::testing::_))
        .WillOnce(::testing::Return(
            std::unexpected<std::string>("Database error: connection lost")
        ));

    auto team = std::make_shared<domain::Team>();
    team->Id = "team-999";
    team->Name = "Team Name";

    auto result = teamDelegate->UpdateTeam("team-999", team);

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Database error: connection lost");
}

// Test 21: Eliminación exitosa de equipo
TEST_F(TeamDelegateTest, DeleteTeamSuccessTest) {
    std::string capturedId;

    EXPECT_CALL(*teamRepositoryMock, Delete(::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&capturedId),
            ::testing::Return(std::expected<void, std::string>())
        ));

    auto result = teamDelegate->DeleteTeam("team-delete-789");

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(capturedId, "team-delete-789");
}

// Test 22: Eliminación fallida - equipo no encontrado
TEST_F(TeamDelegateTest, DeleteTeamNotFoundTest) {
    std::string capturedId;

    EXPECT_CALL(*teamRepositoryMock, Delete(::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&capturedId),
            ::testing::Return(std::unexpected<std::string>("Team not found"))
        ));

    auto result = teamDelegate->DeleteTeam("non-existent-team-id");

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Team not found");
    EXPECT_EQ(capturedId, "non-existent-team-id");
}

// Test 23: Eliminación fallida - violación de foreign key
TEST_F(TeamDelegateTest, DeleteTeamForeignKeyViolationTest) {
    std::string capturedId;

    EXPECT_CALL(*teamRepositoryMock, Delete(::testing::_))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<0>(&capturedId),
            ::testing::Return(std::unexpected<std::string>("SQL error: foreign key constraint violation"))
        ));

    auto result = teamDelegate->DeleteTeam("team-with-references");

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "SQL error: foreign key constraint violation");
}

// Test 24: Eliminación fallida - error de conexión a base de datos
TEST_F(TeamDelegateTest, DeleteTeamDatabaseConnectionErrorTest) {
    EXPECT_CALL(*teamRepositoryMock, Delete(::testing::_))
        .WillOnce(::testing::Return(
            std::unexpected<std::string>("Database error: connection failed")
        ));

    auto result = teamDelegate->DeleteTeam("team-123");

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Database error: connection failed");
}

// Test 25: Eliminación fallida - error de permisos
TEST_F(TeamDelegateTest, DeleteTeamPermissionDeniedTest) {
    EXPECT_CALL(*teamRepositoryMock, Delete(::testing::_))
        .WillOnce(::testing::Return(
            std::unexpected<std::string>("Database error: permission denied for table teams")
        ));

    auto result = teamDelegate->DeleteTeam("team-456");

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Database error: permission denied for table teams");
}

// Test 26: Eliminación fallida - error de timeout
TEST_F(TeamDelegateTest, DeleteTeamTimeoutErrorTest) {
    EXPECT_CALL(*teamRepositoryMock, Delete(::testing::_))
        .WillOnce(::testing::Return(
            std::unexpected<std::string>("Database error: operation timeout")
        ));

    auto result = teamDelegate->DeleteTeam("team-slow-delete");

    ::testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Database error: operation timeout");
}