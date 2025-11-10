#ifndef DOMAIN_GROUP_HPP
#define DOMAIN_GROUP_HPP

#include <string>
#include <vector>

#include "domain/Team.hpp"

namespace domain {
    enum class Conference { AFC, NFC };
    
    class Group {
        /* data */
        std::string id;
        std::string name;
        std::string region;
        Conference conference;
        std::string tournamentId;
        std::vector<Team> teams;

    public:
        explicit Group(const std::string_view & name = "", const std::string_view & region = "", const std::string_view&  id = "", Conference conference = Conference::AFC) : id(id), name(name), region(region), conference(conference) {
        }

        [[nodiscard]] std::string Id() const {
            return  id;
        }

        std::string& Id() {
            return  id;
        }

        [[nodiscard]] std::string Name() const {
            return  name;
        }

        [[nodiscard]] std::string & Name() {
            return  name;
        }

        [[nodiscard]] std::string Region() const {
            return  region;
        }

        [[nodiscard]] std::string & Region() {
            return  region;
        }

        [[nodiscard]] Conference Conference() const {
            return  conference;
        }

        [[nodiscard]] domain::Conference & Conference() {
            return  conference;
        }

        [[nodiscard]] std::string TournamentId() const {
            return  tournamentId;
        }

        [[nodiscard]] std::string & TournamentId() {
            return  tournamentId;
        }

        [[nodiscard]] std::vector<Team> Teams() const {
            return this->teams;
        }

        [[nodiscard]] std::vector<Team> & Teams() {
            return this->teams;
        }
    };
}

#endif