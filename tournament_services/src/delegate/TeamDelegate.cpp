//
// Created by tomas on 8/22/25.
//

#include "delegate/TeamDelegate.hpp"

#include <utility>

TeamDelegate::TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string_view> > repository) : teamRepository(std::move(repository)) {
}

std::vector<std::shared_ptr<domain::Team>> TeamDelegate::GetAllTeams() {
    return teamRepository->ReadAll();
}

std::shared_ptr<domain::Team> TeamDelegate::GetTeam(std::string_view id) {
    return teamRepository->ReadById(id.data());
}

std::string TeamDelegate::UpdateTeam(std::string_view id, std::shared_ptr<domain::Team> team) {
    std::shared_ptr<domain::Team> tp = std::move(team);

    std::string id2 = teamRepository->Update(id.data(), *tp).data();

    return id2;
}

void TeamDelegate::DeleteTeam(std::string_view id) {
    teamRepository->Delete(id.data());
    return;
}

std::string_view TeamDelegate::SaveTeam(const domain::Team& team){

    return teamRepository->Create(team);
}


