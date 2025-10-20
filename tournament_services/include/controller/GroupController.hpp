#ifndef A7B3517D_1DC1_4B59_A78C_D3E03D29710C
#define A7B3517D_1DC1_4B59_A78C_D3E03D29710C

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include <vector>
#include <string>
#include <memory>
#include <regex>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "configuration/RouteDefinition.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "domain/Group.hpp"
#include "domain/Utilities.hpp"

static const std::regex ID_VALUE3("[A-Za-z0-9\\-]+");

class GroupController
{
    std::shared_ptr<IGroupDelegate> groupDelegate;
public:
    GroupController(const std::shared_ptr<IGroupDelegate>& delegate);
    ~GroupController();
    crow::response CreateGroup(const crow::request& request, const std::string& tournamentId) const;
    crow::response GetGroups(const std::string& tournamentId) const;
    crow::response GetGroup(const std::string& tournamentId, const std::string& groupId) const;
    crow::response UpdateGroup(const crow::request& request, const std::string& tournamentId, const std::string& groupId) const;
    crow::response DeleteGroup(const std::string& tournamentId, const std::string& groupId) const;
    crow::response UpdateTeams(const crow::request& request, const std::string& tournamentId, const std::string& groupId) const;
};

GroupController::GroupController(const std::shared_ptr<IGroupDelegate>& delegate) : groupDelegate(std::move(delegate)) {}

GroupController::~GroupController()
{
}

crow::response GroupController::CreateGroup(const crow::request& request, const std::string& tournamentId) const {
    if (!std::regex_match(tournamentId, ID_VALUE3)) {
        return {crow::BAD_REQUEST, "Invalid tournament ID format"};
    }

    try {
        const nlohmann::json body = nlohmann::json::parse(request.body);
        const domain::Group group = body;
        
        const auto result = groupDelegate->CreateGroup(tournamentId, group);

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

crow::response GroupController::GetGroups(const std::string& tournamentId) const {
    if (!std::regex_match(tournamentId, ID_VALUE3)) {
        return {crow::BAD_REQUEST, "Invalid tournament ID format"};
    }

    const auto result = groupDelegate->GetGroups(tournamentId);
    
    if (!result) {
        return {crow::INTERNAL_SERVER_ERROR, result.error()};
    }

    const nlohmann::json body = *result;
    crow::response response{crow::OK, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);

    return response;
}

crow::response GroupController::GetGroup(const std::string& tournamentId, const std::string& groupId) const {
    if (!std::regex_match(tournamentId, ID_VALUE3)) {
        return {crow::BAD_REQUEST, "Invalid tournament ID format"};
    }
    if (!std::regex_match(groupId, ID_VALUE3)) {
        return {crow::BAD_REQUEST, "Invalid group ID format"};
    }

    const auto result = groupDelegate->GetGroup(tournamentId, groupId);

    if (!result) {
        if (result.error() == "Group not found") {
            return {crow::NOT_FOUND, result.error()};
        }

        return {crow::INTERNAL_SERVER_ERROR, result.error()};
    }

    const nlohmann::json body = *result;
    crow::response response{crow::OK, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);

    return response;
}

crow::response GroupController::UpdateGroup(const crow::request& request, const std::string& tournamentId, const std::string& groupId) const {
    if (!std::regex_match(tournamentId, ID_VALUE3)) {
        return {crow::BAD_REQUEST, "Invalid tournament ID format"};
    }
    if (!std::regex_match(groupId, ID_VALUE3)) {
        return {crow::BAD_REQUEST, "Invalid group ID format"};
    }

    try {
        const nlohmann::json body = nlohmann::json::parse(request.body);
        domain::Group group = body;
        group.Id() = groupId;
        group.TournamentId() = tournamentId;

        bool updateTeams = body.contains("teams");
        const auto result = groupDelegate->UpdateGroup(tournamentId, group, updateTeams);

        if (!result) {
            if (result.error() == "Group not found") {
                return {crow::NOT_FOUND, result.error()};
            }

            if (result.error().find("already exists") != std::string::npos ||
                result.error() == "Group exceeds maximum teams capacity") {
                return {crow::CONFLICT, result.error()};
            }

            return {crow::INTERNAL_SERVER_ERROR, result.error()};
        }

        return crow::response{crow::NO_CONTENT};
    } catch (const nlohmann::json::exception& e) {
        return {crow::BAD_REQUEST, "Invalid JSON"};
    }
}

crow::response GroupController::DeleteGroup(const std::string& tournamentId, const std::string& groupId) const {
    if (!std::regex_match(tournamentId, ID_VALUE3)) {
        return {crow::BAD_REQUEST, "Invalid tournament ID format"};
    }
    if (!std::regex_match(groupId, ID_VALUE3)) {
        return {crow::BAD_REQUEST, "Invalid group ID format"};
    }

    const auto result = groupDelegate->RemoveGroup(tournamentId, groupId);

    if (!result) {
        if (result.error() == "Group not found") {
            return {crow::NOT_FOUND, result.error()};
        }

        return {crow::INTERNAL_SERVER_ERROR, result.error()};
    }

    return crow::response{crow::NO_CONTENT};
}

crow::response GroupController::UpdateTeams(const crow::request& request, const std::string& tournamentId, const std::string& groupId) const {
    if (!std::regex_match(tournamentId, ID_VALUE3)) {
        return {crow::BAD_REQUEST, "Invalid tournament ID format"};
    }
    if (!std::regex_match(groupId, ID_VALUE3)) {
        return {crow::BAD_REQUEST, "Invalid group ID format"};
    }

    try {
        const std::vector<domain::Team> teams = nlohmann::json::parse(request.body).get<std::vector<domain::Team>>();
        
        const auto result = groupDelegate->UpdateTeams(tournamentId, groupId, teams);
        if (!result) {
            if (result.error() == "Group not found") {
                return {crow::NOT_FOUND, result.error()};
            }

            if (result.error().find("already exists") != std::string::npos) {
                return {crow::CONFLICT, result.error()};
            }

            return {422, result.error()};
        }

        return crow::response{crow::NO_CONTENT};
    } catch (const nlohmann::json::exception& e) {
        return {crow::BAD_REQUEST, "Invalid JSON"};
    }
}

REGISTER_ROUTE(GroupController, CreateGroup, "/tournaments/<string>/groups", "POST"_method)
REGISTER_ROUTE(GroupController, GetGroups, "/tournaments/<string>/groups", "GET"_method)
REGISTER_ROUTE(GroupController, GetGroup, "/tournaments/<string>/groups/<string>", "GET"_method)
REGISTER_ROUTE(GroupController, UpdateGroup, "/tournaments/<string>/groups/<string>", "PATCH"_method)
REGISTER_ROUTE(GroupController, DeleteGroup, "/tournaments/<string>/groups/<string>", "DELETE"_method)
REGISTER_ROUTE(GroupController, UpdateTeams, "/tournaments/<string>/groups/<string>/teams", "PATCH"_method)

#endif /* A7B3517D_1DC1_4B59_A78C_D3E03D29710C */
