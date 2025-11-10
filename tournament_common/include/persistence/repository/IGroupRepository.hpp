//
// Created by root on 9/27/25.
//

#ifndef COMMON_IGROUPREPOSITORY_HPP
#define COMMON_IGROUPREPOSITORY_HPP

#include <expected>

#include "domain/Group.hpp"
#include "IRepository.hpp"

class IGroupRepository : public IRepository<domain::Group, std::string, std::expected<std::string, std::string>> {
public:
    virtual std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string> FindByTournamentId(const std::string_view& tournamentId) = 0;
    virtual std::expected<std::shared_ptr<domain::Group>, std::string> FindByTournamentIdAndGroupId(const std::string_view& tournamentId, const std::string_view& groupId) = 0;
    virtual std::expected<std::shared_ptr<domain::Group>, std::string> FindByTournamentIdAndTeamId(const std::string_view& tournamentId, const std::string_view& teamId) = 0;
    virtual std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string> FindByTournamentIdAndConference(const std::string_view& tournamentId, const std::string_view& conference) = 0;
    virtual std::expected<void, std::string> UpdateGroupAddTeam(const std::string_view& groupId, const std::shared_ptr<domain::Team> & team) = 0;
};
#endif //COMMON_IGROUPREPOSITORY_HPP