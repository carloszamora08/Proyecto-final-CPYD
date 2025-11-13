#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "cms/GroupAddTeamListener.hpp"
#include "cms/QueueMessageProducer.hpp"
#include "delegate/MatchDelegate2.hpp"
#include "domain/Utilities.hpp"
#include "event/TeamAddEvent.hpp"

class MatchDelegate2Mock : public MatchDelegate2 {
public:
    MatchDelegate2Mock(): MatchDelegate2(nullptr, nullptr, nullptr) {}

    MOCK_METHOD(void, ProcessTeamAddition, (const TeamAddEvent& teamAddEvent), (override));
    MOCK_METHOD(void, ProcessScoreUpdate, (const ScoreUpdateEvent& scoreUpdateEvent), (override));
};

class GroupAddTeamListenerTest : public ::testing::Test{
protected:
    std::shared_ptr<ConnectionManager> connectionManager;
    std::shared_ptr<MatchDelegate2Mock> matchDelegate2Mock;
    std::shared_ptr<GroupAddTeamListener> groupAddTeamListener;

    void SetUp() override {
        connectionManager = std::make_shared<ConnectionManager>();
        matchDelegate2Mock = std::make_shared<MatchDelegate2Mock>();
        groupAddTeamListener = std::make_shared<GroupAddTeamListener>(connectionManager, matchDelegate2Mock);
    }

    // TearDown() function
    void TearDown() override {
        // teardown code comes here
    }

};

TEST_F(GroupAddTeamListenerTest, ProcessMessageSuccessTest) {
    TeamAddEvent capturedTeamAddEvent;

    EXPECT_CALL(*matchDelegate2Mock, ProcessTeamAddition(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTeamAddEvent)
            )
        );

    std::string consumerMessage = R"({"tournamentId":"tournament-id","groupId":"group-id","teamId":"team-id"})";
    groupAddTeamListener->processMessage(consumerMessage);

    testing::Mock::VerifyAndClearExpectations(&matchDelegate2Mock);

    EXPECT_EQ(capturedTeamAddEvent.tournamentId, "tournament-id");
    EXPECT_EQ(capturedTeamAddEvent.groupId, "group-id");
    EXPECT_EQ(capturedTeamAddEvent.teamId, "team-id");
}

TEST_F(GroupAddTeamListenerTest, ProcessMessageInvalidJSONTest) {
    EXPECT_CALL(*matchDelegate2Mock, ProcessTeamAddition(::testing::_))
        .Times(0);
    
    std::string invalidJson = R"({"tournamentId":"tournament-id", INVALID})";
    
    EXPECT_NO_THROW(groupAddTeamListener->processMessage(invalidJson));
    
    testing::Mock::VerifyAndClearExpectations(matchDelegate2Mock.get());
}

TEST_F(GroupAddTeamListenerTest, ProcessMessageMissingJSONFieldsTest) {
    EXPECT_CALL(*matchDelegate2Mock, ProcessTeamAddition(::testing::_))
        .Times(0);
    
    std::string incompleteJson = R"({"tournamentId":"tournament-id","groupId":"group-id"})";
    
    EXPECT_NO_THROW(groupAddTeamListener->processMessage(incompleteJson));
    
    testing::Mock::VerifyAndClearExpectations(matchDelegate2Mock.get());
}

TEST_F(GroupAddTeamListenerTest, ProcessMessageEmptyMessageTest) {
    EXPECT_CALL(*matchDelegate2Mock, ProcessTeamAddition(::testing::_))
        .Times(0);
    
    EXPECT_NO_THROW(groupAddTeamListener->processMessage(""));
    
    testing::Mock::VerifyAndClearExpectations(matchDelegate2Mock.get());
}

TEST_F(GroupAddTeamListenerTest, ProcessMessageWrongDataTypesTest) {
    EXPECT_CALL(*matchDelegate2Mock, ProcessTeamAddition(::testing::_))
        .Times(0);
    
    std::string wrongTypes = R"({"tournamentId":123,"groupId":456,"teamId":789})";
    
    EXPECT_NO_THROW(groupAddTeamListener->processMessage(wrongTypes));

    testing::Mock::VerifyAndClearExpectations(matchDelegate2Mock.get());
}

TEST_F(GroupAddTeamListenerTest, ProcessMessageNullDelegateTest) {
    auto listenerWithNullDelegate = std::make_shared<GroupAddTeamListener>(
        connectionManager, 
        nullptr
    );
    
    std::string validMessage = R"({"tournamentId":"tournament-id","groupId":"group-id","teamId":"team-id"})";
    
    EXPECT_NO_THROW(listenerWithNullDelegate->processMessage(validMessage));
}