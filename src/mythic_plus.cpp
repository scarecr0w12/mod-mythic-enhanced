/*
 * Credits: silviu20092
 */

#include <algorithm>
#include <cmath>
#include <ctime>
#include <iomanip>
#include "AreaDefines.h"
#include "Chat.h"
#include "Group.h"
#include "Config.h"
#include "GameTime.h"
#include "Item.h"
#include "Log.h"
#include "Mail.h"
#include "ObjectMgr.h"
#include "mythic_affix.h"
#include "mythic_plus.h"

namespace
{
std::tm GetUtcTimeBreakdown(std::time_t timeValue)
{
    std::tm utcTime{};

#if AC_PLATFORM == AC_PLATFORM_WINDOWS
    gmtime_s(&utcTime, &timeValue);
#else
    gmtime_r(&timeValue, &utcTime);
#endif

    return utcTime;
}

uint64 BuildUtcMonthStart(uint32 year, uint32 month)
{
    std::tm utcTime{};
    utcTime.tm_year = year - 1900;
    utcTime.tm_mon = month - 1;
    utcTime.tm_mday = 1;
    utcTime.tm_hour = 0;
    utcTime.tm_min = 0;
    utcTime.tm_sec = 0;

#if AC_PLATFORM == AC_PLATFORM_WINDOWS
    return uint64(_mkgmtime(&utcTime));
#else
    return uint64(timegm(&utcTime));
#endif
}

std::string BuildSeasonLabel(uint32 year, uint32 month)
{
    std::ostringstream oss;
    oss << year << '-' << std::setw(2) << std::setfill('0') << month;
    return oss.str();
}

std::string BuildDefaultSeasonRewardSubject(MythicPlus::MythicPlusSeason const& season, uint32 rank)
{
    std::ostringstream oss;
    oss << "Mythic season " << season.label << " rewards";
    if (rank > 0)
        oss << " (rank #" << rank << ')';

    return oss.str();
}

std::string BuildDefaultSeasonRewardBody(MythicPlus::MythicPlusSeason const& season,
    MythicPlus::MythicPlusOverallLeaderboardEntry const& entry, uint32 rank)
{
    std::ostringstream oss;
    oss << "Congratulations, " << entry.charName << "!\n\n";
    oss << "You finished Mythic season " << season.label << " at rank #" << rank;
    oss << " with a total score of " << entry.totalScore << ".\n";
    oss << "Your best key this season was +" << entry.bestLevel << ".\n\n";
    oss << "Your seasonal rewards are attached to this mail.";
    return oss.str();
}

constexpr uint32 MYTHIC_PLUS_MAIL_SENDER_ENTRY = 34337; // The Postmaster (core item recovery sender)

// Try bags first; on failure send stacks via mail (handles count above max stack and >1 mail if needed).
bool DeliverMythicStacksOrMail(Player* player, uint32 itemId, uint32 count, char const* subject, char const* body)
{
    if (!player || count == 0)
        return false;

    ItemTemplate const* itemProto = sObjectMgr->GetItemTemplate(itemId);
    if (!itemProto)
        return false;

    if (player->AddItem(itemId, count))
        return true;

    uint32 remaining = count;
    uint32 const maxStack = itemProto->GetMaxStackSize();

    while (remaining > 0)
    {
        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
        MailDraft draft(subject, body);

        for (uint32 mailSlot = 0; mailSlot < MAX_MAIL_ITEMS && remaining > 0; ++mailSlot)
        {
            uint32 const chunk = std::min(remaining, maxStack);
            Item* mailItem = Item::CreateItem(itemId, chunk, player);
            if (!mailItem)
            {
                LOG_ERROR("module", "MythicPlus: Item::CreateItem failed (entry {} count {}) for {}", itemId, chunk, player->GetName());
                CharacterDatabase.CommitTransaction(trans);
                return false;
            }

            mailItem->SaveToDB(trans);
            draft.AddItem(mailItem);
            remaining -= chunk;
        }

        draft.SendMailTo(trans, player, MailSender(MAIL_CREATURE, MYTHIC_PLUS_MAIL_SENDER_ENTRY));
        CharacterDatabase.CommitTransaction(trans);
    }

    return true;
}
}

MythicPlus::MythicPlus()
{
    enabled = true;
    penaltyOnDeath = 15;
    keystoneBuyTimer = 1440;
    dropKeystoneOnCompletion = true;
}

MythicPlus::~MythicPlus()
{
    ClearMythicLevels();
}

MythicPlus* MythicPlus::instance()
{
    static MythicPlus instance;
    return &instance;
}

MythicPlus::MapData* MythicPlus::GetMapData(Map* map, bool withDefault) const
{
    if (withDefault)
        return map->CustomData.GetDefault<MapData>("mythicPlusMapData");

    return map->CustomData.Get<MapData>("mythicPlusMapData");
}

MythicPlus::CreatureData* MythicPlus::GetCreatureData(Creature* creature, bool withDefault) const
{
    if (withDefault)
        return creature->CustomData.GetDefault<CreatureData>("mythicPlusCreatureData");

    return creature->CustomData.Get<CreatureData>("mythicPlusCreatureData");
}

/*static*/ void MythicPlus::BroadcastToPlayer(const Player* player, const std::string& message)
{
    std::ostringstream oss;
    oss << "|cffa11585[MYTHIC PLUS]|r: ";
    oss << message;
    ChatHandler(player->GetSession()).SendSysMessage(oss.str());
}

/*static*/ void MythicPlus::AnnounceToPlayer(const Player* player, const std::string& message)
{
    ChatHandler(player->GetSession()).SendNotification(message);
}

/*static*/ void MythicPlus::AnnounceToGroup(const Player* player, const std::string& message)
{
    const Group* group = player->GetGroup();
    if (group == nullptr)
    {
        AnnounceToPlayer(player, message);
        return;
    }

    for (const GroupReference* groupRef = group->GetFirstMember(); groupRef != nullptr; groupRef = groupRef->next())
    {
        const Player* groupPlayer = groupRef->GetSource();
        if (groupPlayer && groupPlayer->IsInWorld())
            AnnounceToPlayer(groupPlayer, message);
    }
}

/*static*/ void MythicPlus::AnnounceToMap(const Map* map, const std::string& message)
{
    Map::PlayerList const& playerList = map->GetPlayers();
    if (playerList.IsEmpty())
        return;

    for (Map::PlayerList::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
        if (Player* player = itr->GetSource())
            AnnounceToPlayer(player, message);
}

/*static*/ void MythicPlus::BroadcastToMap(const Map* map, const std::string& message)
{
    Map::PlayerList const& playerList = map->GetPlayers();
    if (playerList.IsEmpty())
        return;

    for (Map::PlayerList::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
        if (Player* player = itr->GetSource())
            BroadcastToPlayer(player, message);
}

/*static*/ void MythicPlus::FallbackTeleport(Player* player)
{
    player->TeleportToEntryPoint();
}

bool MythicPlus::CanBeMythicPlus(const MapEntry* mapEntry) const
{
    if (!mapEntry)
        return false;

    if (!mapEntry->IsNonRaidDungeon())
        return false;

    return IsAllowedMythicPlusDungeon(mapEntry->MapID);
}

bool MythicPlus::CanMapBeMythicPlus(const Map* map) const
{
    return IsEnabled() && MatchMythicPlusMapDiff(map);
}

bool MythicPlus::CanProcessCreature(const Creature* creature) const
{
    if (!creature)
        return false;

    if (creature->GetEntry() == NPC_LIGHTNING_SPHERE)
        return false;

    Map* map = creature->GetMap();
    if (!map || !map->IsNonRaidDungeon() || !map->ToInstanceMap())
        return false;

    // lets assume that mobs under level 5 should not be touched
    if (creature->GetLevel() < 5)
        return false;

    if ((creature->IsHunterPet() || creature->IsPet() || creature->IsSummon()) && creature->IsControlledByPlayer())
        return false;

    if (creature->IsCritter() || creature->IsTotem() || creature->IsTrigger())
        return false;

    if (creature->ToTempSummon() &&
        creature->ToTempSummon()->GetSummoner() &&
        creature->ToTempSummon()->GetSummoner()->ToPlayer())
        return false;

    return true;
}

void MythicPlus::StoreOriginalCreatureData(Creature* creature) const
{
    CreatureData* creatureData = GetCreatureData(creature);
    creatureData->originalCreateHealth = creature->GetCreateHealth();
    creatureData->originalMaxHealth = creature->GetMaxHealth();
}

const MythicLevel* MythicPlus::GetMythicLevel(uint32 level) const
{
    MythicLevelContainer::const_iterator itr = std::find_if(mythicLevels.begin(), mythicLevels.end(), [&level](const MythicLevel& mythicLevel) { return mythicLevel.level == level; });
    if (itr == mythicLevels.end())
        return nullptr;

    return &*itr;
}

void MythicPlus::ProcessStaticAffixes(const MythicLevel* mythicLevel, Creature* creature) const
{
    for (auto* affix : mythicLevel->affixes)
        affix->HandleStaticEffect(creature);
}

void MythicPlus::PrintMythicLevelInfo(const MythicLevel* mythicLevel, const Player* player) const
{
    ChatHandler handler(player->GetSession());
    handler.PSendSysMessage("Current Mythic Plus level is {}", mythicLevel->level);
    handler.PSendSysMessage("Active affix count: {}", mythicLevel->affixes.size());
    uint32 counter = 1;
    for (const auto* affix : mythicLevel->affixes)
        handler.PSendSysMessage("    {}. {}", counter++, affix->ToString());
}

bool MythicPlus::IsInMythicPlus(const Unit* unit) const
{
    return IsMapInMythicPlus(unit->GetMap());
}

bool MythicPlus::IsMapInMythicPlus(Map* map) const
{
    MapData* mapData = GetMapData(map, false);
    return mapData != nullptr && mapData->mythicLevel != nullptr;
}

void MythicPlus::LoadFromDB()
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    trans->Append("DELETE FROM `mythic_plus_dungeon` WHERE `id` NOT IN (SELECT `id` FROM `instance`)");
    trans->Append("DELETE FROM `mythic_plus_char_level` WHERE `guid` NOT IN (SELECT `guid` FROM `characters`)");
    trans->Append("DELETE FROM `mythic_plus_keystone_timer` WHERE `guid` NOT IN (SELECT `guid` FROM `characters`)");
    CharacterDatabase.DirectCommitTransaction(trans);

    WorldDatabaseTransaction wtrans = WorldDatabase.BeginTransaction();
    wtrans->Append("DELETE FROM mythic_plus_level_rewards rw WHERE NOT EXISTS (SELECT 1 FROM mythic_plus_level l where l.lvl = rw.lvl)");
    wtrans->Append("DELETE FROM mythic_plus_affix a WHERE NOT EXISTS (SELECT 1 FROM mythic_plus_level l where l.lvl = a.lvl)");
    WorldDatabase.DirectCommitTransaction(wtrans);

    LoadMythicPlusCapableDungeonsFromDB();
    LoadMythicPlusDungeonsFromDB();
    LoadMythicPlusCharLevelsFromDB();
    LoadMythicPlusKeystoneTimersFromDB();
    LoadMythicPlusSeasonsFromDB();
    LoadIgnoredEntriesForMultiplyAffixFromDB();
    LoadScaleMapFromDB();
    LoadMythicAffixFromDB();
    LoadMythicRewardsFromDB();
    LoadMythicPlusRotationsFromDB();
    LoadSeasonRewardsFromDB();
    LoadMythicLevelsFromDB();
    LoadSpellOverridesFromDB();
}

void MythicPlus::ClearMythicLevels()
{
    for (auto& mythicLevel : mythicLevels)
        for (auto* affix : mythicLevel.affixes)
            delete affix;

    mythicLevels.clear();
}

MythicPlus::MythicPlusDungeonInfo* MythicPlus::GetSavedDungeonInfo(uint32 instanceId)
{
    if (mythicPlusDungeonInfo.find(instanceId) != mythicPlusDungeonInfo.end())
        return &mythicPlusDungeonInfo.at(instanceId);

    return nullptr;
}

void MythicPlus::SaveDungeonInfo(uint32 instanceId, uint32 mapId, uint32 timeLimit, uint64 startTime, uint32 mythicLevel, uint32 penaltyOnDeath, uint32 deaths, bool done, bool isMythic)
{
    CharacterDatabase.Execute("REPLACE INTO mythic_plus_dungeon (id, map, timelimit, starttime, mythiclevel, done, ismythic, penalty_on_death, deaths) VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {})",
        instanceId, mapId, timeLimit, startTime, mythicLevel, done, isMythic, penaltyOnDeath, deaths);

    MythicPlusDungeonInfo* dungeonInfo = GetSavedDungeonInfo(instanceId);
    if (dungeonInfo == nullptr)
    {
        MythicPlusDungeonInfo dungeonInfo;
        dungeonInfo.instanceId = instanceId;
        dungeonInfo.mapId = mapId;
        dungeonInfo.startTime = startTime;
        dungeonInfo.timeLimit = timeLimit;
        dungeonInfo.mythicLevel = mythicLevel;
        dungeonInfo.done = done;
        dungeonInfo.isMythic = isMythic;
        dungeonInfo.penaltyOnDeath = penaltyOnDeath;
        dungeonInfo.deaths = deaths;
        mythicPlusDungeonInfo[instanceId] = dungeonInfo;
    }
    else
    {
        dungeonInfo->startTime = startTime;
        dungeonInfo->done = done;
        dungeonInfo->deaths = deaths;
    }
}

void MythicPlus::AddDungeonSnapshot(uint32 instanceId, uint32 mapId, Difficulty mapDiff, uint64 startTime,
    uint64 snapTime, uint32 combatTime, uint32 timelimit, uint32 charGuid, std::string charName,
    uint32 mythicLevel, uint32 creatureEntry, bool isFinalBoss, bool rewarded, uint32 penaltyOnDeath, uint32 deaths, uint32 randomAffixCount)
{
    CharacterDatabase.Execute("INSERT INTO mythic_plus_dungeon_snapshot (id, map, mapdifficulty, starttime, snaptime, combattime, timelimit, char_guid, char_name, mythiclevel, creature_entry, creature_final_boss, rewarded, penalty_on_death, deaths, random_affix_count) VALUES "
        "({}, {}, {}, {}, {}, {}, {}, {}, \"{}\", {}, {}, {}, {}, {}, {}, {})", instanceId, mapId, mapDiff, startTime, snapTime, combatTime, timelimit, charGuid, charName, mythicLevel, creatureEntry, isFinalBoss, rewarded, penaltyOnDeath, deaths, randomAffixCount);
}

bool MythicPlus::IsAllowedMythicPlusDungeon(uint32 mapId) const
{
    if (!IsEnabled())
        return false;

    return mythicPlusDungeons.find(mapId) != mythicPlusDungeons.end();
}

ObjectGuid MythicPlus::GetLeaderGuid(const Player* player) const
{
    const Group* group = player->GetGroup();
    if (!group)
        return ObjectGuid::Empty;

    return group->GetLeaderGUID();
}

void MythicPlus::LoadMythicPlusCapableDungeonsFromDB()
{
    mythicPlusDungeons.clear();

    QueryResult result = WorldDatabase.Query("SELECT map, mapdifficulty, final_boss_entry FROM mythic_plus_capable_dungeon");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 mapId = fields[0].Get<uint32>();
        uint16 diff = fields[1].Get<uint16>();
        if (diff != DUNGEON_DIFFICULTY_NORMAL && diff != DUNGEON_DIFFICULTY_HEROIC)
        {
            LOG_ERROR("sql.sql", "Table `mythic_plus_capable_dungeon` has invalid mapdifficulty '{}', ignoring", diff);
            continue;
        }
        uint32 finalBossEntry = fields[2].Get<uint32>();

        MythicPlusCapableDungeon dungeon;
        dungeon.map = mapId;
        dungeon.minDifficulty = (Difficulty)diff;
        dungeon.finalBossEntry = finalBossEntry;

        mythicPlusDungeons[mapId] = dungeon;
    } while (result->NextRow());
}

void MythicPlus::LoadMythicPlusDungeonsFromDB()
{
    mythicPlusDungeonInfo.clear();

    QueryResult result = CharacterDatabase.Query("SELECT id, map, timelimit, starttime, mythiclevel, done, ismythic, penalty_on_death, deaths FROM mythic_plus_dungeon");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 instanceId = fields[0].Get<uint32>();
        uint32 mapId = fields[1].Get<uint32>();
        uint32 timeLimit = fields[2].Get<uint32>();
        uint64 startTime = fields[3].Get<uint64>();
        uint32 mythicLevel = fields[4].Get<uint32>();
        bool done = fields[5].Get<bool>();
        bool isMythic = fields[6].Get<bool>();
        uint32 penaltyOnDeath = fields[7].Get<uint32>();
        uint32 deaths = fields[8].Get<uint32>();

        MythicPlusDungeonInfo dungeonInfo;
        dungeonInfo.instanceId = instanceId;
        dungeonInfo.mapId = mapId;
        dungeonInfo.timeLimit = timeLimit;
        dungeonInfo.startTime = startTime;
        dungeonInfo.mythicLevel = mythicLevel;
        dungeonInfo.done = done;
        dungeonInfo.isMythic = isMythic;
        dungeonInfo.penaltyOnDeath = penaltyOnDeath;
        dungeonInfo.deaths = deaths;

        mythicPlusDungeonInfo[instanceId] = dungeonInfo;
    } while (result->NextRow());
}

void MythicPlus::LoadMythicPlusCharLevelsFromDB()
{
    charMythicLevels.clear();

    QueryResult result = CharacterDatabase.Query("SELECT guid, mythiclevel FROM mythic_plus_char_level");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 guid = fields[0].Get<uint32>();
        uint32 mythiclevel = fields[1].Get<uint32>();

        charMythicLevels[guid] = mythiclevel;
    } while (result->NextRow());
}

void MythicPlus::LoadMythicPlusKeystoneTimersFromDB()
{
    charKeystoneBuyTimers.clear();

    QueryResult result = CharacterDatabase.Query("SELECT guid, buytime FROM mythic_plus_keystone_timer");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 guid = fields[0].Get<uint32>();
        uint64 buytime = fields[1].Get<uint64>();

        charKeystoneBuyTimers[guid] = buytime;
    } while (result->NextRow());
}

void MythicPlus::LoadMythicPlusSeasonsFromDB()
{
    mythicPlusSeasons.clear();
    activeSeasonId = 0;

    QueryResult result = CharacterDatabase.Query("SELECT id, year, month, start_unix, end_unix, is_active, label FROM mythic_plus_season");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        MythicPlusSeason season;
        season.id = fields[0].Get<uint32>();
        season.year = fields[1].Get<uint32>();
        season.month = fields[2].Get<uint32>();
        season.startUnix = fields[3].Get<uint64>();
        season.endUnix = fields[4].Get<uint64>();
        season.isActive = fields[5].Get<bool>();
        season.label = fields[6].Get<std::string>();

        if (season.isActive)
            activeSeasonId = season.id;

        mythicPlusSeasons[season.id] = season;
    } while (result->NextRow());
}

void MythicPlus::LoadMythicPlusRotationsFromDB()
{
    mythicPlusRotations.clear();

    QueryResult result = WorldDatabase.Query("SELECT id, rotation_type, start_unix, end_unix, affix_slot, affix_type, val1, val2, enabled FROM mythic_plus_rotation");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        MythicPlusRotationEntry entry;
        entry.id = fields[0].Get<uint32>();
        entry.rotationType = fields[1].Get<std::string>();
        entry.startUnix = fields[2].Get<uint64>();
        entry.endUnix = fields[3].Get<uint64>();
        entry.affixSlot = fields[4].Get<uint32>();
        entry.affixType = fields[5].Get<uint16>();
        entry.val1 = fields[6].IsNull() ? 0.0f : fields[6].Get<float>();
        entry.val2 = fields[7].IsNull() ? 0.0f : fields[7].Get<float>();
        entry.enabled = fields[8].Get<bool>();

        if (entry.affixType >= MAX_AFFIX_TYPE)
        {
            LOG_ERROR("sql.sql", "Table `mythic_plus_rotation` has invalid affix type '{}', ignoring", entry.affixType);
            continue;
        }

        mythicPlusRotations.push_back(entry);
    } while (result->NextRow());
}

void MythicPlus::LoadSeasonRewardsFromDB()
{
    seasonRewardDefinitions.clear();

    QueryResult result = WorldDatabase.Query("SELECT rank_start, rank_end, rewardtype, val1, val2, mail_subject, mail_body, enabled FROM mythic_plus_season_reward ORDER BY rank_start ASC, rank_end ASC, rewardtype ASC");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        MythicPlusSeasonRewardDefinition reward;
        reward.rankStart = fields[0].Get<uint32>();
        reward.rankEnd = fields[1].Get<uint32>();
        reward.rewardType = fields[2].Get<uint32>();
        reward.val1 = fields[3].Get<uint32>();
        reward.val2 = fields[4].IsNull() ? 0 : fields[4].Get<uint32>();
        reward.mailSubject = fields[5].IsNull() ? "" : fields[5].Get<std::string>();
        reward.mailBody = fields[6].IsNull() ? "" : fields[6].Get<std::string>();
        reward.enabled = fields[7].Get<bool>();

        if (reward.rewardType >= DBReward::MAX_REWARD_TYPE)
        {
            LOG_ERROR("sql.sql", "Table `mythic_plus_season_reward` has invalid rewardtype '{}', ignoring", reward.rewardType);
            continue;
        }

        seasonRewardDefinitions.push_back(reward);
    } while (result->NextRow());
}

void MythicPlus::LoadIgnoredEntriesForMultiplyAffixFromDB()
{
    ignoredEntriesForMultiplyAffix.clear();

    QueryResult result = WorldDatabase.Query("SELECT entry FROM mythic_plus_ignore_multiply_affix");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 entry = fields[0].Get<uint32>();
        if (sObjectMgr->GetCreatureTemplate(entry))
            ignoredEntriesForMultiplyAffix.insert(entry);
    } while (result->NextRow());
}

void MythicPlus::LoadScaleMapFromDB()
{
    scaleMap.clear();

    QueryResult result = WorldDatabase.Query("SELECT map, mapdifficulty, dmg_scale_trash, dmg_scale_boss FROM mythic_plus_map_scale");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 map = fields[0].Get<uint32>();
        uint16 diff = fields[1].Get<uint16>();
        float dmgScaleTrash = fields[2].Get<float>();
        float dmgScaleBoss = fields[3].Get<float>();

        MapScale scale;
        scale.map = map;
        scale.difficulty = diff;
        scale.trashDmgScale = dmgScaleTrash;
        scale.bossDmgScale = dmgScaleBoss;

        scaleMap[map][diff] = scale;
    } while (result->NextRow());
}

void MythicPlus::LoadMythicPlusSnapshotsFromDB()
{
    std::string query =
        "select "
            "mpds.id, "
            "mpds.starttime, "
            "mpds.snaptime, "
            "mpds.creature_entry, "
            "mpds.map, "
            "group_concat(mpds.char_name) players, "
            "max(mpds.combattime) combattime, "
            "max(mythiclevel) mythiclevel, "
            "max(creature_final_boss) creature_final_boss, "
            "max(rewarded) rewarded, "
            "mpds.mapdifficulty, "
            "mpds.timelimit, "
            "max(mpds.penalty_on_death) penalty_on_death, "
            "max(mpds.deaths) deaths, "
            "max(random_affix_count) random_affix_count "
        "from mythic_plus_dungeon_snapshot mpds "
        "group by mpds.id, mpds.map, mpds.mapdifficulty, mpds.starttime, mpds.timelimit, mpds.snaptime, mpds.creature_entry";
    _queryProcessor.AddCallback(CharacterDatabase.AsyncQuery(query).WithCallback(std::bind(&MythicPlus::MythicPlusSnapshotsDBCallback, this, std::placeholders::_1)));
}

void MythicPlus::LoadMythicAffixFromDB()
{
    affixesFromDB.clear();

    QueryResult result = WorldDatabase.Query("SELECT lvl, affixtype, val1, val2 FROM mythic_plus_affix");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 lvl = fields[0].Get<uint32>();
        uint16 affixType = fields[1].Get<uint16>();
        if (affixType >= MAX_AFFIX_TYPE)
        {
            LOG_ERROR("sql.sql", "Table `mythic_plus_affix` has invalid affix type '{}', ignoring", affixType);
            continue;
        }
        float val1 = fields[2].Get<float>();
        float val2 = fields[3].Get<float>();
        affixesFromDB[lvl].push_back({lvl, affixType, val1, val2});
    } while (result->NextRow());
}

void MythicPlus::LoadMythicRewardsFromDB()
{
    rewardsFromDB.clear();

    QueryResult result = WorldDatabase.Query("SELECT lvl, rewardtype, val1, val2 FROM mythic_plus_level_rewards");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 lvl = fields[0].Get<uint32>();
        uint16 rewardType = fields[1].Get<uint16>();
        if (rewardType >= DBReward::MAX_REWARD_TYPE)
        {
            LOG_ERROR("sql.sql", "Table `mythic_plus_level_rewards` has invalid rewardtype '{}', ignoring", rewardType);
            continue;
        }
        uint32 val1 = fields[2].Get<uint32>();
        uint32 val2 = fields[3].Get<uint32>();
        rewardsFromDB[lvl].push_back({ lvl, (DBReward::RewardType)rewardType, val1, val2 });
    } while (result->NextRow());
}

void MythicPlus::LoadMythicLevelsFromDB()
{
    ClearMythicLevels();

    QueryResult result = WorldDatabase.Query(
        "SELECT lvl, timelimit, random_affix_count, hp_mult, dmg_mult FROM mythic_plus_level ORDER BY lvl");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 lvl = fields[0].Get<uint32>();
        uint32 timeLimit = fields[1].Get<uint32>();
        uint32 randomAffixCount = fields[2].Get<uint32>();
        float hpMult = fields[3].Get<float>();
        float dmgMult = fields[4].Get<float>();
        if (hpMult <= 0.0f)
            hpMult = 1.0f;
        if (dmgMult <= 0.0f)
            dmgMult = 1.0f;

        MythicLevel level;
        level.level = lvl;
        level.timeLimit = timeLimit;
        level.randomAffixCount = randomAffixCount;
        level.hpMult = hpMult;
        level.dmgMult = dmgMult;

        if (rewardsFromDB.find(lvl) != rewardsFromDB.end())
        {
            const std::vector<DBReward>& rewards = rewardsFromDB.at(lvl);
            for (const auto& r : rewards)
            {
                if (r.type == DBReward::REWARD_COPPER)
                    level.reward.money += r.val1;
                else if (r.type == DBReward::REWARD_TOKEN)
                    level.reward.AddToken(r.val1, r.val2);
            }
        }

        if (affixesFromDB.find(lvl) != affixesFromDB.end())
        {
            const std::vector<DBAffix>& affixes = affixesFromDB.at(lvl);
            for (const auto& a : affixes)
            {
                MythicAffix* affix = MythicAffix::AffixFactory((MythicAffixType)a.affixType, a.val1, a.val2);
                if (affix != nullptr && !affix->IsRandom())
                    level.affixes.push_back(affix);
            }
        }

        if (randomAffixCount > 0)
        {
            std::vector<MythicAffix*> randomAffixes = BuildRandomAffixesForLevel(lvl, randomAffixCount);
            for (auto* a : randomAffixes)
                level.affixes.push_back(a);
        }

        mythicLevels.push_back(level);
    } while (result->NextRow());
}

void MythicPlus::LoadSpellOverridesFromDB()
{
    spellOverrides.clear();

    QueryResult result = WorldDatabase.Query("SELECT spellid, map, modpct, dotmodpct FROM mythic_plus_spell_override");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 spellId = fields[0].Get<uint32>();
        uint32 mapId = fields[1].Get<uint32>();
        float modPct = fields[2].Get<float>();
        float dotModPct = fields[3].Get<float>();

        SpellOverride spellOverride;
        spellOverride.spellId = spellId;
        spellOverride.map = mapId;
        spellOverride.modPct = modPct;
        spellOverride.dotModPct = dotModPct;

        spellOverrides[mapId][spellId] = spellOverride;
    } while (result->NextRow());
}

/*static*/ std::string MythicPlus::Utils::GetCreatureName(const Player* player, const Creature* creature)
{
    return creature->GetNameForLocaleIdx(player->GetSession()->GetSessionDbLocaleIndex());
}

/*static*/ std::string MythicPlus::Utils::GetCreatureNameByEntry(const Player* player, uint32 entry)
{
    auto creatureTemplate = sObjectMgr->GetCreatureTemplate(entry);
    auto creatureLocale = sObjectMgr->GetCreatureLocale(entry);
    std::string name;

    if (creatureLocale)
        name = creatureLocale->Name[player->GetSession()->GetSessionDbcLocale()];

    if (name.empty() && creatureTemplate)
        name = creatureTemplate->Name;

    if (name.empty())
        name = "Unknown creature";

    return name;
}

/*static*/ uint32 MythicPlus::Utils::PlayerGUID(const Player* player)
{
    return player->GetGUID().GetCounter();
}

/*static*/ std::string MythicPlus::Utils::CopperToMoneyStr(uint32 money, bool colored)
{
    uint32 gold = money / GOLD;
    uint32 silver = (money % GOLD) / SILVER;
    uint32 copper = (money % GOLD) % SILVER;

    std::ostringstream oss;
    if (gold > 0)
    {
        if (colored)
            oss << gold << "|cffb3aa34g|r";
        else
            oss << gold << "g";
    }
    if (silver > 0)
    {
        if (colored)
            oss << silver << "|cff7E7C7Fs|r";
        else
            oss << silver << "s";
    }
    if (copper > 0)
    {
        if (colored)
            oss << copper << "|cff974B29c|r";
        else
            oss << copper << "c";
    }

    return oss.str();
}

/*static*/ std::string MythicPlus::Utils::ItemIcon(const ItemTemplate* proto, uint32 width, uint32 height, int x, int y)
{
    std::ostringstream ss;
    ss << "|TInterface";
    const ItemDisplayInfoEntry* dispInfo = nullptr;
    if (proto)
    {
        dispInfo = sItemDisplayInfoStore.LookupEntry(proto->DisplayInfoID);
        if (dispInfo)
            ss << "/ICONS/" << dispInfo->inventoryIcon;
    }
    if (!dispInfo)
        ss << "/InventoryItems/WoWUnknownItem01";
    ss << ":" << width << ":" << height << ":" << x << ":" << y << "|t";
    return ss.str();
}

/*static*/ std::string MythicPlus::Utils::ItemIcon(const ItemTemplate* proto)
{
    return ItemIcon(proto, 30, 30, 0, 0);
}

/*static*/ std::string MythicPlus::Utils::ItemName(const ItemTemplate* proto, const Player* player)
{
    LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
    std::string name = proto->Name1;
    if (ItemLocale const* il = sObjectMgr->GetItemLocale(proto->ItemId))
        ObjectMgr::GetLocaleString(il->Name, loc_idx, name);
    return name;
}

/*static*/ std::string MythicPlus::Utils::ItemLinkForUI(uint32 entry, const Player* player)
{
    const ItemTemplate* proto = sObjectMgr->GetItemTemplate(entry);
    std::ostringstream oss;
    oss << ItemIcon(proto);
    oss << ItemName(proto, player);
    return oss.str();
}

/*static*/ std::string MythicPlus::Utils::Colored(const std::string& message, const std::string& hexColor)
{
    std::ostringstream oss;
    oss << "|cff" << hexColor;
    oss << message;
    oss << "|r";
    return oss.str();
}

/*static*/ std::string MythicPlus::Utils::RedColored(const std::string& message)
{
    return Colored(message, "b50505");
}

/*static*/ std::string MythicPlus::Utils::GreenColored(const std::string& message)
{
    return Colored(message, "0d852d");
}

/*static*/ void MythicPlus::Utils::VisualFeedback(Player* player)
{
    player->CastSpell(player, VISUAL_FEEDBACK_SPELL_ID, true);
}

/*static*/ std::string MythicPlus::Utils::FormatFloat(float val, uint32 decimals)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << val;
    return oss.str();
}

/*static*/ std::string MythicPlus::Utils::DateFromSeconds(uint64 seconds)
{
    std::time_t secs = seconds;
    std::tm* utc_tm = std::gmtime(&secs);
    std::ostringstream oss;
    oss << std::put_time(utc_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

/*static*/ bool MythicPlus::Utils::IsGroupLeader(const Player* player)
{
    const Group* group = player->GetGroup();
    if (group == nullptr)
        return false;

    return group->GetLeaderGUID() == player->GetGUID();
}

/*static*/ long long MythicPlus::Utils::GameTimeCount()
{
    return GameTime::GetGameTime().count();
}

/*static*/ std::mt19937_64 MythicPlus::Utils::RandomEngine()
{
    static std::mt19937_64 random_engine{ std::random_device{}() };
    return random_engine;
}

/*static*/ bool MythicPlus::Utils::CanBeHeroic(uint32 map)
{
    static uint32 heroic[] = {
        MAP_UTGARDE_KEEP, MAP_UTGARDE_PINNACLE, MAP_THE_NEXUS,
        MAP_THE_OCULUS, MAP_AZJOL_NERUB, MAP_AHN_KAHET_THE_OLD_KINGDOM,
        MAP_DRAK_THARON_KEEP, MAP_GUNDRAK, MAP_HALLS_OF_STONE,
        MAP_HALLS_OF_LIGHTNING, MAP_THE_FORGE_OF_SOULS, MAP_PIT_OF_SARON,
        MAP_HALLS_OF_REFLECTION, MAP_TRIAL_OF_THE_CHAMPION,
        MAP_HELLFIRE_CITADEL_RAMPARTS, MAP_HELLFIRE_CITADEL_THE_BLOOD_FURNACE,
        MAP_HELLFIRE_CITADEL_THE_SHATTERED_HALLS, MAP_COILFANG_THE_SLAVE_PENS,
        MAP_COILFANG_THE_UNDERBOG, MAP_COILFANG_THE_STEAMVAULT, MAP_AUCHINDOUN_MANA_TOMBS,
        MAP_AUCHINDOUN_AUCHENAI_CRYPTS, MAP_AUCHINDOUN_SETHEKK_HALLS, MAP_AUCHINDOUN_SHADOW_LABYRINTH,
        MAP_TEMPEST_KEEP_THE_MECHANAR, MAP_TEMPEST_KEEP_THE_BOTANICA, MAP_TEMPEST_KEEP_THE_ARCATRAZ
    };

    auto foundHeroic = std::find(std::begin(heroic), std::end(heroic), map);
    return foundHeroic != std::end(heroic);
}

/*static*/ float MythicPlus::Utils::HealthMod(int32 Rank)
{
    switch (Rank)
    {
        case CREATURE_ELITE_NORMAL:
            return sWorld->getRate(RATE_CREATURE_NORMAL_HP);
        case CREATURE_ELITE_ELITE:
            return sWorld->getRate(RATE_CREATURE_ELITE_ELITE_HP);
        case CREATURE_ELITE_RAREELITE:
            return sWorld->getRate(RATE_CREATURE_ELITE_RAREELITE_HP);
        case CREATURE_ELITE_WORLDBOSS:
            return sWorld->getRate(RATE_CREATURE_ELITE_WORLDBOSS_HP);
        case CREATURE_ELITE_RARE:
            return sWorld->getRate(RATE_CREATURE_ELITE_RARE_HP);
        default:
            return sWorld->getRate(RATE_CREATURE_ELITE_ELITE_HP);
    }
}

bool MythicPlus::IsFinalBoss(uint32 entry) const
{
    for (const auto& d : mythicPlusDungeons)
        if (d.second.finalBossEntry == entry)
            return true;

    return false;
}

void MythicReward::AddToken(uint32 entry, uint32 count)
{
    tokens.push_back(std::make_pair(entry, count));
}

void MythicPlus::Reward(Player* player, const MythicReward& reward) const
{
    if (reward.money)
        player->ModifyMoney(reward.money);

    for (auto const& token : reward.tokens)
    {
        if (!DeliverMythicStacksOrMail(player, token.first, token.second,
                "Mythic Plus reward",
                "Your Mythic Plus reward could not be placed in your bags. It is attached to this letter."))
            LOG_ERROR("module", "MythicPlus: failed to deliver reward item {} x{} to {}", token.first, token.second, player->GetName());
    }

    RewardKeystone(player);
}

void MythicPlus::DistributeItemUpgradeBossTokens(Map* map, bool finalBoss) const
{
    if (!enabled || itemUpgradeTokenEntry == 0 || itemUpgradeTokenCount == 0)
        return;

    if (itemUpgradeTokenSkipFinalBoss && finalBoss)
        return;

    if (!sObjectMgr->GetItemTemplate(itemUpgradeTokenEntry))
        return;

    const Map::PlayerList& players = map->GetPlayers();
    for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
    {
        if (Player* player = itr->GetSource())
        {
            if (!DeliverMythicStacksOrMail(player, itemUpgradeTokenEntry, itemUpgradeTokenCount,
                    "Mythic Plus",
                    "Your Mythic+ item upgrade token could not be placed in your bags. It is attached to this letter."))
                LOG_ERROR("module", "MythicPlus: failed to deliver item upgrade token {} x{} to {}",
                    itemUpgradeTokenEntry, itemUpgradeTokenCount, player->GetName());
        }
    }
}

void MythicPlus::RemoveDungeonInfo(uint32 instanceId)
{
    std::unordered_map<uint32, MythicPlusDungeonInfo>::iterator itr = mythicPlusDungeonInfo.find(instanceId);
    if (itr != mythicPlusDungeonInfo.end())
    {
        mythicPlusDungeonInfo.erase(itr);
        CharacterDatabase.Execute("DELETE FROM mythic_plus_dungeon WHERE id = {}", instanceId);
    }
}

uint32 MythicPlus::GetCurrentMythicPlusLevel(const Player* player) const
{
    return GetCurrentMythicPlusLevelForGUID(Utils::PlayerGUID(player));
}

uint32 MythicPlus::GetCurrentMythicPlusLevelForGUID(uint32 guid) const
{
    if (charMythicLevels.find(guid) != charMythicLevels.end())
        return charMythicLevels.at(guid);

    return 0;
}

bool MythicPlus::SetCurrentMythicPlusLevel(const Player* player, uint32 mythiclevel, bool force)
{
    if (player->GetGroup() == nullptr || force)
    {
        uint32 guid = Utils::PlayerGUID(player);
        charMythicLevels[guid] = mythiclevel;
        CharacterDatabase.Execute("REPLACE INTO mythic_plus_char_level (guid, mythiclevel) VALUES ({}, {})", guid, mythiclevel);
        return true;
    }

    return false;
}

uint32 MythicPlus::GetCurrentMythicPlusLevelForDungeon(const Player* player) const
{
    ObjectGuid leaderGuid = GetLeaderGuid(player);
    if (leaderGuid.IsEmpty())
        return 0;

    Player* leader = ObjectAccessor::FindConnectedPlayer(leaderGuid);
    if (leader == nullptr)
        return 0;

    return GetCurrentMythicPlusLevel(leader);
}

void MythicPlus::ProcessQueryCallbacks()
{
    _queryProcessor.ProcessReadyCallbacks();
}

const MythicPlus::MythicPlusSeason* MythicPlus::GetActiveSeason() const
{
    auto itr = mythicPlusSeasons.find(activeSeasonId);
    if (itr == mythicPlusSeasons.end())
        return nullptr;

    return &itr->second;
}

const MythicPlus::MythicPlusSeason* MythicPlus::GetSeason(uint32 seasonId) const
{
    auto itr = mythicPlusSeasons.find(seasonId);
    if (itr == mythicPlusSeasons.end())
        return nullptr;

    return &itr->second;
}

uint32 MythicPlus::ResolveSeasonId(uint32 seasonId) const
{
    if (seasonId != 0)
        return mythicPlusSeasons.find(seasonId) != mythicPlusSeasons.end() ? seasonId : 0;

    return activeSeasonId;
}

uint64 MythicPlus::CalculateActiveRotationState() const
{
    uint64 state = activeSeasonId;
    uint64 now = GameTime::GetGameTime().count();
    std::vector<uint32> activeRotationIds;

    for (MythicPlusRotationEntry const& rotation : mythicPlusRotations)
    {
        if (!rotation.enabled)
            continue;

        if (rotation.startUnix > now || rotation.endUnix <= now)
            continue;

        activeRotationIds.push_back(rotation.id);
    }

    std::sort(activeRotationIds.begin(), activeRotationIds.end());
    for (uint32 rotationId : activeRotationIds)
        state ^= uint64(rotationId) + 0x9e3779b97f4a7c15ULL + (state << 6) + (state >> 2);

    return state;
}

std::vector<MythicAffix*> MythicPlus::BuildRandomAffixesForLevel(uint32 mythicLevel, uint32 maxCount) const
{
    std::vector<MythicAffix*> result;
    if (maxCount == 0)
        return result;

    uint64 now = GameTime::GetGameTime().count();
    std::set<uint32> usedAffixTypes;
    std::map<uint32, MythicPlusRotationEntry> activeRotationsBySlot;

    for (MythicPlusRotationEntry const& rotation : mythicPlusRotations)
    {
        if (!rotation.enabled)
            continue;

        if (rotation.startUnix > now || rotation.endUnix <= now)
            continue;

        if (rotation.affixSlot == 0 || rotation.affixSlot > maxCount)
            continue;

        auto currentItr = activeRotationsBySlot.find(rotation.affixSlot);
        if (currentItr == activeRotationsBySlot.end() || currentItr->second.startUnix < rotation.startUnix)
            activeRotationsBySlot[rotation.affixSlot] = rotation;
    }

    for (uint32 slot = 1; slot <= maxCount; ++slot)
    {
        auto itr = activeRotationsBySlot.find(slot);
        if (itr == activeRotationsBySlot.end())
            continue;

        MythicAffix* affix = MythicAffix::AffixFactory((MythicAffixType)itr->second.affixType, itr->second.val1, itr->second.val2);
        if (!affix)
            continue;

        result.push_back(affix);
        usedAffixTypes.insert(itr->second.affixType);
    }

    if (result.size() >= maxCount)
        return result;

    std::vector<uint32> pool;
    for (uint32 affixType : MythicAffix::RandomAffixes)
        if (usedAffixTypes.find(affixType) == usedAffixTypes.end())
            pool.push_back(affixType);

    if (pool.empty())
        return result;

    uint64 seasonSeed = activeSeasonId;
    if (seasonSeed == 0)
    {
        std::tm utcNow = GetUtcTimeBreakdown(now);
        seasonSeed = BuildUtcMonthStart(uint32(utcNow.tm_year + 1900), uint32(utcNow.tm_mon + 1));
    }

    std::mt19937_64 engine(seasonSeed ^ (uint64(mythicLevel) << 32) ^ maxCount);
    std::vector<uint32> chosen;
    std::ranges::sample(pool, std::back_inserter(chosen), std::min<std::size_t>(pool.size(), maxCount - result.size()), engine);
    for (uint32 affixType : chosen)
    {
        MythicAffix* affix = MythicAffix::AffixFactory((MythicAffixType)affixType);
        ASSERT(affix);
        result.push_back(affix);
    }

    return result;
}

uint32 MythicPlus::GetSecondsUntilSeasonEnd() const
{
    MythicPlusSeason const* activeSeason = GetActiveSeason();
    if (!activeSeason)
        return 0;

    uint64 now = GameTime::GetGameTime().count();
    if (activeSeason->endUnix <= now)
        return 0;

    return activeSeason->endUnix - now;
}

std::vector<MythicPlus::MythicPlusSeason> MythicPlus::GetRecentSeasons(uint32 limit) const
{
    std::vector<MythicPlusSeason> seasons;
    seasons.reserve(mythicPlusSeasons.size());
    for (auto const& seasonPair : mythicPlusSeasons)
        seasons.push_back(seasonPair.second);

    std::sort(seasons.begin(), seasons.end(), [](MythicPlusSeason const& a, MythicPlusSeason const& b)
    {
        if (a.year != b.year)
            return a.year > b.year;

        return a.month > b.month;
    });

    if (seasons.size() > limit)
        seasons.resize(limit);

    return seasons;
}

void MythicPlus::EnsureActiveSeason()
{
    uint32 previousActiveSeasonId = activeSeasonId;
    uint64 now = GameTime::GetGameTime().count();
    std::tm utcNow = GetUtcTimeBreakdown(now);
    uint32 currentYear = uint32(utcNow.tm_year + 1900);
    uint32 currentMonth = uint32(utcNow.tm_mon + 1);

    MythicPlusSeason* currentSeason = nullptr;
    for (auto& seasonPair : mythicPlusSeasons)
    {
        MythicPlusSeason& season = seasonPair.second;
        if (season.year == currentYear && season.month == currentMonth)
        {
            currentSeason = &season;
            break;
        }
    }

    if (!currentSeason)
    {
        uint32 nextYear = currentYear;
        uint32 nextMonth = currentMonth + 1;
        if (nextMonth == 13)
        {
            nextMonth = 1;
            ++nextYear;
        }

        uint64 startUnix = BuildUtcMonthStart(currentYear, currentMonth);
        uint64 endUnix = BuildUtcMonthStart(nextYear, nextMonth);
        std::string label = BuildSeasonLabel(currentYear, currentMonth);
        CharacterDatabase.EscapeString(label);

        CharacterDatabase.Execute(
            "INSERT INTO mythic_plus_season (year, month, start_unix, end_unix, is_active, label) VALUES ({}, {}, {}, {}, 1, \"{}\")",
            currentYear, currentMonth, startUnix, endUnix, label);

        LoadMythicPlusSeasonsFromDB();

        for (auto& seasonPair : mythicPlusSeasons)
        {
            MythicPlusSeason& season = seasonPair.second;
            if (season.year == currentYear && season.month == currentMonth)
            {
                currentSeason = &season;
                break;
            }
        }
    }

    if (!currentSeason)
        return;

    if (previousActiveSeasonId != 0 && previousActiveSeasonId != currentSeason->id)
        DistributeSeasonRewards(previousActiveSeasonId);

    CharacterDatabase.Execute("UPDATE mythic_plus_season SET is_active = 0 WHERE is_active = 1 AND id <> {}", currentSeason->id);
    CharacterDatabase.Execute("UPDATE mythic_plus_season SET is_active = 1 WHERE id = {}", currentSeason->id);

    activeSeasonId = currentSeason->id;
    for (auto& seasonPair : mythicPlusSeasons)
        seasonPair.second.isActive = seasonPair.first == activeSeasonId;

    uint64 rotationState = CalculateActiveRotationState();
    if (rotationState != activeRotationState)
    {
        activeRotationState = rotationState;
        LoadMythicLevelsFromDB();
    }
}

uint32 MythicPlus::CalculateMythicPlusScore(uint32 mythicLevel, uint32 totalTime, uint32 timeLimit, uint32 deaths) const
{
    uint32 score = mythicLevel * 100;
    uint32 timedBonus = 0;

    if (timeLimit > 0)
    {
        if (totalTime <= timeLimit)
        {
            uint32 timeRemaining = timeLimit - totalTime;
            timedBonus = timeRemaining * 5 > timeLimit ? 30 : 20;
        }
        else if (totalTime <= timeLimit + (timeLimit / 5))
            timedBonus = 5;
    }

    uint32 deathPenalty = std::min(deaths * 2, 20u);
    return score + timedBonus > deathPenalty ? score + timedBonus - deathPenalty : 0;
}

std::vector<MythicPlus::MythicPlusOverallLeaderboardEntry> MythicPlus::GetOverallLeaderboard(uint32 limit, uint32 seasonId) const
{
    std::vector<MythicPlusOverallLeaderboardEntry> entries;
    seasonId = ResolveSeasonId(seasonId);
    if (seasonId == 0)
        return entries;

    QueryResult result = CharacterDatabase.Query(
        "SELECT char_guid, char_name, SUM(score) total_score, MAX(mythic_level) best_level, COUNT(*) runs FROM mythic_plus_leaderboard WHERE season_id = {} GROUP BY char_guid, char_name ORDER BY total_score DESC, best_level DESC, char_name ASC LIMIT {}",
        seasonId, limit);
    if (!result)
        return entries;

    do
    {
        Field* fields = result->Fetch();
        MythicPlusOverallLeaderboardEntry entry;
        entry.charGuid = fields[0].Get<uint32>();
        entry.charName = fields[1].Get<std::string>();
        entry.totalScore = fields[2].Get<uint32>();
        entry.bestLevel = fields[3].Get<uint32>();
        entry.runs = fields[4].Get<uint32>();
        entries.push_back(entry);
    } while (result->NextRow());

    return entries;
}

std::vector<MythicPlus::MythicPlusLeaderboardEntry> MythicPlus::GetMapLeaderboard(uint32 mapId, uint32 limit, uint32 seasonId) const
{
    std::vector<MythicPlusLeaderboardEntry> entries;
    seasonId = ResolveSeasonId(seasonId);
    if (seasonId == 0)
        return entries;

    QueryResult result = CharacterDatabase.Query(
        "SELECT season_id, char_guid, char_name, map_id, difficulty, mythic_level, best_time, deaths, penalty_seconds, completed_in_time, score, group_members, last_update FROM mythic_plus_leaderboard WHERE season_id = {} AND map_id = {} ORDER BY score DESC, mythic_level DESC, best_time ASC, deaths ASC, char_name ASC LIMIT {}",
        seasonId, mapId, limit);
    if (!result)
        return entries;

    do
    {
        Field* fields = result->Fetch();
        MythicPlusLeaderboardEntry entry;
        entry.seasonId = fields[0].Get<uint32>();
        entry.charGuid = fields[1].Get<uint32>();
        entry.charName = fields[2].Get<std::string>();
        entry.mapId = fields[3].Get<uint32>();
        entry.difficulty = fields[4].Get<uint32>();
        entry.mythicLevel = fields[5].Get<uint32>();
        entry.bestTime = fields[6].Get<uint32>();
        entry.deaths = fields[7].Get<uint32>();
        entry.penaltySeconds = fields[8].Get<uint32>();
        entry.completedInTime = fields[9].Get<bool>();
        entry.score = fields[10].Get<uint32>();
        entry.groupMembers = fields[11].Get<std::string>();
        entry.lastUpdate = fields[12].Get<uint64>();
        entries.push_back(entry);
    } while (result->NextRow());

    return entries;
}

MythicPlus::MythicPlusPlayerRatingSummary MythicPlus::GetPlayerRatingSummary(uint32 charGuid, uint32 seasonId) const
{
    MythicPlusPlayerRatingSummary summary;
    seasonId = ResolveSeasonId(seasonId);
    if (seasonId == 0)
        return summary;

    QueryResult result = CharacterDatabase.Query(
        "SELECT SUM(score) total_score, MAX(mythic_level) best_level, COUNT(*) runs FROM mythic_plus_leaderboard WHERE season_id = {} AND char_guid = {}",
        seasonId, charGuid);
    if (result)
    {
        Field* fields = result->Fetch();
        summary.totalScore = fields[0].IsNull() ? 0 : fields[0].Get<uint32>();
        summary.bestLevel = fields[1].IsNull() ? 0 : fields[1].Get<uint32>();
        summary.runs = fields[2].IsNull() ? 0 : fields[2].Get<uint32>();
    }

    if (summary.runs == 0)
        return summary;

    std::vector<MythicPlusOverallLeaderboardEntry> overall = GetOverallLeaderboard(1000, seasonId);
    for (std::size_t i = 0; i < overall.size(); ++i)
    {
        if (overall[i].charGuid == charGuid)
        {
            summary.overallRank = i + 1;
            break;
        }
    }

    return summary;
}

bool MythicPlus::DistributeSeasonRewards(uint32 seasonId)
{
    seasonId = ResolveSeasonId(seasonId);
    MythicPlusSeason const* season = GetSeason(seasonId);
    if (!season || seasonRewardDefinitions.empty())
        return false;

    std::set<uint32> alreadyRewardedPlayers;
    QueryResult rewardedResult = CharacterDatabase.Query("SELECT char_guid FROM mythic_plus_season_reward_log WHERE season_id = {}", seasonId);
    if (rewardedResult)
    {
        do
        {
            Field* fields = rewardedResult->Fetch();
            alreadyRewardedPlayers.insert(fields[0].Get<uint32>());
        } while (rewardedResult->NextRow());
    }

    std::vector<MythicPlusOverallLeaderboardEntry> leaderboard = GetOverallLeaderboard(1000, seasonId);
    if (leaderboard.empty())
        return false;

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    bool rewardedAnyPlayer = false;

    for (std::size_t i = 0; i < leaderboard.size(); ++i)
    {
        uint32 rank = i + 1;
        MythicPlusOverallLeaderboardEntry const& entry = leaderboard[i];
        if (alreadyRewardedPlayers.find(entry.charGuid) != alreadyRewardedPlayers.end())
            continue;

        uint32 totalMoney = 0;
        std::vector<std::pair<uint32, uint32>> itemRewards;
        std::string mailSubject;
        std::string mailBody;

        for (MythicPlusSeasonRewardDefinition const& reward : seasonRewardDefinitions)
        {
            if (!reward.enabled)
                continue;

            if (rank < reward.rankStart || rank > reward.rankEnd)
                continue;

            if (mailSubject.empty() && !reward.mailSubject.empty())
                mailSubject = reward.mailSubject;

            if (mailBody.empty() && !reward.mailBody.empty())
                mailBody = reward.mailBody;

            if (reward.rewardType == DBReward::REWARD_COPPER)
                totalMoney += reward.val1;
            else if (reward.rewardType == DBReward::REWARD_TOKEN && reward.val1 > 0 && reward.val2 > 0)
                itemRewards.emplace_back(reward.val1, reward.val2);
        }

        if (totalMoney == 0 && itemRewards.empty())
            continue;

        if (mailSubject.empty())
            mailSubject = BuildDefaultSeasonRewardSubject(*season, rank);

        if (mailBody.empty())
            mailBody = BuildDefaultSeasonRewardBody(*season, entry, rank);

        MailDraft draft(mailSubject, mailBody);
        if (totalMoney > 0)
            draft.AddMoney(totalMoney);

        std::ostringstream itemsSummary;
        for (std::size_t itemIndex = 0; itemIndex < itemRewards.size(); ++itemIndex)
        {
            auto const& [itemEntry, itemCount] = itemRewards[itemIndex];
            if (Item* item = Item::CreateItem(itemEntry, itemCount))
            {
                item->SaveToDB(trans);
                draft.AddItem(item);
            }

            if (itemIndex > 0)
                itemsSummary << ", ";

            itemsSummary << itemEntry << 'x' << itemCount;
        }

        draft.SendMailTo(trans, MailReceiver(entry.charGuid), MailSender(MAIL_NORMAL, 0, MAIL_STATIONERY_GM));

        std::string charName = entry.charName;
        std::string itemSummaryValue = itemsSummary.str();
        CharacterDatabase.EscapeString(charName);
        CharacterDatabase.EscapeString(itemSummaryValue);

        std::ostringstream query;
        query << "INSERT INTO mythic_plus_season_reward_log (season_id, char_guid, char_name, reward_rank, total_money, items_summary, sent_at) VALUES (";
        query << seasonId << ", " << entry.charGuid << ", \"" << charName << "\", ";
        query << rank << ", " << totalMoney << ", \"" << itemSummaryValue << "\", ";
        query << GameTime::GetGameTime().count() << ')';
        trans->Append(query.str());

        rewardedAnyPlayer = true;
    }

    CharacterDatabase.CommitTransaction(trans);

    if (rewardedAnyPlayer)
    {
        std::ostringstream announcement;
        announcement << "Mythic season " << season->label << " rewards have been distributed.";
        if (!leaderboard.empty())
        {
            announcement << " Top finishers: #1 " << leaderboard[0].charName;
            if (leaderboard.size() > 1)
                announcement << ", #2 " << leaderboard[1].charName;
            if (leaderboard.size() > 2)
                announcement << ", #3 " << leaderboard[2].charName;
        }

        ChatHandler(nullptr).SendWorldText(announcement.str());
    }

    return rewardedAnyPlayer;
}

std::string MythicPlus::BuildLeaderboardGroupMembers(std::vector<std::pair<uint32, std::string>> const& players) const
{
    std::vector<std::string> names;
    names.reserve(players.size());
    for (auto const& playerInfo : players)
        names.push_back(playerInfo.second);

    std::sort(names.begin(), names.end());

    std::ostringstream oss;
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        if (i > 0)
            oss << ", ";

        oss << names[i];
    }

    return oss.str();
}

bool MythicPlus::ShouldReplaceLeaderboardEntry(MythicPlusLeaderboardEntry const& existing, MythicPlusLeaderboardEntry const& candidate) const
{
    if (candidate.score != existing.score)
        return candidate.score > existing.score;

    if (candidate.mythicLevel != existing.mythicLevel)
        return candidate.mythicLevel > existing.mythicLevel;

    if (candidate.bestTime != existing.bestTime)
        return candidate.bestTime < existing.bestTime;

    if (candidate.deaths != existing.deaths)
        return candidate.deaths < existing.deaths;

    return false;
}

void MythicPlus::SubmitCompletedRunToLeaderboard(uint32 mapId, Difficulty mapDiff, uint32 mythicLevel,
    uint32 totalTime, uint32 timeLimit, uint32 penaltyOnDeath, uint32 deaths,
    std::vector<std::pair<uint32, std::string>> const& players)
{
    if (players.empty())
        return;

    EnsureActiveSeason();
    MythicPlusSeason const* activeSeason = GetActiveSeason();
    if (!activeSeason)
        return;

    std::string groupMembers = BuildLeaderboardGroupMembers(players);
    CharacterDatabase.EscapeString(groupMembers);

    uint64 now = GameTime::GetGameTime().count();
    bool completedInTime = timeLimit > 0 && totalTime <= timeLimit;
    uint32 score = CalculateMythicPlusScore(mythicLevel, totalTime, timeLimit, deaths);

    for (auto const& playerInfo : players)
    {
        MythicPlusLeaderboardEntry candidate;
        candidate.seasonId = activeSeason->id;
        candidate.charGuid = playerInfo.first;
        candidate.charName = playerInfo.second;
        candidate.mapId = mapId;
        candidate.difficulty = mapDiff;
        candidate.mythicLevel = mythicLevel;
        candidate.bestTime = totalTime;
        candidate.deaths = deaths;
        candidate.penaltySeconds = penaltyOnDeath * deaths;
        candidate.completedInTime = completedInTime;
        candidate.score = score;
        candidate.groupMembers = groupMembers;
        candidate.lastUpdate = now;

        QueryResult result = CharacterDatabase.Query(
            "SELECT mythic_level, best_time, deaths, penalty_seconds, completed_in_time, score FROM mythic_plus_leaderboard WHERE season_id = {} AND char_guid = {} AND map_id = {} AND difficulty = {}",
            candidate.seasonId, candidate.charGuid, candidate.mapId, candidate.difficulty);

        if (result)
        {
            Field* fields = result->Fetch();
            MythicPlusLeaderboardEntry existing;
            existing.seasonId = candidate.seasonId;
            existing.charGuid = candidate.charGuid;
            existing.mapId = candidate.mapId;
            existing.difficulty = candidate.difficulty;
            existing.mythicLevel = fields[0].Get<uint32>();
            existing.bestTime = fields[1].Get<uint32>();
            existing.deaths = fields[2].Get<uint32>();
            existing.penaltySeconds = fields[3].Get<uint32>();
            existing.completedInTime = fields[4].Get<bool>();
            existing.score = fields[5].Get<uint32>();

            if (!ShouldReplaceLeaderboardEntry(existing, candidate))
                continue;
        }

        std::string charName = candidate.charName;
        CharacterDatabase.EscapeString(charName);
        CharacterDatabase.Execute(
            "REPLACE INTO mythic_plus_leaderboard (season_id, char_guid, char_name, map_id, difficulty, mythic_level, best_time, deaths, penalty_seconds, completed_in_time, score, group_members, last_update) VALUES ({}, {}, \"{}\", {}, {}, {}, {}, {}, {}, {}, {}, \"{}\", {})",
            candidate.seasonId, candidate.charGuid, charName, candidate.mapId, candidate.difficulty,
            candidate.mythicLevel, candidate.bestTime, candidate.deaths, candidate.penaltySeconds,
            candidate.completedInTime, candidate.score, candidate.groupMembers, candidate.lastUpdate);
    }
}

void MythicPlus::MythicPlusSnapshotsDBCallback(QueryResult result)
{
    dungeonMapSnapshots.clear();

    if (!result)
        return;

    std::unordered_map<uint32, std::map<std::pair<uint32, uint64>, std::vector<MythicPlusDungeonSnapshot>>> mapSnapshots;
    do
    {
        Field* fields = result->Fetch();

        MythicPlusDungeonSnapshot snapshot;
        snapshot.id = fields[0].Get<uint32>();
        snapshot.startTime = fields[1].Get<uint64>();
        snapshot.snapTime = fields[2].Get<uint64>();
        snapshot.entry = fields[3].Get<uint32>();
        snapshot.mapId = fields[4].Get<uint32>();
        snapshot.players = fields[5].Get<std::string>();
        snapshot.combatTime = fields[6].Get<uint32>();
        snapshot.mythicLevel = fields[7].Get<uint32>();
        snapshot.finalBoss = fields[8].Get<bool>();
        snapshot.rewarded = fields[9].Get<bool>();
        snapshot.difficulty = fields[10].Get<bool>();
        snapshot.timelimit = fields[11].Get<uint32>();
        snapshot.penaltyOnDeath = fields[12].Get<uint32>();
        snapshot.deaths = fields[13].Get<uint32>();
        snapshot.randomAffixCount = fields[14].Get<uint32>();
        mapSnapshots[snapshot.mapId][std::make_pair(snapshot.id, snapshot.startTime)].push_back(snapshot);
    } while (result->NextRow());

    for (auto& mpair : mapSnapshots)
    {
        std::map<std::pair<uint32, uint64>, std::vector<MythicPlusDungeonSnapshot>>& snapshots = mpair.second;
        for (auto& spair : snapshots)
        {
            std::vector<MythicPlusDungeonSnapshot>& snaps = spair.second;
            uint32 totalTime = 0;
            uint32 endTime = 0;
            bool rewarded = false;
            uint32 totalDeaths = 0;
            for (const auto& s : snaps)
            {
                if (s.finalBoss)
                {
                    rewarded = s.rewarded;
                    totalTime = s.snapTime - s.startTime;
                    endTime = s.snapTime;
                    totalDeaths = s.deaths;
                    break;
                }
            }
            for (auto& s : snaps)
            {
                s.rewarded = rewarded;
                s.totalTime = totalTime;
                s.endTime = endTime;
                s.totalDeaths = totalDeaths;
            }
        }

        std::vector<std::pair<std::pair<uint32, uint64>, std::vector<MythicPlusDungeonSnapshot>>> sortedSnapshots(snapshots.begin(), snapshots.end());
        SortSnapshots(sortedSnapshots);

        dungeonMapSnapshots[mpair.first] = sortedSnapshots;
    }
}

const std::vector<std::pair<std::pair<uint32, uint64>, std::vector<MythicPlus::MythicPlusDungeonSnapshot>>> MythicPlus::GetMapSnapshot(uint32 mapId, uint32 mythicLevel) const
{
    std::vector<std::pair<std::pair<uint32, uint64>, std::vector<MythicPlus::MythicPlusDungeonSnapshot>>> result;
    if (dungeonMapSnapshots.find(mapId) != dungeonMapSnapshots.end())
    {
        const std::vector<std::pair<std::pair<uint32, uint64>, std::vector<MythicPlus::MythicPlusDungeonSnapshot>>>* snaps = &dungeonMapSnapshots.at(mapId);
        for (const auto& s : *snaps)
            if (mythicLevel == 0 || s.second.at(0).mythicLevel == mythicLevel)
                result.push_back(s);
    }

    return result;
}

void MythicPlus::SortSnapshots(std::vector<std::pair<std::pair<uint32, uint64>, std::vector<MythicPlusDungeonSnapshot>>>& snapshots)
{
    std::sort(snapshots.begin(), snapshots.end(), [](const auto& a, const auto& b) {
        const std::vector<MythicPlusDungeonSnapshot>& snapA = a.second;
        const std::vector<MythicPlusDungeonSnapshot>& snapB = b.second;
        uint32 snapATime = snapA.at(0).totalTime;
        uint32 snapBTime = snapB.at(0).totalTime;
        if (snapATime == 0 && snapBTime == 0)
            return false;
        if (snapATime == 0)
            return false;
        if (snapBTime == 0)
            return true;

        return snapATime < snapBTime;
    });

    uint32 internalId = 0;
    for (auto& s : snapshots)
    {
        internalId++;
        for (auto& e : s.second)
            e.internalId = internalId;
    }
}

void MythicPlus::ProcessConfig(bool reload)
{
    if (!reload)
        enabled = sConfigMgr->GetOption<bool>("MythicPlus.Enable", true);

    penaltyOnDeath = sConfigMgr->GetOption<uint32>("MythicPlus.Penalty.OnDeath", 15);
    keystoneBuyTimer = sConfigMgr->GetOption<uint32>("MythicPlus.KeystoneBuyTimer", 1440);
    dropKeystoneOnCompletion = sConfigMgr->GetOption<bool>("MythicPlus.DropKeystoneOnDungeonComplete", true);
    itemUpgradeTokenEntry = sConfigMgr->GetOption<uint32>("MythicPlus.ItemUpgradeTokenEntry", 0);
    itemUpgradeTokenCount = sConfigMgr->GetOption<uint32>("MythicPlus.ItemUpgradeTokenCount", 1);
    itemUpgradeTokenSkipFinalBoss = sConfigMgr->GetOption<bool>("MythicPlus.ItemUpgradeTokenSkipFinalBoss", false);
    if (itemUpgradeTokenCount < 1)
        itemUpgradeTokenCount = 1;
}

bool MythicPlus::MatchMythicPlusMapDiff(const Map* map) const
{
    if (!map || !map->GetEntry())
        return false;

    if (!map->IsNonRaidDungeon())
        return false;

    if (mythicPlusDungeons.find(map->GetId()) != mythicPlusDungeons.end())
    {
        Difficulty mapDiff = map->GetDifficulty();
        Difficulty dungeonDiff = mythicPlusDungeons.at(map->GetId()).minDifficulty;
        return mapDiff >= dungeonDiff;
    }

    return false;
}

bool MythicPlus::IsCreatureIgnoredForMultiplyAffix(uint32 entry) const
{
    return ignoredEntriesForMultiplyAffix.find(entry) != ignoredEntriesForMultiplyAffix.end();
}

bool MythicPlus::GiveKeystone(Player* player)
{
    uint64 now = Utils::GameTimeCount();
    if (keystoneBuyTimer > 0)
    {
        uint64 lastBuyTime = charKeystoneBuyTimers[Utils::PlayerGUID(player)];
        if (lastBuyTime > 0)
        {
            uint64 diff = now - lastBuyTime;
            if (diff < keystoneBuyTimer * 60)
            {
                std::ostringstream oss;
                oss << "You can buy another Mythic Keystone in ";
                oss << secsToTimeString(keystoneBuyTimer * 60 - diff);
                BroadcastToPlayer(player, oss.str());
                return false;
            }
        }
    }
    if (!DeliverMythicStacksOrMail(player, KEYSTONE_ENTRY, 1,
            "Mythic Plus",
            "Your Mythic Keystone could not be placed in your bags. It is attached to this letter."))
    {
        BroadcastToPlayer(player, "Can't deliver Mythic Keystone. Check your inventory and mailbox.");
        return false;
    }

    charKeystoneBuyTimers[Utils::PlayerGUID(player)] = now;
    CharacterDatabase.Execute("REPLACE INTO mythic_plus_keystone_timer (guid, buytime) VALUES ({}, {})", Utils::PlayerGUID(player), now);

    return true;
}

void MythicPlus::RemoveKeystone(Player* player) const
{
    player->DestroyItemCount(KEYSTONE_ENTRY, 1, true);
}

void MythicPlus::RewardKeystone(Player* player) const
{
    if (!dropKeystoneOnCompletion)
        return;

    if (!DeliverMythicStacksOrMail(player, KEYSTONE_ENTRY, 1,
            "Mythic Plus",
            "Your Mythic Keystone could not be placed in your bags. It is attached to this letter."))
        LOG_ERROR("module", "MythicPlus: failed to deliver completion keystone to {}", player->GetName());
}

uint64 MythicPlus::GetKeystoneBuyTimer(const Player* player) const
{
    if (charKeystoneBuyTimers.find(Utils::PlayerGUID(player)) != charKeystoneBuyTimers.end())
        return charKeystoneBuyTimers.at(Utils::PlayerGUID(player));

    return 0;
}

void MythicPlus::ScaleCreature(Creature* creature)
{
    CreatureData* creatureData = sMythicPlus->GetCreatureData(creature, false);
    ASSERT(creatureData);

    Map* map = creature->GetMap();
    MapData* mapData = GetMapData(map, false);
    MythicLevel const* mythicLevel =
        (mapData && mapData->mythicLevel) ? mapData->mythicLevel : nullptr;

    bool boss = IsBoss(creature);
    const MapScale* mapScale = GetMapScale(map);
    float dmgScale = 1.0f;
    if (mapScale != nullptr)
        dmgScale = boss ? mapScale->bossDmgScale : mapScale->trashDmgScale;
    if (mythicLevel != nullptr)
        dmgScale *= mythicLevel->dmgMult;
    creatureData->extraDamageMultiplier = dmgScale;

    float hpLevelMult = (mythicLevel != nullptr) ? mythicLevel->hpMult : 1.0f;
    if (hpLevelMult <= 0.0f)
        hpLevelMult = 1.0f;

    // Heroic WotLK trash is already level 80+: scale health by key so geared
    // players do not trivialize low keys.
    if (creature->GetLevel() >= DEFAULT_MAX_LEVEL)
    {
        if (hpLevelMult != 1.0f)
        {
            uint32 baseHp = creature->GetMaxHealth();
            uint32 newHealth = std::max(1u, uint32(std::ceil(float(baseHp) * hpLevelMult)));
            creature->SetCreateHealth(newHealth);
            creature->SetMaxHealth(newHealth);
            creature->SetStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE, float(newHealth));
            creature->SetHealth(uint32(std::ceil(float(newHealth) * (creature->GetHealthPct() / 100.0f))));
            creature->ResetPlayerDamageReq();
        }
        return;
    }

    // assume level 82 for bosses and level [80, 81] for trash mobs
    uint8 chosenLevel = boss ? 82 : urand(80, 81);

    CreatureTemplate const* cInfo = sObjectMgr->GetCreatureTemplate(creature->GetEntry());
    ASSERT(cInfo);

    CreatureBaseStats const* stats = sObjectMgr->GetCreatureBaseStats(chosenLevel, cInfo->unit_class);
    ASSERT(stats);

    creature->SetLevel(chosenLevel);

    uint8 exp = EXPANSION_WRATH_OF_THE_LICH_KING; // all mobs should scale to WOTLK expansion

    uint32 hpMod = boss ? urand(20, 21) : 4;
    if (map->IsHeroic())
        hpMod = boss ? urand(30, 31) : 5;

    // scale health
    uint32 health = uint32(std::ceil(float(stats->BaseHealth[exp] * hpMod * Utils::HealthMod(cInfo->rank)) * hpLevelMult));
    creature->SetCreateHealth(health);
    creature->SetMaxHealth(health);
    creature->SetHealth(health);
    creature->ResetPlayerDamageReq();
    creature->SetStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE, (float)health);

    // scale mana
    float manaMod = std::max(1.0f, cInfo->ModMana);
    uint32 mana = (uint32)std::ceil(stats->BaseMana * manaMod);

    creature->SetCreateMana(mana);
    creature->SetMaxPower(POWER_MANA, mana);
    creature->SetPower(POWER_MANA, mana);
    creature->SetStatFlatModifier(UNIT_MOD_MANA, BASE_VALUE, (float)mana);

    // scale armor
    float armor = std::ceil(stats->BaseArmor * cInfo->ModArmor);
    creature->SetStatFlatModifier(UNIT_MOD_ARMOR, BASE_VALUE, armor);

    // scale base damage
    float basedamage = stats->BaseDamage[exp];

    float weaponBaseMinDamage = basedamage;
    float weaponBaseMaxDamage = basedamage * 1.5;

    creature->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, weaponBaseMinDamage);
    creature->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

    creature->SetBaseWeaponDamage(OFF_ATTACK, MINDAMAGE, weaponBaseMinDamage);
    creature->SetBaseWeaponDamage(OFF_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

    creature->SetBaseWeaponDamage(RANGED_ATTACK, MINDAMAGE, weaponBaseMinDamage);
    creature->SetBaseWeaponDamage(RANGED_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

    creature->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER, BASE_VALUE, stats->AttackPower);
    creature->SetStatFlatModifier(UNIT_MOD_ATTACK_POWER_RANGED, BASE_VALUE, stats->RangedAttackPower);

    creature->SetCanModifyStats(true);
    creature->UpdateAllStats();
}

bool MythicPlus::IsBoss(Creature* creature) const
{
    return creature->IsDungeonBoss() || IsFinalBoss(creature->GetEntry());
}

const MythicPlus::MapScale* MythicPlus::GetMapScale(const Map* map) const
{
    uint32 entry = map->GetId();
    if (scaleMap.find(entry) != scaleMap.end())
    {
        if (scaleMap.at(entry).find(map->GetDifficulty()) != scaleMap.at(entry).end())
            return &scaleMap.at(entry).at(map->GetDifficulty());
    }

    return nullptr;
}

bool MythicPlus::CheckGroupLevelForKeystone(const Player* player) const
{
    const Group* group = player->GetGroup();
    if (group == nullptr)
        return false;

    for (Group::member_citerator mitr = group->GetMemberSlots().begin(); mitr != group->GetMemberSlots().end(); ++mitr)
    {
        Player* member = ObjectAccessor::FindConnectedPlayer(mitr->guid);
        if (!member)
            return false;
        if (member->GetLevel() < DEFAULT_MAX_LEVEL)
            return false;
    }

    return true;
}

const MythicPlus::SpellOverride* MythicPlus::GetSpellOverride(const Map* map, uint32 spellid) const
{
    if (spellOverrides.find(map->GetId()) != spellOverrides.end())
        if (spellOverrides.at(map->GetId()).find(spellid) != spellOverrides.at(map->GetId()).end())
            return &spellOverrides.at(map->GetId()).at(spellid);

    return nullptr;
}
