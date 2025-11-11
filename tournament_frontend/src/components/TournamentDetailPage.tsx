import { useState, useEffect } from 'react';
import { useParams, useNavigate } from 'react-router-dom';
import { tournamentsApi, groupsApi, matchesApi, teamsApi } from '../services/api';
import type { Tournament, Group, Match, Team } from '../types';
import RankingsTab from './RankingsTab';

export default function TournamentDetailPage() {
    const { tournamentId } = useParams<{ tournamentId: string }>();
    const navigate = useNavigate();
    const [tournament, setTournament] = useState<Tournament | null>(null);
    const [groups, setGroups] = useState<Group[]>([]);
    const [matches, setMatches] = useState<Match[]>([]);
    const [allTeams, setAllTeams] = useState<Team[]>([]);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState('');
    const [activeTab, setActiveTab] = useState<'groups' | 'matches' | 'rankings'>('groups');
    const [selectedRound, setSelectedRound] = useState<string>('all');

    // Modals
    const [showGroupModal, setShowGroupModal] = useState(false);
    const [showTeamModal, setShowTeamModal] = useState(false);
    const [showScoreModal, setShowScoreModal] = useState(false);
    const [selectedGroup, setSelectedGroup] = useState<string>('');
    const [selectedMatch, setSelectedMatch] = useState<Match | null>(null);

    const [groupForm, setGroupForm] = useState({
        name: '',
        region: '',
        conference: 'AFC' as 'AFC' | 'NFC'
    });
    const [selectedTeamIds, setSelectedTeamIds] = useState<string[]>([]);
    const [scoreForm, setScoreForm] = useState({ home: 0, visitor: 0 });

    useEffect(() => {
        if (tournamentId) {
            loadTournamentData();
        }
    }, [tournamentId]);

    const loadTournamentData = async () => {
        try {
            const [tournamentRes, groupsRes, matchesRes, teamsRes] = await Promise.all([
                tournamentsApi.getById(tournamentId!),
                groupsApi.getAll(tournamentId!),
                matchesApi.getAll(tournamentId!),
                teamsApi.getAll(),
            ]);

            setTournament(tournamentRes.data);
            setGroups(groupsRes.data);
            setMatches(matchesRes.data);
            setAllTeams(teamsRes.data);
            setError('');
        } catch (err: any) {
            setError(err.response?.data || 'Failed to load tournament data');
        } finally {
            setLoading(false);
        }
    };

    const handleCreateGroup = async (e: React.FormEvent) => {
        e.preventDefault();
        try {
            await groupsApi.create(tournamentId!, groupForm);
            setShowGroupModal(false);
            setGroupForm({ name: '', region: '', conference: 'AFC' });
            loadTournamentData();
        } catch (err: any) {
            setError(err.response?.data || 'Failed to create group');
        }
    };

    const handleAddTeamsToGroup = async (e: React.FormEvent) => {
        e.preventDefault();
        try {
            const teamsToAdd = selectedTeamIds.map(id => ({
                id,
                name: allTeams.find(t => t.id === id)?.name || ''
            }));

            await groupsApi.updateTeams(tournamentId!, selectedGroup, teamsToAdd);
            setShowTeamModal(false);
            setSelectedTeamIds([]);
            setSelectedGroup('');
            loadTournamentData();
        } catch (err: any) {
            setError(err.response?.data || 'Failed to add teams to group');
        }
    };

    const handleUpdateScore = async (e: React.FormEvent) => {
        e.preventDefault();
        if (!selectedMatch) return;

        try {
            await matchesApi.updateScore(tournamentId!, selectedMatch.id, scoreForm);
            setShowScoreModal(false);
            setSelectedMatch(null);
            setScoreForm({ home: 0, visitor: 0 });
            loadTournamentData();
        } catch (err: any) {
            setError(err.response?.data || 'Failed to update score');
        }
    };

    const openScoreModal = (match: Match) => {
        setSelectedMatch(match);
        setScoreForm(match.score || { home: 0, visitor: 0 });
        setShowScoreModal(true);
    };

    const getAvailableTeams = () => {
        const usedTeamIds = new Set(groups.flatMap(g => g.teams.map(t => t.id)));
        return allTeams.filter(t => !usedTeamIds.has(t.id));
    };

    const filteredMatches = selectedRound === 'all' 
        ? matches 
        : matches.filter(match => match.round === selectedRound);

    const getMatchCountByRound = (round: string) => {
        if (round === 'all') return matches.length;
        return matches.filter(m => m.round === round).length;
    };

    const getRoundLabel = (round: string) => {
        const labels: { [key: string]: string } = {
            'all': 'All Matches',
            'regular': 'Regular Season',
            'wild card': 'Wild Card',
            'divisional': 'Divisional',
            'championship': 'Championship',
            'super bowl': 'Super Bowl'
        };
        return labels[round] || round;
    };

    if (loading) return <div className="loading">Loading tournament...</div>;
    if (!tournament) return <div className="error">Tournament not found</div>;

    return (
        <div>
            <div style={{ marginBottom: '2rem' }}>
                <button onClick={() => navigate('/')} className="btn" style={{ marginBottom: '1rem' }}>
                    ← Back to Tournaments
                </button>
                <div style={{ display: 'flex', alignItems: 'center', gap: '1rem' }}>
                    <h2>{tournament.name}</h2>
                    {tournament.finished === 'yes' && (
                        <span className="badge badge-yellow" style={{ fontSize: '1rem', padding: '0.5rem 1rem' }}>
                            Tournament Finished
                        </span>
                    )}
                </div>
                <p style={{ color: '#6b7280' }}>Year: {tournament.year}</p>
            </div>

            {error && <div className="error">{error}</div>}

            <div style={{ display: 'flex', gap: '1rem', marginBottom: '2rem', borderBottom: '2px solid #e5e7eb' }}>
                <button
                    onClick={() => setActiveTab('groups')}
                    style={{
                        padding: '1rem',
                        border: 'none',
                        background: 'none',
                        borderBottom: activeTab === 'groups' ? '2px solid #3b82f6' : 'none',
                        color: activeTab === 'groups' ? '#3b82f6' : '#6b7280',
                        fontWeight: 500,
                        cursor: 'pointer'
                    }}
                >
                    Groups ({groups.length})
                </button>
                <button
                    onClick={() => setActiveTab('matches')}
                    style={{
                        padding: '1rem',
                        border: 'none',
                        background: 'none',
                        borderBottom: activeTab === 'matches' ? '2px solid #3b82f6' : 'none',
                        color: activeTab === 'matches' ? '#3b82f6' : '#6b7280',
                        fontWeight: 500,
                        cursor: 'pointer'
                    }}
                >
                    Matches ({matches.length})
                </button>
                <button
                    onClick={() => setActiveTab('rankings')}
                    style={{
                        padding: '1rem',
                        border: 'none',
                        background: 'none',
                        borderBottom: activeTab === 'rankings' ? '2px solid #3b82f6' : 'none',
                        color: activeTab === 'rankings' ? '#3b82f6' : '#6b7280',
                        fontWeight: 500,
                        cursor: 'pointer'
                    }}
                >
                    Rankings
                </button>
            </div>

            {activeTab === 'groups' && (
                <>
                    <div style={{ display: 'flex', justifyContent: 'flex-end', marginBottom: '1rem' }}>
                        <button className="btn btn-primary" onClick={() => setShowGroupModal(true)}>
                            + Create Group
                        </button>
                    </div>
                    <div className="grid">
                        {groups.map((group) => (
                            <div key={group.id} className="card">
                                <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '0.5rem' }}>
                                    <h3>{group.name}</h3>
                                    <span className={`badge ${group.conference === 'AFC' ? 'badge-blue' : 'badge-green'}`}>
                                        {group.conference}
                                    </span>
                                </div>
                                <p style={{ color: '#6b7280', fontSize: '0.875rem' }}>Region: {group.region}</p>
                                <div style={{ marginTop: '1rem' }}>
                                    <strong>Teams ({group.teams.length}):</strong>
                                    <ul style={{ marginTop: '0.5rem', paddingLeft: '1.25rem' }}>
                                        {group.teams.map((team) => (
                                            <li key={team.id}>{team.name}</li>
                                        ))}
                                    </ul>
                                </div>
                                <button
                                    className="btn btn-success"
                                    onClick={() => {
                                        setSelectedGroup(group.id!);
                                        setShowTeamModal(true);
                                    }}
                                    style={{ marginTop: '1rem', fontSize: '0.875rem', padding: '0.5rem 1rem', width: '100%' }}
                                    disabled={tournament.finished === 'yes'}
                                >
                                    Add Teams
                                </button>
                            </div>
                        ))}
                    </div>
                </>
            )}

            {activeTab === 'matches' && (
                <>
                    <div style={{ 
                        marginBottom: '2rem',
                        display: 'flex',
                        gap: '0.5rem',
                        flexWrap: 'wrap',
                        background: '#f3f4f6',
                        padding: '0.75rem',
                        borderRadius: '8px'
                    }}>
                        {['all', 'regular', 'wild card', 'divisional', 'championship', 'super bowl'].map((round) => {
                            const count = getMatchCountByRound(round);
                            return (
                                <button
                                    key={round}
                                    onClick={() => setSelectedRound(round)}
                                    style={{
                                        padding: '0.5rem 1rem',
                                        border: 'none',
                                        borderRadius: '6px',
                                        background: selectedRound === round ? '#3b82f6' : 'white',
                                        color: selectedRound === round ? 'white' : '#6b7280',
                                        fontWeight: 500,
                                        cursor: 'pointer',
                                        transition: 'all 0.2s',
                                        fontSize: '0.875rem'
                                    }}
                                >
                                    {getRoundLabel(round)} ({count})
                                </button>
                            );
                        })}
                    </div>

                    {filteredMatches.length === 0 ? (
                        <div style={{ 
                            textAlign: 'center', 
                            padding: '3rem', 
                            color: '#6b7280',
                            background: '#f9fafb',
                            borderRadius: '8px'
                        }}>
                            No matches found for {getRoundLabel(selectedRound)}
                        </div>
                    ) : (
                        <div className="grid">
                            {filteredMatches.map((match) => (
                                <div key={match.id} className="card">
                                    <span className="badge badge-yellow" style={{ marginBottom: '0.5rem' }}>
                                        {match.round.toUpperCase()}
                                    </span>
                                    <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginTop: '0.5rem' }}>
                                        <div style={{ flex: 1 }}>
                                            <p style={{ fontWeight: 500 }}>{match.home?.name || 'TBD'}</p>
                                            <p style={{ fontSize: '1.5rem', fontWeight: 'bold', color: '#3b82f6' }}>
                                                {match.score?.home ?? '-'}
                                            </p>
                                        </div>
                                        <div style={{ padding: '0 1rem', color: '#6b7280' }}>vs</div>
                                        <div style={{ flex: 1, textAlign: 'right' }}>
                                            <p style={{ fontWeight: 500 }}>{match.visitor?.name || 'TBD'}</p>
                                            <p style={{ fontSize: '1.5rem', fontWeight: 'bold', color: '#3b82f6' }}>
                                                {match.score?.visitor ?? '-'}
                                            </p>
                                        </div>
                                    </div>
                                    <button
                                        className="btn btn-primary"
                                        onClick={() => openScoreModal(match)}
                                        style={{ marginTop: '1rem', fontSize: '0.875rem', padding: '0.5rem 1rem', width: '100%' }}
                                        disabled={!match.home?.id || !match.visitor?.id || tournament.finished === 'yes'}
                                    >
                                        {match.score ? 'Update Score' : 'Enter Score'}
                                    </button>
                                </div>
                            ))}
                        </div>
                    )}
                </>
            )}

            {activeTab === 'rankings' && (
                <RankingsTab matches={matches} groups={groups} />
            )}

            {/* Group Modal */}
            {showGroupModal && (
                <div className="modal-overlay" onClick={() => setShowGroupModal(false)}>
                    <div className="modal" onClick={(e) => e.stopPropagation()}>
                        <div className="modal-header">
                            <h3>Create New Group</h3>
                            <button className="close-btn" onClick={() => setShowGroupModal(false)}>×</button>
                        </div>
                        <form onSubmit={handleCreateGroup}>
                            <div className="form-group">
                                <label>Group Name</label>
                                <input
                                    type="text"
                                    value={groupForm.name}
                                    onChange={(e) => setGroupForm({ ...groupForm, name: e.target.value })}
                                    placeholder="e.g., AFC North"
                                    required
                                />
                            </div>
                            <div className="form-group">
                                <label>Region</label>
                                <input
                                    type="text"
                                    value={groupForm.region}
                                    onChange={(e) => setGroupForm({ ...groupForm, region: e.target.value })}
                                    placeholder="e.g., North, South, East, West"
                                    required
                                />
                            </div>
                            <div className="form-group">
                                <label>Conference</label>
                                <select
                                    value={groupForm.conference}
                                    onChange={(e) => setGroupForm({
                                        ...groupForm,
                                        conference: e.target.value as 'AFC' | 'NFC'
                                    })}
                                    required
                                    style={{
                                        width: '100%',
                                        padding: '0.75rem',
                                        border: '2px solid #e5e7eb',
                                        borderRadius: '8px',
                                        fontSize: '1rem',
                                        backgroundColor: 'white',
                                        cursor: 'pointer'
                                    }}
                                >
                                    <option value="AFC">AFC - American Football Conference</option>
                                    <option value="NFC">NFC - National Football Conference</option>
                                </select>
                            </div>
                            <button type="submit" className="btn btn-primary" style={{ width: '100%' }}>
                                Create Group
                            </button>
                        </form>
                    </div>
                </div>
            )}

            {/* Team Modal */}
            {showTeamModal && (
                <div className="modal-overlay" onClick={() => setShowTeamModal(false)}>
                    <div className="modal" onClick={(e) => e.stopPropagation()}>
                        <div className="modal-header">
                            <h3>Add Teams to Group</h3>
                            <button className="close-btn" onClick={() => setShowTeamModal(false)}>×</button>
                        </div>
                        <form onSubmit={handleAddTeamsToGroup}>
                            <div className="form-group">
                                <label>Select Teams</label>
                                <select
                                    multiple
                                    value={selectedTeamIds}
                                    onChange={(e) => setSelectedTeamIds(Array.from(e.target.selectedOptions, o => o.value))}
                                    style={{ height: '200px' }}
                                    required
                                >
                                    {getAvailableTeams().map((team) => (
                                        <option key={team.id} value={team.id}>
                                            {team.name}
                                        </option>
                                    ))}
                                </select>
                                <small style={{ color: '#6b7280', display: 'block', marginTop: '0.5rem' }}>
                                    Hold Ctrl/Cmd to select multiple teams
                                </small>
                            </div>
                            <button type="submit" className="btn btn-primary" style={{ width: '100%' }}>
                                Add Selected Teams
                            </button>
                        </form>
                    </div>
                </div>
            )}

            {/* Score Modal */}
            {showScoreModal && selectedMatch && (
                <div className="modal-overlay" onClick={() => setShowScoreModal(false)}>
                    <div className="modal" onClick={(e) => e.stopPropagation()}>
                        <div className="modal-header">
                            <h3>Update Match Score</h3>
                            <button className="close-btn" onClick={() => setShowScoreModal(false)}>×</button>
                        </div>
                        <form onSubmit={handleUpdateScore}>
                            <div className="form-group">
                                <label>{selectedMatch.home.name} (Home)</label>
                                <input
                                    type="number"
                                    min="0"
                                    max="10"
                                    value={scoreForm.home}
                                    onChange={(e) => setScoreForm({ ...scoreForm, home: parseInt(e.target.value) })}
                                    required
                                />
                            </div>
                            <div className="form-group">
                                <label>{selectedMatch.visitor.name} (Visitor)</label>
                                <input
                                    type="number"
                                    min="0"
                                    max="10"
                                    value={scoreForm.visitor}
                                    onChange={(e) => setScoreForm({ ...scoreForm, visitor: parseInt(e.target.value) })}
                                    required
                                />
                            </div>
                            <button type="submit" className="btn btn-primary" style={{ width: '100%' }}>
                                Update Score
                            </button>
                        </form>
                    </div>
                </div>
            )}
        </div>
    );
}