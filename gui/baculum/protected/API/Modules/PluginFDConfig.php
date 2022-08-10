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

use Prado\Prado;

/**
 * File Daemon (Client) plugin config module.
 * Module is responsible for read/write the config data.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category Config
 * @package Baculum API
 */
class PluginFDConfig extends PluginConfig {

	/**
	 * Plugin config file format.
	 */
	const CONFIG_FILE_FORMAT = 'ini';

	/**
	 * Plugin config file extension.
	 */
	const CONFIG_FILE_EXT = '.conf';

	private $config;

	/**
	 * Get plugin client configuration.
	 *
	 * @param string $plugin plugin name (ex. 'm365' or 'mysql')
	 * @param string $client client name
	 * @param string $section if given, returned is this section from configuration
	 * @return array plugin client config
	 */
	public function getConfig($plugin, $client, $section = null) {
		$config = [];
		if (is_null($this->config)) {
			$fn = sprintf('%s_%s', $client, $plugin);
			$cfg_dir_path = $this->getConfigDirPath();
			$path = Prado::getPathOfNamespace($cfg_dir_path) . DIRECTORY_SEPARATOR . $fn . self::CONFIG_FILE_EXT;
			$this->config = $this->readConfig($path, self::CONFIG_FILE_FORMAT);
		}
		if (!is_null($section)) {
			$config = key_exists($section, $this->config) ? $this->config[$section] : [];
		} else {
			$config = $this->config;
		}
		return $config;
	}
}
