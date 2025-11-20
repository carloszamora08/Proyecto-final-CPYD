from typing import Any

from locust import HttpUser, task
import json
import uuid

class TournamentUser(HttpUser):

    def create_teams(self):
        team_ids = list()
        for i in range(32):
            team_data = {
                "name": f"Team {uuid.uuid4()}"
            }
            with self.client.post(
                    "/teams",
                    json=team_data,
                    catch_response=True,
                    name="POST /teams"
            ) as response:
                if response.status_code == 200 or response.status_code == 201:
                    # Extract group ID from Location header
                    location = response.headers.get("Location") or response.headers.get("location")
                    team_ids.append(location)
                else:
                    response.failure(f"Team creation failed: {response.status_code}")
        return team_ids
    def create_tournament(self):
        tournament_data = {
            "name": f"Tournament - {uuid.uuid4()}",
            "year": 2025,
            "finished": "no"
        }
        with self.client.post(
                "/tournaments",
                json=tournament_data,
                catch_response=True,
                name="POST /tournaments"
        ) as response:
            if response.status_code == 200 or response.status_code == 201:
                # Extract group ID from Location header
                return response.headers.get("Location") or response.headers.get("location")
            else:
                response.failure(f"Tournament creation failed: {response.status_code}")

    def create_group(self, tournament_id: Any | None, conference):
        group_data = {
            "name": f"Group - {uuid.uuid4()}",
            "region": f"Region - {uuid.uuid4()}",
            "conference": f"{conference}"
        }
        with self.client.post(
                f"/tournaments/{tournament_id}/groups",
                json=group_data,
                catch_response=True,
                name=f"POST /tournaments/{tournament_id}/groups"
        ) as response:
            if response.status_code == 200 or response.status_code == 201:
                # Extract group ID from Location header
                return response.headers.get("Location") or response.headers.get("location")
            else:
                response.failure(f"Group creation failed: {response.status_code}")

    def get_tournament(self, tournament_id: Any | None):
        with self.client.get(
                f"/tournaments/{tournament_id}",
                catch_response=True,
                name=f"GET /tournaments/{tournament_id}"
        ) as response:
            if response.status_code == 200 or response.status_code == 201:
                # Tournament finished flag
                return response.json().get("finished")
            else:
                response.failure(f"Tournament selection failed: {response.status_code}")

    def get_pending_matches(self, tournament_id: Any | None):
        with self.client.get(
                f"/tournaments/{tournament_id}/matches?showMatches=pending",
                catch_response=True,
                name=f"GET /tournaments/{tournament_id}/matches?showMatches=pending"
        ) as response:
            if response.status_code == 200 or response.status_code == 201:
                # All pending matches
                return response.json()
            else:
                response.failure(f"Pending match selection failed: {response.status_code}")
                return []

    def update_match_score(self, tournament_id: Any | None, match_id: Any | None):
        score_data = {
            "score": {
                "home": 6,
                "visitor": 7
            }
        }
        with self.client.patch(
                f"/tournaments/{tournament_id}/matches/{match_id}",
                json=score_data,
                catch_response=True,
                name=f"PATCH /tournaments/{tournament_id}/matches/{match_id}"
        ) as response:
            if response.status_code == 204:
                # All pending matches
                return
            else:
                response.failure(f"Score update failed: {response.status_code}")

    @task
    def get_teams(self):
        #self.client.get("/teams")
        team_ids = self.create_teams()
        tournament_id = self.create_tournament()
        group_ids = list()
        group_ids.append(self.create_group(tournament_id, "AFC"))
        group_ids.append(self.create_group(tournament_id, "AFC"))
        group_ids.append(self.create_group(tournament_id, "AFC"))
        group_ids.append(self.create_group(tournament_id, "AFC"))
        group_ids.append(self.create_group(tournament_id, "NFC"))
        group_ids.append(self.create_group(tournament_id, "NFC"))
        group_ids.append(self.create_group(tournament_id, "NFC"))
        group_ids.append(self.create_group(tournament_id, "NFC"))

        groupCounter = 0
        for team_id in team_ids:
            team_data = [{
                "id": f"{team_id}"
            }]
            self.client.patch(
                    f"/tournaments/{tournament_id}/groups/{group_ids[groupCounter % 8]}/teams",
                    json=team_data,
                    catch_response=True,
                    name=f"POST /tournaments/{tournament_id}/groups/{group_ids[groupCounter % 8]}/teams"
            )
            groupCounter += 1
            if (groupCounter == 32):
                groupCounter = 0

        isTournamentDone = False
        while (isTournamentDone == False):
            pendingMatches = self.get_pending_matches(tournament_id)

            for pendingMatch in pendingMatches:
                if (pendingMatch.get("home").get("id") != "" and pendingMatch.get("visitor").get("id") != ""):
                    self.update_match_score(tournament_id, pendingMatch.get("id"))

            isTournamentDone = self.get_tournament(tournament_id) == "yes"
    