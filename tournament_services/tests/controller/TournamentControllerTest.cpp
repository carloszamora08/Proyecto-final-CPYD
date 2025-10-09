#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Tournament.hpp"
#include "delegate/ITournamentDelegate.hpp"
#include "controller/TournamentController.hpp"
#include "domain/Utilities.hpp"

class TournamentDelegateMock : public ITournamentDelegate {
public:
    MOCK_METHOD(std::string, CreateTournament, (const std::shared_ptr<domain::Tournament> tournament), (override));
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, GetTournament, (const std::string_view id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(std::string, UpdateTournament, (const std::string_view id, const std::shared_ptr<domain::Tournament> tournament), (override));
    MOCK_METHOD(void, DeleteTournament, (const std::string_view id), (override));
};

class TournamentControllerTest : public ::testing::Test{
protected:
    std::shared_ptr<TournamentDelegateMock> tournamentDelegateMock;
    std::shared_ptr<TournamentController> tournamentController;

    void SetUp() override {
        tournamentDelegateMock = std::make_shared<TournamentDelegateMock>();
        tournamentController = std::make_shared<TournamentController>(TournamentController(tournamentDelegateMock));
    }

    // TearDown() function
    void TearDown() override {
        // teardown code comes here
    }

};

TEST_F(TournamentControllerTest, CreateTournamentJSONTransformationTest) {
    domain::Tournament capturedTournament;

    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournament),
                testing::Return("new-id")
            )
        );

    nlohmann::json tournamentRequestBody = {{"id", "new-id"}, {"name", "new tournament"}};
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->CreateTournament(tournamentRequest);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ(tournamentRequestBody.at("id").get<std::string>(), capturedTournament.Id());
    EXPECT_EQ(tournamentRequestBody.at("name").get<std::string>(), capturedTournament.Name());
}

TEST_F(TournamentControllerTest, CreateTournamentDBInsertionErrorTest) {
    domain::Tournament capturedTournament;

    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournament),
                testing::Throw(std::runtime_error("Tournament insertion to DB failed"))
            )
        );
    
    nlohmann::json tournamentRequestBody = {{"id", "dup-id"}, {"name", "existing tournament"}};
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->CreateTournament(tournamentRequest);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(tournamentRequestBody.at("id").get<std::string>(), capturedTournament.Id());
    EXPECT_EQ(tournamentRequestBody.at("name").get<std::string>(), capturedTournament.Name());

    EXPECT_EQ(crow::CONFLICT, response.code);
}
