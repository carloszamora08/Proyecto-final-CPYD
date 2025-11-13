#ifndef LISTENER_SCOREUPDATE_LISTENER_HPP
#define LISTENER_SCOREUPDATE_LISTENER_HPP

#include "QueueMessageListener.hpp"
#include "delegate/MatchDelegate2.hpp"
#include "event/ScoreUpdateEvent.hpp"

class ScoreUpdateListener : public QueueMessageListener {
    std::shared_ptr<MatchDelegate2> matchDelegate2;

public:
    ScoreUpdateListener(const std::shared_ptr<ConnectionManager>& connectionManager,
                        const std::shared_ptr<MatchDelegate2>& matchDelegate2);
    ~ScoreUpdateListener() override;

    void processMessage(const std::string& message) override;
};

inline ScoreUpdateListener::ScoreUpdateListener(
    const std::shared_ptr<ConnectionManager>& connectionManager,
    const std::shared_ptr<MatchDelegate2>& matchDelegate2)
    : QueueMessageListener(connectionManager),
      matchDelegate2(matchDelegate2) {
    std::println("ScoreUpdateListener created with MatchDelegate2");
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

        // Verificar que matchDelegate2 no sea nullptr antes de usarlo
        if (!matchDelegate2) {
            std::println("ERROR: matchDelegate2 is null!");
            return;
        }

        ScoreUpdateEvent scoreUpdateEvent{tournamentId, matchId};
        matchDelegate2->ProcessScoreUpdate(scoreUpdateEvent);

    } catch (const std::exception& e) {
        std::println("Error processing message: {}", e.what());
    }
}

#endif