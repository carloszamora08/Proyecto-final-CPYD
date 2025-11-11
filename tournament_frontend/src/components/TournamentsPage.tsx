import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { tournamentsApi } from '../services/api';
import type { Tournament } from '../types';

export default function TournamentsPage() {
    const navigate = useNavigate();
    const [tournaments, setTournaments] = useState<Tournament[]>([]);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState('');
    const [showModal, setShowModal] = useState(false);
    const [formData, setFormData] = useState({
        name: '',
        year: new Date().getFullYear(),
        finished: 'no',
        format: {
            numberOfGroups: 8,
            maxTeamsPerGroup: 4,
            type: 'NFL' as const,
        },
    });

    useEffect(() => {
        loadTournaments();
    }, []);

    const loadTournaments = async () => {
        try {
            const response = await tournamentsApi.getAll();
            setTournaments(response.data);
            setError('');
        } catch (err: any) {
            setError(err.response?.data || 'Failed to load tournaments');
        } finally {
            setLoading(false);
        }
    };

    const handleCreateTournament = async (e: React.FormEvent) => {
        e.preventDefault();
        try {
            await tournamentsApi.create(formData);
            setShowModal(false);
            setFormData({
                name: '',
                year: new Date().getFullYear(),
                finished: 'no',
                format: {
                    numberOfGroups: 8,
                    maxTeamsPerGroup: 4,
                    type: 'NFL',
                },
            });
            loadTournaments();
        } catch (err: any) {
            setError(err.response?.data || 'Failed to create tournament');
        }
    };

    const handleDeleteTournament = async (id: string) => {
        if (!confirm('Are you sure you want to delete this tournament?')) return;

        try {
            await tournamentsApi.delete(id);
            loadTournaments();
        } catch (err: any) {
            setError(err.response?.data || 'Failed to delete tournament');
        }
    };

    if (loading) return <div className="loading">Loading tournaments...</div>;

    return (
        <div>
            <div className="page-header">
                <h2>Tournaments</h2>
                <button className="btn btn-primary" onClick={() => setShowModal(true)}>
                    + Create Tournament
                </button>
            </div>

            {error && <div className="error">{error}</div>}

            <div className="grid">
                {tournaments.map((tournament) => (
                    <div
                        key={tournament.id}
                        className="card"
                        onClick={() => navigate(`/tournaments/${tournament.id}`)}
                        style={{ cursor: 'pointer' }}
                    >
                        <h3>{tournament.name}</h3>
                        <p style={{ color: '#6b7280', marginBottom: '0.5rem' }}>Year: {tournament.year}</p>
                        <div style={{ display: 'flex', gap: '0.5rem', marginTop: '1rem' }}>
                            <span className="badge badge-blue">{tournament.format.type}</span>
                            <span className="badge badge-green">
                {tournament.format.numberOfGroups} Groups
              </span>
                            {tournament.finished === 'yes' && (
                                <span className="badge badge-yellow">Finished</span>
                            )}
                        </div>
                        <button
                            className="btn btn-danger"
                            onClick={(e) => {
                                e.stopPropagation();
                                handleDeleteTournament(tournament.id!);
                            }}
                            style={{ marginTop: '1rem', fontSize: '0.875rem', padding: '0.5rem 1rem' }}
                        >
                            Delete
                        </button>
                    </div>
                ))}
            </div>

            {showModal && (
                <div className="modal-overlay" onClick={() => setShowModal(false)}>
                    <div className="modal" onClick={(e) => e.stopPropagation()}>
                        <div className="modal-header">
                            <h3>Create New Tournament</h3>
                            <button className="close-btn" onClick={() => setShowModal(false)}>×</button>
                        </div>
                        <form onSubmit={handleCreateTournament}>
                            <div className="form-group">
                                <label>Tournament Name</label>
                                <input
                                    type="text"
                                    value={formData.name}
                                    onChange={(e) => setFormData({ ...formData, name: e.target.value })}
                                    placeholder="Enter tournament name"
                                    required
                                />
                            </div>
                            <div className="form-group">
                                <label>Year</label>
                                <input
                                    type="number"
                                    value={formData.year}
                                    onChange={(e) => setFormData({ ...formData, year: parseInt(e.target.value) })}
                                    required
                                />
                            </div>
                            <div className="form-group">
                                <label>Number of Groups</label>
                                <input
                                    type="number"
                                    value={formData.format.numberOfGroups}
                                    onChange={(e) => setFormData({
                                        ...formData,
                                        format: { ...formData.format, numberOfGroups: parseInt(e.target.value) }
                                    })}
                                    required
                                />
                            </div>
                            <div className="form-group">
                                <label>Max Teams per Group</label>
                                <input
                                    type="number"
                                    value={formData.format.maxTeamsPerGroup}
                                    onChange={(e) => setFormData({
                                        ...formData,
                                        format: { ...formData.format, maxTeamsPerGroup: parseInt(e.target.value) }
                                    })}
                                    required
                                />
                            </div>
                            <button type="submit" className="btn btn-primary" style={{ width: '100%' }}>
                                Create Tournament
                            </button>
                        </form>
                    </div>
                </div>
            )}
        </div>
    );
}