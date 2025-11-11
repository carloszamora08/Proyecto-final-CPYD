import { BrowserRouter as Router, Routes, Route, Link } from 'react-router-dom';
import TeamsPage from './components/TeamsPage';
import TournamentsPage from './components/TournamentsPage';
import TournamentDetailPage from './components/TournamentDetailPage';
import './App.css';

function App() {
    return (
        <Router>
            <div className="app">
                <nav className="navbar">
                    <div className="nav-brand">
                        <h1>🏈 NFL Tournament Manager</h1>
                    </div>
                    <ul className="nav-links">
                        <li><Link to="/">Tournaments</Link></li>
                        <li><Link to="/teams">Teams</Link></li>
                    </ul>
                </nav>

                <main className="main-content">
                    <Routes>
                        <Route path="/" element={<TournamentsPage />} />
                        <Route path="/teams" element={<TeamsPage />} />
                        <Route path="/tournaments/:tournamentId" element={<TournamentDetailPage />} />
                    </Routes>
                </main>
            </div>
        </Router>
    );
}

export default App;