#pragma once

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <json.hpp>
#include "entity.hpp"

class Simulator {
public:
    Simulator();
    ~Simulator();

    // Start combat
    void startCombat(const Entity& f1, const Entity& f2, ControlMode m1, ControlMode m2);

    // Getters / Setters for fighters
    std::optional<Entity>& getFighter1() { return fighter1; }
    std::optional<Entity>& getFighter2() { return fighter2; }
    const std::optional<Entity>& getFighter1() const { return fighter1; }
    const std::optional<Entity>& getFighter2() const { return fighter2; }

    // Getters / Setters for action queues
    std::vector<QueuedAction>& getP1Actions() { return p1Actions; }
    std::vector<QueuedAction>& getP2Actions() { return p2Actions; }
    const std::vector<QueuedAction>& getP1Actions() const { return p1Actions; }
    const std::vector<QueuedAction>& getP2Actions() const { return p2Actions; }

    // Free actions counts
    int getP1FreeActions() const { return p1FreeActions; }
    int getP2FreeActions() const { return p2FreeActions; }
    void setP1FreeActions(int count) { p1FreeActions = count; }
    void setP2FreeActions(int count) { p2FreeActions = count; }

    // Current turn
    int getCurrentTurn() const { return currentTurn; }
    void setCurrentTurn(int turn) { currentTurn = turn; }

    // Validation/Finished state
    bool isP1Finished() const { return p1Finished; }
    bool isP2Finished() const { return p2Finished; }
    void setP1Finished(bool finished) { p1Finished = finished; }
    void setP2Finished(bool finished) { p2Finished = finished; }

    // Control modes
    ControlMode getP1Mode() const { return p1Mode; }
    ControlMode getP2Mode() const { return p2Mode; }

    // Actions manipulation
    void addActionP1(ActionType type, const std::string& magicSpell = "", bool useCatalyst = false);
    void addActionP2(ActionType type, const std::string& magicSpell = "", bool useCatalyst = false);
    void popActionP1();
    void popActionP2();
    void clearActions() {
        p1Actions.clear();
        p2Actions.clear();
    }

    // Overclocking multiplier calculation
    std::vector<float> computeOverclockMultipliers(const Entity& entity, const std::vector<ActionType>& actions, int baseFreeActions) const;
    float getNextMultiplier(const Entity& entity, const std::vector<QueuedAction>& currentActions, ActionType nextType, int baseFreeActions) const;

    // Turn Resolution result structures
    struct TurnResult {
        bool combatFinished = false;
        std::string winnerName;
        std::string reason; // "mort" or "épuisement" or "match nul"
        std::vector<std::string> logs;
    };

    // Callback to query external agents (Python script or TCP server)
    // Takes the state JSON string and player number, returns the action name string (e.g. "Attaquer")
    using ExternalAgentQueryCallback = std::function<std::string(const std::string& stateJson, int playerNum)>;
    void setExternalAgentQueryCallback(ExternalAgentQueryCallback cb) {
        externalAgentQuery = cb;
    }

    // Resolve a complete turn
    TurnResult resolveTurn();

    // Check if both players are finished
    bool checkResolve() const {
        return p1Finished && p2Finished;
    }

    // JSON serialization
    std::string serializeStateJson(const Entity& active, const std::vector<QueuedAction>& activeActions,
                                   const Entity& opponent, const std::vector<QueuedAction>& opponentActions) const;
    nlohmann::json serializeEntityJson(const Entity& entity, const std::vector<QueuedAction>& queuedActions, int freeActions) const;

private:
    std::optional<Entity> fighter1;
    std::optional<Entity> fighter2;
    ControlMode p1Mode = ControlMode::Manual;
    ControlMode p2Mode = ControlMode::Manual;

    std::vector<QueuedAction> p1Actions;
    std::vector<QueuedAction> p2Actions;
    int p1FreeActions = 2;
    int p2FreeActions = 2;
    int currentTurn = 1;
    bool p1Finished = false;
    bool p2Finished = false;

    ExternalAgentQueryCallback externalAgentQuery;

    void fetchAutomatedActions(Entity& entity, std::vector<QueuedAction>& actions, int freeActions, ControlMode mode, int playerNum);
    void executeSingleAction(Entity* attacker, Entity* defender, const QueuedAction& action, int actionIndex, std::vector<std::string>& logs);
};