#ifndef LISTENER_GROUPADDTEAM_LISTENER_HPP
#define LISTENER_GROUPADDTEAM_LISTENER_HPP

#include "QueueMessageListener.hpp"
#include "delegate/MatchDelegate2.hpp"
#include "event/TeamAddEvent.hpp"

class GroupAddTeamListener : public QueueMessageListener {
    std::shared_ptr<MatchDelegate2> matchDelegate2;

public:
    GroupAddTeamListener(const std::shared_ptr<ConnectionManager>& connectionManager,
                        const std::shared_ptr<MatchDelegate2>& matchDelegate2);
    ~GroupAddTeamListener() override;

    void processMessage(const std::string& message) override;
};

inline GroupAddTeamListener::GroupAddTeamListener(
    const std::shared_ptr<ConnectionManager>& connectionManager,
    const std::shared_ptr<MatchDelegate2>& matchDelegate2)
    : QueueMessageListener(connectionManager),
      matchDelegate2(matchDelegate2) {
    std::println("GroupAddTeamListener created with MatchDelegate2");
}

inline GroupAddTeamListener::~GroupAddTeamListener() {
    Stop();
}

inline void GroupAddTeamListener::processMessage(const std::string& message) {
    std::println("Received message: {}", message);

    try {
        auto json = nlohmann::json::parse(message);

        std::string tournamentId = json["tournamentId"];
        std::string groupId = json["groupId"];
        std::string teamId = json["teamId"];

        std::println("Adding team {} to group {} in tournament {}",
                     teamId, groupId, tournamentId);

        // Verificar que matchDelegate2 no sea nullptr antes de usarlo
        if (!matchDelegate2) {
            std::println("ERROR: matchDelegate2 is null!");
            return;
        }

        TeamAddEvent teamAddEvent{tournamentId, groupId, teamId};
        matchDelegate2->ProcessTeamAddition(teamAddEvent);

    } catch (const std::exception& e) {
        std::println("Error processing message: {}", e.what());
    }
}

#endif //LISTENER_GROUPADDTEAM_LISTENER_HPP