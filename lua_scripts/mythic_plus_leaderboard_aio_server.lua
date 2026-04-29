--[[
  mod-mythic-enhanced — server-side AIO bridge for Mythic+ leaderboards.
  Requires: mod-ale (Eluna), Rochet2 AIO in lua_scripts/AIO_Server/AIO.lua

  Install: copy this file and mythic_plus_leaderboard_aio_client.lua next to
  worldserver under lua_scripts/ (same layout as env/dist/bin/lua_scripts).
]]

local function load_aio()
    if AIO then
        return true
    end
    local paths = {
        "lua_scripts/AIO_Server/AIO.lua",
        "AIO_Server/AIO.lua",
    }
    for _, p in ipairs(paths) do
        local f, err = loadfile(p)
        if f then
            f()
            return true
        end
    end
    print("[MythicPlusLeaderboardAIO] FATAL: could not load AIO (tried "
        .. table.concat(paths, ", ") .. ")")
    return false
end

if not load_aio() or not AIO or not AIO.IsServer() or not AIO.IsMainState() then
    return
end

local CHANNEL = "MPLB"
local MAX_ROWS = 50

local function clamp_u32(n, default)
    n = tonumber(n)
    if not n or n ~= n then
        return default
    end
    n = math.floor(n)
    if n < 0 then
        return default
    end
    if n > 0xFFFFFFFF then
        return 0xFFFFFFFF
    end
    return n
end

local function clamp_limit(n)
    n = clamp_u32(n, 25)
    if n < 1 then
        return 1
    end
    if n > MAX_ROWS then
        return MAX_ROWS
    end
    return n
end

--- @return table|nil season
local function fetch_season_row(seasonId)
    local q = CharDBQuery(string.format(
        "SELECT id, label, year, month FROM mythic_plus_season WHERE id = %u LIMIT 1",
        seasonId
    ))
    if not q or q:GetRowCount() == 0 then
        return nil
    end
    repeat
        return {
            id = q:GetUInt32(0),
            label = q:GetString(1),
            year = q:GetUInt16(2),
            month = q:GetUInt8(3),
        }
    until not q:NextRow()
end

--- @return table|nil season
local function fetch_active_season()
    local q = CharDBQuery(
        "SELECT id, label, year, month FROM mythic_plus_season WHERE is_active = 1 LIMIT 1"
    )
    if not q or q:GetRowCount() == 0 then
        return nil
    end
    repeat
        return {
            id = q:GetUInt32(0),
            label = q:GetString(1),
            year = q:GetUInt16(2),
            month = q:GetUInt8(3),
        }
    until not q:NextRow()
end

--- @return table|nil season
local function resolve_season(requestedSeasonId)
    local rid = clamp_u32(requestedSeasonId, 0)
    if rid > 0 then
        local s = fetch_season_row(rid)
        if s then
            return s
        end
    end
    return fetch_active_season()
end

local function query_recent_seasons(limit)
    local rows = {}
    local sql = string.format(
        "SELECT id, label, year, month, is_active FROM mythic_plus_season ORDER BY year DESC, month DESC LIMIT %u",
        clamp_limit(limit)
    )
    local q = CharDBQuery(sql)
    if not q or q:GetRowCount() == 0 then
        return rows
    end

    repeat
        rows[#rows + 1] = {
            id = q:GetUInt32(0),
            label = q:GetString(1),
            year = q:GetUInt16(2),
            month = q:GetUInt8(3),
            isActive = q:GetUInt8(4) ~= 0,
        }
    until not q:NextRow()

    return rows
end

local function query_overall(seasonId, limit)
    local sql = string.format(
        "SELECT char_guid, char_name, SUM(score) AS total_score, MAX(mythic_level) AS best_level, COUNT(*) AS runs "
            .. "FROM mythic_plus_leaderboard WHERE season_id = %u "
            .. "GROUP BY char_guid, char_name "
            .. "ORDER BY total_score DESC, best_level DESC, char_name ASC LIMIT %u",
        seasonId,
        limit
    )
    local rows = {}
    local q = CharDBQuery(sql)
    if not q or q:GetRowCount() == 0 then
        return rows
    end
    repeat
        rows[#rows + 1] = {
            charGuid = q:GetUInt32(0),
            charName = q:GetString(1),
            totalScore = q:GetUInt32(2),
            bestLevel = q:GetUInt32(3),
            runs = q:GetUInt32(4),
        }
    until not q:NextRow()
    return rows
end

local function query_map(seasonId, mapId, limit)
    local sql = string.format(
        "SELECT char_name, mythic_level, best_time, deaths, completed_in_time, score, group_members "
            .. "FROM mythic_plus_leaderboard WHERE season_id = %u AND map_id = %u "
            .. "ORDER BY score DESC, mythic_level DESC, best_time ASC, deaths ASC, char_name ASC LIMIT %u",
        seasonId,
        mapId,
        limit
    )
    local rows = {}
    local q = CharDBQuery(sql)
    if not q or q:GetRowCount() == 0 then
        return rows
    end
    repeat
        rows[#rows + 1] = {
            charName = q:GetString(0),
            mythicLevel = q:GetUInt32(1),
            bestTime = q:GetUInt32(2),
            deaths = q:GetUInt32(3),
            inTime = q:GetUInt8(4) ~= 0,
            score = q:GetUInt32(5),
            groupMembers = q:GetString(6),
        }
    until not q:NextRow()
    return rows
end

local function query_self_summary(seasonId, charGuid)
    local sql = string.format(
        "SELECT SUM(score) AS total_score, MAX(mythic_level) AS best_level, COUNT(*) AS runs "
            .. "FROM mythic_plus_leaderboard WHERE season_id = %u AND char_guid = %u",
        seasonId,
        charGuid
    )
    local q = CharDBQuery(sql)
    if not q or q:GetRowCount() == 0 then
        return { totalScore = 0, bestLevel = 0, runs = 0 }
    end
    repeat
        local total = q:GetUInt32(0)
        local best = q:GetUInt32(1)
        local runs = q:GetUInt32(2)
        if q:IsNull(0) then
            total = 0
        end
        if q:IsNull(1) then
            best = 0
        end
        if q:IsNull(2) then
            runs = 0
        end
        return { totalScore = total, bestLevel = best, runs = runs }
    until not q:NextRow()
end

local function compute_overall_rank(seasonId, charGuid)
    local board = query_overall(seasonId, 1000)
    for i = 1, #board do
        if board[i].charGuid == charGuid then
            return i
        end
    end
    return 0
end

AIO.AddHandlers(CHANNEL, {
    ReqSeasons = function(player, seasonId)
        local seasons = query_recent_seasons(12)
        local season = resolve_season(seasonId)
        AIO.Handle(player, CHANNEL, "PushSeasons", seasons, clamp_u32(seasonId, 0), season and season.id or 0)
    end,

    --- Client: AIO.Handle(CHANNEL, "ReqOverall", seasonId or 0, limit or 25)
    ReqOverall = function(player, seasonId, limit)
        local season = resolve_season(seasonId)
        if not season then
            AIO.Handle(player, CHANNEL, "PushOverall", "no_season", nil, {})
            return
        end
        local lim = clamp_limit(limit)
        local rows = query_overall(season.id, lim)
        AIO.Handle(player, CHANNEL, "PushOverall", nil, season, rows)
    end,

    --- Client: AIO.Handle(CHANNEL, "ReqMap", seasonId or 0, mapId, limit)
    ReqMap = function(player, seasonId, mapId, limit)
        local season = resolve_season(seasonId)
        if not season then
            AIO.Handle(player, CHANNEL, "PushMap", "no_season", nil, 0, {})
            return
        end
        local mid = clamp_u32(mapId, 0)
        if mid == 0 then
            AIO.Handle(player, CHANNEL, "PushMap", "bad_map", season, 0, {})
            return
        end
        local lim = clamp_limit(limit)
        local rows = query_map(season.id, mid, lim)
        AIO.Handle(player, CHANNEL, "PushMap", nil, season, mid, rows)
    end,

    --- Client: AIO.Handle(CHANNEL, "ReqSelf", seasonId or 0)
    ReqSelf = function(player, seasonId)
        local season = resolve_season(seasonId)
        if not season then
            AIO.Handle(player, CHANNEL, "PushSelf", "no_season", nil, nil, 0)
            return
        end
        local guid = player:GetGUIDLow()
        local summary = query_self_summary(season.id, guid)
        local rank = 0
        if summary.runs > 0 then
            rank = compute_overall_rank(season.id, guid)
        end
        AIO.Handle(player, CHANNEL, "PushSelf", nil, season, summary, rank)
    end,
})

local client_paths = {
    "lua_scripts/mythic_plus_leaderboard_aio_client.lua",
    "mythic_plus_leaderboard_aio_client.lua",
}

local added = false
for _, rel in ipairs(client_paths) do
    if AIO.AddAddon(rel) then
        added = true
        print("[MythicPlusLeaderboardAIO] client addon registered: " .. rel)
        break
    end
end

if not added then
    print("[MythicPlusLeaderboardAIO] WARNING: client file not found (tried "
        .. table.concat(client_paths, ", ") .. ")")
end
