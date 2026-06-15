# Security Hotspots Plan

This plan tracks review and cleanup of all current SonarCloud security
hotspots for the project.

Inventory source: SonarCloud project `LegalizeAdulthood_iterated-dynamics`,
status `TO_REVIEW`, refreshed 2026-06-14.

Total hotspots: 20.

Review one work item at a time. Each `##` heading is one Sonar key and one
call site.

# Review Policy

Work in small slices. Each work item removes one hotspot or documents why
that hotspot is safe and ready for Sonar review.

Prefer eliminating raw C buffer ownership where a local `std::string`,
`std::array`, `std::string_view`, `fmt`, or parser abstraction makes the
contract simpler.

Do not mechanically replace one C API with another. The replacement must
make the size contract explicit and testable. Avoid swapping `strcpy` for
`strncpy` as the terminal fix.

Give production code priority over tests, but keep test-only hotspots in
the plan so final review reaches zero unreviewed items.

# Progress Rules

Status values are `pending`, `done`, or `reviewed`.

The next work item is the first `pending` item in probability order.

Within a probability bucket, production code comes before tests, legacy
code, Win32 support, and workflows.

If one helper change removes multiple hotspots, implement it while the
current item is active. Then mark each affected item separately.

# High Probability

Items: 4.

## SH-169

- Status: `pending`
- Key: `AZwwyFQy0qvqV_CtK2Bp`
- Rule: `cpp:S5813`
- Area: `math-number-format`
- Path: `libid/math/bigflt.cpp`
- Line: `74`
- Range: `74:16-74:27`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2019-03-17T21:07:38+0000`
- Updated: `2026-02-06T02:20:33+0000`
- Flows: `0`
- Message: Make sure use of "strlen" is safe here.

## SH-101

- Status: `pending`
- Key: `AZwwyE_Y0qvqV_CtK1hf`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/select_video_mode.cpp`
- Line: `52`
- Range: `52:4-52:16`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2024-04-24T01:46:17+0000`
- Updated: `2026-02-06T02:20:33+0000`
- Flows: `0`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-239

- Status: `pending`
- Key: `AZwwyFWt0qvqV_CtK2KF`
- Rule: `cpp:S6069`
- Area: `tests-legacy-win32`
- Path: `win32/create_minidump.cpp`
- Line: `83`
- Range: `83:8-83:20`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2019-03-18T14:49:28+0000`
- Updated: `2026-02-06T02:20:33+0000`
- Flows: `0`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-240

- Status: `pending`
- Key: `AZwwyFXQ0qvqV_CtK2Ku`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `win32/Win32BaseDriver.cpp`
- Line: `514`
- Range: `514:4-514:15`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2025-02-22T03:06:58+0000`
- Updated: `2026-02-06T02:20:33+0000`
- Flows: `0`
- Message: Make sure use of "strcpy" is safe here.

# Medium Probability

Items: 11.

## SH-242

- Status: `pending`
- Key: `AZwwyEeI0qvqV_CtK0_n`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `libid/io/gifview.cpp`
- Line: `375`
- Range: `375:20-375:29`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2025-01-06T07:40:46+0000`
- Updated: `2026-02-06T02:20:33+0000`
- Flows: `0`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-243

- Status: `pending`
- Key: `AZwwyE850qvqV_CtK1ds`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `libid/ui/evolve.cpp`
- Line: `447`
- Range: `447:55-447:64`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2024-12-28T01:51:45+0000`
- Updated: `2026-02-06T02:20:33+0000`
- Flows: `0`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-244

- Status: `pending`
- Key: `AZwwyE850qvqV_CtK1d7`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `libid/ui/evolve.cpp`
- Line: `591`
- Range: `591:55-591:64`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2024-12-28T01:51:45+0000`
- Updated: `2026-02-06T02:20:33+0000`
- Flows: `0`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-245

- Status: `pending`
- Key: `AZwwyE850qvqV_CtK1eE`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `libid/ui/evolve.cpp`
- Line: `886`
- Range: `886:33-886:42`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2025-09-17T05:06:21+0000`
- Updated: `2026-02-06T02:20:33+0000`
- Flows: `0`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-246

- Status: `pending`
- Key: `AZwwyE850qvqV_CtK1eH`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `libid/ui/evolve.cpp`
- Line: `901`
- Range: `901:12-901:21`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2024-05-19T05:52:47+0000`
- Updated: `2026-02-06T02:20:33+0000`
- Flows: `0`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-247

- Status: `pending`
- Key: `AZwwyE9F0qvqV_CtK1eP`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `libid/ui/intro.cpp`
- Line: `85`
- Range: `85:8-85:17`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2025-01-07T06:22:49+0000`
- Updated: `2026-02-06T02:20:33+0000`
- Flows: `0`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-248

- Status: `pending`
- Key: `AZ6oIFYN1A3VsDXBVZqk`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `libid/ui/rotate.cpp`
- Line: `49`
- Range: `49:11-49:20`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2026-06-08T16:39:41+0000`
- Updated: `2026-06-08T16:39:41+0000`
- Flows: `0`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-249

- Status: `pending`
- Key: `AZ6oIFbW1A3VsDXBVZqm`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `libid/ui/starfield.cpp`
- Line: `45`
- Range: `45:11-45:20`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2026-06-08T16:39:41+0000`
- Updated: `2026-06-08T16:39:41+0000`
- Flows: `0`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-250

- Status: `pending`
- Key: `AZ6oIF0a1A3VsDXBVZq6`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `tests/libid/engine/test_random_seed.cpp`
- Line: `96`
- Range: `96:22-96:31`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2026-06-08T16:39:41+0000`
- Updated: `2026-06-08T16:39:41+0000`
- Flows: `0`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-251

- Status: `pending`
- Key: `AZ6oIF0a1A3VsDXBVZq8`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `tests/libid/engine/test_random_seed.cpp`
- Line: `97`
- Range: `97:22-97:31`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2026-06-08T16:39:41+0000`
- Updated: `2026-06-08T16:39:41+0000`
- Flows: `0`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-252

- Status: `pending`
- Key: `AZ6oIF0a1A3VsDXBVZq-`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `tests/libid/engine/test_random_seed.cpp`
- Line: `102`
- Range: `102:22-102:31`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2026-06-08T16:39:41+0000`
- Updated: `2026-06-08T16:39:41+0000`
- Flows: `0`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

# Low Probability

Items: 5.

## SH-253

- Status: `pending`
- Key: `AZwwyFO60qvqV_CtK1_C`
- Rule: `cpp:S5443`
- Area: `command-input`
- Path: `libid/engine/cmdfiles.cpp`
- Line: `459`
- Range: `459:27-459:32`
- Category: `others`
- Probability: `LOW`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2025-02-16T03:12:12+0000`
- Updated: `2026-05-26T15:49:52+0000`
- Flows: `0`
- Message: Make sure publicly writable directories (accessed through an
  environment variable) are used safely here

## SH-254

- Status: `pending`
- Key: `AZwwyFO60qvqV_CtK1_D`
- Rule: `cpp:S5443`
- Area: `command-input`
- Path: `libid/engine/cmdfiles.cpp`
- Line: `462`
- Range: `462:19-462:25`
- Category: `others`
- Probability: `LOW`
- Sonar Status: `TO_REVIEW`
- Created: `2004-11-23T13:13:37+0000`
- Updated: `2026-05-26T15:49:52+0000`
- Flows: `0`
- Message: Make sure publicly writable directories (accessed through an
  environment variable) are used safely here

## SH-263

- Status: `pending`
- Key: `AZ6v2Kk0G00uNmChByDo`
- Rule: `githubactions:S7637`
- Area: `github-workflows`
- Path: `.github/workflows/build.yml`
- Line: `29`
- Range: `29:12-29:30`
- Category: `others`
- Probability: `LOW`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2026-06-10T03:51:36+0000`
- Updated: `2026-06-10T04:43:35+0000`
- Flows: `0`
- Message: Use full commit SHA hash for this dependency.

## SH-264

- Status: `pending`
- Key: `AZwwyFe10qvqV_CtK2O-`
- Rule: `githubactions:S7637`
- Area: `github-workflows`
- Path: `.github/workflows/build.yml`
- Line: `144`
- Range: `144:12-144:30`
- Category: `others`
- Probability: `LOW`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2025-10-26T15:45:58+0000`
- Updated: `2026-02-06T02:20:33+0000`
- Flows: `0`
- Message: Use full commit SHA hash for this dependency.

## SH-271

- Status: `pending`
- Key: `AZ6kqmM7OMiYihiGkbUd`
- Rule: `githubactions:S7637`
- Area: `github-workflows`
- Path: `.github/workflows/docs.yml`
- Line: `56`
- Range: `56:12-56:30`
- Category: `others`
- Probability: `LOW`
- Sonar Status: `TO_REVIEW`
- Assignee: `AWqWD8MuvoFfNZkNexwB`
- Created: `2026-06-07T21:23:26+0000`
- Updated: `2026-06-08T00:31:54+0000`
- Flows: `0`
- Message: Use full commit SHA hash for this dependency.
