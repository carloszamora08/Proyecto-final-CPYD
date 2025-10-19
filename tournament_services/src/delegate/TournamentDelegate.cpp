//
// Created by tomas on 8/31/25.
//

#include <string_view>
#include <memory>

#include "delegate/TournamentDelegate.hpp"

#include "persistence/repository/IRepository.hpp"

TournamentDelegate::TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string, std::expected<std::string, std::string>> > repository, std::shared_ptr<QueueMessageProducer> producer) : tournamentRepository(std::move(repository)), producer(std::move(producer)) {
}

std::expected<std::string, std::string> TournamentDelegate::CreateTournament(std::shared_ptr<domain::Tournament> tournament) {
    const auto result = tournamentRepository->Create(*tournament);

    if (result) {
        producer->SendMessage(*result, "tournament.created");
    }

    return result;
}

std::expected<std::shared_ptr<domain::Tournament>, std::string> TournamentDelegate::GetTournament(std::string_view id) {
    return tournamentRepository->ReadById(id.data());
}

std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string> TournamentDelegate::ReadAll() {
    return tournamentRepository->ReadAll();
}

std::expected<std::string, std::string> TournamentDelegate::UpdateTournament(std::string_view id, std::shared_ptr<domain::Tournament> tournament) {
    const auto result = tournamentRepository->Update(id.data(), *tournament);

    if (result) {
        producer->SendMessage(*result, "tournament.updated");
    }

    return result;
}

std::expected<void, std::string> TournamentDelegate::DeleteTournament(std::string_view id) {
    const auto result = tournamentRepository->Delete(id.data());

    if (result) {
        producer->SendMessage(std::string(id), "tournament.deleted");
    }

    return result;
}