# DESFire CreateApplication Example

This example performs:

1. `SelectApplication(000000)` to select PICC.
2. `Authenticate` with the PICC master key.
3. `CreateApplication`.
4. `SelectApplication(<new aid>)`.
5. `Authenticate` inside the new application.

## Build

PowerShell:

```powershell
.\examples\desfire_create_application\build_example.ps1
```

Batch:

```bat
examples\desfire_create_application\build_example.bat
```

## Command Syntax

```powershell
.\build\examples\Debug\desfire_create_application_example.exe <COM_PORT> [options]
```

## Argument Reference

| Argument | Required | Default | Description |
|---|---|---|---|
| `<COM_PORT>` | Yes | - | Serial port for PN532, for example `COM5`. |
| `--baud <n>` | No | `115200` | Serial baud rate. |
| `--picc-auth-mode <legacy\|iso\|aes>` | No | `iso` | Authentication mode used on PICC (AID `000000`) before `CreateApplication`. |
| `--picc-auth-key-no <n>` | No | `0` | PICC key number to authenticate with. |
| `--picc-auth-key-hex <hex>` | Yes | - | PICC key bytes in hex. Length must match mode: `legacy=8/16`, `iso=8/16/24`, `aes=16` bytes. |
| `--app-aid <hex6>` | Yes | - | New application AID, exactly 3 bytes (6 hex chars), for example `123456`. |
| `--app-key-settings <n>` | No | `0x0F` | Application key settings byte (`KeySettings1`) passed to `CreateApplication`. Accepts decimal or `0x..`. |
| `--app-key-count <n>` | No | `1` | Number of keys in the app (`1..14`). |
| `--app-key-type <des\|2k3des\|3k3des\|aes>` | No | `aes` | Application crypto type encoded in `KeySettings2`. |
| `--app-auth-key-no <n>` | No | `0` | Key number used for the post-create authenticate inside the new app. |
| `--app-auth-mode <auto\|legacy\|iso\|aes>` | No | `auto` | Auth mode for app authenticate. `auto` picks: `des->legacy`, `2k3des/3k3des->iso`, `aes->aes`. |
| `--app-auth-key-hex <hex>` | No | all-zero key | Key used for app authenticate. If omitted, the example uses all-zero with size from app key type (`des=8`, `2k3des=16`, `3k3des=24`, `aes=16` bytes). |
| `--allow-existing` | No | off | If app already exists (`DuplicateError`), continue instead of failing. |

## Hex Input Format

- Hex can be passed as compact form (`00112233...`) or spaced (`00 11 22 33 ...`).
- Non-hex separators are ignored, so spaces and dashes are fine.
- If you use spaced hex in PowerShell, wrap it in quotes, for example:
  `--picc-auth-key-hex "00 00 00 00 00 00 00 00"`.

## Run Example

Example creating AES app `12 34 56`, then authenticating key 0 in that app:

```powershell
.\build\examples\Debug\desfire_create_application_example.exe COM5 `
  --picc-auth-mode iso `
  --picc-auth-key-no 0 `
  --picc-auth-key-hex "0000000000000000" `
  --app-aid 123456 `
  --app-key-settings 0x0F `
  --app-key-count 1 `
  --app-key-type aes `
  --app-auth-mode aes `
  --app-auth-key-no 0 `
  --app-auth-key-hex "00000000000000000000000000000000" `
  --allow-existing
```

Example creating a `2k3des` app with key 0:

```powershell
.\build\examples\Debug\desfire_create_application_example.exe COM5 `
  --picc-auth-mode iso `
  --picc-auth-key-no 0 `
  --picc-auth-key-hex "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00" `
  --app-aid A1B2C3 `
  --app-key-settings 0x0F `
  --app-key-count 1 `
  --app-key-type 2k3des `
  --app-auth-mode iso `
  --app-auth-key-no 0 `
  --app-auth-key-hex "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
```

Example workflow: PICC master key is DES all-`00`, create a new AES app, then set a custom AES key:

1. Create the application (new app key 0 starts as AES all-`00`):

```powershell
.\build\examples\Debug\desfire_create_application_example.exe COM5 `
  --picc-auth-mode legacy `
  --picc-auth-key-no 0 `
  --picc-auth-key-hex "00 00 00 00 00 00 00 00" `
  --app-aid BADA55 `
  --app-key-settings 0x0F `
  --app-key-count 1 `
  --app-key-type aes `
  --app-auth-mode aes `
  --app-auth-key-no 0 `
  --app-auth-key-hex "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00" `
  --allow-existing
```

2. Change app key 0 from AES all-`00` to your custom AES key:

```powershell
.\build\examples\Debug\desfire_auth_changekey_example.exe COM5 `
  --aid BADA55 `
  --auth-mode aes `
  --auth-key-no 0 `
  --auth-key-hex "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00" `
  --change-key-no 0 `
  --new-key-type aes `
  --new-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --new-key-version 0 `
  --confirm-change
```

## Notes

- Use `--allow-existing` if you want reruns to continue when the app already exists.
- For `des` / `2k3des` / `3k3des` applications, set `--app-key-type` and `--app-auth-mode` to matching values.
- Keep PICC master key credentials correct or `CreateApplication` will fail with permission/auth errors.
