#include "delegate/MatchDelegate.hpp"
#include "domain/NFLStrategy.hpp"
#include <format>

MatchDelegate::MatchDelegate(
    const std::shared_ptr<IMatchRepository>& matchRepo,
    const std::shared_ptr<TournamentRepository>& tournamentRepo,
    const std::shared_ptr<IGroupRepository>& groupRepo,
    const std::shared_ptr<QueueMessageProducer>& producer)
    : matchRepository(matchRepo),
      tournamentRepository(tournamentRepo),
      groupRepository(groupRepo),
      messageProducer(producer) {

    // Registrar estrategias disponibles
    strategies["NFL"] = std::make_shared<NFLStrategy>();
    // Aquí se agregarían otras estrategias cuando las implementes:
    // strategies["RoundRobin"] = std::make_shared<RoundRobinStrategy>();
    // strategies["WorldCup"] = std::make_shared<WorldCupStrategy>();
    // strategies["DoubleElimination"] = std::make_shared<DoubleEliminationStrategy>();
}

std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
MatchDelegate::GetMatches(std::string_view tournamentId,
                         std::optional<std::string> filter) {
    // Validar que el torneo existe
    auto tournament = tournamentRepository->ReadById(tournamentId.data());
    if (!tournament) {
        return std::unexpected(tournament.error());
    }

    // Obtener matches según el filtro
    if (filter.has_value()) {
        if (*filter == "played") {
            return matchRepository->FindPlayedMatchesByTournamentId(tournamentId);
        } else if (*filter == "pending") {
            return matchRepository->FindPendingMatchesByTournamentId(tournamentId);
        }
    }

    // Sin filtro, devolver todos
    return matchRepository->FindByTournamentId(tournamentId);
}

std::expected<std::shared_ptr<domain::Match>, std::string>
MatchDelegate::GetMatch(std::string_view tournamentId, std::string_view matchId) {
    // Validar que el torneo existe
    auto tournament = tournamentRepository->ReadById(tournamentId.data());
    if (!tournament) {
        return std::unexpected(tournament.error());
    }

    // Obtener el match
    auto match = matchRepository->ReadById(matchId.data());
    if (!match) {
        return std::unexpected(match.error());
    }

    // Validar que el match pertenece al torneo
    if ((*match)->TournamentId() != tournamentId) {
        return std::unexpected("Match does not belong to the specified tournament");
    }

    return match;
}

std::expected<void, std::string>
MatchDelegate::UpdateMatchScore(std::string_view tournamentId,
                               std::string_view matchId,
                               const domain::Score& score) {
    // Validar que el torneo existe
    auto tournamentResult = tournamentRepository->ReadById(tournamentId.data());
    if (!tournamentResult) {
        return std::unexpected(tournamentResult.error());
    }
    auto tournament = *tournamentResult;

    // Obtener el match
    auto matchResult = matchRepository->ReadById(matchId.data());
    if (!matchResult) {
        return std::unexpected(matchResult.error());
    }
    auto match = *matchResult;

    // Validar que el match pertenece al torneo
    if (match->TournamentId() != tournamentId) {
        return std::unexpected("Match does not belong to the specified tournament");
    }

    // Validar el score según las reglas del torneo
    if (!ValidateScore(score, *tournament, match->Round())) {
        return std::unexpected("Invalid score for this tournament format and round");
    }

    // Actualizar el match con el score
    match->MatchScore() = score;
    auto updateResult = matchRepository->Update(matchId.data(), *match);
    if (!updateResult) {
        return std::unexpected(updateResult.error());
    }

    // Publicar evento de actualización de score
    nlohmann::json event;
    event["tournamentId"] = tournamentId;
    event["matchId"] = matchId;
    event["round"] = static_cast<int>(match->Round());
    event["homeTeamId"] = match->HomeTeamId();
    event["visitorTeamId"] = match->VisitorTeamId();
    event["homeScore"] = score.homeTeamScore;
    event["visitorScore"] = score.visitorTeamScore;

    messageProducer->SendMessage(event.dump(), "match.score-updated");

    return {};
}

std::expected<std::vector<std::string>, std::string>
MatchDelegate::CreateRegularPhaseMatches(std::string_view tournamentId) {
    // Obtener el torneo
    auto tournamentResult = tournamentRepository->ReadById(tournamentId.data());
    if (!tournamentResult) {
        return std::unexpected(tournamentResult.error());
    }
    auto tournament = *tournamentResult;

    // Obtener los grupos del torneo
    auto groupsResult = groupRepository->FindByTournamentId(tournamentId);
    if (!groupsResult) {
        return std::unexpected(groupsResult.error());
    }
    auto groups = *groupsResult;

    // Validar que todos los grupos estén completos
    int expectedGroups = tournament->Format().NumberOfGroups();
    if (groups.size() != expectedGroups) {
        return std::unexpected(std::format(
            "Tournament not ready. Has {} groups, needs {}",
            groups.size(), expectedGroups));
    }

    int maxTeamsPerGroup = tournament->Format().MaxTeamsPerGroup();
    for (const auto& group : groups) {
        if (group->Teams().size() != maxTeamsPerGroup) {
            return std::unexpected(std::format(
                "Group {} is not complete. Has {} teams, needs {}",
                group->Name(), group->Teams().size(), maxTeamsPerGroup));
        }
    }

    // Verificar si ya existen matches para este torneo
    auto existingMatches = matchRepository->FindByTournamentId(tournamentId);
    if (existingMatches && !existingMatches->empty()) {
        return std::unexpected("Matches already created for this tournament");
    }

    // Obtener la estrategia correcta
    auto strategy = GetStrategy(tournament->Format().Type());
    if (!strategy) {
        return std::unexpected("Tournament format not supported");
    }

    // Crear los matches usando la estrategia
    auto matchesResult = strategy->CreateRegularPhaseMatches(*tournament, groups);
    if (!matchesResult) {
        return std::unexpected(matchesResult.error());
    }

    // Guardar los matches en la base de datos
    std::vector<std::string> createdMatchIds;
    for (auto& match : *matchesResult) {
        auto createResult = matchRepository->Create(match);
        if (!createResult) {
            return std::unexpected(std::format(
                "Failed to create match: {}", createResult.error()));
        }
        createdMatchIds.push_back(*createResult);
    }

    return createdMatchIds;
}

std::expected<std::vector<std::string>, std::string>
MatchDelegate::CreatePlayoffMatches(std::string_view tournamentId) {
    // Obtener el torneo
    auto tournamentResult = tournamentRepository->ReadById(tournamentId.data());
    if (!tournamentResult) {
        return std::unexpected(tournamentResult.error());
    }
    auto tournament = *tournamentResult;

    // Verificar que todos los matches de fase regular estén jugados
    if (!AllRegularMatchesPlayed(tournamentId)) {
        return std::unexpected("Not all regular phase matches have been played");
    }

    // Verificar si ya existen matches de playoffs
    auto existingMatches = matchRepository->FindByTournamentId(tournamentId);
    if (existingMatches) {
        for (const auto& match : *existingMatches) {
            if (match->Round() != domain::RoundType::REGULAR) {
                return std::unexpected("Playoff matches already created");
            }
        }
    }

    // Obtener la estrategia correcta
    auto strategy = GetStrategy(tournament->Format().Type());
    if (!strategy) {
        return std::unexpected("Tournament format not supported");
    }

    // Obtener todos los matches de fase regular
    auto regularMatches = matchRepository->FindByTournamentIdAndRound(
        tournamentId, domain::RoundType::REGULAR);
    if (!regularMatches) {
        return std::unexpected(regularMatches.error());
    }

    // ✅ NUEVO: Obtener los grupos del torneo
    auto groupsResult = groupRepository->FindByTournamentId(tournamentId);
    if (!groupsResult) {
        return std::unexpected(groupsResult.error());
    }
    auto groups = *groupsResult;

    // ✅ CAMBIO: Crear los matches de playoffs usando la estrategia (CON GROUPS)
    auto playoffMatchesResult = strategy->CreatePlayoffMatches(
        *tournament, *regularMatches, groups);  // ← AGREGAR groups aquí
    if (!playoffMatchesResult) {
        return std::unexpected(playoffMatchesResult.error());
    }

    // Guardar los matches en la base de datos
    std::vector<std::string> createdMatchIds;
    for (auto& match : *playoffMatchesResult) {
        auto createResult = matchRepository->Create(match);
        if (!createResult) {
            return std::unexpected(std::format(
                "Failed to create playoff match: {}", createResult.error()));
        }
        createdMatchIds.push_back(*createResult);
    }

    return createdMatchIds;
}

bool MatchDelegate::ValidateScore(const domain::Score& score,
                                  const domain::Tournament& tournament,
                                  domain::RoundType round) {
    auto strategy = GetStrategy(tournament.Format().Type());
    if (!strategy) {
        return false;
    }

    return strategy->ValidateScore(score, round);
}

bool MatchDelegate::AllRegularMatchesPlayed(std::string_view tournamentId) {
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

std::shared_ptr<IMatchStrategy> MatchDelegate::GetStrategy(
    const domain::TournamentType& tournamentType) {  // ✅ CAMBIO: recibir TournamentType

    // ✅ CAMBIO: usar switch en lugar de map
    switch (tournamentType) {
        case domain::TournamentType::NFL:
            return strategies["NFL"];
        // case domain::TournamentType::ROUND_ROBIN:
        //     return strategies["ROUND_ROBIN"];
        // case domain::TournamentType::WORLD_CUP:
        //     return strategies["WORLD_CUP"];
        // case domain::TournamentType::DOUBLE_ELIMINATION:
        //     return strategies["DOUBLE_ELIMINATION"];
        default:
            return nullptr;
    }
}