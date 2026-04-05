/*
 * Credits: silviu20092
 */

#ifndef _MYTHIC_AFFIX_H_
#define _MYTHIC_AFFIX_H_

class Creature;
class TempSummon;

enum MythicAffixType
{
    AFFIX_TYPE_HEALTH_INCREASE,
    AFFIX_TYPE_HEALTH_INCREASE_TRASH,
    AFFIX_TYPE_HEALTH_INCREASE_BOSSES,
    AFFIX_TYPE_MULTIPLE_ENEMIES,
    AFFIX_TYPE_MORE_CREATURE_DAMAGE,
    AFFIX_TYPE_RANDOMLY_EXPLODE,
    AFFIX_TYPE_LIGHTNING_SPHERE,
    AFFIX_TYPE_RANDOM_ENEMY_ENRAGE,
    AFFIX_TYPE_RANDOM_ENTANGLING_ROOTS,
    AFFIX_TYPE_FORTIFIED,
    AFFIX_TYPE_TYRANNICAL,
    AFFIX_TYPE_BOLSTERING,
    AFFIX_TYPE_SANGUINE,
    MAX_AFFIX_TYPE
};

class MythicAffix
{
public:
    virtual ~MythicAffix() = default;

    virtual MythicAffixType GetAffixType() const = 0;
    virtual std::string ToString() const = 0;

    virtual void HandleStaticEffect(Creature* /*creature*/) {}
    virtual void HandleOnDamageEffect(Unit* /*attacker*/, Unit* /*victim*/ , uint32& /*damage*/) {}
    virtual void HandlePeriodicEffect(Unit* /*unit*/, uint32 /*diff*/) {}
    virtual void HandlePeriodicEffectMap(Map* /*map*/, uint32 /*diff*/) {}
    virtual void HandleUnitDeath(Creature* /*creature*/, Unit* /*killer*/) {}

    virtual bool IsRandom() const
    {
        return false;
    }

    static MythicAffix* AffixFactory(MythicAffixType type, float val1, float val2);
    static MythicAffix* AffixFactory(MythicAffixType type);
    static std::vector<MythicAffix*> GenerateRandom(uint32 maxCount);
protected:
    static bool IsCreatureProcessed(Creature* creature);
public:
    static constexpr uint32 RANDOM_AFFIX_MAX_COUNT = 3;
    static constexpr uint32 RandomAffixes[RANDOM_AFFIX_MAX_COUNT] = { AFFIX_TYPE_RANDOM_ENEMY_ENRAGE, AFFIX_TYPE_RANDOM_ENTANGLING_ROOTS, AFFIX_TYPE_SANGUINE };
};

class HealthIncreaseAffix : public MythicAffix
{
private:
    float healthMod;
    bool CanApplyHealthIncrease(Creature* creature) const;
public:
    HealthIncreaseAffix(float healthMod) : healthMod(healthMod) {}

    virtual MythicAffixType GetAffixType() const override
    {
        return AFFIX_TYPE_HEALTH_INCREASE;
    }

    std::string ToString() const override;

    void HandleStaticEffect(Creature* creature) override;

    float GetHealthMod() const
    {
        return healthMod;
    }

    virtual bool GetApplyForTrash() const
    {
        return true;
    }

    virtual bool GetApplyForBosses() const
    {
        return true;
    }
};

class TrashHealthIncreaseAffix : public HealthIncreaseAffix
{
public:
    TrashHealthIncreaseAffix(float healthMod) : HealthIncreaseAffix(healthMod) {}

    MythicAffixType GetAffixType() const override
    {
        return AFFIX_TYPE_HEALTH_INCREASE_TRASH;
    }

    bool GetApplyForBosses() const override
    {
        return false;
    }
};

class BossHealthIncreaseAffix : public HealthIncreaseAffix
{
public:
    BossHealthIncreaseAffix(float healthMod) : HealthIncreaseAffix(healthMod) {}

    MythicAffixType GetAffixType() const override
    {
        return AFFIX_TYPE_HEALTH_INCREASE_BOSSES;
    }

    bool GetApplyForTrash() const override
    {
        return false;
    }
};

class MultipleEnemiesAffix : public MythicAffix
{
private:
    float chance;

    TempSummon* DoCreateCopy(Creature* creature);
public:
    MultipleEnemiesAffix(float chance) : chance(chance) {}

    MythicAffixType GetAffixType() const override
    {
        return AFFIX_TYPE_MULTIPLE_ENEMIES;
    }

    void HandleStaticEffect(Creature* creature) override;
    std::string ToString() const override;
};

class MoreDamageForCreaturesAffix : public MythicAffix
{
private:
    float perc;
public:
    MoreDamageForCreaturesAffix(float perc) : perc(perc) {}

    MythicAffixType GetAffixType() const override
    {
        return AFFIX_TYPE_MORE_CREATURE_DAMAGE;
    }

    void HandleOnDamageEffect(Unit* attacker, Unit* victim, uint32& damage) override;
    std::string ToString() const override;
};

class RandomlyExplodeAffix : public MythicAffix
{
private:
    static constexpr uint32 EXPLOSION_VISUAL = 71495;
    std::unordered_map<uint32, uint32> timerMap;
public:
    RandomlyExplodeAffix() {}

    MythicAffixType GetAffixType() const override
    {
        return AFFIX_TYPE_RANDOMLY_EXPLODE;
    }

    void HandlePeriodicEffect(Unit* unit, uint32 diff) override;
    std::string ToString() const override;
};

class LightningSphereAffix : public MythicAffix
{
private:
    uint32 spawnTimer = 0;
    uint32 spawnTimerEnd = 0;
    float chanceOfSpawn = 0;
public:
    LightningSphereAffix(uint32 spawnTimerEnd, float chanceOfSpawn) : spawnTimerEnd(spawnTimerEnd), chanceOfSpawn(chanceOfSpawn) {}

    MythicAffixType GetAffixType() const override
    {
        return AFFIX_TYPE_LIGHTNING_SPHERE;
    }

    void HandlePeriodicEffectMap(Map* map, uint32 diff) override;
    std::string ToString() const override;
};

class EnemyEnrageAffix : public MythicAffix
{
private:
    static constexpr float chance = 6.5f;
    static constexpr uint32 ENRAGE_SPELL_ID = 55285;
    static constexpr uint32 checkAtTimer = 2000;

    std::unordered_map<uint32, uint32> timerMap;
public:
    EnemyEnrageAffix() {}

    MythicAffixType GetAffixType() const override
    {
        return AFFIX_TYPE_RANDOM_ENEMY_ENRAGE;
    }

    bool IsRandom() const override
    {
        return true;
    }

    void HandlePeriodicEffect(Unit* unit, uint32 diff) override;
    std::string ToString() const override;
};

class EntanglingRootsAffix : public MythicAffix
{
private:
    static constexpr float chance = 15.33f;
    static constexpr uint32 checkAtTimer = 10000;

    std::unordered_map<uint32, uint32> timerMap;
public:
    static constexpr uint32 ENTANGLING_ROOTS_SPELL_ID = 57095;

    EntanglingRootsAffix() {}

    MythicAffixType GetAffixType() const override
    {
        return AFFIX_TYPE_RANDOM_ENTANGLING_ROOTS;
    }

    bool IsRandom() const override
    {
        return true;
    }

    void HandlePeriodicEffect(Unit* unit, uint32 diff) override;
    std::string ToString() const override;
};

class FortifiedAffix : public MythicAffix
{
public:
    FortifiedAffix(float healthMod, float damagePct)
        : healthMod(healthMod), damagePct(damagePct) {}

    MythicAffixType GetAffixType() const override
    {
        return AFFIX_TYPE_FORTIFIED;
    }

    void HandleStaticEffect(Creature* creature) override;
    void HandleOnDamageEffect(Unit* attacker, Unit* victim, uint32& damage) override;
    std::string ToString() const override;

private:
    float healthMod;
    float damagePct;
};

class TyrannicalAffix : public MythicAffix
{
public:
    TyrannicalAffix(float healthMod, float damagePct)
        : healthMod(healthMod), damagePct(damagePct) {}

    MythicAffixType GetAffixType() const override
    {
        return AFFIX_TYPE_TYRANNICAL;
    }

    void HandleStaticEffect(Creature* creature) override;
    void HandleOnDamageEffect(Unit* attacker, Unit* victim, uint32& damage) override;
    std::string ToString() const override;

private:
    float healthMod;
    float damagePct;
};

class BolsteringAffix : public MythicAffix
{
public:
    BolsteringAffix(float healthModPct, float damagePct, float radius)
        : healthModPct(healthModPct), damagePct(damagePct), radius(radius) {}

    MythicAffixType GetAffixType() const override
    {
        return AFFIX_TYPE_BOLSTERING;
    }

    void HandleUnitDeath(Creature* creature, Unit* killer) override;
    void HandleOnDamageEffect(Unit* attacker, Unit* victim, uint32& damage) override;
    std::string ToString() const override;

private:
    void ApplyBolsterToCreature(Creature* creature) const;

    float healthModPct;
    float damagePct;
    float radius;
};

class SanguineAffix : public MythicAffix
{
public:
    SanguineAffix(float effectPct, uint32 durationMs, float radius)
        : effectPct(effectPct), durationMs(durationMs), radius(radius) {}

    MythicAffixType GetAffixType() const override
    {
        return AFFIX_TYPE_SANGUINE;
    }

    bool IsRandom() const override
    {
        return true;
    }

    void HandlePeriodicEffect(Unit* unit, uint32 diff) override;
    void HandlePeriodicEffectMap(Map* map, uint32 diff) override;
    void HandleUnitDeath(Creature* creature, Unit* killer) override;
    std::string ToString() const override;

private:
    struct SanguinePool
    {
        uint32 instanceId = 0;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        uint32 remainingMs = 0;
    };

    void PruneExpiredPools(uint32 instanceId = 0);
    bool IsUnitInsideSanguinePool(Unit* unit) const;

    float effectPct;
    uint32 durationMs;
    float radius;
    std::vector<SanguinePool> activePools;
    std::unordered_map<uint64, uint32> unitTickTimers;
};

#endif
