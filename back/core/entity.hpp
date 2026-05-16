class Entity {
public:
    Entity(const std::string& name, int bleedDamage, const std::map<std::string, int>& stats);
    const std::string& getName() const;
    int getBleedDamage() const;
    const std::map<std::string, int>& getStats() const;

private:
    std::string name;
    int bleedDamage;
    std::map<std::string, int> stats;
};