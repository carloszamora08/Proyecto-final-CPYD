//
// Created by tsuny on 8/31/25.
//

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TournamentController.hpp"

#include <string>
#include <utility>
#include  "domain/Tournament.hpp"
#include "domain/Utilities.hpp"

TournamentController::TournamentController(std::shared_ptr<ITournamentDelegate> delegate) : tournamentDelegate(std::move(delegate)) {}

crow::response TournamentController::CreateTournament(const crow::request &request) const {
    nlohmann::json body = nlohmann::json::parse(request.body);
    const std::shared_ptr<domain::Tournament> tournament = std::make_shared<domain::Tournament>(body);

    const std::string id = tournamentDelegate->CreateTournament(tournament);
    crow::response response;

    // Check if the tournament was created
    if (id != "") {
        response.code = crow::CREATED;
        response.add_header("location", id);
    } else {
        response.code = crow::CONFLICT;
    }
    
    return response;
}

crow::response TournamentController::ReadTournament(const std::string& tournamentId) const {
    if(!std::regex_match(tournamentId, ID_VALUE2)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    if(auto tournament = tournamentDelegate->GetTournament(tournamentId); tournament != nullptr) {
        nlohmann::json body = tournament;
        auto response = crow::response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    return crow::response{crow::NOT_FOUND, "tournament not found"};
}

crow::response TournamentController::ReadAll() const {
    std::vector<std::shared_ptr<domain::Tournament>> tournamentsRead = tournamentDelegate->ReadAll();

    crow::response response;

    if (tournamentsRead.size() != 0) {
        nlohmann::json body = tournamentsRead;
        response.body = body.dump();
    }

    response.code = crow::OK;  
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);

    return response;
}

crow::response TournamentController::UpdateTournament(const crow::request &request, const std::string& tournamentId) const {
    if(!std::regex_match(tournamentId, ID_VALUE2)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    nlohmann::json body = nlohmann::json::parse(request.body);
    const std::shared_ptr<domain::Tournament> tournament = std::make_shared<domain::Tournament>(body);

    const std::string id = tournamentDelegate->UpdateTournament(tournamentId, tournament);

    crow::response response;

    if (id != "") {
        response.code = crow::NO_CONTENT;
    } else {
        response.code = crow::NOT_FOUND;
    }

    return response;
}

crow::response TournamentController::DeleteTournament(const std::string& tournamentId) const {
    if(!std::regex_match(tournamentId, ID_VALUE2)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    if(auto tournament = tournamentDelegate->GetTournament(tournamentId); tournament != nullptr) {
        tournamentDelegate->DeleteTournament(tournamentId);

        return crow::response{crow::OK, "tournament deleted successfully"};
    }
    return crow::response{crow::NOT_FOUND, "tournament not found"};
}


REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments", "POST"_method)
REGISTER_ROUTE(TournamentController, ReadTournament, "/tournaments/<string>", "GET"_method)
REGISTER_ROUTE(TournamentController, ReadAll, "/tournaments", "GET"_method)
REGISTER_ROUTE(TournamentController, UpdateTournament, "/tournaments/<string>", "PATCH"_method)
REGISTER_ROUTE(TournamentController, DeleteTournament, "/tournaments/<string>", "DELETE"_method)