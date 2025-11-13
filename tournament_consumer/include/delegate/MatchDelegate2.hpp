#ifndef CONSUMER_MATCHDELEGATE2_HPP
#define CONSUMER_MATCHDELEGATE2_HPP

#include <memory>
#include <vector>
#include <print>

#include "event/TeamAddEvent.hpp"
#include "event/ScoreUpdateEvent.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "domain/Match.hpp"
#include "domain/NFLStrategy.hpp"

class MatchDelegate2 {
    std::shared_ptr<IMatchRepository> matchRepository;
    std::shared_ptr<IGroupRepository> groupRepository;
    std::shared_ptr<TournamentRepository> tournamentRepository;

public:
    MatchDelegate2(const std::shared_ptr<IMatchRepository>& matchRepository,
                  const std::shared_ptr<IGroupRepository>& groupRepository,
                  const std::shared_ptr<TournamentRepository>& tournamentRepository);

    virtual void ProcessTeamAddition(const TeamAddEvent& teamAddEvent);
    virtual void ProcessScoreUpdate(const ScoreUpdateEvent& scoreUpdateEvent);

private:
    bool IsTournamentComplete(const std::string& tournamentId);
    bool AllRegularMatchesPlayed(const std::string& tournamentId);
    void CreateRegularPhaseMatches(const std::string& tournamentId);
    void CreatePlayoffMatches(const std::string& tournamentId);
    bool CheckIfInPlayoffs(const std::string& tournamentId);
    void AdvancePlayoffMatch(const std::string& matchId);
};

inline MatchDelegate2::MatchDelegate2(
    const std::shared_ptr<IMatchRepository>& matchRepository,
    const std::shared_ptr<IGroupRepository>& groupRepository,
    const std::shared_ptr<TournamentRepository>& tournamentRepository)
    : matchRepository(matchRepository),
      groupRepository(groupRepository),
      tournamentRepository(tournamentRepository) {}

inline void MatchDelegate2::ProcessTeamAddition(const TeamAddEvent& teamAddEvent) {
    std::println("[MatchDelegate2] Processing team addition for tournament: {}", teamAddEvent.tournamentId);

    if (IsTournamentComplete(teamAddEvent.tournamentId)) {
        std::println("[MatchDelegate2] Tournament is complete! Creating matches...");
        CreateRegularPhaseMatches(teamAddEvent.tournamentId);
    } else {
        std::println("[MatchDelegate2] Tournament not complete yet, waiting for more teams...");
    }
}

inline void MatchDelegate2::ProcessScoreUpdate(const ScoreUpdateEvent& scoreUpdateEvent) {
    std::println("[MatchDelegate2] Processing score update for tournament: {}", scoreUpdateEvent.tournamentId);

    if (AllRegularMatchesPlayed(scoreUpdateEvent.tournamentId)) {
        if (CheckIfInPlayoffs(scoreUpdateEvent.tournamentId)) {
            AdvancePlayoffMatch(scoreUpdateEvent.matchId);
        } else {
            std::println("[MatchDelegate2] Regular season is complete! Creating Wild Card playoff matches...");
            CreatePlayoffMatches(scoreUpdateEvent.tournamentId);
        }
    } else {
        std::println("[MatchDelegate2] Regular season not complete yet, waiting for more scores...");
    }
}

inline bool MatchDelegate2::IsTournamentComplete(const std::string& tournamentId) {
    auto groupsResult = groupRepository->FindByTournamentId(tournamentId);

    if (!groupsResult) {
        std::println("[MatchDelegate2] Error getting groups: {}", groupsResult.error());
        return false;
    }

    auto groups = *groupsResult;

    auto tournamentResult = tournamentRepository->ReadById(tournamentId);
    if (!tournamentResult) {
        std::println("[MatchDelegate2] Error getting tournament: {}", tournamentResult.error());
        return false;
    }

    auto tournament = *tournamentResult;
    int expectedGroups = tournament->Format().NumberOfGroups();
    int teamsPerGroup = tournament->Format().MaxTeamsPerGroup();

    std::println("[MatchDelegate2] Tournament has {} groups, expected {}", groups.size(), expectedGroups);

    if (groups.size() != expectedGroups) {
        return false;
    }

    for (const auto& group : groups) {
        std::println("[MatchDelegate2] Group '{}' has {} teams (expected {})",
                     group->Name(), group->Teams().size(), teamsPerGroup);

        if (group->Teams().size() != teamsPerGroup) {
            return false;
        }
    }

    return true;
}

inline bool MatchDelegate2::AllRegularMatchesPlayed(const std::string& tournamentId) {
    auto pendingMatches = matchRepository->FindPendingMatchesByTournamentId(tournamentId);

    if (!pendingMatches) {
        return false;
    }

    // Contar solo los matches de fase regular pendientes
    int regularPending = 0;
    for (const auto& match : *pendingMatches) {
        if (match->Round() == domain::RoundType::REGULAR) {
            regularPending++;
        }
    }

    return regularPending == 0;
}

inline void MatchDelegate2::CreateRegularPhaseMatches(const std::string& tournamentId) {
    std::println("[MatchDelegate2] Creating regular phase matches for tournament {}", tournamentId);

    auto tournamentResult = tournamentRepository->ReadById(tournamentId);
    if (!tournamentResult) {
        std::println("[MatchDelegate2] ERROR: Cannot get tournament: {}", tournamentResult.error());
        return;
    }
    auto tournament = *tournamentResult;

    auto groupsResult = groupRepository->FindByTournamentId(tournamentId);
    if (!groupsResult) {
        std::println("[MatchDelegate2] ERROR: Cannot get groups: {}", groupsResult.error());
        return;
    }
    auto groups = *groupsResult;

    // Usar la estrategia NFL (orden correcto de parámetros)
    NFLStrategy strategy;
    auto matchesResult = strategy.CreateRegularPhaseMatches(*tournament, groups);

    if (!matchesResult) {
        std::println("[MatchDelegate2] ERROR: Strategy failed: {}", matchesResult.error());
        return;
    }

    auto matches = *matchesResult;
    std::println("[MatchDelegate2] Strategy created {} matches", matches.size());

    // Guardar cada match en la BD
    int successCount = 0;
    for (auto& match : matches) {
        auto result = matchRepository->Create(match);
        if (result) {
            successCount++;
        } else {
            std::println("[MatchDelegate2] ERROR creating match: {}", result.error());
        }
    }

    std::println("[MatchDelegate2] SUCCESS: Created {}/{} matches for tournament {}",
                 successCount, matches.size(), tournamentId);
}

inline void MatchDelegate2::CreatePlayoffMatches(const std::string& tournamentId) {
    std::println("[MatchDelegate2] Creating playoff matches for tournament {}", tournamentId);

    auto tournamentResult = tournamentRepository->ReadById(tournamentId);
    if (!tournamentResult) {
        std::println("[MatchDelegate2] ERROR: Cannot get tournament: {}", tournamentResult.error());
        return;
    }
    auto tournament = *tournamentResult;

    auto groupsResult = groupRepository->FindByTournamentId(tournamentId);
    if (!groupsResult) {
        std::println("[MatchDelegate2] ERROR: Cannot get groups: {}", groupsResult.error());
        return;
    }
    auto groups = *groupsResult;

    auto regularMatchesResult = matchRepository->FindByTournamentId(tournamentId);
    if (!regularMatchesResult) {
        std::println("[MatchDelegate2] ERROR: Cannot get regular season matches: {}", regularMatchesResult.error());
        return;
    }
    auto regularMatches = *regularMatchesResult;

    // Usar la estrategia NFL (orden correcto de parámetros)
    NFLStrategy strategy;
    auto matchesResult = strategy.CreatePlayoffMatches(*tournament, regularMatches, groups);

    if (!matchesResult) {
        std::println("[MatchDelegate2] ERROR: Strategy failed: {}", matchesResult.error());
        return;
    }

    auto matches = *matchesResult;
    std::println("[MatchDelegate2] Strategy created {} playoff matches", matches.size());

    std::vector<std::string> playoffMatchesIds;
    std::vector<std::shared_ptr<domain::Match>> playoffMatches;

    // Guardar cada match en la BD
    int successCount = 0;
    for (auto& match : matches) {
        auto result = matchRepository->Create(match);
        if (result) {
            successCount++;
            playoffMatchesIds.push_back(*result);
            playoffMatches.push_back(*matchRepository->ReadById(*result));
        } else {
            std::println("[MatchDelegate2] ERROR creating match: {}", result.error());
        }
    }

    // Generar avances
    playoffMatches[0]->WinnerNextMatchId() = playoffMatchesIds[6];
    playoffMatches[1]->WinnerNextMatchId() = playoffMatchesIds[7];
    playoffMatches[2]->WinnerNextMatchId() = playoffMatchesIds[7];

    playoffMatches[3]->WinnerNextMatchId() = playoffMatchesIds[8];
    playoffMatches[4]->WinnerNextMatchId() = playoffMatchesIds[9];
    playoffMatches[5]->WinnerNextMatchId() = playoffMatchesIds[9];

    playoffMatches[6]->WinnerNextMatchId() = playoffMatchesIds[10];
    playoffMatches[7]->WinnerNextMatchId() = playoffMatchesIds[10];

    playoffMatches[8]->WinnerNextMatchId() = playoffMatchesIds[11];
    playoffMatches[9]->WinnerNextMatchId() = playoffMatchesIds[11];

    playoffMatches[10]->WinnerNextMatchId() = playoffMatchesIds[12];
    playoffMatches[11]->WinnerNextMatchId() = playoffMatchesIds[12];

    // Actualizar equipos
    for (auto& playoffMatch : playoffMatches) {
        matchRepository->Update(playoffMatch->Id(), *playoffMatch);
    }

    std::println("[MatchDelegate2] SUCCESS: Created {}/{} playoff matches for tournament {}",
                 successCount, matches.size(), tournamentId);
}

inline bool MatchDelegate2::CheckIfInPlayoffs(const std::string& tournamentId) {
    std::println("[MatchDelegate2] Checking if tournament {} has entered playoffs", tournamentId);

    auto tournamentResult = tournamentRepository->ReadById(tournamentId);
    if (!tournamentResult) {
        std::println("[MatchDelegate2] ERROR: Cannot get tournament: {}", tournamentResult.error());
        return false;
    }
    auto tournament = *tournamentResult;

    auto matchesResult = matchRepository->FindByTournamentIdAndRound(tournamentId, domain::RoundType::WILDCARD);
    if (!matchesResult) {
        std::println("[MatchDelegate2] ERROR: Cannot get matches: {}", matchesResult.error());
        return false;
    }
    auto matches = *matchesResult;

    if (matches.size() == 0) {
        std::println("[MatchDelegate2] Tournmanet {} is still in regular season", tournamentId);
        return false;
    } else {
        std::println("[MatchDelegate2] Tournmanet {} is in playoffs", tournamentId);
        return true;
    }
}

inline void MatchDelegate2::AdvancePlayoffMatch(const std::string& matchId) {
    auto matchResult = matchRepository->ReadById(matchId);
    if (!matchResult) {
        std::println("[MatchDelegate2] ERROR: Cannot get match: {}", matchResult.error());
        return;
    }
    auto match = *matchResult;

    // Checar si es partido de playoff no super bowl
    if (match->Round() != domain::RoundType::REGULAR && match->Round() != domain::RoundType::SUPERBOWL) {
        // Pasar equipo ganador al siguiente match
        auto nextMatch = matchRepository->ReadById(match->WinnerNextMatchId());
        
        // Verificar que el match exista
        if (!nextMatch) {
            std::println("[MatchDelegate2] ERROR: Cannot get next match: {}", nextMatch.error());
            return;
        }

        // Asignar equipo a espacio disponible
        if (nextMatch.value()->getHome().id == "") {
            if (match->MatchScore()->GetWinner() == domain::Winner::HOME) {
                nextMatch.value()->getHome() = match->getHome();
            } else {
                nextMatch.value()->getHome() = domain::Home(match->getVisitor().id, match->getVisitor().name);
            }
        } else {
            if (match->MatchScore()->GetWinner() == domain::Winner::HOME) {
                nextMatch.value()->getVisitor() = domain::Visitor(match->getHome().id, match->getHome().name);
            } else {
                nextMatch.value()->getVisitor() = match->getVisitor();
            }
        }

        matchRepository->Update(nextMatch.value()->Id(), *nextMatch.value());
    }
}

#endif //CONSUMER_MATCHDELEGATE2_HPP