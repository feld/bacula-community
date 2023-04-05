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
 * Source (client + fileset + job) manager module.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category Module
 * @package Baculum API
 */
class SourceManager extends APIModule {

	public function getSources($criteria = [], $limit_val = null, $offset_val = 0) {
		$limit = '';
		if(is_int($limit_val) && $limit_val > 0) {
			$limit = ' LIMIT ' . $limit_val;
		}
		$offset = '';
		if (is_int($offset_val) && $offset_val > 0) {
			$offset = ' OFFSET ' . $offset_val;
		}
		$where = Database::getWhere($criteria, true);
		$sql = 'SELECT DISTINCT 
	sres.fileset, sres.client, sres.job, jres.StartTime, jres.EndTime, ores.jobid, fres.Content, jres.JobStatus, jres.JobErrors
	FROM Job AS jres,
	     FileSet AS fres,
	(
		SELECT DISTINCT
			FileSet.FileSet AS fileset,
			Client.Name AS client,
			Job.Name AS job
		FROM Job
		JOIN FileSet USING (FileSetId)
		JOIN Client USING (ClientId)
	) AS sres, (
		SELECT
			MAX(JobId) AS jobid,
			FileSet.FileSet AS fileset,
			Client.Name AS client,
			Job.Name AS job
		FROM Job
		JOIN FileSet USING (FileSetId)
		JOIN Client USING (ClientId)
		GROUP BY FileSet.FileSet, Client.Name, Job.Name
	) AS ores
	LEFT JOIN Object USING (JobId)
	WHERE
		jres.JobId = ores.jobid
		AND jres.FileSetId = fres.FileSetId
		AND sres.job = ores.job
		AND sres.client = ores.client
		AND sres.fileset = ores.fileset
		AND jres.Type = \'B\'
		' . (!empty($where['where']) ? ' AND ' .  $where['where']  : '') . '
	ORDER BY sres.fileset ASC, sres.client ASC, sres.job ASC, jres.starttime ASC ' . $limit . $offset;

		$connection = SourceRecord::finder()->getDbConnection();
		$connection->setActive(true);
		$pdo = $connection->getPdoInstance();
		$sth = $pdo->prepare($sql);
		$sth->execute($where['params']);
		return $sth->fetchAll(\PDO::FETCH_ASSOC);
	}
}
?>
