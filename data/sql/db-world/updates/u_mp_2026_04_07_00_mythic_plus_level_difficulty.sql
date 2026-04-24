-- Per-key multipliers for max-level dungeon mobs (health) and outgoing damage scale.
-- Tune hp_mult / dmg_mult without recompiling; reload with `.mythic reload`.
-- Idempotent: same pattern as u_mp_2026_04_06_01 (columns may already exist from base / heal migration).

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

UPDATE `mythic_plus_level` SET `hp_mult` = 1.35, `dmg_mult` = 1.18 WHERE `lvl` = 1;
UPDATE `mythic_plus_level` SET `hp_mult` = 1.42, `dmg_mult` = 1.22 WHERE `lvl` = 2;
UPDATE `mythic_plus_level` SET `hp_mult` = 1.50, `dmg_mult` = 1.26 WHERE `lvl` = 3;
UPDATE `mythic_plus_level` SET `hp_mult` = 1.58, `dmg_mult` = 1.30 WHERE `lvl` = 4;
UPDATE `mythic_plus_level` SET `hp_mult` = 1.66, `dmg_mult` = 1.34 WHERE `lvl` = 5;
UPDATE `mythic_plus_level` SET `hp_mult` = 1.74, `dmg_mult` = 1.38 WHERE `lvl` = 6;
UPDATE `mythic_plus_level` SET `hp_mult` = 1.82, `dmg_mult` = 1.42 WHERE `lvl` = 7;
UPDATE `mythic_plus_level` SET `hp_mult` = 1.90, `dmg_mult` = 1.46 WHERE `lvl` = 8;
UPDATE `mythic_plus_level` SET `hp_mult` = 1.98, `dmg_mult` = 1.50 WHERE `lvl` = 9;
