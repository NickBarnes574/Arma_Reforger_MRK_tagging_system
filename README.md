# Arma Reforger MRK Tagging System

A binocular-driven tagging system inspired by Sniper Elite 5.

## Current behavior

- Initializes automatically from the local `SCR_PlayerController`.
- Only acquires targets while binoculars are raised and in ADS.
- Uses a long-range camera-center trace for target acquisition.
- Resolves child entities (turrets, weapons, cargo sections, etc.) back to a taggable parent entity.
- Automatically tags a valid target after a short dwell time.
- Shows a circular acquisition progress indicator.
- Creates persistent HUD markers for tagged infantry and vehicles.
- Updates infantry alert-state colors.
- Removes markers when infantry die or vehicles are destroyed.

## Script responsibilities

- `MRK_PlayerController.c` - creates the local tagging manager once the local player controller is available.
- `MRK_TagManager.c` - coordinates binocular acquisition, tagging, reticle UI state, and tagged-target updates.
- `MRK_MarkerUIService.c` - creates, positions, colors, and destroys persistent HUD markers.
- `MRK_TargetClassifier.c` - classifies taggable entities, resolves child hits to parent targets, and selects marker presentation data.
- `MRK_TargetStateService.c` - reads alert state and determines when tagged entities should be removed.
- `MRK_TaggedTarget.c` - data object for one tagged target.
- `MRK_TagTypes.c` - shared tag/alert enums.
- `MRK_Constants.c` - shared resource names and tuning constants.

## Next planned work

- Scope-aware marker projection using the active optic/PIP camera rather than manual FOV projection math.
- Multiplayer tag synchronization.
- Map markers and temporary binocular point markers.

## Tag persistence policy

The current default HUD policy is centralized through `MRK_Settings.c`:

- Enemy infantry and occupied enemy vehicles persist until dead/destroyed.
- Friendly tags expire after 8 seconds.
- Civilian/neutral tags expire after 5 seconds.
- Unoccupied vehicles remain logically tagged, but their HUD marker is only shown within 750 meters.
- Civilian/neutral targets stay white and do not use hostile AI alert-state colors.

`MRK_Settings.c` is intentionally kept separate so these values can later be connected to player-facing menu options without rewriting the tag lifecycle logic.


## Distance-based marker fading

HUD markers now stay fully opaque through 300 m, fade smoothly to 35% opacity by 1200 m, and remain at 35% until the global 1500 m HUD display cutoff. Tags remain logically persistent when hidden by distance, so they reappear if the target returns to display range. Unoccupied vehicles still use their stricter 750 m display limit.

## Stronger distance readability

The fade curve now uses distinct distance bands instead of one shallow linear interpolation:

- 0-200 m: 100% opacity
- 200-800 m: fades to 60%
- 800-1300 m: fades to 15%
- 1300-1500 m: fades to 2%
- beyond the maximum display distance: hidden, but still logically tagged

This makes markers encountered at the edge of the HUD range appear very faint instead of popping in around one-third opacity.

## Settings backend

`MRK_GameSettings : ModuleGameSettings` now provides native game-user-settings storage for the tagging system. `MRK_Settings` caches those values for the high-frequency HUD update loops and refreshes when Reforger reports a user-settings change.

The runtime now honors settings for:

- enemy / friendly / civilian / unoccupied-vehicle marker visibility
- maximum HUD marker distance
- distance fading on/off
- minimum distant opacity
- global marker scale
- show markers through PIP optics
- tag confirmation sound
- tag acquisition time
- acquisition forgiveness (low / normal / high)
- friendly and civilian lifetimes
- unoccupied-vehicle display distance

`MRK_Settings.ResetToDefaults()` restores the module defaults and saves them.

### Native Settings-menu UI note

The source ZIP supplied for this revision contains only scripts; it does not contain the addon's Workbench UI/layout resources or its Settings-tab resource/config. Reforger's stock Settings screen binds settings modules to named widgets in a layout, so a visible MRK tab cannot be safely fabricated from this script-only package without the actual UI resource layer. The settings module and runtime wiring are complete; once the full addon project (including UI/config resources) is supplied, the native tab can be added without changing the gameplay code again.
