You can still run the legacy Authenticate (0x0A) on an EV2 card—the chips stay backward‑compatible. That command gives you exactly what it did on EV1:

- A 16‑byte session key built from RndA‖RndB (for DES/2K3DES it’s RndA[0..3]‖RndB[0..3]‖RndA[4..7]‖RndB[4..7], parity bits cleared).
- An encrypted channel that works with the “DES send/receive mode” commands you already have (read/write data, select applications, etc.).

What it doesn’t give you is the extra EV2 session material: the transaction identifier, command counter, and the AES‑based ENC/MAC keys. Without those, EV2 won’t accept operations that depend on the new integrity rules—most notably ChangeKey on the same slot. The card will happily let you read files, write files, change key settings, etc., as long as you stay within the legacy feature set. But once you hit commands where EV2 tightened up the integrity checks (ChangeKey, secure messaging, transaction MACs), you need the EV2 authentication (0x71/0x77) to derive the new session keys; otherwise the card returns 91 1E (“integrity error”) like you keep seeing.

So: legacy 0x0A still works for EV1‑era tasks, but it’s not enough for the EV2‑specific flows. That’s why ChangeKey is failing until we add AuthenticateEV2First.