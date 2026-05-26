# Documentation - Spécification du Protocole TCP pour l'IA

Ce document présente la spécification du protocole de communication TCP utilisé pour connecter des moteurs d'intelligence artificielle externes (tels que des serveurs de modèles de Deep Learning) au **Danjon Simulator**.

---

## 1. Fonctionnement Réseau

Le simulateur fonctionne en tant que **client TCP** et se connecte à un **serveur TCP** externe (configuré via les champs d'hôte et de port dans l'interface de sélection).

À chaque action que le personnage doit choisir :
1. Le simulateur ouvre une connexion TCP vers le serveur.
2. Le simulateur envoie l'état complet du combat sous forme d'une ligne JSON terminée par un caractère de saut de ligne (`\n`).
3. Le serveur TCP doit lire cette ligne, calculer l'action appropriée, et renvoyer la décision sous forme d'une ligne JSON également terminée par un saut de ligne (`\n`).
4. Le simulateur ferme ensuite la connexion TCP.

---

## 2. Format des Messages

### Requête (Simulateur -> Serveur)

La requête est un JSON compact encodé en UTF-8 sur une seule ligne. Elle contient les objets `active_character` et `opponent_character` :

```json
{"active_character":{"blood":32,"force":24,"force_magique":10,"free_actions":2,"magic_reserve":0,"max_physical_reserve":250,"name":"Haru Dahrendorf","physical_reserve":250,"queued_actions":["Attaquer"],"rank":2,"resistance":18.25,"resistance_magique":19,"stade":10,"vitesse":26.25},"opponent_character":{"blood":32,"force":10,"force_magique":10,"free_actions":2,"magic_reserve":0,"max_physical_reserve":32,"name":"Bayleth Myphitic","physical_reserve":32,"queued_actions":[],"rank":1,"resistance":8,"resistance_magique":13.25,"stade":1,"vitesse":8}}
```

### Réponse (Serveur -> Simulateur)

Le serveur doit répondre avec un JSON contenant l'action choisie, encodé en UTF-8, impérativement terminé par un caractère `\n` :

```json
{"action": "Attaquer"}
```

Les actions valides sont :
* `"Attaquer"`
* `"Parer"`
* `"Esquiver"`
* `"Passer"`

---

## 3. Gestion des Timeouts et Erreurs

* Le simulateur applique un **timeout de connexion de 3 secondes**. Si la connexion TCP ne s'établit pas, la décision par défaut est `"Passer"`.
* Le simulateur attend la réponse pendant un **timeout de 10 secondes maximum**. Si aucun message n'est reçu avant ce délai, la connexion est coupée et l'action `"Passer"` est renvoyée par défaut.

---

## 4. Exemple de Serveur TCP en Python

Voici un script Python simple mettant en place un serveur TCP conforme au protocole (disponible en tant que mock dans `scripts/mock_tcp_server.py`) :

```python
import socket
import json
import random

HOST = '127.0.0.1'
PORT = 8080

def handle_decision(state):
    # Logique d'évaluation simple
    active = state["active_character"]
    stamina = active["physical_reserve"]
    queued = active["queued_actions"]
    
    if stamina <= 15.0:
        return "Passer"
    
    if len(queued) < 2:
        return random.choice(["Attaquer", "Parer", "Esquiver"])
    return "Passer"

def run_server():
    # Création du socket TCP
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((HOST, PORT))
        server.listen(1)
        print(f"Serveur TCP d'IA à l'écoute sur {HOST}:{PORT}...")
        
        while True:
            conn, addr = server.accept()
            with conn:
                print(f"Connexion établie par {addr}")
                
                # Lire le flux jusqu'à recevoir le saut de ligne
                data = b""
                while b"\n" not in data:
                    chunk = conn.recv(1024)
                    if not chunk:
                        break
                    data += chunk
                
                if not data:
                    continue
                
                try:
                    # Décoder et parser l'état du simulateur
                    line = data.decode('utf-8').strip()
                    state = json.loads(line)
                    
                    # Déterminer l'action
                    chosen_action = handle_decision(state)
                    print(f"Action décidée pour {state['active_character']['name']} : {chosen_action}")
                    
                    # Envoyer la réponse formatée avec \n
                    response = json.dumps({"action": chosen_action}) + "\n"
                    conn.sendall(response.encode('utf-8'))
                except Exception as e:
                    print(f"Erreur lors du traitement : {e}")
                    # Envoi d'une réponse de secours
                    conn.sendall(b'{"action": "Passer"}\n')

if __name__ == "__main__":
    run_server()
```
