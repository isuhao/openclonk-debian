__Quick start__
To directly use the masterserver you will need a webserver with PHP >= 5 and access to a MySQL-database. Start by opening the folder web and navigating through server and include, open the config.ini. Enter you're preferences there, everything is documented.
IMPORTANT: Before you continue make sure this folder will be inaccessible through the web later by making sure a working .htaccess is present on an Apache server or there is a corresponding chmod on the whole include folder after uploading later on.
Open a connection to your MySQL-server directly or via a tool like phpMyAdmin and query the command listet below to create the table structure. You can change the default prefix without any problems, just don't forget to change it in your config file.
Now upload the contents of the web/ folder. Now you should be able to open the root and see the server frontend. Make sure again, that the /server/include folder can NOT be accessed via web, since it contains your MySQL-data.
The installation should now be complete and ready to use. You can see the server link on the frontend, just put it in your Clonk network settings and you're done!

__Database__

CREATE TABLE `c4ms_flood` (
  `ip` char(32) CHARACTER SET latin1 NOT NULL,
  `count` int(11) NOT NULL,
  `time` char(20) CHARACTER SET latin1 NOT NULL,
  PRIMARY KEY (`ip`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

CREATE TABLE `c4ms_games` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `ip` varchar(255) CHARACTER SET latin1 NOT NULL,
  `csid` varchar(255) CHARACTER SET latin1 NOT NULL,
  `data` text CHARACTER SET latin1 NOT NULL,
  `start` varchar(255) CHARACTER SET latin1 NOT NULL,
  `time` varchar(255) CHARACTER SET latin1 NOT NULL,
  `valid` tinyint(4) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

__Updates__

If you are running an update service for OpenClonk, you also need to insert the following table and specifiy the config entries. (see there for more details)

CREATE TABLE `c4ms_update` (
  `old_version` varchar(16) CHARACTER SET latin1 NOT NULL,
  `new_version` varchar(16) CHARACTER SET latin1 NOT NULL,
  `platform` varchar(64) CHARACTER SET latin1 NOT NULL,
  `file` varchar(255) CHARACTER SET latin1 NOT NULL,
  PRIMARY KEY (`old_version`,`platform`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

__License__
This work is licensed under the Creative Commons Attribution 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by/3.0/ or send a letter to Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.

__Append__
Please also note that 'Clonk' is a registered trademark of Matthes Bender (http://www.clonk.de).