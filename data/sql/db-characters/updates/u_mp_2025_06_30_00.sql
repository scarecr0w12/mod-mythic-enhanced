SET @col_exists := (
    SELECT COUNT(*)
    FROM INFORMATION_SCHEMA.COLUMNS
    WHERE TABLE_SCHEMA = DATABASE()
      AND TABLE_NAME = 'mythic_plus_dungeon_snapshot'
      AND COLUMN_NAME = 'random_affix_count'
);
SET @sql := IF(@col_exists = 0,
    'ALTER TABLE `mythic_plus_dungeon_snapshot` ADD COLUMN `random_affix_count` int unsigned NOT NULL DEFAULT 0',
    'SELECT 1'
);
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
