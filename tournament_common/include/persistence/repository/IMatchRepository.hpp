//
// Created by developer on 10/14/25.
//

#ifndef TOURNAMENTS_IMATCHREPOSITORY_HPP
#define TOURNAMENTS_IMATCHREPOSITORY_HPP

#include <string_view>
#include <vector>
#include <memory>
#include <expected>
#include "domain/Match.hpp"

class IMatchRepository {
public:
    virtual ~IMatchRepository() = default;

    virtual std::expected<std::string, std::string> Create(const domain::Match& match) = 0;
    virtual std::expected<std::shared_ptr<domain::Match>, std::string> ReadById(const std::string& id) = 0;
    virtual std::expected<std::string, std::string> Update(const std::string& id, const domain::Match& match) = 0;
    virtual std::expected<void, std::string> Delete(const std::string& id) = 0;

    // Búsquedas específicas para matches
    virtual std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
        FindByTournamentId(const std::string_view& tournamentId) = 0;

    virtual std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
        FindByTournamentIdAndRound(const std::string_view& tournamentId, domain::RoundType round) = 0;

    virtual std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
        FindPlayedMatchesByTournamentId(const std::string_view& tournamentId) = 0;

    virtual std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
        FindPendingMatchesByTournamentId(const std::string_view& tournamentId) = 0;

    // Para el bracket de doble eliminación
    virtual std::expected<std::shared_ptr<domain::Match>, std::string>
        FindLastOpenMatch(const std::string_view& tournamentId) = 0;
};
#endif //TOURNAMENTS_IMATCHREPOSITORY_HPP