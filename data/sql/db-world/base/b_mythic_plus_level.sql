DROP TABLE IF EXISTS `mythic_plus_level`;
CREATE TABLE `mythic_plus_level`(
	`lvl` int unsigned NOT NULL,
	`timelimit` int unsigned NOT NULL,
	`random_affix_count` int unsigned NOT NULL DEFAULT '0',
	`hp_mult` float NOT NULL DEFAULT '1',
	`dmg_mult` float NOT NULL DEFAULT '1',
	PRIMARY KEY (`lvl`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (2, 60*45, 0, 1.35, 1.18);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (3, 60*45, 0, 1.43, 1.22);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (4, 60*45, 0, 1.52, 1.26);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (5, 60*45, 0, 1.61, 1.31);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (6, 60*45, 0, 1.71, 1.36);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (7, 60*45, 0, 1.83, 1.42);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (8, 60*45, 0, 1.96, 1.48);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (9, 60*45, 0, 2.10, 1.55);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (10, 60*40, 0, 2.25, 1.63);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (11, 60*40, 0, 2.42, 1.72);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (12, 60*40, 0, 2.60, 1.82);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (13, 60*40, 0, 2.80, 1.93);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (14, 60*40, 0, 3.02, 2.05);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (15, 60*38, 0, 3.27, 2.19);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (16, 60*38, 0, 3.55, 2.34);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (17, 60*38, 0, 3.86, 2.50);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (18, 60*38, 0, 4.20, 2.68);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (19, 60*38, 0, 4.58, 2.87);
INSERT INTO `mythic_plus_level` (`lvl`, `timelimit`, `random_affix_count`, `hp_mult`, `dmg_mult`) VALUES (20, 60*35, 0, 5.00, 3.08);
