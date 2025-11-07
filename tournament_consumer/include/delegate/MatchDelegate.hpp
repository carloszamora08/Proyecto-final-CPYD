#ifndef CONSUMER_MATCHDELEGATE_HPP
#define CONSUMER_MATCHDELEGATE_HPP

#include <memory>
#include <vector>
#include <print>

#include "event/TeamAddEvent.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "domain/Match.hpp"
#include "domain/NFLStrategy.hpp"

class MatchDelegate {
    std::shared_ptr<IMatchRepository> matchRepository;
    std::shared_ptr<IGroupRepository> groupRepository;
    std::shared_ptr<TournamentRepository> tournamentRepository;

public:
    MatchDelegate(const std::shared_ptr<IMatchRepository>& matchRepository,
                  const std::shared_ptr<IGroupRepository>& groupRepository,
                  const std::shared_ptr<TournamentRepository>& tournamentRepository);

    void ProcessTeamAddition(const TeamAddEvent& teamAddEvent);

private:
    bool IsTournamentComplete(const std::string& tournamentId);
    void CreateRegularPhaseMatches(const std::string& tournamentId);
};

inline MatchDelegate::MatchDelegate(
    const std::shared_ptr<IMatchRepository>& matchRepository,
    const std::shared_ptr<IGroupRepository>& groupRepository,
    const std::shared_ptr<TournamentRepository>& tournamentRepository)
    : matchRepository(matchRepository),
      groupRepository(groupRepository),
      tournamentRepository(tournamentRepository) {}

inline void MatchDelegate::ProcessTeamAddition(const TeamAddEvent& teamAddEvent) {
    std::println("[MatchDelegate] Processing team addition for tournament: {}", teamAddEvent.tournamentId);

    if (IsTournamentComplete(teamAddEvent.tournamentId)) {
        std::println("[MatchDelegate] Tournament is complete! Creating matches...");
        CreateRegularPhaseMatches(teamAddEvent.tournamentId);
    } else {
        std::println("[MatchDelegate] Tournament not complete yet, waiting for more teams...");
    }
}

inline bool MatchDelegate::IsTournamentComplete(const std::string& tournamentId) {
    auto groupsResult = groupRepository->FindByTournamentId(tournamentId);

    if (!groupsResult) {
        std::println("[MatchDelegate] Error getting groups: {}", groupsResult.error());
        return false;
    }

    auto groups = *groupsResult;

    auto tournamentResult = tournamentRepository->ReadById(tournamentId);
    if (!tournamentResult) {
        std::println("[MatchDelegate] Error getting tournament: {}", tournamentResult.error());
        return false;
    }

    auto tournament = *tournamentResult;
    int expectedGroups = tournament->Format().NumberOfGroups();
    int teamsPerGroup = tournament->Format().MaxTeamsPerGroup();

    std::println("[MatchDelegate] Tournament has {} groups, expected {}", groups.size(), expectedGroups);

    if (groups.size() != expectedGroups) {
        return false;
    }

    for (const auto& group : groups) {
        std::println("[MatchDelegate] Group '{}' has {} teams (expected {})",
                     group->Name(), group->Teams().size(), teamsPerGroup);

        if (group->Teams().size() != teamsPerGroup) {
            return false;
        }
    }

    return true;
}

inline void MatchDelegate::CreateRegularPhaseMatches(const std::string& tournamentId) {
    std::println("[MatchDelegate] Creating regular phase matches for tournament {}", tournamentId);

    auto tournamentResult = tournamentRepository->ReadById(tournamentId);
    if (!tournamentResult) {
        std::println("[MatchDelegate] ERROR: Cannot get tournament: {}", tournamentResult.error());
        return;
    }
    auto tournament = *tournamentResult;

    auto groupsResult = groupRepository->FindByTournamentId(tournamentId);
    if (!groupsResult) {
        std::println("[MatchDelegate] ERROR: Cannot get groups: {}", groupsResult.error());
        return;
    }
    auto groups = *groupsResult;

    // ✅ ARREGLADO: Usar la estrategia NFL (orden correcto de parámetros)
    NFLStrategy strategy;
    auto matchesResult = strategy.CreateRegularPhaseMatches(*tournament, groups);

    if (!matchesResult) {
        std::println("[MatchDelegate] ERROR: Strategy failed: {}", matchesResult.error());
        return;
    }

    auto matches = *matchesResult;
    std::println("[MatchDelegate] Strategy created {} matches", matches.size());

    // Guardar cada match en la BD
    int successCount = 0;
    for (auto& match : matches) {
        auto result = matchRepository->Create(match);
        if (result) {
            successCount++;
        } else {
            std::println("[MatchDelegate] ERROR creating match: {}", result.error());
        }
    }

    std::println("[MatchDelegate] SUCCESS: Created {}/{} matches for tournament {}",
                 successCount, matches.size(), tournamentId);
}

#endif //CONSUMER_MATCHDELEGATE_HPP