#ifndef DOMAIN_MATCH_HPP
#define DOMAIN_MATCH_HPP

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace domain {
    enum class Winner { HOME, VISITOR };
    enum class RoundType { REGULAR, QUARTERFINALS, SEMIFINALS, FINAL };

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

    class Match {
        std::string id;
        std::string homeTeamId;
        std::string homeTeamName;
        std::string visitorTeamId;
        std::string visitorTeamName;
        std::optional<Score> score;
        RoundType round;
        std::string tournamentId;

        // For tournament bracket tracking
        std::string winnerNextMatchId;
        std::string loserNextMatchId;

    public:
        Match() = default;

        explicit Match(const std::string& tournamentId, const std::string& homeId = "",
                      const std::string& homeName = "", const std::string& visitorId = "",
                      const std::string& visitorName = "", RoundType roundType = RoundType::REGULAR) {
            this->tournamentId = tournamentId;
            this->homeTeamId = homeId;
            this->homeTeamName = homeName;
            this->visitorTeamId = visitorId;
            this->visitorTeamName = visitorName;
            this->round = roundType;
        }

        // Getters const
        [[nodiscard]] std::string Id() const { return id; }
        [[nodiscard]] std::string HomeTeamId() const { return homeTeamId; }
        [[nodiscard]] std::string HomeTeamName() const { return homeTeamName; }
        [[nodiscard]] std::string VisitorTeamId() const { return visitorTeamId; }
        [[nodiscard]] std::string VisitorTeamName() const { return visitorTeamName; }
        [[nodiscard]] std::optional<Score> MatchScore() const { return score; }
        [[nodiscard]] RoundType Round() const { return round; }
        [[nodiscard]] std::string TournamentId() const { return tournamentId; }
        [[nodiscard]] std::string WinnerNextMatchId() const { return winnerNextMatchId; }
        [[nodiscard]] std::string LoserNextMatchId() const { return loserNextMatchId; }

        // Getters no-const
        std::string& Id() { return id; }
        std::string& HomeTeamId() { return homeTeamId; }
        std::string& HomeTeamName() { return homeTeamName; }
        std::string& VisitorTeamId() { return visitorTeamId; }
        std::string& VisitorTeamName() { return visitorTeamName; }
        std::optional<Score>& MatchScore() { return score; }
        RoundType& Round() { return round; }
        std::string& TournamentId() { return tournamentId; }
        std::string& WinnerNextMatchId() { return winnerNextMatchId; }
        std::string& LoserNextMatchId() { return loserNextMatchId; }

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

    // RoundType helper functions
    inline std::string roundTypeToString(RoundType round) {
        switch (round) {
            case RoundType::REGULAR: return "regular";
            case RoundType::QUARTERFINALS: return "quarterfinals";
            case RoundType::SEMIFINALS: return "semifinals";
            case RoundType::FINAL: return "final";
        }
        return "regular";
    }

    inline RoundType stringToRoundType(const std::string& str) {
        if (str == "quarterfinals") return RoundType::QUARTERFINALS;
        if (str == "semifinals") return RoundType::SEMIFINALS;
        if (str == "final") return RoundType::FINAL;
        return RoundType::REGULAR;
    }

    // Match serialization
    inline void to_json(nlohmann::json& json, const Match& match) {
        json = {
            {"tournamentId", match.TournamentId()},
            {"homeTeamId", match.HomeTeamId()},
            {"homeTeamName", match.HomeTeamName()},
            {"visitorTeamId", match.VisitorTeamId()},
            {"visitorTeamName", match.VisitorTeamName()},
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
        if (!match.LoserNextMatchId().empty()) {
            json["loserNextMatchId"] = match.LoserNextMatchId();
        }
    }

    inline void from_json(const nlohmann::json& json, Match& match) {
        if (json.contains("id")) {
            match.Id() = json["id"].get<std::string>();
        }
        if (json.contains("tournamentId")) {
            match.TournamentId() = json["tournamentId"].get<std::string>();
        }
        if (json.contains("homeTeamId")) {
            match.HomeTeamId() = json["homeTeamId"].get<std::string>();
        }
        if (json.contains("homeTeamName")) {
            match.HomeTeamName() = json["homeTeamName"].get<std::string>();
        }
        if (json.contains("visitorTeamId")) {
            match.VisitorTeamId() = json["visitorTeamId"].get<std::string>();
        }
        if (json.contains("visitorTeamName")) {
            match.VisitorTeamName() = json["visitorTeamName"].get<std::string>();
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
        if (json.contains("loserNextMatchId")) {
            match.LoserNextMatchId() = json["loserNextMatchId"].get<std::string>();
        }
    }

    // Serialization for shared_ptr<Match>
    inline void to_json(nlohmann::json& json, const std::shared_ptr<Match>& match) {
        json = {
            {"tournamentId", match->TournamentId()},
            {"homeTeamId", match->HomeTeamId()},
            {"homeTeamName", match->HomeTeamName()},
            {"visitorTeamId", match->VisitorTeamId()},
            {"visitorTeamName", match->VisitorTeamName()},
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
        if (!match->LoserNextMatchId().empty()) {
            json["loserNextMatchId"] = match->LoserNextMatchId();
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