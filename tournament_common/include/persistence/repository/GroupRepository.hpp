//
// Created by root on 9/21/25.
//

#ifndef TOURNAMENTS_GROUPREPOSITORY_HPP
#define TOURNAMENTS_GROUPREPOSITORY_HPP

#include <string>
#include <memory>

#include "IGroupRepository.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "domain/Group.hpp"

class GroupRepository : public IGroupRepository {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;
public:
    explicit GroupRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider);
    std::expected<std::string, std::string> Create (const domain::Group & entity) override;
    std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string> ReadAll() override;
    std::expected<std::shared_ptr<domain::Group>, std::string> ReadById(std::string id) override;
    std::expected<std::string, std::string> Update (std::string id, const domain::Group & entity) override;
    std::expected<void, std::string> Delete(std::string id) override;
    std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string> FindByTournamentId(const std::string_view& tournamentId) override;
    std::expected<std::shared_ptr<domain::Group>, std::string> FindByTournamentIdAndGroupId(const std::string_view& tournamentId, const std::string_view& groupId) override;
    std::expected<std::shared_ptr<domain::Group>, std::string> FindByTournamentIdAndTeamId(const std::string_view& tournamentId, const std::string_view& teamId) override;
    std::expected<void, std::string> UpdateGroupAddTeam(const std::string_view& groupId, const std::shared_ptr<domain::Team> & team) override;
};

#endif //TOURNAMENTS_GROUPREPOSITORY_HPP