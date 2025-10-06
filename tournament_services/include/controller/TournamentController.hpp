//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_TOURNAMENTCONTROLLER_HPP
#define TOURNAMENTS_TOURNAMENTCONTROLLER_HPP

#include <memory>
#include <crow.h>
#include <regex>

#include "delegate/ITournamentDelegate.hpp"

static const std::regex ID_VALUE2("[A-Za-z0-9\\-]+");

class TournamentController {
    std::shared_ptr<ITournamentDelegate> tournamentDelegate;
public:
    explicit TournamentController(std::shared_ptr<ITournamentDelegate> tournament);
    [[nodiscard]] crow::response CreateTournament(const crow::request &request) const;
    [[nodiscard]] crow::response ReadTournament(const std::string& tournamentId) const;
    [[nodiscard]] crow::response UpdateTournament(const crow::request &request, const std::string& tournamentId) const;
    [[nodiscard]] crow::response DeleteTournament(const std::string& tournamentId) const;
    [[nodiscard]] crow::response ReadAll() const;
};


#endif //TOURNAMENTS_TOURNAMENTCONTROLLER_HPP