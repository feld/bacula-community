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

namespace Baculum\Web\Modules;

use Baculum\Common\Modules\ISessionItem;
use Baculum\Common\Modules\SessionRecord;
use Prado\Prado;

/**
 * OAuth2 session record module.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category Module
 * @package Baculum Web
 */
class OAuth2Record extends SessionRecord implements ISessionItem {

	public $host;
	public $state;
	public $tokens;
	public $refresh_time;

	public static function getRecordId() {
		return 'oauth2_cli_params';
	}

	public static function getPrimaryKey() {
		return 'host';
	}

	public static function getSessionFile() {
		return Prado::getPathOfNamespace('Baculum.Web.Config.session', '.dump');
	}
}
