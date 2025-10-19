//
// Created by tomas on 8/24/25.
//

#ifndef RESTAPI_TEAMREPOSITORY_HPP
#define RESTAPI_TEAMREPOSITORY_HPP
#include <string>
#include <memory>
#include <iostream>
#include <format>
#include <nlohmann/json.hpp>


#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "IRepository.hpp"
#include "domain/Team.hpp"
#include "domain/Utilities.hpp"


class TeamRepository : public IRepository<domain::Team, std::string, std::expected<std::string, std::string>> {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;
public:

    explicit TeamRepository(std::shared_ptr<IDbConnectionProvider> connectionProvider) : connectionProvider(std::move(connectionProvider)){}

    std::expected<std::string, std::string> Create(const domain::Team &entity) override {
        const nlohmann::json teamBody = entity;

        auto pooled = connectionProvider->Connection();
        const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
        pqxx::work tx(*(connection->connection));

        try {
            const pqxx::result result = tx.exec(pqxx::prepped{"insert_team"}, teamBody.dump());

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

    std::expected<std::vector<std::shared_ptr<domain::Team>>, std::string> ReadAll() override {
        std::vector<std::shared_ptr<domain::Team>> teams;

        auto pooled = connectionProvider->Connection();
        const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
        pqxx::work tx(*(connection->connection));

        try {
            const pqxx::result result{tx.exec("select id, document->>'name' as name from teams")};

            for(const auto& row : result){
                teams.push_back(std::make_shared<domain::Team>(
                    domain::Team{row["id"].as<std::string>(), row["name"].as<std::string>()}
                ));
            }

            tx.commit();
            return teams;
        } catch (const pqxx::sql_error &e) {
            std::cerr << "SQL error: " << e.what() << std::endl;
            std::cerr << "Query was: " << e.query() << std::endl;

            return std::unexpected(std::format("SQL error: {}", e.what()));
        } catch (const std::exception &e) {
            std::cerr << "Unexpected error: " << e.what() << std::endl;

            return std::unexpected(std::format("Database error: {}", e.what()));
        }
    }

    std::expected<std::shared_ptr<domain::Team>, std::string> ReadById(std::string id) override {
        auto pooled = connectionProvider->Connection();
        const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
        pqxx::work tx(*(connection->connection));

        try {
            const pqxx::result result = tx.exec(pqxx::prepped{"select_team_by_id"}, id);

            if (result.empty()) {
                return std::unexpected("Team not found");
            }

            auto team = std::make_shared<domain::Team>(
                nlohmann::json::parse(result.at(0)["document"].as<std::string>())
            );
            team->Id = result.at(0)["id"].as<std::string>();

            tx.commit();
            return team;
        } catch (const pqxx::sql_error &e) {
            std::cerr << "SQL error: " << e.what() << std::endl;
            std::cerr << "Query was: " << e.query() << std::endl;

            return std::unexpected(std::format("SQL error: {}", e.what()));
        } catch (const std::exception &e) {
            std::cerr << "Unexpected error: " << e.what() << std::endl;

            return std::unexpected(std::format("Database error: {}", e.what()));
        }
    }

    std::expected<std::string, std::string> Update(std::string id, const domain::Team & entity) override {
        const nlohmann::json teamDoc = entity;

        auto pooled = connectionProvider->Connection();
        const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
        pqxx::work tx(*(connection->connection));

        try {
            const pqxx::result result = tx.exec(pqxx::prepped{"update_team_by_id"}, pqxx::params{id, teamDoc.dump()});

            if (result.affected_rows() == 0) {
                return std::unexpected("Team not found");
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

    std::expected<void, std::string> Delete(std::string id) override{
        auto pooled = connectionProvider->Connection();
        const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
        pqxx::work tx(*(connection->connection));

        try {
            auto result = tx.exec(pqxx::prepped{"delete_team_by_id"}, id);

            if (result.affected_rows() == 0) {
                return std::unexpected("Team not found");
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
};


#endif //RESTAPI_TEAMREPOSITORY_HPP