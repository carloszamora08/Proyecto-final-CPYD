import { useState, useEffect } from 'react';
import { teamsApi } from '../services/api';
import type { Team } from '../types';

export default function TeamsPage() {
    const [teams, setTeams] = useState<Team[]>([]);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState('');
    const [showModal, setShowModal] = useState(false);
    const [newTeamName, setNewTeamName] = useState('');

    useEffect(() => {
        loadTeams();
    }, []);

    const loadTeams = async () => {
        try {
            const response = await teamsApi.getAll();
            setTeams(response.data);
            setError('');
        } catch (err: any) {
            setError(err.response?.data || 'Failed to load teams');
        } finally {
            setLoading(false);
        }
    };

    const handleCreateTeam = async (e: React.FormEvent) => {
        e.preventDefault();
        try {
            await teamsApi.create({ name: newTeamName });
            setNewTeamName('');
            setShowModal(false);
            loadTeams();
        } catch (err: any) {
            setError(err.response?.data || 'Failed to create team');
        }
    };

    const handleDeleteTeam = async (id: string) => {
        if (!confirm('Are you sure you want to delete this team?')) return;

        try {
            await teamsApi.delete(id);
            loadTeams();
        } catch (err: any) {
            setError(err.response?.data || 'Failed to delete team');
        }
    };

    if (loading) return <div className="loading">Loading teams...</div>;

    return (
        <div>
            <div className="page-header">
                <h2>Teams</h2>
                <button className="btn btn-primary" onClick={() => setShowModal(true)}>
                    + Create Team
                </button>
            </div>

            {error && <div className="error">{error}</div>}

            <div className="grid">
                {teams.map((team) => (
                    <div key={team.id} className="card">
                        <h3>{team.name}</h3>
                        <p style={{ color: '#6b7280', fontSize: '0.875rem' }}>ID: {team.id}</p>
                        <div style={{ marginTop: '1rem', display: 'flex', gap: '0.5rem' }}>
                            <button
                                className="btn btn-danger"
                                onClick={() => handleDeleteTeam(team.id)}
                                style={{ fontSize: '0.875rem', padding: '0.5rem 1rem' }}
                            >
                                Delete
                            </button>
                        </div>
                    </div>
                ))}
            </div>

            {showModal && (
                <div className="modal-overlay" onClick={() => setShowModal(false)}>
                    <div className="modal" onClick={(e) => e.stopPropagation()}>
                        <div className="modal-header">
                            <h3>Create New Team</h3>
                            <button className="close-btn" onClick={() => setShowModal(false)}>×</button>
                        </div>
                        <form onSubmit={handleCreateTeam}>
                            <div className="form-group">
                                <label>Team Name</label>
                                <input
                                    type="text"
                                    value={newTeamName}
                                    onChange={(e) => setNewTeamName(e.target.value)}
                                    placeholder="Enter team name"
                                    required
                                />
                            </div>
                            <button type="submit" className="btn btn-primary" style={{ width: '100%' }}>
                                Create Team
                            </button>
                        </form>
                    </div>
                </div>
            )}
        </div>
    );
}