import axios from 'axios';
import type { Team, Tournament, Group, Match, Score } from '../types';

const API_BASE = '/api'; // Usa el proxy de Vite

const api = axios.create({
    baseURL: API_BASE,
    headers: {
        'Content-Type': 'application/json',
    },
});

// Teams
export const teamsApi = {
    getAll: () => api.get<Team[]>('/teams'),
    getById: (id: string) => api.get<Team>(`/teams/${id}`),
    create: (team: Omit<Team, 'id'>) => api.post<void>('/teams', team),
    update: (id: string, team: Team) => api.patch<void>(`/teams/${id}`, team),
    delete: (id: string) => api.delete<void>(`/teams/${id}`),
};

// Tournaments
export const tournamentsApi = {
    getAll: () => api.get<Tournament[]>('/tournaments'),
    getById: (id: string) => api.get<Tournament>(`/tournaments/${id}`),
    create: (tournament: Omit<Tournament, 'id'>) => api.post<void>('/tournaments', tournament),
    update: (id: string, tournament: Tournament) => api.patch<void>(`/tournaments/${id}`, tournament),
    delete: (id: string) => api.delete<void>(`/tournaments/${id}`),
};

// Groups
export const groupsApi = {
    getAll: (tournamentId: string) => api.get<Group[]>(`/tournaments/${tournamentId}/groups`),
    getById: (tournamentId: string, groupId: string) =>
        api.get<Group>(`/tournaments/${tournamentId}/groups/${groupId}`),
    create: (tournamentId: string, group: Omit<Group, 'id' | 'tournamentId' | 'teams'>) =>
        api.post<void>(`/tournaments/${tournamentId}/groups`, { ...group, teams: [] }),
    update: (tournamentId: string, groupId: string, group: Group) =>
        api.patch<void>(`/tournaments/${tournamentId}/groups/${groupId}`, group),
    delete: (tournamentId: string, groupId: string) =>
        api.delete<void>(`/tournaments/${tournamentId}/groups/${groupId}`),
    updateTeams: (tournamentId: string, groupId: string, teams: Team[]) =>
        api.patch<void>(`/tournaments/${tournamentId}/groups/${groupId}/teams`, teams),
};

// Matches
export const matchesApi = {
    getAll: (tournamentId: string, filter?: 'played' | 'pending') => {
        const params = filter ? { showMatches: filter } : {};
        return api.get<Match[]>(`/tournaments/${tournamentId}/matches`, { params });
    },
    getById: (tournamentId: string, matchId: string) =>
        api.get<Match>(`/tournaments/${tournamentId}/matches/${matchId}`),
    updateScore: (tournamentId: string, matchId: string, score: Score) =>
        api.patch<void>(`/tournaments/${tournamentId}/matches/${matchId}`, { score }),
};

export default api;