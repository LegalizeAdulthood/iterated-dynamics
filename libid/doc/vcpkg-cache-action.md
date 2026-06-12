<!--
SPDX-License-Identifier: GPL-3.0-only

Copyright 2026 Richard Thomson
-->

# vcpkg GitHub Packages Cache Action Plan

## Goal

Create reusable GitHub Actions for vcpkg binary caching backed by GitHub
Packages NuGet feeds, using the workflow `GITHUB_TOKEN` path by default.

The action should remove the repeated setup burden across repositories.
It should make cache failures diagnosable without forcing each repository
to add its own ad hoc logging.

The action should support the flow that was verified in this repository:

- configure a GitHub Packages NuGet source with `${{ github.token }}`;
- use `VCPKG_BINARY_SOURCES` with a NuGet `readwrite` source;
- restore packages from the cache on warm runs;
- upload cache misses on cold or partial runs;
- recreate packages as public when publishing from a public repository
  through `GITHUB_TOKEN`, as observed in this repository;
- avoid private package quota pressure when packages are public.

The action should also diagnose the failure modes that made this hard to
debug:

- missing token;
- missing `packages: write` permission;
- wrong feed owner;
- unbootstrapped vcpkg;
- missing NuGet or Mono;
- GitHub Packages authentication failures;
- GitHub billing or quota blocks;
- restore misses caused by package identity mismatch;
- upload failures or submissions to zero binary caches;
- unexpected private package visibility.

## Shape

Provide two JavaScript actions in one action repository:

- `setup`: prepare NuGet, configure the feed, and emit cache outputs.
- `analyze`: inspect cache health after the build.

Do not wrap caller build commands.  That is too intrusive and slows
adoption.  Callers keep their own checkout, bootstrap, build, test, and
artifact steps.

JavaScript is preferred over a pure composite action because the work is
cross-platform and diagnostic-heavy.  A Node action can spawn commands
without going through bash on Windows, keep sanitization in one place, and
share parser code between setup and analyze.

The actions should use the current GitHub Actions Node runtime.  Use Node
24 when available so the action does not start life on a deprecated
runtime.

## Caller Contract

The caller grants package permission and passes the built-in token
explicitly.  The action should not require a PAT for the normal path.

```yaml
permissions:
  contents: read
  packages: write

steps:
- uses: actions/checkout@v6
  with:
    submodules: true

- run: ./vcpkg/bootstrap-vcpkg.sh

- id: vc
  uses: owner/vcpkg-github-cache/setup@v1
  with:
    token: ${{ github.token }}

- run: cmake --workflow --preset ci-debug
  env:
    VCPKG_BINARY_SOURCES: ${{ steps.vc.outputs.binary-sources }}

- if: always()
  uses: owner/vcpkg-github-cache/analyze@v1
  with:
    token: ${{ github.token }}
    build-log: build.log
```

If the build output is not captured to a file, omit `build-log`.  The
analyzer must then report feed and restore-probe health without claiming
whether the actual build was a warm hit, partial hit, or cold seed.

## Setup Inputs

- `token`: required.  Expected value is `${{ github.token }}`.
- `feed-owner`: optional.  Defaults to the owner parsed from
  `GITHUB_REPOSITORY`.
- `vcpkg-root`: optional.  Defaults to `vcpkg`.
- `bootstrap`: optional.  Defaults to `false`.
- `install-nuget`: optional.  Defaults to `true`.
- `install-mono`: optional.  Defaults to `true`.
- `source-name`: optional.  Defaults to `GitHubPackages`.
- `access`: optional.  Defaults to `readwrite`.
- `debug`: optional.  Defaults to `false`.
- `trace`: optional.  Defaults to `false`.

GitHub action inputs are strings.  Boolean inputs should treat only `true`,
`1`, `yes`, and `on`, case-insensitively, as enabled.

## Setup Outputs

- `feed-url`: the GitHub Packages NuGet feed URL.
- `binary-sources`: the value for `VCPKG_BINARY_SOURCES`.
- `nuget-command`: the command selected for NuGet.
- `vcpkg-version`: the bootstrapped vcpkg tool version.
- `diagnosis`: short setup diagnosis text.

The `binary-sources` output should normally be:

```text
clear;nuget,https://nuget.pkg.github.com/OWNER/index.json,readwrite
```

## Setup Behavior

The setup action should:

1. Register the token with the GitHub Actions secret masker.
2. Resolve `vcpkg-root`.
3. Optionally bootstrap vcpkg.
4. Verify that vcpkg is bootstrapped.
5. Ensure NuGet is available through `vcpkg fetch nuget`.
6. Ensure Mono is available when the NuGet tool is a `.exe` on Unix.
7. Configure the GitHub Packages NuGet source.
8. Set the API key for the same feed.
9. Emit `binary-sources`.
10. Write a concise step summary.

On Windows, run the vcpkg-fetched `nuget.exe` directly.  On Linux and
macOS, run it as `mono /path/to/nuget.exe`.

If `install-mono` is enabled and Mono is missing:

- on Ubuntu, install Mono with `apt-get`;
- on macOS, install Mono with Homebrew;
- on unsupported platforms, fail with a clear prerequisite message.

The action should never assume bash exists on Windows.  Use direct process
spawn for commands.  If a shell is needed, choose the platform-native shell
explicitly and trace that choice when `trace` is enabled.

## Analyze Inputs

- `token`: required.  Expected value is `${{ github.token }}`.
- `feed-owner`: optional.  Defaults to the owner parsed from
  `GITHUB_REPOSITORY`.
- `vcpkg-root`: optional.  Defaults to `vcpkg`.
- `build-log`: optional path to a captured build log.
- `package-config-glob`: optional.  Defaults to `**/packages.config`.
- `expect-token`: optional.  Defaults to `github`.
- `fail-on`: optional.  Defaults to `never`.
- `debug`: optional.  Defaults to `false`.
- `trace`: optional.  Defaults to `false`.

`fail-on` should support these values:

- `never`;
- `auth`;
- `quota`;
- `restore-failure`;
- `upload-failure`;
- `cache-miss`;
- `private-package`.

## Analyze Outputs

- `cache-status`: high-level cache result.
- `diagnosis`: short human-readable diagnosis.
- `requested-count`: package count from `packages.config`, if known.
- `restored-count`: restored package count, if known.
- `built-count`: vcpkg package build count, if known.
- `uploaded-count`: successful binary cache upload count, if known.
- `failure-kind`: normalized failure kind, if any.
- `diagnostics-artifact`: artifact name, when `debug` is enabled.

## Analyze Sources

The analyzer should combine live probes, filesystem state, and optional
build logs.

Live probes:

- token presence;
- feed URL;
- feed HTTP status with basic auth;
- feed HTTP status with bearer auth;
- vcpkg version;
- vcpkg NuGet path;
- NuGet version;
- NuGet locals;
- NuGet source list;
- sanitized NuGet config files;
- exact restore probe from discovered `packages.config` files.

Filesystem state:

- `vcpkg/buildtrees/packages.config`;
- matching `packages.config` files under the workspace;
- `vcpkg/packages/*/share/*/vcpkg_abi_info.txt`;
- vcpkg buildtree logs;
- generated `.nupkg` files under the workspace.

Optional build log:

- vcpkg restore lines;
- vcpkg package build lines;
- binary cache submission lines;
- NuGet auth and HTTP failures;
- GitHub billing and quota failures;
- `packages.config` package identities.

## Diagnosis Model

The analyzer should classify results into these statuses:

- `warm-hit`: all requested packages restored; no package builds.
- `partial-hit`: some packages restored; some packages built.
- `cold-seed`: no packages restored; misses built and uploaded.
- `restore-healthy`: exact restore probe succeeds, but build log is absent.
- `restore-miss`: exact restore finds no requested packages.
- `auth-failure`: feed or package restore is unauthorized.
- `quota-failure`: GitHub Packages reports billing or quota block.
- `upload-failure`: built packages were not uploaded successfully.
- `cache-disabled`: no binary cache source appears to be active.
- `tooling-failure`: vcpkg, NuGet, or Mono is missing or unusable.
- `unknown`: not enough evidence for a useful conclusion.

The diagnosis should lead with the conclusion, then cite the evidence.  For
example:

```text
vcpkg GitHub Packages cache: warm hit

Feed auth: ok
Token path: GITHUB_TOKEN
Restore: 15/15 packages from NuGet
Build misses: 0
Upload: not needed
Package visibility: public
Quota risk: none detected
```

If no build log is supplied, the action must not claim a warm or cold build
result.  It should say:

```text
Feed and exact restore are healthy, but build cache effectiveness is
unknown
because no build log was provided.
```

## Failure Patterns

The analyzer should recognize these patterns.

Token and permission failures:

- `GITHUB_TOKEN` input is empty;
- feed probe returns `401` or `403`;
- NuGet restore reports `Unauthorized`;
- NuGet publish reports `Forbidden`;
- package upload reports `Completed submission ... to 0 binary cache(s)`;
- package publish is forbidden while feed metadata access succeeds.

Quota failures:

- `Account has reached its billing limit`;
- `twirp error permission_denied`;
- package download returns `403` with quota text;
- package upload returns `403` with quota text.

Restore misses:

- `Restored 0 package(s) from NuGet`;
- `packages.config` requests package IDs that are absent from the feed;
- vcpkg builds packages immediately after a zero restore;
- exact restore probe fails for the same package IDs.

Warm hits:

- `Restored N package(s) from NuGet` where `N` equals the requested count;
- vcpkg handles each package in milliseconds;
- no `Building <port>:<triplet>` lines appear;
- no upload is needed.

Cold seed or partial seed:

- vcpkg builds missing packages;
- `Starting submission ... to 1 binary cache(s)` appears;
- `Uploading binaries ... to NuGet` appears;
- `Completed submission ... to 1 binary cache(s)` appears.

Package visibility and quota risk:

- query package metadata when the package names are known;
- report public packages as no storage quota risk;
- report private packages as storage quota risk;
- warn if a public repository creates private packages.

GitHub documentation does not state the public-repository behavior as a
simple unconditional rule.  The action should report actual observed
visibility when metadata is available instead of assuming it.

## Debug Output

`debug` defaults to `false`.

When `debug` is disabled:

- emit a short diagnosis;
- write a compact `GITHUB_STEP_SUMMARY`;
- do not dump raw configs or long logs.

When `debug` is enabled:

- emit the short diagnosis first;
- upload a sanitized diagnostics artifact;
- include raw scraped data needed for manual or AI-assisted analysis;
- keep secrets redacted.

The diagnostics artifact should use this shape:

```text
vcpkg-cache-diagnostics/
  summary.md
  environment.txt
  github-context.txt
  vcpkg-version.txt
  vcpkg-tool-metadata.txt
  nuget-version.txt
  nuget-command.txt
  nuget-sources.txt
  nuget-locals.txt
  nuget-config-sanitized.txt
  feed-probe-basic.txt
  feed-probe-bearer.txt
  packages-config.txt
  restore-probe.txt
  build-log-extract.txt
  abi-info/
```

## Trace Output

`trace` defaults to `false`.

`debug` answers "what did the action observe?"  `trace` answers "what did
the action do?"

When `trace` is enabled, show:

- resolved inputs, with secrets masked;
- chosen platform path;
- selected NuGet command;
- selected feed URL;
- whether vcpkg bootstrap ran or was skipped;
- whether Mono install ran or was skipped;
- exact non-secret commands before execution;
- command exit codes;
- elapsed time per setup or probe step;
- file paths read and written;
- branch decisions and their reasons.

Trace output should make it possible to understand unexpected behavior
without opening the action source.

## Sanitization

Sanitization is mandatory for both debug and trace output.

Always redact:

- token input value;
- `Authorization` headers;
- `X-NuGet-ApiKey` headers;
- NuGet password entries;
- NuGet API key entries;
- cookies;
- any value that exactly matches the token.

Sanitize NuGet config files before writing them to logs or artifacts.
Preserve source names, source URLs, and usernames.  Remove password and API
key values.

Do not dump the full GitHub context.  Select safe fields only:

- event name;
- repository;
- repository owner;
- ref;
- SHA;
- runner OS;
- runner architecture;
- image OS and image version, when available.

## Package Visibility Probe

When package IDs are known, query package metadata for a small bounded
sample.  Use the GitHub REST API with the same token.

The probe should report:

- package name;
- package type;
- visibility;
- repository association, if present;
- package URL;
- newest version count, if available.

If the API cannot read visibility, report `unknown` and keep the main
diagnosis based on restore and upload behavior.

## NuGet Source Setup

The setup action should configure both source credentials and the API key
for the feed selected by `feed-owner`.

The equivalent command behavior is:

```text
nuget sources add
  -Source FEED_URL
  -StorePasswordInClearText
  -Name GitHubPackages
  -UserName OWNER
  -Password TOKEN

nuget setapikey TOKEN
  -Source FEED_URL
```

The action should use the vcpkg-fetched NuGet executable for both commands
so the configured tool matches the tool vcpkg will use.

If a source with the same name already exists, update it or remove and
recreate it.  Do not silently leave a stale source pointing at another
owner.

## Exact Restore Probe

The analyzer should find `packages.config` files and run an exact restore
probe against the configured feed.

The probe should:

- use the same NuGet command selected by setup;
- use the same feed URL;
- use a temporary package directory;
- disable the HTTP cache when possible;
- use detailed verbosity;
- continue collecting diagnostics after a failed restore.

The restore probe is the most useful post-build test when the build log is
missing.  It proves whether the requested package identities can be
downloaded from the feed at analysis time.

## Build Log Parser

When `build-log` is supplied, parse it into structured facts:

- requested package count;
- restored package count;
- package IDs restored;
- packages built locally;
- submissions started;
- uploads attempted;
- uploads completed;
- failed HTTP status codes;
- quota messages;
- auth messages;
- NuGet config paths used;
- feeds used.

The parser should tolerate GitHub log prefixes and ANSI color escapes.

Keep regexes small and covered by fixture tests.  Do not make one giant
parser that depends on exact full log shape.

## Security

The action must be safe to run on public repositories.

- Do not print token values.
- Do not print raw NuGet configs.
- Do not upload unsanitized build logs.
- Bound package metadata probes.
- Bound artifact size.
- Avoid recursive dumps of the whole workspace.
- Treat forked pull requests as likely read-only.

If package write permission is unavailable on a pull request, prefer a
clear diagnosis over a noisy failure:

```text
GitHub Packages cache is readable, but writes are not available for this
event.  This is expected for some pull request events.
```

## Implementation Slices

### Slice 1: Action Skeleton

Create the action repository with:

- `setup/action.yml`;
- `analyze/action.yml`;
- shared TypeScript source;
- build step that emits checked-in `dist` files;
- unit test framework;
- lint and format checks.

### Slice 2: Setup Action

Implement:

- input parsing;
- token masking;
- vcpkg root resolution;
- optional bootstrap;
- `vcpkg fetch nuget`;
- Mono install and detection;
- NuGet source configuration;
- outputs;
- concise step summary.

### Slice 3: Analyzer Core

Implement:

- live probe runner;
- `packages.config` discovery;
- exact restore probe;
- build log parser;
- diagnosis classifier;
- `fail-on` policy.

### Slice 4: Debug And Trace

Implement:

- trace logging;
- sanitized diagnostic artifact assembly;
- safe GitHub context dump;
- sanitized NuGet config dump;
- build log extracts.

### Slice 5: Package Metadata

Implement bounded package metadata probes:

- package visibility;
- repository association;
- package version count;
- quota-risk warning.

### Slice 6: Documentation And Examples

Document:

- minimal setup;
- setup plus analyze;
- build-log capture examples;
- public repository expectations;
- private repository quota behavior;
- forked pull request behavior;
- troubleshooting examples.

## Tests

Unit tests:

- boolean input parsing;
- token sanitization;
- NuGet config sanitization;
- feed URL construction;
- platform command selection;
- log parser patterns;
- diagnosis classification;
- `fail-on` behavior.

Fixture tests:

- warm Windows restore with 15 packages;
- cold or partial Linux seed with uploads;
- quota blocked restore;
- auth failure;
- upload to zero binary caches;
- missing Mono;
- unbootstrapped vcpkg;
- private package visibility warning.

Integration tests:

- mock HTTP feed probes;
- mocked GitHub package metadata API;
- dry-run setup without writing real package sources;
- public test repository that publishes a small vcpkg package;
- private test repository that verifies quota-risk reporting.

The action should not require real package publish tests for every pull
request.  Use mocked tests for normal CI and scheduled or manual
integration runs for live GitHub Packages behavior.

## Acceptance Criteria

- A public repository can configure vcpkg NuGet binary caching with
  `GITHUB_TOKEN` and no PAT.
- Setup emits a usable `VCPKG_BINARY_SOURCES` value.
- NuGet provisioning works on Windows, Linux, and macOS hosted runners.
- The analyzer identifies warm hit, partial hit, cold seed, auth failure,
  and quota failure from fixtures.
- Without a build log, the analyzer performs live probes and reports only
  what it can prove.
- With `debug: true`, a sanitized artifact contains enough raw data for
  manual or AI-assisted failure analysis.
- With `trace: true`, the action explains its commands and decisions.
- No secret values appear in logs or artifacts.
- Public package visibility is reported from metadata when available.
- The default output is concise.

## Open Questions

- Should setup default `bootstrap` to `false`, or offer an `auto` mode?
- Should Mono install use `mono-complete` or a smaller package set?
- Should source update remove and recreate existing sources, or use NuGet
  source update when available?
- Should package visibility probing be enabled by default or only in
  analyze debug mode?
- Should the action offer a read-only cache mode for pull request builds?
- Should the repository ship one action with `mode`, or two separate action
  entry points?  The current plan prefers two entry points.
