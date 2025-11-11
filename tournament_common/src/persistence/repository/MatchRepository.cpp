#include "persistence/repository/MatchRepository.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

MatchRepository::MatchRepository(std::shared_ptr<IDbConnectionProvider> connection)
    : connectionProvider(std::move(connection)) {}

std::expected<std::string, std::string> MatchRepository::Create(const domain::Match& entity) {
    nlohmann::json matchDoc;
    matchDoc["tournamentId"] = entity.TournamentId();
    matchDoc["home"] = entity.getHome();
    matchDoc["visitor"] = entity.getVisitor();
    matchDoc["round"] = static_cast<int>(entity.Round());
    
    if (entity.IsPlayed()) {
        const auto& score = entity.MatchScore().value();
        matchDoc["score"]["home"] = score.homeTeamScore;
        matchDoc["score"]["visitor"] = score.visitorTeamScore;
    }
    
    if (!entity.WinnerNextMatchId().empty()) {
        matchDoc["winnerNextMatchId"] = entity.WinnerNextMatchId();
    }

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        const pqxx::result result = tx.exec(
            pqxx::prepped{"insert_match"}, matchDoc.dump());

        tx.commit();
        return result[0]["id"].as<std::string>();
    } catch (const pqxx::sql_error &e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        std::cerr << "Query was: " << e.query() << std::endl;
        return std::unexpected(std::format("SQL error: {}", e.what()));
    } catch (const std::exception &e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return std::unexpected(std::format("Database error: {}", e.what()));
    }
}

std::expected<std::shared_ptr<domain::Match>, std::string> 
MatchRepository::ReadById(const std::string& id) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        const pqxx::result result = tx.exec(pqxx::prepped{"select_match_by_id"}, id);

        if (result.empty()) {
            return std::unexpected("Match not found");
        }
        
        nlohmann::json matchJson = nlohmann::json::parse(
            result.at(0)["document"].as<std::string>());
        
        auto match = std::make_shared<domain::Match>();
        match->Id() = result.at(0)["id"].as<std::string>();
        match->TournamentId() = matchJson["tournamentId"];
        match->getHome() = matchJson["home"];
        match->getVisitor() = matchJson["visitor"];
        match->Round() = static_cast<domain::RoundType>(matchJson["round"].get<int>());
        
        if (matchJson.contains("score")) {
            domain::Score score;
            score.homeTeamScore = matchJson["score"]["home"];
            score.visitorTeamScore = matchJson["score"]["visitor"];
            match->MatchScore() = score;
        }
        
        if (matchJson.contains("winnerNextMatchId")) {
            match->WinnerNextMatchId() = matchJson["winnerNextMatchId"];
        }

        tx.commit();
        return match;
    } catch (const pqxx::sql_error &e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        std::cerr << "Query was: " << e.query() << std::endl;
        return std::unexpected(std::format("SQL error: {}", e.what()));
    } catch (const std::exception &e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return std::unexpected(std::format("Database error: {}", e.what()));
    }
}

std::expected<std::string, std::string> 
MatchRepository::Update(const std::string& id, const domain::Match& entity) {
    nlohmann::json matchDoc;
    matchDoc["tournamentId"] = entity.TournamentId();
    matchDoc["home"] = entity.getHome();
    matchDoc["visitor"] = entity.getVisitor();
    matchDoc["round"] = static_cast<int>(entity.Round());
    
    if (entity.IsPlayed()) {
        const auto& score = entity.MatchScore().value();
        matchDoc["score"]["home"] = score.homeTeamScore;
        matchDoc["score"]["visitor"] = score.visitorTeamScore;
    }
    
    if (!entity.WinnerNextMatchId().empty()) {
        matchDoc["winnerNextMatchId"] = entity.WinnerNextMatchId();
    }

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        const pqxx::result result = tx.exec(
            pqxx::prepped{"update_match_by_id"}, 
            pqxx::params{id, matchDoc.dump()});

        if (result.affected_rows() == 0) {
            return std::unexpected("Match not found");
        }

        tx.commit();
        return result[0]["id"].as<std::string>();
    } catch (const pqxx::sql_error &e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        std::cerr << "Query was: " << e.query() << std::endl;
        return std::unexpected(std::format("SQL error: {}", e.what()));
    } catch (const std::exception &e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return std::unexpected(std::format("Database error: {}", e.what()));
    }
}

std::expected<void, std::string> MatchRepository::Delete(const std::string& id) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        auto result = tx.exec("DELETE FROM MATCHES WHERE id = " + tx.quote(id));

        if (result.affected_rows() == 0) {
            return std::unexpected("Match not found");
        }

        tx.commit();
        return {};
    } catch (const pqxx::sql_error &e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        std::cerr << "Query was: " << e.query() << std::endl;
        return std::unexpected(std::format("SQL error: {}", e.what()));
    } catch (const std::exception &e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return std::unexpected(std::format("Database error: {}", e.what()));
    }
}

std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string> 
MatchRepository::FindByTournamentId(const std::string_view& tournamentId) {
    std::vector<std::shared_ptr<domain::Match>> matches;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        const pqxx::result result = tx.exec(
            pqxx::prepped{"select_matches_by_tournament"}, tournamentId.data());

        for(const auto& row : result) {
            nlohmann::json matchJson = nlohmann::json::parse(
                row["document"].as<std::string>());
            
            auto match = std::make_shared<domain::Match>();
            match->Id() = row["id"].as<std::string>();
            match->TournamentId() = matchJson["tournamentId"];
            match->getHome() = matchJson["home"];
            match->getVisitor() = matchJson["visitor"];
            match->Round() = static_cast<domain::RoundType>(matchJson["round"].get<int>());
            
            if (matchJson.contains("score")) {
                domain::Score score;
                score.homeTeamScore = matchJson["score"]["home"];
                score.visitorTeamScore = matchJson["score"]["visitor"];
                match->MatchScore() = score;
            }
            
            if (matchJson.contains("winnerNextMatchId")) {
                match->WinnerNextMatchId() = matchJson["winnerNextMatchId"];
            }

            matches.push_back(match);
        }

        tx.commit();
        return matches;
    } catch (const pqxx::sql_error &e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        std::cerr << "Query was: " << e.query() << std::endl;
        return std::unexpected(std::format("SQL error: {}", e.what()));
    } catch (const std::exception &e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return std::unexpected(std::format("Database error: {}", e.what()));
    }
}

std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string> 
MatchRepository::FindByTournamentIdAndRound(const std::string_view& tournamentId, 
                                           domain::RoundType round) {
    auto allMatches = FindByTournamentId(tournamentId);
    if (!allMatches) {
        return allMatches;
    }
    
    std::vector<std::shared_ptr<domain::Match>> filtered;
    for (const auto& match : *allMatches) {
        if (match->Round() == round) {
            filtered.push_back(match);
        }
    }
    
    return filtered;
}

std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string> 
MatchRepository::FindPlayedMatchesByTournamentId(const std::string_view& tournamentId) {
    std::vector<std::shared_ptr<domain::Match>> matches;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        const pqxx::result result = tx.exec(
            pqxx::prepped{"select_played_matches_by_tournament"}, tournamentId.data());

        for(const auto& row : result) {
            nlohmann::json matchJson = nlohmann::json::parse(
                row["document"].as<std::string>());
            
            auto match = std::make_shared<domain::Match>();
            match->Id() = row["id"].as<std::string>();
            match->TournamentId() = matchJson["tournamentId"];
            match->getHome() = matchJson["home"];
            match->getVisitor() = matchJson["visitor"];
            match->Round() = static_cast<domain::RoundType>(matchJson["round"].get<int>());
            
            domain::Score score;
            score.homeTeamScore = matchJson["score"]["home"];
            score.visitorTeamScore = matchJson["score"]["visitor"];
            match->MatchScore() = score;
            
            if (matchJson.contains("winnerNextMatchId")) {
                match->WinnerNextMatchId() = matchJson["winnerNextMatchId"];
            }

            matches.push_back(match);
        }

        tx.commit();
        return matches;
    } catch (const pqxx::sql_error &e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        return std::unexpected(std::format("SQL error: {}", e.what()));
    } catch (const std::exception &e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return std::unexpected(std::format("Database error: {}", e.what()));
    }
}

std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string> 
MatchRepository::FindPendingMatchesByTournamentId(const std::string_view& tournamentId) {
    std::vector<std::shared_ptr<domain::Match>> matches;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        const pqxx::result result = tx.exec(
            pqxx::prepped{"select_pending_matches_by_tournament"}, tournamentId.data());

        for(const auto& row : result) {
            nlohmann::json matchJson = nlohmann::json::parse(
                row["document"].as<std::string>());
            
            auto match = std::make_shared<domain::Match>();
            match->Id() = row["id"].as<std::string>();
            match->TournamentId() = matchJson["tournamentId"];
            match->getHome() = matchJson["home"];
            match->getVisitor() = matchJson["visitor"];
            match->Round() = static_cast<domain::RoundType>(matchJson["round"].get<int>());
            
            if (matchJson.contains("winnerNextMatchId")) {
                match->WinnerNextMatchId() = matchJson["winnerNextMatchId"];
            }

            matches.push_back(match);
        }

        tx.commit();
        return matches;
    } catch (const pqxx::sql_error &e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        return std::unexpected(std::format("SQL error: {}", e.what()));
    } catch (const std::exception &e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return std::unexpected(std::format("Database error: {}", e.what()));
    }
}

std::expected<std::shared_ptr<domain::Match>, std::string> 
MatchRepository::FindLastOpenMatch(const std::string_view& tournamentId) {
    // Esta función es específica para doble eliminación
    // Por ahora retornamos un error ya que NFL no la necesita
    return std::unexpected("FindLastOpenMatch not implemented for this tournament type");
}