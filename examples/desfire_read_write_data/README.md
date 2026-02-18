# DESFire ReadData + WriteData Example

This example performs:

1. `SelectApplication(<aid>)`
2. Optional `Authenticate`
3. Optional `WriteData(<fileNo>, <offset>, <bytes>)`
4. Optional `ReadData(<fileNo>, <offset>, <length>)`

If only `--write-hex` is provided, the example automatically reads back the same number of bytes from the same offset.

## Build

PowerShell:

```powershell
.\examples\desfire_read_write_data\build_example.ps1
```

Batch:

```bat
examples\desfire_read_write_data\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_read_write_data_example.exe <COM_PORT> [options]
```

## Argument Reference

| Argument | Required | Default | Description |
|---|---|---|---|
| `<COM_PORT>` | Yes | - | Serial port for PN532, for example `COM5`. |
| `--baud <n>` | No | `115200` | Serial baud rate. |
| `--aid <hex6>` | No | `000000` | Target AID (3 bytes). |
| `--file-no <n>` | No | `0` | File number (`0..31`). |
| `--chunk-size <n>` | No | command default | Max chunk size per command cycle (`0..240`, `0` means default). |
| `--authenticate` | No | off | Authenticate before read/write. |
| `--auth-mode <legacy\|iso\|aes\|des\|2k3des\|3k3des>` | No | `iso` | Auth mode. `des` maps to `legacy`; `2k3des/3k3des` map to `iso`. |
| `--auth-key-no <n>` | No | `0` | Key number used to authenticate. |
| `--auth-key-hex <hex>` | Conditional | - | Required when `--authenticate` is used. |
| `--write-offset <n>` | No | `0` | Start offset for write (24-bit). |
| `--write-hex <hex>` | Optional | - | Bytes to write. |
| `--read-offset <n>` | No | `0` | Start offset for read (24-bit). |
| `--read-length <n>` | Optional | - | Number of bytes to read. |

## Usage Examples

Write bytes to file 1 in `BADA55` and read back automatically:

```powershell
.\build\examples\Debug\desfire_read_write_data_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 2 `
  --write-offset 0 `
  --write-hex "DE AD BE EF 01 02 03 04"
```

Read 32 bytes from offset 0x10:

```powershell
.\build\examples\Debug\desfire_read_write_data_example.exe COM5 `
  --aid BADA55 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 1 `
  --read-offset 16 `
  --read-length 32 `
  --chunk-size 64
```

## Notes

- `ReadData` uses DESFire command `0xBD`.
- `WriteData` uses DESFire command `0x3D`.
- This example targets standard/backup data files.
- Secure read/write behavior depends on your application/file communication settings.

