# DESFire Authenticate + ChangeKey Example

This example performs:

1. `SelectApplication`
2. `Authenticate` (`legacy`, `iso`, or `aes`)
3. `ChangeKey` (only when `--confirm-change` is supplied)

## Build

PowerShell:

```powershell
.\examples\desfire_auth_changekey\build_example.ps1
```

Batch:

```bat
examples\desfire_auth_changekey\build_example.bat
```

## Run

Example (change app `BADA55` key 0 from AES to a new AES value):

```powershell
.\build\examples\Debug\desfire_auth_changekey_example.exe COM5 `
  --aid BADA55 `
  --auth-mode aes `
  --current-key-type aes `
  --auth-key-no 0 `
  --auth-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --change-key-no 0 `
  --new-key-type aes `
  --new-key-hex "00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF" `
  --new-key-version 0 `
  --confirm-change
```

Example (PICC key type migration DES/2K3DES family -> AES):

```powershell
.\build\examples\Debug\desfire_auth_changekey_example.exe COM5 `
  --aid 000000 `
  --auth-mode iso `
  --current-key-type 2k3des `
  --auth-key-no 0 `
  --auth-key-hex "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00" `
  --change-key-no 0 `
  --new-key-type aes `
  --new-key-hex "11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF 00" `
  --new-key-version 0 `
  --confirm-change
```

If `--confirm-change` is omitted, the tool authenticates and exits without sending `ChangeKey`.

## Notes

- `--auth-mode` accepts `legacy|iso|aes` and aliases `des|2k3des|3k3des`.
- `--current-key-type` is optional but recommended to avoid ambiguity.
- For changing a different key slot than the authenticated key slot, provide `--old-key-hex` (and optionally `--old-key-type`).
- Application key family is fixed at `CreateApplication` time:
  - DES/2K3DES family
  - 3K3DES family
  - AES family
- So app-level `ChangeKey` can rotate key bytes/version, but cannot switch family (for example AES -> 2K3DES). Use delete + recreate app for family changes.
- In PowerShell, quote spaced hex values (`"11 22 ..."`), otherwise each byte is parsed as a separate argument.
- Be careful: `ChangeKey` permanently updates card keys.
