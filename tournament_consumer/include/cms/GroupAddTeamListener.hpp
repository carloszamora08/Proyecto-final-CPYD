#ifndef LISTENER_GROUPADDTEAM_LISTENER_HPP
#define LISTENER_GROUPADDTEAM_LISTENER_HPP

#include "QueueMessageListener.hpp"
#include "delegate/MatchDelegate.hpp"
#include "event/TeamAddEvent.hpp"

class GroupAddTeamListener : public QueueMessageListener {
    std::shared_ptr<MatchDelegate> matchDelegate;

    void processMessage(const std::string& message) override;

public:
    GroupAddTeamListener(const std::shared_ptr<ConnectionManager>& connectionManager,
                        const std::shared_ptr<MatchDelegate>& matchDelegate);
    ~GroupAddTeamListener() override;
};

inline GroupAddTeamListener::GroupAddTeamListener(
    const std::shared_ptr<ConnectionManager>& connectionManager,
    const std::shared_ptr<MatchDelegate>& matchDelegate)
    : QueueMessageListener(connectionManager),
      matchDelegate(matchDelegate) {
    std::println("GroupAddTeamListener created with MatchDelegate");
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

        // Verificar que matchDelegate no sea nullptr antes de usarlo
        if (!matchDelegate) {
            std::println("ERROR: matchDelegate is null!");
            return;
        }

        TeamAddEvent teamAddEvent{tournamentId, groupId, teamId};
        matchDelegate->ProcessTeamAddition(teamAddEvent);

    } catch (const std::exception& e) {
        std::println("Error processing message: {}", e.what());
    }
}

#endif //LISTENER_GROUPADDTEAM_LISTENER_HPP