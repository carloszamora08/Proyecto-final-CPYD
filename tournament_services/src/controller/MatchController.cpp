#include "controller/MatchController.hpp"
#include "configuration/RouteDefinition.hpp"
#include "domain/Utilities.hpp"
#include <nlohmann/json.hpp>

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

MatchController::MatchController(std::shared_ptr<IMatchDelegate> delegate) 
    : matchDelegate(std::move(delegate)) {}

crow::response MatchController::GetMatches(const crow::request& request, 
                                           const std::string& tournamentId) const {
    // Obtener el parámetro de query ?showMatches=played|pending
    std::optional<std::string> filter;
    auto showMatches = request.url_params.get("showMatches");
    if (showMatches != nullptr) {
        filter = std::string(showMatches);
    }

    const auto result = matchDelegate->GetMatches(tournamentId, filter);

    if (!result) {
        if (result.error() == "Tournament not found") {
            return {crow::NOT_FOUND, result.error()};
        }
        return {crow::INTERNAL_SERVER_ERROR, result.error()};
    }

    nlohmann::json body = *result;
    crow::response response{crow::OK, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);

    return response;
}

crow::response MatchController::GetMatch(const std::string& tournamentId, 
                                         const std::string& matchId) const {
    const auto result = matchDelegate->GetMatch(tournamentId, matchId);

    if (!result) {
        if (result.error() == "Match not found" || 
            result.error() == "Tournament not found") {
            return {crow::NOT_FOUND, result.error()};
        }
        return {crow::INTERNAL_SERVER_ERROR, result.error()};
    }

    nlohmann::json body = *result;
    crow::response response{crow::OK, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);

    return response;
}

crow::response MatchController::UpdateMatchScore(const crow::request& request,
                                                 const std::string& tournamentId,
                                                 const std::string& matchId) const {
    try {
        nlohmann::json body = nlohmann::json::parse(request.body);
        
        // Parsear el score del body
        if (!body.contains("score")) {
            return {crow::BAD_REQUEST, "Missing 'score' field"};
        }

        domain::Score score;
        body["score"].get_to(score);

        const auto result = matchDelegate->UpdateMatchScore(tournamentId, matchId, score);

        if (!result) {
            if (result.error() == "Match not found" || 
                result.error() == "Tournament not found") {
                return {crow::NOT_FOUND, result.error()};
            }
            
            // Si el error contiene "Invalid score" o "Tie not allowed", es 422
            if (result.error().find("Invalid score") != std::string::npos ||
                result.error().find("Tie not allowed") != std::string::npos) {
                return crow::response(422, result.error());
            }
            
            return {crow::INTERNAL_SERVER_ERROR, result.error()};
        }

        return crow::response{crow::NO_CONTENT};

    } catch (const nlohmann::json::exception& e) {
        return {crow::BAD_REQUEST, "Invalid JSON"};
    }
}

// Registrar las rutas
REGISTER_ROUTE(MatchController, GetMatches, "/tournaments/<string>/matches", "GET"_method)
REGISTER_ROUTE(MatchController, GetMatch, "/tournaments/<string>/matches/<string>", "GET"_method)
REGISTER_ROUTE(MatchController, UpdateMatchScore, "/tournaments/<string>/matches/<string>", "PATCH"_method)