#ifndef TOURNAMENTS_IMATCHSTRATEGY_HPP
#define TOURNAMENTS_IMATCHSTRATEGY_HPP

#include <vector>
#include <memory>
#include <expected>
#include "domain/Match.hpp"
#include "domain/Group.hpp"
#include "domain/Tournament.hpp"

class IMatchStrategy {
public:
    virtual ~IMatchStrategy() = default;

    // Crear matches de la fase regular
    virtual std::expected<std::vector<domain::Match>, std::string>
        CreateRegularPhaseMatches(const domain::Tournament& tournament,
                                 const std::vector<std::shared_ptr<domain::Group>>& groups) = 0;

    // Crear matches de playoffs (AGREGAR groups como parámetro)
    virtual std::expected<std::vector<domain::Match>, std::string>
        CreatePlayoffMatches(const domain::Tournament& tournament,
                            const std::vector<std::shared_ptr<domain::Match>>& regularMatches,
                            const std::vector<std::shared_ptr<domain::Group>>& groups) = 0;

    // Validar score según las reglas del torneo
    virtual bool ValidateScore(const domain::Score& score, domain::RoundType round) = 0;

    // Procesar el resultado de un match (para doble eliminación principalmente)
    virtual std::expected<std::vector<domain::Match>, std::string>
        ProcessMatchResult(const domain::Match& match,
                          const std::vector<std::shared_ptr<domain::Match>>& allMatches) = 0;

    // Tabular equipos según las reglas del torneo
    virtual std::vector<std::string>
        TabulateTeams(const std::vector<std::shared_ptr<domain::Match>>& matches,
                     const std::vector<std::shared_ptr<domain::Group>>& groups) = 0;
};

#endif // TOURNAMENTS_IMATCHSTRATEGY_HPP