# DESFire CreateValueFile Example

This example performs:

1. `SelectApplication(<aid>)`
2. `Authenticate` in that application
3. `CreateValueFile`

## Build

PowerShell:

```powershell
.\examples\desfire_create_value_file\build_example.ps1
```

Batch:

```bat
examples\desfire_create_value_file\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_create_value_file_example.exe <COM_PORT> [options]
```

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
| `--lower-limit <n>` | Yes | - | Lower bound for value file (signed 32-bit). |
| `--upper-limit <n>` | Yes | - | Upper bound for value file (signed 32-bit). |
| `--limited-credit-value <n>` | Yes | - | Limited credit value (signed 32-bit), must be within bounds. |
| `--limited-credit-enabled` | No | off | Sets value-options bit0. |
| `--free-get-value-enabled` | No | off | Sets value-options bit1. |
| `--allow-existing` | No | off | Continue when `CreateValueFile` returns `DuplicateError`. |

## What CreateValueFile Is For

`CreateValueFile` creates a DESFire value-file container that stores a signed 32-bit value managed by:

- `Credit`
- `Debit`
- `LimitedCredit`
- `GetValue`

It also sets:

- lower and upper bounds,
- limited-credit behavior/options,
- communication mode and access rights for operations.

## Access Rights Encoding

The command sends access rights as two bytes:

- Byte0: `[RW|CAR]`
- Byte1: `[R|W]`

Each field is a 4-bit nibble:

- `0x0..0xD`: key number 0..13
- `0xE`: free access
- `0xF`: never

## Value Options Byte

`CreateValueFile` sends a 1-byte options field:

- bit0 (`0x01`): limited credit enabled
- bit1 (`0x02`): free get value enabled

Other bits are reserved and rejected by this implementation.

## Run Example

Create file `3` with range `-1000..1000`, limited-credit value `100`, key0-protected:

```powershell
.\build\examples\Debug\desfire_create_value_file_example.exe COM5 `
  --aid BADA55 `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 3 `
  --comm-mode enc `
  --read-access key0 `
  --write-access key0 `
  --read-write-access key0 `
  --change-access key0 `
  --lower-limit -1000 `
  --upper-limit 1000 `
  --limited-credit-value 100 `
  --limited-credit-enabled `
  --allow-existing
```

Create a wider range and enable free-get-value:

```powershell
.\build\examples\Debug\desfire_create_value_file_example.exe COM5 `
  --aid BADA55 `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --file-no 4 `
  --comm-mode plain `
  --read-access free `
  --write-access key0 `
  --read-write-access key0 `
  --change-access key0 `
  --lower-limit 0 `
  --upper-limit 100000 `
  --limited-credit-value 500 `
  --limited-credit-enabled `
  --free-get-value-enabled `
  --allow-existing
```

## Notes

- `CreateValueFile` is an application-level command. Select and authenticate that application first.
- Most applications require master-key authentication for file creation.
- This command creates settings/limits; it does not perform `Credit`, `Debit`, or `GetValue`.
- Use `--allow-existing` for idempotent reruns while iterating.

## Relation To ChangeKeySettings

`ChangeKeySettings` and `CreateValueFile` serve different purposes:

- `ChangeKeySettings` changes application security policy (who may list/create/delete/change keys).
- `CreateValueFile` creates one file object and defines its value semantics/access controls.

Use `ChangeKeySettings` to configure policy, and `CreateValueFile` to define value storage.

