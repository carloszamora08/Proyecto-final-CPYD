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

class TournamentRepositoryMock2 : public TournamentRepository {
public:
    TournamentRepositoryMock2() : TournamentRepository(nullptr) {}

    MOCK_METHOD((std::expected<std::shared_ptr<domain::Tournament>, std::string>), ReadById, (std::string id), (override));
};

class TeamRepositoryMock2 : public TeamRepository {
public:
    TeamRepositoryMock2() : TeamRepository(nullptr) {}

    MOCK_METHOD((std::expected<std::shared_ptr<domain::Team>, std::string>), ReadById, (std::string id), (override));
};

class GroupDelegateTest : public ::testing::Test{
protected:
    std::shared_ptr<TournamentRepositoryMock2> tournamentRepositoryMock2;
    std::shared_ptr<GroupRepositoryMock> groupRepositoryMock;
    std::shared_ptr<TeamRepositoryMock2> teamRepositoryMock2;
    std::shared_ptr<GroupDelegate> groupDelegate;

    void SetUp() override {
        tournamentRepositoryMock2 = std::make_shared<TournamentRepositoryMock2>();
        groupRepositoryMock = std::make_shared<GroupRepositoryMock>();
        teamRepositoryMock2 = std::make_shared<TeamRepositoryMock2>();
        groupDelegate = std::make_shared<GroupDelegate>(GroupDelegate(tournamentRepositoryMock2, groupRepositoryMock, teamRepositoryMock2));
    }

    // TearDown() function
    void TearDown() override {
        // teardown code comes here
    }

};

TEST_F(GroupDelegateTest, CreateGroupSucessTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock2, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string_view capturedTournamentIdGroupRepo;
    std::vector<std::shared_ptr<domain::Group>> existingGroups;
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroupRepo),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(existingGroups))
            )
        );

    EXPECT_CALL(*teamRepositoryMock2, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(::testing::_, ::testing::_))
        .Times(0);

    domain::Group capturedGroup;

    EXPECT_CALL(*groupRepositoryMock, Create(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedGroup),
                testing::Return(std::expected<std::string, std::string>("new-id"))
            )
        );

    nlohmann::json groupRequestBody = {{"id", "new-id"}, {"name", "new name"}, {"region", "new region"}, {"teams", nlohmann::json::array()}};
    const domain::Group group = groupRequestBody;
    std::string_view tournamentId = "tournament-id";
    auto response = groupDelegate->CreateGroup(tournamentId, group);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock2);
    testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId.data());
    EXPECT_EQ(capturedTournamentIdGroupRepo, tournamentId);
    EXPECT_EQ(capturedGroup.Id(), group.Id());
    EXPECT_EQ(capturedGroup.Name(), group.Name());
    EXPECT_EQ(capturedGroup.Region(), group.Region());
    EXPECT_EQ(capturedGroup.TournamentId(), tournamentId);
    EXPECT_EQ(capturedGroup.Teams().size(), group.Teams().size());
    EXPECT_EQ(*response, "new-id");
}

TEST_F(GroupDelegateTest, CreateGroupDBInsertionFailTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock2, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string_view capturedTournamentIdGroupRepo;
    std::vector<std::shared_ptr<domain::Group>> existingGroups;
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroupRepo),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(existingGroups))
            )
        );

    EXPECT_CALL(*teamRepositoryMock2, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(::testing::_, ::testing::_))
        .Times(0);

    domain::Group capturedGroup;
    
    EXPECT_CALL(*groupRepositoryMock, Create(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedGroup),
                testing::Return(std::unexpected<std::string>("Group insertion failed"))
            )
        );

    nlohmann::json groupRequestBody = {{"id", "existing-id"}, {"name", "existing name"}, {"region", "existing region"}, {"teams", nlohmann::json::array()}};
    const domain::Group group = groupRequestBody;
    std::string_view tournamentId = "tournament-id";
    auto response = groupDelegate->CreateGroup(tournamentId, group);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock2);
    testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId.data());
    EXPECT_EQ(capturedTournamentIdGroupRepo, tournamentId);
    EXPECT_EQ(capturedGroup.Id(), group.Id());
    EXPECT_EQ(capturedGroup.Name(), group.Name());
    EXPECT_EQ(capturedGroup.Region(), group.Region());
    EXPECT_EQ(capturedGroup.TournamentId(), tournamentId);
    EXPECT_EQ(capturedGroup.Teams().size(), group.Teams().size());
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Group insertion failed");
}

TEST_F(GroupDelegateTest, CreateGroupOverflowingTournamentTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock2, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string_view capturedTournamentIdGroupRepo;
    std::vector<std::shared_ptr<domain::Group>> existingGroups;
    for (int i = 0; i < 8; i++) {
        nlohmann::json groupData = {
            {"id", "group-id-" + std::to_string(i)}, 
            {"name", "Group " + std::to_string(i)}, 
            {"region", "Region " + std::to_string(i)}, 
            {"teams", nlohmann::json::array()}
        };
        auto group = std::make_shared<domain::Group>(groupData);
        existingGroups.push_back(group);
    }
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroupRepo),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(existingGroups))
            )
        );

    EXPECT_CALL(*teamRepositoryMock2, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(::testing::_, ::testing::_))
        .Times(0);
    
    EXPECT_CALL(*groupRepositoryMock, Create(::testing::_))
        .Times(0);

    nlohmann::json groupRequestBody = {{"id", "new-id"}, {"name", "new name"}, {"region", "new region"}, {"teams", nlohmann::json::array()}};
    const domain::Group group = groupRequestBody;
    std::string_view tournamentId = "tournament-id";
    auto response = groupDelegate->CreateGroup(tournamentId, group);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock2);
    testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId.data());
    EXPECT_EQ(capturedTournamentIdGroupRepo, tournamentId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Tournament has reached maximum number of groups");
}

TEST_F(GroupDelegateTest, CreateGroupTournamentFailTest) {
    std::string capturedTournamentIdTournamentRepo;
    EXPECT_CALL(*tournamentRepositoryMock2, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::unexpected<std::string>("Tournament does not exist"))
            )
        );

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentId(::testing::_))
        .Times(0);

    EXPECT_CALL(*teamRepositoryMock2, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(::testing::_, ::testing::_))
        .Times(0);
    
    EXPECT_CALL(*groupRepositoryMock, Create(::testing::_))
        .Times(0);

    nlohmann::json groupRequestBody = {{"id", "new-id"}, {"name", "new name"}, {"region", "new region"}, {"teams", nlohmann::json::array()}};
    const domain::Group group = groupRequestBody;
    std::string_view tournamentId = "non-existing-tournament-id";
    auto response = groupDelegate->CreateGroup(tournamentId, group);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock2);
    testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId.data());
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Tournament does not exist");
}

TEST_F(GroupDelegateTest, CreateGroupExistingGroupsFailTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock2, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string_view capturedTournamentIdGroupRepo;
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroupRepo),
                testing::Return(std::unexpected<std::string>("Tournament groups read failed"))
            )
        );

    EXPECT_CALL(*teamRepositoryMock2, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(::testing::_, ::testing::_))
        .Times(0);
    
    EXPECT_CALL(*groupRepositoryMock, Create(::testing::_))
        .Times(0);

    nlohmann::json groupRequestBody = {{"id", "new-id"}, {"name", "new name"}, {"region", "new region"}, {"teams", nlohmann::json::array()}};
    const domain::Group group = groupRequestBody;
    std::string_view tournamentId = "tournament-id";
    auto response = groupDelegate->CreateGroup(tournamentId, group);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock2);
    testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId.data());
    EXPECT_EQ(capturedTournamentIdGroupRepo, tournamentId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Tournament groups read failed");
}

TEST_F(GroupDelegateTest, CreateGroupOverflowingGroupTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock2, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string_view capturedTournamentIdGroupRepo;
    std::vector<std::shared_ptr<domain::Group>> existingGroups;
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroupRepo),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(existingGroups))
            )
        );

    EXPECT_CALL(*teamRepositoryMock2, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(::testing::_, ::testing::_))
        .Times(0);
    
    EXPECT_CALL(*groupRepositoryMock, Create(::testing::_))
        .Times(0);

    nlohmann::json groupRequestBody = {{"id", "new-id"}, {"name", "new name"}, {"region", "new region"}, {"teams", nlohmann::json::array()}};
    for (int i = 0; i < 5; i++) {
        groupRequestBody["teams"].push_back({
            {"id", "team-id-" + std::to_string(i)},
            {"name", "Team " + std::to_string(i)}
        });
    }
    const domain::Group group = groupRequestBody;
    std::string_view tournamentId = "tournament-id";
    auto response = groupDelegate->CreateGroup(tournamentId, group);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock2);
    testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId.data());
    EXPECT_EQ(capturedTournamentIdGroupRepo, tournamentId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Group exceeds maximum teams capacity");
}

TEST_F(GroupDelegateTest, CreateGroupInvalidTeamTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock2, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string_view capturedTournamentIdGroupRepo;
    std::vector<std::shared_ptr<domain::Group>> existingGroups;
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroupRepo),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(existingGroups))
            )
        );

    std::vector<std::string> capturedTeamIds;
    nlohmann::json team1Data = {
        {"id", "team-id-0"},
        {"name", "Team 0"}
    };
    auto team1 = std::make_shared<domain::Team>(team1Data);
    EXPECT_CALL(*teamRepositoryMock2, ReadById(::testing::_))
        .Times(2)
        .WillOnce(testing::DoAll(
                testing::Invoke([&capturedTeamIds](const std::string& id) {
                    capturedTeamIds.push_back(id);
                }),
                testing::Return(std::expected<std::shared_ptr<domain::Team>, std::string>(team1))
            )
        )
        .WillOnce(testing::DoAll(
                testing::Invoke([&capturedTeamIds](const std::string& id) {
                    capturedTeamIds.push_back(id);
                }),
                testing::Return(std::unexpected<std::string>("Team does not exist"))
            )
        );

    std::string_view capturedTournamentIdFindBy;
    std::string_view capturedTeamIdFindBy;
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdFindBy),
                testing::SaveArg<1>(&capturedTeamIdFindBy),
                testing::Return(std::unexpected<std::string>("Group not found"))
            )
        );
    
    EXPECT_CALL(*groupRepositoryMock, Create(::testing::_))
        .Times(0);

    nlohmann::json groupRequestBody = {{"id", "new-id"}, {"name", "new name"}, {"region", "new region"}, {"teams", nlohmann::json::array()}};
    for (int i = 0; i < 2; i++) {
        groupRequestBody["teams"].push_back({
            {"id", "team-id-" + std::to_string(i)},
            {"name", "Team " + std::to_string(i)}
        });
    }
    const domain::Group group = groupRequestBody;
    std::string_view tournamentId = "tournament-id";
    auto response = groupDelegate->CreateGroup(tournamentId, group);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock2);
    testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId.data());
    EXPECT_EQ(capturedTournamentIdGroupRepo, tournamentId);
    EXPECT_EQ(capturedTeamIds.size(), 2);
    EXPECT_EQ(capturedTeamIds[0], groupRequestBody["teams"][0]["id"].get<std::string>());
    EXPECT_EQ(capturedTeamIds[1], groupRequestBody["teams"][1]["id"].get<std::string>());
    EXPECT_EQ(capturedTournamentIdFindBy, tournamentId);
    EXPECT_EQ(capturedTeamIdFindBy, groupRequestBody["teams"][0]["id"].get<std::string>());
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Team does not exist");
}

TEST_F(GroupDelegateTest, CreateGroupExistingTeamFailTest) {
    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock2, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    std::string_view capturedTournamentIdGroupRepo;
    std::vector<std::shared_ptr<domain::Group>> existingGroups;
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroupRepo),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(existingGroups))
            )
        );

    std::vector<std::string> capturedTeamIds;
    nlohmann::json team1Data = {
        {"id", "team-id-0"},
        {"name", "Team 0"}
    };
    auto team1 = std::make_shared<domain::Team>(team1Data);
    nlohmann::json team2Data = {
        {"id", "team-id-1"},
        {"name", "Team 1"}
    };
    auto team2 = std::make_shared<domain::Team>(team2Data);
    EXPECT_CALL(*teamRepositoryMock2, ReadById(::testing::_))
        .Times(2)
        .WillOnce(testing::DoAll(
                testing::Invoke([&capturedTeamIds](const std::string& id) {
                    capturedTeamIds.push_back(id);
                }),
                testing::Return(std::expected<std::shared_ptr<domain::Team>, std::string>(team1))
            )
        )
        .WillOnce(testing::DoAll(
                testing::Invoke([&capturedTeamIds](const std::string& id) {
                    capturedTeamIds.push_back(id);
                }),
                testing::Return(std::expected<std::shared_ptr<domain::Team>, std::string>(team2))
            )
        );

    std::string_view capturedTournamentIdFindBy1;
    std::string_view capturedTeamIdFindBy1;
    std::string_view capturedTournamentIdFindBy2;
    std::string_view capturedTeamIdFindBy2;
    nlohmann::json foundGroupData = {
        {"id", "existing-group-id"},
        {"name", "Existing Group"},
        {"region", "Existing Region"},
        {"teams", nlohmann::json::array({
            {
                {"id", "team-id-1"},
                {"name", "Team 1"}
            }
        })}
    };
    auto foundGroup = std::make_shared<domain::Group>(foundGroupData);
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdFindBy1),
                testing::SaveArg<1>(&capturedTeamIdFindBy1),
                testing::Return(std::unexpected<std::string>("Group not found"))
            )
        )
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdFindBy2),
                testing::SaveArg<1>(&capturedTeamIdFindBy2),
                testing::Return(std::expected<std::shared_ptr<domain::Group>, std::string>(foundGroup))
            )
        );
    
    EXPECT_CALL(*groupRepositoryMock, Create(::testing::_))
        .Times(0);

    nlohmann::json groupRequestBody = {{"id", "new-id"}, {"name", "new name"}, {"region", "new region"}, {"teams", nlohmann::json::array()}};
    for (int i = 0; i < 2; i++) {
        groupRequestBody["teams"].push_back({
            {"id", "team-id-" + std::to_string(i)},
            {"name", "Team " + std::to_string(i)}
        });
    }
    const domain::Group group = groupRequestBody;
    std::string_view tournamentId = "tournament-id";
    auto response = groupDelegate->CreateGroup(tournamentId, group);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock2);
    testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId.data());
    EXPECT_EQ(capturedTournamentIdGroupRepo, tournamentId);
    EXPECT_EQ(capturedTeamIds.size(), 2);
    EXPECT_EQ(capturedTeamIds[0], groupRequestBody["teams"][0]["id"].get<std::string>());
    EXPECT_EQ(capturedTeamIds[1], groupRequestBody["teams"][1]["id"].get<std::string>());
    EXPECT_EQ(capturedTournamentIdFindBy1, tournamentId);
    EXPECT_EQ(capturedTeamIdFindBy1, groupRequestBody["teams"][0]["id"].get<std::string>());
    EXPECT_EQ(capturedTournamentIdFindBy2, tournamentId);
    EXPECT_EQ(capturedTeamIdFindBy2, groupRequestBody["teams"][1]["id"].get<std::string>());
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Team team-id-1 already exists in another group");
}

TEST_F(GroupDelegateTest, GetGroupsSuccessTest) {
    std::string_view capturedTournamentId;
    std::vector<std::shared_ptr<domain::Group>> existingGroups;
    for (int i = 0; i < 2; i++) {
        nlohmann::json groupData = {
            {"id", "group-id-" + std::to_string(i)}, 
            {"name", "Group " + std::to_string(i)}, 
            {"region", "Region " + std::to_string(i)}, 
            {"teams", nlohmann::json::array()}
        };
        auto group = std::make_shared<domain::Group>(groupData);
        existingGroups.push_back(group);
    }
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(existingGroups))
            )
        );

    std::string_view tournamentId = "tournament-id";
    auto response = groupDelegate->GetGroups(tournamentId);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);

    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_TRUE(response.has_value());
    EXPECT_EQ(response.value().size(), 2);
    EXPECT_EQ(response.value()[0]->Id(), "group-id-0");
    EXPECT_EQ(response.value()[0]->Name(), "Group 0");
    EXPECT_EQ(response.value()[0]->Region(), "Region 0");
    EXPECT_EQ(response.value()[0]->Teams().size(), 0);
    EXPECT_EQ(response.value()[1]->Id(), "group-id-1");
    EXPECT_EQ(response.value()[1]->Name(), "Group 1");
    EXPECT_EQ(response.value()[1]->Region(), "Region 1");
    EXPECT_EQ(response.value()[1]->Teams().size(), 0);
}

TEST_F(GroupDelegateTest, GetGroupsEmptyTest) {
    std::string_view capturedTournamentId;
    std::vector<std::shared_ptr<domain::Group>> existingGroups;
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(existingGroups))
            )
        );

    std::string_view tournamentId = "tournament-id";
    auto response = groupDelegate->GetGroups(tournamentId);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);

    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_TRUE(response.has_value());
    EXPECT_TRUE(response.value().empty());
}

TEST_F(GroupDelegateTest, GetGroupsFailTest) {
    std::string_view capturedTournamentId;
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentId(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::Return(std::unexpected<std::string>("Database connection failed"))
            )
        );

    std::string_view tournamentId = "tournament-id";
    auto response = groupDelegate->GetGroups(tournamentId);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);

    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Database connection failed");
}