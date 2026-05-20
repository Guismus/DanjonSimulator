import sys
import json
import random

def main():
    if len(sys.argv) < 2:
        print("Error: Missing JSON argument")
        sys.exit(1)
        
    try:
        state = json.loads(sys.argv[1])
        # Simple AI that chooses random action
        actions = ["Attaquer", "Parer", "Esquiver", "Magie", "Passer"]
        chosen = random.choice(actions)
        
        # Return decision
        print(json.dumps({"action": chosen}))
    except Exception as e:
        print(f"Error processing AI step: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    main()
