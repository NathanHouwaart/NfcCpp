# DESFire CommitTransaction Example

This example performs:

1. `SelectApplication(<aid>)`
2. Optional `Authenticate`
3. `CommitTransaction()`

Use it after transactional commands like:

- `WriteData` on backup data files
- `Credit`, `Debit`, `LimitedCredit` on value files

## Build

PowerShell:

```powershell
.\examples\desfire_commit_transaction\build_example.ps1
```

Batch:

```bat
examples\desfire_commit_transaction\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_commit_transaction_example.exe <COM_PORT> [options]
```

## Argument Reference

| Argument | Required | Default | Description |
|---|---|---|---|
| `<COM_PORT>` | Yes | - | Serial port for PN532, e.g. `COM5`. |
| `--baud <n>` | No | `115200` | Serial baud rate. |
| `--aid <hex6>` | Yes | - | Target application AID, exactly 3 bytes. |
| `--authenticate` | No | off | Authenticate before commit. |
| `--auth-mode <legacy\|iso\|aes\|des\|2k3des\|3k3des>` | No | `iso` | Authentication mode (`des` -> `legacy`, `2k3des/3k3des` -> `iso`). |
| `--auth-key-no <n>` | No | `0` | Auth key number. |
| `--auth-key-hex <hex>` | Cond. | - | Required when `--authenticate` is set. |
| `--allow-no-changes` | No | off | Treat `NoChanges (0x0C)` as success when no pending transaction exists. |

## Usage Examples

Commit pending transaction in app `BADA55` after AES auth:

```powershell
.\build\examples\Debug\desfire_commit_transaction_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00"
```

Allow "no pending transaction" without failing:

```powershell
.\build\examples\Debug\desfire_commit_transaction_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --allow-no-changes
```

## Notes

- `CommitTransaction` commits pending transactional updates in the currently selected application.
- If access rights require it, authenticate before committing.
- If you skip commit, pending transactional updates may be discarded.

