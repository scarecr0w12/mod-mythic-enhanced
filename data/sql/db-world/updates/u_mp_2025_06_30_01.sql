SET @col_exists := (
    SELECT COUNT(*)
    FROM INFORMATION_SCHEMA.COLUMNS
    WHERE TABLE_SCHEMA = DATABASE()
      AND TABLE_NAME = 'mythic_plus_level'
      AND COLUMN_NAME = 'random_affix_count'
);
SET @sql := IF(@col_exists = 0,
    'ALTER TABLE `mythic_plus_level` ADD COLUMN `random_affix_count` int unsigned NOT NULL DEFAULT 0',
    'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`)
VALUES (7, 60 * 40, 1)
ON DUPLICATE KEY UPDATE
    `timelimit` = VALUES(`timelimit`),
    `random_affix_count` = VALUES(`random_affix_count`);

INSERT INTO `mythic_plus_affix` (`lvl`, `affixtype`, `val1`) VALUES (7, 1, 0.2);
INSERT INTO `mythic_plus_affix` (`lvl`, `affixtype`, `val1`) VALUES (7, 2, 0.15);
INSERT INTO `mythic_plus_affix` (`lvl`, `affixtype`, `val1`) VALUES (7, 3, 30);
INSERT INTO `mythic_plus_affix` (`lvl`, `affixtype`, `val1`) VALUES (7, 4, 30);
INSERT INTO `mythic_plus_affix` (`lvl`, `affixtype`, `val1`, `val2`) VALUES (7, 6, 25000, 65);

INSERT INTO `mythic_plus_level_rewards` (`lvl`, `rewardtype`, `val1`, `val2`) VALUES (7, 0, 30000000, null);
INSERT INTO `mythic_plus_level_rewards` (`lvl`, `rewardtype`, `val1`, `val2`) VALUES (7, 1, 29434, 30);
