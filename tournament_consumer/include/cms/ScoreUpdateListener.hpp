#ifndef LISTENER_SCOREUPDATE_LISTENER_HPP
#define LISTENER_SCOREUPDATE_LISTENER_HPP

#include "QueueMessageListener.hpp"
#include "delegate/MatchDelegate.hpp"
#include "event/ScoreUpdateEvent.hpp"

class ScoreUpdateListener : public QueueMessageListener {
    std::shared_ptr<MatchDelegate> matchDelegate;

    void processMessage(const std::string& message) override;

public:
    ScoreUpdateListener(const std::shared_ptr<ConnectionManager>& connectionManager,
                        const std::shared_ptr<MatchDelegate>& matchDelegate);
    ~ScoreUpdateListener() override;
};

inline ScoreUpdateListener::ScoreUpdateListener(
    const std::shared_ptr<ConnectionManager>& connectionManager,
    const std::shared_ptr<MatchDelegate>& matchDelegate)
    : QueueMessageListener(connectionManager),
      matchDelegate(matchDelegate) {
    std::println("ScoreUpdateListener created with MatchDelegate");
}

inline ScoreUpdateListener::~ScoreUpdateListener() {
    Stop();
}

inline void ScoreUpdateListener::processMessage(const std::string& message) {
    std::println("Score updated: {}", message);
    
    try {
        auto json = nlohmann::json::parse(message);

        std::string tournamentId = json["tournamentId"];
        std::string matchId = json["matchId"];

        std::println("Updating score in match {} in tournament {}",
                     matchId, tournamentId);

        // Verificar que matchDelegate no sea nullptr antes de usarlo
        if (!matchDelegate) {
            std::println("ERROR: matchDelegate is null!");
            return;
        }

        ScoreUpdateEvent scoreUpdateEvent{tournamentId, matchId};
        matchDelegate->ProcessScoreUpdate(scoreUpdateEvent);

    } catch (const std::exception& e) {
        std::println("Error processing message: {}", e.what());
    }
}

#endif