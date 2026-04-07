DROP TABLE IF EXISTS `mythic_plus_level`;
CREATE TABLE `mythic_plus_level`(
	`lvl` int unsigned NOT NULL,
	`timelimit` int unsigned NOT NULL,
	`random_affix_count` int unsigned NOT NULL DEFAULT '0',
	`hp_mult` float NOT NULL DEFAULT '1',
	`dmg_mult` float NOT NULL DEFAULT '1',
	PRIMARY KEY (`lvl`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (1, 60*45, 0, 1.35, 1.18);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (2, 60*45, 0, 1.42, 1.22);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (3, 60*40, 0, 1.50, 1.26);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (4, 60*40, 0, 1.58, 1.30);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (5, 60*40, 0, 1.66, 1.34);
