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

namespace Baculum\Common\Modules\Errors;

/**
 * Actions error class.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category Errors
 * @package Baculum Common
 */
class ActionsError extends GenericError {

	const ERROR_ACTIONS_ACTION_DOES_NOT_EXIST = 110;
	const ERROR_ACTIONS_DISABLED = 111;
	const ERROR_ACTIONS_WRONG_EXITCODE = 112;
	const ERROR_ACTIONS_NOT_CONFIGURED = 113;

	const MSG_ERROR_ACTIONS_ACTION_DOES_NOT_EXIST = 'Action does not exist.';
	const MSG_ERROR_ACTIONS_DISABLED = 'Actions support is disabled.';
	const MSG_ERROR_ACTIONS_WRONG_EXITCODE = 'Action command returned wrong exitcode.';
	const MSG_ERROR_ACTIONS_NOT_CONFIGURED = 'Action is not configured.';
}
