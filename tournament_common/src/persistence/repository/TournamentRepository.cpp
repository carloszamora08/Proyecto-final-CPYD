//
// Created by tsuny on 9/1/25.
//

#include <iostream>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

#include "persistence/repository/TournamentRepository.hpp"
#include "domain/Utilities.hpp"
#include "persistence/configuration/PostgresConnection.hpp"

TournamentRepository::TournamentRepository(std::shared_ptr<IDbConnectionProvider> connection) : connectionProvider(std::move(connection)) {
}

std::expected<std::string, std::string> TournamentRepository::Create (const domain::Tournament & entity) {
    const nlohmann::json tournamentDoc = entity;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        const pqxx::result result = tx.exec(pqxx::prepped{"insert_tournament"}, tournamentDoc.dump());

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

std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string> TournamentRepository::ReadAll() {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        const pqxx::result result{tx.exec("select id, document from tournaments")};

        for(const auto& row : result){
            nlohmann::json rowTournament = nlohmann::json::parse(row["document"].as<std::string>());
            auto tournament = std::make_shared<domain::Tournament>(rowTournament);
            tournament->Id() = row["id"].as<std::string>();

            tournaments.push_back(tournament);
        }

        tx.commit();
        return tournaments;
    } catch (const pqxx::sql_error &e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        std::cerr << "Query was: " << e.query() << std::endl;

        return std::unexpected(std::format("SQL error: {}", e.what()));
    } catch (const std::exception &e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        
        return std::unexpected(std::format("Database error: {}", e.what()));
    }
}

std::expected<std::shared_ptr<domain::Tournament>, std::string> TournamentRepository::ReadById(std::string id) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        const pqxx::result result = tx.exec(pqxx::prepped{"select_tournament_by_id"}, id);

        if (result.empty()) {
            return std::unexpected("Tournament not found");
        }
        
        nlohmann::json rowTournament = nlohmann::json::parse(result.at(0)["document"].as<std::string>());
        auto tournament = std::make_shared<domain::Tournament>(rowTournament);
        tournament->Id() = result.at(0)["id"].as<std::string>();

        tx.commit();
        return tournament;
    } catch (const pqxx::sql_error &e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        std::cerr << "Query was: " << e.query() << std::endl;
        
        return std::unexpected(std::format("SQL error: {}", e.what()));
    } catch (const std::exception &e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        
        return std::unexpected(std::format("Database error: {}", e.what()));
    }
}

std::expected<std::string, std::string> TournamentRepository::Update(std::string id, const domain::Tournament & entity) {
    const nlohmann::json tournamentDoc = entity;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        const pqxx::result result = tx.exec(pqxx::prepped{"update_tournament_by_id"}, pqxx::params{id, tournamentDoc.dump()});

        if (result.affected_rows() == 0) {
            return std::unexpected("Tournament not found");
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

std::expected<void, std::string> TournamentRepository::Delete(std::string id) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        auto result = tx.exec(pqxx::prepped{"delete_tournament_by_id"}, id);

        if (result.affected_rows() == 0) {
            return std::unexpected("Tournament not found");
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