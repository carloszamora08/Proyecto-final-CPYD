//
// Created by tsuny on 8/31/25.
//

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TournamentController.hpp"

#include <string>
#include <utility>
#include <expected>
#include  "domain/Tournament.hpp"
#include "domain/Utilities.hpp"

TournamentController::TournamentController(std::shared_ptr<ITournamentDelegate> delegate) : tournamentDelegate(std::move(delegate)) {}

crow::response TournamentController::CreateTournament(const crow::request &request) const {
    try {
        nlohmann::json body = nlohmann::json::parse(request.body);
        const std::shared_ptr<domain::Tournament> tournament = std::make_shared<domain::Tournament>(body);

        const auto result = tournamentDelegate->CreateTournament(tournament);

        if (!result) {
            return {crow::CONFLICT, result.error()};
        }

        crow::response response{crow::CREATED};
        response.add_header("location", *result);

        return response;
    } catch (const nlohmann::json::exception& e) {
        return {crow::BAD_REQUEST, "Invalid JSON"};
    }
}

crow::response TournamentController::ReadTournament(const std::string& tournamentId) const {
    if(!std::regex_match(tournamentId, ID_VALUE2)) {
        return {crow::BAD_REQUEST, "Invalid ID format"};
    }

    const auto result = tournamentDelegate->GetTournament(tournamentId);

    if (!result) {
        return {crow::NOT_FOUND, result.error()};
    }

    nlohmann::json body = *result;
    crow::response response{crow::OK, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);

    return response;
}

crow::response TournamentController::ReadAll() const {
    const auto result = tournamentDelegate->ReadAll();

    if (!result) {
        return {crow::INTERNAL_SERVER_ERROR, result.error()};
    }

    nlohmann::json body = *result;
    crow::response response{crow::OK, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);

    return response;
}

crow::response TournamentController::UpdateTournament(const crow::request &request, const std::string& tournamentId) const {
    if(!std::regex_match(tournamentId, ID_VALUE2)) {
        return {crow::BAD_REQUEST, "Invalid ID format"};
    }

    try {
        nlohmann::json body = nlohmann::json::parse(request.body);
        const std::shared_ptr<domain::Tournament> tournament = std::make_shared<domain::Tournament>(body);

        const auto result = tournamentDelegate->UpdateTournament(tournamentId, tournament);

        if (!result) {
            if (result.error() == "Tournament not found") {
                return {crow::NOT_FOUND, result.error()};
            }
            
            return {crow::INTERNAL_SERVER_ERROR, result.error()};
        }

        return crow::response{crow::NO_CONTENT};
    } catch (const nlohmann::json::exception& e) {
        return {crow::BAD_REQUEST, "Invalid JSON"};
    }
}

crow::response TournamentController::DeleteTournament(const std::string& tournamentId) const {
    if(!std::regex_match(tournamentId, ID_VALUE2)) {
        return {crow::BAD_REQUEST, "Invalid ID format"};
    }
    
    const auto result = tournamentDelegate->DeleteTournament(tournamentId);
    
    if (!result) {
        if (result.error() == "Tournament not found") {
            return {crow::NOT_FOUND, result.error()};
        }

        return {crow::INTERNAL_SERVER_ERROR, result.error()};
    }

    return crow::response{crow::NO_CONTENT};
}

REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments", "POST"_method)
REGISTER_ROUTE(TournamentController, ReadTournament, "/tournaments/<string>", "GET"_method)
REGISTER_ROUTE(TournamentController, ReadAll, "/tournaments", "GET"_method)
REGISTER_ROUTE(TournamentController, UpdateTournament, "/tournaments/<string>", "PATCH"_method)
REGISTER_ROUTE(TournamentController, DeleteTournament, "/tournaments/<string>", "DELETE"_method)