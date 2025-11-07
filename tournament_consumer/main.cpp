//
// Created by tomas on 9/6/25.
//
#include <activemq/library/ActiveMQCPP.h>
#include <thread>

#include "configuration/ContainerSetup.hpp"
#include "cms/GroupAddTeamListener.hpp"
#include "cms/MatchCreationListener.hpp"
#include "cms/ScoreUpdateListener.hpp"

int main() {
    activemq::library::ActiveMQCPP::initializeLibrary();
    {
        std::println("Starting tournament consumer...");
        const auto container = config::containerSetup();
        std::println("Container initialized successfully");

        // Thread para escuchar cuando se agregan equipos a grupos
        // Este listener verificará si el torneo está completo y creará los matches
        std::thread teamAddThread([&] {
            auto listener = container->resolve<GroupAddTeamListener>();
            listener->Start("tournament.team-add");
        });

        std::println("All listeners started. Press Ctrl+C to stop.");

        // Esperar a que todos los threads terminen
        teamAddThread.join();
    }
    activemq::library::ActiveMQCPP::shutdownLibrary();
    return 0;
}