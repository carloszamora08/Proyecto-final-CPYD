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
        
        // Create listeners BEFORE starting threads
        auto teamAddListener = container->resolve<GroupAddTeamListener>();
        auto scoreUpdateListener = container->resolve<ScoreUpdateListener>();
        
        // Start threads with proper captures
        std::thread teamAddThread([listener = std::move(teamAddListener)] {
            listener->Start("tournament.team-add");
        });
        
        std::thread matchUpdateThread([listener = std::move(scoreUpdateListener)] {
            listener->Start("match.score-updated");
        });
        
        std::println("All listeners started. Press Ctrl+C to stop.");
        
        teamAddThread.join();
        matchUpdateThread.join();
    }
    activemq::library::ActiveMQCPP::shutdownLibrary();
    return 0;
}