#include <QCoreApplication>
#include "../core/entity.hpp"
#include "../core/simulator.hpp"
#include "../data/DataStore.hpp"
#include "../bindings/AgentRunner.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <ctime>

// C++ implementation of the heuristic ai_agent.py to avoid heavy process spawning
std::string runHeuristicCpp(const Entity& active, const std::vector<QueuedAction>& activeActions,
                            const Entity& opponent, const std::vector<QueuedAction>& opponentActions) {
    float stamina = active.physicalReserve;
    size_t numQueued = activeActions.size();

    // 1. Protection contre l'épuisement
    if (stamina <= 15.0f) {
        return "Passer";
    }

    // 2. Gestion du surcadençage
    if (numQueued >= 2) {
        if (stamina < 80.0f) {
            return "Passer";
        }
        if (numQueued >= 4) {
            return "Passer";
        }
    }

    // Count opponent attacks and our defenses
    int oppAttacks = 0;
    for (const auto& act : opponentActions) {
        if (act.type == ActionType::Attack) oppAttacks++;
    }
    int ourDefenses = 0;
    for (const auto& act : activeActions) {
        if (act.type == ActionType::Parry || act.type == ActionType::Dodge) ourDefenses++;
    }

    // 3. Stratégie défensive
    if (oppAttacks > ourDefenses) {
        if (stamina < 30.0f) {
            return "Parer";
        } else {
            return "Esquiver";
        }
    }

    // 4. Stratégie offensive
    int attacksQueued = 0;
    for (const auto& act : activeActions) {
        if (act.type == ActionType::Attack) attacksQueued++;
    }
    if (attacksQueued < 2) {
        return "Attaquer";
    }

    return "Passer";
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    std::srand(std::time(nullptr));

    int numEpisodes = 200;
    std::string host = "127.0.0.1";
    int port = 8080;
    bool verbose = false;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--episodes" && i + 1 < argc) {
            numEpisodes = std::stoi(argv[++i]);
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--verbose") {
            verbose = true;
        }
    }

    std::cout << "[CLI] Loading simulator data..." << std::endl;
    if (!DataStore::getInstance().loadSystemData("data/diff_stats.json")) {
        std::cerr << "Failed to load system data" << std::endl;
        return 1;
    }
    DataStore::getInstance().loadEnergySystem("data/energy_system.json");
    DataStore::getInstance().loadCatalysts("data/catalysts.json");
    DataStore::getInstance().loadSpells("data/magies");
    DataStore::getInstance().loadRaces("data/races");
    DataStore::getInstance().loadArmors("data/Equipement/Armure");
    DataStore::getInstance().loadWeapons("data/Equipement/Arme");
    DataStore::getInstance().loadEntities("data/entities");

    // Filter rank E entities
    std::vector<std::string> rankEEntities;
    auto allEntities = DataStore::getInstance().getAvailableEntityNames();
    for (const auto& name : allEntities) {
        auto tOpt = DataStore::getInstance().getEntityTemplate(name);
        if (tOpt && tOpt->rank == 2) { // Rank E is 2
            rankEEntities.push_back(name);
        }
    }

    if (rankEEntities.empty()) {
        std::cerr << "Error: No Rank E entities found in data/entities!" << std::endl;
        return 1;
    }

    std::cout << "[CLI] Found " << rankEEntities.size() << " Rank E characters:" << std::endl;
    for (const auto& name : rankEEntities) {
        std::cout << "  - " << name << std::endl;
    }

    std::cout << "[CLI] Starting training: " << numEpisodes << " episodes on host " << host << ":" << port << std::endl;

    Simulator sim;
    AgentRunner runner;

    sim.setExternalAgentQueryCallback([&runner, &sim](const std::string& stateJson, int playerNum) {
        if (playerNum == 1) {
            return runner.query(stateJson, playerNum);
        } else {
            return runHeuristicCpp(*sim.getFighter2(), sim.getP2Actions(), *sim.getFighter1(), sim.getP1Actions());
        }
    });

    AgentConfig config1;
    config1.mode = ControlMode::TCP;
    config1.tcpHost = host;
    config1.tcpPort = port;

    runner.configurePlayer1(config1);

    runner.setLogCallback([verbose](const std::string& msg) {
        if (verbose) {
            std::cout << "[AgentRunner] " << msg << std::endl;
        }
    });

    int p1Wins = 0;
    int p2Wins = 0;
    int draws = 0;

    for (int ep = 0; ep < numEpisodes; ++ep) {
        std::string name1 = rankEEntities[std::rand() % rankEEntities.size()];
        std::string name2 = rankEEntities[std::rand() % rankEEntities.size()];
        while (name1 == name2 && rankEEntities.size() > 1) {
            name2 = rankEEntities[std::rand() % rankEEntities.size()];
        }

        if (verbose) {
            std::cout << "\n=== Episode " << ep + 1 << "/" << numEpisodes << ": " << name1 << " vs " << name2 << " ===" << std::endl;
        }

        Entity f1 = DataStore::getInstance().createFighter(name1, "Aucune", "Aucune");
        Entity f2 = DataStore::getInstance().createFighter(name2, "Aucune", "Aucune");

        sim.startCombat(f1, f2, ControlMode::TCP, ControlMode::Script);

        bool finished = false;
        while (!finished) {
            sim.setP1Finished(false);
            sim.setP2Finished(false);

            auto turnResult = sim.resolveTurn();
            
            if (verbose) {
                for (const auto& log : turnResult.logs) {
                    std::cout << log << std::endl;
                }
            }

            if (turnResult.combatFinished) {
                finished = true;
                if (turnResult.reason == "match nul") {
                    draws++;
                    if (verbose) std::cout << "Result: Draw" << std::endl;
                } else if (turnResult.winnerName == f1.getName()) {
                    p1Wins++;
                    if (verbose) std::cout << "Result: Winner is " << f1.getName() << " (TCP Agent)" << std::endl;
                } else {
                    p2Wins++;
                    if (verbose) std::cout << "Result: Winner is " << f2.getName() << " (Heuristic Script)" << std::endl;
                }
            }
        }
        
        if (!verbose && (ep + 1) % 10 == 0) {
            std::cout << "Episode " << ep + 1 << "/" << numEpisodes 
                      << " | TCP wins: " << p1Wins 
                      << " | Heuristic wins: " << p2Wins 
                      << " | Draws: " << draws << std::endl;
        }
    }

    std::cout << "\n[CLI] Training Completed!" << std::endl;
    std::cout << "Total episodes: " << numEpisodes << std::endl;
    std::cout << "TCP Agent wins: " << p1Wins << " (" << (p1Wins * 100.0 / numEpisodes) << "%)" << std::endl;
    std::cout << "Heuristic Script wins: " << p2Wins << " (" << (p2Wins * 100.0 / numEpisodes) << "%)" << std::endl;
    std::cout << "Draws: " << draws << " (" << (draws * 100.0 / numEpisodes) << "%)" << std::endl;

    return 0;
}
