#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Group.hpp"
#include "cms/QueueMessageProducer.hpp"
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

class QueueMessageProducerMock : public QueueMessageProducer {
public:
    QueueMessageProducerMock(): QueueMessageProducer(nullptr) {}

    MOCK_METHOD(void, SendMessage, (const std::string_view& message, const std::string_view& queue), (override));
};

class GroupDelegateTest : public ::testing::Test{
protected:
    std::shared_ptr<TournamentRepositoryMock2> tournamentRepositoryMock2;
    std::shared_ptr<GroupRepositoryMock> groupRepositoryMock;
    std::shared_ptr<TeamRepositoryMock2> teamRepositoryMock2;
    std::shared_ptr<QueueMessageProducerMock> producerMock;
    std::shared_ptr<GroupDelegate> groupDelegate;

    void SetUp() override {
        tournamentRepositoryMock2 = std::make_shared<TournamentRepositoryMock2>();
        groupRepositoryMock = std::make_shared<GroupRepositoryMock>();
        teamRepositoryMock2 = std::make_shared<TeamRepositoryMock2>();
        producerMock = std::make_shared<QueueMessageProducerMock>();
        groupDelegate = std::make_shared<GroupDelegate>(GroupDelegate(tournamentRepositoryMock2, groupRepositoryMock, teamRepositoryMock2, producerMock));
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

TEST_F(GroupDelegateTest, GetGroupSuccessTest) {
    std::string_view capturedTournamentId;
    std::string_view capturedGroupId;
    nlohmann::json groupData = {
        {"id", "group-id"},
        {"name", "Test Group"},
        {"region", "Test Region"},
        {"teams", nlohmann::json::array()}
    };
    auto group = std::make_shared<domain::Group>(groupData);
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::SaveArg<1>(&capturedGroupId),
                testing::Return(std::expected<std::shared_ptr<domain::Group>, std::string>(group))
            )
        );

    std::string_view tournamentId = "tournament-id";
    std::string_view groupId = "group-id";
    auto response = groupDelegate->GetGroup(tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);

    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedGroupId, groupId);
    EXPECT_TRUE(response.has_value());
    EXPECT_EQ(response.value()->Id(), groupData["id"].get<std::string>());
    EXPECT_EQ(response.value()->Name(), groupData["name"].get<std::string>());
    EXPECT_EQ(response.value()->Region(), groupData["region"].get<std::string>());
    EXPECT_EQ(response.value()->Teams().size(), 0);
}

TEST_F(GroupDelegateTest, GetGroupDBSelectionFailTest) {
    std::string_view capturedTournamentId;
    std::string_view capturedGroupId;
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::SaveArg<1>(&capturedGroupId),
                testing::Return(std::unexpected<std::string>("Group not found"))
            )
        );

    std::string_view tournamentId = "tournament-id";
    std::string_view groupId = "group-id";
    auto response = groupDelegate->GetGroup(tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);

    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedGroupId, groupId);
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Group not found");
}

TEST_F(GroupDelegateTest, UpdateGroupSuccessTest) {
    std::string capturedTournamentIdGroupFindBy;
    std::string capturedGroupIdGroupFindBy;
    nlohmann::json groupData = {
        {"id", "update-id"},
        {"name", "Test Group"},
        {"region", "Test Region"},
        {"teams", nlohmann::json::array()}
    };
    auto returnGroup = std::make_shared<domain::Group>(groupData);
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroupFindBy),
                testing::SaveArg<1>(&capturedGroupIdGroupFindBy),
                testing::Return(std::expected<std::shared_ptr<domain::Group>, std::string>(returnGroup))
            )
        );

    EXPECT_CALL(*tournamentRepositoryMock2, ReadById(::testing::_))
        .Times(0);
    
    EXPECT_CALL(*teamRepositoryMock2, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(::testing::_, ::testing::_))
        .Times(0);

    std::string capturedGroupIdGroupUpdate;
    domain::Group capturedGroupGroupUpdate;
    EXPECT_CALL(*groupRepositoryMock, Update(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedGroupIdGroupUpdate),
                testing::SaveArg<1>(&capturedGroupGroupUpdate),
                testing::Return(std::expected<std::string, std::string>("update-id"))
            )
        );
    
    std::string_view tournamentId = "tournament-id";
    nlohmann::json groupRequestBody = {{"id", "update-id"}, {"name", "update name"}, {"region", "update region"}};
    domain::Group group = groupRequestBody;
    group.Id() = "update-id";
    bool updateTeams = false;
    auto response = groupDelegate->UpdateGroup(tournamentId, group, updateTeams);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock2);
    testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdGroupFindBy, tournamentId.data());
    EXPECT_EQ(capturedGroupIdGroupFindBy, groupRequestBody["id"].get<std::string>());
    EXPECT_EQ(capturedGroupIdGroupUpdate, groupRequestBody["id"].get<std::string>());
    EXPECT_EQ(capturedGroupGroupUpdate.Id(), groupRequestBody["id"].get<std::string>());
    EXPECT_EQ(capturedGroupGroupUpdate.Name(), groupRequestBody["name"].get<std::string>());
    EXPECT_EQ(capturedGroupGroupUpdate.Region(), groupRequestBody["region"].get<std::string>());
    EXPECT_EQ(capturedGroupGroupUpdate.Teams().size(), 0);
    EXPECT_TRUE(response.has_value());
}

TEST_F(GroupDelegateTest, UpdateGroupFailTest) {
    std::string capturedTournamentIdGroupFindBy;
    std::string capturedGroupIdGroupFindBy;
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroupFindBy),
                testing::SaveArg<1>(&capturedGroupIdGroupFindBy),
                testing::Return(std::unexpected<std::string>("Group not found"))
            )
        );

    EXPECT_CALL(*tournamentRepositoryMock2, ReadById(::testing::_))
        .Times(0);
    
    EXPECT_CALL(*teamRepositoryMock2, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(::testing::_, ::testing::_))
        .Times(0);

    EXPECT_CALL(*groupRepositoryMock, Update(::testing::_, ::testing::_))
        .Times(0);
    
    std::string_view tournamentId = "tournament-id";
    nlohmann::json groupRequestBody = {{"id", "update-id"}, {"name", "update name"}, {"region", "update region"}};
    domain::Group group = groupRequestBody;
    group.Id() = "update-id";
    bool updateTeams = false;
    auto response = groupDelegate->UpdateGroup(tournamentId, group, updateTeams);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock2);
    testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdGroupFindBy, tournamentId.data());
    EXPECT_EQ(capturedGroupIdGroupFindBy, groupRequestBody["id"].get<std::string>());
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Group not found");
}

TEST_F(GroupDelegateTest, RemoveGroupSuccessTest) {
    std::string capturedTournamentIdGroupFindBy;
    std::string capturedGroupIdGroupFindBy;
    nlohmann::json groupData = {
        {"id", "delete-group-id"},
        {"name", "Test Group"},
        {"region", "Test Region"},
        {"teams", nlohmann::json::array()}
    };
    auto returnGroup = std::make_shared<domain::Group>(groupData);
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroupFindBy),
                testing::SaveArg<1>(&capturedGroupIdGroupFindBy),
                testing::Return(std::expected<std::shared_ptr<domain::Group>, std::string>(returnGroup))
            )
        );

    std::string capturedGroupIdGroupDelete;
    EXPECT_CALL(*groupRepositoryMock, Delete(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedGroupIdGroupDelete),
                testing::Return(std::expected<void, std::string>())
            )
        );

    std::string_view tournamentId = "tournament-id";
    std::string_view groupId = "delete-group-id";
    auto response = groupDelegate->RemoveGroup(tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);

    EXPECT_EQ(capturedTournamentIdGroupFindBy, tournamentId.data());
    EXPECT_EQ(capturedGroupIdGroupFindBy, groupId.data());
    EXPECT_EQ(capturedGroupIdGroupDelete, groupId.data());
    EXPECT_TRUE(response.has_value());
}

TEST_F(GroupDelegateTest, RemoveGroupFailTest) {
    std::string capturedTournamentIdGroupFindBy;
    std::string capturedGroupIdGroupFindBy;
    nlohmann::json groupData = {
        {"id", "delete-group-id"},
        {"name", "Test Group"},
        {"region", "Test Region"},
        {"teams", nlohmann::json::array()}
    };
    auto returnGroup = std::make_shared<domain::Group>(groupData);
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroupFindBy),
                testing::SaveArg<1>(&capturedGroupIdGroupFindBy),
                testing::Return(std::expected<std::shared_ptr<domain::Group>, std::string>(returnGroup))
            )
        );

    std::string capturedGroupIdGroupDelete;
    EXPECT_CALL(*groupRepositoryMock, Delete(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedGroupIdGroupDelete),
                testing::Return(std::unexpected<std::string>("Group not found"))
            )
        );

    std::string_view tournamentId = "tournament-id";
    std::string_view groupId = "delete-group-id";
    auto response = groupDelegate->RemoveGroup(tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);

    EXPECT_EQ(capturedTournamentIdGroupFindBy, tournamentId.data());
    EXPECT_EQ(capturedGroupIdGroupFindBy, groupId.data());
    EXPECT_EQ(capturedGroupIdGroupDelete, groupId.data());
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Group not found");
}

TEST_F(GroupDelegateTest, RemoveGroupNotFoundTest) {
    std::string capturedTournamentIdGroupFindBy;
    std::string capturedGroupIdGroupFindBy;
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroupFindBy),
                testing::SaveArg<1>(&capturedGroupIdGroupFindBy),
                testing::Return(std::unexpected<std::string>("Group not found"))
            )
        );

    EXPECT_CALL(*groupRepositoryMock, Delete(::testing::_))
        .Times(0);

    std::string_view tournamentId = "tournament-id";
    std::string_view groupId = "delete-group-id";
    auto response = groupDelegate->RemoveGroup(tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);

    EXPECT_EQ(capturedTournamentIdGroupFindBy, tournamentId.data());
    EXPECT_EQ(capturedGroupIdGroupFindBy, groupId.data());
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Group not found");
}

TEST_F(GroupDelegateTest, UpdateTeamsSuccessTest) {
    std::string capturedTournamentIdGroupFindBy;
    std::string capturedGroupIdGroupFindBy;
    nlohmann::json groupData = {
        {"id", "group-id"},
        {"name", "Test Group"},
        {"region", "Test Region"},
        {"teams", nlohmann::json::array()}
    };
    auto returnGroup = std::make_shared<domain::Group>(groupData);
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroupFindBy),
                testing::SaveArg<1>(&capturedGroupIdGroupFindBy),
                testing::Return(std::expected<std::shared_ptr<domain::Group>, std::string>(returnGroup))
            )
        );

    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock2, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
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
        .Times(4)
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
        )
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

    std::vector<std::string> capturedTournamentIdsFindBy;
    std::vector<std::string> capturedTeamIdsFindBy;
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(testing::DoAll(
                testing::Invoke([&capturedTournamentIdsFindBy, &capturedTeamIdsFindBy](const std::string_view& tournId, const std::string_view& teamId) {
                    capturedTournamentIdsFindBy.push_back(std::string(tournId));
                    capturedTeamIdsFindBy.push_back(std::string(teamId));
                }),
                testing::Return(std::unexpected<std::string>("Group not found"))
            )
        )
        .WillOnce(testing::DoAll(
                testing::Invoke([&capturedTournamentIdsFindBy, &capturedTeamIdsFindBy](const std::string_view& tournId, const std::string_view& teamId) {
                    capturedTournamentIdsFindBy.push_back(std::string(tournId));
                    capturedTeamIdsFindBy.push_back(std::string(teamId));
                }),
                testing::Return(std::unexpected<std::string>("Group not found"))
            )
        );

    std::vector<std::string> capturedGroupIds;
    std::vector<std::shared_ptr<domain::Team>> capturedTeams;
    EXPECT_CALL(*groupRepositoryMock, UpdateGroupAddTeam(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(testing::DoAll(
                testing::Invoke([&capturedGroupIds, &capturedTeams](const std::string_view& grpId, const std::shared_ptr<domain::Team>& team) {
                    capturedGroupIds.push_back(std::string(grpId));
                    capturedTeams.push_back(team);
                }),
                testing::Return(std::expected<void, std::string>())
            )
        )
        .WillOnce(testing::DoAll(
                testing::Invoke([&capturedGroupIds, &capturedTeams](const std::string_view& grpId, const std::shared_ptr<domain::Team>& team) {
                    capturedGroupIds.push_back(std::string(grpId));
                    capturedTeams.push_back(team);
                }),
                testing::Return(std::expected<void, std::string>())
            )
        );

    std::unique_ptr<nlohmann::json> message1 = std::make_unique<nlohmann::json>();
    message1->emplace("tournamentId", "tournament-id");
    message1->emplace("groupId", "group-id");
    message1->emplace("teamId", "team-id-0");

    std::unique_ptr<nlohmann::json> message2 = std::make_unique<nlohmann::json>();
    message2->emplace("tournamentId", "tournament-id");
    message2->emplace("groupId", "group-id");
    message2->emplace("teamId", "team-id-1");

    EXPECT_CALL(*producerMock, SendMessage(message1->dump(), "tournament.team-add"))
        .Times(1);
    EXPECT_CALL(*producerMock, SendMessage(message2->dump(), "tournament.team-add"))
        .Times(1);

    std::string_view tournamentId = "tournament-id";
    std::string_view groupId = "group-id";
    std::vector<domain::Team> teams;
    nlohmann::json team1DataExe = {
        {"id", "team-id-0"},
        {"name", "Team 0"}
    };
    teams.push_back(domain::Team(team1DataExe));
    nlohmann::json team2DataExe = {
        {"id", "team-id-1"},
        {"name", "Team 1"}
    };
    teams.push_back(domain::Team(team2DataExe));
    auto response = groupDelegate->UpdateTeams(tournamentId, groupId, teams);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock2);
    testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdGroupFindBy, tournamentId.data());
    EXPECT_EQ(capturedGroupIdGroupFindBy, groupId.data());
    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId.data());
    EXPECT_EQ(capturedTeamIds.size(), 4);
    EXPECT_EQ(capturedTeamIds[0], team1DataExe["id"].get<std::string>());
    EXPECT_EQ(capturedTeamIds[1], team2DataExe["id"].get<std::string>());
    EXPECT_EQ(capturedTeamIds[2], team1DataExe["id"].get<std::string>());
    EXPECT_EQ(capturedTeamIds[3], team2DataExe["id"].get<std::string>());
    EXPECT_EQ(capturedTournamentIdsFindBy.size(), 2);
    EXPECT_EQ(capturedTournamentIdsFindBy[0], tournamentId);
    EXPECT_EQ(capturedTournamentIdsFindBy[1], tournamentId);
    EXPECT_EQ(capturedTeamIdsFindBy.size(), 2);
    EXPECT_EQ(capturedTeamIdsFindBy[0], team1DataExe["id"].get<std::string>());
    EXPECT_EQ(capturedTeamIdsFindBy[1], team2DataExe["id"].get<std::string>());
    EXPECT_EQ(capturedGroupIds.size(), 2);
    EXPECT_EQ(capturedGroupIds[0], groupId);
    EXPECT_EQ(capturedGroupIds[1], groupId);
    EXPECT_EQ(capturedTeams.size(), 2);
    EXPECT_EQ(capturedTeams[0]->Id, team1DataExe["id"].get<std::string>());
    EXPECT_EQ(capturedTeams[1]->Id, team2DataExe["id"].get<std::string>());
    EXPECT_TRUE(response.has_value());
}

TEST_F(GroupDelegateTest, UpdateTeamsTeamFailTest) {
    std::string capturedTournamentIdGroupFindBy;
    std::string capturedGroupIdGroupFindBy;
    nlohmann::json groupData = {
        {"id", "group-id"},
        {"name", "Test Group"},
        {"region", "Test Region"},
        {"teams", nlohmann::json::array()}
    };
    auto returnGroup = std::make_shared<domain::Group>(groupData);
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroupFindBy),
                testing::SaveArg<1>(&capturedGroupIdGroupFindBy),
                testing::Return(std::expected<std::shared_ptr<domain::Group>, std::string>(returnGroup))
            )
        );

    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock2, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
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
                testing::Return(std::unexpected<std::string>("Team not found"))
            )
        );

    std::vector<std::string> capturedTournamentIdsFindBy;
    std::vector<std::string> capturedTeamIdsFindBy;
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(testing::DoAll(
                testing::Invoke([&capturedTournamentIdsFindBy, &capturedTeamIdsFindBy](const std::string_view& tournId, const std::string_view& teamId) {
                    capturedTournamentIdsFindBy.push_back(std::string(tournId));
                    capturedTeamIdsFindBy.push_back(std::string(teamId));
                }),
                testing::Return(std::unexpected<std::string>("Group not found"))
            )
        );

    EXPECT_CALL(*groupRepositoryMock, UpdateGroupAddTeam(::testing::_, ::testing::_))
        .Times(0);

    std::string_view tournamentId = "tournament-id";
    std::string_view groupId = "group-id";
    std::vector<domain::Team> teams;
    nlohmann::json team1DataExe = {
        {"id", "team-id-0"},
        {"name", "Team 0"}
    };
    teams.push_back(domain::Team(team1DataExe));
    nlohmann::json team2DataExe = {
        {"id", "team-id-1"},
        {"name", "Team 1"}
    };
    teams.push_back(domain::Team(team2DataExe));
    auto response = groupDelegate->UpdateTeams(tournamentId, groupId, teams);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock2);
    testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdGroupFindBy, tournamentId.data());
    EXPECT_EQ(capturedGroupIdGroupFindBy, groupId.data());
    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId.data());
    EXPECT_EQ(capturedTeamIds.size(), 2);
    EXPECT_EQ(capturedTeamIds[0], team1DataExe["id"].get<std::string>());
    EXPECT_EQ(capturedTeamIds[1], team2DataExe["id"].get<std::string>());
    EXPECT_EQ(capturedTournamentIdsFindBy.size(), 1);
    EXPECT_EQ(capturedTournamentIdsFindBy[0], tournamentId);
    EXPECT_EQ(capturedTeamIdsFindBy.size(), 1);
    EXPECT_EQ(capturedTeamIdsFindBy[0], team1DataExe["id"].get<std::string>());
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Team not found");
}

TEST_F(GroupDelegateTest, UpdateTeamsOverflowingGroupTest) {
    std::string capturedTournamentIdGroupFindBy;
    std::string capturedGroupIdGroupFindBy;
    nlohmann::json groupData = {
        {"id", "group-id"},
        {"name", "Test Group"},
        {"region", "Test Region"},
        {"teams", nlohmann::json::array({
            {{"id", "team-id-2"}, {"name", "Team 2"}},
            {{"id", "team-id-3"}, {"name", "Team 4"}},
            {{"id", "team-id-4"}, {"name", "Team 5"}}
        })}
    };
    auto returnGroup = std::make_shared<domain::Group>(groupData);
    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdGroupFindBy),
                testing::SaveArg<1>(&capturedGroupIdGroupFindBy),
                testing::Return(std::expected<std::shared_ptr<domain::Group>, std::string>(returnGroup))
            )
        );

    std::string capturedTournamentIdTournamentRepo;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament", 2025);
    tournament->Id() = "tournament-id";
    EXPECT_CALL(*tournamentRepositoryMock2, ReadById(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentIdTournamentRepo),
                testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(tournament))
            )
        );

    EXPECT_CALL(*teamRepositoryMock2, ReadById(::testing::_))
        .Times(0);

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(::testing::_, ::testing::_))
        .Times(0);

    EXPECT_CALL(*groupRepositoryMock, UpdateGroupAddTeam(::testing::_, ::testing::_))
        .Times(0);

    std::string_view tournamentId = "tournament-id";
    std::string_view groupId = "group-id";
    std::vector<domain::Team> teams;
    nlohmann::json team1DataExe = {
        {"id", "team-id-0"},
        {"name", "Team 0"}
    };
    teams.push_back(domain::Team(team1DataExe));
    nlohmann::json team2DataExe = {
        {"id", "team-id-1"},
        {"name", "Team 1"}
    };
    teams.push_back(domain::Team(team2DataExe));
    auto response = groupDelegate->UpdateTeams(tournamentId, groupId, teams);

    testing::Mock::VerifyAndClearExpectations(&groupRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock2);
    testing::Mock::VerifyAndClearExpectations(&teamRepositoryMock2);

    EXPECT_EQ(capturedTournamentIdGroupFindBy, tournamentId.data());
    EXPECT_EQ(capturedGroupIdGroupFindBy, groupId.data());
    EXPECT_EQ(capturedTournamentIdTournamentRepo, tournamentId.data());
    EXPECT_FALSE(response.has_value());
    EXPECT_EQ(response.error(), "Group exceeds maximum teams capacity");
}