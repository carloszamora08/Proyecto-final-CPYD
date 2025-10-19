//
// Created by tsuny on 9/1/25.
//

#ifndef TOURNAMENTS_TOURNAMENTREPOSITORY_HPP
#define TOURNAMENTS_TOURNAMENTREPOSITORY_HPP
#include <string>

#include "IRepository.hpp"
#include "domain/Tournament.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"


class TournamentRepository : public IRepository<domain::Tournament, std::string, std::expected<std::string, std::string>> {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;
public:
    explicit TournamentRepository(std::shared_ptr<IDbConnectionProvider> connectionProvider);
    std::expected<std::string, std::string> Create (const domain::Tournament & entity) override;
    std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string> ReadAll() override;
    std::expected<std::shared_ptr<domain::Tournament>, std::string> ReadById(std::string id) override;
    std::expected<std::string, std::string> Update (std::string id, const domain::Tournament & entity) override;
    std::expected<void, std::string> Delete(std::string id) override;
};

#endif //TOURNAMENTS_TOURNAMENTREPOSITORY_HPP