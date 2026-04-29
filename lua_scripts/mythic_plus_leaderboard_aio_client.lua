--[[
  mod-mythic-enhanced — AIO client UI for Mythic+ leaderboards (pushed by server).
  3.3.5a — uses UISpecialFrames (ESC), optional AIO.SavePosition / AddSavedVarChar.
]]

local AIO = AIO or require("AIO")
if AIO.AddAddon() then
    return
end

local CHANNEL = "MPLB"

-- Persisted per character (AIO restores before addon runs)
local SAVED_MAP_KEY = "MythicPlusLB_LastMapId"
local SAVED_LIMIT_KEY = "MythicPlusLB_RowLimit"
local SAVED_SEASON_KEY = "MythicPlusLB_SeasonId"
if AIO.AddSavedVarChar then
    AIO.AddSavedVarChar(SAVED_MAP_KEY)
    AIO.AddSavedVarChar(SAVED_LIMIT_KEY)
    AIO.AddSavedVarChar(SAVED_SEASON_KEY)
end

_G[SAVED_MAP_KEY] = _G[SAVED_MAP_KEY] or 574
_G[SAVED_LIMIT_KEY] = math.min(50, math.max(10, tonumber(_G[SAVED_LIMIT_KEY]) or 25))
_G[SAVED_SEASON_KEY] = math.max(0, tonumber(_G[SAVED_SEASON_KEY]) or 0)

local rowLimit = _G[SAVED_LIMIT_KEY]
local viewMode = "overall"
local seasonList = {}
local selectedSeasonId = _G[SAVED_SEASON_KEY]
local activeSeasonId = 0
local doRefresh

local function fmtTime(sec)
    sec = math.floor(tonumber(sec) or 0)
    local m = math.floor(sec / 60)
    local s = sec % 60
    return string.format("%d:%02d", m, s)
end

local function seasonTitle(season)
    if not season then
        return "Mythic+ leaderboards"
    end
    local y = season.year or 0
    local mo = season.month or 0
    local lab = season.label or "?"
    return string.format("%s  |  %04u-%02u", lab, y, mo)
end

local function seasonStateLabel(season)
    if not season then
        return "No season"
    end

    return season.isActive and "Active season" or "Archived season"
end

local function truncName(name, maxLen)
    name = tostring(name or "?")
    maxLen = maxLen or 12
    if #name <= maxLen then
        return name
    end
    return string.sub(name, 1, maxLen - 1) .. "…"
end

-- ——— frame chrome ——————————————————————————————————————————

local frame = CreateFrame("Frame", "MythicPlusLeaderboardAIOFrame", UIParent)
frame:SetFrameStrata("DIALOG")
frame:SetClampedToScreen(true)
frame:SetSize(460, 392)
frame:SetPoint("CENTER")
frame:SetBackdrop({
    bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background",
    edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Border",
    tile = true,
    tileSize = 32,
    edgeSize = 32,
    insets = { left = 10, right = 10, top = 10, bottom = 10 },
})
frame:SetBackdropColor(0.05, 0.05, 0.07, 0.92)
frame:SetMovable(true)
frame:EnableMouse(true)
frame:Hide()

tinsert(UISpecialFrames, frame:GetName())

local titleBar = CreateFrame("Button", nil, frame)
titleBar:SetHeight(28)
titleBar:SetPoint("TOPLEFT", 12, -10)
titleBar:SetPoint("TOPRIGHT", -12, -10)
titleBar:RegisterForDrag("LeftButton")
titleBar:SetScript("OnDragStart", function()
    frame:StartMoving()
end)
titleBar:SetScript("OnDragStop", function()
    frame:StopMovingOrSizing()
end)

local titleBarTex = titleBar:CreateTexture(nil, "BACKGROUND")
titleBarTex:SetAllPoints()
titleBarTex:SetTexture("Interface\\Buttons\\WHITE8X8")
titleBarTex:SetVertexColor(0.2, 0.16, 0.1, 0.95)

local title = titleBar:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
title:SetPoint("LEFT", 8, 0)
title:SetText("Mythic+ leaderboards")

local closeBtn = CreateFrame("Button", nil, frame, "UIPanelCloseButton")
closeBtn:SetPoint("TOPRIGHT", -6, -6)

-- ——— toolbar ———————————————————————————————————————————————

local toolbarY = -42

local tabOverall = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
tabOverall:SetSize(100, 24)
tabOverall:SetPoint("TOPLEFT", frame, "TOPLEFT", 16, toolbarY)
tabOverall:SetText("Overall")

local tabMap = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
tabMap:SetSize(100, 24)
tabMap:SetPoint("LEFT", tabOverall, "RIGHT", 6, 0)
tabMap:SetText("Dungeon")

local tabSelf = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
tabSelf:SetSize(100, 24)
tabSelf:SetPoint("LEFT", tabMap, "RIGHT", 6, 0)
tabSelf:SetText("My rating")

local refreshBtn = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
refreshBtn:SetSize(72, 24)
refreshBtn:SetPoint("LEFT", tabSelf, "RIGHT", 10, 0)
refreshBtn:SetText("Refresh")

local seasonPrev = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
seasonPrev:SetSize(24, 20)
seasonPrev:SetPoint("TOPRIGHT", frame, "TOPRIGHT", -108, -74)
seasonPrev:SetText("<")

local seasonNext = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
seasonNext:SetSize(24, 20)
seasonNext:SetPoint("LEFT", seasonPrev, "RIGHT", 92, 0)
seasonNext:SetText(">")

local seasonBtn = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
seasonBtn:SetSize(86, 20)
seasonBtn:SetPoint("LEFT", seasonPrev, "RIGHT", 4, 0)
seasonBtn:SetText("Active")

local mapLabel = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
mapLabel:SetPoint("TOPLEFT", tabOverall, "BOTTOMLEFT", 0, -10)
mapLabel:SetText("Map id")

local mapEdit = CreateFrame("EditBox", nil, frame, "InputBoxTemplate")
mapEdit:SetSize(72, 20)
mapEdit:SetPoint("LEFT", mapLabel, "RIGHT", 8, -1)
mapEdit:SetAutoFocus(false)
mapEdit:SetNumeric(true)
mapEdit:SetMaxLetters(5)
mapEdit:SetText(tostring(_G[SAVED_MAP_KEY]))

local mapGo = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
mapGo:SetSize(44, 24)
mapGo:SetPoint("LEFT", mapEdit, "RIGHT", 8, 0)
mapGo:SetText("Go")

local limLabel = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
limLabel:SetPoint("LEFT", mapGo, "RIGHT", 14, -1)
limLabel:SetText("Rows")

local lim25 = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
lim25:SetSize(36, 24)
lim25:SetPoint("LEFT", limLabel, "RIGHT", 6, 0)
lim25:SetText("25")

local lim50 = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
lim50:SetSize(36, 24)
lim50:SetPoint("LEFT", lim25, "RIGHT", 4, 0)
lim50:SetText("50")

-- ——— scroll body —————————————————————————————————————————————

local scrollTop = -108
local scroll = CreateFrame("ScrollFrame", nil, frame, "UIPanelScrollFrameTemplate")
scroll:SetPoint("TOPLEFT", 18, scrollTop)
scroll:SetPoint("BOTTOMRIGHT", -34, 36)

local content = CreateFrame("Frame", nil, scroll)
content:SetSize(400, 1)
scroll:SetScrollChild(content)

local text = content:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
text:SetPoint("TOPLEFT", 2, -4)
text:SetWidth(400)
text:SetJustifyH("LEFT")
text:SetJustifyV("TOP")
text:SetNonSpaceWrap(false)

local status = frame:CreateFontString(nil, "OVERLAY", "GameFontDisableSmall")
status:SetPoint("BOTTOMLEFT", 18, 12)
status:SetPoint("BOTTOMRIGHT", -18, 12)
status:SetJustifyH("LEFT")
status:SetText(" ")

local hint = frame:CreateFontString(nil, "OVERLAY", "GameFontDisableSmall")
hint:SetPoint("TOP", frame, "BOTTOM", 0, -2)
hint:SetWidth(420)
hint:SetJustifyH("CENTER")
hint:SetText("|cff888888/mythiclb |cff666666or|r /mplb  |cff888888·|r  use < > to browse seasons  |cff888888·|r  Map id = instance map from |cff888888mythic_plus_capable_dungeon|r")

-- ——— tab highlight + row limit ——————————————————————————————

local function setTabVisual(active)
    local dim = { 0.62, 0.62, 0.62 }
    local gold = { 1, 0.82, 0 }
    local tabs = { overall = tabOverall, map = tabMap, self = tabSelf }
    for mode, btn in pairs(tabs) do
        local fs = btn:GetFontString()
        if fs then
            local c = (active == mode) and gold or dim
            fs:SetTextColor(c[1], c[2], c[3])
        end
    end
end

local function setRowLimitUi(n)
    rowLimit = n
    _G[SAVED_LIMIT_KEY] = n
    local onR, onG, onB = 1, 0.85, 0.35
    local offR, offG, offB = 0.55, 0.55, 0.55
    local fs25, fs50 = lim25:GetFontString(), lim50:GetFontString()
    if fs25 and fs50 then
        if n == 25 then
            fs25:SetTextColor(onR, onG, onB)
            fs50:SetTextColor(offR, offG, offB)
        else
            fs25:SetTextColor(offR, offG, offB)
            fs50:SetTextColor(onR, onG, onB)
        end
    end
end

setRowLimitUi(rowLimit)

local function setBody(msg)
    text:SetText(msg or "")
    local h = math.max(text:GetStringHeight() + 16, 48)
    content:SetHeight(h)
    scroll:SetVerticalScroll(0)
    scroll:UpdateScrollChildRect()
end

local function getSeasonIndexById(seasonId)
    for i = 1, #seasonList do
        if (seasonList[i].id or 0) == (seasonId or 0) then
            return i
        end
    end
    return nil
end

local function updateSeasonButton(season)
    if not season then
        seasonBtn:SetText("Active")
        return
    end

    local label = season.label or string.format("%04u-%02u", season.year or 0, season.month or 0)
    if season.isActive then
        label = label .. "*"
    end
    seasonBtn:SetText(label)
end

local function requestSeasons()
    AIO.Handle(CHANNEL, "ReqSeasons", selectedSeasonId)
end

local function setSelectedSeason(seasonId, skipRefresh)
    selectedSeasonId = math.max(0, tonumber(seasonId) or 0)
    _G[SAVED_SEASON_KEY] = selectedSeasonId
    local season = nil
    if selectedSeasonId > 0 then
        local idx = getSeasonIndexById(selectedSeasonId)
        season = idx and seasonList[idx] or nil
    else
        for i = 1, #seasonList do
            if (activeSeasonId > 0 and seasonList[i].id == activeSeasonId) or seasonList[i].isActive then
                season = seasonList[i]
                break
            end
        end
    end
    updateSeasonButton(season)
    if not skipRefresh and frame:IsShown() then
        doRefresh()
    end
end

local function showErr(err)
    if err == "no_season" then
        setBody("|cffff5555No active Mythic+ season.|r\n|cff888888Season rows must exist in the characters DB.|r")
    elseif err == "bad_map" then
        setBody("|cffff5555Invalid map id.|r\n|cff888888Enter a positive instance map id (see capable-dungeon list).|r")
    else
        setBody("|cffff5555" .. tostring(err or "Unknown error") .. "|r")
    end
end

doRefresh = function()
    if viewMode == "overall" then
        tabOverall:OnClick()
    elseif viewMode == "map" then
        tabMap:OnClick()
    else
        tabSelf:OnClick()
    end
end

function tabOverall:OnClick()
    viewMode = "overall"
    setTabVisual("overall")
    status:SetText("Loading overall…")
    setBody("|cffaaaaaaFetching leaderboard…|r")
    AIO.Handle(CHANNEL, "ReqOverall", selectedSeasonId, rowLimit)
end

function tabMap:OnClick()
    viewMode = "map"
    setTabVisual("map")
    local mid = tonumber(mapEdit:GetText()) or 0
    _G[SAVED_MAP_KEY] = mid
    status:SetText("Loading dungeon " .. mid .. "…")
    setBody("|cffaaaaaaFetching dungeon board…|r")
    AIO.Handle(CHANNEL, "ReqMap", selectedSeasonId, mid, rowLimit)
end

function tabSelf:OnClick()
    viewMode = "self"
    setTabVisual("self")
    status:SetText("Loading your summary…")
    setBody("|cffaaaaaaFetching your stats…|r")
    AIO.Handle(CHANNEL, "ReqSelf", selectedSeasonId)
end

tabOverall:SetScript("OnClick", function()
    tabOverall:OnClick()
end)
tabMap:SetScript("OnClick", function()
    tabMap:OnClick()
end)
tabSelf:SetScript("OnClick", function()
    tabSelf:OnClick()
end)
mapGo:SetScript("OnClick", function()
    tabMap:OnClick()
end)

refreshBtn:SetScript("OnClick", function()
    requestSeasons()
    doRefresh()
end)

seasonPrev:SetScript("OnClick", function()
    if #seasonList == 0 then
        requestSeasons()
        return
    end

    local idx = getSeasonIndexById(selectedSeasonId)
    if not idx then
        idx = 1
        for i = 1, #seasonList do
            if seasonList[i].isActive then
                idx = i
                break
            end
        end
    end

    if idx < #seasonList then
        setSelectedSeason(seasonList[idx + 1].id)
    end
end)

seasonNext:SetScript("OnClick", function()
    if #seasonList == 0 then
        requestSeasons()
        return
    end

    local idx = getSeasonIndexById(selectedSeasonId)
    if not idx then
        idx = 1
        for i = 1, #seasonList do
            if seasonList[i].isActive then
                idx = i
                break
            end
        end
    end

    if idx > 1 then
        setSelectedSeason(seasonList[idx - 1].id)
    else
        setSelectedSeason(0)
    end
end)

seasonBtn:SetScript("OnClick", function()
    setSelectedSeason(0)
end)

lim25:SetScript("OnClick", function()
    setRowLimitUi(25)
    if viewMode == "overall" or viewMode == "map" then
        doRefresh()
    end
end)

lim50:SetScript("OnClick", function()
    setRowLimitUi(50)
    if viewMode == "overall" or viewMode == "map" then
        doRefresh()
    end
end)

mapEdit:SetScript("OnEnterPressed", function(self)
    self:ClearFocus()
    tabMap:OnClick()
end)

mapEdit:SetScript("OnEscapePressed", function(self)
    self:ClearFocus()
end)

-- ——— server pushes ———————————————————————————————————————————

AIO.AddHandlers(CHANNEL, {
    PushSeasons = function(seasons, selectedId, resolvedId)
        seasonList = seasons or {}
        activeSeasonId = tonumber(resolvedId) or 0
        if resolvedId and resolvedId > 0 then
            if selectedId and selectedId > 0 then
                setSelectedSeason(selectedId, true)
            else
                setSelectedSeason(0, true)
            end
        else
            setSelectedSeason(0, true)
        end
    end,

    PushOverall = function(err, season, rows)
        if err then
            showErr(err)
            status:SetText("")
            return
        end
        if season and season.id then
            setSelectedSeason(season.id, true)
        end
        title:SetText(seasonTitle(season))
        local lines = {}
        lines[#lines + 1] = "|cffccaa77#|r  |cffccaa77Player|r          |cffccaa77Score|r    |cffccaa77Best|r   |cffccaa77Runs|r"
        lines[#lines + 1] = "|cff444444---------------------------------------------|r"
        for i = 1, #rows do
            local r = rows[i]
            lines[#lines + 1] = string.format(
                "|cffffffff%2d.|r  %-14s  |cffffffff%6u|r   |cffffffff+%-3u|r  |cffaaaaaa(%u)|r",
                i,
                truncName(r.charName, 14),
                r.totalScore or 0,
                r.bestLevel or 0,
                r.runs or 0
            )
        end
        if #rows == 0 then
            lines[#lines + 1] = "|cffffcc66No ranked entries for this season selection.|r"
        end
        setBody(table.concat(lines, "\n"))
        status:SetText(string.format("|cff888888%u rows|r  |cff666666·|r  |cff888888%s|r", #rows, seasonStateLabel(season)))
    end,

    PushMap = function(err, season, mapId, rows)
        if err then
            showErr(err)
            status:SetText("")
            return
        end
        if season and season.id then
            setSelectedSeason(season.id, true)
        end
        _G[SAVED_MAP_KEY] = tonumber(mapId) or _G[SAVED_MAP_KEY]
        title:SetText(seasonTitle(season) .. "  |  map " .. tostring(mapId))
        local lines = {}
        lines[#lines + 1] = "|cffccaa77#|r |cffccaa77Player|r       |cffccaa77+|r   |cffccaa77Time|r  |cffccaa77Deaths|r |cffccaa77+|rtime |cffccaa77Score|r"
        lines[#lines + 1] = "|cff444444----------------------------------------------------------|r"
        for i = 1, #rows do
            local r = rows[i]
            local it = r.inTime and "|cff88ff88Y|r" or "|cffff6666N|r"
            lines[#lines + 1] = string.format(
                "|cffffffff%2d.|r %-12s |cffffffff+%-2u|r %6s   |cffffffff%-2u|r     %s    |cffffffff%u|r",
                i,
                truncName(r.charName, 12),
                r.mythicLevel or 0,
                fmtTime(r.bestTime),
                r.deaths or 0,
                it,
                r.score or 0
            )
        end
        if #rows == 0 then
            lines[#lines + 1] = "|cffffcc66No entries for this dungeon in the selected season.|r"
        end
        setBody(table.concat(lines, "\n"))
        status:SetText(string.format("|cff888888%u rows|r  |cff666666·|r  |cff888888Map %s|r  |cff666666·|r  |cff888888%s|r", #rows, tostring(mapId), seasonStateLabel(season)))
    end,

    PushSelf = function(err, season, summary, rank)
        if err then
            showErr(err)
            status:SetText("")
            return
        end
        if season and season.id then
            setSelectedSeason(season.id, true)
        end
        title:SetText(seasonTitle(season))
        if not summary or (summary.runs or 0) == 0 then
            setBody("|cffffcc66You have no ranked runs for this season yet.|r\n|cff888888Complete a Mythic+ run to appear here.|r")
            status:SetText("")
            return
        end
        local rk = rank and rank > 0 and tostring(rank) or "—"
        setBody(string.format(
            "|cffccaa77Your season summary|r\n\n"
                .. "|cffffffffSeason|r          |cffffffff%s|r\n"
                .. "|cffffffffState|r           |cffffffff%s|r\n"
                .. "|cffffffffTotal score|r     |cffffffff%u|r\n"
                .. "|cffffffffBest key|r         |cffffffff+%u|r\n"
                .. "|cffffffffRuns logged|r     |cffffffff%u|r\n"
                .. "|cffffffffOverall rank|r    |cffffffff%s|r",
            season and (season.label or "?") or "?",
            seasonStateLabel(season),
            summary.totalScore or 0,
            summary.bestLevel or 0,
            summary.runs or 0,
            rk
        ))
        status:SetText(string.format("|cff888888%s|r", seasonStateLabel(season)))
    end,
})

-- ——— show / position —————————————————————————————————————————

if AIO.SavePosition then
    AIO.SavePosition(frame, true)
end

SLASH_MYTHICPLUSLB1 = "/mythiclb"
SLASH_MYTHICPLUSLB2 = "/mplb"
SlashCmdList["MYTHICPLUSLB"] = function()
    if frame:IsShown() then
        frame:Hide()
    else
        frame:Show()
        requestSeasons()
        setTabVisual(viewMode)
        setRowLimitUi(rowLimit)
        if viewMode == "overall" then
            tabOverall:OnClick()
        elseif viewMode == "map" then
            tabMap:OnClick()
        else
            tabSelf:OnClick()
        end
    end
end

closeBtn:SetScript("OnClick", function()
    frame:Hide()
end)

print("|cff00ccff[Mythic+]|r Leaderboard UI — |cffffffff/mythiclb|r, |cffffffff/mplb|r  |cff888888(ESC closes)|r")
