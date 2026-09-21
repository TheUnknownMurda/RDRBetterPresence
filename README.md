# RDR Better Presence

Discord Rich Presence détaillée pour **Red Dead Redemption** (port PC 2024), sous forme de plugin
[RedHook](https://www.nexusmods.com/reddeadredemption/mods/192) (`.red`).

Ce que Discord affiche, mis à jour en temps réel depuis le jeu :

| Ligne | Contenu | Source (natives RedHook) |
|---|---|---|
| Activité | Exploration libre, En fusillade, Dead Eye activé, À cheval / mule / taureau / bison, À bord du train, Conduit une diligence, Capture au lasso, Mini-jeu, Cinématique, En pause, Mort, Ivre, À l'intérieur… + PV en % | `IS_PLAYER_IN_COMBAT`, `IS_PLAYER_DEADEYE`, `IS_ACTOR_RIDING`, `IS_ACTOR_ON_TRAIN`, `IS_MINIGAME_RUNNING`, `IS_GAME_PAUSED`, `GET_ACTOR_HEALTH`… |
| Lieu | Région · heure du jeu · météo | `GET_DISTRICTS_NAME`, `GET_TIME_OF_DAY`, `GET_WEATHER` |
| Grande image | Carte de la région (`region_<slug>`), survol : personnage (John / Jack) + tenue | `GET_ACTOR_ENUM`, `GET_CURRENT_ACTOR_ENUM_VARIATION` |
| Petite image | Icône d'activité ou catégorie d'arme, survol : nom de l'arme | `GET_WEAPON_IN_HAND`, `GET_WEAPON_DISPLAY_NAME` |
| Timer | Temps de jeu de la session | — |

> **Pourquoi pas UE4SS ?** Le port PC tourne sur le moteur **RAGE** de Rockstar (archives `.rpf`),
> pas sur Unreal Engine. UE4SS ne peut pas fonctionner. RedHook est le ScriptHook adapté.

## Installation (joueur)

1. Installer [RedHook v0.8](https://github.com/K3rhos/RedHookSDK/releases) : `winmm.dll`, `RedHook.dll`,
   `RedHook.ini` à la racine du jeu (à côté de `RDR.exe`). Nécessite le VC++ Redistributable x64.
2. Copier `RDRBetterPresence.red`, `RDRBetterPresence.ini` et `RDRBetterPresence.regions.ini` à la racine du jeu.
3. Mettre votre *Application ID* Discord dans `RDRBetterPresence.ini` (`ClientId=`).
4. Lancer Discord, puis le jeu.

Le plugin écrit `RDRBetterPresence.log` à la racine du jeu et parle aussi dans la console RedHook (F8).
Commandes utiles dans la console : `reload "RDRBetterPresence"` (relit le `.ini`), `unload "RDRBetterPresence"`.

## Images (Discord Developer Portal)

Dans votre application Discord → *Rich Presence* → *Art Assets*, ajoutez des images avec ces clés
(toutes optionnelles, Discord ignore les clés absentes) :

- `rdr_logo` — image par défaut (`LargeImageDefault` dans le `.ini`, accepte aussi une URL https).
- `region_<slug>` — une par région/lieu : `region_cholla_springs`, `region_rio_bravo`, `region_gaptooth_ridge`,
  `region_hennigan_s_stead`, `region_diez_coronas`, `region_punta_orgullosa`, `region_perdido`,
  `region_tall_trees`, `region_great_plains`… Le slug est le nom retourné par le jeu en minuscules ASCII,
  `_` entre les mots (voir le log : `District changed: ... -> 'Nom'`).
- Activités : `paused`, `cutscene`, `dead`, `minigame`, `lasso`, `deadeye`, `train`, `stagecoach`, `horse`.
- Armes : `weapon_pistol`, `weapon_revolver`, `weapon_repeater`, `weapon_rifle`, `weapon_shotgun`,
  `weapon_sniper`, `weapon_lasso`, `weapon_melee`, `weapon_explosive`, `weapon_thrown`, `weapon_turret`,
  `weapon_cannon`, `weapon_bow`.

## Configuration

`RDRBetterPresence.ini` (lu au chargement du plugin) : langue (`fr`/`en`), éléments affichés, intervalle de
mise à jour, boutons, niveau de log. `RDRBetterPresence.regions.ini` : rectangles de coordonnées pour nommer
villes et lieux précisément (prioritaires sur le district du jeu) — activez `ShowCoordinates=true` pour
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
├── game/Regions.*          district du jeu + rectangles utilisateur → nom + slug d'asset
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
