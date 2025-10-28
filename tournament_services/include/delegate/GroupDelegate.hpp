#ifndef SERVICE_GROUP_DELEGATE_HPP
#define SERVICE_GROUP_DELEGATE_HPP

#include <string>
#include <string_view>
#include <memory>
#include <expected>

#include "IGroupDelegate.hpp"
#include "../cms/QueueMessageProducer.hpp"

class GroupDelegate : public IGroupDelegate{
    std::shared_ptr<TournamentRepository> tournamentRepository;
    std::shared_ptr<IGroupRepository> groupRepository;
    std::shared_ptr<TeamRepository> teamRepository;
    std::shared_ptr<QueueMessageProducer> messageProducer;

public:
    inline GroupDelegate(const std::shared_ptr<TournamentRepository>& tournamentRepository, const std::shared_ptr<IGroupRepository>& groupRepository, const std::shared_ptr<TeamRepository>& teamRepository, const std::shared_ptr<QueueMessageProducer>& messageProducer);
    std::expected<std::string, std::string> CreateGroup(const std::string_view& tournamentId, const domain::Group& group) override;
    std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string> GetGroups(const std::string_view& tournamentId) override;
    std::expected<std::shared_ptr<domain::Group>, std::string> GetGroup(const std::string_view& tournamentId, const std::string_view& groupId) override;
    std::expected<void, std::string> UpdateGroup(const std::string_view& tournamentId, const domain::Group& group, bool updateTeams) override;
    std::expected<void, std::string> RemoveGroup(const std::string_view& tournamentId, const std::string_view& groupId) override;
    std::expected<void, std::string> UpdateTeams(const std::string_view& tournamentId, const std::string_view& groupId, const std::vector<domain::Team>& teams) override;
};

GroupDelegate::GroupDelegate(
    const std::shared_ptr<TournamentRepository>& tournamentRepository,
    const std::shared_ptr<IGroupRepository>& groupRepository,
    const std::shared_ptr<TeamRepository>& teamRepository,
    const std::shared_ptr<QueueMessageProducer>& messageProducer)
    : tournamentRepository(tournamentRepository),
      groupRepository(groupRepository),
      teamRepository(teamRepository),
      messageProducer(messageProducer){}

inline std::expected<std::string, std::string> GroupDelegate::CreateGroup(const std::string_view& tournamentId, const domain::Group& group) {
    const auto tournament = tournamentRepository->ReadById(tournamentId.data());

    if (!tournament) {
        return std::unexpected(tournament.error());
    }

    const auto existingGroups = groupRepository->FindByTournamentId(tournamentId);

    if (!existingGroups) {
        return std::unexpected(existingGroups.error());
    }
    if (existingGroups.value().size() >= tournament.value()->Format().NumberOfGroups()) {
        return std::unexpected("Tournament has reached maximum number of groups");
    }

    domain::Group g = group;
    g.TournamentId() = tournament.value()->Id();

    if (g.Teams().size() > tournament.value()->Format().MaxTeamsPerGroup()) {
        return std::unexpected("Group exceeds maximum teams capacity");
    }

    if (!g.Teams().empty()) {
        for (const auto& t : g.Teams()) {
            const auto team = teamRepository->ReadById(t.Id);
            if (!team) {
                return std::unexpected(team.error());
            }

            const auto groupTeams = groupRepository->FindByTournamentIdAndTeamId(tournamentId, t.Id);
            if (groupTeams) {
                return std::unexpected(std::format("Team {} already exists in another group", t.Id));
            }
        }
    }

    return groupRepository->Create(g);
}

inline std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string> GroupDelegate::GetGroups(const std::string_view& tournamentId) {
    return groupRepository->FindByTournamentId(tournamentId);
}

inline std::expected<std::shared_ptr<domain::Group>, std::string> GroupDelegate::GetGroup(const std::string_view& tournamentId, const std::string_view& groupId) {
    return groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
}

inline std::expected<void, std::string> GroupDelegate::UpdateGroup(const std::string_view& tournamentId, const domain::Group& group, bool updateTeams) {
    const auto existingGroup = groupRepository->FindByTournamentIdAndGroupId(tournamentId, group.Id());
    if (!existingGroup) {
        return std::unexpected(existingGroup.error());
    }

    domain::Group updatedGroup = group;

    if (!updateTeams) {
        updatedGroup.Teams() = existingGroup.value()->Teams();
    } else {
        const auto tournament = tournamentRepository->ReadById(tournamentId.data());

        if (!tournament) {
            return std::unexpected(tournament.error());
        }
        if (updatedGroup.Teams().size() > tournament.value()->Format().MaxTeamsPerGroup()) {
            return std::unexpected("Group exceeds maximum teams capacity");
        }

        for (const auto& team : updatedGroup.Teams()) {
            const auto persistedTeam = teamRepository->ReadById(team.Id);
            if (!persistedTeam) {
                return std::unexpected(persistedTeam.error());
            }
           
            const auto teamInGroup = groupRepository->FindByTournamentIdAndTeamId(tournamentId, team.Id);
            if (teamInGroup && teamInGroup.value()->Id() != updatedGroup.Id()) {
                return std::unexpected(std::format("Team {} already exists in another group", team.Id));
            }
        }
    }

    const auto result = groupRepository->Update(updatedGroup.Id(), updatedGroup);

    if (!result) {
        return std::unexpected(result.error());
    }

    return {};
}

inline std::expected<void, std::string> GroupDelegate::RemoveGroup(const std::string_view& tournamentId, const std::string_view& groupId) {
    const auto existingGroup = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (!existingGroup) {
        return std::unexpected(existingGroup.error());
    }

    return groupRepository->Delete(groupId.data());
}

inline std::expected<void, std::string> GroupDelegate::UpdateTeams(const std::string_view& tournamentId, const std::string_view& groupId, const std::vector<domain::Team>& teams) {
    const auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (!group) {
        return std::unexpected(group.error());
    }

    const auto tournament = tournamentRepository->ReadById(tournamentId.data());
    if (!tournament) {
        return std::unexpected(tournament.error());
    }
    if (group.value()->Teams().size() + teams.size() > tournament.value()->Format().MaxTeamsPerGroup()) {
        return std::unexpected("Group exceeds maximum teams capacity");
    }

    for (const auto& team : teams) {
        const auto persistedTeam = teamRepository->ReadById(team.Id);
        if (!persistedTeam) {
            return std::unexpected(persistedTeam.error());
        }

        const auto groupTeams = groupRepository->FindByTournamentIdAndTeamId(tournamentId, team.Id);
        if (groupTeams) {
            return std::unexpected(std::format("Team {} already exists in another group", team.Id));
        }
    }

    for (const auto& team : teams) {
        const auto persistedTeam = teamRepository->ReadById(team.Id);
        const auto updateResult = groupRepository->UpdateGroupAddTeam(groupId, persistedTeam.value());
        if (!updateResult) {
            return std::unexpected(updateResult.error());
        }

        std::unique_ptr<nlohmann::json> message = std::make_unique<nlohmann::json>();
        message->emplace("tournamentId", tournamentId);
        message->emplace("groupId", groupId);
        message->emplace("teamId", team.Id);
        messageProducer->SendMessage(message->dump(), "tournament.team-add");
    }

    return {};
}

#endif /* SERVICE_GROUP_DELEGATE_HPP */
