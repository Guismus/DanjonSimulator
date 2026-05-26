# Documentation - Réalisation d'un Script Python pour l'IA

Ce document explique comment réaliser et intégrer un script d'intelligence artificielle en Python pour prendre des décisions de combat dans le **Danjon Simulator**.

---

## 1. Fonctionnement Général

Le simulateur exécute le script Python en tant que sous-processus de la manière suivante :
```bash
python3 <chemin_du_script> '<json_d_etat>'
```

Le script reçoit l'état complet du combat sous la forme d'une chaîne JSON compacte passée en tant que premier argument de ligne de commande (`sys.argv[1]`). Il doit ensuite effectuer ses calculs, puis imprimer sa décision sur la sortie standard (`stdout`) au format JSON.

---

## 2. Structure du JSON d'Entrée

Le JSON reçu contient deux objets principaux :
* `active_character` : Informations sur le personnage contrôlé par le script.
* `opponent_character` : Informations sur l'adversaire.

Chaque personnage possède la structure suivante :
```json
{
  "name": "Nom du personnage",
  "stade": 10,
  "rank": 2,
  "blood": 32.0,
  "physical_reserve": 250.0,
  "max_physical_reserve": 250.0,
  "magic_reserve": 0.0,
  "vitesse": 26.25,
  "force": 24.0,
  "resistance": 18.25,
  "force_magique": 10.0,
  "resistance_magique": 19.0,
  "queued_actions": ["Attaquer", "Parer"],
  "free_actions": 2
}
```

---

## 3. Format de Réponse Attendu

Le script doit imprimer un unique objet JSON sur sa sortie standard (`stdout`) contenant la clé `"action"` :
```json
{"action": "Attaquer"}
```

Les actions possibles sont :
* `"Attaquer"` : Prépare une attaque physique.
* `"Parer"` : Prépare une parade (réduit les dégâts de la prochaine attaque reçue de 10%).
* `"Esquiver"` : Prépare une esquive (permet d'esquiver la prochaine attaque reçue).
* `"Passer"` : Termine la phase de planification du tour pour ce personnage.

---

## 4. Exemple de Script Complet

Voici un squelette de script complet et robuste à placer dans le dossier `scripts/` (par exemple `scripts/mon_ia.py`) :

```python
import sys
import json

def make_decision(state):
    active = state["active_character"]
    opponent = state["opponent_character"]
    
    # 1. Récupération des ressources et des files d'actions
    stamina = active.get("physical_reserve", 0.0)
    queued_actions = active.get("queued_actions", [])
    opp_queued_actions = opponent.get("queued_actions", [])
    
    # 2. Sécurité de base
    if stamina <= 15.0:
        return "Passer"  # Plus assez d'endurance, on passe notre tour pour régénérer
        
    # Limite sur le nombre d'actions (Surcadençage)
    if len(queued_actions) >= 2 and stamina < 80.0:
        return "Passer"
    if len(queued_actions) >= 4:
        return "Passer"
        
    # 3. Logique décisionnelle simple
    opp_attacks = opp_queued_actions.count("Attaquer")
    our_defenses = queued_actions.count("Esquiver") + queued_actions.count("Parer")
    
    # Si l'ennemi prévoit plus d'attaques que nous n'avons prévu de défenses
    if opp_attacks > our_defenses:
        if stamina > 40.0:
            return "Esquiver"  # L'esquive annule totalement mais coûte cher (10)
        else:
            return "Parer"     # La parade réduit les dégâts et coûte moins cher (7.5)
            
    # Sinon, on attaque !
    if queued_actions.count("Attaquer") < 2:
        return "Attaquer"
        
    return "Passer"

def main():
    if len(sys.argv) < 2:
        print(json.dumps({"action": "Passer"}))
        sys.exit(0)
        
    try:
        # Lire le JSON passé en paramètre
        raw_state = sys.argv[1]
        state = json.loads(raw_state)
        
        # Prendre la décision
        action = make_decision(state)
        
        # Répondre sur stdout au format JSON
        print(json.dumps({"action": action}))
    except Exception as e:
        # En cas d'erreur, on retourne "Passer" pour éviter de bloquer la simulation
        print(json.dumps({"action": "Passer"}))

if __name__ == "__main__":
    main()
```
