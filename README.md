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
