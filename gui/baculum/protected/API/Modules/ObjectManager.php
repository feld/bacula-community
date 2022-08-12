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
 * Object manager module.
 *
 * @author Marcin Haba <marcin.haba@bacula.pl>
 * @category Module
 * @package Baculum API
 */
class ObjectManager extends APIModule {

	public function getObjects($criteria = array(), $limit_val = null) {
		$sort_col = 'ObjectId';
		$db_params = $this->getModule('api_config')->getConfig('db');
		if ($db_params['type'] === Database::PGSQL_TYPE) {
		    $sort_col = strtolower($sort_col);
		}
		$order = ' ORDER BY ' . $sort_col . ' DESC';
		$limit = '';
		if(is_int($limit_val) && $limit_val > 0) {
			$limit = ' LIMIT ' . $limit_val;
		}

		$where = Database::getWhere($criteria);

		$sql = 'SELECT Object.*, 
Job.Name as jobname 
FROM Object 
LEFT JOIN Job USING (JobId) '
. $where['where'] . $order . $limit;

		return ObjectRecord::finder()->findAllBySql($sql, $where['params']);
	}

	public function getObjectById($objectid) {
		$params = [
			'Object.ObjectId' => [
				'vals' => $objectid,
				'operator' => ''
			]
		];
		$obj = $this->getObjects($params, 1);
		if (is_array($obj) && count($obj) > 0) {
			$obj = array_shift($obj);
		}
		return $obj;
	}

	/**
	 * Get number object per object category.
	 *
	 * @param string $objecttype object type (usually short name such as 'm365' or 'MySQL')
	 * @param string $objectsource object source
	 * @param string $datestart start date
	 * @param string $dateend end date
	 * @return array summary in form [objectcategory => '', objecttype => '', objectsource => '', count => 0, last_job_time => '']
	 */
	public function getObjectCategorySum($objecttype = null, $objectsource = null, $datestart = null, $dateend = null) {
		$otype = '';
		if (!is_null($objecttype)) {
			$otype = ' AND oobj.ObjectType=:objecttype ';

		}
		$osource = '';
		if (!is_null($objectsource)) {
			$osource = ' AND oobj.ObjectSource=:objectsource ';
		}
		$dformat = 'Y-m-d H:i:s';
		if (is_null($datestart)) {
			$m_ago = new \DateTime('1 month ago');
			$datestart = $m_ago->format($dformat);
		}
		if (is_null($dateend)) {
			$dateend = date($dformat);
		}

		$sql = 'SELECT oobj.ObjectCategory AS objectcategory,
				oobj.ObjectType AS objecttype,
				oobj.ObjectSource AS objectsource,
				COUNT(DISTINCT oobj.ObjectUUID) AS count,
				MAX(Job.StartTime) AS last_job_time
			FROM Object AS oobj
			JOIN Job USING(JobId)
			WHERE
				Job.StartTime BETWEEN :datestart AND :dateend
				AND Job.JobStatus IN (\'T\', \'W\')
				AND oobj.JobId=(
					SELECT MAX(iobj.JobId) FROM Object AS iobj WHERE iobj.ObjectId=oobj.ObjectId
				)
				' . $otype . $osource . '
			GROUP BY oobj.ObjectCategory, oobj.ObjectType, oobj.ObjectSource';

		$connection = ObjectRecord::finder()->getDbConnection();
		$connection->setActive(true);
		$pdo = $connection->getPdoInstance();
		$sth = $pdo->prepare($sql);
		if (!is_null($objecttype)) {
			$sth->bindParam(':objecttype', $objecttype, \PDO::PARAM_STR, 100);
		}
		if (!is_null($objectsource)) {
			$sth->bindParam(':objectsource', $objectsource, \PDO::PARAM_STR, 400);
		}
		$sth->bindParam(':datestart', $datestart, \PDO::PARAM_STR, 19);
		$sth->bindParam(':dateend', $dateend, \PDO::PARAM_STR, 19);
		$sth->execute();
		return $sth->fetchAll(\PDO::FETCH_ASSOC);
	}

	/**
	 * Get object size statistics.
	 *
	 * @param string $objecttype object type (usually short name such as 'm365' or 'MySQL')
	 * @param string $objectsource object source
	 * @param string $datestart start date
	 * @param string $dateend end date
	 * @return array summary in form [sum => 0, month => '']
	 */
	public function getObjectSizeSum($objecttype = null, $objectsource = null, $datestart = null, $dateend = null) {
		$otype = '';
		if (!is_null($objecttype)) {
			$otype = ' AND Object.ObjectType=:objecttype ';

		}
		$osource = '';
		if (!is_null($objectsource)) {
			$osource = ' AND Object.ObjectSource=:objectsource ';
		}
		$dformat = 'Y-m-d H:i:s';
		if (is_null($datestart)) {
			$m_ago = new \DateTime('1 month ago');
			$datestart = $m_ago->format($dformat);
		}
		if (is_null($dateend)) {
			$dateend = date($dformat);
		}
		$db_params = $this->getModule('api_config')->getConfig('db');
		if ($db_params['type'] === Database::PGSQL_TYPE) {
			$date_month = ' date_trunc(\'month\', Job.StartTime) ';
		} elseif ($db_params['type'] === Database::MYSQL_TYPE) {
			$date_month = ' DATE_FORMAT(Job.StartTime, \'%Y-%m-01 00:00:00\') ';
		} elseif ($db_params['type'] === Database::SQLITE_TYPE) {
			$date_month = ' strftime(\'%Y-%m-01 00:00:00\', Job.StartTime) ';
		}
		$sql = 'SELECT SUM(Object.ObjectSize) AS sum,
			' . $date_month . '           AS month
			FROM Object
				LEFT JOIN Job USING (JobId)
			WHERE
				Job.StartTime BETWEEN :datestart AND :dateend
				' . $otype . $osource . '
			GROUP BY month
			ORDER BY month ASC';
		$connection = ObjectRecord::finder()->getDbConnection();
		$connection->setActive(true);
		$pdo = $connection->getPdoInstance();
		$sth = $pdo->prepare($sql);
		if (!is_null($objecttype)) {
			$sth->bindParam(':objecttype', $objecttype, \PDO::PARAM_STR, 100);
		}
		if (!is_null($objectsource)) {
			$sth->bindParam(':objectsource', $objectsource, \PDO::PARAM_STR, 400);
		}
		$sth->bindParam(':datestart', $datestart, \PDO::PARAM_STR, 19);
		$sth->bindParam(':dateend', $dateend, \PDO::PARAM_STR, 19);
		$sth->execute();
		return $sth->fetchAll(\PDO::FETCH_ASSOC);
	}

	/**
	 * Get all object versions by objectuuid.
	 *
	 * @param string $objectuuid object UUID
	 * @return array object versions
	 */
	public function getObjectVersions($objectuuid) {
		$sql = 'SELECT
				obj.ObjectUUID     AS objectuuid,
				obj.ObjectId	   AS objectid,
				obj.ObjectType	   AS objecttype,
				obj.ObjectName	   AS objectname,
				obj.ObjectCategory AS objectcategory,
				Job.JobId 	   AS jobid,
				Job.Name	   AS jobname,
				Job.Level	   AS level,
				Job.JobStatus	   AS jobstatus,
				Job.JobBytes	   AS jobbytes,
				Job.JobFiles	   AS jobfiles,
				Job.StartTime	   AS starttime,
				Job.EndTime	   AS endtime,
				Job.JobErrors	   AS joberrors,
				Client.Name	   AS client,
				FileSet.FileSet	   AS fileset
				FROM
					Object AS obj
					LEFT JOIN Job USING(JobId)
					LEFT JOIN Client USING(ClientId)
					LEFT JOIN FileSet USING(FileSetId)
				WHERE
					obj.ObjectUUID=:objectuuid
				ORDER BY Job.StartTime DESC';
		$connection = ObjectRecord::finder()->getDbConnection();
		$connection->setActive(true);
		$pdo = $connection->getPdoInstance();
		$sth = $pdo->prepare($sql);
		$sth->bindParam(':objectuuid', $objectuuid, \PDO::PARAM_STR, 400);
		$sth->execute();
		return $sth->fetchAll(\PDO::FETCH_ASSOC);
	}

	/**
	 * Get object job statistics by category.
	 * There are taken into account last three jobs.
	 *
	 * @param string $objecttype object type (usually short name such as 'm365' or 'MySQL')
	 * @param string $objectsource object source
	 * @param string $objectcategory object category like: 'mailbox' or 'team'
	 * @return array object statistics
	 */
	public function getObjectCategoryStatus($objecttype = null, $objectsource = null, $objectcategory = null) {
		$where = [];
		if (!is_null($objecttype)) {
			$where[] = ' Object.ObjectType=:objecttype ';
		}
		if (!is_null($objectsource)) {
			$where[] = ' Object.ObjectSource=:objectsource ';
		}
		if (!is_null($objectcategory)) {
			$where[] = ' Object.ObjectCategory=:objectcategory ';
		}
		$where_val = implode(' AND ', $where);

		$sql = 'SELECT
				DISTINCT b.JobId, b.JobStatus, b.StartTime, b.ObjectCategory
			FROM (
				SELECT
					Job.JobId	 AS jobid,
					Job.JobStatus	 AS jobstatus,
					Job.StartTime	 AS starttime,
					Object.ObjectCategory AS objectcategory,
					ROW_NUMBER() OVER (PARTITION BY Object.ObjectCategory ORDER BY Job.JobId DESC) AS r
				FROM Job
					JOIN Object USING(JobId)
				' . ($where_val ? ' WHERE ' . $where_val : '') . '
			) AS b
			WHERE
				b.r <= 3';
		$connection = ObjectRecord::finder()->getDbConnection();
		$connection->setActive(true);
		$pdo = $connection->getPdoInstance();
		$sth = $pdo->prepare($sql);
		if (!is_null($objecttype)) {
			$sth->bindParam(':objecttype', $objecttype, \PDO::PARAM_STR, 100);
		}
		if (!is_null($objectsource)) {
			$sth->bindParam(':objectsource', $objectsource, \PDO::PARAM_STR, 400);
		}
		if (!is_null($objectcategory)) {
			$sth->bindParam(':objectcategory', $objectcategory, \PDO::PARAM_STR, 400);
		}
		$sth->execute();
		return $sth->fetchAll(\PDO::FETCH_ASSOC);
	}
}
