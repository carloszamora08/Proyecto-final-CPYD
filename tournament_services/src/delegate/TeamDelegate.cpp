//
// Created by tomas on 8/22/25.
//

#include "delegate/TeamDelegate.hpp"

#include <utility>
#include <string_view>
#include <memory>

TeamDelegate::TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string, std::expected<std::string, std::string>>> repository)
    : teamRepository(std::move(repository)) {
}

std::expected<std::string, std::string> TeamDelegate::CreateTeam(std::shared_ptr<domain::Team> team) {
    return teamRepository->Create(*team);
}

std::expected<std::shared_ptr<domain::Team>, std::string> TeamDelegate::GetTeam(std::string_view id) {
    return teamRepository->ReadById(std::string(id));
}

std::expected<std::vector<std::shared_ptr<domain::Team>>, std::string> TeamDelegate::ReadAll() {
    return teamRepository->ReadAll();
}

std::expected<std::string, std::string> TeamDelegate::UpdateTeam(std::string_view id, std::shared_ptr<domain::Team> team) {
    return teamRepository->Update(std::string(id), *team);
}

std::expected<void, std::string> TeamDelegate::DeleteTeam(std::string_view id) {
    return teamRepository->Delete(std::string(id));
}

