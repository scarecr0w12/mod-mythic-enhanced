/*
 * Credits: silviu20092
 */

#include "Chat.h"
#include "CommandScript.h"
#include "DBCStores.h"
#include "mythic_plus.h"

using namespace Acore::ChatCommands;

class mythic_plus_commandscript : public CommandScript
{
public:
    mythic_plus_commandscript() : CommandScript("mythic_plus_commandscript") {}

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable mythicCommandTable =
        {
            { "leaderboard map", HandleMythicLeaderboardMapCommand,  SEC_PLAYER,        Console::No  },
            { "leaderboard maps", HandleMythicLeaderboardMapsCommand, SEC_PLAYER,        Console::No  },
            { "leaderboard",     HandleMythicLeaderboardCommand,     SEC_PLAYER,        Console::No  },
            { "info",            HandleMythicInfoCommand,            SEC_PLAYER,        Console::No  },
            { "rating",          HandleMythicRatingCommand,          SEC_PLAYER,        Console::No  },
            { "reload",          HandleMythicReloadCommand,          SEC_ADMINISTRATOR, Console::Yes },
            { "season history",  HandleMythicSeasonHistoryCommand,   SEC_PLAYER,        Console::No  },
            { "season rewards",  HandleMythicSeasonRewardsCommand,   SEC_ADMINISTRATOR, Console::Yes },
            { "season",          HandleMythicSeasonCommand,          SEC_PLAYER,        Console::No  }
        };
        static ChatCommandTable commandTable =
        {
            { "mythic", mythicCommandTable }
        };
        return commandTable;
    }

private:
    static bool HandleMythicInfoCommand(ChatHandler* handler)
    {
        Player* player = handler->GetPlayer();
        if (player && sMythicPlus->IsInMythicPlus(player))
        {
            const MythicPlus::MapData* mapData = sMythicPlus->GetMapData(player->GetMap(), false);
            ASSERT(mapData);

            const MythicLevel* level = mapData->mythicLevel;
            ASSERT(level);

            sMythicPlus->PrintMythicLevelInfo(level, player);
        }
        else
            handler->SendSysMessage("You are not in a Mythic Plus dungeon right now.");

        return true;
    }

    static bool HandleMythicSeasonCommand(ChatHandler* handler, Optional<uint32> seasonIdArg)
    {
        MythicPlus::MythicPlusSeason const* season = seasonIdArg ? sMythicPlus->GetSeason(*seasonIdArg) : sMythicPlus->GetActiveSeason();
        if (!season)
        {
            handler->SendSysMessage(seasonIdArg ? "Requested Mythic Plus season was not found." : "No active Mythic Plus season found.");
            return true;
        }

        handler->PSendSysMessage("Mythic season #{}: {}{}", season->id, season->label, season->isActive ? " [active]" : "");
        handler->PSendSysMessage("Season window: {} -> {}", MythicPlus::Utils::DateFromSeconds(season->startUnix), MythicPlus::Utils::DateFromSeconds(season->endUnix));
        if (season->isActive)
            handler->PSendSysMessage("Season reset in: {}", secsToTimeString(sMythicPlus->GetSecondsUntilSeasonEnd()));

        return true;
    }

    static bool HandleMythicSeasonHistoryCommand(ChatHandler* handler)
    {
        std::vector<MythicPlus::MythicPlusSeason> seasons = sMythicPlus->GetRecentSeasons();
        if (seasons.empty())
        {
            handler->SendSysMessage("No Mythic Plus seasons have been recorded yet.");
            return true;
        }

        handler->SendSysMessage("Recent Mythic seasons:");
        for (std::size_t i = 0; i < seasons.size(); ++i)
        {
            handler->PSendSysMessage("{}. #{} {}{} [{} -> {}]",
                i + 1,
                seasons[i].id,
                seasons[i].label,
                seasons[i].isActive ? " [active]" : "",
                MythicPlus::Utils::DateFromSeconds(seasons[i].startUnix),
                MythicPlus::Utils::DateFromSeconds(seasons[i].endUnix));
        }

        return true;
    }

    static bool HandleMythicRatingCommand(ChatHandler* handler, Optional<uint32> seasonIdArg)
    {
        Player* player = handler->GetPlayer();
        if (!player)
        {
            handler->SendSysMessage("This command requires an in-game player.");
            return true;
        }

        uint32 seasonId = seasonIdArg ? *seasonIdArg : 0;
        MythicPlus::MythicPlusPlayerRatingSummary summary = sMythicPlus->GetPlayerRatingSummary(player->GetGUID().GetCounter(), seasonId);
        MythicPlus::MythicPlusSeason const* season = seasonIdArg ? sMythicPlus->GetSeason(*seasonIdArg) : sMythicPlus->GetActiveSeason();
        if (!season)
        {
            handler->SendSysMessage(seasonIdArg ? "Requested Mythic Plus season was not found." : "No active Mythic Plus season found.");
            return true;
        }

        handler->PSendSysMessage("Season #{}: {}", season->id, season->label);
        handler->PSendSysMessage("Your total Mythic rating: {}", summary.totalScore);
        handler->PSendSysMessage("Your best Mythic level: {}", summary.bestLevel);
        handler->PSendSysMessage("Ranked dungeon entries this season: {}", summary.runs);
        if (summary.overallRank > 0)
            handler->PSendSysMessage("Your current overall rank: {}", summary.overallRank);
        else
            handler->SendSysMessage("You have no leaderboard entries for the selected season yet.");

        return true;
    }

    static bool HandleMythicLeaderboardCommand(ChatHandler* handler, Optional<uint32> seasonIdArg)
    {
        uint32 seasonId = seasonIdArg ? *seasonIdArg : 0;
        MythicPlus::MythicPlusSeason const* season = seasonIdArg ? sMythicPlus->GetSeason(*seasonIdArg) : sMythicPlus->GetActiveSeason();
        if (!season)
        {
            handler->SendSysMessage(seasonIdArg ? "Requested Mythic Plus season was not found." : "No active Mythic Plus season found.");
            return true;
        }

        std::vector<MythicPlus::MythicPlusOverallLeaderboardEntry> entries = sMythicPlus->GetOverallLeaderboard(10, seasonId);
        handler->PSendSysMessage("Overall Mythic leaderboard for season #{} ({})", season->id, season->label);
        if (entries.empty())
        {
            handler->SendSysMessage("No runs have been ranked yet.");
            return true;
        }

        for (std::size_t i = 0; i < entries.size(); ++i)
        {
            MythicPlus::MythicPlusOverallLeaderboardEntry const& entry = entries[i];
            handler->PSendSysMessage("{}. {} - score {}, best key {}, runs {}",
                i + 1, entry.charName, entry.totalScore, entry.bestLevel, entry.runs);
        }

        return true;
    }

    static bool HandleMythicLeaderboardMapCommand(ChatHandler* handler, uint32 mapId, Optional<uint32> seasonIdArg)
    {
        uint32 seasonId = seasonIdArg ? *seasonIdArg : 0;
        MythicPlus::MythicPlusSeason const* season = seasonIdArg ? sMythicPlus->GetSeason(*seasonIdArg) : sMythicPlus->GetActiveSeason();
        if (!season)
        {
            handler->SendSysMessage(seasonIdArg ? "Requested Mythic Plus season was not found." : "No active Mythic Plus season found.");
            return true;
        }

        MapEntry const* map = sMapStore.LookupEntry(mapId);
        if (!map)
        {
            handler->PSendSysMessage("Map {} was not found.", mapId);
            return true;
        }

        uint32 locale = handler->GetSession() ? handler->GetSession()->GetSessionDbcLocale() : 0;
        std::string mapName = map->name[locale];
        if (mapName.empty())
            mapName = map->name[0];

        std::vector<MythicPlus::MythicPlusLeaderboardEntry> entries = sMythicPlus->GetMapLeaderboard(mapId, 10, seasonId);
        handler->PSendSysMessage("Mythic leaderboard for {} (season #{} / {})", mapName, season->id, season->label);
        if (entries.empty())
        {
            handler->SendSysMessage("No ranked runs found for that map in the selected season.");
            return true;
        }

        for (std::size_t i = 0; i < entries.size(); ++i)
        {
            MythicPlus::MythicPlusLeaderboardEntry const& entry = entries[i];
            handler->PSendSysMessage("{}. {} - score {}, key {}, time {}, deaths {}{}",
                i + 1, entry.charName, entry.score, entry.mythicLevel,
                secsToTimeString(entry.bestTime), entry.deaths,
                entry.completedInTime ? " [timed]" : " [overtime]");
        }

        return true;
    }

    static bool HandleMythicSeasonRewardsCommand(ChatHandler* handler, Optional<uint32> seasonIdArg)
    {
        uint32 seasonId = seasonIdArg ? *seasonIdArg : 0;
        if (!sMythicPlus->DistributeSeasonRewards(seasonId))
        {
            handler->SendSysMessage("No Mythic season rewards were distributed. Check that reward rows exist, rankings are populated, and rewards were not already sent.");
            return true;
        }

        handler->SendSysMessage("Mythic season rewards were distributed successfully.");
        return true;
    }

    static bool HandleMythicLeaderboardMapsCommand(ChatHandler* handler)
    {
        uint32 locale = handler->GetSession() ? handler->GetSession()->GetSessionDbcLocale() : 0;
        handler->SendSysMessage("Mythic-capable dungeon maps:");

        for (auto const& dungeonPair : sMythicPlus->GetAllMythicPlusDungeons())
        {
            MapEntry const* map = sMapStore.LookupEntry(dungeonPair.first);
            if (!map)
                continue;

            std::string mapName = map->name[locale];
            if (mapName.empty())
                mapName = map->name[0];

            handler->PSendSysMessage("{} - {}", dungeonPair.first, mapName);
        }

        return true;
    }

    static bool HandleMythicReloadCommand(ChatHandler* handler)
    {
        sMythicPlus->LoadFromDB();
        sMythicPlus->EnsureActiveSeason();
        handler->SendGlobalGMSysMessage("Mythic Plus data was reloaded from the database, including seasonal rotations and rewards.");

        return true;
    }
};

void AddSC_mythic_plus_commandscript()
{
    new mythic_plus_commandscript();
}
