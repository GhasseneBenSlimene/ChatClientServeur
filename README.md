# Mise en place d'une application de chat client/serveur

## Description

Ce projet consiste en la réalisation d'une application de chat client/serveur en C permettant aux utilisateurs de se connecter et d'échanger des messages ou des fichiers entre eux en temps réel.

## Structure du code

Les fichiers client.c et sever.c sont les fichiers principaux. server.c utilise de plus les fonctions écrites dans channel.c et listclients.c
Le fichier common.c est partagé par les server.c et client.c, il contient une fonction pour envoyer et recevoir convenablement des données. Il y a également une fonction utilisée pour tester les retours des fonctions, et en cas d'erreurs le programme se ferme.

Ce projet est organisé en plusieurs dossiers et fichiers clés :

- `src/` : Contient les sources du projet, notamment les fichiers `.c` pour le client et le serveur.
- `include/` : Dossier pour les fichiers d'en-tête `.h` qui contiennent les déclarations des fonctions et les définitions des types.
- `obj/` : Ce dossier est utilisé pour stocker les fichiers objets `.o` qui sont générés lors de la compilation.
- `recieved_files/` : Dossier pour stocker les fichiers reçus via la fonctionnalité de transfert de fichiers de l'application.
- `Makefile` : Fichier utilisé par l'utilitaire `make` pour automatiser la compilation du projet.
- `client` et `server` : Exécutables générés après la compilation.
- `README.md` : Ce fichier, contenant des informations et des instructions sur le projet.


## Compilation
1. Compilez le code avec la commande `make`.
2. Lancez le serveur avec `./server <port>`.
3. Lancez un client avec `./client 127.0.0.1 <port>`.

L'adresse 127.0.0.1 est choisie ici à titre d'exemple pour une utilisation locale. Pour une utilisation en réseau, utilisez une adresse IPv4 appropriée. Le port peut être, par exemple, 8080.

## Fonctionnalités

Les fonctionnalités de base sont les suivantes :

1. **Choix de pseudo** : À la connexion, les utilisateurs doivent choisir un pseudo avec `/nick <pseudo>`. Le pseudo peut être modifié ultérieurement.
2. **Liste des utilisateurs** : Pour voir les utilisateurs connectés, utilisez `/who`.
3. **Informations sur un utilisateur** : Pour obtenir des informations sur un utilisateur, utilisez `/whois <user>`.
4. **Envoi de message privé** : Pour envoyer un message privé, utilisez `/msg <user> <message>`.
5. **Envoi de message à tous** : Pour envoyer un message à tous les utilisateurs, utilisez `/msgall <message>`.
6. **Envoi de fichier** : Pour envoyer un fichier à un autre utilisateur, utilisez `/send <user> <nom_du_fichier>`. L'utilisateur récepteur reçoit une demande d'acceptation.
7. **Création de salon** : Pour créer un salon, utilisez `/create <nom_du_salon>`.
8. **Liste des salons** : Pour voir la liste des salons, utilisez `/channel_list`.
9. **Rejoindre un salon** : Pour rejoindre un salon, utilisez `/join <nom_du_salon>`.
10. **Quitter un salon** : Pour quitter un salon, utilisez `/quit <nom_du_salon>`.
11. **Déconnexion** : Pour se déconnecter du serveur, utilisez `/quit`.

Fonctionnalités supplémentaires :
1. **Informations sur un salon** : Pour obtenir des informations sur un salon, utilisez `/channel_info <nom_du_salon>`.
2. **Renommer un salon** : Le créateur d'un salon peut le renommer avec `/channel_name <nouveau_nom_du_salon>`.
3. **Envoi de fichier à un salon** : Pour envoyer un fichier à tous les membres d'un salon, utilisez `/sendall <nom_du_fichier>`. Les membres du salon recevront une demande d'acceptation.