import sys
import json
import torch
import torch.nn as nn
import numpy as np

class QNetwork(nn.Module):
    def __init__(self, state_dim, action_dim):
        super(QNetwork, self).__init__()
        self.fc = nn.Sequential(
            nn.Linear(state_dim, 128),
            nn.ReLU(),
            nn.Linear(128, 128),
            nn.ReLU(),
            nn.Linear(128, action_dim)
        )
        
    def forward(self, x):
        return self.fc(x)

def estimate_stamina_spent(character):
    free_actions = character.get("free_actions", 2)
    queued = character.get("queued_actions", [])
    overclock_multipliers = [2.0, 2.3, 2.6, 3.4, 5.0, 7.0]
    
    total_cost = 0.0
    standard_actions_count = 0
    
    for act in queued:
        if act == "Parer":
            base_cost = 7.5
        elif act == "Esquiver":
            base_cost = 10.0
        elif act == "Attaquer":
            base_cost = 10.0
        else: # Magie or others
            base_cost = 10.0
            
        if standard_actions_count < free_actions:
            mult = 1.0
        else:
            idx = standard_actions_count - free_actions
            if idx > 5: idx = 5
            mult = overclock_multipliers[idx]
        
        total_cost += base_cost * mult
        standard_actions_count += 1
        
    return total_cost

def extract_state_vector(character, opponent):
    active_hp = character.get("blood", 32.0) / 32.0
    max_stam = max(character.get("max_physical_reserve", 100.0), 1.0)
    active_stamina = character.get("physical_reserve", 0.0) / max_stam
    active_mana = character.get("magic_reserve", 0.0) / 100.0
    active_free_actions = character.get("free_actions", 2.0) / 6.0
    
    queued = character.get("queued_actions", [])
    active_queued_total = len(queued) / 6.0
    active_queued_attacks = queued.count("Attaquer") / 6.0
    active_queued_parries = queued.count("Parer") / 6.0
    active_queued_dodges = queued.count("Esquiver") / 6.0
    active_queued_magics = queued.count("Magie") / 6.0
    
    opp_hp = opponent.get("blood", 32.0) / 32.0
    opp_max_stam = max(opponent.get("max_physical_reserve", 100.0), 1.0)
    opp_stamina = opponent.get("physical_reserve", 0.0) / opp_max_stam
    opp_mana = opponent.get("magic_reserve", 0.0) / 100.0
    opp_free_actions = opponent.get("free_actions", 2.0) / 6.0
    
    opp_queued = opponent.get("queued_actions", [])
    opp_queued_total = len(opp_queued) / 6.0
    opp_queued_attacks = opp_queued.count("Attaquer") / 6.0
    opp_queued_parries = opp_queued.count("Parer") / 6.0
    opp_queued_dodges = opp_queued.count("Esquiver") / 6.0
    opp_queued_magics = opp_queued.count("Magie") / 6.0
    
    force_ratio = character.get("force", 10.0) / max(opponent.get("resistance", 10.0), 1.0)
    force_magique_ratio = character.get("force_magique", 10.0) / max(opponent.get("resistance_magique", 10.0), 1.0)
    speed_ratio = character.get("vitesse", 10.0) / max(opponent.get("vitesse", 10.0), 1.0)
    
    pred_stam_spent = estimate_stamina_spent(character)
    pred_stamina_ratio = max(0.0, character.get("physical_reserve", 0.0) - pred_stam_spent) / max_stam
    
    return np.array([
        active_hp, active_stamina, active_mana, active_free_actions,
        active_queued_total, active_queued_attacks, active_queued_parries, active_queued_dodges, active_queued_magics,
        opp_hp, opp_stamina, opp_mana, opp_free_actions,
        opp_queued_total, opp_queued_attacks, opp_queued_parries, opp_queued_dodges, opp_queued_magics,
        force_ratio, force_magique_ratio, speed_ratio, pred_stamina_ratio
    ], dtype=np.float32)

def main():
    actions = ["Attaquer", "Parer", "Esquiver", "Passer"]
    
    if len(sys.argv) < 2:
        print(json.dumps({"action": "Passer"}))
        sys.exit(0)
        
    try:
        raw_state = sys.argv[1]
        state = json.loads(raw_state)
        
        active = state["active_character"]
        opponent = state["opponent_character"]
        state_vec = extract_state_vector(active, opponent)
        
        model = QNetwork(state_dim=22, action_dim=4)
        try:
            model.load_state_dict(torch.load("scripts/rl_model.pt", map_location=torch.device('cpu')))
            model.eval()
            
            with torch.no_grad():
                state_t = torch.FloatTensor(state_vec).unsqueeze(0)
                q_values = model(state_t)
                action_idx = q_values.argmax().item()
                chosen_action = actions[action_idx]
                
                # Fallback safety rule
                if chosen_action == "Passer" and len(active.get("queued_actions", [])) == 0:
                    stamina = active.get("physical_reserve", 0.0)
                    if stamina > 15.0:
                        opp_queued = opponent.get("queued_actions", [])
                        opp_attacks = opp_queued.count("Attaquer")
                        if opp_attacks > 0 and stamina >= 30.0:
                            chosen_action = "Esquiver"
                        else:
                            chosen_action = "Attaquer"
        except Exception as e:
            import traceback
            traceback.print_exc(file=sys.stderr)
            chosen_action = "Passer"
            
        print(json.dumps({"action": chosen_action}))
        
    except Exception as e:
        import traceback
        traceback.print_exc(file=sys.stderr)
        print(json.dumps({"action": "Passer"}))

if __name__ == "__main__":
    main()
