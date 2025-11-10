// Types matching your C++ domain models

export interface Team {
    id: string;
    name: string;
}

export interface TournamentFormat {
    numberOfGroups: number;
    maxTeamsPerGroup: number;
    type: 'NFL' | 'ROUND_ROBIN';
}

export interface Tournament {
    id?: string;
    name: string;
    year: number;
    format: TournamentFormat;
    finished: string;
}

export interface Group {
    id?: string;
    name: string;
    region: string;
    tournamentId: string;
    teams: Team[];
}

export interface Score {
    home: number;
    visitor: number;
}

export type RoundType = 'regular' | 'quarterfinals' | 'semifinals' | 'final';

export interface Match {
    id: string;
    tournamentId: string;
    homeTeamId: string;
    homeTeamName: string;
    visitorTeamId: string;
    visitorTeamName: string;
    round: RoundType;
    score?: Score;
    winnerNextMatchId?: string;
    loserNextMatchId?: string;
}