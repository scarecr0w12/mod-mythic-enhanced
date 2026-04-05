# Mythic Enhanced Improvement Plan

## Purpose

This document captures the implementation plan for evolving `mod-mythic-enhanced`
into a more complete Mythic+ style system for AzerothCore.

The goals are to:

- add a monthly-reset leaderboard system
- introduce clearer affix rotations and new affixes
- improve player-facing progression and replayability
- preserve compatibility with the existing module structure
- implement changes in phases that are safe to test and ship

## Current State

The module already provides a strong base:

- keystone item flow
- dungeon activation and timer handling
- death penalties
- dungeon completion tracking
- rewards by mythic level
- affix factory system with custom affixes
- snapshot-based standings UI through NPC gossip
- run history persistence in the characters database

### Current limitations

- no true seasonal or monthly leaderboard table
- no monthly reset mechanism
- no player rating or score aggregation
- standings are sorted mostly by run completion time only
- no affix rotation framework tied to week or month
- random affixes are generated on server restart instead of by season/rotation
- no seasonal rewards or end-of-cycle recognition

### Verified codebase audit (April 2026)

The current module implementation confirms the following extension points and
constraints:

- `src/mythic_plus.cpp` and `src/mythic_plus.h` already persist active dungeon
   state in `mythic_plus_dungeon`, store boss-by-boss snapshots in
   `mythic_plus_dungeon_snapshot`, and sort standings by `totalTime` only via
   `MythicPlus::SortSnapshots`.
- `src/mythic_plus_unitscript.cpp` is the current finalization hook. On final
  boss death it:
  - marks the run complete,
  - gives rewards if the timer was beaten,
  - saves the finished dungeon state, and
  - inserts snapshot rows for each player present in the map.
- `src/mythic_plus_worldscript.cpp` loads module data at startup and runs the
  periodic snapshot refresh loop. This is the best existing place to:
  - ensure the active season exists on startup,
  - poll for UTC month rollover,
  - trigger lightweight recurring maintenance.
- `src/mythic_plus_npc_support.cpp` already exposes standings, per-run detail
   browsing, level browsing, and random-affix pages through gossip. It should be
   extended rather than replaced.
- `src/mythic_plus_commandscript.cpp` currently exposes only `.mythic info`
  and `.mythic reload`, leaving room for leaderboard and season commands.
- `src/mythic_affix.h` already has a clean affix enum/factory model, but the
  current affix roster is still custom and random-affix selection is tied to
  startup-time generation.
- Current base SQL files are already separated cleanly:
  - characters DB: `b_mythic_plus_dungeon.sql`,
    `b_mythic_plus_dungeon_snapshot.sql`, `b_mythic_plus_char_level.sql`,
    `b_mythic_plus_keystone_timer.sql`
  - world DB: `b_mythic_plus_level.sql`, `b_mythic_plus_affix.sql`,
    `b_mythic_plus_capable_dungeon.sql`, `b_mythic_plus_level_rewards.sql`,
    `b_mythic_plus_map_scale.sql`, `b_mythic_plus_spell_override.sql`

This means the leaderboard system should be added as a new layer on top of the
existing snapshot audit trail, not as a replacement for it.

## Design Goals

### Primary goals

1. **Monthly leaderboard reset**
   - all competitive rankings reset automatically each month
   - historical records remain queryable

2. **Meaningful scoring**
   - higher keys should matter more than faster low keys
   - timed runs should matter more than overtime runs
   - deaths should slightly reduce score without dominating it

3. **Affix identity**
   - affixes should create recognizable gameplay patterns
   - affixes should be WotLK-friendly and implementable with AzerothCore scripting

4. **Reuse existing systems**
   - extend snapshots and current NPC standings UI instead of replacing them
   - avoid duplicate systems when existing structures can be upgraded

5. **Incremental rollout**
   - ship in phases with testing after each milestone

### Non-goals for initial release

- exact retail UI parity
- exact Blizzard scoring formula parity
- region-wide ladder infrastructure
- cross-realm grouping support
- highly complex seasonal affixes in phase 1

## Research Summary

The Mythic+ model worth emulating is the following:

- keystones activate progressively harder dungeon runs
- timer performance matters
- death penalties matter
- affixes rotate and change the feel of the dungeon
- players are ranked by season, not permanently
- score should reward both difficulty and execution

For this module, the best approach is not to reproduce retail exactly, but to
adapt the **principles** to a WotLK private-server environment.

## Proposed Feature Set

## 1. Monthly Leaderboard System

### Implementation status (April 2026)

Completed in code:

- monthly season persistence via `mythic_plus_season`
- monthly leaderboard persistence via `mythic_plus_leaderboard`
- startup and periodic active-season checks in `mythic_plus_worldscript.cpp`
- score calculation and leaderboard replacement logic in `mythic_plus.cpp`
- final-boss leaderboard submission in `mythic_plus_unitscript.cpp`
- command support for season, rating, leaderboard, map leaderboard,
   map list, season history, archived season lookup, and manual seasonal reward
   distribution
- NPC support for current season info, top players, per-dungeon monthly
   leaderboard browsing, and archived season browsing
- world DB rotation metadata via `mythic_plus_rotation`
- deterministic rotating-affix assignment with season/window fallback logic
- seasonal reward definitions via `mythic_plus_season_reward`
- seasonal reward audit logging via `mythic_plus_season_reward_log`
- season-end reward mail distribution and public winner announcements
- second tactical affix support via `Sanguine`

Still intentionally pending:

- advanced seasonal affixes beyond the current tactical set

### Overview

Add a real leaderboard system on top of the existing run snapshot system.

The existing snapshot system remains useful for:

- run history
- boss-by-boss breakdowns
- dungeon replay browsing
- debugging and analytics

The new leaderboard layer will provide:

- best runs per player per dungeon per month
- top runs per dungeon for the active month
- overall player score for the active month
- archived season/month history

### Data model

Add two new tables in `data/sql/db-characters/`.

#### `mythic_plus_season`

Suggested fields:

- `id`
- `year`
- `month`
- `start_unix`
- `end_unix`
- `is_active`
- `label`

Purpose:

- tracks the currently active monthly season
- supports archive/history views
- avoids hardcoding season logic into code only

#### `mythic_plus_leaderboard`

Suggested fields:

- `season_id`
- `char_guid`
- `char_name`
- `map_id`
- `difficulty`
- `mythic_level`
- `best_time`
- `deaths`
- `penalty_seconds`
- `completed_in_time`
- `score`
- `group_members`
- `last_update`

Purpose:

- stores one best competitive entry per player per dungeon per season
- enables per-dungeon and overall score calculations

### Reset behavior

Monthly reset should be based on **UTC month boundaries**.

Recommended logic:

- on world startup, ensure an active season exists for current UTC month
- on periodic world update or daily check, detect month change
- when month changes:
  - mark previous season inactive
  - create new season row
  - use the new season for all subsequent leaderboard submissions
- do not delete old seasons immediately; retain them as archives

### Submission rules

When a run ends:

- if the dungeon was completed, calculate score
- submit an entry for each eligible group member
- replace an existing monthly entry only if the new run is better

Recommended comparison order:

1. higher score wins
2. if score ties, higher mythic level wins
3. if level ties, lower completion time wins
4. if time ties, lower deaths wins

### Eligibility rules

Recommended phase-1 rules:

- player must be in the finishing group
- player must be alive or still map-present when the final boss dies
- player must meet existing module eligibility rules
- optional later rule: minimum party size or anti-boost checks

## 2. Scoring Model

### Requirements

The score must:

- reward pushing higher keys
- reward beating the timer
- still allow overtime completions to be recorded
- be simple enough to explain to players
- be stable enough to support hidden tests and future balancing

### Recommended formula

Use a server-friendly scoring model rather than retail-perfect scoring.

Suggested score design:

- `base score = mythic level * 100`
- timed bonus:
  - `+30` if more than 20% of timer remains
  - `+20` if completed in time
  - `+5` if overtime but within 20% over timer
  - `0` if more than 20% over timer
- death adjustment:
  - `-1` to `-2` per death, capped at a reasonable max penalty

Alternative formula if more granularity is desired:

- `base = mythic level * 100`
- `timer bonus = percentage-based bonus capped below next key tier`
- `death penalty score = min(deaths * 2, 20)`

### Recommendation

Start with the simpler bucketed score model first. It is easier to:

- explain to players
- test
- debug
- rebalance later

## 3. Affix Expansion

### Current state

The module already supports affixes via:

- `MythicAffixType`
- `MythicAffix::AffixFactory(...)`
- static, periodic, damage, and map-level hooks

This is a good extension point and should be preserved.

### Recommended phase-1 affixes

#### Fortified

Effect:

- non-boss enemies gain more health and damage

Why first:

- iconic Mythic+ affix
- easy to implement using existing scaling patterns
- low scripting risk

#### Tyrannical

Effect:

- bosses gain more health and damage

Why first:

- iconic pairing with Fortified
- easy to communicate to players
- fits current architecture well

#### Bolstering

Effect:

- when a non-boss enemy dies, nearby enemies gain a stacking buff

Why it matters:

- encourages kill coordination
- changes pack strategy without exotic scripting

Status:

- implemented for the current module as the first tactical post-foundation affix
- current implementation buffs nearby non-boss enemies when trash dies

#### Sanguine

Effect:

- non-boss enemies leave damaging/healing pools on death

Why it matters:

- strongly changes positioning
- recognizable Mythic+ gameplay pattern

### Recommended phase-2 affixes

- Bursting
- Volcanic
- Spiteful
- seasonal custom affix

### Affixes to postpone

These are better deferred until the system is mature:

- Prideful-like affixes
- Encrypted-style affixes
- Afflicted/Incorporeal-style affixes
- route-dependent or puzzle-heavy seasonal affixes

## 4. Rotation System

### Problem

Random affixes currently depend on server restarts, which is not ideal for a
competitive monthly system.

### Proposal

Add a world DB table for rotation metadata, for example:

#### `mythic_plus_rotation`

Suggested fields:

- `rotation_type` (`weekly`, `monthly`, `seasonal`)
- `start_unix`
- `end_unix`
- `affix_slot`
- `affix_type`
- `val1`
- `val2`
- `enabled`

### Behavior

- weekly rotation controls the main rotating affixes
- monthly rotation may control the featured seasonal affix
- randomization should be deterministic per active rotation window, not per restart

## 5. Player-Facing Features

### NPC menu improvements

Extend the existing standings UI in `mythic_plus_npc_support.cpp` to include:

- current season label and reset timer
- top monthly runs for a selected dungeon
- top monthly players overall
- personal best this month
- current score summary

### Command additions

Extend `mythic_plus_commandscript.cpp` with commands such as:

- `.mythic leaderboard`
- `.mythic leaderboard map <id>`
- `.mythic rating`
- `.mythic season`

### Optional future UI additions

- gossip filters by role/class
- historical season browser
- personal run history summary

## 6. Seasonal Rewards

### Proposed rewards

At monthly reset or by command distribution:

- title for rank 1 overall
- token/currency reward for top N
- cosmetic or vanity item for top N
- public announcement for winners

### Reward rollout recommendation

Do not implement distribution first.

Instead:

1. build the leaderboard
2. validate ranking accuracy
3. then add reward distribution logic

This reduces the risk of rewarding bad data.

## 7. Technical Implementation Plan

### Code areas to change

#### Core system

Files:

- `src/mythic_plus.h`
- `src/mythic_plus.cpp`

Add:

- season structs
- leaderboard structs
- score calculation helpers
- active season lookup/creation
- leaderboard submit/query methods

#### Dungeon completion hooks

Primary files:

- `src/mythic_plus_unitscript.cpp`
- `src/mythic_plus_worldscript.cpp`

Add:

- final run submission trigger inside the existing final-boss completion flow
- completion validation and eligible-player filtering at run finalization time
- score generation at the moment the run is finalized
- UTC month rollover check inside the world update loop

#### Affixes

Files:

- `src/mythic_affix.h`
- `src/mythic_affix.cpp`

Add:

- new affix enum values
- new affix classes
- factory support
- optional DB-configurable parameters

#### Player-facing views

Files:

- `src/mythic_plus_npc_support.cpp`
- `src/mythic_plus_commandscript.cpp`

Add:

- leaderboard pages
- season timer display
- rating and ranking commands

#### SQL

Directories:

- `data/sql/db-characters/base/`
- `data/sql/db-characters/updates/`
- `data/sql/db-world/base/`
- `data/sql/db-world/updates/`

Add:

- new base tables for seasons and leaderboard, following the existing
   `b_*.sql` naming pattern inside `base/`
- update files for existing installations, following the existing
   `u_mp_YYYY_MM_DD_NN.sql` naming pattern inside `updates/`
- optional rotation tables in world DB

Note:

- AzerothCore auto-discovers module SQL recursively under
   `modules/mod-mythic-enhanced/data/sql/<db-name>/`, so the module now uses a
   conventional `base/` + `updates/` split to match common AzerothCore module
   layout and keep filenames unique per database.

## 8. Phased Delivery

### Phase 1: Seasonal foundation

Deliver:

- season table
- leaderboard table
- active season detection
- score calculation
- leaderboard submission on completion
- NPC and command read-only leaderboard views

Acceptance criteria:

- completed runs generate leaderboard entries
- the same player’s worse run does not overwrite a better one
- month change switches to a new season automatically
- archived season data remains intact

### Phase 2: Core affix refresh

Deliver:

- Fortified
- Tyrannical
- one additional mechanic affix (`Bolstering` or `Sanguine`)
- affix rotation table or deterministic seasonal assignment

Acceptance criteria:

- affixes load cleanly from DB
- affix behavior is visible and testable in dungeon runs
- no duplicate or restart-dependent randomness affects competition unfairly

### Current implementation progress

Completed already:

- Phase 1 seasonal foundation
- Fortified
- Tyrannical
- one tactical affix: Bolstering
- a second tactical affix: Sanguine
- archive-specific leaderboard browsing by selected season
- rotation metadata table and deterministic affix assignment
- seasonal reward support with mail delivery and reward audit logging
- command-side and NPC-side season history/archive visibility

Next recommended work:

- validate and tune score and affix values through gameplay testing
- consider richer reward types such as titles or cosmetics
- expand advanced seasonal affix coverage beyond the current tactical set

### Phase 3: Competitive polish

Deliver:

- overall player monthly rating
- personal best summary
- seasonal reward support
- improved browsing/filtering UI

Acceptance criteria:

- players can inspect top runs and top players easily
- monthly winners can be identified without manual SQL work

### Phase 4: Advanced seasonal content

Deliver:

- one custom seasonal affix
- richer reward logic
- archive browser for past months

Acceptance criteria:

- seasonal content changes run behavior in a visible way
- historical seasons remain queryable

## 9. Testing Plan

### Database tests

Validate:

- season creation
- season rollover
- leaderboard upsert behavior
- archive preservation

### Gameplay tests

Validate:

- timed completion submission
- overtime submission
- death-heavy run scoring
- boss kill completion edge cases
- group completion and player eligibility

### Regression tests

Validate that existing systems still work:

- keystone acquisition
- level selection
- run activation
- rewards
- snapshot history pages
- existing affixes

### Performance checks

Review:

- leaderboard query cost
- snapshot growth over time
- possible need for indexes on season/map/player fields

## 10. Risks and Mitigations

### Risk: snapshot and leaderboard disagreement

Mitigation:

- generate leaderboard entries only from finalized completion data
- keep snapshot history as audit/debug support

### Risk: unfair scoring

Mitigation:

- start with a transparent simple score model
- tune after live testing

### Risk: restart-based randomness affecting competition

Mitigation:

- move toward DB-defined rotation windows

### Risk: reward abuse or edge-case exploits

Mitigation:

- delay automated reward payouts until ranking integrity is verified

### Risk: database growth

Mitigation:

- retain snapshots for history, but add indexes and consider future retention rules

## 11. Recommended Implementation Order

The recommended practical order is:

1. monthly season tracking
2. leaderboard persistence and score calculation
3. leaderboard UI/commands
4. Fortified and Tyrannical
5. one tactical affix (`Bolstering` or `Sanguine`)
6. seasonal rewards
7. advanced seasonal affixes

## 12. First Milestone Definition

The first milestone should ship the following:

- monthly reset season support
- leaderboard submission and ranking
- top dungeon runs view
- overall top players view
- current season info view
- Fortified
- Tyrannical

This is the smallest feature set that turns the module into a recognizable,
replayable Mythic+ progression system.

### Recommended implementation slices for milestone 1

To keep the first milestone safe and reviewable, split it into the following
slices:

1. **Season persistence**
   - add `mythic_plus_season`
   - load or create the active UTC month on startup
   - add world-update rollover checks

2. **Leaderboard persistence and scoring**
   - add `mythic_plus_leaderboard`
   - add score calculation helpers in `mythic_plus.cpp`
   - implement best-entry replacement rules

3. **Run finalization integration**
   - submit leaderboard rows from the existing final-boss path in
     `mythic_plus_unitscript.cpp`
   - ensure overtime runs are still recorded even when not rewarded
   - keep snapshots as the detailed audit trail

4. **Read-only player surfaces**
   - extend NPC gossip with current season info, top runs, and top players
   - extend commands with `.mythic leaderboard`, `.mythic rating`, and
     `.mythic season`

5. **Affix refresh**
   - add Fortified and Tyrannical as non-random affixes
   - keep rotation simple for the first pass if needed, but remove
     restart-dependent competitive randomness from any leaderboard-critical
     presentation

Each slice should compile independently and be testable before moving to the
next one.

### Immediate working assumptions for milestone 1

Unless design decisions change later, the safest defaults for the first pass
are:

- overtime completions **do** appear on the leaderboard, but score lower than
  in-time completions
- standings remain per-character, not per-party
- season boundaries use UTC month transitions only
- snapshot history remains the source of truth for debugging disputed rankings
- seasonal rewards stay disabled until leaderboard correctness is verified

## 13. Open Questions

These decisions should be settled before implementation begins:

1. Should overtime completions still appear on the leaderboard or only in history?
2. Should solo and under-sized group runs count for monthly rankings?
3. Should rewards go only to top overall players or also top-per-dungeon players?
4. Should the first release use weekly affix rotation or a static monthly set?
5. Should random affixes remain at all once rotation tables exist?
6. Do we want the score visible to players immediately in NPC gossip and chat?

### Questions resolved by current implementation

- overtime completions currently remain on the leaderboard
- score is now visible in both chat leaderboard output and NPC leaderboard rows
- seasons are archived rather than deleted, and recent season history is now
   visible by command and NPC archive browsing
- deterministic random-affix selection now uses the active season/rotation
   window rather than server restarts
- seasonal rewards can now be distributed automatically on rollover or manually
   by command

## 14. Recommendation Summary

Build the upgrade around the module’s existing strengths:

- keep snapshots
- add a real season/leaderboard layer
- use simple transparent scoring first
- add iconic affixes before ambitious seasonal mechanics
- ship in phases with gameplay validation after each phase

This approach minimizes risk, keeps implementation understandable, and provides a
clear path from the current module to a more complete Mythic+ style feature set.

## 15. Immediate Backlog To Execute

The next concrete work items to implement from this document should be:

1. validate and tune score and affix values through gameplay testing
2. add richer reward types such as titles or cosmetics if desired
3. add additional tactical or seasonal affixes beyond `Sanguine`
4. consider stricter leaderboard eligibility and anti-boost rules
5. expand archive browsing with deeper historical filtering if needed

Items previously listed in this section are now implemented and should not be
scheduled again unless they need refactoring or bug fixes.
