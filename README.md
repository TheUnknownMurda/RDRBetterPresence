# RDR Better Presence

Discord Rich Presence détaillée pour **Red Dead Redemption** (port PC 2024), sous forme de plugin
[RedHook](https://www.nexusmods.com/reddeadredemption/mods/192) (`.red`).

Ce que Discord affiche, mis à jour en temps réel depuis le jeu :

| Ligne | Contenu | Source (natives RedHook) |
|---|---|---|
| Activité | **Mission en cours (titre)**, mission d'inconnu, duel, mini-jeu (+ lieu) ; sinon Exploration libre, En fusillade, Dead Eye, À cheval / mule / taureau / bison, Train, Diligence, Lasso, Cinématique, **En pause**, Mort, Ivre, À l'intérieur… + PV % + prime « Wanted $x » | Journal du jeu (`GET_JOURNAL_ENTRY_IN_LIST`, voir plus bas), `IS_PLAYER_IN_COMBAT`, `IS_PLAYER_DEADEYE`, `IS_ACTOR_RIDING`, `IS_ACTOR_ON_TRAIN`, `IS_MINIGAME_RUNNING`, `GET_ACTOR_HEALTH`, stat 222 (prime)… |
| Lieu | Région · lieu le plus proche · heure du jeu · météo | Table de 29 repères (x, z) → région, `GET_TIME_OF_DAY`, `GET_WEATHER` |
| Grande image | Carte de la région (`region_<slug>`), survol : personnage (John / Jack) + tenue | `GET_ACTOR_ENUM`, `GET_CURRENT_ACTOR_ENUM_VARIATION` |
| Petite image | Icône d'activité ou catégorie d'arme, survol : nom de l'arme | `GET_WEAPON_IN_HAND`, `GET_WEAPON_DISPLAY_NAME` |
| Timer | Temps de jeu de la session | — |

> **Pourquoi pas UE4SS ?** Le port PC tourne sur le moteur **RAGE** de Rockstar (archives `.rpf`),
> pas sur Unreal Engine. UE4SS ne peut pas fonctionner. RedHook est le ScriptHook adapté.

### Ce que l'on a appris sur le jeu (vérifié en jeu, build 1.0.42)

- Le monde est **Y-up** : X croît vers l'est, Z vers le sud, Y est l'altitude. `GET_DISTRICTS_NAME` renvoie toujours
  `swall` (un seul district en solo) — les régions viennent d'une table de repères avec leurs coordonnées.
- **Mission en cours** : au démarrage d'une mission, le jeu ajoute à la **liste 0 du journal** une entrée de type 1
  dont le handle vaut `STRING_TO_HASH("miss<N>_short")` ; `<N>` est l'identifiant d'interface de la mission
  (`miss12` = « Exhuming and Other Fine Hobbies »). Elle disparaît à la fin. Les missions disponibles sont en
  liste 1 sous `hash("miss<N>")`. `_IS_ANY_NAMED_SCRIPT_RUNNING` ne voit que les scripts enfants de l'appelant
  et `DOES_SCRIPT_EXIST` teste l'existence du fichier : inutilisables.
- **Pause** : la VM de scripts est gelée pendant la pause, donc la fibre RedHook ne tourne plus ; le plugin
  détecte la pause quand son battement de cœur s'arrête (`IS_GAME_PAUSED` ne bouge pas).
- Les natives déclarées `int` par le SDK renvoient parfois des **floats** (`GET_PLAYER_DEADEYE_POINTS`, les stats
  SAG). La prime est le stat 222 ; l'argent n'a pas été trouvé (stat 0 ≠ argent).
- `Print` de RedHook **plante le jeu** sur certains messages longs ou riches en crochets : les lignes verbeuses
  vont uniquement dans le fichier log.

## Installation (joueur)

1. Installer [RedHook v0.8](https://github.com/K3rhos/RedHookSDK/releases) : `winmm.dll`, `RedHook.dll`,
   `RedHook.ini` à la racine du jeu (à côté de `RDR.exe`). Nécessite le VC++ Redistributable x64.
2. Copier `RDRBetterPresence.red`, `RDRBetterPresence.ini` et `RDRBetterPresence.regions.ini` à la racine du jeu.
3. Mettre votre *Application ID* Discord dans `RDRBetterPresence.ini` (`ClientId=`).
4. Lancer Discord, puis le jeu.

Le plugin écrit `RDRBetterPresence.log` à la racine du jeu et parle aussi dans la console RedHook (F8).
Commandes utiles dans la console (sans guillemets) : `reload RDRBetterPresence` (relit le `.ini`),
`unload RDRBetterPresence`, `load RDRBetterPresence`.

## Images (Discord Developer Portal)

Dans votre application Discord → *Rich Presence* → *Art Assets*, ajoutez des images avec ces clés
(toutes optionnelles, Discord ignore les clés absentes) :

- `rdr_logo` — image par défaut (`LargeImageDefault` dans le `.ini`, accepte aussi une URL https).
- `region_<slug>` — une par région : `region_cholla_springs`, `region_rio_bravo`, `region_gaptooth_ridge`,
  `region_hennigan_s_stead`, `region_punta_orgullo`, `region_perdido`, `region_diez_coronas`,
  `region_tall_trees`, `region_great_plains`.
- Activités : `mission`, `stranger`, `duel`, `paused`, `cutscene`, `dead`, `minigame`, `lasso`, `deadeye`, `train`,
  `stagecoach`, `horse`.
- Armes : `weapon_pistol`, `weapon_revolver`, `weapon_repeater`, `weapon_rifle`, `weapon_shotgun`,
  `weapon_sniper`, `weapon_lasso`, `weapon_melee`, `weapon_explosive`, `weapon_thrown`, `weapon_turret`,
  `weapon_cannon`, `weapon_bow`.

## Configuration

`RDRBetterPresence.ini` (lu au chargement du plugin) : langue (`fr`/`en`), éléments affichés, intervalle de
mise à jour, boutons, niveau de log. `RDRBetterPresence.regions.ini` : rectangles de coordonnées (x, z) pour
nommer des lieux supplémentaires (prioritaires sur la table intégrée) — activez `ShowCoordinates=true` pour
relever les coordonnées.

## Compilation (développeur)

Prérequis : Visual Studio 2022 Build Tools avec la charge de travail « Développement Desktop en C++ ».

```powershell
.\build.ps1                 # build Release + copie du .red et des .ini (si absents) dans le dossier du jeu
.\build.ps1 -NoDeploy       # build seulement
.\build.ps1 -GameDir "D:\Jeux\Red Dead Redemption"
.\tests\run_smoke.ps1       # teste la couche IPC Discord + le PresenceBuilder sans le jeu
```

### Architecture

```
src/
├── dllmain.cpp             DllMain → Plugin::Initialize / Shutdown
├── Plugin.cpp              orchestration : fibre de script RedHook + thread Discord
├── Config.*                lecture du .ini
├── Log.*                   log fichier + console F8 (thread-safe)
├── discord/DiscordIPC.*    protocole RPC Discord sur named pipe (handshake, SET_ACTIVITY, PING/PONG)
├── discord/Json.h          mini-sérialiseur JSON
├── game/GameState.*        échantillonnage de l'état du jeu via les natives (fibre de script uniquement)
├── game/Regions.*          repères (x, z) + rectangles utilisateur → région, lieu, slug d'asset
├── game/Journal.*          mission en cours via le journal du jeu (hash des libellés miss<N>_short)
└── presence/PresenceBuilder.*, Localization.h   GameSnapshot → Activity (FR / EN)
sdk/                        RedHook SDK vendored (MIT, © K3rhos)
```

Deux threads : la **fibre de script RedHook** appelle les natives toutes les `PollIntervalMs` et publie un
`GameSnapshot` ; le **thread Discord** possède le pipe, reconstruit l'`Activity` et n'envoie que si elle a
changé, au plus une fois par `UpdateIntervalMs`. Les natives ne sont jamais appelées hors de la fibre, le
pipe jamais touché depuis la fibre.

## Crédits

- [K3rhos](https://github.com/K3rhos) — RedHook, RedHookSDK, RDR-PC-Natives-DB
- [EvilBlunt](https://github.com/EvilBlunt/RDR-Strings-and-Enums) — enums tenues / météo
