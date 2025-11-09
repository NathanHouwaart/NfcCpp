
```
MIFARE DESFire Authentication ISO (0x1A) Flow
DES Key: [00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00]
═══════════════════════════════════════════════════════════════════════════

READER                                                                CARD
  │                                                                     │
  │  (1) Authenticate ISO Command (0x1A)                                │
  ├────────────────────────────────────────────────────────────────────>│
  │                                                                     │
  │  (2) RndB-Encrypted: 84 76 D1 CF 30 24 B7 C7                        │
  │<────────────────────────────────────────────────────────────────────┤
  │                                                                     │
  │  (3) DECRYPT RndB-Encrypted                                         │
  │      ┌─────────────────────────────────────────┐                    │
  │      │ Algorithm: TDES-CBC-NOPADDING           │                    │
  │      │ Key: 00 00 00 00 00 00 00 00            │                    │
  │      │      00 00 00 00 00 00 00 00            │                    │
  │      │ IV:  00 00 00 00 00 00 00 00            │                    │
  │      │ Result:                                 │                    │
  │      │ RndB = 3B 3D FE 62 81 64 BF DA          │                    │
  │      └─────────────────────────────────────────┘                    │
  │                                                                     │
  │  (4) UPDATE IV                                                      │
  │      IV = Last 8 bytes of RndB-Encrypted                            │
  │      IV = 84 76 D1 CF 30 24 B7 C7                                   │
  │                                                                     │
  │  (5) ROTATE RndB Right by 1                                         │
  │      RndB-Rotated = 3D FE 62 81 64 BF DA 3B                         │
  │                                                                     │
  │  (6) GENERATE Random RndA (8 bytes)                                 │
  │      RndA = 49 EC 63 DE CD E0 07 72                                 │
  │                                                                     │
  │  (7) CONCATENATE RndA + RndB-Rotated                                │
  │      RndAB = 49 EC 63 DE CD E0 07 72                                │
  │              3D FE 62 81 64 BF DA 3B                                │
  │                                                                     │
  │  (8) ENCRYPT RndAB                                                  │
  │      ┌─────────────────────────────────────────┐                    │
  │      │ Algorithm: TDES-CBC-NOPADDING           │                    │
  │      │ Key: 00 00 00 00 00 00 00 00            │                    │
  │      │      00 00 00 00 00 00 00 00            │                    │
  │      │ IV:  84 76 D1 CF 30 24 B7 C7            │                    │
  │      │ Result:                                 │                    │
  │      │ RndAB-Enc = DA C6 7A B7 43 76 3D C9     │                    │
  │      │             FA F8 A0 AE 50 4E 80 C5     │                    │
  │      └─────────────────────────────────────────┘                    │
  │                                                                     │
  │  (9) UPDATE IV                                                      │
  │      IV = Last 8 bytes of RndAB-Encrypted                           │
  │      IV = FA F8 A0 AE 50 4E 80 C5                                   │
  │                                                                     │
  │  (10) Send RndAB-Encrypted                                          │
  │       DA C6 7A B7 43 76 3D C9 FA F8 A0 AE 50 4E 80 C5               │
  ├────────────────────────────────────────────────────────────────────>│
  │                                                                     │
  │  (11) RndA-Encoded: 13 E9 E4 FA 43 88 BF 16                         │
  │<────────────────────────────────────────────────────────────────────┤
  │                                                                     │
  │  (12) DECRYPT RndA-Encoded                                          │
  │      ┌─────────────────────────────────────────┐                    │
  │      │ Algorithm: TDES-CBC-NOPADDING           │                    │
  │      │ Key: 00 00 00 00 00 00 00 00            │                    │
  │      │      00 00 00 00 00 00 00 00            │                    │
  │      │ IV:  FA F8 A0 AE 50 4E 80 C5            │                    │
  │      │ Result:                                 │                    │
  │      │ RndA-Dec-Rot = EC 63 DE CD E0 07 72 49  │                    │
  │      └─────────────────────────────────────────┘                    │
  │                                                                     │
  │  (13) ROTATE Right by 1 to get Original RndA                        │
  │       RndA-Orig = 49 EC 63 DE CD E0 07 72                           │
  │                                                                     │
  │  (14) VERIFY AUTHENTICATION                                         │
  │      ┌─────────────────────────────────────────┐                    │
  │      │ Check: RndA-Orig == RndA Generated?     │                    │
  │      │  Match = Authentication Success         │                    │
  │      │  No Match = Authentication Error        │                    │
  │      └─────────────────────────────────────────┘                    │
  │                                                                     │
  │  (15) GENERATE SESSION KEY                                          │
  │      ┌─────────────────────────────────────────┐                    │
  │      │ Sess-Key Raw:                           │                    │
  │      │ RndA[0..3] || RndB[0..3] ||             │                    │
  │      │ RndA[0..3] || RndB[0..3]                │                    │
  │      │ = 49 EC 63 DE 3B 3D FE 62               │                    │
  │      │   49 EC 63 DE 3B 3D FE 62               │                    │
  │      │                                         │                    │
  │      │ Sess-Key Versioned (LSB → 0):           │                    │
  │      │ = 48 EC 62 DE 3A 3C FE 62               │                    │
  │      │   48 EC 62 DE 3A 3C FE 62               │                    │
  │      └─────────────────────────────────────────┘                    │
  │                                                                     │
═══════════════════════════════════════════════════════════════════════════
                        ✓ AUTHENTICATION COMPLETE
═══════════════════════════════════════════════════════════════════════════
```