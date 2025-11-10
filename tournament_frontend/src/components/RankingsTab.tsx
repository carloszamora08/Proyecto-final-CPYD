import { useMemo } from 'react';
import type { Match, Group } from '../types';

interface TeamStats {
    teamId: string;
    teamName: string;
    conference: string;
    division: string;
    wins: number;
    losses: number;
    ties: number;
    pointsFor: number;
    pointsAgainst: number;
}

interface RankingsTabProps {
    matches: Match[];
    groups: Group[];
}

export default function RankingsTab({ matches, groups }: RankingsTabProps) {
    const standings = useMemo(() => {
        const stats = new Map<string, TeamStats>();

        // Inicializar stats para todos los equipos
        groups.forEach(group => {
            // Usar directamente el campo conference del grupo
            const conference = group.conference || 'AFC'; // Fallback por si acaso

            group.teams.forEach(team => {
                stats.set(team.id, {
                    teamId: team.id,
                    teamName: team.name,
                    conference: conference,
                    division: group.name,
                    wins: 0,
                    losses: 0,
                    ties: 0,
                    pointsFor: 0,
                    pointsAgainst: 0,
                });
            });
        });

        // Calcular estadísticas de matches de fase regular
        matches.forEach(match => {
            if (match.round !== 'regular' || !match.score) return;

            // Usar la nueva estructura de match con home y visitor
            const homeId = match.home?.id;
            const visitorId = match.visitor?.id;

            if (!homeId || !visitorId) return;

            const homeStats = stats.get(homeId);
            const visitorStats = stats.get(visitorId);

            if (!homeStats || !visitorStats) return;

            homeStats.pointsFor += match.score.home;
            homeStats.pointsAgainst += match.score.visitor;
            visitorStats.pointsFor += match.score.visitor;
            visitorStats.pointsAgainst += match.score.home;

            if (match.score.home > match.score.visitor) {
                homeStats.wins++;
                visitorStats.losses++;
            } else if (match.score.home < match.score.visitor) {
                homeStats.losses++;
                visitorStats.wins++;
            } else {
                homeStats.ties++;
                visitorStats.ties++;
            }
        });

        return Array.from(stats.values());
    }, [matches, groups]);

    const afcStandings = useMemo(() => {
        return standings
            .filter(team => team.conference === 'AFC')
            .sort((a, b) => {
                const aWinPct = (a.wins + 0.5 * a.ties) / (a.wins + a.losses + a.ties || 1);
                const bWinPct = (b.wins + 0.5 * b.ties) / (b.wins + b.losses + b.ties || 1);

                if (aWinPct !== bWinPct) return bWinPct - aWinPct;
                if (a.pointsFor !== b.pointsFor) return b.pointsFor - a.pointsFor;
                return (b.pointsFor - b.pointsAgainst) - (a.pointsFor - a.pointsAgainst);
            });
    }, [standings]);

    const nfcStandings = useMemo(() => {
        return standings
            .filter(team => team.conference === 'NFC')
            .sort((a, b) => {
                const aWinPct = (a.wins + 0.5 * a.ties) / (a.wins + a.losses + a.ties || 1);
                const bWinPct = (b.wins + 0.5 * b.ties) / (b.wins + b.losses + b.ties || 1);

                if (aWinPct !== bWinPct) return bWinPct - aWinPct;
                if (a.pointsFor !== b.pointsFor) return b.pointsFor - a.pointsFor;
                return (b.pointsFor - b.pointsAgainst) - (a.pointsFor - a.pointsAgainst);
            });
    }, [standings]);

    const getWinPercentage = (team: TeamStats) => {
        const total = team.wins + team.losses + team.ties;
        if (total === 0) return '0.000';
        return ((team.wins + 0.5 * team.ties) / total).toFixed(3);
    };

    const renderStandingsTable = (teams: TeamStats[], conference: string) => (
        <div style={{ marginBottom: '2rem' }}>
            <h3 style={{ marginBottom: '1rem', color: '#1e3a8a' }}>{conference} Conference</h3>
            <div style={{ overflowX: 'auto' }}>
                <table style={{
                    width: '100%',
                    borderCollapse: 'collapse',
                    background: 'white',
                    boxShadow: '0 2px 8px rgba(0,0,0,0.08)',
                    borderRadius: '8px'
                }}>
                    <thead>
                    <tr style={{ borderBottom: '2px solid #e5e7eb' }}>
                        <th style={{ padding: '1rem', textAlign: 'left', fontWeight: 600 }}>Rank</th>
                        <th style={{ padding: '1rem', textAlign: 'left', fontWeight: 600 }}>Team</th>
                        <th style={{ padding: '1rem', textAlign: 'left', fontWeight: 600 }}>Division</th>
                        <th style={{ padding: '1rem', textAlign: 'center', fontWeight: 600 }}>W</th>
                        <th style={{ padding: '1rem', textAlign: 'center', fontWeight: 600 }}>L</th>
                        <th style={{ padding: '1rem', textAlign: 'center', fontWeight: 600 }}>T</th>
                        <th style={{ padding: '1rem', textAlign: 'center', fontWeight: 600 }}>PCT</th>
                        <th style={{ padding: '1rem', textAlign: 'center', fontWeight: 600 }}>PF</th>
                        <th style={{ padding: '1rem', textAlign: 'center', fontWeight: 600 }}>PA</th>
                        <th style={{ padding: '1rem', textAlign: 'center', fontWeight: 600 }}>DIFF</th>
                    </tr>
                    </thead>
                    <tbody>
                    {teams.map((team, index) => {
                        const isPlayoffTeam = index < 7;
                        return (
                            <tr
                                key={team.teamId}
                                style={{
                                    borderBottom: '1px solid #e5e7eb',
                                    background: isPlayoffTeam ? '#f0f9ff' : 'white'
                                }}
                            >
                                <td style={{ padding: '1rem' }}>
                                    {index + 1}
                                    {isPlayoffTeam && (
                                        <span style={{
                                            marginLeft: '0.5rem',
                                            color: '#3b82f6',
                                            fontWeight: 600
                                        }}>
                                            ★
                                        </span>
                                    )}
                                </td>
                                <td style={{ padding: '1rem', fontWeight: 500 }}>{team.teamName}</td>
                                <td style={{ padding: '1rem', color: '#6b7280' }}>{team.division}</td>
                                <td style={{ padding: '1rem', textAlign: 'center' }}>{team.wins}</td>
                                <td style={{ padding: '1rem', textAlign: 'center' }}>{team.losses}</td>
                                <td style={{ padding: '1rem', textAlign: 'center' }}>{team.ties}</td>
                                <td style={{ padding: '1rem', textAlign: 'center', fontWeight: 600 }}>
                                    {getWinPercentage(team)}
                                </td>
                                <td style={{ padding: '1rem', textAlign: 'center' }}>{team.pointsFor}</td>
                                <td style={{ padding: '1rem', textAlign: 'center' }}>{team.pointsAgainst}</td>
                                <td style={{
                                    padding: '1rem',
                                    textAlign: 'center',
                                    color: team.pointsFor - team.pointsAgainst > 0 ? '#10b981' : '#ef4444'
                                }}>
                                    {team.pointsFor - team.pointsAgainst > 0 ? '+' : ''}
                                    {team.pointsFor - team.pointsAgainst}
                                </td>
                            </tr>
                        );
                    })}
                    </tbody>
                </table>
            </div>
            <p style={{ marginTop: '0.5rem', color: '#6b7280', fontSize: '0.875rem' }}>
                ★ = Playoff Team (Top 7)
            </p>
        </div>
    );

    return (
        <div>
            {renderStandingsTable(afcStandings, 'AFC')}
            {renderStandingsTable(nfcStandings, 'NFC')}
        </div>
    );
}