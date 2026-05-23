import sys
import json

def make_decision(state):
    active = state["active_character"]
    opponent = state["opponent_character"]
    
    queued = active.get("queued_actions", [])
    opp_queued = opponent.get("queued_actions", [])
    
    num_queued = len(queued)
    stamina = active.get("physical_reserve", 0.0)
    
    opp_attacks = opp_queued.count("Attaquer")
    our_defenses = queued.count("Esquiver") + queued.count("Parer")
    
    # 1. Protection contre l'épuisement (Stamina check)
    # Si l'endurance est critique, on s'arrête immédiatement
    if stamina <= 15.0:
        return "Passer"
        
    # 2. Gestion du surcadençage (Overclocking)
    # Le surcadençage consomme beaucoup d'endurance (multiplicateurs élevés)
    if num_queued >= 2:
        # On n'autorise plus de 2 actions que si on a beaucoup d'endurance
        if stamina < 80.0:
            return "Passer"
        # Limite stricte pour éviter d'épuiser toute notre endurance en un tour
        if num_queued >= 4:
            return "Passer"
            
    # 3. Stratégie défensive (Défense prioritaire si l'adversaire prépare des attaques)
    # Si l'adversaire a prévu plus d'attaques que nous n'avons prévu de défenses
    if opp_attacks > our_defenses:
        # On privilégie l'esquive car elle annule complètement les dégâts
        # Sauf si on a peu d'endurance, on choisit la parade qui coûte moins cher (7.5 vs 10)
        if stamina < 30.0:
            return "Parer"
        else:
            return "Esquiver"
            
    # 4. Stratégie offensive (Attaque si on est en sécurité relative)
    # Si on a de l'endurance disponible et qu'on n'a pas encore trop d'attaques
    attacks_queued = queued.count("Attaquer")
    if attacks_queued < 2:
        return "Attaquer"
        
    # Par défaut, si on est bien défendu et qu'on a déjà attaqué, on passe le tour
    return "Passer"

def main():
    if len(sys.argv) < 2:
        print("Error: Missing JSON argument")
        sys.exit(1)
        
    try:
        state = json.loads(sys.argv[1])
        chosen = make_decision(state)
        
        # Retourne la décision au format JSON attendu par le simulateur
        print(json.dumps({"action": chosen}))
    except Exception as e:
        print(f"Error processing AI step: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    main()
