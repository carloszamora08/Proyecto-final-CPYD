//
// Created by root on 9/27/25.
//

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TeamController.hpp"
#include "domain/Utilities.hpp"

TeamController::TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate)
    : teamDelegate(teamDelegate) {}

crow::response TeamController::getTeam(const std::string& teamId) const {
    if(!std::regex_match(teamId, ID_VALUE)) {
        return {crow::BAD_REQUEST, "Invalid ID format"};
    }

    const auto result = teamDelegate->GetTeam(teamId);

    if (!result) {
        return {crow::NOT_FOUND, result.error()};
    }

    nlohmann::json body = *result;
    crow::response response{crow::OK, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);

    return response;
}

crow::response TeamController::getAllTeams() const {
    const auto result = teamDelegate->ReadAll();

    if (!result) {
        return {crow::INTERNAL_SERVER_ERROR, result.error()};
    }

    nlohmann::json body = *result;
    crow::response response{crow::OK, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);

    return response;
}

crow::response TeamController::UpdateTeam(const crow::request &request, const std::string& teamId) const {
    if(!std::regex_match(teamId, ID_VALUE)) {
        return {crow::BAD_REQUEST, "Invalid ID format"};
    }

    try {
        nlohmann::json body = nlohmann::json::parse(request.body);
        const std::shared_ptr<domain::Team> team = std::make_shared<domain::Team>(body);

        const auto result = teamDelegate->UpdateTeam(teamId, team);

        if (!result) {
            if (result.error() == "Team not found") {
                return {crow::NOT_FOUND, result.error()};
            }

            return {crow::INTERNAL_SERVER_ERROR, result.error()};
        }

        return crow::response{crow::NO_CONTENT};
    } catch (const nlohmann::json::exception& e) {
        return {crow::BAD_REQUEST, "Invalid JSON"};
    }
}

crow::response TeamController::DeleteTeam(const std::string& teamId) const {
    if(!std::regex_match(teamId, ID_VALUE)) {
        return {crow::BAD_REQUEST, "Invalid ID format"};
    }

    const auto result = teamDelegate->DeleteTeam(teamId);

    if (!result) {
        if (result.error() == "Team not found") {
            return {crow::NOT_FOUND, result.error()};
        }

        return {crow::INTERNAL_SERVER_ERROR, result.error()};
    }

    return crow::response{crow::NO_CONTENT};
}

crow::response TeamController::SaveTeam(const crow::request& request) const {
    try {
        nlohmann::json body = nlohmann::json::parse(request.body);
        const std::shared_ptr<domain::Team> team = std::make_shared<domain::Team>(body);

        const auto result = teamDelegate->CreateTeam(team);

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

REGISTER_ROUTE(TeamController, getTeam, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, UpdateTeam, "/teams/<string>", "PATCH"_method)
REGISTER_ROUTE(TeamController, DeleteTeam, "/teams/<string>", "DELETE"_method)
REGISTER_ROUTE(TeamController, SaveTeam, "/teams", "POST"_method)