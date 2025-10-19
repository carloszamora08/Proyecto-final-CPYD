//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_TOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_TOURNAMENTDELEGATE_HPP

#include <string>

#include "cms/QueueMessageProducer.hpp"
#include "delegate/ITournamentDelegate.hpp"
#include "persistence/repository/IRepository.hpp"

class TournamentDelegate : public ITournamentDelegate{
    std::shared_ptr<IRepository<domain::Tournament, std::string, std::expected<std::string, std::string>>> tournamentRepository;
    std::shared_ptr<QueueMessageProducer> producer;
public:
    explicit TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string, std::expected<std::string, std::string>>> repository, std::shared_ptr<QueueMessageProducer> producer);

    std::expected<std::string, std::string> CreateTournament(std::shared_ptr<domain::Tournament> tournament) override;
    std::expected<std::shared_ptr<domain::Tournament>, std::string> GetTournament(std::string_view id) override;
    std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string> ReadAll() override;
    std::expected<std::string, std::string> UpdateTournament(std::string_view id, std::shared_ptr<domain::Tournament> tournament) override;
    std::expected<void, std::string> DeleteTournament(std::string_view id) override;
};

#endif //TOURNAMENTS_TOURNAMENTDELEGATE_HPP