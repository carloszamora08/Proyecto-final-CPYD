#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Group.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "controller/GroupController.hpp"
#include "domain/Utilities.hpp"

class GroupDelegateMock : public IGroupDelegate {
public:
    MOCK_METHOD((std::expected<std::string, std::string>), CreateGroup, (const std::string_view& tournamentId, const domain::Group& group), (override));
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>), GetGroups, (const std::string_view& tournamentId), (override));
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Group>, std::string>), GetGroup, (const std::string_view& tournamentId, const std::string_view& groupId), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateGroup, (const std::string_view& tournamentId, const domain::Group& group, const bool updateTeams), (override));
    MOCK_METHOD((std::expected<void, std::string>), RemoveGroup, (const std::string_view& tournamentId, const std::string_view& groupId), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateTeams, (const std::string_view& tournamentId, const std::string_view& groupId, const std::vector<domain::Team>& teams), (override));
};

class GroupControllerTest : public ::testing::Test{
protected:
    std::shared_ptr<GroupDelegateMock> groupDelegateMock;
    std::shared_ptr<GroupController> groupController;

    void SetUp() override {
        groupDelegateMock = std::make_shared<GroupDelegateMock>();
        groupController = std::make_shared<GroupController>(GroupController(groupDelegateMock));
    }

    // TearDown() function
    void TearDown() override {
        // teardown code comes here
    }

};

TEST_F(GroupControllerTest, CreateGroupSuccessTest) {
    std::string capturedId;
    domain::Group capturedGroup;

    EXPECT_CALL(*groupDelegateMock, CreateGroup(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedId),
                testing::SaveArg<1>(&capturedGroup),
                testing::Return(std::expected<std::string, std::string>("new-id"))
            )
        );

    nlohmann::json groupRequestBody = {{"id", "new-id"}, {"name", "new name"}, {"region", "new region"}, {"teams", nlohmann::json::array()}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();
    std::string tournamentId = "tournament-id";
    auto response = groupController->CreateGroup(groupRequest, tournamentId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(capturedId, tournamentId);
    EXPECT_EQ(capturedGroup.Id(), groupRequestBody.at("id").get<std::string>());
    EXPECT_EQ(capturedGroup.Name(), groupRequestBody.at("name").get<std::string>());
    EXPECT_EQ(capturedGroup.Region(), groupRequestBody.at("region").get<std::string>());
    EXPECT_EQ(capturedGroup.Teams().size(), 0);
    EXPECT_EQ(response.code, crow::CREATED);
    EXPECT_EQ(response.get_header_value("location"), "new-id");
}

TEST_F(GroupControllerTest, CreateGroupDBInsertionFailTest) {
    std::string capturedId;
    domain::Group capturedGroup;

    EXPECT_CALL(*groupDelegateMock, CreateGroup(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedId),
                testing::SaveArg<1>(&capturedGroup),
                testing::Return(std::unexpected<std::string>("Group insertion failed"))
            )
        );

    nlohmann::json groupRequestBody = {{"id", "existing-id"}, {"name", "existing name"}, {"region", "existing region"}, {"teams", nlohmann::json::array()}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();
    std::string tournamentId = "tournament-id";
    auto response = groupController->CreateGroup(groupRequest, tournamentId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(capturedId, tournamentId);
    EXPECT_EQ(capturedGroup.Id(), groupRequestBody.at("id").get<std::string>());
    EXPECT_EQ(capturedGroup.Name(), groupRequestBody.at("name").get<std::string>());
    EXPECT_EQ(capturedGroup.Region(), groupRequestBody.at("region").get<std::string>());
    EXPECT_EQ(capturedGroup.Teams().size(), 0);
    EXPECT_EQ(response.code, crow::CONFLICT);
    EXPECT_EQ(response.body, "Group insertion failed");
}

TEST_F(GroupControllerTest, CreateGroupInvalidTournamentIDTest) {
    EXPECT_CALL(*groupDelegateMock, CreateGroup(::testing::_, ::testing::_))
        .Times(0);

    nlohmann::json groupRequestBody = {{"id", "new-id"}, {"name", "new name"}, {"region", "new region"}, {"teams", nlohmann::json::array()}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();
    std::string tournamentId = "bad tournament-id";
    auto response = groupController->CreateGroup(groupRequest, tournamentId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid tournament ID format");
}

TEST_F(GroupControllerTest, CreateGroupMalformedJSONTest) {
    EXPECT_CALL(*groupDelegateMock, CreateGroup(::testing::_, ::testing::_))
        .Times(0);

    crow::request groupRequest;
    groupRequest.body = R"({malformed json})";
    std::string tournamentId = "tournament-id";
    auto response = groupController->CreateGroup(groupRequest, tournamentId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid JSON");
}

TEST_F(GroupControllerTest, CreateGroupInvalidDataTest) {
    EXPECT_CALL(*groupDelegateMock, CreateGroup(::testing::_, ::testing::_))
        .Times(0);

    crow::request groupRequest;
    groupRequest.body = R"({"id": 123, "name": 456, "year": "789", "teams": "teams})";
    std::string tournamentId = "tournament-id";
    auto response = groupController->CreateGroup(groupRequest, tournamentId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid JSON");
}

TEST_F(GroupControllerTest, GetGroupSuccessTest) {
    std::string capturedTournamentId;
    std::string capturedGroupId;

    nlohmann::json body = {{"id", "read-group-id"}, {"name", "read name"}, {"region", "read region"}, {"tournamentId", "read-tournament-id"}, {"teams", nlohmann::json::array()}};
    auto group = std::make_shared<domain::Group>(body);

    EXPECT_CALL(*groupDelegateMock, GetGroup(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::SaveArg<1>(&capturedGroupId),
                testing::Return(std::expected<std::shared_ptr<domain::Group>, std::string>(group))
            )
        );
    
    std::string tournamentId = "read-tournament-id";
    std::string groupId = "read-group-id";
    auto response = groupController->GetGroup(tournamentId, groupId);
    auto bodyJson = nlohmann::json::parse(response.body);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedGroupId, groupId);
    EXPECT_EQ(bodyJson["id"], body.at("id").get<std::string>());
    EXPECT_EQ(bodyJson["name"], body.at("name").get<std::string>());
    EXPECT_EQ(bodyJson["region"], body.at("region").get<std::string>());
    EXPECT_EQ(bodyJson["tournamentId"], body.at("tournamentId").get<std::string>());
    EXPECT_EQ(bodyJson["teams"], body["teams"]);
    EXPECT_TRUE(bodyJson["teams"].is_array());
    EXPECT_EQ(response.code, crow::OK);
    EXPECT_EQ(response.get_header_value(CONTENT_TYPE_HEADER), JSON_CONTENT_TYPE);
}

TEST_F(GroupControllerTest, GetGroupDBSelectionFailTest) {
    std::string capturedTournamentId;
    std::string capturedGroupId;

    EXPECT_CALL(*groupDelegateMock, GetGroup(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::SaveArg<1>(&capturedGroupId),
                testing::Return(std::unexpected<std::string>("Group not found"))
            )
        );
    
    std::string tournamentId = "non-existing-tournament-id";
    std::string groupId = "non-existing-group-id";
    auto response = groupController->GetGroup(tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedGroupId, groupId);
    EXPECT_EQ(response.code, crow::NOT_FOUND);
    EXPECT_EQ(response.body, "Group not found");
}

TEST_F(GroupControllerTest, GetGroupInvalidTournamentIDTest) {
    EXPECT_CALL(*groupDelegateMock, GetGroup(::testing::_, ::testing::_))
        .Times(0);

    std::string tournamentId = "bad tournament-id";
    std::string groupId = "group-id";
    auto response = groupController->GetGroup(tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid tournament ID format");
}

TEST_F(GroupControllerTest, GetGroupInvalidGroupIDTest) {
    EXPECT_CALL(*groupDelegateMock, GetGroup(::testing::_, ::testing::_))
        .Times(0);

    std::string tournamentId = "tournament-id";
    std::string groupId = "bad group-id";
    auto response = groupController->GetGroup(tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid group ID format");
}

TEST_F(GroupControllerTest, GetGroupDBFailTest) {
    EXPECT_CALL(*groupDelegateMock, GetGroup(::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Database connection failed")));

    std::string tournamentId = "tournament-id";
    std::string groupId = "group-id";
    auto response = groupController->GetGroup(tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);
    
    EXPECT_EQ(response.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(response.body, "Database connection failed");
}

TEST_F(GroupControllerTest, GetGroupsSuccessTest) {
    std::string capturedTournamentId;
    std::vector<std::shared_ptr<domain::Group>> groups;

    nlohmann::json body1 = {{"id", "first-group-id"}, {"name", "first name"}, {"region", "first region"}, {"tournamentId", "first-tournament-id"}, {"teams", nlohmann::json::array()}};
    groups.push_back(std::make_shared<domain::Group>(body1));

    nlohmann::json body2 = {{"id", "second-group-id"}, {"name", "second name"}, {"region", "second region"}, {"tournamentId", "second-tournament-id"}, {"teams", nlohmann::json::array()}};
    groups.push_back(std::make_shared<domain::Group>(body2));

    EXPECT_CALL(*groupDelegateMock, GetGroups(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(groups))
            )
        );

    std::string tournamentId = "read-tournament-id";
    auto response = groupController->GetGroups(tournamentId);
    auto bodyJson = nlohmann::json::parse(response.body);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);
    
    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(bodyJson.size(), groups.size());
    EXPECT_EQ(bodyJson[0]["id"], body1.at("id").get<std::string>());
    EXPECT_EQ(bodyJson[0]["name"], body1.at("name").get<std::string>());
    EXPECT_EQ(bodyJson[0]["region"], body1.at("region").get<std::string>());
    EXPECT_EQ(bodyJson[0]["tournamentId"], body1.at("tournamentId").get<std::string>());
    EXPECT_EQ(bodyJson[0]["teams"], body1["teams"]);
    EXPECT_TRUE(bodyJson[0]["teams"].is_array());
    EXPECT_EQ(bodyJson[1]["id"], body2.at("id").get<std::string>());
    EXPECT_EQ(bodyJson[1]["name"], body2.at("name").get<std::string>());
    EXPECT_EQ(bodyJson[1]["region"], body2.at("region").get<std::string>());
    EXPECT_EQ(bodyJson[1]["tournamentId"], body2.at("tournamentId").get<std::string>());
    EXPECT_EQ(bodyJson[1]["teams"], body2["teams"]);
    EXPECT_TRUE(bodyJson[1]["teams"].is_array());
    EXPECT_EQ(response.code, crow::OK);
    EXPECT_EQ(response.get_header_value(CONTENT_TYPE_HEADER), JSON_CONTENT_TYPE);
}

TEST_F(GroupControllerTest, GetGroupsEmptyTest) {
    std::string capturedTournamentId;
    std::vector<std::shared_ptr<domain::Group>> groups;

    EXPECT_CALL(*groupDelegateMock, GetGroups(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::Return(std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>(groups))
            )
        );

    std::string tournamentId = "read-tournament-id";
    auto response = groupController->GetGroups(tournamentId);
    auto bodyJson = nlohmann::json::parse(response.body);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(bodyJson.size(), 0);
    EXPECT_EQ(response.body, "[]");
    EXPECT_EQ(response.code, crow::OK);
    EXPECT_EQ(response.get_header_value(CONTENT_TYPE_HEADER), JSON_CONTENT_TYPE);
}

TEST_F(GroupControllerTest, GetGroupsInvalidTournamentIDTest) {
    EXPECT_CALL(*groupDelegateMock, GetGroups(::testing::_))
        .Times(0);

    std::string tournamentId = "bad tournament-id";
    auto response = groupController->GetGroups(tournamentId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid tournament ID format");
}

TEST_F(GroupControllerTest, GetGroupsDBFailTest) {
    EXPECT_CALL(*groupDelegateMock, GetGroups(::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Database connection failed")));

    std::string tournamentId = "tournament-id";
    auto response = groupController->GetGroups(tournamentId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);
    
    EXPECT_EQ(response.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(response.body, "Database connection failed");
}

TEST_F(GroupControllerTest, UpdateGroupSuccessTest) {
    std::string capturedTournamentId;
    domain::Group capturedGroup;
    bool capturedUpdateTeams;

    EXPECT_CALL(*groupDelegateMock, UpdateGroup(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::SaveArg<1>(&capturedGroup),
                testing::SaveArg<2>(&capturedUpdateTeams),
                testing::Return(std::expected<void, std::string>())
            )
        );

    nlohmann::json groupRequestBody = {{"id", "updated-group-id"}, {"name", "updated name"}, {"region", "updated region"}, {"tournamentId", "updated-tournament-id"}, {"teams", nlohmann::json::array()}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();
    std::string tournamentId = "updated-tournament-id";
    std::string groupId = "updated-group-id";
    auto response = groupController->UpdateGroup(groupRequest, tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedGroup.Id(), groupRequestBody.at("id").get<std::string>());
    EXPECT_EQ(capturedGroup.Name(), groupRequestBody.at("name").get<std::string>());
    EXPECT_EQ(capturedGroup.Region(), groupRequestBody.at("region").get<std::string>());
    EXPECT_EQ(capturedGroup.TournamentId(), groupRequestBody.at("tournamentId").get<std::string>());
    EXPECT_EQ(capturedGroup.Teams().size(), 0);
    EXPECT_TRUE(capturedUpdateTeams);
    EXPECT_EQ(response.code, crow::NO_CONTENT);
}

TEST_F(GroupControllerTest, UpdateGroupFailTest) {
    std::string capturedTournamentId;
    domain::Group capturedGroup;
    bool capturedUpdateTeams;

    EXPECT_CALL(*groupDelegateMock, UpdateGroup(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::SaveArg<1>(&capturedGroup),
                testing::SaveArg<2>(&capturedUpdateTeams),
                testing::Return(std::unexpected<std::string>("Group not found"))
            )
        );

    nlohmann::json groupRequestBody = {{"id", "non-existing-group-id"}, {"name", "non-existing name"}, {"region", "non-existing region"}, {"tournamentId", "non-existing-tournament-id"}, {"teams", nlohmann::json::array()}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();
    std::string tournamentId = "non-existing-tournament-id";
    std::string groupId = "non-existing-group-id";
    auto response = groupController->UpdateGroup(groupRequest, tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedGroup.Id(), groupRequestBody.at("id").get<std::string>());
    EXPECT_EQ(capturedGroup.Name(), groupRequestBody.at("name").get<std::string>());
    EXPECT_EQ(capturedGroup.Region(), groupRequestBody.at("region").get<std::string>());
    EXPECT_EQ(capturedGroup.TournamentId(), groupRequestBody.at("tournamentId").get<std::string>());
    EXPECT_EQ(capturedGroup.Teams().size(), 0);
    EXPECT_TRUE(capturedUpdateTeams);
    EXPECT_EQ(response.code, crow::NOT_FOUND);
    EXPECT_EQ(response.body, "Group not found");
}

TEST_F(GroupControllerTest, UpdateGroupInvalidTournamentIDTest) {
    EXPECT_CALL(*groupDelegateMock, UpdateGroup(::testing::_, ::testing::_, ::testing::_))
        .Times(0);

    nlohmann::json groupRequestBody = {{"id", "group-id"}, {"name", "name"}, {"region", "region"}, {"tournamentId", "tournament-id"}, {"teams", nlohmann::json::array()}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();
    std::string tournamentId = "bad tournament-id";
    std::string groupId = "group-id";
    auto response = groupController->UpdateGroup(groupRequest, tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid tournament ID format");
}

TEST_F(GroupControllerTest, UpdateGroupInvalidGroupIDTest) {
    EXPECT_CALL(*groupDelegateMock, UpdateGroup(::testing::_, ::testing::_, ::testing::_))
        .Times(0);

    nlohmann::json groupRequestBody = {{"id", "bad group-id"}, {"name", "name"}, {"region", "region"}, {"tournamentId", "tournament-id"}, {"teams", nlohmann::json::array()}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();
    std::string tournamentId = "tournament-id";
    std::string groupId = "bad group-id";
    auto response = groupController->UpdateGroup(groupRequest, tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid group ID format");
}

TEST_F(GroupControllerTest, UpdateGroupMalformedJSONTest) {
    EXPECT_CALL(*groupDelegateMock, UpdateGroup(::testing::_, ::testing::_, ::testing::_))
        .Times(0);

    crow::request groupRequest;
    groupRequest.body = R"({malformed json})";
    std::string tournamentId = "tournament-id";
    std::string groupId = "group-id";
    auto response = groupController->UpdateGroup(groupRequest, tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid JSON");
}

TEST_F(GroupControllerTest, UpdateGroupInvalidDataTest) {
    EXPECT_CALL(*groupDelegateMock, UpdateGroup(::testing::_, ::testing::_, ::testing::_))
        .Times(0);

    crow::request groupRequest;
    groupRequest.body = R"({"id": 123, "name": 456, "year": "789"})";
    std::string tournamentId = "tournament-id";
    std::string groupId = "group-id";
    auto response = groupController->UpdateGroup(groupRequest, tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid JSON");
}

TEST_F(GroupControllerTest, UpdateGroupOverflowingGroupTest) {
    EXPECT_CALL(*groupDelegateMock, UpdateGroup(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Group exceeds maximum teams capacity")));

    nlohmann::json groupRequestBody = {
        {"id", "group-id"}, 
        {"name", "name"}, 
        {"region", "region"}, 
        {"tournamentId", "tournament-id"}, 
        {"teams", nlohmann::json::array({
            {{"id", "team-1"}, {"name", "Team One"}},
            {{"id", "team-2"}, {"name", "Team Two"}},
            {{"id", "team-3"}, {"name", "Team Three"}},
            {{"id", "team-4"}, {"name", "Team Four"}},
            {{"id", "team-5"}, {"name", "Team Five"}}
        })}
    };
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();
    std::string tournamentId = "tournament-id";
    std::string groupId = "group-id";
    auto response = groupController->UpdateGroup(groupRequest, tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(response.code, crow::CONFLICT);
    EXPECT_EQ(response.body, "Group exceeds maximum teams capacity");
}

TEST_F(GroupControllerTest, UpdateGroupDBFailTest) {
    EXPECT_CALL(*groupDelegateMock, UpdateGroup(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Database connection failed")));

    nlohmann::json groupRequestBody = {{"id", "group-id"}, {"name", "name"}, {"region", "region"}, {"tournamentId", "tournament-id"}, {"teams", nlohmann::json::array()}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();
    std::string tournamentId = "tournament-id";
    std::string groupId = "group-id";
    auto response = groupController->UpdateGroup(groupRequest, tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(response.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(response.body, "Database connection failed");
}

TEST_F(GroupControllerTest, DeleteGroupSuccessTest) {
    std::string capturedTournamentId;
    std::string capturedGroupId;

    EXPECT_CALL(*groupDelegateMock, RemoveGroup(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::SaveArg<1>(&capturedGroupId),
                testing::Return(std::expected<void, std::string>())
            )
        );
    
    std::string tournamentId = "tournament-id";
    std::string groupId = "group-id";
    auto response = groupController->DeleteGroup(tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedGroupId, groupId);
    EXPECT_EQ(response.code, crow::NO_CONTENT);
}

TEST_F(GroupControllerTest, DeleteGroupDBDeletionFailTest) {
    std::string capturedTournamentId;
    std::string capturedGroupId;

    EXPECT_CALL(*groupDelegateMock, RemoveGroup(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::SaveArg<1>(&capturedGroupId),
                testing::Return(std::unexpected<std::string>("Group not found"))
            )
        );
    
    std::string tournamentId = "tournament-id";
    std::string groupId = "group-id";
    auto response = groupController->DeleteGroup(tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedGroupId, groupId);
    EXPECT_EQ(response.code, crow::NOT_FOUND);
    EXPECT_EQ(response.body, "Group not found");
}

TEST_F(GroupControllerTest, DeleteGroupInvalidTournamentIDTest) {
    EXPECT_CALL(*groupDelegateMock, RemoveGroup(::testing::_, ::testing::_))
        .Times(0);

    std::string tournamentId = "bad tournament-id";
    std::string groupId = "group-id";
    auto response = groupController->DeleteGroup(tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid tournament ID format");
}

TEST_F(GroupControllerTest, DeleteGroupInvalidGroupIDTest) {
    EXPECT_CALL(*groupDelegateMock, RemoveGroup(::testing::_, ::testing::_))
        .Times(0);

    std::string tournamentId = "tournament-id";
    std::string groupId = "bad group-id";
    auto response = groupController->DeleteGroup(tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);
    
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid group ID format");
}

TEST_F(GroupControllerTest, DeleteGroupDBFailTest) {
    EXPECT_CALL(*groupDelegateMock, RemoveGroup(::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Database connection failed")));

    std::string tournamentId = "tournament-id";
    std::string groupId = "group-id";
    auto response = groupController->DeleteGroup(tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);
    
    EXPECT_EQ(response.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(response.body, "Database connection failed");
}

TEST_F(GroupControllerTest, UpdateTeamsSuccessTest) {
    std::string capturedTournamentId;
    std::string capturedGroupId;
    std::vector<domain::Team> capturedTeams;

    EXPECT_CALL(*groupDelegateMock, UpdateTeams(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::SaveArg<1>(&capturedGroupId),
                testing::SaveArg<2>(&capturedTeams),
                testing::Return(std::expected<void, std::string>())
            )
        );

    nlohmann::json groupRequestBody = {{{"id", "team-1"}, {"name", "Team One"}}, {{"id", "team-2"}, {"name", "Team Two"}}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();
    std::string tournamentId = "updated-tournament-id";
    std::string groupId = "updated-group-id";
    auto response = groupController->UpdateTeams(groupRequest, tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedGroupId, groupId);
    EXPECT_EQ(capturedTeams[0].Id, groupRequestBody[0].at("id").get<std::string>());
    EXPECT_EQ(capturedTeams[0].Name, groupRequestBody[0].at("name").get<std::string>());
    EXPECT_EQ(capturedTeams[1].Id, groupRequestBody[1].at("id").get<std::string>());
    EXPECT_EQ(capturedTeams[1].Name, groupRequestBody[1].at("name").get<std::string>());
    EXPECT_EQ(response.code, crow::NO_CONTENT);
}

TEST_F(GroupControllerTest, UpdateTeamsDBInsertionFailTest) {
    std::string capturedTournamentId;
    std::string capturedGroupId;
    std::vector<domain::Team> capturedTeams;

    EXPECT_CALL(*groupDelegateMock, UpdateTeams(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournamentId),
                testing::SaveArg<1>(&capturedGroupId),
                testing::SaveArg<2>(&capturedTeams),
                testing::Return(std::unexpected<std::string>("Group not found"))
            )
        );

    nlohmann::json groupRequestBody = {{{"id", "team-1"}, {"name", "Team One"}}, {{"id", "team-2"}, {"name", "Team Two"}}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();
    std::string tournamentId = "updated-tournament-id";
    std::string groupId = "updated-group-id";
    auto response = groupController->UpdateTeams(groupRequest, tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(capturedTournamentId, tournamentId);
    EXPECT_EQ(capturedGroupId, groupId);
    EXPECT_EQ(capturedTeams[0].Id, groupRequestBody[0].at("id").get<std::string>());
    EXPECT_EQ(capturedTeams[0].Name, groupRequestBody[0].at("name").get<std::string>());
    EXPECT_EQ(capturedTeams[1].Id, groupRequestBody[1].at("id").get<std::string>());
    EXPECT_EQ(capturedTeams[1].Name, groupRequestBody[1].at("name").get<std::string>());
    EXPECT_EQ(response.code, crow::NOT_FOUND);
    EXPECT_EQ(response.body, "Group not found");
}

TEST_F(GroupControllerTest, UpdateTeamsDBFailTest) {
    EXPECT_CALL(*groupDelegateMock, UpdateTeams(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Team team-1 already exists in another group")));

    nlohmann::json groupRequestBody = {{{"id", "team-1"}, {"name", "Team One"}}, {{"id", "team-2"}, {"name", "Team Two"}}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();
    std::string tournamentId = "updated-tournament-id";
    std::string groupId = "updated-group-id";
    auto response = groupController->UpdateTeams(groupRequest, tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(response.code, crow::CONFLICT);
    EXPECT_EQ(response.body, "Team team-1 already exists in another group");
}

TEST_F(GroupControllerTest, UpdateTeamsOverflowingGroupTest) {
    EXPECT_CALL(*groupDelegateMock, UpdateTeams(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Group exceeds maximum teams capacity")));

    nlohmann::json groupRequestBody = {{{"id", "team-1"}, {"name", "Team One"}}, {{"id", "team-2"}, {"name", "Team Two"}}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();
    std::string tournamentId = "updated-tournament-id";
    std::string groupId = "updated-group-id";
    auto response = groupController->UpdateTeams(groupRequest, tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(response.code, 422);
    EXPECT_EQ(response.body, "Group exceeds maximum teams capacity");
}

TEST_F(GroupControllerTest, UpdateTeamsTeamNotFoundTest) {
    EXPECT_CALL(*groupDelegateMock, UpdateTeams(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Team not found")));

    nlohmann::json groupRequestBody = {{{"id", "team-1"}, {"name", "Team One"}}, {{"id", "team-2"}, {"name", "Team Two"}}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();
    std::string tournamentId = "updated-tournament-id";
    std::string groupId = "updated-group-id";
    auto response = groupController->UpdateTeams(groupRequest, tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(response.code, 422);
    EXPECT_EQ(response.body, "Team not found");
}

TEST_F(GroupControllerTest, UpdateTeamsInvalidTournamentIDTest) {
    EXPECT_CALL(*groupDelegateMock, UpdateTeams(::testing::_, ::testing::_, ::testing::_))
        .Times(0);

    nlohmann::json groupRequestBody = {{{"id", "team-1"}, {"name", "Team One"}}, {{"id", "team-2"}, {"name", "Team Two"}}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();
    std::string tournamentId = "bad tournament-id";
    std::string groupId = "group-id";
    auto response = groupController->UpdateTeams(groupRequest, tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid tournament ID format");
}

TEST_F(GroupControllerTest, UpdateTeamsInvalidGroupIDTest) {
    EXPECT_CALL(*groupDelegateMock, UpdateTeams(::testing::_, ::testing::_, ::testing::_))
        .Times(0);

    nlohmann::json groupRequestBody = {{{"id", "team-1"}, {"name", "Team One"}}, {{"id", "team-2"}, {"name", "Team Two"}}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();
    std::string tournamentId = "tournament-id";
    std::string groupId = "bad group-id";
    auto response = groupController->UpdateTeams(groupRequest, tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid group ID format");
}

TEST_F(GroupControllerTest, UpdateTeamsMalformedJSONTest) {
    EXPECT_CALL(*groupDelegateMock, UpdateTeams(::testing::_, ::testing::_, ::testing::_))
        .Times(0);

    crow::request groupRequest;
    groupRequest.body = R"({malformed json})";
    std::string tournamentId = "tournament-id";
    std::string groupId = "group-id";
    auto response = groupController->UpdateTeams(groupRequest, tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid JSON");
}

TEST_F(GroupControllerTest, UpdateTeamsInvalidDataTest) {
    EXPECT_CALL(*groupDelegateMock, UpdateTeams(::testing::_, ::testing::_, ::testing::_))
        .Times(0);

    crow::request groupRequest;
    groupRequest.body = R"({"id": 123, "name": 456, "year": "789", "teams": "teams})";
    std::string tournamentId = "tournament-id";
    std::string groupId = "group-id";
    auto response = groupController->UpdateTeams(groupRequest, tournamentId, groupId);

    testing::Mock::VerifyAndClearExpectations(&groupDelegateMock);

    EXPECT_EQ(response.code, crow::BAD_REQUEST);
    EXPECT_EQ(response.body, "Invalid JSON");
}