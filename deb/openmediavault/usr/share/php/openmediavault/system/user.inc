<?php
/**
 * This file is part of OpenMediaVault.
 *
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2016 Volker Theile
 *
 * OpenMediaVault is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * OpenMediaVault is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenMediaVault. If not, see <http://www.gnu.org/licenses/>.
 */
namespace OMV\System;

/**
 * This class provides a simple interface to handle a system user.
 * @ingroup api
 */
class User {
	protected $name = null;
	protected $uid = null;
	protected $gid = null;
	protected $password = null;
	protected $gecos = null;
	protected $dir = null;
	protected $shell = null;
	protected $lastchanged = null;
	protected $minimum = null;
	protected $maximum = null;
	protected $warn = null;
	protected $inactive = null;
	protected $expire = null;
	protected $reserved = null;
	protected $groups = null;

	/**
	 * Constructor
	 * @param id The name or UID of the system user.
	 */
	public function __construct($id) {
		if(is_int($id))
			$this->uid = $id;
		else
			$this->name = $id;
	}

	/**
	 * Get the system group data.
	 * @private
	 * @return None.
	 * @throw \OMV\Exception
	 * @throw \OMV\ExecException
	 */
	private function getData($type) {
		switch ($type) {
		case "shadow":
			if (!is_null($this->password))
				return;
			// http://www.cyberciti.biz/faq/understanding-etcshadow-file
			$cmdArgs = array();
			$cmdArgs[] = "shadow";
			$cmdArgs[] = escapeshellarg($this->getName());
			$cmd = new \OMV\System\Process("getent", $cmdArgs);
			$cmd->setRedirect2to1(TRUE);
			$cmd->execute($output);
			$output = explode(":", $output[0]);
			$this->password = $output[1];
			$this->lastchanged = $output[2];
			$this->minimum = $output[3];
			$this->maximum = $output[4];
			$this->warn = $output[5];
			$this->inactive = $output[6];
			$this->expire = $output[7];
			$this->reserved = $output[8];
			break;
		case "passwd":
			if (!is_null($this->name) && !is_null($this->uid))
				return;
			// Query user information.
			$userInfo = FALSE;
			if (!is_null($this->uid))
				$userInfo = posix_getpwuid($this->uid);
			else
				$userInfo = posix_getpwnam($this->name);
			if (FALSE === $userInfo)
				throw new \OMV\Exception(posix_strerror(posix_errno()));
			$this->name = $userInfo['name'];
			$this->uid = $userInfo['uid'];
			$this->gid = $userInfo['gid'];
			$this->gecos = $userInfo['gecos'];
			$this->dir = $userInfo['dir'];
			$this->shell = $userInfo['shell'];
			break;
		case "groups":
			if (!is_null($this->groups))
				return;
			// Get the group IDs the user is in as white-space separated
			// numbers. Thus it is possible to correctly process group
			// names that contain blanks (e.g. imported via LDAP or AD).
			$cmdArgs = array();
			$cmdArgs[] = "-G";
			$cmdArgs[] = escapeshellarg($this->getName());
			$cmd = new \OMV\System\Process("id", $cmdArgs);
			$cmd->setRedirect2to1(TRUE);
			$cmd->execute($output);
			$gids = explode(" ", $output[0]);
			// Resolve the group names.
			$this->groups = array();
			foreach ($gids as $gidk => $gidv) {
				$groupInfo = posix_getgrgid($gidv);
				$this->groups[] = $groupInfo['name'];
			}
			break;
		}
	}

	/**
	 * Check whether the system user exists.
	 * @return TRUE if the system user exists, otherwise FALSE.
	 */
	public function exists() {
		try {
			$this->getData("passwd");
		} catch(\Exception $e) {
			return FALSE;
		}
		return !is_null($this->name) && !is_null($this->uid);
	}

	public function assertExists() {
		if (FALSE === $this->exists()) {
			$id = "unknown";
			if (!is_null($this->name))
				$id = $this->name;
			else if (!is_null($this->uid))
				$id = $this->uid;
			throw new \OMV\Exception("The user '%s' does not exist.",
			  strval($id));
		}
	}

	public function assertNotExists() {
		if (TRUE === $this->exists()) {
			$id = "unknown";
			if (!is_null($this->name))
				$id = $this->name;
			else if (!is_null($this->uid))
				$id = $this->uid;
			throw new \OMV\Exception("The user '%s' already exists.",
			  strval($id));
		}
	}

	/**
	 * Get the user name.
	 * @return The user name.
	 */
	public function getName() {
		if (!is_null($this->name))
			return $this->name;
		$this->getData("passwd");
		return $this->name;
	}

	/**
	 * Get the user ID.
	 * @return The user ID, otherwise FALSE.
	 */
	public function getUid() {
		if (!is_null($this->uid))
			return $this->uid;
		$this->getData("passwd");
		return $this->uid;
	}

	/**
	 * Get the users group ID.
	 * @return The user group ID, otherwise FALSE.
	 */
	public function getGid() {
		$this->getData("passwd");
		return $this->gid;
	}

	/**
	 * Get the comment.
	 * @return The comment, otherwise FALSE.
	 */
	public function getGecos() {
		$this->getData("passwd");
		return $this->gecos;
	}

	/**
	 * Get the home directory.
	 * @return The home directory, otherwise FALSE.
	 */
	public function getHomeDirectory() {
		$this->getData("passwd");
		return $this->dir;
	}

	/**
	 * Get the shell.
	 * @return The shell, otherwise FALSE.
	 */
	public function getShell() {
		$this->getData("passwd");
		return $this->shell;
	}

	/**
	 * Get the encrypted password.
	 * @return The encrypted password, otherwise FALSE.
	 */
	public function getPassword() {
		$this->getData("shadow");
		return $this->password;
	}

	public function getLastChanged() {
		$this->getData("shadow");
		return $this->lastchanged;
	}

	public function getMinimum() {
		$this->getData("shadow");
		return $this->minimum;
	}

	public function getMaximum() {
		$this->getData("shadow");
		return $this->maximum;
	}

	public function getWarn() {
		$this->getData("shadow");
		return $this->warn;
	}

	public function getInactive() {
		$this->getData("shadow");
		return $this->inactive;
	}

	public function getExpire() {
		$this->getData("shadow");
		return $this->expire;
	}

	public function getReserved() {
		$this->getData("shadow");
		return $this->reserved;
	}

	/**
	 * Get the groups the user is in.
	 * @return An array of groups the user is in, otherwise FALSE.
	 */
	public function getGroups() {
		$this->getData("groups");
		return $this->groups;
	}

	/**
	 * Get the user quotas.
	 * @return An array containing the quotas.
	 * @throw \OMV\ExecException
	 */
	public function getQuotas() {
		$cmdArgs = array();
		$cmdArgs[] = "-u";
		$cmdArgs[] = escapeshellarg($this->getName());
		$cmd = new \OMV\System\Process("edquota", $cmdArgs);
		$cmd->setEnv("EDITOR=cat");
		$cmd->setRedirect2to1(TRUE);
		$cmd->execute($output);
		// Parse command output:
		// Filesystem                   blocks       soft       hard     inodes     soft     hard
		// /dev/sdb1                     10188          0      12288          4        0        0
		// /dev/sdc1                         0          0      45056          0        0        0
		$result = array();
		foreach ($output as $outputk => $outputv) {
			if (preg_match("/^\s+(\S+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+".
			  "(\d+)\s+(\d+)$/i", $outputv, $matches)) {
				$result[$matches[1]] = array(
					"devicefile" => $matches[1],
					"blocks" => $matches[2],
					"bsoft" => $matches[3],
					"bhard" => $matches[4],
					"inodes" => $matches[5],
					"isoft" => $matches[6],
					"ihard" => $matches[7]
				);
			}
		}
		return $result;
	}

	/**
	 * Authenticate user via PAM. Account expiration and access hours are
	 * checked, too. Note, the calling process must have the privilege to
	 * read the user shadow file.
	 * @param password The password.
	 * @return TRUE if the authentication was successful, otherwise FALSE.
	 */
	public function authenticate($password) {
		if (FALSE === pam_auth($this->getName(), $password))
			return FALSE;
		return TRUE;
	}

	/**
	 * Check if the given user is a system account.
	 * @return TRUE if the user is a system account, otherwise FALSE.
	 */
	public function isSystemAccount() {
		if (FALSE === ($uid = $this->getUid()))
			return FALSE;
		// Get shadow password suite configuration.
		$ld = \OMV\System\System::getLoginDefs();
		// Get the min/max values of the non-system account ID sequence.
		$min = intval(array_value($ld, "UID_MIN", 1000));
		$max = intval(array_value($ld, "UID_MAX", 60000));
		// Check if the given account ID is within the sequence.
		return !in_range($uid, $min, $max);
	}

	/**
	 * Enumerate user names.
	 * @return An array of user names.
	 * @throw \OMV\ExecException
	 */
	public static function getUsers() {
		$cmd = new \OMV\System\Process("getent", "passwd");
		$cmd->setRedirect2to1(TRUE);
		$cmd->execute($output);
		// Parse command output:
		// proftpd:x:109:65534::/var/run/proftpd:/bin/false
		// ftp:x:110:65534::/home/ftp:/bin/false
		// openmediavault:x:999:999::/home/openmediavault:/bin/sh
		// admin:x:998:100:WebGUI administrator:/home/admin:/usr/sbin/nologin
		// nut:x:111:114::/var/lib/nut:/bin/false
		// test:x:1001:100:sdfds:/home/test:/bin/dash
		$list = array();
		foreach ($output as $outputv) {
			$data = explode(":", $outputv);
			if (empty($data))
				continue;
			$list[] = $data[0]; // User name
		}
		return $list;
	}
}