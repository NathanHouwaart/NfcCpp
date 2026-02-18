# DESFire FreeMemory Example

This example performs:

1. `SelectApplication(<aid>)` (default `000000`, PICC)
2. Optional `Authenticate`
3. `FreeMemory()`
4. Print available memory in bytes/KiB

Use this to query remaining PICC memory before creating applications/files.

## Build

PowerShell:

```powershell
.\examples\desfire_free_memory\build_example.ps1
```

Batch:

```bat
examples\desfire_free_memory\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_free_memory_example.exe <COM_PORT> [options]
```

## Command Key and Purpose

- `FreeMemory` uses INS `0x6E`.
- Payload is empty.
- Response payload is 3 bytes (little-endian): available free memory on the PICC.
- This is typically a PICC-level command; use AID `000000`.

## Argument Reference

| Argument | Required | Default | Description |
|---|---|---|---|
| `<COM_PORT>` | Yes | - | Serial port for PN532, e.g. `COM5`. |
| `--baud <n>` | No | `115200` | Serial baud rate. |
| `--aid <hex6>` | No | `000000` | Selected AID before command; PICC context is recommended. |
| `--authenticate` | No | off | Authenticate before calling `FreeMemory`. |
| `--auth-mode <legacy\|iso\|aes\|des\|2k3des\|3k3des>` | No | `iso` | Auth mode (`des` -> `legacy`, `2k3des/3k3des` -> `iso`). |
| `--auth-key-no <n>` | No | `0` | Auth key number. |
| `--auth-key-hex <hex>` | Cond. | - | Required when `--authenticate` is set. |

## Usage Examples

Query free memory in PICC context without authenticate:

```powershell
.\build\examples\Debug\desfire_free_memory_example.exe COM5
```

Query free memory with PICC AES authenticate:

```powershell
.\build\examples\Debug\desfire_free_memory_example.exe COM5 `
  --aid 000000 `
  --authenticate `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00"
```

## Notes

- Some card configurations may require PICC authenticate to read free memory.
- `FreeMemory` reports PICC-level available memory, not per-application usage.

