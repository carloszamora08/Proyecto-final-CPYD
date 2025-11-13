#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "cms/ScoreUpdateListener.hpp"
#include "cms/QueueMessageProducer.hpp"
#include "delegate/MatchDelegate2.hpp"
#include "domain/Utilities.hpp"
#include "event/ScoreUpdateEvent.hpp"

class MatchDelegate2Mock : public MatchDelegate2 {
public:
    MatchDelegate2Mock(): MatchDelegate2(nullptr, nullptr, nullptr) {}

    MOCK_METHOD(void, ProcessTeamAddition, (const TeamAddEvent& teamAddEvent), (override));
    MOCK_METHOD(void, ProcessScoreUpdate, (const ScoreUpdateEvent& scoreUpdateEvent), (override));
};

class ScoreUpdateListenerTest : public ::testing::Test{
protected:
    std::shared_ptr<ConnectionManager> connectionManager;
    std::shared_ptr<MatchDelegate2Mock> matchDelegate2Mock;
    std::shared_ptr<ScoreUpdateListener> scoreUpdateListener;

    void SetUp() override {
        connectionManager = std::make_shared<ConnectionManager>();
        matchDelegate2Mock = std::make_shared<MatchDelegate2Mock>();
        scoreUpdateListener = std::make_shared<ScoreUpdateListener>(connectionManager, matchDelegate2Mock);
    }

    // TearDown() function
    void TearDown() override {
        // teardown code comes here
    }

};

TEST_F(ScoreUpdateListenerTest, ProcessMessageSuccessTest) {
    ScoreUpdateEvent capturedScoreUpdateEvent;

    EXPECT_CALL(*matchDelegate2Mock, ProcessScoreUpdate(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedScoreUpdateEvent)
            )
        );

    std::string consumerMessage = R"({"tournamentId":"tournament-id","matchId":"match-id"})";
    scoreUpdateListener->processMessage(consumerMessage);

    testing::Mock::VerifyAndClearExpectations(&matchDelegate2Mock);

    EXPECT_EQ(capturedScoreUpdateEvent.tournamentId, "tournament-id");
    EXPECT_EQ(capturedScoreUpdateEvent.matchId, "match-id");
}

TEST_F(ScoreUpdateListenerTest, ProcessMessageInvalidJSONTest) {
    EXPECT_CALL(*matchDelegate2Mock, ProcessScoreUpdate(::testing::_))
        .Times(0);
    
    std::string invalidJson = R"({"tournamentId":"tournament-id", INVALID})";
    
    EXPECT_NO_THROW(scoreUpdateListener->processMessage(invalidJson));
    
    testing::Mock::VerifyAndClearExpectations(matchDelegate2Mock.get());
}

TEST_F(ScoreUpdateListenerTest, ProcessMessageMissingJSONFieldsTest) {
    EXPECT_CALL(*matchDelegate2Mock, ProcessScoreUpdate(::testing::_))
        .Times(0);
    
    std::string incompleteJson = R"({"tournamentId":"tournament-id","groupId":"group-id"})";
    
    EXPECT_NO_THROW(scoreUpdateListener->processMessage(incompleteJson));
    
    testing::Mock::VerifyAndClearExpectations(matchDelegate2Mock.get());
}

TEST_F(ScoreUpdateListenerTest, ProcessMessageEmptyMessageTest) {
    EXPECT_CALL(*matchDelegate2Mock, ProcessScoreUpdate(::testing::_))
        .Times(0);
    
    EXPECT_NO_THROW(scoreUpdateListener->processMessage(""));
    
    testing::Mock::VerifyAndClearExpectations(matchDelegate2Mock.get());
}

TEST_F(ScoreUpdateListenerTest, ProcessMessageWrongDataTypesTest) {
    EXPECT_CALL(*matchDelegate2Mock, ProcessScoreUpdate(::testing::_))
        .Times(0);
    
    std::string wrongTypes = R"({"tournamentId":123,"matchId":456})";
    
    EXPECT_NO_THROW(scoreUpdateListener->processMessage(wrongTypes));

    testing::Mock::VerifyAndClearExpectations(matchDelegate2Mock.get());
}

TEST_F(ScoreUpdateListenerTest, ProcessMessageNullDelegateTest) {
    auto listenerWithNullDelegate = std::make_shared<ScoreUpdateListener>(
        connectionManager, 
        nullptr
    );
    
    std::string validMessage = R"({"tournamentId":"tournament-id","groupId":"match-id"})";
    
    EXPECT_NO_THROW(listenerWithNullDelegate->processMessage(validMessage));
}