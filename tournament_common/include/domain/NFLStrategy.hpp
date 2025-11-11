#ifndef TOURNAMENTS_NFLSTRATEGY_HPP
#define TOURNAMENTS_NFLSTRATEGY_HPP

#include "IMatchStrategy.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <print>

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
                    // Parte 1
                    domain::Match match;
                    domain::Home home{teams[i].Id, teams[i].Name};
                    domain::Visitor visitor{teams[j].Id, teams[j].Name};
                    match.TournamentId() = tournament.Id();
                    match.getHome() = home;
                    match.getVisitor() = visitor;
                    match.Round() = domain::RoundType::REGULAR;
                    matches.push_back(match);

                    // Parte 2
                    domain::Match match2;
                    domain::Home home2{teams[j].Id, teams[j].Name};
                    domain::Visitor visitor2{teams[i].Id, teams[i].Name};
                    match2.TournamentId() = tournament.Id();
                    match2.getHome() = home2;
                    match2.getVisitor() = visitor2;
                    match2.Round() = domain::RoundType::REGULAR;
                    matches.push_back(match2);
                }
            }
        }

        // Separate groups by conference
        std::vector<std::shared_ptr<domain::Group>> afcGroups;
        std::vector<std::shared_ptr<domain::Group>> nfcGroups;

        for (const auto& group : groups) {
            if (group->getConference() == domain::Conference::AFC) {
                afcGroups.push_back(group);
            } else {
                nfcGroups.push_back(group);
            }
        }

        std::vector<std::pair<int, int>> divisionPairs = {
            {0, 1},  // Division 0 vs Division 1
            {2, 3}   // Division 2 vs Division 3
        };

        // Process AFC divisions
        for (const auto& [div1Index, div2Index] : divisionPairs) {
            const auto& division1 = afcGroups[div1Index];
            const auto& division2 = afcGroups[div2Index];
            
            const auto& teams1 = division1->Teams();
            const auto& teams2 = division2->Teams();
            
            for (size_t i = 0; i < teams1.size(); i++) {                
                for (size_t j = 0; j < teams2.size(); j++) {
                    bool isHome = (i + j) % 2 == 0;
                    
                    domain::Match match;
                    match.TournamentId() = tournament.Id();
                    match.Round() = domain::RoundType::REGULAR;
                    
                    if (isHome) {
                        match.getHome() = domain::Home{teams1[i].Id, teams1[i].Name};
                        match.getVisitor() = domain::Visitor{teams2[j].Id, teams2[j].Name};
                    } else {
                        match.getHome() = domain::Home{teams2[j].Id, teams2[j].Name};
                        match.getVisitor() = domain::Visitor{teams1[i].Id, teams1[i].Name};
                    }
                    
                    matches.push_back(match);
                }
            }
        }

        // Process NFC divisions
        for (const auto& [div1Index, div2Index] : divisionPairs) {
            const auto& division1 = nfcGroups[div1Index];
            const auto& division2 = nfcGroups[div2Index];
            
            const auto& teams1 = division1->Teams();
            const auto& teams2 = division2->Teams();
            
            for (size_t i = 0; i < teams1.size(); i++) {                
                for (size_t j = 0; j < teams2.size(); j++) {
                    bool isHome = (i + j) % 2 == 0;
                    
                    domain::Match match;
                    match.TournamentId() = tournament.Id();
                    match.Round() = domain::RoundType::REGULAR;
                    
                    if (isHome) {
                        match.getHome() = domain::Home{teams1[i].Id, teams1[i].Name};
                        match.getVisitor() = domain::Visitor{teams2[j].Id, teams2[j].Name};
                    } else {
                        match.getHome() = domain::Home{teams2[j].Id, teams2[j].Name};
                        match.getVisitor() = domain::Visitor{teams1[i].Id, teams1[i].Name};
                    }
                    
                    matches.push_back(match);
                }
            }
        }

        // Interconference games

        // Process AFC divisions
        for (const auto& [div1Index, div2Index] : divisionPairs) {
            const auto& division1 = afcGroups[div1Index];
            const auto& division2 = nfcGroups[div2Index];
            
            const auto& teams1 = division1->Teams();
            const auto& teams2 = division2->Teams();
            
            for (size_t i = 0; i < teams1.size(); i++) {                
                for (size_t j = 0; j < teams2.size(); j++) {
                    bool isHome = (i + j) % 2 == 0;
                    
                    domain::Match match;
                    match.TournamentId() = tournament.Id();
                    match.Round() = domain::RoundType::REGULAR;
                    
                    if (isHome) {
                        match.getHome() = domain::Home{teams1[i].Id, teams1[i].Name};
                        match.getVisitor() = domain::Visitor{teams2[j].Id, teams2[j].Name};
                    } else {
                        match.getHome() = domain::Home{teams2[j].Id, teams2[j].Name};
                        match.getVisitor() = domain::Visitor{teams1[i].Id, teams1[i].Name};
                    }
                    
                    matches.push_back(match);
                }
            }
        }

        // Process NFC divisions
        for (const auto& [div1Index, div2Index] : divisionPairs) {
            const auto& division1 = nfcGroups[div1Index];
            const auto& division2 = afcGroups[div2Index];
            
            const auto& teams1 = division1->Teams();
            const auto& teams2 = division2->Teams();
            
            for (size_t i = 0; i < teams1.size(); i++) {                
                for (size_t j = 0; j < teams2.size(); j++) {
                    bool isHome = (i + j) % 2 == 0;
                    
                    domain::Match match;
                    match.TournamentId() = tournament.Id();
                    match.Round() = domain::RoundType::REGULAR;
                    
                    if (isHome) {
                        match.getHome() = domain::Home{teams1[i].Id, teams1[i].Name};
                        match.getVisitor() = domain::Visitor{teams2[j].Id, teams2[j].Name};
                    } else {
                        match.getHome() = domain::Home{teams2[j].Id, teams2[j].Name};
                        match.getVisitor() = domain::Visitor{teams1[i].Id, teams1[i].Name};
                    }
                    
                    matches.push_back(match);
                }
            }
        }

        // AFC: Divisions 0,1 vs Divisions 2,3
        for (int div1 : {0, 1}) {
            for (int div2 : {2, 3}) {
                const auto& division1 = afcGroups[div1];
                const auto& division2 = afcGroups[div2];
                const auto& teams1 = division1->Teams();
                const auto& teams2 = division2->Teams();
                
                for (size_t teamIndex = 0; teamIndex < teams1.size(); teamIndex++) {
                    // Alternate home/away: Div 0 gets home vs Div 2, away vs Div 3
                    //                      Div 1 gets away vs Div 2, home vs Div 3
                    bool div1IsHome = (div1 + div2) % 2 == 0;
                    
                    domain::Match match;
                    match.TournamentId() = tournament.Id();
                    match.Round() = domain::RoundType::REGULAR;
                    
                    if (div1IsHome) {
                        match.getHome() = domain::Home{teams1[teamIndex].Id, teams1[teamIndex].Name};
                        match.getVisitor() = domain::Visitor{teams2[teamIndex].Id, teams2[teamIndex].Name};
                    } else {
                        match.getHome() = domain::Home{teams2[teamIndex].Id, teams2[teamIndex].Name};
                        match.getVisitor() = domain::Visitor{teams1[teamIndex].Id, teams1[teamIndex].Name};
                    }
                    
                    matches.push_back(match);
                }
            }
        }

        // NFC: Same logic
        for (int div1 : {0, 1}) {
            for (int div2 : {2, 3}) {
                const auto& division1 = nfcGroups[div1];
                const auto& division2 = nfcGroups[div2];
                const auto& teams1 = division1->Teams();
                const auto& teams2 = division2->Teams();
                
                for (size_t teamIndex = 0; teamIndex < teams1.size(); teamIndex++) {
                    bool div1IsHome = (div1 + div2) % 2 == 0;
                    
                    domain::Match match;
                    match.TournamentId() = tournament.Id();
                    match.Round() = domain::RoundType::REGULAR;
                    
                    if (div1IsHome) {
                        match.getHome() = domain::Home{teams1[teamIndex].Id, teams1[teamIndex].Name};
                        match.getVisitor() = domain::Visitor{teams2[teamIndex].Id, teams2[teamIndex].Name};
                    } else {
                        match.getHome() = domain::Home{teams2[teamIndex].Id, teams2[teamIndex].Name};
                        match.getVisitor() = domain::Visitor{teams1[teamIndex].Id, teams1[teamIndex].Name};
                    }
                    
                    matches.push_back(match);
                }
            }
        }

        // 17th game: interconference matchup based on division ranking
        // Each AFC division plays against one NFC division they haven't played yet
        std::map<int, int> interconferenceMatchups = {
            {0, 0},  // AFC division 0 vs NFC division 0
            {1, 1},  // AFC division 1 vs NFC division 1
            {2, 2},  // AFC division 2 vs NFC division 2
            {3, 3}   // AFC division 3 vs NFC division 3
        };

        // Process AFC vs NFC
        for (const auto& [afcDivIndex, nfcDivIndex] : interconferenceMatchups) {
            const auto& afcDivision = afcGroups[afcDivIndex];
            const auto& nfcDivision = nfcGroups[nfcDivIndex];
            
            const auto& afcTeams = afcDivision->Teams();
            const auto& nfcTeams = nfcDivision->Teams();
            
            for (size_t teamIndex = 0; teamIndex < afcTeams.size(); teamIndex++) {
                // Alternate home/away: even ranked teams (0,2) get home, odd (1,3) get away
                bool afcIsHome = (teamIndex % 2 == 0);
                
                domain::Match match;
                match.TournamentId() = tournament.Id();
                match.Round() = domain::RoundType::REGULAR;
                
                if (afcIsHome) {
                    match.getHome() = domain::Home{afcTeams[teamIndex].Id, afcTeams[teamIndex].Name};
                    match.getVisitor() = domain::Visitor{nfcTeams[teamIndex].Id, nfcTeams[teamIndex].Name};
                } else {
                    match.getHome() = domain::Home{nfcTeams[teamIndex].Id, nfcTeams[teamIndex].Name};
                    match.getVisitor() = domain::Visitor{afcTeams[teamIndex].Id, afcTeams[teamIndex].Name};
                }
                
                matches.push_back(match);
            }
        }

        return matches;
    }

    std::expected<std::vector<domain::Match>, std::string>
    CreatePlayoffMatches(const domain::Tournament& tournament,
                        const std::vector<std::shared_ptr<domain::Match>>& regularMatches,
                        const std::vector<std::shared_ptr<domain::Group>>& groups) override {
        std::vector<domain::Match> playoffMatches;

        // Separate groups by conference
        std::vector<std::shared_ptr<domain::Group>> afcGroups;
        std::vector<std::shared_ptr<domain::Group>> nfcGroups;

        for (const auto& group : groups) {
            if (group->getConference() == domain::Conference::AFC) {
                afcGroups.push_back(group);
            } else {
                nfcGroups.push_back(group);
            }
        }

        auto afcPlayoffTeams = GetPlayoffTeamsWithNames(afcGroups, regularMatches);
        auto nfcPlayoffTeams = GetPlayoffTeamsWithNames(nfcGroups, regularMatches);

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
            match.Round() = domain::RoundType::DIVISIONAL;
            playoffMatches.push_back(match);
        }

        // Asignar matches a bye teams
        playoffMatches[6].getHome() = domain::Home(afcPlayoffTeams[0].id, afcPlayoffTeams[0].name);
        playoffMatches[8].getHome() = domain::Home(afcPlayoffTeams[1].id, afcPlayoffTeams[1].name);

        // Conference Championships (Semifinals) - 2 matches vacíos
        for (int i = 0; i < 2; i++) {
            domain::Match match;
            match.TournamentId() = tournament.Id();
            match.Round() = domain::RoundType::CHAMPIONSHIP;
            playoffMatches.push_back(match);
        }

        // Super Bowl (Final) - 1 match vacío
        domain::Match finalMatch;
        finalMatch.TournamentId() = tournament.Id();
        finalMatch.Round() = domain::RoundType::SUPERBOWL;
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
        
        // Initialize stats
        for (const auto& group : groups) {
            for (const auto& team : group->Teams()) {
                stats[team.Id] = {team.Id, group->Id(), 0, 0, 0, 0, 0};
            }
        }
        
        // Collect statistics - ONLY for teams in our groups
        for (const auto& match : matches) {
            if (!match->IsPlayed() || match->Round() != domain::RoundType::REGULAR)
                continue;
            
            const auto& score = match->MatchScore().value();
            
            // Check if teams are in our groups before updating stats
            bool homeInGroup = stats.find(match->getHome().id) != stats.end();
            bool visitorInGroup = stats.find(match->getVisitor().id) != stats.end();
            
            // Update home team stats (if in group)
            if (homeInGroup) {
                stats[match->getHome().id].pointsFor += score.homeTeamScore;
                stats[match->getHome().id].pointsAgainst += score.visitorTeamScore;
                
                if (score.homeTeamScore > score.visitorTeamScore) {
                    stats[match->getHome().id].wins++;
                } else if (score.homeTeamScore < score.visitorTeamScore) {
                    stats[match->getHome().id].losses++;
                } else {
                    stats[match->getHome().id].ties++;
                }
            }
            
            // Update visitor team stats (if in group)
            if (visitorInGroup) {
                stats[match->getVisitor().id].pointsFor += score.visitorTeamScore;
                stats[match->getVisitor().id].pointsAgainst += score.homeTeamScore;
                
                if (score.homeTeamScore < score.visitorTeamScore) {
                    stats[match->getVisitor().id].wins++;
                } else if (score.homeTeamScore > score.visitorTeamScore) {
                    stats[match->getVisitor().id].losses++;
                } else {
                    stats[match->getVisitor().id].ties++;
                }
            }
        }
        
        // Sort
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
    struct TeamInfo {
        std::string id;
        std::string name;
    };

    // Versión que retorna TeamInfo con ID y nombre
    std::vector<TeamInfo> GetPlayoffTeamsWithNames(
        const std::vector<std::shared_ptr<domain::Group>>& groups,
        const std::vector<std::shared_ptr<domain::Match>>& matches) {

        std::vector<TeamInfo> playoffTeams;
        std::set<std::string> divisionWinners;

        // Obtener ganador de cada división
        for (const auto& group : groups) {
            auto winner = GetDivisionWinnerWithName(group, matches);
            if (!winner.id.empty()) {
                playoffTeams.push_back(winner);
                divisionWinners.insert(winner.id);
            }
        }

        // Obtener wildcards
        std::vector<std::shared_ptr<domain::Group>> confGroups;
        for (const auto& group : groups) {
            confGroups.push_back(group);
        }

        auto allTeamsRanked = TabulateTeams(matches, confGroups);

        int wildcardsAdded = 0;
        for (const auto& teamId : allTeamsRanked) {
            if (wildcardsAdded >= 3) break;

            if (divisionWinners.find(teamId) == divisionWinners.end()) {
                bool inConference = false;
                std::string teamName;

                for (const auto& group : groups) {
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

    // Obtener ganador con nombre
    TeamInfo GetDivisionWinnerWithName(
        const std::shared_ptr<domain::Group>& group,
        const std::vector<std::shared_ptr<domain::Match>>& matches) {

        std::vector<std::shared_ptr<domain::Match>> groupMatches;
        std::set<std::string> groupTeamIds;

        for (const auto& team : group->Teams()) {
            groupTeamIds.insert(team.Id);
        }

        for (const auto& match : matches) {
            if (groupTeamIds.count(match->getHome().id) ||
                groupTeamIds.count(match->getVisitor().id)) {
                groupMatches.push_back(match);
            }
        }

        std::vector<std::shared_ptr<domain::Group>> singleGroup = {group};
        auto ranked = TabulateTeams(groupMatches, singleGroup);

        if (ranked.empty()) {
            return {"", ""};
        }

        std::println("[NFL Strategy] Group {}, in {} conference standings", group->Name(), ((group->getConference() == domain::Conference::AFC) ? "AFC" : "NFC"));
        for (int i = 0; i < ranked.size(); i++) {

            std::println("[NFL Strategy] {} - id: {}", i + 1, ranked[i]);
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
            domain::Home homeObj{playoffTeams[home].id, playoffTeams[home].name};
            domain::Visitor visitorObj{playoffTeams[away].id, playoffTeams[away].name};
            match.TournamentId() = tournamentId;
            match.getHome() = homeObj;
            match.getVisitor() = visitorObj;
            match.Round() = domain::RoundType::WILDCARD;
            matches.push_back(match);
        }
    }
};

#endif