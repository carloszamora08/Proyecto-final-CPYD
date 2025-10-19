#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Tournament.hpp"
#include "cms/QueueMessageProducer.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "delegate/TournamentDelegate.hpp"
#include "domain/Utilities.hpp"

class TournamentRepositoryMock : public TournamentRepository {
public:
    TournamentRepositoryMock() : TournamentRepository(nullptr) {}

    MOCK_METHOD((std::expected<std::string, std::string>), Create, (const domain::Tournament& entity), (override));
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string>), ReadAll, (), (override));
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Tournament>, std::string>), ReadById, (const std::string id), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), Update, (const std::string id, const domain::Tournament& entity), (override));
    MOCK_METHOD((std::expected<void, std::string>), Delete, (const std::string id), (override));
};

class QueueMessageProducerMock : public QueueMessageProducer {
public:
    QueueMessageProducerMock(): QueueMessageProducer(nullptr) {}

    MOCK_METHOD(void, SendMessage, (const std::string_view& message, const std::string_view& queue), (override));
};

class TournamentDelegateTest : public ::testing::Test{
protected:
    std::shared_ptr<TournamentRepositoryMock> tournamentRepositoryMock;
    std::shared_ptr<QueueMessageProducerMock> producerMock;
    std::shared_ptr<TournamentDelegate> tournamentDelegate;

    void SetUp() override {
        tournamentRepositoryMock = std::make_shared<TournamentRepositoryMock>();
        producerMock = std::make_shared<QueueMessageProducerMock>();
        tournamentDelegate = std::make_shared<TournamentDelegate>(TournamentDelegate(tournamentRepositoryMock, producerMock));
    }

    // TearDown() function
    void TearDown() override {
        // teardown code comes here
    }

};

TEST_F(TournamentDelegateTest, CreateTournamentSucessTest) {
    domain::Tournament capturedTournament;

    EXPECT_CALL(*tournamentRepositoryMock, Create(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournament),
                testing::Return(std::expected<std::string, std::string>("new-id"))
            )
        );

    EXPECT_CALL(*producerMock, SendMessage("new-id", "tournament.created"))
        .Times(1);

    nlohmann::json body = {{"id", "new-id"}, {"name", "new tournament"}, {"year", "new year"}};
    std::shared_ptr<domain::Tournament> tournament = std::make_shared<domain::Tournament>(body);
    auto response = tournamentDelegate->CreateTournament(tournament);

    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock);

    EXPECT_EQ(capturedTournament.Id(), body.at("id").get<std::string>());
    EXPECT_EQ(capturedTournament.Name(), body.at("name").get<std::string>());
    EXPECT_EQ(capturedTournament.Year(), body.at("year").get<std::string>());
    EXPECT_EQ(response.value(), "new-id");
}

