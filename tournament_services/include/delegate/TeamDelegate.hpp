//
// Created by tomas on 8/22/25.
//

#ifndef RESTAPI_TESTDELEGATE_HPP
#define RESTAPI_TESTDELEGATE_HPP
#include <memory>

#include "persistence/repository/IRepository.hpp"
#include "domain/Team.hpp"
#include "ITeamDelegate.hpp"

class TeamDelegate : public ITeamDelegate {
    std::shared_ptr<IRepository<domain::Team, std::string, std::expected<std::string, std::string>>> teamRepository;

public:
    explicit TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string, std::expected<std::string, std::string>>> repository);
    std::expected<std::string, std::string> CreateTeam(std::shared_ptr<domain::Team> team) override;
    std::expected<std::shared_ptr<domain::Team>, std::string> GetTeam(std::string_view id) override;
    std::expected<std::vector<std::shared_ptr<domain::Team>>, std::string> ReadAll() override;
    std::expected<std::string, std::string> UpdateTeam(std::string_view id, std::shared_ptr<domain::Team> team) override;
    std::expected<void, std::string> DeleteTeam(std::string_view id) override;
};


#endif //RESTAPI_TESTDELEGATE_HPP