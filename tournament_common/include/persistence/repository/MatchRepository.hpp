#ifndef RESTAPI_MATCHREPOSITORY_HPP
#define RESTAPI_MATCHREPOSITORY_HPP

#include <memory>
#include "IMatchRepository.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"

class MatchRepository : public IMatchRepository {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;

public:
    explicit MatchRepository(std::shared_ptr<IDbConnectionProvider> connection);

    std::expected<std::string, std::string> Create(const domain::Match& match) override;
    std::expected<std::shared_ptr<domain::Match>, std::string> ReadById(const std::string& id) override;
    std::expected<std::string, std::string> Update(const std::string& id, const domain::Match& match) override;
    std::expected<void, std::string> Delete(const std::string& id) override;

    std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
        FindByTournamentId(const std::string_view& tournamentId) override;

    std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
        FindByTournamentIdAndRound(const std::string_view& tournamentId, domain::RoundType round) override;

    std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
        FindPlayedMatchesByTournamentId(const std::string_view& tournamentId) override;

    std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
        FindPendingMatchesByTournamentId(const std::string_view& tournamentId) override;

    std::expected<std::shared_ptr<domain::Match>, std::string>
        FindLastOpenMatch(const std::string_view& tournamentId) override;
};
#endif