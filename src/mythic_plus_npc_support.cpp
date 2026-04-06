/*
 * Credits: silviu20092
 */

#include "Player.h"
#include "Creature.h"
#include "ScriptedGossip.h"
#include "StringConvert.h"
#include "Group.h"
#include "mythic_plus.h"
#include "mythic_affix.h"
#include "mythic_plus_npc_support.h"

void MythicPlusNpcSupport::AddMainMenu(Player* player, Creature* /*creature*/)
{
    PagedData& pagedData = GetPagedData(player);
    pagedData.Reset();
    pagedData.backMenu = false;
    pagedData.type = GossipSupport::PAGED_DATA_TYPE_MYTHIC_NPC_MENU;

    if (!sMythicPlus->IsEnabled())
    {
        Identifier* disabledIdnt = new Identifier();
        disabledIdnt->id = 0;
        disabledIdnt->uiName = MythicPlus::Utils::RedColored("!!! SYSTEM IS NOT ACTIVE !!!");
        pagedData.data.push_back(disabledIdnt);
    }

    Identifier* i1 = new Identifier();
    i1->id = 1;
    i1->uiName = "Choose Mythic Plus level";
    i1->optionIcon = GOSSIP_ICON_BATTLE;
    pagedData.data.push_back(i1);

    uint32 setLevel = sMythicPlus->GetCurrentMythicPlusLevel(player);
    if (setLevel > 0)
    {
        Identifier* resetIdnt = new Identifier();
        resetIdnt->id = 2;
        resetIdnt->uiName = "Reset Mythic Plus level [CURRENT: " + Acore::ToString(setLevel) + "]";
        resetIdnt->optionIcon = GOSSIP_ICON_BATTLE;
        pagedData.data.push_back(resetIdnt);
    }
    else
    {
        Identifier* nothingIdnt = new Identifier();
        nothingIdnt->id = 3;
        nothingIdnt->uiName = MythicPlus::Utils::Colored("No Mythic Plus level set", "b50505");
        nothingIdnt->optionIcon = GOSSIP_ICON_CHAT;
        pagedData.data.push_back(nothingIdnt);
    }

    if (player->GetGroup() != nullptr)
    {
        ObjectGuid leaderGuid = player->GetGroup()->GetLeaderGUID();
        uint32 leaderLevel = sMythicPlus->GetCurrentMythicPlusLevelForGUID(leaderGuid.GetCounter());
        Player* leader = ObjectAccessor::FindConnectedPlayer(leaderGuid);
        Identifier* dungeonLevelIdnt = new Identifier();
        dungeonLevelIdnt->id = 4;
        dungeonLevelIdnt->optionIcon = GOSSIP_ICON_CHAT;
        std::ostringstream oss;
        oss << "Current dungeon Mythic Plus level (based on leader): ";
        if (leaderLevel == 0)

            oss << MythicPlus::Utils::Colored("NONE (0)", "b50505");
        else
            oss << leaderLevel;
        if (!leader)
            oss << " [LEADER OFFLINE]";
        dungeonLevelIdnt->uiName = oss.str();
        pagedData.data.push_back(dungeonLevelIdnt);
    }

    Identifier* mPlusListIdnt = new Identifier();
    mPlusListIdnt->id = 5;
    mPlusListIdnt->uiName = "List of all Mythic Plus capable dungeons";
    pagedData.data.push_back(mPlusListIdnt);

    Identifier* standingsRefreshIdnt = new Identifier();
    standingsRefreshIdnt->id = 6;
    std::ostringstream oss;
    oss << "Mythic Plus standings refresh in: ";
    oss << MythicPlus::Utils::Colored(secsToTimeString((MythicPlus::MYTHIC_SNAPSHOTS_TIMER_FREQ - sMythicPlus->GetMythicSnapshotsTimer()) / 1000), "b50505");
    standingsRefreshIdnt->uiName = oss.str();
    pagedData.data.push_back(standingsRefreshIdnt);

    Identifier* standings = new Identifier();
    standings->id = 7;
    standings->uiName = "Mythic Plus standings -->";
    pagedData.data.push_back(standings);

    Identifier* keystoneIdnt = new Identifier();
    keystoneIdnt->id = 8;
    std::ostringstream koss;
    koss << MythicPlus::Utils::Colored("Acquire Mythic Plus keystone", "700c63");
    if (sMythicPlus->GetKeystoneBuyTimer() > 0)
    {
        uint32 playerKeystoneBuyTimer = sMythicPlus->GetKeystoneBuyTimer(player);
        std::string available = MythicPlus::Utils::GreenColored(" [AVAILABLE NOW]");
        if (playerKeystoneBuyTimer > 0)
        {
            uint64 now = MythicPlus::Utils::GameTimeCount();
            uint64 diff = now - playerKeystoneBuyTimer;
            if (diff < sMythicPlus->GetKeystoneBuyTimer() * 60)
                available = MythicPlus::Utils::RedColored(" [AVAILABLE IN " + secsToTimeString(sMythicPlus->GetKeystoneBuyTimer() * 60 - diff));
        }
        koss << available;
    }
    keystoneIdnt->uiName = koss.str();
    keystoneIdnt->optionIcon = GOSSIP_ICON_MONEY_BAG;
    pagedData.data.push_back(keystoneIdnt);

    Identifier* randomMythicIdnt = new Identifier();
    randomMythicIdnt->id = 9;
    randomMythicIdnt->uiName = "See the list of possible random affixes -->";
    randomMythicIdnt->optionIcon = GOSSIP_ICON_BATTLE;
    pagedData.data.push_back(randomMythicIdnt);

    Identifier* bye = new Identifier();
    bye->id = 10;
    bye->uiName = "Nevermind...";
    pagedData.data.push_back(bye);

    Identifier* seasonInfoIdnt = new Identifier();
    seasonInfoIdnt->id = 11;
    seasonInfoIdnt->uiName = "Current Mythic season info -->";
    seasonInfoIdnt->optionIcon = GOSSIP_ICON_CHAT;
    pagedData.data.push_back(seasonInfoIdnt);

    Identifier* overallLeaderboardIdnt = new Identifier();
    overallLeaderboardIdnt->id = 12;
    overallLeaderboardIdnt->uiName = "Monthly top players -->";
    overallLeaderboardIdnt->optionIcon = GOSSIP_ICON_BATTLE;
    pagedData.data.push_back(overallLeaderboardIdnt);

    Identifier* dungeonLeaderboardIdnt = new Identifier();
    dungeonLeaderboardIdnt->id = 13;
    dungeonLeaderboardIdnt->uiName = "Monthly dungeon leaderboards -->";
    dungeonLeaderboardIdnt->optionIcon = GOSSIP_ICON_BATTLE;
    pagedData.data.push_back(dungeonLeaderboardIdnt);

    pagedData.SortAndCalculateTotals(CompareIdentifierById);
}

void MythicPlusNpcSupport::AddSeasonInfo(Player* player)
{
    PagedData& pagedData = GetPagedData(player);
    pagedData.Reset();
    pagedData.type = GossipSupport::PAGED_DATA_TYPE_MYTHIC_SEASON_INFO;
    pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->seasonId = 0;

    uint32 id = 1;
    MythicPlus::MythicPlusSeason const* season = sMythicPlus->GetActiveSeason();
    if (!season)
    {
        Identifier* noSeasonIdnt = new Identifier();
        noSeasonIdnt->id = id;
        noSeasonIdnt->uiName = MythicPlus::Utils::RedColored("No active Mythic season found");
        pagedData.data.push_back(noSeasonIdnt);
        pagedData.SortAndCalculateTotals(CompareIdentifierById);
        return;
    }

    MythicPlus::MythicPlusPlayerRatingSummary summary = sMythicPlus->GetPlayerRatingSummary(player->GetGUID().GetCounter());

    Identifier* seasonIdnt = new Identifier();
    seasonIdnt->id = id++;
    seasonIdnt->uiName = "Season: " + MythicPlus::Utils::Colored(season->label, "0d852d");
    pagedData.data.push_back(seasonIdnt);

    Identifier* resetIdnt = new Identifier();
    resetIdnt->id = id++;
    resetIdnt->uiName = "Season resets in: " + MythicPlus::Utils::Colored(secsToTimeString(sMythicPlus->GetSecondsUntilSeasonEnd()), "b50505");
    pagedData.data.push_back(resetIdnt);

    Identifier* scoreIdnt = new Identifier();
    scoreIdnt->id = id++;
    scoreIdnt->uiName = "Your rating this season: " + Acore::ToString(summary.totalScore);
    pagedData.data.push_back(scoreIdnt);

    Identifier* bestLevelIdnt = new Identifier();
    bestLevelIdnt->id = id++;
    bestLevelIdnt->uiName = "Your best key this season: " + Acore::ToString(summary.bestLevel);
    pagedData.data.push_back(bestLevelIdnt);

    Identifier* runsIdnt = new Identifier();
    runsIdnt->id = id++;
    runsIdnt->uiName = "Your ranked dungeon entries: " + Acore::ToString(summary.runs);
    pagedData.data.push_back(runsIdnt);

    Identifier* rankIdnt = new Identifier();
    rankIdnt->id = id++;
    if (summary.overallRank > 0)
        rankIdnt->uiName = "Your overall rank: " + MythicPlus::Utils::Colored(Acore::ToString(summary.overallRank), "700c63");
    else
        rankIdnt->uiName = MythicPlus::Utils::RedColored("You are not ranked yet this season");
    pagedData.data.push_back(rankIdnt);

    std::vector<MythicPlus::MythicPlusSeason> recentSeasons = sMythicPlus->GetRecentSeasons();
    bool addedArchiveHeader = false;
    for (MythicPlus::MythicPlusSeason const& archivedSeason : recentSeasons)
    {
        if (archivedSeason.isActive)
            continue;

        if (!addedArchiveHeader)
        {
            Identifier* archiveHeaderIdnt = new Identifier();
            archiveHeaderIdnt->id = id++;
            archiveHeaderIdnt->uiName = MythicPlus::Utils::Colored("-- Recent season archives --", "1a0966");
            pagedData.data.push_back(archiveHeaderIdnt);
            addedArchiveHeader = true;
        }

        Identifier* archiveIdnt = new Identifier();
        archiveIdnt->id = 100000 + archivedSeason.id;
        archiveIdnt->optionIcon = GOSSIP_ICON_BATTLE;
        archiveIdnt->uiName = "Browse archived season " + archivedSeason.label + " -->";
        pagedData.data.push_back(archiveIdnt);
    }

    pagedData.SortAndCalculateTotals(CompareIdentifierById);
}

void MythicPlusNpcSupport::AddSeasonArchiveMenu(Player* player, uint32 seasonId)
{
    PagedData& pagedData = GetPagedData(player);
    pagedData.Reset();
    pagedData.type = GossipSupport::PAGED_DATA_TYPE_MYTHIC_SEASON_ARCHIVE_MENU;
    pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->seasonId = seasonId;

    MythicPlus::MythicPlusSeason const* season = sMythicPlus->GetSeason(seasonId);
    uint32 id = 1;

    if (!season)
    {
        Identifier* missingIdnt = new Identifier();
        missingIdnt->id = id;
        missingIdnt->uiName = MythicPlus::Utils::RedColored("Requested Mythic season archive was not found");
        pagedData.data.push_back(missingIdnt);
        pagedData.SortAndCalculateTotals(CompareIdentifierById);
        return;
    }

    Identifier* seasonIdnt = new Identifier();
    seasonIdnt->id = id++;
    seasonIdnt->uiName = "Archived season: " + MythicPlus::Utils::Colored(season->label, "1a0966");
    pagedData.data.push_back(seasonIdnt);

    Identifier* datesIdnt = new Identifier();
    datesIdnt->id = id++;
    datesIdnt->uiName = "Season window: " + MythicPlus::Utils::DateFromSeconds(season->startUnix) + " -> " + MythicPlus::Utils::DateFromSeconds(season->endUnix);
    pagedData.data.push_back(datesIdnt);

    Identifier* overallIdnt = new Identifier();
    overallIdnt->id = id++;
    overallIdnt->optionIcon = GOSSIP_ICON_BATTLE;
    overallIdnt->uiName = "Archived top players -->";
    pagedData.data.push_back(overallIdnt);

    Identifier* dungeonIdnt = new Identifier();
    dungeonIdnt->id = id++;
    dungeonIdnt->optionIcon = GOSSIP_ICON_BATTLE;
    dungeonIdnt->uiName = "Archived dungeon leaderboards -->";
    pagedData.data.push_back(dungeonIdnt);

    pagedData.SortAndCalculateTotals(CompareIdentifierById);
}

void MythicPlusNpcSupport::AddOverallLeaderboard(Player* player)
{
    PagedData& pagedData = GetPagedData(player);
    pagedData.Reset();
    pagedData.type = GossipSupport::PAGED_DATA_TYPE_MYTHIC_OVERALL_LEADERBOARD;

    uint32 id = 1;
    uint32 seasonId = pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->seasonId;
    MythicPlus::MythicPlusSeason const* season = seasonId > 0 ? sMythicPlus->GetSeason(seasonId) : sMythicPlus->GetActiveSeason();

    Identifier* headerIdnt = new Identifier();
    headerIdnt->id = id++;
    headerIdnt->uiName = season ? "Top players for season " + season->label : "Top Mythic players";
    pagedData.data.push_back(headerIdnt);

    std::vector<MythicPlus::MythicPlusOverallLeaderboardEntry> entries = sMythicPlus->GetOverallLeaderboard(50, seasonId);
    if (entries.empty())
    {
        Identifier* emptyIdnt = new Identifier();
        emptyIdnt->id = id++;
        emptyIdnt->uiName = MythicPlus::Utils::RedColored("No ranked runs yet");
        pagedData.data.push_back(emptyIdnt);
    }
    else
    {
        for (std::size_t i = 0; i < entries.size(); ++i)
        {
            Identifier* idnt = new Identifier();
            idnt->id = id++;
            std::ostringstream oss;
            oss << i + 1 << ". " << entries[i].charName;
            oss << " [SCORE: " << entries[i].totalScore << "]";
            oss << " [BEST: +" << entries[i].bestLevel << "]";
            oss << " [RUNS: " << entries[i].runs << "]";
            idnt->uiName = oss.str();
            pagedData.data.push_back(idnt);
        }
    }

    pagedData.SortAndCalculateTotals(CompareIdentifierById);
}

void MythicPlusNpcSupport::AddDungeonListForLeaderboard(Player* player, uint32 seasonId)
{
    PagedData& pagedData = GetPagedData(player);
    pagedData.Reset();
    pagedData.type = GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_LIST_LEADERBOARD;
    pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->seasonId = seasonId;

    const std::unordered_map<uint32, MythicPlus::MythicPlusCapableDungeon>& dungeons = sMythicPlus->GetAllMythicPlusDungeons();
    LocaleConstant locale = player->GetSession()->GetSessionDbcLocale();
    for (const auto& dpair : dungeons)
    {
        MapEntry const* map = sMapStore.LookupEntry(dpair.first);
        ASSERT(map);

        Identifier* idnt = new Identifier();
        idnt->id = dpair.first;
        idnt->optionIcon = GOSSIP_ICON_BATTLE;
        idnt->uiName = std::string(map->name[locale]) + " -->";
        pagedData.data.push_back(idnt);
    }

    pagedData.SortAndCalculateTotals(CompareIdentifierById);
}

void MythicPlusNpcSupport::AddMapLeaderboard(Player* player, uint32 mapEntry, uint32 seasonId)
{
    PagedData& pagedData = GetPagedData(player);
    pagedData.Reset();
    pagedData.type = GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_MAP_LEADERBOARD;
    pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->mapEntry = mapEntry;
    pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->seasonId = seasonId;

    MapEntry const* map = sMapStore.LookupEntry(mapEntry);
    ASSERT(map);

    LocaleConstant locale = player->GetSession()->GetSessionDbcLocale();
    uint32 id = 1;
    MythicPlus::MythicPlusSeason const* season = seasonId > 0 ? sMythicPlus->GetSeason(seasonId) : sMythicPlus->GetActiveSeason();

    Identifier* headerIdnt = new Identifier();
    headerIdnt->id = id++;
    headerIdnt->uiName = std::string(seasonId > 0 ? "Archived leaderboard for " : "Monthly leaderboard for ") + map->name[locale];
    if (season)
        headerIdnt->uiName += " [" + season->label + "]";
    pagedData.data.push_back(headerIdnt);

    std::vector<MythicPlus::MythicPlusLeaderboardEntry> entries = sMythicPlus->GetMapLeaderboard(mapEntry, 50, seasonId);
    if (entries.empty())
    {
        Identifier* emptyIdnt = new Identifier();
        emptyIdnt->id = id++;
        emptyIdnt->uiName = MythicPlus::Utils::RedColored("No ranked runs yet for this dungeon");
        pagedData.data.push_back(emptyIdnt);
    }
    else
    {
        for (std::size_t i = 0; i < entries.size(); ++i)
        {
            Identifier* idnt = new Identifier();
            idnt->id = id++;
            std::ostringstream oss;
            oss << i + 1 << ". " << entries[i].charName;
            oss << " [SCORE: " << entries[i].score << "]";
            oss << " [+" << entries[i].mythicLevel << "]";
            oss << " [" << secsToTimeString(entries[i].bestTime) << "]";
            oss << " [DEATHS: " << entries[i].deaths << "]";
            oss << (entries[i].completedInTime ? MythicPlus::Utils::GreenColored(" [TIMED]") : MythicPlus::Utils::RedColored(" [OVERTIME]"));
            idnt->uiName = oss.str();
            pagedData.data.push_back(idnt);
        }
    }

    pagedData.SortAndCalculateTotals(CompareIdentifierById);
}

void MythicPlusNpcSupport::AddMythicPlusLevels(Player* player)
{
    PagedData& pagedData = GetPagedData(player);
    pagedData.Reset();
    pagedData.type = GossipSupport::PAGED_DATA_TYPE_MYTHIC_LEVELS;

    const MythicLevelContainer& mythicLevels = sMythicPlus->GetAllMythicLevels();
    for (const auto& mlevel : mythicLevels)
    {
        Identifier* idnt = new Identifier();
        idnt->id = mlevel.level;
        idnt->optionIcon = GOSSIP_ICON_BATTLE;
        std::ostringstream oss;
        oss << "Mythic level " << mlevel.level;
        oss << " (" << mlevel.affixes.size() << " affix(es))";
        if (mlevel.randomAffixCount > 0)
            oss << " (" << mlevel.randomAffixCount << " random affix(es))";
        oss << " -->";
        idnt->uiName = oss.str();
        pagedData.data.push_back(idnt);
    }

    pagedData.SortAndCalculateTotals(CompareIdentifierById);
}

void MythicPlusNpcSupport::AddMythicPlusLevelInfo(Player* player, uint32 mythicLevel)
{
    PagedData& pagedData = GetPagedData(player);
    pagedData.Reset();
    pagedData.type = GossipSupport::PAGED_DATA_TYPE_MYTHIC_LEVEL_INFO;
    pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->mythicLevel = mythicLevel;

    uint32 id = 0;

    Identifier* idnt = new Identifier();
    idnt->id = ++id;
    idnt->optionIcon = GOSSIP_ICON_BATTLE;
    idnt->uiName = MythicPlus::Utils::Colored("Click to choose Mythic Level " + Acore::ToString(mythicLevel), "0a4a0e");
    pagedData.data.push_back(idnt);

    const MythicLevel* level = sMythicPlus->GetMythicLevel(mythicLevel);
    ASSERT(level);

    Identifier* timerIdnt = new Identifier();
    timerIdnt->id = ++id;
    timerIdnt->uiName = "Time limit to get rewards: " + secsToTimeString(level->timeLimit);
    pagedData.data.push_back(timerIdnt);

    for (size_t i = 0; i < level->affixes.size(); ++i)
    {
        const MythicAffix* affix = level->affixes[i];

        Identifier* affixIdnt = new Identifier();
        affixIdnt->id = ++id;
        std::ostringstream oss;
        oss << "Affix ";
        oss << i + 1 << ": ";
        oss << affix->ToString();
        if (affix->IsRandom())
            oss << MythicPlus::Utils::Colored(" [RANDOMLY GENERATED]", "1a0966");
        affixIdnt->uiName = oss.str();
        pagedData.data.push_back(affixIdnt);
    }

    Identifier* rewardsIdnt = new Identifier();
    rewardsIdnt->id = ++id;
    rewardsIdnt->optionIcon = GOSSIP_ICON_MONEY_BAG;
    rewardsIdnt->uiName = MythicPlus::Utils::Colored("-- REWARDS --", "0d852d");
    pagedData.data.push_back(rewardsIdnt);

    const MythicReward& reward = level->reward;
    if (reward.money)
    {
        Identifier* moneyIdnt = new Identifier();
        moneyIdnt->id = ++id;
        moneyIdnt->optionIcon = GOSSIP_ICON_MONEY_BAG;
        moneyIdnt->uiName = "Gold: " + MythicPlus::Utils::CopperToMoneyStr(reward.money, false);
        pagedData.data.push_back(moneyIdnt);
    }

    if (!reward.tokens.empty())
    {
        for (const auto& token : reward.tokens)
        {
            Identifier* tokenIdnt = new Identifier();
            tokenIdnt->id = ++id;
            tokenIdnt->optionIcon = GOSSIP_ICON_VENDOR;
            std::ostringstream oss;
            oss << MythicPlus::Utils::ItemLinkForUI(token.first, player);
            oss << " - " << token.second << "x";
            tokenIdnt->uiName = oss.str();
            pagedData.data.push_back(tokenIdnt);
        }
    }

    pagedData.SortAndCalculateTotals(CompareIdentifierById);
}

void MythicPlusNpcSupport::AddMythicPlusDungeonList(Player* player)
{
    PagedData& pagedData = GetPagedData(player);
    pagedData.Reset();
    pagedData.type = GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_LIST;

    const std::unordered_map<uint32, MythicPlus::MythicPlusCapableDungeon>& dungeons = sMythicPlus->GetAllMythicPlusDungeons();
    LocaleConstant locale = player->GetSession()->GetSessionDbcLocale();
    uint32 id = 0;
    for (const auto& dpair : dungeons)
    {
        uint32 mapEntry = dpair.first;
        Difficulty diff = dpair.second.minDifficulty;

        MapEntry const* map = sMapStore.LookupEntry(mapEntry);
        ASSERT(map);

        Identifier* idnt = new Identifier();
        idnt->id = ++id;
        std::ostringstream oss;
        oss << map->name[locale];
        oss << " [";
        if (diff == DUNGEON_DIFFICULTY_NORMAL)
        {
            if (MythicPlus::Utils::CanBeHeroic(mapEntry))
                oss << "NORMAL/HEROIC]";
            else
                oss << "NORMAL]";
        }
        else
            oss << "HEROIC ONLY]";
        idnt->uiName = oss.str();
        pagedData.data.push_back(idnt);
    }

    pagedData.SortAndCalculateTotals(CompareIdentifierById);
}

void MythicPlusNpcSupport::AddMythicPlusDungeonListForSnapshots(Player* player, uint32 snapMythicLevel)
{
    PagedData& pagedData = GetPagedData(player);
    pagedData.Reset();
    pagedData.type = GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_LIST_SNAPSHOT;
    pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->snapMythicLevel = snapMythicLevel;

    Identifier* mlevelIdnt = new Identifier();
    mlevelIdnt->id = 1;
    std::ostringstream oss;
    oss << "Selected Mythic Plus level: ";
    if (snapMythicLevel == 0)
        oss << "ALL";
    else
        oss << snapMythicLevel;
    mlevelIdnt->uiName = oss.str();
    mlevelIdnt->optionIcon = GOSSIP_ICON_CHAT;
    pagedData.data.push_back(mlevelIdnt);

    const std::unordered_map<uint32, MythicPlus::MythicPlusCapableDungeon>& dungeons = sMythicPlus->GetAllMythicPlusDungeons();
    LocaleConstant locale = player->GetSession()->GetSessionDbcLocale();
    for (const auto& dpair : dungeons)
    {
        uint32 mapEntry = dpair.first;

        MapEntry const* map = sMapStore.LookupEntry(mapEntry);
        ASSERT(map);

        const std::vector<std::pair<std::pair<uint32, uint64>, std::vector<MythicPlus::MythicPlusDungeonSnapshot>>> snapshots = sMythicPlus->GetMapSnapshot(mapEntry, snapMythicLevel);

        Identifier* idnt = new Identifier();
        idnt->id = mapEntry;
        std::ostringstream oss;
        oss << map->name[locale];
        oss << " [TOTAL RUNS: ";
        if (snapshots.empty())
            oss << MythicPlus::Utils::Colored("NONE", "b50505");
        else
            oss << snapshots.size();
        oss << "]";
        idnt->uiName = oss.str();
        pagedData.data.push_back(idnt);
    }

    pagedData.SortAndCalculateTotals(CompareIdentifierById);
}

void MythicPlusNpcSupport::AddMythicPlusSnapshotAllRuns(Player* player, uint32 mapEntry)
{
    PagedData& pagedData = GetPagedData(player);
    pagedData.Reset();
    pagedData.type = GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_LIST_SNAPSHOT_RUNS;
    pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->mapEntry = mapEntry;

    MapEntry const* map = sMapStore.LookupEntry(mapEntry);
    ASSERT(map);

    LocaleConstant locale = player->GetSession()->GetSessionDbcLocale();

    Identifier* mapIdnt = new Identifier();
    mapIdnt->id = 1;
    std::ostringstream oss;
    oss << "Mythic Plus top timers for ";
    oss << map->name[locale];
    mapIdnt->uiName = oss.str();
    pagedData.data.push_back(mapIdnt);

    uint32 snapMythicLevel = pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->snapMythicLevel;

    const std::vector<std::pair<std::pair<uint32, uint64>, std::vector<MythicPlus::MythicPlusDungeonSnapshot>>> snapshots = sMythicPlus->GetMapSnapshot(mapEntry, snapMythicLevel);
    if (!snapshots.empty())
    {
        uint32 id = 1;
        for (const auto& s : snapshots)
        {
            const MythicPlus::MythicPlusDungeonSnapshot& snap = s.second.at(0);
            uint32 internalId = snap.internalId;

            Identifier* idnt = new Identifier();
            idnt->id = internalId + 10;
            std::ostringstream oss;
            oss << id++ << ". ";
            if (snap.totalTime > 0)
            {
                oss << secsToTimeString(snap.totalTime);
                oss << " [LIMIT: ";
                oss << secsToTimeString(snap.timelimit);
                oss << "]";
                if (snap.rewarded)
                    oss << MythicPlus::Utils::GreenColored(" [REWARDED]");
                else
                    oss << MythicPlus::Utils::RedColored(" [NOT REWARDED]");
            }
            else
            {
                oss << MythicPlus::Utils::RedColored("NOT FINISHED");
                oss << " [LIMIT: ";
                oss << secsToTimeString(snap.timelimit);
                oss << "]";
            }
            oss << " [M+ LEVEL ";
            oss << snap.mythicLevel;
            oss << "]";
            if (snap.difficulty == DUNGEON_DIFFICULTY_NORMAL)
                oss << " [NORMAL]";
            else
                oss << MythicPlus::Utils::Colored(" [HEROIC]", "9e1849");
            idnt->uiName = oss.str();
            pagedData.data.push_back(idnt);
        }
    }

    pagedData.SortAndCalculateTotals(CompareIdentifierById);
}

void MythicPlusNpcSupport::AddMythicPlusAllLevels(Player* player)
{
    PagedData& pagedData = GetPagedData(player);
    pagedData.Reset();
    pagedData.type = GossipSupport::PAGED_DATA_TYPE_MYTHIC_ALL_LEVELS;

    Identifier* allIdnt = new Identifier();
    allIdnt->id = 0;
    allIdnt->uiName = "ALL Mythic Plus levels";
    pagedData.data.push_back(allIdnt);

    const MythicLevelContainer& mythicLevels = sMythicPlus->GetAllMythicLevels();
    for (const auto& mlevel : mythicLevels)
    {
        Identifier* idnt = new Identifier();
        idnt->id = mlevel.level;
        idnt->optionIcon = GOSSIP_ICON_BATTLE;
        std::ostringstream oss;
        oss << "Mythic level " << mlevel.level << " -- >";
        idnt->uiName = oss.str();
        pagedData.data.push_back(idnt);
    }

    pagedData.SortAndCalculateTotals(CompareIdentifierById);
}

void MythicPlusNpcSupport::AddMythicPlusDungeonSnapshotDetails(Player* player, uint32 internalId)
{
    PagedData& pagedData = GetPagedData(player);
    pagedData.Reset();
    pagedData.type = GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_SNAPSHOT_DETAILS;
    pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->internalId = internalId;

    uint32 mapEntry = pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->mapEntry;
    uint32 snapMythicLevel = pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->snapMythicLevel;
    const std::vector<std::pair<std::pair<uint32, uint64>, std::vector<MythicPlus::MythicPlusDungeonSnapshot>>> snapshots = sMythicPlus->GetMapSnapshot(mapEntry, snapMythicLevel);

    std::vector<MythicPlus::MythicPlusDungeonSnapshot> chosenSnaps;
    for (const auto& s : snapshots)
    {
        if (s.second.at(0).internalId == internalId)
        {
            chosenSnaps = s.second;
            break;
        }
    }
    if (chosenSnaps.empty())
        return;

    std::sort(chosenSnaps.begin(), chosenSnaps.end(), [](const MythicPlus::MythicPlusDungeonSnapshot& a, const MythicPlus::MythicPlusDungeonSnapshot& b) {
        return a.snapTime < b.snapTime;
    });

    MapEntry const* map = sMapStore.LookupEntry(mapEntry);
    ASSERT(map);

    LocaleConstant locale = player->GetSession()->GetSessionDbcLocale();

    const MythicPlus::MythicPlusDungeonSnapshot* csnap = &chosenSnaps.at(0);

    Identifier* idnt = new Identifier();
    idnt->id = 1;
    std::ostringstream oss;
    oss << "Mythic Plus run for ";
    oss << map->name[locale];
    oss << " at level ";
    oss << csnap->mythicLevel;
    idnt->uiName = oss.str();
    pagedData.data.push_back(idnt);

    oss.str("");
    oss.clear();

    Identifier* startTimeIdnt = new Identifier();
    startTimeIdnt->id = 2;
    oss << "Run started at ";
    oss << MythicPlus::Utils::DateFromSeconds(csnap->startTime);
    oss << " [UTC TIME]";
    startTimeIdnt->uiName = oss.str();
    pagedData.data.push_back(startTimeIdnt);

    oss.str("");
    oss.clear();

    Identifier* endTimeIdnt = new Identifier();
    endTimeIdnt->id = 3;
    if (csnap->totalTime > 0)
    {
        oss << "Run ended at ";
        oss << MythicPlus::Utils::DateFromSeconds(csnap->endTime);
        oss << " [UTC TIME]";
        oss << " [DURATION: ";
        oss << secsToTimeString(csnap->totalTime);
        oss << "]";
    }
    else
        oss << MythicPlus::Utils::RedColored("Run did not end (yet, or orphaned instance)");
    endTimeIdnt->uiName = oss.str();
    pagedData.data.push_back(endTimeIdnt);

    Identifier* deathsIdnt = new Identifier();
    deathsIdnt->id = 4;
    oss.str("");
    oss.clear();
    if (csnap->totalDeaths > 0)
    {
        oss << "Deaths: ";
        oss << MythicPlus::Utils::RedColored(Acore::ToString(csnap->totalDeaths));
        oss << ". Time penalty: ";
        oss << secsToTimeString(csnap->penaltyOnDeath * csnap->totalDeaths);
    }
    else
        oss << MythicPlus::Utils::GreenColored("NO DEATHS");
    deathsIdnt->uiName = oss.str();
    pagedData.data.push_back(deathsIdnt);

    uint32 id = 4;
    for (const auto& s : chosenSnaps)
    {
        oss.str("");
        oss.clear();

        oss << MythicPlus::Utils::Colored(MythicPlus::Utils::GetCreatureNameByEntry(player, s.entry), "102163");
        oss << " downed at ";
        oss << MythicPlus::Utils::DateFromSeconds(s.snapTime);
        oss << " [took ";
        oss << secsToTimeString(s.combatTime);
        oss << "]";
        oss << " [PLAYERS: ";
        oss << MythicPlus::Utils::Colored(s.players, "6e1849");
        oss << "]";

        if (s.randomAffixCount > 0)
            oss << " [RANDOM AFFIXES: " << s.randomAffixCount << "]";

        Identifier* idnt = new Identifier();
        idnt->id = ++id;
        idnt->uiName = oss.str();
        pagedData.data.push_back(idnt);
    }

    if (csnap->totalTime > 0)
    {
        Identifier* rewardIdnt = new Identifier();
        rewardIdnt->id = ++id;
        if (csnap->rewarded)
            rewardIdnt->uiName = MythicPlus::Utils::GreenColored("TIMER WAS BEATEN - REWARDS RECEIVED");
        else
            rewardIdnt->uiName = MythicPlus::Utils::RedColored("NO REWARDS RECEIVED - TIMER LIMIT EXCEEDED");
        pagedData.data.push_back(rewardIdnt);
    }

    pagedData.SortAndCalculateTotals(CompareIdentifierById);
}

void MythicPlusNpcSupport::AddRandomAfixes(Player* player)
{
    PagedData& pagedData = GetPagedData(player);
    pagedData.Reset();
    pagedData.type = GossipSupport::PAGED_DATA_TYPE_RANDOM_AFFIXES;

    uint32 id = 1;

    Identifier* infoIdnt = new Identifier();
    infoIdnt->id = id++;
    infoIdnt->uiName = "Some Mythic levels can have rotating affix slots. These affixes are loaded deterministically from the active season/rotation window instead of changing on every server restart.";
    infoIdnt->optionIcon = GOSSIP_ICON_CHAT;
    pagedData.data.push_back(infoIdnt);

    const MythicLevelContainer& mythicLevels = sMythicPlus->GetAllMythicLevels();
    for (const auto& mlevel : mythicLevels)
    {
        if (mlevel.randomAffixCount > 0)
        {
            Identifier* idnt = new Identifier();
            idnt->id = 100 + mlevel.level;
            std::ostringstream oss;
            oss << "Mythic level ";
            oss << mlevel.level;
            oss << " has ";
            oss << mlevel.randomAffixCount << " random affixes set -->";
            idnt->uiName = oss.str();
            pagedData.data.push_back(idnt);
        }
    }

    Identifier* affixesInfoIdnt = new Identifier();
    affixesInfoIdnt->id = 1000 + (id++);
    affixesInfoIdnt->uiName = "Pool of random affixes:";
    affixesInfoIdnt->optionIcon = GOSSIP_ICON_CHAT;
    pagedData.data.push_back(affixesInfoIdnt);

    for (uint32 i = 0; i < MythicAffix::RANDOM_AFFIX_MAX_COUNT; i++)
    {
        MythicAffix* affix = MythicAffix::AffixFactory((MythicAffixType)MythicAffix::RandomAffixes[i]);
        ASSERT(affix && affix->IsRandom());

        Identifier* affixIdnt = new Identifier();
        affixIdnt->id = 1000 + (id++);
        std::ostringstream aoss;
        aoss << (i + 1) << ". ";
        aoss << MythicPlus::Utils::Colored(affix->ToString(), "1a0966");
        affixIdnt->uiName = aoss.str();
        pagedData.data.push_back(affixIdnt);

        delete affix;
    }

    pagedData.SortAndCalculateTotals(CompareIdentifierById);
}

void MythicPlusNpcSupport::AddRandomAffixesForLevel(Player* player, uint32 level)
{
    PagedData& pagedData = GetPagedData(player);
    pagedData.Reset();
    pagedData.type = GossipSupport::PAGED_DATA_TYPE_RANDOM_AFFIXES_FOR_LEVEL;
    pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->randomMythicLevel = level;

    uint32 id = 1;
    Identifier* levelIdnt = new Identifier();
    levelIdnt->id = id++;
    levelIdnt->uiName = "Randomly generated affixes for Mythic Level " + Acore::ToString(level);
    levelIdnt->optionIcon = GOSSIP_ICON_CHAT;
    pagedData.data.push_back(levelIdnt);

    const MythicLevel* mythicLevel = sMythicPlus->GetMythicLevel(level);
    ASSERT(mythicLevel);

    uint32 affixIndex = 1;
    for (const auto* a : mythicLevel->affixes)
    {
        if (a->IsRandom())
        {
            Identifier* affixIdnt = new Identifier();
            affixIdnt->id = id++;
            std::ostringstream aoss;
            aoss << affixIndex++ << ". ";
            aoss << MythicPlus::Utils::Colored(a->ToString(), "1a0966");
            affixIdnt->uiName = aoss.str();
            pagedData.data.push_back(affixIdnt);
        }
    }

    pagedData.SortAndCalculateTotals(CompareIdentifierById);
}

bool MythicPlusNpcSupport::TakePagedDataAction(Player* player, Creature* creature, uint32 action)
{
    PagedData& pagedData = GetPagedData(player);
    if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_NPC_MENU)
    {
        if (action == 1)
        {
            AddMythicPlusLevels(player);
            return AddPagedData(player, creature, 0);
        }
        else if (action == 2)
        {
            if (sMythicPlus->SetCurrentMythicPlusLevel(player, 0))
            {
                MythicPlus::BroadcastToPlayer(player, "Your Mythic Plus level was reset!");
                MythicPlus::Utils::VisualFeedback(player);
            }
            else
                MythicPlus::BroadcastToPlayer(player, "You can't reset your Mythic Plus level while in a group.");

            CloseGossipMenuFor(player);
            return true;
        }
        else if (action == 3 || action == 4 || action == 6 || action == 0)
            return OnGossipHello(player, creature);
        else if (action == 5)
        {
            AddMythicPlusDungeonList(player);
            return AddPagedData(player, creature, 0);
        }
        else if (action == 7)
        {
            AddMythicPlusAllLevels(player);
            return AddPagedData(player, creature, 0);
        }
        else if (action == 8)
        {
            if (sMythicPlus->GiveKeystone(player))
            {
                CloseGossipMenuFor(player);
                return true;
            }
            else
                return OnGossipHello(player, creature);
        }
        else if (action == 9)
        {
            AddRandomAfixes(player);
            return AddPagedData(player, creature, 0);
        }
        else if (action == 10)
        {
            CloseGossipMenuFor(player);
            return true;
        }
        else if (action == 11)
        {
            AddSeasonInfo(player);
            return AddPagedData(player, creature, 0);
        }
        else if (action == 12)
        {
            pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->seasonId = 0;
            AddOverallLeaderboard(player);
            return AddPagedData(player, creature, 0);
        }
        else if (action == 13)
        {
            AddDungeonListForLeaderboard(player, 0);
            return AddPagedData(player, creature, 0);
        }
    }
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_LEVELS)
    {
        AddMythicPlusLevelInfo(player, action);
        return AddPagedData(player, creature, 0);
    }
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_LEVEL_INFO)
    {
        uint32 chosenMythicLevel = pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->mythicLevel;
        if (action == 1)
        {
            if (sMythicPlus->SetCurrentMythicPlusLevel(player, chosenMythicLevel))
            {
                MythicPlus::BroadcastToPlayer(player, "Your Mythic Plus level was set to " + Acore::ToString(chosenMythicLevel));
                MythicPlus::Utils::VisualFeedback(player);
            }
            else
                MythicPlus::BroadcastToPlayer(player, "You can't set your Mythic Plus level while in a group.");

            CloseGossipMenuFor(player);
            return true;
        }
        else
        {
            AddMythicPlusLevelInfo(player, chosenMythicLevel);
            return AddPagedData(player, creature, pagedData.currentPage);
        }
    }
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_LIST)
    {
        AddMythicPlusDungeonList(player);
        return AddPagedData(player, creature, pagedData.currentPage);
    }
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_LIST_SNAPSHOT)
    {
        if (action == 1)
        {
            AddMythicPlusDungeonListForSnapshots(player, pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->snapMythicLevel);
            return AddPagedData(player, creature, pagedData.currentPage);
        }
        AddMythicPlusSnapshotAllRuns(player, action);
        return AddPagedData(player, creature, 0);
    }
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_ALL_LEVELS)
    {
        AddMythicPlusDungeonListForSnapshots(player, action);
        return AddPagedData(player, creature, 0);
    }
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_LIST_SNAPSHOT_RUNS)
    {
        if (action == 1)
        {
            AddMythicPlusSnapshotAllRuns(player, pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->mapEntry);
            return AddPagedData(player, creature, pagedData.currentPage);
        }
        else
        {
            AddMythicPlusDungeonSnapshotDetails(player, action - 10);
            return AddPagedData(player, creature, 0);
        }
    }
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_SNAPSHOT_DETAILS)
    {
        AddMythicPlusDungeonSnapshotDetails(player, pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->internalId);
        return AddPagedData(player, creature, pagedData.currentPage);
    }
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_RANDOM_AFFIXES)
    {
        if (action >= 100 && action <= 1000)
        {
            uint32 level = action - 100;
            AddRandomAffixesForLevel(player, level);
            return AddPagedData(player, creature, 0);
        }
        else
        {
            AddRandomAfixes(player);
            return AddPagedData(player, creature, pagedData.currentPage);
        }
    }
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_RANDOM_AFFIXES_FOR_LEVEL)
    {
        AddRandomAffixesForLevel(player, pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->randomMythicLevel);
        return AddPagedData(player, creature, pagedData.currentPage);
    }
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_SEASON_INFO)
    {
        if (action >= 100000)
        {
            AddSeasonArchiveMenu(player, action - 100000);
            return AddPagedData(player, creature, 0);
        }

        AddSeasonInfo(player);
        return AddPagedData(player, creature, pagedData.currentPage);
    }
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_SEASON_ARCHIVE_MENU)
    {
        uint32 seasonId = pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->seasonId;
        if (action == 3)
        {
            AddOverallLeaderboard(player);
            return AddPagedData(player, creature, 0);
        }
        else if (action == 4)
        {
            AddDungeonListForLeaderboard(player, seasonId);
            return AddPagedData(player, creature, 0);
        }

        AddSeasonArchiveMenu(player, seasonId);
        return AddPagedData(player, creature, pagedData.currentPage);
    }
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_OVERALL_LEADERBOARD)
    {
        AddOverallLeaderboard(player);
        return AddPagedData(player, creature, pagedData.currentPage);
    }
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_LIST_LEADERBOARD)
    {
        AddMapLeaderboard(player, action, pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->seasonId);
        return AddPagedData(player, creature, 0);
    }
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_MAP_LEADERBOARD)
    {
        AddMapLeaderboard(player,
            pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->mapEntry,
            pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->seasonId);
        return AddPagedData(player, creature, pagedData.currentPage);
    }

    return GossipSupport::TakePagedDataAction(player, creature, action);
}

/*static*/ bool MythicPlusNpcSupport::CompareIdentifierById(const Identifier* a, const Identifier* b)
{
    return a->id < b->id;
}

uint32 MythicPlusNpcSupport::_PageZeroSender(const PagedData& pagedData) const
{
    MythicPlusNpcPageInfo const* pageInfo = pagedData.customInfo ? static_cast<MythicPlusNpcPageInfo const*>(pagedData.customInfo) : nullptr;

    if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_LEVELS
        || pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_LIST
        || pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_ALL_LEVELS
        || pagedData.type == GossipSupport::PAGED_DATA_TYPE_RANDOM_AFFIXES)
        return GOSSIP_SENDER_MAIN;
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_LEVEL_INFO)
        return GOSSIP_SENDER_MAIN + 9;
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_LIST_SNAPSHOT)
        return GOSSIP_SENDER_MAIN + 10;
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_LIST_SNAPSHOT_RUNS)
        return GOSSIP_SENDER_MAIN + 11;
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_SNAPSHOT_DETAILS)
        return GOSSIP_SENDER_MAIN + 12;
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_RANDOM_AFFIXES_FOR_LEVEL)
        return GOSSIP_SENDER_MAIN + 13;
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_SEASON_INFO)
        return GOSSIP_SENDER_MAIN + 14;
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_SEASON_ARCHIVE_MENU)
        return GOSSIP_SENDER_MAIN + 15;
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_OVERALL_LEADERBOARD)
        return pageInfo && pageInfo->seasonId > 0 ? GOSSIP_SENDER_MAIN + 19 : GOSSIP_SENDER_MAIN;
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_LIST_LEADERBOARD)
        return pageInfo && pageInfo->seasonId > 0 ? GOSSIP_SENDER_MAIN + 19 : GOSSIP_SENDER_MAIN;
    else if (pagedData.type == GossipSupport::PAGED_DATA_TYPE_MYTHIC_DUNGEON_MAP_LEADERBOARD)
        return GOSSIP_SENDER_MAIN + 18;

    return GOSSIP_SENDER_MAIN;
}

bool MythicPlusNpcSupport::OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
{
    PagedData& pagedData = GetPagedData(player);
    if (sender <= GOSSIP_SENDER_MAIN + 2)
        return GossipSupport::OnGossipSelect(player, creature, sender, action);
    else if (sender == GOSSIP_SENDER_MAIN + 9)
    {
        AddMythicPlusLevels(player);
        return AddPagedData(player, creature, 0);
    }
    else if (sender == GOSSIP_SENDER_MAIN + 10)
    {
        AddMythicPlusAllLevels(player);
        return AddPagedData(player, creature, 0);
    }
    else if (sender == GOSSIP_SENDER_MAIN + 11)
    {
        AddMythicPlusDungeonListForSnapshots(player, pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->snapMythicLevel);
        return AddPagedData(player, creature, 0);
    }
    else if (sender == GOSSIP_SENDER_MAIN + 12)
    {
        AddMythicPlusSnapshotAllRuns(player, pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->mapEntry);
        return AddPagedData(player, creature, 0);
    }
    else if (sender == GOSSIP_SENDER_MAIN + 13)
    {
        AddRandomAfixes(player);
        return AddPagedData(player, creature, 0);
    }
    else if (sender == GOSSIP_SENDER_MAIN + 14)
    {
        AddSeasonInfo(player);
        return AddPagedData(player, creature, 0);
    }
    else if (sender == GOSSIP_SENDER_MAIN + 15)
    {
        AddSeasonInfo(player);
        return AddPagedData(player, creature, 0);
    }
    else if (sender == GOSSIP_SENDER_MAIN + 16)
    {
        AddOverallLeaderboard(player);
        return AddPagedData(player, creature, 0);
    }
    else if (sender == GOSSIP_SENDER_MAIN + 17)
    {
        AddDungeonListForLeaderboard(player, pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->seasonId);
        return AddPagedData(player, creature, 0);
    }
    else if (sender == GOSSIP_SENDER_MAIN + 18)
    {
        AddDungeonListForLeaderboard(player, pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->seasonId);
        return AddPagedData(player, creature, 0);
    }
    else if (sender == GOSSIP_SENDER_MAIN + 19)
    {
        AddSeasonArchiveMenu(player, pagedData.GetCustomInfo<MythicPlusNpcPageInfo>()->seasonId);
        return AddPagedData(player, creature, 0);
    }

    return false;
}
