#ifndef TOURNAMENTS_CONSUMER_CONTAINER_SETUP_HPP
#define TOURNAMENTS_CONSUMER_CONTAINER_SETUP_HPP

#include <Hypodermic/Hypodermic.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <memory>
#include <print>

#include "configuration/DatabaseConfiguration.hpp"
#include "cms/ConnectionManager.hpp"
#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/MatchRepository.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/configuration/PostgresConnectionProvider.hpp"
#include "cms/QueueMessageListener.hpp"
#include "cms/GroupAddTeamListener.hpp"
#include "cms/ScoreUpdateListener.hpp"
#include "delegate/MatchDelegate.hpp"

namespace config {
    inline std::shared_ptr<Hypodermic::Container> containerSetup() {
        Hypodermic::ContainerBuilder builder;

        std::ifstream file("configuration.json");
        nlohmann::json configuration;
        file >> configuration;

        // Database connection
        std::shared_ptr<PostgresConnectionProvider> postgressConnection = 
            std::make_shared<PostgresConnectionProvider>(
                configuration["databaseConfig"]["connectionString"].get<std::string>(),
                configuration["databaseConfig"]["poolSize"].get<size_t>());
        builder.registerInstance(postgressConnection).as<IDbConnectionProvider>();

        // Connection Manager (ActiveMQ)
        builder.registerType<ConnectionManager>()
            .onActivated([configuration](Hypodermic::ComponentContext&, 
                                        const std::shared_ptr<ConnectionManager>& instance) {
                instance->initialize(configuration["activemq"]["broker-url"].get<std::string>());
            })
            .singleInstance();

        // Repositories
        builder.registerType<TeamRepository>()
            .as<IRepository<domain::Team, std::string, std::expected<std::string, std::string>>>()
            .singleInstance();

        builder.registerType<TournamentRepository>()
            .as<IRepository<domain::Tournament, std::string, std::expected<std::string, std::string>>>()
            .singleInstance();

        builder.registerType<GroupRepository>()
            .as<IGroupRepository>()
            .singleInstance();

        builder.registerType<MatchRepository>()
            .as<IMatchRepository>()
            .singleInstance();

        // Registrar MatchDelegate
        builder.registerType<MatchDelegate>()
            .singleInstance();

        // Registrar GroupAddTeamListener con sus dependencias
        builder.registerType<GroupAddTeamListener>()
            .singleInstance();

        builder.registerType<ScoreUpdateListener>()
            .singleInstance();

        return builder.build();
    }
}
#endif //TOURNAMENTS_CONSUMER_CONTAINER_SETUP_HPP