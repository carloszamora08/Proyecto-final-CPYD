//
// Created by tomas on 8/24/25.
//

#ifndef RESTAPI_TEAMREPOSITORY_HPP
#define RESTAPI_TEAMREPOSITORY_HPP
#include <string>
#include <memory>
#include <nlohmann/json.hpp>


#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "IRepository.hpp"
#include "domain/Team.hpp"
#include "domain/Utilities.hpp"


class TeamRepository : public IRepository<domain::Team, std::string_view> {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;
public:

    explicit TeamRepository(std::shared_ptr<IDbConnectionProvider> connectionProvider) : connectionProvider(std::move(connectionProvider)){}

    std::vector<std::shared_ptr<domain::Team>> ReadAll() override {
        std::vector<std::shared_ptr<domain::Team>> teams;

        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
        
        pqxx::work tx(*(connection->connection));
        pqxx::result result{tx.exec("select id, document->>'name' as name from teams")};
        tx.commit();

        for(auto row : result){
            teams.push_back(std::make_shared<domain::Team>(domain::Team{row["id"].c_str(), row["name"].c_str()}));
        }

        return teams;
    }

    std::shared_ptr<domain::Team> ReadById(std::string_view id) override {
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

        pqxx::work tx(*(connection->connection));
        pqxx::result result = tx.exec(pqxx::prepped{"select_team_by_id"}, id.data());
        tx.commit();
        auto team = std::make_shared<domain::Team>( nlohmann::json::parse(result[0]["document"].c_str()));
        team->Id = result[0]["id"].c_str();

        return team;
    }

    std::string_view Create(const domain::Team &entity) override {
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
        nlohmann::json teamBody = entity;

        pqxx::work tx(*(connection->connection));
        pqxx::result result = tx.exec(pqxx::prepped{"insert_team"}, teamBody.dump());

        tx.commit();

        return result[0]["id"].c_str();
    }

    std::string_view Update(std::string_view id, const domain::Team & entity) override {
        const nlohmann::json teamDoc = entity;

        auto pooled = connectionProvider->Connection();
        const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

        pqxx::work tx(*(connection->connection));
        const pqxx::result result = tx.exec_prepared("update_team_by_id", id, teamDoc.dump());

        tx.commit();

        return result[0]["id"].c_str();
    }


    void Delete(std::string_view id) override{
        auto pooled = connectionProvider->Connection();
        const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
        
        pqxx::work tx(*(connection->connection));
        tx.exec(pqxx::prepped{"delete_team_by_id"}, std::string{id});
        tx.commit();
    }
};


#endif //RESTAPI_TEAMREPOSITORY_HPP