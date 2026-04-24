-- mythic_plus_level: hp_mult / dmg_mult were added to base schema (b_mythic_plus_level.sql) but no
-- ALTER was shipped; existing realms still have the old three-column layout, causing startup query errors.
-- Idempotent: skips ADD when columns already exist (e.g. after u_mp_2026_04_07_02 or base import).

SET @hp_mult_exists := (
    SELECT COUNT(*)
    FROM INFORMATION_SCHEMA.COLUMNS
    WHERE TABLE_SCHEMA = DATABASE()
      AND TABLE_NAME = 'mythic_plus_level'
      AND COLUMN_NAME = 'hp_mult'
);
SET @sql := IF(
    @hp_mult_exists = 0,
    'ALTER TABLE `mythic_plus_level` ADD COLUMN `hp_mult` float NOT NULL DEFAULT 1.0 AFTER `random_affix_count`',
    'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @dmg_mult_exists := (
    SELECT COUNT(*)
    FROM INFORMATION_SCHEMA.COLUMNS
    WHERE TABLE_SCHEMA = DATABASE()
      AND TABLE_NAME = 'mythic_plus_level'
      AND COLUMN_NAME = 'dmg_mult'
);
SET @sql := IF(
    @dmg_mult_exists = 0,
    'ALTER TABLE `mythic_plus_level` ADD COLUMN `dmg_mult` float NOT NULL DEFAULT 1.0 AFTER `hp_mult`',
    'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- Match b_mythic_plus_level.sql for levels 1-5; extend same per-level step for 6-9 (+0.08 hp, +0.04 dmg).
UPDATE `mythic_plus_level` SET `hp_mult` = 1.35, `dmg_mult` = 1.18 WHERE `lvl` = 1;
UPDATE `mythic_plus_level` SET `hp_mult` = 1.42, `dmg_mult` = 1.22 WHERE `lvl` = 2;
UPDATE `mythic_plus_level` SET `hp_mult` = 1.50, `dmg_mult` = 1.26 WHERE `lvl` = 3;
UPDATE `mythic_plus_level` SET `hp_mult` = 1.58, `dmg_mult` = 1.30 WHERE `lvl` = 4;
UPDATE `mythic_plus_level` SET `hp_mult` = 1.66, `dmg_mult` = 1.34 WHERE `lvl` = 5;
UPDATE `mythic_plus_level` SET `hp_mult` = 1.74, `dmg_mult` = 1.38 WHERE `lvl` = 6;
UPDATE `mythic_plus_level` SET `hp_mult` = 1.82, `dmg_mult` = 1.42 WHERE `lvl` = 7;
UPDATE `mythic_plus_level` SET `hp_mult` = 1.90, `dmg_mult` = 1.46 WHERE `lvl` = 8;
UPDATE `mythic_plus_level` SET `hp_mult` = 1.98, `dmg_mult` = 1.50 WHERE `lvl` = 9;
