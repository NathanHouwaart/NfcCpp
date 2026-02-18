# DESFire CreateCyclicRecordFile Example

This example performs:

1. `SelectApplication(<aid>)`
2. `Authenticate` in that application
3. `CreateCyclicRecordFile`

## Build

PowerShell:

```powershell
.\examples\desfire_create_cyclic_record_file\build_example.ps1
```

Batch:

```bat
examples\desfire_create_cyclic_record_file\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_create_cyclic_record_file_example.exe <COM_PORT> [options]
```

## Command Key and Purpose

- DESFire command key (INS): `0xC0`
- Command name: `CreateCyclicRecordFile`
- Purpose: create a cyclic record file where writes continue in a ring buffer and overwrite the oldest records when full.

Cyclic record files are useful for rolling logs where only the most recent records need to be retained.

## Argument Reference

| Argument | Required | Default | Description |
|---|---|---|---|
| `<COM_PORT>` | Yes | - | Serial port for PN532, for example `COM5`. |
| `--baud <n>` | No | `115200` | Serial baud rate. |
| `--aid <hex6>` | Yes | - | Target application AID, exactly 3 bytes (6 hex chars). |
| `--auth-mode <legacy\|iso\|aes\|des\|2k3des\|3k3des>` | No | `iso` | Authentication mode for app authenticate. `des` maps to `legacy`, `2k3des/3k3des` map to `iso`. |
| `--auth-key-no <n>` | No | `0` | Key number used for authenticate. |
| `--auth-key-hex <hex>` | Yes | - | Authentication key bytes. Length must match selected auth mode. |
| `--file-no <n>` | No | `0` | File number (`0..31`). |
| `--comm-mode <plain\|mac\|enc\|0x00\|0x01\|0x03>` | No | `plain` | File communication mode byte (`0x00`, `0x01`, `0x03`). |
| `--read-access <n\|keyN\|free\|never>` | No | `0` | Read access nibble (`R`). |
| `--write-access <n\|keyN\|free\|never>` | No | `0` | Write access nibble (`W`). |
| `--read-write-access <n\|keyN\|free\|never>` | No | `0` | Read+write access nibble (`RW`). |
| `--change-access <n\|keyN\|free\|never>` | No | `0` | Change access rights nibble (`CAR`). |
| `--record-size <n>` | Yes | - | Size of one record in bytes (`1..16777215`). |
| `--max-records <n>` | Yes | - | Maximum number of records (`1..16777215`). |
| `--allow-existing` | No | off | Continue when `CreateCyclicRecordFile` returns `DuplicateError`. |

## Access Rights Encoding

The command sends access rights as two bytes:

- Byte0: `[RW|CAR]`
- Byte1: `[R|W]`

Each field is a 4-bit nibble:

- `0x0..0xD`: key number 0..13
- `0xE`: free access
- `0xF`: never

## Run Example

Create file `4` with 16-byte records and room for 20 records:

```powershell
.\build\examples\Debug\desfire_create_cyclic_record_file_example.exe COM5 `
  --aid BADA55 `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 5 `
  --comm-mode enc `
  --read-access key0 `
  --write-access key0 `
  --read-write-access key0 `
  --change-access key0 `
  --record-size 16 `
  --max-records 20 `
  --allow-existing
```

Create public read-only log file:

```powershell
.\build\examples\Debug\desfire_create_cyclic_record_file_example.exe COM5 `
  --aid BADA55 `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 5 `
  --comm-mode enc `
  --read-access free `
  --write-access never `
  --read-write-access never `
  --change-access key0 `
  --record-size 24 `
  --max-records 10 `
  --allow-existing
```

## Notes

- `CreateCyclicRecordFile` is an application-level command. Select and authenticate that application first.
- Most apps require master-key authentication for file creation.
- This command creates metadata only; it does not write records.
- Use `WriteRecord`, `ReadRecords`, and `ClearRecordFile` for record operations.
- Use `--allow-existing` for idempotent reruns while iterating.


