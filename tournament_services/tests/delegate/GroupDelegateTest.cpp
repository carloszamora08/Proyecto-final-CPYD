#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Group.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "delegate/GroupDelegate.hpp"
#include "domain/Utilities.hpp"

class GroupRepositoryMock : public GroupRepository {
public:
    GroupRepositoryMock() : GroupRepository(nullptr) {}

    MOCK_METHOD((std::expected<std::string, std::string>), Create, (const domain::Group& entity), (override));
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>), ReadAll, (), (override));
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Group>, std::string>), ReadById, (std::string id), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), Update, (std::string id, const domain::Group& entity), (override));
    MOCK_METHOD((std::expected<void, std::string>), Delete, (std::string id), (override));
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>), FindByTournamentId, (const std::string_view& tournamentId), (override));
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Group>, std::string>), FindByTournamentIdAndGroupId, (const std::string_view& tournamentId, const std::string_view& groupId), (override));
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Group>, std::string>), FindByTournamentIdAndTeamId, (const std::string_view& tournamentId, const std::string_view& teamId), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateGroupAddTeam, (const std::string_view& groupId, const std::shared_ptr<domain::Team>& team), (override));
};

class TournamentRepositoryMock : public TournamentRepository {
public:
    TournamentRepositoryMock() : TournamentRepository(nullptr) {}
};

class TeamRepositoryMock : public TeamRepository {
public:
    TeamRepositoryMock() : TeamRepository(nullptr) {}
};

class GroupDelegateTest : public ::testing::Test{
protected:
    std::shared_ptr<TournamentRepositoryMock> tournamentRepositoryMock;
    std::shared_ptr<GroupRepositoryMock> groupRepositoryMock;
    std::shared_ptr<TeamRepositoryMock> teamRepositoryMock;
    std::shared_ptr<GroupDelegate> groupDelegate;

    void SetUp() override {
        tournamentRepositoryMock = std::make_shared<TournamentRepositoryMock>();
        groupRepositoryMock = std::make_shared<GroupRepositoryMock>();
        teamRepositoryMock = std::make_shared<TeamRepositoryMock>();
        groupDelegate = std::make_shared<GroupDelegate>(GroupDelegate(tournamentRepositoryMock, groupRepositoryMock, teamRepositoryMock));
    }

    // TearDown() function
    void TearDown() override {
        // teardown code comes here
    }

};

TEST_F(GroupDelegateTest, CreateSucessTest) {
    
}