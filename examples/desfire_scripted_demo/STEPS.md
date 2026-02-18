# Script Demo Steps

This is the execution plan used by `run_demo.ps1`.

## 1. PICC Discovery

1. Run `GetVersion`.
2. Run `GetApplicationIDs` in PICC context (with auth).
3. Run `GetCardUID` (with PICC auth).
4. Run `GetKeySettings` + `GetKeyVersion` for PICC key 0.
5. Run `FreeMemory` (before operations).

## 2. Prepare Demo Applications

1. Optionally delete existing demo AIDs (`--allow-missing`).
2. Create AES app (AID `A1A551`) with one AES key.
3. Create 3K3DES app (AID `A1A552`) with one 3K3DES key.
4. Create DES app (AID `A1A553`) with one DES key.
5. Create 2K3DES app (AID `A1A554`) with one 2K3DES key.
6. Change AES app key 0 from zero key to custom AES key.
7. Change 3K3DES app key 0 from zero key to custom 3K3DES key.
8. Change DES app key 0 from zero key to custom DES key.
9. Change 2K3DES app key 0 from zero key to custom 2K3DES key.
10. List applications again.
11. Read key settings/key version for all apps.
12. Apply `ChangeKeySettings` (`0x0F`) to all apps.

## 3. AES App (`A1A551`) File Lifecycle and Usage

1. Create files:
1. Standard data file (file 1)
1. Backup data file (file 2)
1. Value file (file 3)
1. Linear record file (file 4)
1. Cyclic record file (file 5)
2. Query file inventory/settings:
1. `GetFileIDs`
1. `GetFileSettings` for file 1 and file 3
3. Data operations:
1. Write/read standard file 1
1. Write backup file 2
1. `CommitTransaction`
1. Read backup file 2 after commit
4. Value operations on file 3:
1. `GetValue`
1. `Credit` + commit + before/after
1. `Debit` + commit + before/after
1. `LimitedCredit` + commit + before/after
5. Record operations on file 5:
1. `WriteRecord` + commit
1. `ReadRecords` (1 record)
1. `ClearRecordFile` + commit
6. File settings and cleanup:
1. `ChangeFileSettings` on file 1
1. `DeleteFile` file 4
1. `GetFileIDs` again

## 4. 3K3DES App (`A1A552`) File Lifecycle and Usage

1. Create files:
1. Standard data file (file 1)
1. Value file (file 3)
1. Cyclic record file (file 5)
2. Query file inventory with `GetFileIDs`.
3. Write/read standard file 1.
4. Value operations on file 3:
1. `Credit` + commit + before/after
1. `Debit` + commit + before/after
5. Record operations on file 5:
1. `WriteRecord` + commit
1. `ReadRecords` (1 record)
6. Read `GetFileSettings` for file 5.

## 5. DES App (`A1A553`) File Lifecycle and Usage

1. Create files:
1. Standard data file (file 1)
1. Value file (file 3)
2. Query file inventory with `GetFileIDs`.
3. Write/read standard file 1.
4. Value operations on file 3:
1. `Credit` + commit + before/after
1. `Debit` + commit + before/after
5. Read `GetFileSettings` for file 3.

## 6. 2K3DES App (`A1A554`) File Lifecycle and Usage

1. Create files:
1. Standard data file (file 1)
1. Linear record file (file 4)
2. Query file inventory with `GetFileIDs`.
3. Write/read standard file 1.
4. Record operations on file 4:
1. `WriteRecord` + commit
1. `ReadRecords` (1 record)
5. Read `GetFileSettings` for file 4.

## 7. Final PICC Checks

1. Run `FreeMemory` (after operations).
2. Run `GetApplicationIDs` again.
3. Optional cleanup: delete all demo apps.

## 8. Optional Dangerous PICC Commands (Explicit Flags Required)

1. `SetConfiguration` (`-RunSetConfiguration` + `-ConfirmDangerousPiccOperations`).
2. `FormatPICC` (`-RunFormatPicc` + `-ConfirmDangerousPiccOperations`).

These are intentionally opt-in and not part of the default demo flow.
