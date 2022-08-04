<?php
/*
 * Bacula(R) - The Network Backup Solution
 * Baculum   - Bacula web interface
 *
 * Copyright (C) 2013-2019 Kern Sibbald
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

namespace Baculum\Common\Modules;

use Prado\Prado;
use Prado\Util\TLogger;

/**
 * Main logger class.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category Module
 * @package Baculum Common
 */
class Logging extends CommonModule {

	/*
	 * Stores debug enable state.
	 *
	 * @var bool
	 */
	public static $debug_enabled = false;

	/**
	 * Log categories.
	 */
	const CATEGORY_EXECUTE = 'Execute';
	const CATEGORY_EXTERNAL = 'External';
	const CATEGORY_APPLICATION = 'Application';
	const CATEGORY_GENERAL = 'General';
	const CATEGORY_SECURITY = 'Security';
	const CATEGORY_AUDIT = 'Audit';

	/**
	 * Main log method used to log message.
	 *
	 * @param string $category log category
	 */
	public function log($category, $message) {
		if (!$this->isEnabled($category)) {
			return;
		}
		$this->prepareMessage($category, $message);
		Prado::log($message, TLogger::INFO, $category);
	}

	/**
	 * Check if log is enabled.
	 *
	 * @param string $category log category
	 * @return bool true if log is enabled, otherwise false
	 */
	private function isEnabled($category) {
		$is_enabled = false;
		if (self::$debug_enabled === true || $category === self::CATEGORY_AUDIT) {
			// NOTE: Audit log is written always, it is not possible to disable it
			$is_enabled = true;
		}
		return $is_enabled;
	}

	/**
	 * Prepare log to send to log manager.
	 *
	 * @param string $category log category
	 * @param array|string|object &$message log message reference
	 */
	private function prepareMessage($category, &$message) {
		if (is_object($message) || is_array($message)) {
			// make a message as string if needed
			$message = print_r($message, true);
		}
		if (self::$debug_enabled === true && $category !== self::CATEGORY_AUDIT) {
			// If debug enabled, add file and line to log message
			$f = '';
			$trace = debug_backtrace();
			if (isset($trace[1]['file']) && isset($trace[1]['line'])) {
				$f = sprintf(
					'%s:%s:',
					basename($trace[1]['file']),
					$trace[1]['line']
				);
			}
			$message = $f . $message;
		}
		$message .= PHP_EOL . PHP_EOL;
	}

	/**
	 * Helper method for preparing logs that come from executing programs or scripts.
	 * This log consists of command and output.
	 * Useful for bconsole, b*json... and others.
	 *
	 * @param string $cmd command
	 * @param array|object|string $output command output
	 * @return string formatted command log
	 */
	public static function prepareOutput($cmd, $output) {
		if (is_array($output)) {
			$output = implode(PHP_EOL, $output);
		} elseif(is_object($output)) {
			$output = print_r($output, true);
		}
		return sprintf(
			"\n\n===> Command:\n\n%s\n\n===> Output:\n\n%s",
			$cmd,
			$output
		);
	}
}
