CREATE TABLE `accounts` (
  `id` int(11) NOT NULL auto_increment,
  `accno` int(11) unsigned NOT NULL default '0',
  `password` varchar(32) NOT NULL default '',
  `type` int(11) NOT NULL default '0',
  `premDays` int(11) NOT NULL default '0',
  `premEnd` int(20) NOT NULL default '0',
  `email` varchar(50) NOT NULL default '',
  `blocked` tinyint(4) NOT NULL default '0',
  `rlname` varchar(45) NOT NULL default '',
  `location` varchar(45) NOT NULL default '',
  `hide` tinyint(1) NOT NULL default '0',
  `hidemail` tinyint(1) NOT NULL default '0',
  UNIQUE KEY `id` (`id`),
  KEY `accno` (`accno`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=3 ;

INSERT INTO `accounts` VALUES (1, 111111, 'tibia', 0, 0, 0, 'Xidaozu@hotmail.com', 0, 'Xidaozu', 'Holland', 0, 0);
INSERT INTO `accounts` VALUES (2, 222222, 'tibia', 0, 0, 0, 'Xidaozu@hotmail.com', 0, 'Xidaozu', 'Holland', 0, 0);

CREATE TABLE `addons` (
  `player` int(10) unsigned NOT NULL default '0',
  `outfit` int(10) unsigned NOT NULL default '0',
  `addon` int(10) unsigned NOT NULL default '0'
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE `bans` (
  `type` int(10) unsigned NOT NULL default '0',
  `ip` int(10) unsigned NOT NULL default '0',
  `mask` int(10) unsigned NOT NULL default '0',
  `player` varchar(32) NOT NULL default '0',
  `account` int(10) unsigned NOT NULL default '0',
  `time` int(10) unsigned NOT NULL default '0',
  `reason` varchar(50) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE `commands` (
  `id` int(11) NOT NULL,
  `date` varchar(25) collate latin1_general_ci NOT NULL,
  `player` varchar(32) collate latin1_general_ci NOT NULL,
  `command` varchar(50) collate latin1_general_ci NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COLLATE=latin1_general_ci;

CREATE TABLE `deathlist` (
  `player` int(11) NOT NULL default '0',
  `name` varchar(30) NOT NULL default '0',
  `killer` varchar(30) NOT NULL default '0',
  `level` int(11) NOT NULL default '0',
  `date` int(11) NOT NULL default '0'
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE `guildinvited` (
  `guildid` int(11) NOT NULL default '0',
  `playerid` int(11) NOT NULL default '0',
  `guildrank` varchar(32) NOT NULL default ''
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE `guilds` (
  `guildid` int(11) NOT NULL auto_increment,
  `guildname` varchar(100) NOT NULL default '',
  `ownerid` int(11) NOT NULL default '0',
  `guildstory` text NOT NULL,
  PRIMARY KEY  (`guildid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

CREATE TABLE `houseaccess` (
  `houseid` int(10) unsigned NOT NULL default '0',
  `listid` int(10) unsigned default '0',
  `list` text,
  KEY `houseid` USING BTREE (`houseid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE `houses` (
  `houseid` int(10) unsigned NOT NULL default '0',
  `owner` varchar(32) NOT NULL default '0',
  `paid` int(10) unsigned default '0',
  `warnings` int(10) unsigned default '0',
  PRIMARY KEY  (`houseid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE `items` (
  `player` int(11) NOT NULL default '0',
  `slot` tinyint(4) NOT NULL default '0',
  `sid` int(11) NOT NULL default '0',
  `pid` int(11) NOT NULL default '0',
  `type` int(11) NOT NULL default '0',
  `number` tinyint(4) NOT NULL default '0',
  `actionid` int(5) NOT NULL default '0',
  `duration` int(11) NOT NULL default '0',
  `charges` int(11) NOT NULL default '0',
  `decaystate` int(11) NOT NULL,
  `text` text NOT NULL,
  `specialdesc` text NOT NULL,
  KEY `player` (`player`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE `players` (
  `id` int(11) unsigned NOT NULL auto_increment,
  `name` varchar(32) NOT NULL default '',
  `access` int(11) NOT NULL default '0',
  `account` int(11) NOT NULL default '0',
  `level` int(11) NOT NULL default '0',
  `vocation` int(11) NOT NULL default '0',
  `cid` int(11) NOT NULL default '0',
  `health` int(11) NOT NULL default '0',
  `healthmax` int(11) NOT NULL default '0',
  `soul` int(11) NOT NULL default '0',
  `soulmax` int(11) NOT NULL default '100',
  `direction` int(11) NOT NULL default '0',
  `experience` int(11) NOT NULL default '0',
  `lookbody` int(11) NOT NULL default '0',
  `lookfeet` int(11) NOT NULL default '0',
  `lookhead` int(11) NOT NULL default '0',
  `looklegs` int(11) NOT NULL default '0',
  `looktype` int(11) NOT NULL default '0',
  `lookaddons` int(11) NOT NULL default '0',
  `knowaddons` int(11) NOT NULL default '0',
  `maglevel` int(11) NOT NULL default '0',
  `mana` int(11) NOT NULL default '0',
  `manamax` int(11) NOT NULL default '0',
  `manaspent` int(11) NOT NULL default '0',
  `masterpos` varchar(16) NOT NULL default '',
  `pos` varchar(16) NOT NULL default '',
  `speed` int(11) NOT NULL default '0',
  `cap` int(11) NOT NULL default '0',
  `maxdepotitems` int(11) NOT NULL default '1000',
  `food` int(11) NOT NULL default '0',
  `sex` int(11) NOT NULL default '0',
  `guildid` int(11) unsigned NOT NULL default '0',
  `guildrank` varchar(32) NOT NULL default '',
  `guildnick` varchar(32) NOT NULL default '',
  `ownguild` int(11) NOT NULL default '0',
  `lastlogin` int(11) unsigned NOT NULL default '0',
  `lastip` int(11) unsigned NOT NULL default '0',
  `save` int(11) unsigned NOT NULL default '1',
  `redskulltime` int(11) unsigned NOT NULL default '0',
  `redskull` tinyint(1) unsigned NOT NULL default '0',
  `comment` varchar(255) NOT NULL,
  `hide` tinyint(1) NOT NULL,
  UNIQUE KEY `id` (`id`),
  KEY `name` (`name`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=4 ;

INSERT INTO `players` VALUES (1, 'Account Manager', 0, 111111, 18, 0, 675, 840, 840, 0, 100, 0, 76000, 20, 30, 40, 50, 128, 0, 0, 20, 640, 640, 21700, '1000;1000;7', '1000;1000;7', 900, 0, 1000, 129, 1, 0, '', '', 0, 1168269485, 16777343, 1, 0, 0, '', 0);
INSERT INTO `players` VALUES (2, 'GM Xidaozu', 5, 222222, 18, 0, 675, 840, 840, 0, 100, 0, 76000, 20, 30, 40, 50, 128, 0, 0, 20, 640, 640, 21700, '1000;1000;7', '1000;1006;7', 900, 0, 1000, 129, 1, 0, '', '', 0, 1168269485, 16777343, 1, 0, 0, '', 0);
INSERT INTO `players` VALUES (3, 'Xidaozu', 0, 222222, 18, 1, 675, 840, 840, 0, 100, 2, 76000, 20, 30, 40, 50, 128, 0, 0, 20, 640, 640, 21700, '1000;1000;7', '1000;1000;7', 900, 300, 1000, 129, 0, 0, '', '', 0, 0, 0, 1, 0, 0, '', 0);

CREATE TABLE `playerstorage` (
  `player` int(11) NOT NULL default '0',
  `key` int(11) unsigned NOT NULL default '0',
  `value` int(11) NOT NULL default '0',
  KEY `player` (`player`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

CREATE TABLE `reports` (
  `id` int(11) unsigned NOT NULL auto_increment,
  `date` varchar(25) collate latin1_general_ci NOT NULL,
  `report` varchar(150) collate latin1_general_ci NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COLLATE=latin1_general_ci AUTO_INCREMENT=1 ;

CREATE TABLE `skills` (
  `player` int(11) NOT NULL default '0',
  `id` tinyint(4) NOT NULL default '0',
  `skill` int(11) unsigned NOT NULL default '0',
  `tries` int(11) unsigned NOT NULL default '0',
  KEY `player` (`player`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

INSERT INTO `skills` VALUES (1, 6, 10, 0);
INSERT INTO `skills` VALUES (1, 5, 10, 0);
INSERT INTO `skills` VALUES (1, 4, 10, 0);
INSERT INTO `skills` VALUES (1, 3, 10, 0);
INSERT INTO `skills` VALUES (1, 2, 10, 0);
INSERT INTO `skills` VALUES (1, 1, 10, 0);
INSERT INTO `skills` VALUES (1, 0, 10, 0);
INSERT INTO `skills` VALUES (2, 6, 10, 0);
INSERT INTO `skills` VALUES (2, 5, 10, 0);
INSERT INTO `skills` VALUES (2, 4, 10, 0);
INSERT INTO `skills` VALUES (2, 3, 10, 0);
INSERT INTO `skills` VALUES (2, 2, 10, 0);
INSERT INTO `skills` VALUES (2, 1, 10, 0);
INSERT INTO `skills` VALUES (2, 0, 10, 0);
INSERT INTO `skills` VALUES (3, 6, 10, 0);
INSERT INTO `skills` VALUES (3, 5, 10, 0);
INSERT INTO `skills` VALUES (3, 4, 10, 0);
INSERT INTO `skills` VALUES (3, 3, 10, 0);
INSERT INTO `skills` VALUES (3, 2, 10, 0);
INSERT INTO `skills` VALUES (3, 1, 10, 0);
INSERT INTO `skills` VALUES (3, 0, 10, 0);

CREATE TABLE `spells` (
  `player` int(11) NOT NULL,
  `spell` varchar(50) collate latin1_general_ci NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COLLATE=latin1_general_ci;

CREATE TABLE `tileitems` (
  `tileid` int(11) NOT NULL default '0',
  `sid` int(11) NOT NULL default '0',
  `pid` int(11) NOT NULL default '0',
  `type` int(11) NOT NULL default '0',
  `number` tinyint(4) NOT NULL,
  `attributes` blob,
  KEY `tileid` (`tileid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

INSERT INTO `tileitems` VALUES (20, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (47, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (66, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (78, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (109, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (128, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (145, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (154, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (174, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (182, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (207, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (243, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (280, 1, 0, 1220, 1, '');
INSERT INTO `tileitems` VALUES (302, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (338, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (357, 1, 0, 2006, 7, '');
INSERT INTO `tileitems` VALUES (360, 1, 0, 2304, 100, 0x0c64);
INSERT INTO `tileitems` VALUES (374, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (387, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (421, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (437, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (521, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (541, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (616, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (651, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (710, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (767, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (784, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (797, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (819, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (896, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (944, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (959, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (989, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1029, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1048, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (1079, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (1092, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1093, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1112, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (1155, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1156, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1217, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1218, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1267, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (1280, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1281, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1300, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (1343, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1344, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1405, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1406, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1446, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1477, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1490, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (1507, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (1524, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (1544, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1578, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (1630, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (1640, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (1656, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1672, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (1694, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (1695, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1736, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (1764, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1793, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1822, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1847, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1881, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (1889, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1890, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (1909, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (1944, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (2000, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (2076, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (2121, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (2146, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (2171, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (2196, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (2221, 1, 0, 1220, 1, '');
INSERT INTO `tileitems` VALUES (2246, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (2265, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (2301, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (2320, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (2345, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (2362, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (2390, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (2399, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (2433, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (2437, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (2481, 1, 0, 1219, 1, '');
INSERT INTO `tileitems` VALUES (2498, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (2524, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (2528, 1, 0, 1221, 1, '');
INSERT INTO `tileitems` VALUES (2548, 1, 0, 1219, 1, '');

CREATE TABLE `tilelist` (
  `tileid` int(11) unsigned NOT NULL,
  `x` int(11) unsigned NOT NULL,
  `y` int(11) unsigned NOT NULL,
  `z` int(11) unsigned NOT NULL,
  PRIMARY KEY  (`tileid`),
  UNIQUE KEY `x` (`x`,`y`,`z`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

INSERT INTO `tilelist` VALUES (20, 1010, 983, 6);
INSERT INTO `tilelist` VALUES (47, 1009, 983, 7);
INSERT INTO `tilelist` VALUES (66, 1011, 989, 7);
INSERT INTO `tilelist` VALUES (78, 1000, 988, 6);
INSERT INTO `tilelist` VALUES (109, 1000, 988, 7);
INSERT INTO `tilelist` VALUES (128, 1003, 989, 7);
INSERT INTO `tilelist` VALUES (145, 992, 986, 6);
INSERT INTO `tilelist` VALUES (154, 994, 983, 6);
INSERT INTO `tilelist` VALUES (174, 992, 989, 7);
INSERT INTO `tilelist` VALUES (182, 994, 983, 7);
INSERT INTO `tilelist` VALUES (207, 984, 991, 6);
INSERT INTO `tilelist` VALUES (243, 983, 991, 7);
INSERT INTO `tilelist` VALUES (280, 989, 994, 7);
INSERT INTO `tilelist` VALUES (302, 983, 1005, 6);
INSERT INTO `tilelist` VALUES (338, 982, 1004, 7);
INSERT INTO `tilelist` VALUES (374, 989, 1003, 7);
INSERT INTO `tilelist` VALUES (387, 997, 1010, 6);
INSERT INTO `tilelist` VALUES (421, 996, 1010, 7);
INSERT INTO `tilelist` VALUES (437, 1000, 1008, 7);
INSERT INTO `tilelist` VALUES (439, 1000, 1010, 7);
INSERT INTO `tilelist` VALUES (521, 1008, 1008, 7);
INSERT INTO `tilelist` VALUES (541, 1011, 1013, 7);
INSERT INTO `tilelist` VALUES (616, 1011, 1003, 7);
INSERT INTO `tilelist` VALUES (651, 1016, 1005, 7);
INSERT INTO `tilelist` VALUES (710, 1011, 997, 7);
INSERT INTO `tilelist` VALUES (767, 984, 1017, 7);
INSERT INTO `tilelist` VALUES (784, 984, 1021, 7);
INSERT INTO `tilelist` VALUES (797, 988, 1023, 7);
INSERT INTO `tilelist` VALUES (819, 997, 1021, 7);
INSERT INTO `tilelist` VALUES (896, 962, 978, 6);
INSERT INTO `tilelist` VALUES (944, 959, 980, 7);
INSERT INTO `tilelist` VALUES (959, 962, 978, 7);
INSERT INTO `tilelist` VALUES (989, 974, 977, 6);
INSERT INTO `tilelist` VALUES (1029, 974, 976, 7);
INSERT INTO `tilelist` VALUES (1048, 977, 978, 7);
INSERT INTO `tilelist` VALUES (1079, 993, 971, 7);
INSERT INTO `tilelist` VALUES (1092, 996, 968, 7);
INSERT INTO `tilelist` VALUES (1093, 1000, 968, 7);
INSERT INTO `tilelist` VALUES (1112, 1003, 971, 7);
INSERT INTO `tilelist` VALUES (1155, 996, 968, 6);
INSERT INTO `tilelist` VALUES (1156, 1000, 968, 6);
INSERT INTO `tilelist` VALUES (1217, 996, 968, 5);
INSERT INTO `tilelist` VALUES (1218, 1000, 968, 5);
INSERT INTO `tilelist` VALUES (1267, 1010, 971, 7);
INSERT INTO `tilelist` VALUES (1280, 1013, 968, 7);
INSERT INTO `tilelist` VALUES (1281, 1017, 968, 7);
INSERT INTO `tilelist` VALUES (1300, 1020, 971, 7);
INSERT INTO `tilelist` VALUES (1343, 1013, 968, 6);
INSERT INTO `tilelist` VALUES (1344, 1017, 968, 6);
INSERT INTO `tilelist` VALUES (1405, 1013, 968, 5);
INSERT INTO `tilelist` VALUES (1406, 1017, 968, 5);
INSERT INTO `tilelist` VALUES (1446, 1003, 957, 7);
INSERT INTO `tilelist` VALUES (1477, 1009, 956, 7);
INSERT INTO `tilelist` VALUES (1490, 1012, 949, 7);
INSERT INTO `tilelist` VALUES (1507, 1017, 949, 7);
INSERT INTO `tilelist` VALUES (1524, 1022, 949, 7);
INSERT INTO `tilelist` VALUES (1544, 1060, 992, 5);
INSERT INTO `tilelist` VALUES (1578, 1058, 991, 6);
INSERT INTO `tilelist` VALUES (1630, 1063, 993, 6);
INSERT INTO `tilelist` VALUES (1640, 1064, 997, 6);
INSERT INTO `tilelist` VALUES (1656, 1056, 995, 7);
INSERT INTO `tilelist` VALUES (1672, 1058, 991, 7);
INSERT INTO `tilelist` VALUES (1694, 1059, 1001, 7);
INSERT INTO `tilelist` VALUES (1695, 1060, 992, 7);
INSERT INTO `tilelist` VALUES (1736, 1064, 997, 7);
INSERT INTO `tilelist` VALUES (1764, 1027, 956, 7);
INSERT INTO `tilelist` VALUES (1793, 1027, 961, 7);
INSERT INTO `tilelist` VALUES (1822, 1027, 965, 7);
INSERT INTO `tilelist` VALUES (1847, 1027, 969, 7);
INSERT INTO `tilelist` VALUES (1881, 1044, 978, 7);
INSERT INTO `tilelist` VALUES (1889, 1046, 976, 7);
INSERT INTO `tilelist` VALUES (1890, 1050, 976, 7);
INSERT INTO `tilelist` VALUES (1909, 1053, 978, 7);
INSERT INTO `tilelist` VALUES (1944, 1046, 976, 6);
INSERT INTO `tilelist` VALUES (2000, 1026, 1016, 7);
INSERT INTO `tilelist` VALUES (2076, 1035, 1016, 7);
INSERT INTO `tilelist` VALUES (2121, 962, 1001, 7);
INSERT INTO `tilelist` VALUES (2146, 962, 994, 7);
INSERT INTO `tilelist` VALUES (2171, 962, 987, 7);
INSERT INTO `tilelist` VALUES (2196, 973, 1000, 7);
INSERT INTO `tilelist` VALUES (2221, 973, 1005, 7);
INSERT INTO `tilelist` VALUES (2246, 973, 1000, 6);
INSERT INTO `tilelist` VALUES (2265, 969, 989, 6);
INSERT INTO `tilelist` VALUES (2301, 969, 989, 7);
INSERT INTO `tilelist` VALUES (2320, 973, 987, 7);
INSERT INTO `tilelist` VALUES (2345, 962, 1014, 5);
INSERT INTO `tilelist` VALUES (2362, 964, 1017, 5);
INSERT INTO `tilelist` VALUES (2390, 962, 1014, 6);
INSERT INTO `tilelist` VALUES (2399, 963, 1017, 6);
INSERT INTO `tilelist` VALUES (2433, 968, 1014, 6);
INSERT INTO `tilelist` VALUES (2437, 968, 1018, 6);
INSERT INTO `tilelist` VALUES (2481, 962, 1014, 7);
INSERT INTO `tilelist` VALUES (2498, 964, 1017, 7);
INSERT INTO `tilelist` VALUES (2524, 968, 1014, 7);
INSERT INTO `tilelist` VALUES (2528, 968, 1018, 7);
INSERT INTO `tilelist` VALUES (2548, 971, 1017, 7);

CREATE TABLE `viplist` (
  `account` int(11) unsigned NOT NULL default '0',
  `player` varchar(32) NOT NULL default '0',
  KEY `player` USING BTREE (`account`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
