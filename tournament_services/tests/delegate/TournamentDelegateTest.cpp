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

TEST_F(TournamentDelegateTest, CreateTournamentFailureTest) {
    domain::Tournament capturedTournament;

    EXPECT_CALL(*tournamentRepositoryMock, Create(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournament),
                testing::Return(std::unexpected<std::string>("Error creating tournament"))
            )
        );

    EXPECT_CALL(*producerMock, SendMessage(::testing::_, ::testing::_))
        .Times(0);

    nlohmann::json body = {{"id", "new-id"}, {"name", "new tournament"}, {"year", "new year"}};
    std::shared_ptr<domain::Tournament> tournament = std::make_shared<domain::Tournament>(body);
    auto response = tournamentDelegate->CreateTournament(tournament);

    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock);
    testing::Mock::VerifyAndClearExpectations(&producerMock);

    EXPECT_EQ(capturedTournament.Id(), body.at("id").get<std::string>());
    EXPECT_EQ(capturedTournament.Name(), body.at("name").get<std::string>());
    EXPECT_EQ(capturedTournament.Year(), body.at("year").get<std::string>());
    EXPECT_EQ(response.has_value(), false);
    EXPECT_EQ(response.error(), "Error creating tournament");
}

TEST_F(TournamentDelegateTest, GetTournamentSuccessTest) {
    const std::string tournamentId = "test-id-123";
    nlohmann::json tournamentData = {{"id", tournamentId}, {"name", "Test Tournament"}, {"year", "2025"}};
    auto expectedTournament = std::make_shared<domain::Tournament>(tournamentData);

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>(expectedTournament)));

    auto response = tournamentDelegate->GetTournament(tournamentId);

    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock);

    EXPECT_EQ(response.has_value(), true);
    EXPECT_EQ(response.value() != nullptr, true);
    EXPECT_EQ(response.value()->Id(), tournamentId);
    EXPECT_EQ(response.value()->Name(), "Test Tournament");
    EXPECT_EQ(response.value()->Year(), "2025");
}

TEST_F(TournamentDelegateTest, GetTournamentNotFoundTest) {
    const std::string tournamentId = "non-existent-id";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(std::unexpected<std::string>("Tournament not found")));

    auto response = tournamentDelegate->GetTournament(tournamentId);

    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock);

    EXPECT_EQ(response.has_value(), false);
    EXPECT_EQ(response.error(), "Tournament not found");
}

TEST_F(TournamentDelegateTest, ReadAllTournamentsSuccessTest) {
    std::vector<std::shared_ptr<domain::Tournament>> tournamentsList;

    nlohmann::json tournament1Data = {{"id", "id-1"}, {"name", "Tournament 1"}, {"year", "2024"}};
    nlohmann::json tournament2Data = {{"id", "id-2"}, {"name", "Tournament 2"}, {"year", "2025"}};

    tournamentsList.push_back(std::make_shared<domain::Tournament>(tournament1Data));
    tournamentsList.push_back(std::make_shared<domain::Tournament>(tournament2Data));

    EXPECT_CALL(*tournamentRepositoryMock, ReadAll())
        .WillOnce(testing::Return(std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string>(tournamentsList)));

    auto response = tournamentDelegate->ReadAll();

    testing::Mock::VerifyAndClearExpectations(&tournamentRepositoryMock);

    EXPECT_EQ(response.has_value(), true);
    EXPECT_EQ(response.value().size(), 2);
    EXPECT_EQ(response.value()[0]->Id(), "id-1");
    EXPECT_EQ(response.value()[0]->Name(), "Tournament 1");
    EXPECT_EQ(response.value()[1]->Id(), "id-2");
    EXPECT_EQ(response.value()[1]->Name(), "Tournament 2");
}
