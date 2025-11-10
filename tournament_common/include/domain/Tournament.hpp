#ifndef DOMAIN_TOURNAMENT_HPP
#define DOMAIN_TOURNAMENT_HPP

#include <string>
#include <vector>

#include "domain/Group.hpp"
#include "domain/Match.hpp"

namespace domain {
    enum class TournamentType {
        ROUND_ROBIN, NFL
    };

    class TournamentFormat {
        int numberOfGroups;
        int maxTeamsPerGroup;
        int maxGroupsPerConference;
        TournamentType type;
    public:
        TournamentFormat(int numberOfGroups = 8, int maxTeamsPerGroup = 4, int maxGroupsPerConference = 4, TournamentType tournamentType = TournamentType::NFL) {
            this->numberOfGroups = numberOfGroups;
            this->maxTeamsPerGroup = maxTeamsPerGroup;
            this->maxGroupsPerConference = maxGroupsPerConference;
            this->type = tournamentType;
        }

        int NumberOfGroups() const {
            return this->numberOfGroups;
        }
        int & NumberOfGroups() {
            return this->numberOfGroups;
        }

        int MaxTeamsPerGroup() const {
            return this->maxTeamsPerGroup;
        }

        int & MaxTeamsPerGroup() {
            return this->maxTeamsPerGroup;
        }

        int MaxGroupsPerConference() const {
            return this->maxGroupsPerConference;
        }

        int & MaxGroupsPerConference() {
            return this->maxGroupsPerConference;
        }

        TournamentType Type() const {
            return this->type;
        }

        TournamentType & Type() {
            return this->type;
        }
    };

    class Tournament
    {
        std::string id;
        std::string name;
        int year;
        std::string finished;
        TournamentFormat format;
        std::vector<Group> groups;
        std::vector<Match> matches;

    public:
        explicit Tournament(const std::string &name = "", const int &year = 0, const TournamentFormat& format = TournamentFormat()) {
            this->name = name;
            this->year = year;
            this->format = format;
            finished = "no";
        }

        [[nodiscard]] std::string Id() const {
            return this->id;
        }

        std::string& Id() {
            return this->id;
        }

        [[nodiscard]] std::string Name() const {
            return this->name;
        }

        std::string& Name() {
            return this->name;
        }

        [[nodiscard]] int Year() const {
            return this->year;
        }

        int& Year() {
            return this->year;
        }

        [[nodiscard]] std::string Finished() const {
            return this->finished;
        }

        std::string& Finished() {
            return this->finished;
        }

        [[nodiscard]] TournamentFormat Format() const {
            return this->format;
        }

        TournamentFormat & Format () {
            return this->format;
        }

        [[nodiscard]] std::vector<Group> & Groups() {
            return this->groups;
        }

        [[nodiscard]] std::vector<Match> Matches() const {
            return this->matches;
        }
    };
}
#endif