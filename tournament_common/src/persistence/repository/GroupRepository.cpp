//
// Created by root on 9/27/25.
//

#include <iostream>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

#include "domain/Utilities.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/configuration/PostgresConnection.hpp"

GroupRepository::GroupRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider) : connectionProvider(std::move(connectionProvider)) {}

std::expected<std::string, std::string> GroupRepository::Create(const domain::Group& entity) {
    const nlohmann::json groupBody = entity;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        const pqxx::result result = tx.exec(pqxx::prepped{"insert_group"}, pqxx::params{entity.TournamentId(), groupBody.dump()});

        tx.commit();
        return result[0]["id"].as<std::string>();
    } catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        std::cerr << "Query was: " << e.query() << std::endl;

        return std::unexpected(std::format("SQL error: {}", e.what()));
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        
        return std::unexpected(std::format("Database error: {}", e.what()));
    }
}

std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string> GroupRepository::FindByTournamentId(const std::string_view& tournamentId) {
    std::vector<std::shared_ptr<domain::Group>> groups;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        const pqxx::result result = tx.exec(pqxx::prepped{"select_groups_by_tournament"}, pqxx::params{tournamentId.data()});

        for(const auto& row : result) {
            nlohmann::json groupDocument = nlohmann::json::parse(row["document"].as<std::string>());
            auto group = std::make_shared<domain::Group>(groupDocument);
            group->Id() = row["id"].as<std::string>();

            groups.push_back(group);
        }

        tx.commit();
        return groups;
    } catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        std::cerr << "Query was: " << e.query() << std::endl;

        return std::unexpected(std::format("SQL error: {}", e.what()));
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        
        return std::unexpected(std::format("Database error: {}", e.what()));
    }
}

std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string> GroupRepository::ReadAll() {
    return std::unexpected("Not implemented");
}

std::expected<std::shared_ptr<domain::Group>, std::string> GroupRepository::ReadById(std::string id) {
    return std::unexpected("Not implemented");
}

std::expected<std::shared_ptr<domain::Group>, std::string> GroupRepository::FindByTournamentIdAndGroupId(const std::string_view& tournamentId, const std::string_view& groupId) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        const pqxx::result result = tx.exec(pqxx::prepped{"select_group_by_tournamentid_groupid"}, pqxx::params{tournamentId.data(), groupId.data()});

        if (result.empty()) {
            return std::unexpected("Group not found");
        }

        nlohmann::json groupDocument = nlohmann::json::parse(result.at(0)["document"].as<std::string>());
        auto group = std::make_shared<domain::Group>(groupDocument);
        group->Id() = result.at(0)["id"].as<std::string>();

        tx.commit();
        return group;
    } catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        std::cerr << "Query was: " << e.query() << std::endl;
        
        return std::unexpected(std::format("SQL error: {}", e.what()));
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        
        return std::unexpected(std::format("Database error: {}", e.what()));
    }
}

std::expected<std::shared_ptr<domain::Group>, std::string> GroupRepository::FindByTournamentIdAndTeamId(const std::string_view& tournamentId, const std::string_view& teamId) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        const pqxx::result result = tx.exec(pqxx::prepped{"select_group_in_tournament"}, pqxx::params{tournamentId.data(), teamId.data()});

        if (result.empty()) {
            return std::unexpected("Group not found");
        }

        nlohmann::json groupDocument = nlohmann::json::parse(result.at(0)["document"].as<std::string>());
        auto group = std::make_shared<domain::Group>(groupDocument);
        group->Id() = result.at(0)["id"].as<std::string>();

        tx.commit();
        return group;
    } catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        std::cerr << "Query was: " << e.query() << std::endl;
        
        return std::unexpected(std::format("SQL error: {}", e.what()));
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        
        return std::unexpected(std::format("Database error: {}", e.what()));
    }
}

std::expected<std::string, std::string> GroupRepository::Update(std::string id, const domain::Group& entity) {
    const nlohmann::json groupBody = entity;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        const pqxx::result result = tx.exec(pqxx::prepped{"update_group_by_id"}, pqxx::params{id, groupBody.dump()});

        if (result.affected_rows() == 0) {
            return std::unexpected("Group not found");
        }

        tx.commit();
        return result[0]["id"].as<std::string>();
    } catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        std::cerr << "Query was: " << e.query() << std::endl;
        
        return std::unexpected(std::format("SQL error: {}", e.what()));
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        
        return std::unexpected(std::format("Database error: {}", e.what()));
    }
}

std::expected<void, std::string> GroupRepository::Delete(std::string id) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        const pqxx::result result = tx.exec(pqxx::prepped{"delete_group_by_id"}, id);

        if (result.affected_rows() == 0) {
            return std::unexpected("Group not found");
        }

        tx.commit();
        return {};
    } catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        std::cerr << "Query was: " << e.query() << std::endl;

        return std::unexpected(std::format("SQL error: {}", e.what()));
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;

        return std::unexpected(std::format("Database error: {}", e.what()));
    }
}

std::expected<void, std::string> GroupRepository::UpdateGroupAddTeam(const std::string_view& groupId, const std::shared_ptr<domain::Team>& team) {
    const nlohmann::json teamDocument = team;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    try {
        const pqxx::result result = tx.exec(pqxx::prepped{"update_group_add_team"}, pqxx::params{groupId.data(), teamDocument.dump()});

        if (result.affected_rows() == 0) {
            return std::unexpected("Group not found");
        }

        tx.commit();
        return {};
    } catch (const pqxx::sql_error& e) {
        std::cerr << "SQL error: " << e.what() << std::endl;
        std::cerr << "Query was: " << e.query() << std::endl;

        return std::unexpected(std::format("SQL error: {}", e.what()));
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;

        return std::unexpected(std::format("Database error: {}", e.what()));
    }
}