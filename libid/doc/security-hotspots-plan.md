# Security Hotspots Plan

This plan tracks review and cleanup of all SonarCloud security hotspots
for the project.

Inventory source: SonarCloud project `LegalizeAdulthood_iterated-dynamics`,
status `TO_REVIEW`.

Total hotspots: 264.

Review one work item at a time.  Each `##` heading is one Sonar key
and one call site.

# Review Policy

Work in small slices.  Each work item removes one hotspot or documents
why that hotspot is safe and ready for Sonar review.

Prefer eliminating raw C buffer ownership where a local `std::string`,
`std::array`, `std::string_view`, `fmt`, or parser abstraction makes the
contract simpler.

Do not mechanically replace one C API with another.  The replacement must
make the size contract explicit and testable.  Avoid swapping `strcpy` for
`strncpy` as the terminal fix.

Give production code priority over tests, but keep test-only hotspots in
the plan so final review reaches zero unreviewed items.

# Progress Rules

Status values are `pending`, `done`, or `reviewed`.

The next work item is the first `pending` item in probability order.

Within a probability bucket, production code comes before tests, legacy
code, Win32 support, and workflows.

If one helper change removes multiple hotspots, implement it while the
current item is active.  Then mark each affected item separately.

# High Probability

Items: 231.

## SH-009

- Status: `pending`
- Key: `AZwwyFO60qvqV_CtK1-8`
- Rule: `cpp:S5816`
- Area: `command-input`
- Path: `libid/engine/cmdfiles.cpp`
- Line: `4324`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strncpy" is safe here.

## SH-010

- Status: `pending`
- Key: `AZwwyFO60qvqV_CtK1-_`
- Rule: `cpp:S5814`
- Area: `command-input`
- Path: `libid/engine/cmdfiles.cpp`
- Line: `4329`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-011

- Status: `pending`
- Key: `AZwwyE260qvqV_CtK1Si`
- Rule: `cpp:S5801`
- Area: `command-input`
- Path: `libid/fractals/lsystem.cpp`
- Line: `144`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-012

- Status: `pending`
- Key: `AZwwyE260qvqV_CtK1Su`
- Rule: `cpp:S5814`
- Area: `command-input`
- Path: `libid/fractals/lsystem.cpp`
- Line: `195`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-013

- Status: `pending`
- Key: `AZwwyE260qvqV_CtK1Sv`
- Rule: `cpp:S5813`
- Area: `command-input`
- Path: `libid/fractals/lsystem.cpp`
- Line: `210`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-014

- Status: `pending`
- Key: `AZwwyE260qvqV_CtK1S1`
- Rule: `cpp:S5801`
- Area: `command-input`
- Path: `libid/fractals/lsystem.cpp`
- Line: `246`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-015

- Status: `pending`
- Key: `AZwwyE260qvqV_CtK1S3`
- Rule: `cpp:S5814`
- Area: `command-input`
- Path: `libid/fractals/lsystem.cpp`
- Line: `249`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-016

- Status: `pending`
- Key: `AZwwyE260qvqV_CtK1S7`
- Rule: `cpp:S5813`
- Area: `command-input`
- Path: `libid/fractals/lsystem.cpp`
- Line: `293`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-017

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1Uh`
- Rule: `cpp:S5813`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `762`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-018

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1Uv`
- Rule: `cpp:S5813`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `868`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-019

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1VK`
- Rule: `cpp:S5814`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `1532`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-020

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1VI`
- Rule: `cpp:S5814`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `1534`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-021

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1VJ`
- Rule: `cpp:S5814`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `1535`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-022

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1VN`
- Rule: `cpp:S5814`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `1544`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-023

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1VL`
- Rule: `cpp:S5814`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `1546`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-024

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1VM`
- Rule: `cpp:S5814`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `1547`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-025

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1Vk`
- Rule: `cpp:S5813`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2207`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-026

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1Vi`
- Rule: `cpp:S5801`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2209`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-027

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1Vj`
- Rule: `cpp:S5814`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2210`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-028

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1Vt`
- Rule: `cpp:S5801`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2527`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-029

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1Vy`
- Rule: `cpp:S5814`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2549`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-030

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1Vz`
- Rule: `cpp:S5813`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2553`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-031

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1V0`
- Rule: `cpp:S5813`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2565`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-032

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1V1`
- Rule: `cpp:S5813`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2572`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-033

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1V2`
- Rule: `cpp:S5813`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2592`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-034

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1V4`
- Rule: `cpp:S5813`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2602`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-035

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1V5`
- Rule: `cpp:S5814`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2605`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-036

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1V6`
- Rule: `cpp:S5813`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2609`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-037

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1Vw`
- Rule: `cpp:S5814`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2613`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-038

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1V7`
- Rule: `cpp:S5813`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2614`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-039

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1V8`
- Rule: `cpp:S5814`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2617`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-040

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1V9`
- Rule: `cpp:S5813`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2623`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-041

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1V-`
- Rule: `cpp:S5814`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2625`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-042

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1Vx`
- Rule: `cpp:S5814`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2627`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-043

- Status: `pending`
- Key: `AZwwyE4I0qvqV_CtK1Wf`
- Rule: `cpp:S5813`
- Area: `command-input`
- Path: `libid/fractals/parser.cpp`
- Line: `2683`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-044

- Status: `pending`
- Key: `AZwwyEjM0qvqV_CtK1FR`
- Rule: `cpp:S5813`
- Area: `command-input`
- Path: `libid/io/load_config.cpp`
- Line: `75`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-045

- Status: `pending`
- Key: `AZwwyEjM0qvqV_CtK1FO`
- Rule: `cpp:S5816`
- Area: `command-input`
- Path: `libid/io/load_config.cpp`
- Line: `118`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strncpy" is safe here.

## SH-046

- Status: `pending`
- Key: `AZwwyEo10qvqV_CtK1IV`
- Rule: `cpp:S5816`
- Area: `ui-prompt-menu`
- Path: `libid/include/ui/ChoiceBuilder.h`
- Line: `99`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strncpy" is safe here.

## SH-048

- Status: `pending`
- Key: `AZwwyFAU0qvqV_CtK1k0`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/file_get_window.cpp`
- Line: `384`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-049

- Status: `pending`
- Key: `AZwwyFAU0qvqV_CtK1k1`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/file_get_window.cpp`
- Line: `385`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-050

- Status: `pending`
- Key: `AZwwyFA20qvqV_CtK1lW`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/full_screen_choice.cpp`
- Line: `73`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-051

- Status: `pending`
- Key: `AZwwyFA20qvqV_CtK1lY`
- Rule: `cpp:S5813`
- Area: `ui-prompt-menu`
- Path: `libid/ui/full_screen_choice.cpp`
- Line: `74`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-052

- Status: `pending`
- Key: `AZwwyFA20qvqV_CtK1lZ`
- Rule: `cpp:S5813`
- Area: `ui-prompt-menu`
- Path: `libid/ui/full_screen_choice.cpp`
- Line: `82`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-053

- Status: `pending`
- Key: `AZwwyFA20qvqV_CtK1ld`
- Rule: `cpp:S5813`
- Area: `ui-prompt-menu`
- Path: `libid/ui/full_screen_choice.cpp`
- Line: `97`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-054

- Status: `pending`
- Key: `AZwwyFA20qvqV_CtK1ls`
- Rule: `cpp:S5813`
- Area: `ui-prompt-menu`
- Path: `libid/ui/full_screen_choice.cpp`
- Line: `183`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-055

- Status: `pending`
- Key: `AZwwyFA20qvqV_CtK1lw`
- Rule: `cpp:S5813`
- Area: `ui-prompt-menu`
- Path: `libid/ui/full_screen_choice.cpp`
- Line: `255`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-056

- Status: `pending`
- Key: `AZwwyFBo0qvqV_CtK1m5`
- Rule: `cpp:S5813`
- Area: `ui-prompt-menu`
- Path: `libid/ui/full_screen_prompt.cpp`
- Line: `371`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-057

- Status: `pending`
- Key: `AZwwyFBo0qvqV_CtK1m7`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/full_screen_prompt.cpp`
- Line: `433`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-058

- Status: `pending`
- Key: `AZwwyFBo0qvqV_CtK1m9`
- Rule: `cpp:S5813`
- Area: `ui-prompt-menu`
- Path: `libid/ui/full_screen_prompt.cpp`
- Line: `442`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-059

- Status: `pending`
- Key: `AZwwyFBo0qvqV_CtK1m-`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/full_screen_prompt.cpp`
- Line: `451`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-060

- Status: `pending`
- Key: `AZwwyFBo0qvqV_CtK1m_`
- Rule: `cpp:S5813`
- Area: `ui-prompt-menu`
- Path: `libid/ui/full_screen_prompt.cpp`
- Line: `454`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-061

- Status: `pending`
- Key: `AZwwyFBo0qvqV_CtK1nm`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/full_screen_prompt.cpp`
- Line: `1073`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-062

- Status: `pending`
- Key: `AZwwyFBo0qvqV_CtK1no`
- Rule: `cpp:S5813`
- Area: `ui-prompt-menu`
- Path: `libid/ui/full_screen_prompt.cpp`
- Line: `1075`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-063

- Status: `pending`
- Key: `AZwwyFBo0qvqV_CtK1nr`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/full_screen_prompt.cpp`
- Line: `1131`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-064

- Status: `pending`
- Key: `AZwwyFFl0qvqV_CtK1ui`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_3d_params.cpp`
- Line: `115`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-065

- Status: `pending`
- Key: `AZwwyFFl0qvqV_CtK1v2`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_3d_params.cpp`
- Line: `473`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-066

- Status: `pending`
- Key: `AZwwyFED0qvqV_CtK1rN`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_file_entry.cpp`
- Line: `314`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-067

- Status: `pending`
- Key: `AZwwyFED0qvqV_CtK1rU`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_file_entry.cpp`
- Line: `317`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-068

- Status: `pending`
- Key: `AZwwyFED0qvqV_CtK1rV`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_file_entry.cpp`
- Line: `322`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-069

- Status: `pending`
- Key: `AZwwyFED0qvqV_CtK1rO`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_file_entry.cpp`
- Line: `325`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-070

- Status: `pending`
- Key: `AZwwyE-U0qvqV_CtK1f-`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_fract_type.cpp`
- Line: `158`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-071

- Status: `pending`
- Key: `AZwwyE-U0qvqV_CtK1gy`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_fract_type.cpp`
- Line: `828`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-072

- Status: `pending`
- Key: `AZwwyE8P0qvqV_CtK1bo`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_rds_params.cpp`
- Line: `85`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-073

- Status: `pending`
- Key: `AZ58sSjgxhoxqMM9Z1TY`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_rds_params.cpp`
- Line: `86`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-074

- Status: `pending`
- Key: `AZwwyE8P0qvqV_CtK1bq`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_rds_params.cpp`
- Line: `87`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-075

- Status: `pending`
- Key: `AZwwyFDj0qvqV_CtK1p0`
- Rule: `cpp:S5816`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_toggles.cpp`
- Line: `187`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strncpy" is safe here.

## SH-076

- Status: `pending`
- Key: `AZwwyFDj0qvqV_CtK1p8`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_toggles.cpp`
- Line: `228`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-077

- Status: `pending`
- Key: `AZwwyFDt0qvqV_CtK1qX`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_toggles2.cpp`
- Line: `89`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-078

- Status: `pending`
- Key: `AZwwyFDt0qvqV_CtK1qY`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_toggles2.cpp`
- Line: `96`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-079

- Status: `pending`
- Key: `AZwwyFB10qvqV_CtK1n7`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_video_mode.cpp`
- Line: `530`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-080

- Status: `pending`
- Key: `AZwwyFB10qvqV_CtK1n8`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_video_mode.cpp`
- Line: `534`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-081

- Status: `pending`
- Key: `AZwwyFB10qvqV_CtK1n9`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_video_mode.cpp`
- Line: `538`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-082

- Status: `pending`
- Key: `AZwwyFB10qvqV_CtK1n-`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_video_mode.cpp`
- Line: `542`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-083

- Status: `pending`
- Key: `AZwwyFB10qvqV_CtK1n_`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_video_mode.cpp`
- Line: `546`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-084

- Status: `pending`
- Key: `AZwwyFB10qvqV_CtK1oA`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_video_mode.cpp`
- Line: `550`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-085

- Status: `pending`
- Key: `AZwwyFB10qvqV_CtK1oB`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/get_video_mode.cpp`
- Line: `554`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-086

- Status: `pending`
- Key: `AZwwyFFC0qvqV_CtK1s5`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/make_batch_file.cpp`
- Line: `144`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-087

- Status: `pending`
- Key: `AZwwyFFC0qvqV_CtK1s7`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/make_batch_file.cpp`
- Line: `191`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-088

- Status: `pending`
- Key: `AZwwyFFC0qvqV_CtK1s8`
- Rule: `cpp:S5816`
- Area: `ui-prompt-menu`
- Path: `libid/ui/make_batch_file.cpp`
- Line: `206`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strncpy" is safe here.

## SH-089

- Status: `pending`
- Key: `AZwwyFFC0qvqV_CtK1s9`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/make_batch_file.cpp`
- Line: `210`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-090

- Status: `pending`
- Key: `AZwwyFFC0qvqV_CtK1s6`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/make_batch_file.cpp`
- Line: `211`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-091

- Status: `pending`
- Key: `AZwwyFFC0qvqV_CtK1s-`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/make_batch_file.cpp`
- Line: `214`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-092

- Status: `pending`
- Key: `AZwwyFFC0qvqV_CtK1s_`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/make_batch_file.cpp`
- Line: `219`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-093

- Status: `pending`
- Key: `AZwwyFFC0qvqV_CtK1tO`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/make_batch_file.cpp`
- Line: `364`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-094

- Status: `pending`
- Key: `AZwwyFFC0qvqV_CtK1tP`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/make_batch_file.cpp`
- Line: `368`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-095

- Status: `pending`
- Key: `AZwwyFFC0qvqV_CtK1tW`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/make_batch_file.cpp`
- Line: `484`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-096

- Status: `pending`
- Key: `AZwwyFFC0qvqV_CtK1uF`
- Rule: `cpp:S5813`
- Area: `ui-prompt-menu`
- Path: `libid/ui/make_batch_file.cpp`
- Line: `1817`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-097

- Status: `pending`
- Key: `AZwwyFFC0qvqV_CtK1uG`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/make_batch_file.cpp`
- Line: `1825`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-098

- Status: `pending`
- Key: `AZwwyFFC0qvqV_CtK1uJ`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/make_batch_file.cpp`
- Line: `1844`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-099

- Status: `pending`
- Key: `AZwwyFFC0qvqV_CtK1uL`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/make_batch_file.cpp`
- Line: `1848`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-100

- Status: `pending`
- Key: `AZwwyFAE0qvqV_CtK1kQ`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/rotate.cpp`
- Line: `566`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-101

- Status: `pending`
- Key: `AZwwyE_Y0qvqV_CtK1hf`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/select_video_mode.cpp`
- Line: `52`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-102

- Status: `pending`
- Key: `AZwwyE8o0qvqV_CtK1cL`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/slideshw.cpp`
- Line: `138`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-103

- Status: `pending`
- Key: `AZwwyE8o0qvqV_CtK1cY`
- Rule: `cpp:S5813`
- Area: `ui-prompt-menu`
- Path: `libid/ui/slideshw.cpp`
- Line: `316`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-104

- Status: `pending`
- Key: `AZwwyE8o0qvqV_CtK1ca`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/slideshw.cpp`
- Line: `333`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-105

- Status: `pending`
- Key: `AZwwyE8o0qvqV_CtK1cm`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/slideshw.cpp`
- Line: `502`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-106

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1v9`
- Rule: `cpp:S5813`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `82`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-107

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1wA`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `150`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-108

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1wU`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `244`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-109

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1wh`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `376`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-110

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1wk`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `402`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-111

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1wm`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `420`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-112

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1wo`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `438`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-113

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1wq`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `446`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-114

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1ws`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `452`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-115

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1wx`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `460`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-116

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1wy`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `463`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-117

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1wz`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `468`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-118

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1w0`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `471`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-119

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1w1`
- Rule: `cpp:S5816`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `478`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strncpy" is safe here.

## SH-120

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1w2`
- Rule: `cpp:S5816`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `485`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strncpy" is safe here.

## SH-121

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1w4`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `492`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-122

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1w8`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `505`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-123

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xF`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `544`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-124

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xG`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `547`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-125

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xH`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `550`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-126

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xI`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `553`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-127

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xU`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `562`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-128

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xV`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `565`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-129

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xY`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `571`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-130

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xZ`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `576`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-131

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xa`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `579`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-132

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xb`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `582`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-133

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xc`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `585`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-134

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xd`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `588`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-135

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xg`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `614`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-136

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xi`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `618`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-137

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xk`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `622`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-138

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xm`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `628`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-139

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xo`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `632`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-140

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xp`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `641`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-141

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xq`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `644`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-142

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xt`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `650`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-143

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xy`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `657`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-144

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1xz`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `660`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-145

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1x0`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `663`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-146

- Status: `pending`
- Key: `AZwwyFF00qvqV_CtK1x5`
- Rule: `cpp:S6069`
- Area: `ui-prompt-menu`
- Path: `libid/ui/tab_display.cpp`
- Line: `728`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-147

- Status: `pending`
- Key: `AZwwyFAe0qvqV_CtK1lH`
- Rule: `cpp:S5816`
- Area: `ui-prompt-menu`
- Path: `libid/ui/temp_msg.cpp`
- Line: `52`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strncpy" is safe here.

## SH-148

- Status: `pending`
- Key: `AZwwyFAe0qvqV_CtK1lK`
- Rule: `cpp:S5813`
- Area: `ui-prompt-menu`
- Path: `libid/ui/temp_msg.cpp`
- Line: `68`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-149

- Status: `pending`
- Key: `AZwwyE_D0qvqV_CtK1hV`
- Rule: `cpp:S5801`
- Area: `ui-prompt-menu`
- Path: `libid/ui/thinking.cpp`
- Line: `47`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-150

- Status: `pending`
- Key: `AZwwyE_D0qvqV_CtK1hW`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/thinking.cpp`
- Line: `48`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-151

- Status: `pending`
- Key: `AZwwyE_D0qvqV_CtK1hX`
- Rule: `cpp:S5814`
- Area: `ui-prompt-menu`
- Path: `libid/ui/thinking.cpp`
- Line: `49`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-152

- Status: `pending`
- Key: `AZwwyEdC0qvqV_CtK0-D`
- Rule: `cpp:S5816`
- Area: `io-path-metadata`
- Path: `libid/io/decode_info.cpp`
- Line: `61`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strncpy" is safe here.

## SH-153

- Status: `pending`
- Key: `AZwwyEdC0qvqV_CtK0-E`
- Rule: `cpp:S5816`
- Area: `io-path-metadata`
- Path: `libid/io/decode_info.cpp`
- Line: `65`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strncpy" is safe here.

## SH-154

- Status: `pending`
- Key: `AZwwyEhW0qvqV_CtK1CN`
- Rule: `cpp:S5801`
- Area: `io-path-metadata`
- Path: `libid/io/encoder.cpp`
- Line: `733`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-155

- Status: `pending`
- Key: `AZwwyEhW0qvqV_CtK1CR`
- Rule: `cpp:S6069`
- Area: `io-path-metadata`
- Path: `libid/io/encoder.cpp`
- Line: `734`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-156

- Status: `pending`
- Key: `AZwwyEhW0qvqV_CtK1CS`
- Rule: `cpp:S5816`
- Area: `io-path-metadata`
- Path: `libid/io/encoder.cpp`
- Line: `762`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strncpy" is safe here.

## SH-157

- Status: `pending`
- Key: `AZwwyEhW0qvqV_CtK1CU`
- Rule: `cpp:S5801`
- Area: `io-path-metadata`
- Path: `libid/io/encoder.cpp`
- Line: `796`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-158

- Status: `pending`
- Key: `AZwwyEd80qvqV_CtK0--`
- Rule: `cpp:S5813`
- Area: `io-path-metadata`
- Path: `libid/io/ends_with_slash.cpp`
- Line: `14`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-159

- Status: `pending`
- Key: `AZwwyEjh0qvqV_CtK1Fc`
- Rule: `cpp:S5801`
- Area: `io-path-metadata`
- Path: `libid/io/expand_dirname.cpp`
- Line: `22`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-160

- Status: `pending`
- Key: `AZwwyEjh0qvqV_CtK1Fd`
- Rule: `cpp:S5801`
- Area: `io-path-metadata`
- Path: `libid/io/expand_dirname.cpp`
- Line: `23`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-161

- Status: `pending`
- Key: `AZwwyEjh0qvqV_CtK1Fe`
- Rule: `cpp:S5813`
- Area: `io-path-metadata`
- Path: `libid/io/expand_dirname.cpp`
- Line: `23`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-162

- Status: `pending`
- Key: `AZwwyEiz0qvqV_CtK1FC`
- Rule: `cpp:S5801`
- Area: `io-path-metadata`
- Path: `libid/io/file_item.cpp`
- Line: `175`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-163

- Status: `pending`
- Key: `AZwwyEct0qvqV_CtK09l`
- Rule: `cpp:S5813`
- Area: `io-path-metadata`
- Path: `libid/io/fix_dirname.cpp`
- Line: `18`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-164

- Status: `pending`
- Key: `AZwwyEct0qvqV_CtK09k`
- Rule: `cpp:S5814`
- Area: `io-path-metadata`
- Path: `libid/io/fix_dirname.cpp`
- Line: `28`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-165

- Status: `pending`
- Key: `AZwwyEct0qvqV_CtK09m`
- Rule: `cpp:S5801`
- Area: `io-path-metadata`
- Path: `libid/io/fix_dirname.cpp`
- Line: `34`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-166

- Status: `pending`
- Key: `AZwwyEfi0qvqV_CtK1An`
- Rule: `cpp:S5801`
- Area: `io-path-metadata`
- Path: `libid/io/loadfile.cpp`
- Line: `1379`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-167

- Status: `pending`
- Key: `AZwwyEfi0qvqV_CtK1Ap`
- Rule: `cpp:S5801`
- Area: `io-path-metadata`
- Path: `libid/io/loadfile.cpp`
- Line: `1444`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-168

- Status: `pending`
- Key: `AZwwyEfi0qvqV_CtK1Ag`
- Rule: `cpp:S5801`
- Area: `io-path-metadata`
- Path: `libid/io/loadfile.cpp`
- Line: `1551`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-169

- Status: `pending`
- Key: `AZwwyFQy0qvqV_CtK2Bp`
- Rule: `cpp:S5813`
- Area: `math-number-format`
- Path: `libid/math/bigflt.cpp`
- Line: `74`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-170

- Status: `pending`
- Key: `AZwwyFQy0qvqV_CtK2Bs`
- Rule: `cpp:S5801`
- Area: `math-number-format`
- Path: `libid/math/bigflt.cpp`
- Line: `174`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-171

- Status: `pending`
- Key: `AZwwyFQy0qvqV_CtK2Bu`
- Rule: `cpp:S5801`
- Area: `math-number-format`
- Path: `libid/math/bigflt.cpp`
- Line: `199`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-172

- Status: `pending`
- Key: `AZwwyFQy0qvqV_CtK2Bw`
- Rule: `cpp:S5801`
- Area: `math-number-format`
- Path: `libid/math/bigflt.cpp`
- Line: `216`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-173

- Status: `pending`
- Key: `AZwwyFQy0qvqV_CtK2DE`
- Rule: `cpp:S5801`
- Area: `math-number-format`
- Path: `libid/math/bigflt.cpp`
- Line: `2255`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-174

- Status: `pending`
- Key: `AZwwyFQy0qvqV_CtK2DF`
- Rule: `cpp:S6069`
- Area: `math-number-format`
- Path: `libid/math/bigflt.cpp`
- Line: `2293`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-175

- Status: `pending`
- Key: `AZwwyFQy0qvqV_CtK2DG`
- Rule: `cpp:S5801`
- Area: `math-number-format`
- Path: `libid/math/bigflt.cpp`
- Line: `2305`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-176

- Status: `pending`
- Key: `AZwwyFRR0qvqV_CtK2D7`
- Rule: `cpp:S5813`
- Area: `math-number-format`
- Path: `libid/math/bignum.cpp`
- Line: `268`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-177

- Status: `pending`
- Key: `AZwwyFRR0qvqV_CtK2EA`
- Rule: `cpp:S5801`
- Area: `math-number-format`
- Path: `libid/math/bignum.cpp`
- Line: `382`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-178

- Status: `pending`
- Key: `AZwwyFRR0qvqV_CtK2EB`
- Rule: `cpp:S5813`
- Area: `math-number-format`
- Path: `libid/math/bignum.cpp`
- Line: `383`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-179

- Status: `pending`
- Key: `AZwwyFRk0qvqV_CtK2Ec`
- Rule: `cpp:S6069`
- Area: `math-number-format`
- Path: `libid/math/round_float_double.cpp`
- Line: `14`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-180

- Status: `pending`
- Key: `AZwwyFFY0qvqV_CtK1ue`
- Rule: `cpp:S6069`
- Area: `math-number-format`
- Path: `libid/ui/double_to_string.cpp`
- Line: `17`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-181

- Status: `pending`
- Key: `AZwwyFFY0qvqV_CtK1uf`
- Rule: `cpp:S5813`
- Area: `math-number-format`
- Path: `libid/ui/double_to_string.cpp`
- Line: `18`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-182

- Status: `pending`
- Key: `AZwwyFjq0qvqV_CtK2S0`
- Rule: `cpp:S5813`
- Area: `help-compiler`
- Path: `hc/AsciiDocCompiler.cpp`
- Line: `234`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-183

- Status: `pending`
- Key: `AZwwyFiF0qvqV_CtK2Pw`
- Rule: `cpp:S5813`
- Area: `help-compiler`
- Path: `hc/HelpSource.cpp`
- Line: `272`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-184

- Status: `pending`
- Key: `AZwwyFiF0qvqV_CtK2P7`
- Rule: `cpp:S6069`
- Area: `help-compiler`
- Path: `hc/HelpSource.cpp`
- Line: `549`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-185

- Status: `pending`
- Key: `AZwwyFiF0qvqV_CtK2P9`
- Rule: `cpp:S6069`
- Area: `help-compiler`
- Path: `hc/HelpSource.cpp`
- Line: `554`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-186

- Status: `pending`
- Key: `AZwwyFiF0qvqV_CtK2QF`
- Rule: `cpp:S5813`
- Area: `help-compiler`
- Path: `hc/HelpSource.cpp`
- Line: `602`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-187

- Status: `pending`
- Key: `AZwwyFiF0qvqV_CtK2QG`
- Rule: `cpp:S5813`
- Area: `help-compiler`
- Path: `hc/HelpSource.cpp`
- Line: `650`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-188

- Status: `pending`
- Key: `AZwwyFiF0qvqV_CtK2QH`
- Rule: `cpp:S5813`
- Area: `help-compiler`
- Path: `hc/HelpSource.cpp`
- Line: `652`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-189

- Status: `pending`
- Key: `AZwwyFiF0qvqV_CtK2QI`
- Rule: `cpp:S5813`
- Area: `help-compiler`
- Path: `hc/HelpSource.cpp`
- Line: `688`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-190

- Status: `pending`
- Key: `AZwwyFiF0qvqV_CtK2QJ`
- Rule: `cpp:S5813`
- Area: `help-compiler`
- Path: `hc/HelpSource.cpp`
- Line: `690`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-191

- Status: `pending`
- Key: `AZwwyFiF0qvqV_CtK2QL`
- Rule: `cpp:S6069`
- Area: `help-compiler`
- Path: `hc/HelpSource.cpp`
- Line: `748`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-192

- Status: `pending`
- Key: `AZwwyFiF0qvqV_CtK2QM`
- Rule: `cpp:S5813`
- Area: `help-compiler`
- Path: `hc/HelpSource.cpp`
- Line: `749`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-193

- Status: `pending`
- Key: `AZwwyFiF0qvqV_CtK2QO`
- Rule: `cpp:S5813`
- Area: `help-compiler`
- Path: `hc/HelpSource.cpp`
- Line: `817`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-194

- Status: `pending`
- Key: `AZwwyFiF0qvqV_CtK2QV`
- Rule: `cpp:S5813`
- Area: `help-compiler`
- Path: `hc/HelpSource.cpp`
- Line: `1185`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-195

- Status: `pending`
- Key: `AZwwyFiF0qvqV_CtK2Qp`
- Rule: `cpp:S5813`
- Area: `help-compiler`
- Path: `hc/HelpSource.cpp`
- Line: `1378`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-196

- Status: `pending`
- Key: `AZwwyFiF0qvqV_CtK2Qq`
- Rule: `cpp:S5813`
- Area: `help-compiler`
- Path: `hc/HelpSource.cpp`
- Line: `1446`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-197

- Status: `pending`
- Key: `AZwwyFiF0qvqV_CtK2Qs`
- Rule: `cpp:S5813`
- Area: `help-compiler`
- Path: `hc/HelpSource.cpp`
- Line: `1706`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-198

- Status: `pending`
- Key: `AZwwyFiF0qvqV_CtK2Qt`
- Rule: `cpp:S5813`
- Area: `help-compiler`
- Path: `hc/HelpSource.cpp`
- Line: `1720`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strlen" is safe here.

## SH-199

- Status: `pending`
- Key: `AZwwyFNd0qvqV_CtK18j`
- Rule: `cpp:S5801`
- Area: `engine-core`
- Path: `libid/engine/trig_fns.cpp`
- Line: `116`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-200

- Status: `pending`
- Key: `AZwwyFNd0qvqV_CtK18k`
- Rule: `cpp:S5814`
- Area: `engine-core`
- Path: `libid/engine/trig_fns.cpp`
- Line: `120`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-201

- Status: `pending`
- Key: `AZwwyFNd0qvqV_CtK18l`
- Rule: `cpp:S5814`
- Area: `engine-core`
- Path: `libid/engine/trig_fns.cpp`
- Line: `121`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcat" is safe here.

## SH-203

- Status: `pending`
- Key: `AZwwyFMU0qvqV_CtK16X`
- Rule: `cpp:S6069`
- Area: `engine-core`
- Path: `libid/engine/video_mode.cpp`
- Line: `114`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-204

- Status: `pending`
- Key: `AZwwyFkQ0qvqV_CtK2T0`
- Rule: `c:S5801`
- Area: `tests-legacy-win32`
- Path: `legacy/dos/sound.c`
- Line: `521`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-205

- Status: `pending`
- Key: `AZwwyFkQ0qvqV_CtK2T7`
- Rule: `c:S5801`
- Area: `tests-legacy-win32`
- Path: `legacy/dos/sound.c`
- Line: `633`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-206

- Status: `pending`
- Key: `AZwwyFkQ0qvqV_CtK2T_`
- Rule: `c:S5801`
- Area: `tests-legacy-win32`
- Path: `legacy/dos/sound.c`
- Line: `738`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-207

- Status: `pending`
- Key: `AZwwyFeL0qvqV_CtK2Of`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/engine/test_cmdfiles.cpp`
- Line: `193`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-208

- Status: `pending`
- Key: `AZwwyFef0qvqV_CtK2O4`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/engine/test_lowerize_parameter.cpp`
- Line: `26`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-209

- Status: `pending`
- Key: `AZwwyFef0qvqV_CtK2O5`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/engine/test_lowerize_parameter.cpp`
- Line: `36`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-210

- Status: `pending`
- Key: `AZwwyFaw0qvqV_CtK2Mx`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/io/test_expand_dirname.cpp`
- Line: `49`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-211

- Status: `pending`
- Key: `AZwwyFaw0qvqV_CtK2My`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/io/test_expand_dirname.cpp`
- Line: `50`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-212

- Status: `pending`
- Key: `AZwwyFaw0qvqV_CtK2Mz`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/io/test_expand_dirname.cpp`
- Line: `60`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-213

- Status: `pending`
- Key: `AZwwyFaw0qvqV_CtK2M0`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/io/test_expand_dirname.cpp`
- Line: `69`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-214

- Status: `pending`
- Key: `AZwwyFc00qvqV_CtK2N3`
- Rule: `cpp:S5816`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_ChoiceBuilder.cpp`
- Line: `746`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strncpy" is safe here.

## SH-215

- Status: `pending`
- Key: `AZwwyFc00qvqV_CtK2N4`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_ChoiceBuilder.cpp`
- Line: `761`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-216

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2N8`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `82`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-217

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2N9`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `101`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-218

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2N-`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `110`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-219

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2N_`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `119`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-220

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OA`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `128`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-221

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OB`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `138`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-222

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OC`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `148`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-223

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OD`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `158`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-224

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OE`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `168`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-225

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OF`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `178`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-226

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OG`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `188`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-227

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OH`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `198`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-228

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OI`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `207`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-229

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OJ`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `217`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-230

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OK`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `227`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-231

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OL`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `237`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-232

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OM`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `246`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-233

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2ON`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `255`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-234

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OO`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `264`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-235

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OP`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `273`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-236

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OQ`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `282`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-237

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OR`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `291`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-238

- Status: `pending`
- Key: `AZwwyFc_0qvqV_CtK2OS`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `tests/libid/ui/test_comments.cpp`
- Line: `302`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-239

- Status: `pending`
- Key: `AZwwyFWt0qvqV_CtK2KF`
- Rule: `cpp:S6069`
- Area: `tests-legacy-win32`
- Path: `win32/create_minidump.cpp`
- Line: `83`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "sprintf" function is safe here or replace it
  with a call to "snprintf".

## SH-240

- Status: `pending`
- Key: `AZwwyFXQ0qvqV_CtK2Ku`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `win32/Win32BaseDriver.cpp`
- Line: `514`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here.

## SH-241

- Status: `pending`
- Key: `AZwwyFXF0qvqV_CtK2KS`
- Rule: `cpp:S5801`
- Area: `tests-legacy-win32`
- Path: `win32/WinText.cpp`
- Line: `163`
- Category: `buffer-overflow`
- Probability: `HIGH`
- Message: Make sure use of "strcpy" is safe here. rows are intentionally
  accepted in Sonar with rationale. were not practical.

# Medium Probability

Items: 11.

## SH-242

- Status: `pending`
- Key: `AZwwyEeI0qvqV_CtK0_n`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `libid/io/gifview.cpp`
- Line: `375`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-243

- Status: `pending`
- Key: `AZwwyE850qvqV_CtK1ds`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `libid/ui/evolve.cpp`
- Line: `447`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-244

- Status: `pending`
- Key: `AZwwyE850qvqV_CtK1d7`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `libid/ui/evolve.cpp`
- Line: `591`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-245

- Status: `pending`
- Key: `AZwwyE850qvqV_CtK1eE`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `libid/ui/evolve.cpp`
- Line: `886`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-246

- Status: `pending`
- Key: `AZwwyE850qvqV_CtK1eH`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `libid/ui/evolve.cpp`
- Line: `901`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-247

- Status: `pending`
- Key: `AZwwyE9F0qvqV_CtK1eP`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `libid/ui/intro.cpp`
- Line: `85`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-248

- Status: `pending`
- Key: `AZ6oIFYN1A3VsDXBVZqk`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `libid/ui/rotate.cpp`
- Line: `49`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-249

- Status: `pending`
- Key: `AZ6oIFbW1A3VsDXBVZqm`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `libid/ui/starfield.cpp`
- Line: `45`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-250

- Status: `pending`
- Key: `AZ6oIF0a1A3VsDXBVZq6`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `tests/libid/engine/test_random_seed.cpp`
- Line: `96`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-251

- Status: `pending`
- Key: `AZ6oIF0a1A3VsDXBVZq8`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `tests/libid/engine/test_random_seed.cpp`
- Line: `97`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

## SH-252

- Status: `pending`
- Key: `AZ6oIF0a1A3VsDXBVZq-`
- Rule: `cpp:S2245`
- Area: `randomness`
- Path: `tests/libid/engine/test_random_seed.cpp`
- Line: `102`
- Category: `weak-cryptography`
- Probability: `MEDIUM`
- Message: Make sure that using this pseudorandom number generator "rand"
  is safe here.

# Low Probability

Items: 19.

## SH-253

- Status: `pending`
- Key: `AZwwyFO60qvqV_CtK1_C`
- Rule: `cpp:S5443`
- Area: `command-input`
- Path: `libid/engine/cmdfiles.cpp`
- Line: `425`
- Category: `others`
- Probability: `LOW`
- Message: Make sure publicly writable directories (accessed through an
  environment variable) are used safely here

## SH-254

- Status: `pending`
- Key: `AZwwyFO60qvqV_CtK1_D`
- Rule: `cpp:S5443`
- Area: `command-input`
- Path: `libid/engine/cmdfiles.cpp`
- Line: `428`
- Category: `others`
- Probability: `LOW`
- Message: Make sure publicly writable directories (accessed through an
  environment variable) are used safely here

## SH-255

- Status: `pending`
- Key: `AZwwyFfA0qvqV_CtK2PE`
- Rule: `githubactions:S7636`
- Area: `github-workflows`
- Path: `.github/workflows/analysis.yml`
- Line: `38`
- Category: `others`
- Probability: `LOW`
- Message: Avoid expanding secrets in a run block.

## SH-256

- Status: `pending`
- Key: `AZwwyFfA0qvqV_CtK2PF`
- Rule: `githubactions:S7636`
- Area: `github-workflows`
- Path: `.github/workflows/analysis.yml`
- Line: `40`
- Category: `others`
- Probability: `LOW`
- Message: Avoid expanding secrets in a run block.

## SH-257

- Status: `pending`
- Key: `AZwwyFfA0qvqV_CtK2PG`
- Rule: `githubactions:S7636`
- Area: `github-workflows`
- Path: `.github/workflows/analysis.yml`
- Line: `102`
- Category: `others`
- Probability: `LOW`
- Message: Avoid expanding secrets in a run block.

## SH-258

- Status: `pending`
- Key: `AZwwyFfA0qvqV_CtK2PH`
- Rule: `githubactions:S7636`
- Area: `github-workflows`
- Path: `.github/workflows/analysis.yml`
- Line: `104`
- Category: `others`
- Probability: `LOW`
- Message: Avoid expanding secrets in a run block.

## SH-259

- Status: `pending`
- Key: `AZwwyFfA0qvqV_CtK2PI`
- Rule: `githubactions:S7636`
- Area: `github-workflows`
- Path: `.github/workflows/analysis.yml`
- Line: `182`
- Category: `others`
- Probability: `LOW`
- Message: Avoid expanding secrets in a run block.

## SH-260

- Status: `pending`
- Key: `AZwwyFfA0qvqV_CtK2PJ`
- Rule: `githubactions:S7636`
- Area: `github-workflows`
- Path: `.github/workflows/analysis.yml`
- Line: `184`
- Category: `others`
- Probability: `LOW`
- Message: Avoid expanding secrets in a run block.

## SH-261

- Status: `pending`
- Key: `AZwwyFe10qvqV_CtK2O8`
- Rule: `githubactions:S7636`
- Area: `github-workflows`
- Path: `.github/workflows/build.yml`
- Line: `49`
- Category: `others`
- Probability: `LOW`
- Message: Avoid expanding secrets in a run block.

## SH-262

- Status: `pending`
- Key: `AZwwyFe10qvqV_CtK2O9`
- Rule: `githubactions:S7636`
- Area: `github-workflows`
- Path: `.github/workflows/build.yml`
- Line: `51`
- Category: `others`
- Probability: `LOW`
- Message: Avoid expanding secrets in a run block.

## SH-263

- Status: `pending`
- Key: `AZ6v2Kk0G00uNmChByDo`
- Rule: `githubactions:S7637`
- Area: `github-workflows`
- Path: `.github/workflows/build.yml`
- Line: `55`
- Category: `others`
- Probability: `LOW`
- Message: Use full commit SHA hash for this dependency.

## SH-264

- Status: `pending`
- Key: `AZwwyFe10qvqV_CtK2O-`
- Rule: `githubactions:S7637`
- Area: `github-workflows`
- Path: `.github/workflows/build.yml`
- Line: `146`
- Category: `others`
- Probability: `LOW`
- Message: Use full commit SHA hash for this dependency.

## SH-265

- Status: `pending`
- Key: `AZwwyFe10qvqV_CtK2O_`
- Rule: `githubactions:S7636`
- Area: `github-workflows`
- Path: `.github/workflows/build.yml`
- Line: `339`
- Category: `others`
- Probability: `LOW`
- Message: Avoid expanding secrets in a run block.

## SH-266

- Status: `pending`
- Key: `AZwwyFe10qvqV_CtK2PA`
- Rule: `githubactions:S7636`
- Area: `github-workflows`
- Path: `.github/workflows/build.yml`
- Line: `341`
- Category: `others`
- Probability: `LOW`
- Message: Avoid expanding secrets in a run block.

## SH-267

- Status: `pending`
- Key: `AZwwyFe10qvqV_CtK2PB`
- Rule: `githubactions:S7636`
- Area: `github-workflows`
- Path: `.github/workflows/build.yml`
- Line: `536`
- Category: `others`
- Probability: `LOW`
- Message: Avoid expanding secrets in a run block.

## SH-268

- Status: `pending`
- Key: `AZwwyFe10qvqV_CtK2PC`
- Rule: `githubactions:S7636`
- Area: `github-workflows`
- Path: `.github/workflows/build.yml`
- Line: `538`
- Category: `others`
- Probability: `LOW`
- Message: Avoid expanding secrets in a run block.

## SH-269

- Status: `pending`
- Key: `AZ6kqmM7OMiYihiGkbUb`
- Rule: `githubactions:S7636`
- Area: `github-workflows`
- Path: `.github/workflows/docs.yml`
- Line: `49`
- Category: `others`
- Probability: `LOW`
- Message: Avoid expanding secrets in a run block.

## SH-270

- Status: `pending`
- Key: `AZ6kqmM7OMiYihiGkbUc`
- Rule: `githubactions:S7636`
- Area: `github-workflows`
- Path: `.github/workflows/docs.yml`
- Line: `51`
- Category: `others`
- Probability: `LOW`
- Message: Avoid expanding secrets in a run block.

## SH-271

- Status: `pending`
- Key: `AZ6kqmM7OMiYihiGkbUd`
- Rule: `githubactions:S7637`
- Area: `github-workflows`
- Path: `.github/workflows/docs.yml`
- Line: `55`
- Category: `others`
- Probability: `LOW`
- Message: Use full commit SHA hash for this dependency.

# Completion Criteria

- Every item is marked `done` or `reviewed`.
- SonarCloud shows no unreviewed hotspots for the project, or remaining
  rows are intentionally accepted in Sonar with rationale.
- Each code-changing item has focused tests or a documented reason tests
  were not practical.
- Full build and relevant focused tests pass before final cleanup.
