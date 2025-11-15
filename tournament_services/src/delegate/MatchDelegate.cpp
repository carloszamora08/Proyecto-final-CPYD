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

    // Validar que el match ya tenga a ambos equipos
    if (match->getHome().id == "" || match->getVisitor().id == "") {
        return std::unexpected("Match teams are not ready");
    }

    // Validar que el match no sea de playoff ya jugado
    if (match->Round() != domain::RoundType::REGULAR && match->IsPlayed()) {
        return std::unexpected("Cannot modify an already played playoff game");
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
    event["homeTeamId"] = match->getHome().id;
    event["visitorTeamId"] = match->getVisitor().id;
    event["homeScore"] = score.homeTeamScore;
    event["visitorScore"] = score.visitorTeamScore;
    
    if (match->WinnerNextMatchId() != "") {
        event["winnerNextMatchId"] = match->WinnerNextMatchId();
    }

    messageProducer->SendMessage(event.dump(), "match.score-updated");

    if (match->Round() == domain::RoundType::SUPERBOWL) {
        tournament->Finished() = "yes";
        auto tournamentUpdateResult = tournamentRepository->Update(tournament->Id(), *tournament);
        if (!tournamentUpdateResult) {
            return std::unexpected(tournamentUpdateResult.error());
        }
    }

    return {};
}

bool MatchDelegate::ValidateScore(const domain::Score& score,
                                  const domain::Tournament& tournament,
                                  domain::RoundType round) {
    auto strategy = strategies["NFL"];

    if (!strategy) {
        return false;
    }

    return strategy->ValidateScore(score, round);
}