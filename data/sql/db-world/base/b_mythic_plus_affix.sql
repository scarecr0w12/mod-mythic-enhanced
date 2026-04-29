DROP TABLE IF EXISTS `mythic_plus_affix`;
CREATE TABLE `mythic_plus_affix`(
	`lvl` int unsigned NOT NULL,
	`affixtype` smallint unsigned NOT NULL,
	`val1` float,
	`val2` float
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
