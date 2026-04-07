# Mythic Enhanced website integration

This document is the handoff packet for a frontend or full-stack LLM building a
website around the Mythic Enhanced system.

The module exposes two kinds of data:

- **characters database**: live, seasonal, leaderboard, and run-history data
- **world database**: static dungeon catalog / UI metadata

Design assumption: the website should treat the SQL views documented here as the
stable backend contract and avoid reading raw tables unless absolutely needed.

## Product goal

Build a website that lets players:

- view the current Mythic season
- browse overall and per-dungeon leaderboards
- inspect recent runs
- browse previous seasons
- navigate using readable dungeon names/slugs instead of raw map ids

## Data sources

### Characters database

Use this for all leaderboard, season, and run-history pages.

### World database

Use this for static dungeon metadata via `mythic_plus_web_dungeon_catalog`.

The recommended backend approach is:

1. fetch leaderboard data from the characters DB
2. fetch dungeon catalog data from the world DB
3. join them in the application layer by `map_id`

Do **not** rely on hardcoded database names or cross-database SQL joins in the
site unless you fully control the deployment environment.

## Recommended views

### Seasons

- `mythic_plus_web_current_season`
  - active season only
  - includes `seconds_until_end`
- `mythic_plus_web_seasons`
  - season archive / selector data
  - includes player and leaderboard-entry counts

### Leaderboards

- `mythic_plus_web_leaderboard_current_overall`
  - current season, one row per player
- `mythic_plus_web_leaderboard_current_map`
  - current season, one row per player per dungeon per difficulty
- `mythic_plus_web_leaderboard_overall`
  - all seasons, one row per player per season
- `mythic_plus_web_leaderboard_map`
  - all seasons, one row per player per dungeon per difficulty

### Recent runs / history

- `mythic_plus_web_current_run_history`
  - completed runs in the current season
- `mythic_plus_web_run_history`
  - completed runs across all seasons

### Dungeon catalog

- `mythic_plus_web_dungeon_catalog` *(world database)*
  - stable dungeon display metadata
  - includes slug, display name, short name, expansion, difficulty label,
    and asset keys

## Exposed character metadata

Leaderboard views join against the core `characters` table and expose:

- `char_guid`
- `char_name`
- `char_race`
- `char_class`
- `char_gender`
- `char_level`
- `char_online`

This makes it easier to build class/race icons, profile links, and online
markers without performing extra joins in every page query.

## Exposed dungeon metadata

The world DB view `mythic_plus_web_dungeon_catalog` exposes:

- `map_id`
- `slug`
- `display_name`
- `short_name`
- `expansion`
- `icon_key`
- `image_key`
- `sort_order`
- `min_difficulty`
- `min_difficulty_label`
- `final_boss_entry`
- `is_mythic_enabled`

Use `slug` for routes, `display_name` for page headings, and `icon_key` /
`image_key` as asset identifiers in the frontend.

## Suggested website pages

### 1. Home / season overview

Show:

- active season label
- countdown until reset
- top 10 overall players
- latest 10 completed runs
- dungeon quick links

### 2. Overall leaderboard page

Show:

- rank
- player name
- class/race icon
- total score
- best key level
- runs completed
- online indicator (optional)

### 3. Dungeon leaderboard page

Route example:

- `/dungeons/utgarde-keep`

Show:

- dungeon header art/name
- current top runs for that dungeon
- difficulty, score, key level, best time, deaths
- group members for each run

### 4. Season archive

Route example:

- `/seasons`
- `/seasons/2026-04`

Show:

- list of seasons
- player counts
- total entries
- archived overall leaderboard

### 5. Recent runs / activity feed

Show:

- dungeon
- group members
- key level
- completion time
- timed / overtime state
- reward state
- finished timestamp

## Suggested queries

### Current season banner / countdown

```sql
SELECT *
FROM mythic_plus_web_current_season;
```

### Dungeon catalog for navigation

```sql
SELECT *
FROM mythic_plus_web_dungeon_catalog
WHERE is_mythic_enabled = 1
ORDER BY sort_order ASC, display_name ASC;
```

### Current overall top 20

```sql
SELECT *
FROM mythic_plus_web_leaderboard_current_overall
ORDER BY total_score DESC, best_level DESC, char_name ASC
LIMIT 20;
```

### Current dungeon leaderboard for a map

```sql
SELECT *
FROM mythic_plus_web_leaderboard_current_map
WHERE map_id = 574
ORDER BY score DESC, mythic_level DESC, best_time ASC, deaths ASC, char_name ASC
LIMIT 20;
```

### Current dungeon leaderboard with app-layer map lookup

Use `map_id` from this result to match an entry from
`mythic_plus_web_dungeon_catalog`.

### Season archive list

```sql
SELECT *
FROM mythic_plus_web_seasons
ORDER BY year DESC, month DESC;
```

### Overall standings for a specific archived season

```sql
SELECT *
FROM mythic_plus_web_leaderboard_overall
WHERE season_id = 3
ORDER BY total_score DESC, best_level DESC, char_name ASC
LIMIT 50;
```

### Latest completed runs

```sql
SELECT *
FROM mythic_plus_web_current_run_history
ORDER BY end_time DESC
LIMIT 25;
```

### All dungeon metadata for frontend caches

```sql
SELECT *
FROM mythic_plus_web_dungeon_catalog
ORDER BY sort_order ASC, display_name ASC;
```

## Route and API suggestions

Recommended routes:

- `/`
- `/leaderboard`
- `/dungeons`
- `/dungeons/:slug`
- `/runs`
- `/seasons`
- `/seasons/:seasonLabel`

Recommended backend DTOs / API responses:

- `SeasonSummary`
- `OverallLeaderboardEntry`
- `DungeonLeaderboardEntry`
- `DungeonCatalogEntry`
- `RunHistoryEntry`

The backend should normalize SQL rows into clean JSON objects before returning
them to the frontend.

## Field semantics

- `best_time`, `time_limit`, `total_time`, and `penalty_seconds` are in seconds
- `completed_in_time` is `1` for timed runs and `0` otherwise
- `rewarded` is `1` when the run granted rewards
- `group_members` is a comma-separated display string, not a normalized list
- `char_online` is a convenience flag for live character state
- `season_label` is the safest display token for archive UX
- `slug` is the safest routing token for dungeon pages

## Presentation guidance for the LLM building the site

- treat the project as a **Wrath of the Lich King Mythic+ leaderboard site**
- use a dark fantasy MMO visual style
- design for desktop first, but keep mobile support solid
- prefer readable tables with sticky headers for leaderboard pages
- use dungeon cards for navigation
- use countdown components for season reset timing
- use colored badges for timed vs overtime runs
- use small party chips or avatar initials for `group_members`

## Sorting rules

- Store `season_id` or `season_label` in archive routes.
- Use `seconds_until_end` for countdown timers.
- Sort overall leaderboards by:
  1. `total_score` DESC
  2. `best_level` DESC
  3. `char_name` ASC
- Sort dungeon leaderboards by:
  1. `score` DESC
  2. `mythic_level` DESC
  3. `best_time` ASC
  4. `deaths` ASC
  5. `char_name` ASC
- Sort recent runs by `end_time` DESC.

## Raw tables behind the views

### Characters DB

- `mythic_plus_season`
- `mythic_plus_leaderboard`
- `mythic_plus_dungeon_snapshot`

### World DB

- `mythic_plus_capable_dungeon`
- `mythic_plus_dungeon_ui`

Use the views whenever possible; they are intended to be the stable contract
for website work.
