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
 * Get console output for query type commands.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category API
 * @package Baculum API
 */
abstract class ConsoleOutputQueryPage extends ConsoleOutputPage {

	/**
	 * Parse '.query' type command output in key=value form.
	 *
	 * @param array $output dot query command output
	 * @return array parsed output
	 */
	protected function parseOutputKeyValue(array $output) {
		$ret = [];
		for ($i = 0; $i < count($output); $i++) {
			if (preg_match('/(?P<key>\w+)=(?P<value>.*?)$/i', $output[$i], $matches) === 1) {
				$ret[$matches['key']] = $matches['value'];
			}
		}
		return $ret;
	}
}
?>
