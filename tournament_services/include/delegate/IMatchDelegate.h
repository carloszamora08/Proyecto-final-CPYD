#ifndef TOURNAMENTS_IMATCHDELEGATE_H
#define TOURNAMENTS_IMATCHDELEGATE_H

#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <expected>
#include "domain/Match.hpp"

class IMatchDelegate {
public:
    virtual ~IMatchDelegate() = default;

    // Obtener todos los matches de un torneo con filtros opcionales
    virtual std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
        GetMatches(std::string_view tournamentId,
                   std::optional<std::string> filter = std::nullopt) = 0;

    // Obtener un match específico
    virtual std::expected<std::shared_ptr<domain::Match>, std::string>
        GetMatch(std::string_view tournamentId, std::string_view matchId) = 0;

    // Actualizar el score de un match
    virtual std::expected<void, std::string>
        UpdateMatchScore(std::string_view tournamentId,
                        std::string_view matchId,
                        const domain::Score& score) = 0;

};

#endif // TOURNAMENTS_IMATCHDELEGATE_H