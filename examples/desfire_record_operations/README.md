# DESFire ReadRecords + WriteRecord Example

This example performs:

1. `SelectApplication(<aid>)`
2. Optional `Authenticate`
3. Optional `WriteRecord(<fileNo>, <offset>, <payload>)`
4. Optional `CommitTransaction`
5. Optional `ReadRecords(<fileNo>, <offset>, <count>)`

If only `--write-hex` is provided, the example automatically reads back 1 record from read offset `0`.

## Build

PowerShell:

```powershell
.\examples\desfire_record_operations\build_example.ps1
```

Batch:

```bat
examples\desfire_record_operations\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_record_operations_example.exe <COM_PORT> [options]
```

## Command Keys and Purpose

- `ReadRecords` uses INS `0xBB` and reads records from a record file.
- `WriteRecord` uses INS `0x3B` and writes records into a record file.
- Record file updates are transactional. Use `--commit` to call `CommitTransaction` after writes.
 
`ReadRecords` CLI values are record-based (`--read-offset`, `--read-count`).
`WriteRecord` CLI values are byte-based (`--write-offset` is byte offset within one record, `--write-hex` length is bytes to write).

## Argument Reference

| Argument | Required | Default | Description |
|---|---|---|---|
| `<COM_PORT>` | Yes | - | Serial port for PN532, for example `COM5`. |
| `--baud <n>` | No | `115200` | Serial baud rate. |
| `--aid <hex6>` | No | `000000` | Target AID (3 bytes). |
| `--file-no <n>` | No | `0` | File number (`0..31`). |
| `--chunk-size <n>` | No | command default | Max chunk size per command cycle (`0..240`, `0` means default). |
| `--authenticate` | No | off | Authenticate before operations. |
| `--auth-mode <legacy\|iso\|aes\|des\|2k3des\|3k3des>` | No | `iso` | Auth mode. `des` maps to `legacy`; `2k3des/3k3des` map to `iso`. |
| `--auth-key-no <n>` | No | `0` | Key number used to authenticate. |
| `--auth-key-hex <hex>` | Conditional | - | Required when `--authenticate` is used. |
| `--write-offset <n>` | No | `0` | Byte offset within record for write. |
| `--write-hex <hex>` | Optional | - | Record payload bytes. Must satisfy `write-offset + data-length <= record-size`. |
| `--commit` | No | off | Run `CommitTransaction` after write. |
| `--read-offset <n>` | No | `0` | Record index offset for read. |
| `--read-count <n>` | Optional | - | Number of records to read. `0` means all from offset. |
| `--read-length <n>` | Optional | - | Alias for `--read-count`. |

## Usage Examples

Write 1 record (16 bytes) to record file `5`, commit, and auto-read it back:

```powershell
.\build\examples\Debug\desfire_record_operations_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 5 `
  --write-offset 0 `
  --write-hex "DE AD BE EF 01 02 03 04 05 06 07 08 09 0A 0B 0C" `
  --commit
```

Read 1 record from offset 0:

```powershell
.\build\examples\Debug\desfire_record_operations_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 5 `
  --read-offset 0 `
  --read-count 1
```

Read all records from offset 0:

```powershell
.\build\examples\Debug\desfire_record_operations_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 5 `
  --read-offset 0 `
  --read-count 0
```

## Notes

- This example targets linear/cyclic DESFire record files.
- It prints file `record-size` and `current/max` counts to help choose valid read values.
- If you request more records than currently available, the card returns `BoundaryError`.
- For `WriteRecord`, invalid byte ranges (for example offset `1` with `16` bytes on a `16`-byte record) exceed the record boundary and are rejected.
- Write operations on record files usually require `CommitTransaction` to persist.
- Access permissions and file communication settings are enforced by the card.
