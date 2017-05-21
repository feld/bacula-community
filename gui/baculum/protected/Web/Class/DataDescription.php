<?php
/*
 * Bacula(R) - The Network Backup Solution
 * Baculum   - Bacula web interface
 *
 * Copyright (C) 2013-2016 Kern Sibbald
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

Prado::using('Application.Web.Class.WebModule');

class DataDescription extends WebModule {

	/**
	 * Data description file
	 */
	const DATA_DESC_FILE = 'Application.Web.Data.data_desc';

	private $data_desc;

	public function init($config) {
		parent::init($config);
		$this->data_desc = $this->loadDataDescription();
	}

	private function loadDataDescription() {
		$data_desc = null;
		$desc_file = Prado::getPathOfNamespace(self::DATA_DESC_FILE, '.json');
		if (file_exists($desc_file) && is_readable($desc_file)) {
			$desc_file = file_get_contents($desc_file);
			$data_desc = json_decode($desc_file);
		}
		return $data_desc;
	}

	public function getDescription($component_type, $resource_type, $directive_name = null) {
		$desc = null;
		if (!is_null($directive_name) && isset($this->data_desc->{$component_type}->{$resource_type}->{$directive_name})) {
			$desc = $this->data_desc->{$component_type}->{$resource_type}->{$directive_name};
		} elseif (isset($this->data_desc->{$component_type}->{$resource_type})) {
			$desc = (array)$this->data_desc->{$component_type}->{$resource_type};
		}
		return $desc;
	}
}
?>
