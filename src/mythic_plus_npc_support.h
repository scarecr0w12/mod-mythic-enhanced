/*
 * Credits: silviu20092
 */

#include "gossip_support.h"

class Player;

class MythicPlusNpcSupport : public GossipSupport
{
public:
    void AddMythicPlusLevels(Player* player);
    void AddMythicPlusLevelInfo(Player* player, uint32 mythicLevel);
    void AddMythicPlusDungeonList(Player* player);
    void AddMythicPlusDungeonListForSnapshots(Player* player, uint32 snapMythicLevel);
    void AddMythicPlusSnapshotAllRuns(Player* player, uint32 mapEntry);
    void AddMythicPlusAllLevels(Player* player);
    void AddMythicPlusDungeonSnapshotDetails(Player* player, uint32 internalId);
    void AddSeasonInfo(Player* player);
    void AddSeasonArchiveMenu(Player* player, uint32 seasonId);
    void AddOverallLeaderboard(Player* player);
    void AddDungeonListForLeaderboard(Player* player, uint32 seasonId = 0);
    void AddMapLeaderboard(Player* player, uint32 mapEntry, uint32 seasonId = 0);
    void AddRandomAfixes(Player* player);
    void AddRandomAffixesForLevel(Player* player, uint32 level);
    void AddHelpGuide(Player* player);
    void AddNpcSubmenuRun(Player* player);
    void AddNpcSubmenuRankings(Player* player);
    void AddNpcSubmenuHelp(Player* player);
    bool ProcessRunSubmenuAction(Player* player, Creature* creature, uint32 action);
    bool ProcessRankingsSubmenuAction(Player* player, Creature* creature, uint32 action);
    bool ProcessHelpSubmenuAction(Player* player, Creature* creature, uint32 action);
    bool TakePagedDataAction(Player* player, Creature* creature, uint32 action) override;
    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override;
private:
    static bool CompareIdentifierById(const Identifier* a, const Identifier* b);

    struct MythicPlusNpcPageInfo : public PagedDataCustomInfo
    {
        uint32 mythicLevel = 0;
        uint32 mapEntry = 0;
        uint32 snapMythicLevel = 0;
        uint32 internalId = 0;
        uint32 randomMythicLevel = 0;
        uint32 seasonId = 0;
    };
protected:
    uint32 _PageZeroSender(const PagedData& pagedData) const override;
    void AddMainMenu(Player* player, Creature* creature) override;
};
