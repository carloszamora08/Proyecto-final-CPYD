//
// Created by developer on 10/14/25.
//

#ifndef LISTENER_GROUPADDTEAM_LISTENER_HPP
#define LISTENER_GROUPADDTEAM_LISTENER_HPP
#include "QueueMessageListener.hpp"

class GroupAddTeamListener : public QueueMessageListener{
    void processMessage(const std::string& message) override;
public:
    GroupAddTeamListener(const std::shared_ptr<ConnectionManager> &connectionManager);
    ~GroupAddTeamListener() override;

};

inline GroupAddTeamListener::GroupAddTeamListener(const std::shared_ptr<ConnectionManager> &connectionManager)
    : QueueMessageListener(connectionManager) {
}

inline GroupAddTeamListener::~GroupAddTeamListener() {
    Stop();
}

inline void GroupAddTeamListener::processMessage(const std::string &message) {
    std::println("Received message: {}", message);
    
    try {
        auto json = nlohmann::json::parse(message);
        
        std::string tournamentId = json["tournamentId"];
        std::string groupId = json["groupId"];
        std::string teamId = json["teamId"];
        
        std::println("Adding team {} to group {} in tournament {}", 
                     teamId, groupId, tournamentId);
        
    } catch (const std::exception& e) {
        std::println("Error processing message: {}", e.what());
    }
}


#endif //LISTENER_GROUPADDTEAM_LISTENER_HPP