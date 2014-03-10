<?php

/**
 * C4Masterserver engine backend
 *
 * @package C4Masterserver
 * @version 1.2.0-en
 * @author  Benedict Etzel <b.etzel@live.de>
 * @license http://creativecommons.org/licenses/by/3.0/ CC-BY 3.0
 */
//error_reporting(E_ALL); //suppress errors

require_once('include/C4Masterserver.php');
require_once('include/C4Network.php');
require_once('include/FloodProtection.php');
require_once('include/ParseINI.php');
$C4HostTestIncludeMode = true;
require_once('include/C4HostTest.php');

$config = file_get_contents('include/config.ini');
$link = mysql_connect(
	ParseINI::parseValue('mysql_host', $config),
	ParseINI::parseValue('mysql_user', $config),
	ParseINI::parseValue('mysql_password', $config)); //connect to MySQL
$db = mysql_selectdb(ParseINI::parseValue('mysql_db', $config), $link); //select the database

header('Content-Type: text/plain'); //output as plain text

if ($link && $db) {
	$prefix = ParseINI::parseValue('mysql_prefix', $config);
	$server = new C4Masterserver($link, $config);
	$server->setTimeoutgames(intval(ParseINI::parseValue('c4ms_timeoutgames', $config)));
	$server->setDeletegames(intval(ParseINI::parseValue('c4ms_deletegames', $config)));
	$server->setMaxgames(intval(ParseINI::parseValue('c4ms_maxgames', $config)));
	$protect = new FloodProtection($link, $prefix);
	$protect->setMaxflood(intval(ParseINI::parseValue('flood_maxrequests', $config)));
	if ($protect->checkRequest($_SERVER['REMOTE_ADDR'])) { //flood protection
		C4Network::sendAnswer(C4Network::createError('Flood protection.'));
		die();
	}
	$server->cleanUp(true); //Cleanup old stuff
	
	// register new release
	if (ParseINI::parseValue('oc_enable_update', $config) == 1 && isset($_REQUEST['action']) && $_REQUEST['action'] == 'release-file' && isset($_REQUEST['file']) && isset($_REQUEST['hash']) && isset($_REQUEST['new_version']) && isset($_REQUEST['platform'])) {
		$absolutefile = ParseINI::parseValue('oc_update_path', $config) . $_REQUEST['file'];
		if (file_exists($absolutefile)) {
			if(hash_hmac_file('sha256', $absolutefile, ParseINI::parseValue('oc_update_secret', $config)) == $_REQUEST['hash']) {
				$old_version = isset($_REQUEST['old_version']) && !empty($_REQUEST['old_version']) ? explode(',', mysql_real_escape_string($_REQUEST['old_version'], $link)) : array();
				$new_version = mysql_real_escape_string($_REQUEST['new_version'], $link);
				$platform = mysql_real_escape_string($_REQUEST['platform'], $link);
				$file = mysql_real_escape_string($_REQUEST['file'], $link);
				if (!empty($old_version)) {
					if (isset($_REQUEST['delete_old_files']) && $_REQUEST['delete_old_files'] == 'yes') {
						$result = mysql_query('SELECT `file` FROM `' . $prefix . 'update` WHERE `new_version` != \'' . $new_version . '\' AND `old_version` != \'\' AND `platform` = \'' . $platform . '\'');
						while (($row = mysql_fetch_assoc($result)) != false) {
							unlink(ParseINI::parseValue('oc_update_path', $config) . $row['file']);
						}
					}
					mysql_query('DELETE FROM `' . $prefix . 'update` WHERE `new_version` != \'' . $new_version . '\' AND `old_version` != \'\' AND `platform` = \'' . $platform . '\'');
					foreach ($old_version as $version) {
						mysql_query('INSERT INTO `' . $prefix . 'update` (`old_version`, `new_version`, `platform`, `file`) VALUES (\'' . $version . '\', \'' . $new_version . '\', \'' . $platform . '\', \'' . $file . '\')');
					}
				} else {
					if (isset($_REQUEST['delete_old_files']) && $_REQUEST['delete_old_files'] == 'yes') {
						$row = mysql_fetch_assoc(mysql_query('SELECT `file` FROM `' . $prefix . 'update` WHERE `old_version` = \'\' AND `platform` = \'' . $platform . '\''));
						unlink(ParseINI::parseValue('oc_update_path', $config) . $row['file']);
					}
					mysql_query('DELETE FROM `' . $prefix . 'update` WHERE `old_version` = \'\' AND `platform` = \'' . $platform . '\'');
					mysql_query('INSERT INTO `' . $prefix . 'update` (`old_version`, `new_version`, `platform`, `file`) VALUES (\'\', \'' . $new_version . '\', \'' . $platform . '\', \'' . $file . '\')');
				}
			} else {
				C4Network::sendAnswer(C4Network::createError('Hash incorrect.'));
			}
		} else {
			C4Network::sendAnswer(C4Network::createError('Specified file not found.'));
		}
	// prepare data for the engine
	} else if (isset($GLOBALS['HTTP_RAW_POST_DATA'])) {
		$input = $GLOBALS['HTTP_RAW_POST_DATA'];
		$action = ParseINI::parseValue('Action', $input);
		$csid = ParseINI::parseValue('CSID', $input);
		$csid = mysql_real_escape_string($csid, $link);
		$reference = mysql_real_escape_string(strstr($input, '[Reference]'), $link);
		$engine_string = ParseINI::parseValue('c4ms_engine', $config);
		if (empty($engine_string) || ParseINI::parseValue('Game', $input) == $engine_string) {
			switch ($action) {
			case 'Start': //start a new round
				if (ParseINI::parseValue('LeagueAddress', $reference)) {
					C4Network::sendAnswer(C4Network::createError('IDS_MSG_LEAGUENOTSUPPORTED'));
				} else {
					$csid = $server->addReference($reference);
					if ($csid) {
						$answer = array('Status' => 'Success', 'CSID' => $csid);
						if(!testHostConn($input))
							$answer['Message'] = 'IDS_MSG_MASTERSERVNATERROR';
						C4Network::sendAnswer(C4Network::createAnswer($answer));
						unset($answer);
					} else {
						C4Network::sendAnswer(C4Network::createError('IDS_MSG_MATERSERVSIGNUPFAIL'));
					}
				}
				break;
			case 'Update': //update an existing round
				if ($server->updateReference($csid, $reference)) {
					C4Network::sendAnswer(C4Network::createAnswer(array('Status' => 'Success')));
				} else {
					C4Network::sendAnswer(C4Network::createError('IDS_MSG_MASTERSERVUPDATEFAIL'));
				}
				break;
			case 'End': //remove a round
				if ($server->removeReference($csid)) {
					C4Network::sendAnswer(C4Network::createAnswer(array('Status' => 'Success')));
				} else {
					C4Network::sendAnswer(C4Network::createError('IDS_MSG_MASTERSERVENDFAIL'));
				}
				break;
			default:
				if (!empty($action)) {
					C4Network::sendAnswer(C4Network::createError('IDS_MSG_MASTERSERVNOOP'));
				} else {
					C4Network::sendAnswer(C4Network::createError('IDS_MSG_MASTERSERVNOOP'));
				}
				break;
			}
		} else {
			C4Network::sendAnswer(C4Network::createError('IDS_MSG_MASTERSERVWRONGENGINE'));
		}
	} else { //list availabe games
		$list = array();
		if (!isset($_GET['action']) || (isset($_GET['action']) && $_GET['action'] != 'version')) {
			$list = $server->getReferenceArray(false);
		}
		$message = '';
		$engine = ParseINI::parseValue('c4ms_title_engine', $config);
		$platform = isset($_REQUEST['platform']) ? mysql_real_escape_string($_REQUEST['platform'], $link) : 0;
		$client_version = isset($_REQUEST['version']) ? mysql_real_escape_string($_REQUEST['version'], $link) : 0;
		if (!empty($engine)) {
			$message .= '[' . $engine . ']' . PHP_EOL;
			if (ParseINI::parseValue('oc_enable_update', $config) == 1) {
				if ($platform && $client_version) {
					$result = mysql_query('SELECT `new_version` FROM `' . ParseINI::parseValue('mysql_prefix', $config) . 'update` WHERE `old_version` = \'\' AND `platform` = \'' . $platform . '\'');
					$row = mysql_fetch_assoc($result);
					$version = $row['new_version'];
					if ($version) {
						$message .= 'Version=' . $version . PHP_EOL;
						// strip build version from client request
						$n = 0;
						for($i=0;$i<strlen($client_version);$i++){
							if($client_version[$i]=='.'){
								$n++;
								if($n>=3){
									break;
								}
							}
						}
						$client_version = substr($client_version,0,$i);
						if (version_compare($client_version, $version) < 0) { //We need to update
							$result = mysql_query('SELECT `file` FROM `' . $prefix . 'update` WHERE `old_version` = \'' . $client_version . '\' AND `platform` = \'' . $platform . '\'');
							$row = mysql_fetch_assoc($result);
							if ($row['file'])
								$message .= 'UpdateURL=' . ParseINI::parseValue('oc_update_url', $config) . $row['file'] . PHP_EOL;
						}
					}
				}
			}
			$motd = ParseINI::parseValue('c4ms_motd', $config);
			if (!empty($motd))
				$message .= 'MOTD=' . $motd . PHP_EOL;
		}
		foreach ($list as $reference) {
			if (!empty($message))
				$message .= PHP_EOL;
			$message .= $reference['data'];
			$message .= 'GameId=' . $reference['id'] . PHP_EOL;
			$message .= 'OfficialServer=false' . PHP_EOL;
		}
		C4Network::sendAnswer($message);
	}
	mysql_close($link);
}
else {
	C4Network::sendAnswer(C4Network::createError('Database error.'));
}
?>
