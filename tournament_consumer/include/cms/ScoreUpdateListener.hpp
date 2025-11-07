#ifndef LISTENER_SCOREUPDATE_LISTENER_HPP
#define LISTENER_SCOREUPDATE_LISTENER_HPP

#include "QueueMessageListener.hpp"

class ScoreUpdateListener : public QueueMessageListener {
    void processMessage(const std::string& message) override;

public:
    ScoreUpdateListener(const std::shared_ptr<ConnectionManager>& connectionManager);
    ~ScoreUpdateListener() override;
};

inline ScoreUpdateListener::ScoreUpdateListener(
    const std::shared_ptr<ConnectionManager>& connectionManager)
    : QueueMessageListener(connectionManager) {}

inline ScoreUpdateListener::~ScoreUpdateListener() {
    Stop();
}

inline void ScoreUpdateListener::processMessage(const std::string& message) {
    std::println("Score updated: {}", message);
    // Por ahora solo registra el evento
    // Más adelante aquí podrías crear matches de playoffs
}

#endif