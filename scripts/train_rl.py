import socket
import json
import torch
import torch.nn as nn
import torch.optim as optim
import numpy as np
import random
import sys
from collections import deque

# Hyperparameters
GAMMA = 0.95
LR = 0.001
BUFFER_SIZE = 20000
BATCH_SIZE = 128
EPS_START = 1.0
EPS_END = 0.05
TARGET_UPDATE = 10

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

class DQNAgent:
    def __init__(self, state_dim, action_dim):
        self.state_dim = state_dim
        self.action_dim = action_dim
        
        self.policy_net = QNetwork(state_dim, action_dim)
        self.target_net = QNetwork(state_dim, action_dim)
        self.target_net.load_state_dict(self.policy_net.state_dict())
        self.target_net.eval()
        
        self.optimizer = optim.Adam(self.policy_net.parameters(), lr=LR)
        self.memory = deque(maxlen=BUFFER_SIZE)
        
        self.epsilon = EPS_START
        self.actions = ["Attaquer", "Parer", "Esquiver", "Passer"]
        
    def select_action(self, state, evaluate=False):
        if not evaluate and random.random() < self.epsilon:
            return random.randint(0, self.action_dim - 1)
        with torch.no_grad():
            state_t = torch.FloatTensor(state).unsqueeze(0)
            q_values = self.policy_net(state_t)
            return q_values.argmax().item()
            
    def store_transition(self, state, action, reward, next_state, done):
        self.memory.append((state, action, reward, next_state, done))
        
    def train_step(self):
        if len(self.memory) < BATCH_SIZE:
            return 0.0
            
        batch = random.sample(self.memory, BATCH_SIZE)
        states, actions, rewards, next_states, dones = zip(*batch)
        
        states = torch.FloatTensor(np.array(states))
        actions = torch.LongTensor(actions).unsqueeze(1)
        rewards = torch.FloatTensor(rewards).unsqueeze(1)
        next_states = torch.FloatTensor(np.array(next_states))
        dones = torch.FloatTensor(dones).unsqueeze(1)
        
        current_q = self.policy_net(states).gather(1, actions)
        
        with torch.no_grad():
            next_q = self.target_net(next_states).max(1)[0].unsqueeze(1)
            target_q = rewards + (1 - dones) * GAMMA * next_q
            
        loss = nn.MSELoss()(current_q, target_q)
        
        self.optimizer.zero_grad()
        loss.backward()
        self.optimizer.step()
        
        return loss.item()
        
    def update_target_network(self):
        self.target_net.load_state_dict(self.policy_net.state_dict())

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

def calculate_reward(prev_state, curr_state):
    prev_active = prev_state["active_character"]
    curr_active = curr_state["active_character"]
    prev_opponent = prev_state["opponent_character"]
    curr_opponent = curr_state["opponent_character"]
    
    is_resolve = len(curr_active.get("queued_actions", [])) < len(prev_active.get("queued_actions", []))
    
    reward = 0.0
    
    if is_resolve:
        hp_taken = (curr_active.get("blood", 32.0) - prev_active.get("blood", 32.0)) / 32.0
        hp_dealt = (prev_opponent.get("blood", 32.0) - curr_opponent.get("blood", 32.0)) / 32.0
        
        reward += hp_taken * 120.0
        reward += hp_dealt * 100.0
        
        prev_pred_spent = estimate_stamina_spent(prev_active)
        actual_drop = prev_active.get("physical_reserve", 0.0) - curr_active.get("physical_reserve", 0.0)
        
        discrepancy = actual_drop - prev_pred_spent
        max_stam = max(prev_active.get("max_physical_reserve", 100.0), 1.0)
        if discrepancy > 0:
            reward -= (discrepancy / max_stam) * 40.0
        elif discrepancy < 0:
            reward += (-discrepancy / max_stam) * 30.0
            
        if curr_opponent.get("blood", 0.0) <= 0.0 or curr_opponent.get("physical_reserve", 0.0) <= 0.0:
            reward += 60.0
        if curr_active.get("blood", 0.0) <= 0.0 or curr_active.get("physical_reserve", 0.0) <= 0.0:
            reward -= 60.0
    else:
        prev_pred_spent = estimate_stamina_spent(prev_active)
        curr_pred_spent = estimate_stamina_spent(curr_active)
        pred_drop = curr_pred_spent - prev_pred_spent
        
        if pred_drop > 0:
            max_stam = max(curr_active.get("max_physical_reserve", 100.0), 1.0)
            reward -= (pred_drop / max_stam) * 45.0
            
        curr_pred_stamina = curr_active.get("physical_reserve", 0.0) - curr_pred_spent
        curr_pred_stamina_ratio = curr_pred_stamina / max(curr_active.get("max_physical_reserve", 100.0), 1.0)
        
        if curr_pred_stamina_ratio < 0.5:
            reward -= 0.5
        if curr_pred_stamina_ratio < 0.25:
            reward -= 2.0
        if curr_pred_stamina_ratio < 0.10:
            reward -= 6.0
        if curr_pred_stamina_ratio <= 0.0:
            reward -= 20.0
            
    return reward

def main():
    port = 8080
    num_episodes = 1000
    if len(sys.argv) > 1:
        port = int(sys.argv[1])
    if len(sys.argv) > 2:
        num_episodes = int(sys.argv[2])
        
    agent = DQNAgent(state_dim=22, action_dim=4)
    
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('127.0.0.1', port))
    server.listen(1)
    print(f"[RL Trainer] Listening on 127.0.0.1:{port}...")
    
    last_state = None
    last_action = None
    last_state_vec = None
    episode_count = 0
    total_rewards = []
    episode_reward = 0.0
    losses = []
    
    try:
        while True:
            conn, addr = server.accept()
            data = b""
            while b"\n" not in data:
                chunk = conn.recv(1024)
                if not chunk:
                    break
                data += chunk
                
            if not data:
                conn.close()
                continue
                
            try:
                line = data.decode('utf-8').strip()
                state = json.loads(line)
                
                active = state["active_character"]
                opponent = state["opponent_character"]
                curr_state_vec = extract_state_vector(active, opponent)
                
                is_reset = False
                if last_state is not None:
                    # Robust new combat detection: names change, or both at full blood and queue resets
                    if (active["name"] != last_state["active_character"]["name"] or 
                        opponent["name"] != last_state["opponent_character"]["name"]):
                        is_reset = True
                    elif (active["blood"] == 32.0 and opponent["blood"] == 32.0 and 
                          len(active.get("queued_actions", [])) == 0 and 
                          len(last_state["active_character"].get("queued_actions", [])) > 0):
                        is_reset = True
                
                if is_reset and last_state_vec is not None:
                    # Final combat reward based on HP difference
                    win_reward = 20.0 if last_state_vec[0] > last_state_vec[9] else (-20.0 if last_state_vec[0] < last_state_vec[9] else 0.0)
                    agent.store_transition(last_state_vec, last_action, win_reward, last_state_vec, True)
                    total_rewards.append(episode_reward)
                    
                    episode_count += 1
                    decay_fraction = min(1.0, episode_count / num_episodes)
                    agent.epsilon = max(EPS_END, EPS_START - decay_fraction * (EPS_START - EPS_END))
                    
                    if episode_count % TARGET_UPDATE == 0:
                        agent.update_target_network()
                        
                    if episode_count % 20 == 0:
                        torch.save(agent.policy_net.state_dict(), "scripts/rl_model.pt")
                        
                    print(f"Episode {episode_count} Finished | Reward: {episode_reward:.1f} | Epsilon: {agent.epsilon:.3f}")
                    
                    episode_reward = 0.0
                    last_state_vec = None
                    last_action = None
                    last_state = None
                
                if last_state_vec is not None:
                    reward = calculate_reward(last_state, state)
                    agent.store_transition(last_state_vec, last_action, reward, curr_state_vec, False)
                    episode_reward += reward
                    
                    loss = agent.train_step()
                    if loss > 0:
                        losses.append(loss)
                
                action_idx = agent.select_action(curr_state_vec)
                chosen_action = agent.actions[action_idx]
                
                last_state = state
                last_action = action_idx
                last_state_vec = curr_state_vec
                
                response = json.dumps({"action": chosen_action}) + "\n"
                conn.sendall(response.encode('utf-8'))
                
            except Exception as e:
                print(f"Error handling step: {e}")
                conn.sendall(b'{"action": "Passer"}\n')
            finally:
                conn.close()
                
    except KeyboardInterrupt:
        print("\nStopping trainer. Saving model...")
    finally:
        server.close()
        
    torch.save(agent.policy_net.state_dict(), "scripts/rl_model.pt")
    print("Model saved to scripts/rl_model.pt")
    
    if len(total_rewards) > 0:
        avg_reward = np.mean(total_rewards)
        print(f"Average training reward: {avg_reward:.2f}")
        with open("scripts/training_summary.txt", "w") as f:
            f.write(f"Total training episodes: {episode_count}\n")
            f.write(f"Average reward: {avg_reward:.2f}\n")
            if len(losses) > 0:
                f.write(f"Average training loss: {np.mean(losses):.4f}\n")

if __name__ == '__main__':
    main()
