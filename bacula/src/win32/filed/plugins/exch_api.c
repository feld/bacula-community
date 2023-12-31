/*
   Bacula(R) - The Network Backup Solution

   Copyright (C) 2000-2022 Kern Sibbald

   The original author of Bacula is Kern Sibbald, with contributions
   from many others, a complete list can be found in the file AUTHORS.

   You may use this file and others of this release according to the
   license defined in the LICENSE file, which includes the Affero General
   Public License, v3.0 ("AGPLv3") and some additional permissions and
   terms pursuant to its AGPLv3 Section 7.

   This notice must be preserved when any source code is
   conveyed and/or propagated.

   Bacula(R) is a registered trademark of Kern Sibbald.
*/
/* 
 *  Written by James Harper, July 2010
 *  
 *  Used only in "old Exchange plugin" now deprecated.
 */

#include "exchange-fd.h"

HrESEBackupRestoreGetNodes_t HrESEBackupRestoreGetNodes;
HrESEBackupPrepare_t HrESEBackupPrepare;
HrESEBackupGetLogAndPatchFiles_t HrESEBackupGetLogAndPatchFiles;
HrESEBackupTruncateLogs_t HrESEBackupTruncateLogs;
HrESEBackupEnd_t HrESEBackupEnd;
HrESEBackupSetup_t HrESEBackupSetup;
HrESEBackupInstanceEnd_t HrESEBackupInstanceEnd;
HrESEBackupOpenFile_t HrESEBackupOpenFile;
HrESEBackupReadFile_t HrESEBackupReadFile;
HrESEBackupCloseFile_t HrESEBackupCloseFile;

HrESERestoreOpen_t HrESERestoreOpen;
HrESERestoreReopen_t HrESERestoreReopen;
HrESERestoreComplete_t HrESERestoreComplete;
HrESERestoreClose_t HrESERestoreClose;
HrESERestoreGetEnvironment_t HrESERestoreGetEnvironment;
HrESERestoreSaveEnvironment_t HrESERestoreSaveEnvironment;
HrESERestoreAddDatabase_t HrESERestoreAddDatabase;
HrESERestoreOpenFile_t HrESERestoreOpenFile;

bRC
loadExchangeApi()
{
   HMODULE h;
   LONG status;
   HKEY key_handle;
   WCHAR *buf;
   DWORD buf_len;
   DWORD type;

   status = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\BackupRestore\\DLLPaths", &key_handle);
   if (status != ERROR_SUCCESS)
   {
      _JobMessageNull(M_FATAL, "Cannot get key for Exchange DLL path, result = %08x\n", status);
      return bRC_Error;
   }
   
   type = REG_EXPAND_SZ;
   status = RegQueryValueExW(key_handle, L"esebcli2", NULL, &type, NULL, &buf_len);
   if (status != ERROR_SUCCESS)
   {
      _JobMessageNull(M_FATAL, "Cannot get key for Exchange DLL path, result = %08x\n", status);
      return bRC_Error;
   }
   buf_len += 2;
   buf = new WCHAR[buf_len];

   type = REG_EXPAND_SZ;
   status = RegQueryValueExW(key_handle, L"esebcli2", NULL, &type, (LPBYTE)buf, &buf_len);
   if (status != ERROR_SUCCESS)
   {
      _JobMessageNull(M_FATAL, "Cannot get key for Exchange DLL path, result = %08x\n", status);
      delete buf;
      return bRC_Error;
   }

printf("Got value %S\n", buf);

   // strictly speaking, a REG_EXPAND_SZ should be run through ExpandEnvironmentStrings

   h = LoadLibraryW(buf);
   delete buf;
   if (!h) {
      _JobMessageNull(M_FATAL, "Cannot load Exchange DLL\n");
      return bRC_Error;
   }
   HrESEBackupRestoreGetNodes = (HrESEBackupRestoreGetNodes_t)GetProcAddress(h, "HrESEBackupRestoreGetNodes");
   HrESEBackupPrepare = (HrESEBackupPrepare_t)GetProcAddress(h, "HrESEBackupPrepare");
   HrESEBackupEnd = (HrESEBackupEnd_t)GetProcAddress(h, "HrESEBackupEnd");
   HrESEBackupSetup = (HrESEBackupSetup_t)GetProcAddress(h, "HrESEBackupSetup");
   HrESEBackupGetLogAndPatchFiles = (HrESEBackupGetLogAndPatchFiles_t)GetProcAddress(h, "HrESEBackupGetLogAndPatchFiles");
   HrESEBackupTruncateLogs = (HrESEBackupTruncateLogs_t)GetProcAddress(h, "HrESEBackupTruncateLogs");
   HrESEBackupInstanceEnd = (HrESEBackupInstanceEnd_t)GetProcAddress(h, "HrESEBackupInstanceEnd");
   HrESEBackupOpenFile = (HrESEBackupOpenFile_t)GetProcAddress(h, "HrESEBackupOpenFile");
   HrESEBackupReadFile = (HrESEBackupReadFile_t)GetProcAddress(h, "HrESEBackupReadFile");
   HrESEBackupCloseFile = (HrESEBackupCloseFile_t)GetProcAddress(h, "HrESEBackupCloseFile");
   HrESERestoreOpen = (HrESERestoreOpen_t)GetProcAddress(h, "HrESERestoreOpen");
   HrESERestoreReopen = (HrESERestoreReopen_t)GetProcAddress(h, "HrESERestoreReopen");
   HrESERestoreComplete = (HrESERestoreComplete_t)GetProcAddress(h, "HrESERestoreComplete");
   HrESERestoreClose = (HrESERestoreClose_t)GetProcAddress(h, "HrESERestoreClose");
   HrESERestoreSaveEnvironment = (HrESERestoreSaveEnvironment_t)GetProcAddress(h, "HrESERestoreSaveEnvironment");
   HrESERestoreGetEnvironment = (HrESERestoreGetEnvironment_t)GetProcAddress(h, "HrESERestoreGetEnvironment");
   HrESERestoreAddDatabase = (HrESERestoreAddDatabase_t)GetProcAddress(h, "HrESERestoreAddDatabase");
   HrESERestoreOpenFile = (HrESERestoreOpenFile_t)GetProcAddress(h, "HrESERestoreOpenFile");
   return bRC_OK;
}

const char *
ESEErrorMessage(HRESULT result)
{
   switch (result) {
   case 0:
      return "No error.";
   case hrLogfileHasBadSignature:
      return "Log file has bad signature. Check that no stale files are left in the Exchange data/log directories.";
   case hrCBDatabaseInUse:
      return "Database in use. Make sure database is dismounted.";
   case hrRestoreAtFileLevel:
      return "File must be restored using Windows file I/O calls.";
   case hrMissingFullBackup:
      return "Exchange reports that no previous full backup has been done.";
   case hrBackupInProgress:
      return "Exchange backup already in progress.";
   case hrLogfileNotContiguous:
      return "Existing log file is not contiguous. Check that no stale files are left in the Exchange data/log directories.";
   case hrErrorFromESECall:
      return "Error returned from ESE function call. Check the Windows Event Logs for more information.";
   case hrCBDatabaseNotFound:
      return "Database not found. Check that the Database you are trying to restore actually exists in the Storage Group you are restoring to.";
   default:
      return "Unknown error.";
   }
}
