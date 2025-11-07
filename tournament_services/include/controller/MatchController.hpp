#ifndef TOURNAMENTS_MATCHCONTROLLER_HPP
#define TOURNAMENTS_MATCHCONTROLLER_HPP

#include <memory>
#include <string>
#include <crow.h>
#include "delegate/IMatchDelegate.h"

class MatchController {
    std::shared_ptr<IMatchDelegate> matchDelegate;

public:
    explicit MatchController(std::shared_ptr<IMatchDelegate> delegate);

    // GET /tournaments/<tournamentId>/matches
    crow::response GetMatches(const crow::request& request, const std::string& tournamentId) const;

    // GET /tournaments/<tournamentId>/matches/<matchId>
    crow::response GetMatch(const std::string& tournamentId, const std::string& matchId) const;

    // PATCH /tournaments/<tournamentId>/matches/<matchId>
    crow::response UpdateMatchScore(const crow::request& request,
                                    const std::string& tournamentId,
                                    const std::string& matchId) const;
};

#endif // TOURNAMENTS_MATCHCONTROLLER_HPP