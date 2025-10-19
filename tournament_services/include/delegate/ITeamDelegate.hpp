#ifndef ITEAM_DELEGATE_HPP
#define ITEAM_DELEGATE_HPP

#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <expected>

#include "domain/Team.hpp"

class ITeamDelegate {
    public:
    virtual ~ITeamDelegate() = default;
    virtual std::expected<std::string, std::string> CreateTeam(std::shared_ptr<domain::Team> team) = 0;
    virtual std::expected<std::shared_ptr<domain::Team>, std::string> GetTeam(std::string_view id) = 0;
    virtual std::expected<std::vector<std::shared_ptr<domain::Team>>, std::string> ReadAll() = 0;
    virtual std::expected<std::string, std::string> UpdateTeam(std::string_view id, std::shared_ptr<domain::Team> team) = 0;
    virtual std::expected<void, std::string> DeleteTeam(std::string_view id) = 0;
};

#endif /* ITEAM_DELEGATE_HPP */
