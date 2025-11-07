#ifndef TOURNAMENTS_NFLSTRATEGY_HPP
#define TOURNAMENTS_NFLSTRATEGY_HPP

#include "IMatchStrategy.hpp"
#include <algorithm>
#include <map>
#include <set>

class NFLStrategy : public IMatchStrategy {
public:
    std::expected<std::vector<domain::Match>, std::string> 
    CreateRegularPhaseMatches(const domain::Tournament& tournament,
                             const std::vector<std::shared_ptr<domain::Group>>& groups) override {
        std::vector<domain::Match> matches;
        
        if (groups.size() != 8) {
            return std::unexpected("NFL format requires 8 groups");
        }

        for (const auto& group : groups) {
            if (group->Teams().size() != 4) {
                return std::unexpected("Each group must have exactly 4 teams");
            }
        }

        // Matches intragrupales
        for (const auto& group : groups) {
            const auto& teams = group->Teams();
            for (size_t i = 0; i < teams.size(); i++) {
                for (size_t j = i + 1; j < teams.size(); j++) {
                    domain::Match match;
                    match.TournamentId() = tournament.Id();
                    match.HomeTeamId() = teams[i].Id;
                    match.HomeTeamName() = teams[i].Name;
                    match.VisitorTeamId() = teams[j].Id;
                    match.VisitorTeamName() = teams[j].Name;
                    match.Round() = domain::RoundType::REGULAR;
                    matches.push_back(match);
                }
            }
        }

        // Matches intergrupales (mismo índice en diferentes grupos)
        for (size_t teamPos = 0; teamPos < 4; teamPos++) {
            for (size_t groupA = 0; groupA < groups.size(); groupA++) {
                for (size_t groupB = groupA + 1; groupB < groups.size(); groupB++) {
                    const auto& teamA = groups[groupA]->Teams()[teamPos];
                    const auto& teamB = groups[groupB]->Teams()[teamPos];

                    domain::Match match;
                    match.TournamentId() = tournament.Id();
                    match.HomeTeamId() = teamA.Id;
                    match.HomeTeamName() = teamA.Name;
                    match.VisitorTeamId() = teamB.Id;
                    match.VisitorTeamName() = teamB.Name;
                    match.Round() = domain::RoundType::REGULAR;
                    matches.push_back(match);
                }
            }
        }

        return matches;
    }

    std::expected<std::vector<domain::Match>, std::string>
    CreatePlayoffMatches(const domain::Tournament& tournament,
                        const std::vector<std::shared_ptr<domain::Match>>& regularMatches,
                        const std::vector<std::shared_ptr<domain::Group>>& groups) override {
        std::vector<domain::Match> playoffMatches;

        auto [afc, nfc] = ClassifyByConference(groups);
        auto afcPlayoffTeams = GetPlayoffTeamsWithNames(afc, regularMatches);
        auto nfcPlayoffTeams = GetPlayoffTeamsWithNames(nfc, regularMatches);

        if (afcPlayoffTeams.size() != 7 || nfcPlayoffTeams.size() != 7) {
            return std::unexpected("Failed to determine playoff teams");
        }

        // Wild Card (Quarterfinals) - 6 matches
        CreateWildCardMatches(playoffMatches, tournament.Id(), afcPlayoffTeams, "AFC");
        CreateWildCardMatches(playoffMatches, tournament.Id(), nfcPlayoffTeams, "NFC");

        // Divisional Round (seguirá siendo Quarterfinals) - 4 matches vacíos
        for (int i = 0; i < 4; i++) {
            domain::Match match;
            match.TournamentId() = tournament.Id();
            match.Round() = domain::RoundType::QUARTERFINALS;
            playoffMatches.push_back(match);
        }

        // Conference Championships (Semifinals) - 2 matches vacíos
        for (int i = 0; i < 2; i++) {
            domain::Match match;
            match.TournamentId() = tournament.Id();
            match.Round() = domain::RoundType::SEMIFINALS;
            playoffMatches.push_back(match);
        }

        // Super Bowl (Final) - 1 match vacío
        domain::Match finalMatch;
        finalMatch.TournamentId() = tournament.Id();
        finalMatch.Round() = domain::RoundType::FINAL;
        playoffMatches.push_back(finalMatch);

        return playoffMatches;
    }

    bool ValidateScore(const domain::Score& score, domain::RoundType round) override {
        // Rango 0-10
        if (score.homeTeamScore < 0 || score.homeTeamScore > 10 ||
            score.visitorTeamScore < 0 || score.visitorTeamScore > 10) {
            return false;
        }

        // En playoffs NO se permiten empates
        if (round != domain::RoundType::REGULAR && score.IsTie()) {
            return false;
        }

        return true;
    }

    std::expected<std::vector<domain::Match>, std::string>
    ProcessMatchResult(const domain::Match& match,
                      const std::vector<std::shared_ptr<domain::Match>>& allMatches) override {
        std::vector<domain::Match> updatedMatches;

        if (!match.IsPlayed()) {
            return std::unexpected("Match has no score");
        }

        // Para NFL, esto se maneja mejor en el delegate
        // Aquí solo validamos que el flujo sea correcto
        return updatedMatches;
    }

    std::vector<std::string>
    TabulateTeams(const std::vector<std::shared_ptr<domain::Match>>& matches,
                 const std::vector<std::shared_ptr<domain::Group>>& groups) override {
        struct TeamStats {
            std::string teamId;
            std::string groupId;
            int wins = 0;
            int losses = 0;
            int ties = 0;
            int pointsFor = 0;
            int pointsAgainst = 0;

            double winPercentage() const {
                if (wins + losses + ties == 0) return 0.0;
                return (wins + 0.5 * ties) / (wins + losses + ties);
            }

            int pointDifferential() const {
                return pointsFor - pointsAgainst;
            }
        };

        std::map<std::string, TeamStats> stats;

        // Inicializar stats
        for (const auto& group : groups) {
            for (const auto& team : group->Teams()) {
                stats[team.Id] = {team.Id, group->Id(), 0, 0, 0, 0, 0};
            }
        }

        // Recolectar estadísticas
        for (const auto& match : matches) {
            if (!match->IsPlayed() || match->Round() != domain::RoundType::REGULAR)
                continue;

            const auto& score = match->MatchScore().value();

            stats[match->HomeTeamId()].pointsFor += score.homeTeamScore;
            stats[match->HomeTeamId()].pointsAgainst += score.visitorTeamScore;
            stats[match->VisitorTeamId()].pointsFor += score.visitorTeamScore;
            stats[match->VisitorTeamId()].pointsAgainst += score.homeTeamScore;

            if (score.homeTeamScore > score.visitorTeamScore) {
                stats[match->HomeTeamId()].wins++;
                stats[match->VisitorTeamId()].losses++;
            } else if (score.homeTeamScore < score.visitorTeamScore) {
                stats[match->VisitorTeamId()].wins++;
                stats[match->HomeTeamId()].losses++;
            } else {
                stats[match->HomeTeamId()].ties++;
                stats[match->VisitorTeamId()].ties++;
            }
        }

        // Ordenar
        std::vector<TeamStats> sortedStats;
        for (const auto& [id, stat] : stats) {
            sortedStats.push_back(stat);
        }

        std::sort(sortedStats.begin(), sortedStats.end(),
                 [](const TeamStats& a, const TeamStats& b) {
            if (a.winPercentage() != b.winPercentage())
                return a.winPercentage() > b.winPercentage();
            if (a.pointsFor != b.pointsFor)
                return a.pointsFor > b.pointsFor;
            return a.pointDifferential() > b.pointDifferential();
        });

        std::vector<std::string> result;
        for (const auto& stat : sortedStats) {
            result.push_back(stat.teamId);
        }

        return result;
    }

private:
    struct Conference {
        std::string name;
        std::vector<std::shared_ptr<domain::Group>> groups;
    };

    struct TeamInfo {
        std::string id;
        std::string name;
    };

    std::pair<Conference, Conference> ClassifyByConference(
        const std::vector<std::shared_ptr<domain::Group>>& groups) {
        Conference afc{"AFC", {}};
        Conference nfc{"NFC", {}};

        for (size_t i = 0; i < groups.size(); i++) {
            if (i < 4) {
                afc.groups.push_back(groups[i]);
            } else {
                nfc.groups.push_back(groups[i]);
            }
        }

        return {afc, nfc};
    }

    // ✅ NUEVO: Versión que retorna TeamInfo con ID y nombre
    std::vector<TeamInfo> GetPlayoffTeamsWithNames(
        const Conference& conference,
        const std::vector<std::shared_ptr<domain::Match>>& matches) {

        std::vector<TeamInfo> playoffTeams;
        std::set<std::string> divisionWinners;

        // Obtener ganador de cada división
        for (const auto& group : conference.groups) {
            auto winner = GetDivisionWinnerWithName(group, matches);
            if (!winner.id.empty()) {
                playoffTeams.push_back(winner);
                divisionWinners.insert(winner.id);
            }
        }

        // Obtener wildcards
        std::vector<std::shared_ptr<domain::Group>> confGroups;
        for (const auto& group : conference.groups) {
            confGroups.push_back(group);
        }

        auto allTeamsRanked = TabulateTeams(matches, confGroups);

        int wildcardsAdded = 0;
        for (const auto& teamId : allTeamsRanked) {
            if (wildcardsAdded >= 3) break;

            if (divisionWinners.find(teamId) == divisionWinners.end()) {
                bool inConference = false;
                std::string teamName;

                for (const auto& group : conference.groups) {
                    for (const auto& team : group->Teams()) {
                        if (team.Id == teamId) {
                            inConference = true;
                            teamName = team.Name;
                            break;
                        }
                    }
                    if (inConference) break;
                }

                if (inConference) {
                    playoffTeams.push_back({teamId, teamName});
                    wildcardsAdded++;
                }
            }
        }

        return playoffTeams;
    }

    // ✅ NUEVO: Obtener ganador con nombre
    TeamInfo GetDivisionWinnerWithName(
        const std::shared_ptr<domain::Group>& group,
        const std::vector<std::shared_ptr<domain::Match>>& matches) {

        std::vector<std::shared_ptr<domain::Match>> groupMatches;
        std::set<std::string> groupTeamIds;

        for (const auto& team : group->Teams()) {
            groupTeamIds.insert(team.Id);
        }

        for (const auto& match : matches) {
            if (groupTeamIds.count(match->HomeTeamId()) &&
                groupTeamIds.count(match->VisitorTeamId())) {
                groupMatches.push_back(match);
            }
        }

        std::vector<std::shared_ptr<domain::Group>> singleGroup = {group};
        auto ranked = TabulateTeams(groupMatches, singleGroup);

        if (ranked.empty()) {
            return {"", ""};
        }

        // Buscar el nombre del equipo ganador
        std::string winnerId = ranked[0];
        for (const auto& team : group->Teams()) {
            if (team.Id == winnerId) {
                return {winnerId, team.Name};
            }
        }

        return {winnerId, ""};
    }

    void CreateWildCardMatches(std::vector<domain::Match>& matches,
                              const std::string& tournamentId,
                              const std::vector<TeamInfo>& playoffTeams,
                              const std::string& conference) {
        // Matchups: 2 vs 7, 3 vs 6, 4 vs 5 (el #1 tiene BYE)
        std::vector<std::pair<int, int>> matchups = {
            {1, 6}, // #2 vs #7
            {2, 5}, // #3 vs #6
            {3, 4}  // #4 vs #5
        };

        for (const auto& [home, away] : matchups) {
            domain::Match match;
            match.TournamentId() = tournamentId;
            match.HomeTeamId() = playoffTeams[home].id;
            match.HomeTeamName() = playoffTeams[home].name;
            match.VisitorTeamId() = playoffTeams[away].id;
            match.VisitorTeamName() = playoffTeams[away].name;
            match.Round() = domain::RoundType::QUARTERFINALS;
            matches.push_back(match);
        }
    }
};

#endif