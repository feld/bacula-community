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

use Baculum\Common\Modules\Logging;
use Baculum\Common\Modules\Errors\BconsoleError;
use Baculum\API\Modules\ConsoleOutputPage;
use Baculum\API\Modules\ConsoleOutputQueryPage;

/**
 * List search results from .search Bconsole command.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category API
 * @package Baculum API
 */
class SearchItems extends ConsoleOutputQueryPage {

	const CATEGORY_ALL = 'all';
	const CATEGORY_CLIENT = 'client';
	const CATEGORY_JOB = 'job';
	const CATEGORY_VOLUME = 'volume';

	public function get() {
		$misc = $this->getModule('misc');

		$out_format = ConsoleOutputPage::OUTPUT_FORMAT_RAW;
		if ($this->Request->contains('output') && $this->isOutputFormatValid($this->Request['output'])) {
			$out_format = $this->Request['output'];
		}
		$text = $this->Request->contains('text') && $misc->isValidName($this->Request['text']) ? $this->Request['text'] : null;
		$category = self::CATEGORY_ALL; // default category
		if ($this->Request->contains('category') && $this->categoryExists($this->Request['category'])) {
			$category = $this->Request['category'];
		}

		if (is_null($text)) {
			// no text, do nothing
			$this->output = [];
			$this->error = BconsoleError::ERROR_NO_ERRORS;
			return;
		}

		$params = [
			'text' => $text,
			'category' => $category
		];

		$out = new \StdClass;
		if ($out_format === ConsoleOutputPage::OUTPUT_FORMAT_RAW) {
			$out = $this->getRawOutput($params);
		} elseif($out_format === ConsoleOutputPage::OUTPUT_FORMAT_JSON) {
			$out = $this->getJSONOutput($params);
		}

		if ($out->exitcode === 0) {
			$this->output = $out->output;
			$this->error = BconsoleError::ERROR_NO_ERRORS;
		} else {
			$this->output = BconsoleError::MSG_ERROR_WRONG_EXITCODE . $out->output;
			$this->error = BconsoleError::ERROR_WRONG_EXITCODE;
			$this->getModule('logging')->log(
				Logging::CATEGORY_EXECUTE,
				$this->output . ", Error={$this->error}"
			);
		}
	}

	/**
	 * Get search command output from console in raw format.
	 *
	 * @param array $params command parameters
	 * @return StdClass object with output and exitcode
	 */
	protected function getRawOutput($params = []) {
		$ret = $this->getModule('bconsole')->bconsoleCommand(
			$this->director,
			[
				'.search',
				'category="' . $params['category'] . '"',
				'text="' . $params['text'] . '"'
			]
		);
		if ($ret->exitcode !== 0) {
			$ret->output = []; // don't provide errors to output, only in logs
			$this->getModule('logging')->log(
				Logging::CATEGORY_EXECUTE,
				'Wrong output from RAW search items: ' . implode(PHP_EOL, $ret->output)
			);
		}
		return $ret;
	}

	/**
	 * Get search items results in JSON format.
	 *
	 * @param array $params command parameters
	 * @return StdClass object with output and exitcode
	 */
	protected function getJSONOutput($params = []) {
		$ret = $this->getRawOutput($params);
		if ($ret->exitcode === 0) {
			$result = $this->parseOutputKeyValue($ret->output);
			if (key_exists('error', $result) && $result['error'] === '0') {
				$ret->output = (object)[
					'client' => !empty($result['client']) ? explode(',', $result['client']) : [],
					'volume' => !empty($result['volume']) ? explode(',', $result['volume']) : [],
					'job' => !empty($result['job']) ? explode(',', $result['job']) : []
				];
			} else {
				$ret->output = []; // don't provide errors to output, only in logs
				$this->getModule('logging')->log(
					Logging::CATEGORY_EXECUTE,
					'Wrong output from .search command output: ' . implode(PHP_EOL, $result)
				);
			}
		}
		return $ret;
	}

	/**
	 * Check if category exists.
	 *
	 * @param string $category category
	 * @return bool true if exists, otherwise false
	 */
	private function categoryExists($category) {
		$cats = $this->getCategories();
		return in_array($category, $cats);
	}

	/**
	 * Get all supported categories.
	 * return array category list
	 */
	private function getCategories() {
		return [
			self::CATEGORY_ALL,
			self::CATEGORY_VOLUME,
			self::CATEGORY_CLIENT,
			self::CATEGORY_JOB
		];
	}
}
