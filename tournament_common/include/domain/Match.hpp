#ifndef DOMAIN_MATCH_HPP
#define DOMAIN_MATCH_HPP

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace domain {
    enum class Winner { HOME, VISITOR };
    enum class RoundType { REGULAR, WILDCARD, DIVISIONAL, CHAMPIONSHIP, SUPERBOWL };

    struct Score {
        int homeTeamScore;
        int visitorTeamScore;

        [[nodiscard]] Winner GetWinner() const {
            if (visitorTeamScore < homeTeamScore) {
                return Winner::HOME;
            }
            return Winner::VISITOR;
        }

        [[nodiscard]] bool IsTie() const {
            return homeTeamScore == visitorTeamScore;
        }
    };

    struct Home {
        std::string id;
        std::string name;
    };

    struct Visitor {
        std::string id;
        std::string name;
    };

    class Match {
        std::string id;
        Home home;
        Visitor visitor;
        std::optional<Score> score;
        RoundType round;
        std::string tournamentId;

        // For tournament bracket tracking
        std::string winnerNextMatchId;

    public:
        Match() = default;

        explicit Match(const std::string& tournamentId, const Home& home, const Visitor& visitor, RoundType roundType = RoundType::REGULAR) {
            this->tournamentId = tournamentId;
            this->home = home;
            this->visitor = visitor;
            this->round = roundType;
        }

        // Getters const
        [[nodiscard]] std::string Id() const { return id; }
        [[nodiscard]] Home getHome() const { return home; }
        [[nodiscard]] Visitor getVisitor() const { return visitor; }
        [[nodiscard]] std::optional<Score> MatchScore() const { return score; }
        [[nodiscard]] RoundType Round() const { return round; }
        [[nodiscard]] std::string TournamentId() const { return tournamentId; }
        [[nodiscard]] std::string WinnerNextMatchId() const { return winnerNextMatchId; }

        // Getters no-const
        std::string& Id() { return id; }
        domain::Home& getHome() { return home; }
        domain::Visitor& getVisitor() { return visitor; }
        std::optional<Score>& MatchScore() { return score; }
        RoundType& Round() { return round; }
        std::string& TournamentId() { return tournamentId; }
        std::string& WinnerNextMatchId() { return winnerNextMatchId; }

        [[nodiscard]] bool IsPlayed() const { return score.has_value(); }
    };

    // ========== SERIALIZATION FUNCTIONS (MUST BE IN domain NAMESPACE) ==========

    // Score serialization
    inline void to_json(nlohmann::json& j, const Score& s) {
        j = nlohmann::json{{"home", s.homeTeamScore}, {"visitor", s.visitorTeamScore}};
    }

    inline void from_json(const nlohmann::json& j, Score& s) {
        j.at("home").get_to(s.homeTeamScore);
        j.at("visitor").get_to(s.visitorTeamScore);
    }

    // Home serialization
    inline void to_json(nlohmann::json& j, const Home& h) {
        j = nlohmann::json{{"id", h.id}, {"name", h.name}};
    }

    inline void from_json(const nlohmann::json& j, Home& h) {
        j.at("id").get_to(h.id);
        j.at("name").get_to(h.name);
    }

    // Visitor serialization
    inline void to_json(nlohmann::json& j, const Visitor& v) {
        j = nlohmann::json{{"id", v.id}, {"name", v.name}};
    }

    inline void from_json(const nlohmann::json& j, Visitor& v) {
        j.at("id").get_to(v.id);
        j.at("name").get_to(v.name);
    }

    // RoundType helper functions
    inline std::string roundTypeToString(RoundType round) {
        switch (round) {
            case RoundType::REGULAR: return "regular";
            case RoundType::WILDCARD: return "wild card";
            case RoundType::DIVISIONAL: return "divisional";
            case RoundType::CHAMPIONSHIP: return "championship";
            case RoundType::SUPERBOWL: return "super bowl";
        }
        return "regular";
    }

    inline RoundType stringToRoundType(const std::string& str) {
        if (str == "wild card") return RoundType::WILDCARD;
        if (str == "divisional") return RoundType::DIVISIONAL;
        if (str == "championship") return RoundType::CHAMPIONSHIP;
        if (str == "super bowl") return RoundType::SUPERBOWL;
        return RoundType::REGULAR;
    }

    // Match serialization
    inline void to_json(nlohmann::json& json, const Match& match) {
        json = {
            {"tournamentId", match.TournamentId()},
            {"home", match.getHome()},
            {"visitor", match.getVisitor()},
            {"round", roundTypeToString(match.Round())}
        };

        if (!match.Id().empty()) {
            json["id"] = match.Id();
        }

        if (match.IsPlayed()) {
            json["score"] = match.MatchScore().value();
        }

        if (!match.WinnerNextMatchId().empty()) {
            json["winnerNextMatchId"] = match.WinnerNextMatchId();
        }
    }

    inline void from_json(const nlohmann::json& json, Match& match) {
        if (json.contains("id")) {
            match.Id() = json["id"].get<std::string>();
        }
        if (json.contains("tournamentId")) {
            match.TournamentId() = json["tournamentId"].get<std::string>();
        }

        if (json.contains("home") && json["home"].is_object()) {
            Home home;
            json["home"].get_to(home);
            match.getHome() = home;
        }
        
        if (json.contains("round")) {
            match.Round() = stringToRoundType(json["round"].get<std::string>());
        }

        if (json.contains("score") && json["score"].is_object()) {
            Score score;
            json["score"].get_to(score);
            match.MatchScore() = score;
        }

        if (json.contains("winnerNextMatchId")) {
            match.WinnerNextMatchId() = json["winnerNextMatchId"].get<std::string>();
        }
    }

    // Serialization for shared_ptr<Match>
    inline void to_json(nlohmann::json& json, const std::shared_ptr<Match>& match) {
        json = {
            {"tournamentId", match->TournamentId()},
            {"home", match->getHome()},
            {"visitor", match->getVisitor()},
            {"round", roundTypeToString(match->Round())}
        };

        if (!match->Id().empty()) {
            json["id"] = match->Id();
        }

        if (match->IsPlayed()) {
            json["score"] = match->MatchScore().value();
        }

        if (!match->WinnerNextMatchId().empty()) {
            json["winnerNextMatchId"] = match->WinnerNextMatchId();
        }
    }

    // Serialization for vector of matches
    inline void to_json(nlohmann::json& json, const std::vector<std::shared_ptr<Match>>& matches) {
        json = nlohmann::json::array();
        for (const auto& match : matches) {
            nlohmann::json jsonMatch;
            to_json(jsonMatch, match);
            json.push_back(jsonMatch);
        }
    }

} // namespace domain

#endif