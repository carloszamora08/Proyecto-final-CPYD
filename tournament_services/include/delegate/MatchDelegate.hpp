#ifndef TOURNAMENTS_MATCHDELEGATE_HPP
#define TOURNAMENTS_MATCHDELEGATE_HPP

#include <memory>
#include <map>
#include "IMatchDelegate.h"
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "cms/QueueMessageProducer.hpp"
#include "domain/IMatchStrategy.hpp"

class MatchDelegate : public IMatchDelegate {
    std::shared_ptr<IMatchRepository> matchRepository;
    std::shared_ptr<TournamentRepository> tournamentRepository;
    std::shared_ptr<IGroupRepository> groupRepository;
    std::shared_ptr<QueueMessageProducer> messageProducer;

    // Estrategias por tipo de torneo
    std::map<std::string, std::shared_ptr<IMatchStrategy>> strategies;

public:
    MatchDelegate(const std::shared_ptr<IMatchRepository>& matchRepo,
                  const std::shared_ptr<TournamentRepository>& tournamentRepo,
                  const std::shared_ptr<IGroupRepository>& groupRepo,
                  const std::shared_ptr<QueueMessageProducer>& producer);

    std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
        GetMatches(std::string_view tournamentId,
                   std::optional<std::string> filter = std::nullopt) override;

    std::expected<std::shared_ptr<domain::Match>, std::string>
        GetMatch(std::string_view tournamentId, std::string_view matchId) override;

    std::expected<void, std::string>
        UpdateMatchScore(std::string_view tournamentId,
                        std::string_view matchId,
                        const domain::Score& score) override;

private:
    // Validaciones
    bool ValidateScore(const domain::Score& score,
                      const domain::Tournament& tournament,
                      domain::RoundType round);

    // GetStrategy ahora recibe TournamentType
    std::shared_ptr<IMatchStrategy> GetStrategy(const domain::TournamentType& tournamentType);
};

#endif // TOURNAMENTS_MATCHDELEGATE_HPP