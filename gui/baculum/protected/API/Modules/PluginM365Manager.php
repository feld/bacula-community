<?php
/*
 * Bacula(R) - The Network Backup Solution
 * Baculum   - Bacula web interface
 *
 * Copyright (C) 2013-2023 Kern Sibbald
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

use PDO;

/**
 * Microsoft 365 plugin manager module.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category Module
 * @package Baculum API
 */
class PluginM365Manager extends APIModule {

	/**
	 * Get Microsoft 365 jobs by tenant and email owner.
	 *
	 * @param string $tenant tenant name
	 * @param string $owner email owner
	 * @param array $criteria SQL criteria
	 * @return array job list or empty array if no job found
	 */
	public function getJobsByTenantAndOwner($tenant, $owner, $criteria = []) {
		$params = count($criteria) > 0 ? $criteria : [];

		// email owner
		$params['MetaEmail.EmailOwner'] = [];
		$params['MetaEmail.EmailOwner'][] = [
			'operator' => 'AND',
			'vals' => [$owner]
		];

		// email tenant
		$params['MetaEmail.EmailTenant'] = [];
		$params['MetaEmail.EmailTenant'][] = [
			'operator' => 'AND',
			'vals' => [$tenant]
		];

		// job types
		$params['Job.Type'] = [];
		$params['Job.Type'][] = [
			'operator' => 'IN',
			'vals' => ['B']
		];

		// job status
		$params['Job.JobStatus'] = [];
		$params['Job.JobStatus'][] = [
			'operator' => 'IN',
			'vals' => ['T', 'f', 'E', 'A', 'e']
		];

		$where = Database::getWhere($params);

		$sql = 'SELECT DISTINCT Job.JobId     AS jobid,
					Job.Name      AS name,
					Job.StartTime AS starttime,
					Job.EndTime   AS endtime,
					Job.JobFiles  AS jobfiles,
					Job.JobBytes  AS jobbytes
			FROM Job
			LEFT JOIN MetaEmail USING (JobId)
			' . $where['where'] . '
			ORDER BY Job.EndTime DESC';

		$statement = Database::runQuery($sql, $where['params']);
		return $statement->fetchAll(PDO::FETCH_OBJ);
	}
}
