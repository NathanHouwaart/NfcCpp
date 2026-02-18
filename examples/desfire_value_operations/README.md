# DESFire Value Operations Example

This example performs:

1. `SelectApplication(<aid>)`
2. Optional `Authenticate`
3. One operation: `GetValue`, `Credit`, `Debit`, or `LimitedCredit`

## Build

PowerShell:

```powershell
.\examples\desfire_value_operations\build_example.ps1
```

Batch:

```bat
examples\desfire_value_operations\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_value_operations_example.exe <COM_PORT> [options]
```

## Argument Reference

| Argument | Required | Default | Description |
|---|---|---|---|
| `<COM_PORT>` | Yes | - | Serial port for PN532, e.g. `COM5`. |
| `--baud <n>` | No | `115200` | Serial baud rate. |
| `--aid <hex6>` | Yes | - | Target application AID, exactly 3 bytes. |
| `--file-no <n>` | No | `0` | Value-file number (`0..31`). |
| `--op <get\|credit\|debit\|limited-credit>` | Yes | - | Operation to execute. |
| `--value <n>` | Cond. | - | Signed 32-bit amount for `credit`, `debit`, `limited-credit`. |
| `--show-before` | No | off | Read value before mutating op. |
| `--show-after` | No | off* | Read value after mutating op. Auto-enabled if neither before/after is set for mutating ops. |
| `--commit` | No | off | Executes `CommitTransaction` after `credit`/`debit`/`limited-credit`. |
| `--allow-no-changes-on-commit` | No | off | With `--commit`, treat `NoChanges (0x0C)` as success. |
| `--authenticate` | No | off | Authenticate before operation. |
| `--auth-mode <legacy\|iso\|aes\|des\|2k3des\|3k3des>` | No | `iso` | Authentication mode (`des` -> `legacy`, `2k3des/3k3des` -> `iso`). |
| `--auth-key-no <n>` | No | `0` | Auth key number. |
| `--auth-key-hex <hex>` | Cond. | - | Required when `--authenticate` is used. |

## Operations

- `get`: reads the current signed value from the target value file.
- `credit`: increases the value by `--value` (non-negative).
- `debit`: decreases the value by `--value` (non-negative).
- `limited-credit`: executes limited-credit with `--value` (non-negative).

## Usage Examples

Read current value:

```powershell
.\build\examples\Debug\desfire_value_operations_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 3 `
  --op get
```

Credit by 25 and show before/after:

```powershell
.\build\examples\Debug\desfire_value_operations_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 3 `
  --op credit `
  --value 25 `
  --commit `
  --show-before `
  --show-after
```

Debit by 10:

```powershell
.\build\examples\Debug\desfire_value_operations_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 3 `
  --op debit `
  --value 10 `
  --commit
```

Limited credit by 5:

```powershell
.\build\examples\Debug\desfire_value_operations_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 3 `
  --op limited-credit `
  --value 5 `
  --commit
```

## Notes

- Value operations are application-level operations on a value file that should already exist (`CreateValueFile`).
- Access rights set on that file determine whether auth is required for each operation.
- `Credit`, `Debit`, and `LimitedCredit` are transactional on many cards; use `--commit` to persist changes immediately.
- Without `--commit`, your after-read may still show the pre-transaction value.
- You can still use `desfire_commit_transaction_example` as a separate explicit commit step.
