<?php
/*
 * Bacula(R) - The Network Backup Solution
 * Baculum   - Bacula web interface
 *
 * Copyright (C) 2013-2022 Kern Sibbald
 *
 * The main author of Baculum is Marcin Haba.
 * The original author of Bacula is Kern Sibbald, with contributions
 * from many others, a complete list can be found in the file AUTHORS.
 *
 * You may use this file and others of this release according to the
 * license defined in the LICENSE file, which includes the Affero General
 * Public License, v3.0 ("AGPLv3") and some additional permissions and
 * terms pursuant to its AGPLv3 Section 7.
 *
 * This notice must be preserved when any source code is
 * conveyed and/or propagated.
 *
 * Bacula(R) is a registered trademark of Kern Sibbald.
 */

namespace Baculum\API\Modules;

/**
 * Get console output for JSON type commands (like .jlist).
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category API
 * @package Baculum API
 */
abstract class ConsoleOutputJSONPage extends ConsoleOutputPage {

	/**
	 * Parse '.jlist' type command output.
	 *
	 * @param array $output dot query command output
	 * @return object parsed output or empty object if there occurs a problem with parsing output
	 */
	protected function parseOutput(array $output) {
		$out = new \StdClass;
		for ($i = 0; $i < count($output); $i++) {
			if (preg_match('/^[{\[]/', $output[$i]) === 1) {
				$out = json_decode($output[$i]);
				break;
			}
		}
		return $out;
	}
}
?>
