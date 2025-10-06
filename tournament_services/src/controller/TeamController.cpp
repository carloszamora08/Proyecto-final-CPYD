//
// Created by root on 9/27/25.
//

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TeamController.hpp"
#include "domain/Utilities.hpp"


TeamController::TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate) : teamDelegate(teamDelegate) {}

crow::response TeamController::getTeam(const std::string& teamId) const {
    if(!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    if(auto team = teamDelegate->GetTeam(teamId); team != nullptr) {
        nlohmann::json body = team;
        auto response = crow::response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    return crow::response{crow::NOT_FOUND, "team not found"};
}

crow::response TeamController::getAllTeams() const {

    nlohmann::json body = teamDelegate->GetAllTeams();
    crow::response response{200, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return response;
}

crow::response TeamController::UpdateTeam(const crow::request &request, const std::string& teamId) const {
    if(!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    nlohmann::json body = nlohmann::json::parse(request.body);
    const std::shared_ptr<domain::Team> team = std::make_shared<domain::Team>(body);

    const std::string id = teamDelegate->UpdateTeam(teamId, team);
    crow::response response;
    response.code = crow::OK;
    response.add_header("location", id);
    return response;
}

crow::response TeamController::DeleteTeam(const std::string& teamId) const {
    if(!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    if(auto team = teamDelegate->GetTeam(teamId); team != nullptr) {
        teamDelegate->DeleteTeam(teamId);

        return crow::response{crow::OK, "team deleted successfully"};
    }
    return crow::response{crow::NOT_FOUND, "team not found"};
}

crow::response TeamController::SaveTeam(const crow::request& request) const {
    crow::response response;

    if(!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        return response;
    }
    auto requestBody = nlohmann::json::parse(request.body);
    domain::Team team = requestBody;

    auto createdId = teamDelegate->SaveTeam(team);
    response.code = crow::CREATED;
    response.add_header("location", createdId.data());

    return response;
}

REGISTER_ROUTE(TeamController, getTeam, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, UpdateTeam, "/teams/<string>", "PATCH"_method)
REGISTER_ROUTE(TeamController, DeleteTeam, "/teams/<string>", "DELETE"_method)
REGISTER_ROUTE(TeamController, SaveTeam, "/teams", "POST"_method)