<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# Engine Polling I/O Inversion Plan

Goal: invert rendering control so input and presentation code decide when
to ask for another bounded unit of work, while engine and fractal code own
calculation state, lifecycle, timing, setup, and cleanup.

This is true inversion of control for the existing polling renderer.  It
is not yet the later worker-thread or event-driven GUI design.  The
release 1.5 target is a clean directory boundary: only `libid/ui`
interacts directly with keyboard and mouse input, and engine code never
selects UI control paths.

Inversion is not achieved by moving a call to a new function, class,
callback table, adapter, or wrapper while the same synchronous control
flow remains hidden inside it.  A slice counts as inversion only when it
changes who owns a decision or return point:

- input polling and key meaning move outward to `libid/ui`;
- waiting and presentation pacing move outward to `libid/ui`;
- calculation work becomes explicit, bounded, and resumable;
- calculation code returns to its caller instead of polling input or
  waiting internally;
- application phase and command effect are represented separately.

The release 1.5 input boundary is implemented.  The descriptions below
record current ownership and behavior, including remaining presentation
and backend coupling.  They are not instructions to restore an earlier
design where implementation has superseded it.

The standard escape-time renderer follows the same shape as the other
calculation types split into UI coordination code and calculation objects.

## Existing Shape

These files show the calculation-object and UI-controller structure.
The audit below records the implemented boundary and its current limits:

- `libid/ui/AntController.cpp` drives selected Ant work in
  `libid/fractals/Ant.cpp`.
- `libid/ui/BifurcationController.cpp` drives
  `libid/fractals/Bifurcation.cpp`.
- `libid/ui/CellularController.cpp` drives selected Cellular work in
  `libid/fractals/Cellular.cpp`.
- `libid/ui/DiffusionController.cpp` drives `libid/fractals/Diffusion.cpp`.
- `libid/ui/OrbitController.cpp` drives selected 2D orbit and Dynamic2D
  work in `libid/fractals/lorenz.cpp`.
- `libid/ui/IFS2DController.cpp` drives selected 2D IFS work in
  `libid/fractals/lorenz.cpp`.
- `libid/ui/StereoController.cpp` drives selected 3D IFS and orbit work
  owned by `libid/engine/NonStandardFractal.cpp`.
- `libid/ui/PlasmaController.cpp` drives `libid/fractals/Plasma.cpp`.
- `libid/ui/Standard4DController.cpp` drives
  `libid/fractals/julibrot.cpp`.
- `libid/ui/LSystemController.cpp` drives selected sizing and drawing
  work in `libid/fractals/lsystem.cpp`.
- `libid/ui/PopcornController.cpp` drives selected image work in
  `libid/fractals/popcorn.cpp`.
- `libid/ui/FrothyBasinController.cpp` drives selected pixel and overlay
  work in `libid/fractals/FrothyBasin.cpp`.
- `libid/ui/TestPointController.cpp` drives `libid/fractals/TestPoint.cpp`.

The common structure is:

```text
libid/ui/<type>.cpp
    construct or resume the calculation object
    poll keyboard input for this UI context
    suspend or exit when the UI context decides work should stop
    call bounded iterate() steps until the calculation object is done

libid/fractals/<Type>.cpp or libid/engine/<Type>.cpp
    own calculation state
    expose resume(), suspend(), done(), and iterate()
    do not poll keyboard or mouse input
```

Cellular exposes initial-generation preparation as calculation state;
its UI controller owns thinking-screen updates and cleanup.  L-system
sizing and drawing use explicit frame stacks and advance incrementally.
Its UI controller owns the thinking screen and interruption in both
phases; sizing and drawing do not poll input.

JIIM's `InverseJulia` calculation owns orbit state and accepts explicit
point and plot inputs.  `InverseJuliaController` in
`libid/ui/inverse_julia.cpp` owns the outer interaction loop, cursor,
coordinate messages, mode preferences, and keyboard and mouse scopes.
`InverseJulia::iterate()` only calculates and plots bounded orbit work.

## Standard Render Structure

The standard escape-time calculation object already has this lifecycle:

```cpp
namespace id::engine
{
class StandardFractal
{
public:
    std::vector<std::string> resume();
    void suspend();
    void finish();
    bool done() const;
    void iterate();
};
}
```

`StandardFractal` owns the standard renderer lifecycle.  That includes the
work list, the active work-list item, the active `StandardPass`, and the
active pixel's orbit state when a pixel needs to yield before it is
finished.

The active `StandardPass` owns pass-specific traversal and fill state.
That includes active row and column, subdivision state, block state, scan
stack, and decisions about whether a pixel needs real orbit work or can be
filled from surrounding pixels.

Pixel traversal passes do not compute the orbit.  They expose
`select_pixel()`, `selected_pixel()`, and `complete_pixel(color)`;
`StandardFractal` computes the selected pixel and supplies its color.
Communication still uses globals such as `g_row`, `g_col`, `g_color`, and
`g_plot`.  Removing those globals is separate work.

`SynchronousOrbit` and `StickyOrbits` are different: they advance orbit
work directly.  `StandardPass::iterate(PertEngine *)` routes their bounded
work and supplies the selected perturbation strategy when active.  They
do not use the pixel-pass selection/completion contract.

The non-virtual `StandardPass` router holds a `std::variant` of concrete
pass classes.  It routes chunk preparation, selection, pixel completion,
orbit iteration, status, suspension, and resume storage to the active
alternative.

Each concrete pass owns its state and operation methods in its own class:
`BoundaryTrace`, `Diffusion`, `OnePass`, `TwoPass`, `SolidGuess`,
`SynchronousOrbit`, `Tesseral`, and `StickyOrbits`.  `OnePass` and
`TwoPass` share `ScanlinePass`.  `StandardPass` includes the matching
pass headers, stores the concrete class in the variant, and routes calls
to it.  Old free-function pass traversal entry points have been removed;
do not reintroduce them as call adapters.

`StandardFractal` also owns the standard-mode dispatch currently selected
by `g_std_calc_mode`, including `SynchronousOrbit`, `Tesseral`,
`BoundaryTrace`, `SolidGuess`, `Diffusion`, `StickyOrbits`, and the
`OnePass` and `TwoPass` paths.

Perturbation is an alternate orbit strategy selected and owned by
`StandardFractal`, separately from traversal.  Pixel passes receive its
completed colors; synchronous and sticky orbit work receive the strategy
through their iteration interface.  `passes=p` remains compatibility
syntax selecting perturbation with solid-guess traversal, not a separate
perturbation pass.

The UI polling loop owns input policy.  It constructs or resumes
`StandardFractal`, polls input through the active UI context, asks the
object to suspend when work is interrupted, and calls bounded `iterate()`
steps until `done()` is true.

The standard pixel kernels are calculation details.  They should not
double as image-level UI entry points.  If the current names are kept for
compatibility during the refactor, the final shape should still separate:

- image-level UI polling loop
- standard calculation object
- per-pixel escape-time helpers
- per-orbit helper functions

## Keyboard Handler Stack

The handler stack is UI input plumbing.  It supports the standard UI
polling loop; it is not the calculation-side architecture.

Handlers decide what a key means in a UI context:

```cpp
namespace id::ui
{
class KeyboardHandler
{
public:
    virtual ~KeyboardHandler() = default;

    virtual bool handle_key(int key) = 0;
};

using KeyboardHandlerPtr = std::shared_ptr<KeyboardHandler>;
}
```

`handle_key()` returns `true` only when the handler consumed the key.  It
does not return an interrupt result.  Handlers that want calculation to
yield set UI-owned interruption state.

`MainLoopKeyboardHandler` is the outer image UI context.  It consumes keys
that reach it, records the next main-loop command, and requests
calculation interruption.

`OrbitToggleKeyboardHandler` is a more-specific render context.  It
consumes `o` and `O`, toggles orbit display, and does not request
calculation interruption.

The UI dispatcher polls the driver queue, reads available keys, and offers
them to the active handler stack from top to bottom.  The first handler
that returns `true` stops propagation.  If no handler consumes the key,
the key is discarded.

## Application Phase and Command Effects

The menu command-result split has been implemented.  `MenuHandler`
callbacks return `CommandResult`; the boundary near `big_while_loop()`
maps those effects onto the existing `MainState` application-phase switch
and local-loop behavior.  Keep that distinction as the outer application
loop is inverted.

The broader application-loop inversion remains future work.  In that
shape, input is the outer loop.  Keyboard, mouse, command-file,
command-line, and batch input are all command sources.  The
engine should construct into a default, well-defined render model, but no
image can be generated until a display target exists.  Selecting a video
mode supplies the image dimensions; after that, default parameters such
as `type=mandel` and `passes=g` are enough to begin rendering.

The control inputs should be separated from coarsest to finest:

- Application phase: no display target, configured idle, rendering, image
  complete, restoring or loading, and exiting.
- Fractal identity: changing `type=` rebuilds dispatch, resets
  type-specific state, and invalidates the image, but is not a full
  restart.  `<Insert>` is the full reset path and resets the active type
  back to `mandel`.
- Render invalidation: changing passes, bailout, corners, maxiter,
  inside/outside color modes, precision, or similar values requires pixel
  orbits to be recomputed.
- Presentation invalidation: changing the orbit window, palette or
  colormap, tab display, or other display-only state does not require
  pixel orbits to be recomputed.
- Local UI or no-op: menu navigation, help, cancelled dialogs, save,
  shell, and other actions that do not require an application phase
  change.

Current handlers communicate command effects through `CommandResult`,
with remaining `g_calc_status` and `MainContext::more_keys` side effects.
`MainState::CONTINUE` remains loop mechanics, not a semantic command
effect.  Preserve the explicit command-effect mapping as the outer
application loop learns to drive render increments.

The menu command-result split keeps this boundary:

- `MenuHandler` callbacks return `CommandResult`.
- Keep the existing `MainState` switch in `id_main`.
- One mapping point near `big_while_loop()` translates command results
  into existing `MainState` transitions and local-loop behavior.
- Retain the current `g_calc_status` and `MainContext::more_keys` side
  effects until a focused slice replaces each with an explicit effect.

The audited command-result categories are:

- ignored
- handled locally
- presentation changed
- display target changed
- resumable render state changed
- render parameters changed
- fractal type changed
- interaction mode changed
- restore requested
- full reset requested
- exit requested

## Parameter Processing Boundary

Parameter-set processing and engine parameter interpretation remain
engine concerns.  Option 1 partitions handlers by ownership: UI handles
UI parameters and delegates unrecognized names to engine.  Both groups
share syntax processing and preserve the original parameter order.

Shared syntax, recognition, source routing, mixed-parameter ownership,
initialization, ordering, failure behavior, and consequence reporting are
implemented as described below.  `CommandRouter` owns UI-first routing;
`CommandProcessor` owns incremental source traversal and aggregation.

### Agreed Shared Syntax

- Reuse the existing `Command` in `engine/Command.h` and
  `engine/Command.cpp`.  Both UI and engine handlers receive the same
  `const Command &`; decode each parameter once before handler selection.
  Source routing follows the agreed design below.
- Retain the existing parsed fields, numeric helpers, sentinels, array
  limits, and empty, omitted, and trailing-field behavior.  Preserve
  original numeric text for arbitrary-precision conversion; actual
  conversion and precision changes remain with engine parameter handlers.
- Keep `lowerize_parameter` and its parameter-specific case-preservation
  rules as shared language processing.  The exemption list describes
  syntax, not ownership of UI runtime state.
- Own the normalized argument text privately and do not mutate it after
  construction.  Its string views and character pointers refer into that
  text.  Prohibit copying and moving `Command`; construct it locally and
  pass references while processing the parameter.
- Retain `CmdFile`, the existing `CommandParseResult &`, `bad_arg()`, and
  `warn_missing_file()` contracts.  This is a parsed command with context
  and a result destination, not a pure value object.  Diagnostic behavior
  stays unchanged under the consequence-reporting agreement below.
- Keep supported-name recognition, startup restrictions, setting changes,
  and handler selection outside `Command`.  For example,
  `maxiter=garbage` can be decomposed syntactically; its handler rejects
  the nonnumeric value.  Do not redesign decoding during extraction.
- Direct command-syntax tests cover the shared representation separately
  from handler ownership, routing, and result aggregation.

### Agreed Recognition

- Reuse `std::optional<CmdArgFlags>` for each handler group.  An empty
  optional means the group does not own the name and permits delegation.
  `NONE` and other nonnegative flags mean acceptance; `BAD_ARG` means
  recognition followed by rejection.  Neither acceptance nor rejection
  permits delegation.
- UI handles its names first.  Delegate to engine only for an empty UI
  result.  Reject syntax errors before either group runs.  If both groups
  return empty, the outer coordinator calls `Command::bad_arg()` once.
  Do not append another diagnostic for a rejection already reported.
- Recognize ownership before checking permission.  A restricted UI name
  remains UI-owned and is rejected without falling through to engine.
  Preserve the current error text and change flags.
- UI owns `askvideo`, `virtual`, `maxhistory`, `autokey`, and
  `autokeyname`.  `maxhistory` remains startup-only.  Existing ordinary
  engine parameters remain engine-owned.  UI owns the complete `video`,
  `rds`, and `rds-texture` parameters under the mixed-ownership agreement
  below, and `exitnoask` and `reset` under the initialization agreement.
- Retain separate startup-only and unrestricted tables within each owner.
  Startup commands are permitted when `CmdFile` is not
  `AT_AFTER_STARTUP`, or when `ALLOW_INIT_COMMANDS_ANYTIME` applies.
  Evaluate eligibility for each parameter rather than snapshotting the
  debug override for the whole source.
- Require unique names within and across all owner tables, including
  startup-only versus unrestricted tables.  Validate the actual assembled
  metadata once before executing parameters.  Report duplicate ownership
  with `std::logic_error`, not lookup precedence or assertions.  The UI
  coordinator validates tables before its first source advance.
- Reuse existing parameter tests.  Add focused coverage for delegation,
  rejection without delegation, startup restrictions and their override,
  single diagnostic emission, and duplicate ownership.  Engine lookup
  implements the shared result contract.  The UI coordinator composes
  UI-first routing and validates owner tables before source advancement.

### Agreed Source Routing

- `engine::CommandProcessor` performs incremental source processing.
  UI owns the outer routing loop; engine never calls a UI handler or
  receives a UI callback.
- Engine retains command-line position, startup configuration, file
  lookup/open/read/close, existing `@file` and `@file/setname` expansion,
  processing context, comments, continuation lines, and set boundaries.
  Do not introduce recursive include behavior that is not already
  supported.
- Expose one current `Command` at a time, held in place without copying
  or moving.  UI applies the agreed recognition rules to that same
  object and writes the outcome into its existing result destination.
  The next advance incorporates that outcome, completes source work as
  necessary, and presents the next parameter.  No acknowledgement call
  is needed; the current command stays valid until the next advance.
- Engine owns per-command, per-file, and overall result accumulation and
  source-specific continuation.  It sees command outcomes, not UI
  settings.  A rejected command or syntax error prevents further handler
  dispatch for that command.  Initial-startup and batch errors also stop
  later source processing; nonfatal interactive restart errors can
  continue under the ordering and failure rules below.
- Support startup/restart sources, a selected parameter set supplied as
  an already-positioned file, and a single interactive parameter.  Adopt
  file ownership explicitly and close files on completion and exceptions.
  Preserve distinct initialization and completion rules for each source;
  a single interactive parameter is not a PAR-file scope.
- Use the same UI-first loop for every parameter read from an `@` set.
  For `@scene.par/example` containing `video=F6`, engine opens and locates
  the set, then presents `video=F6` to UI.  UI resolves its binding and
  records the requested mode; engine then advances to the next parameter.
  Preserve mode-activation timing under the mixed-ownership agreement
  below.  `@` chooses the source, not the handler.
- Read and expand sources incrementally.  Do not flatten all inputs or
  open later files before earlier parameters have taken effect.  Keep
  bare-GIF argument recognition in engine source processing.
- Preserve parameter-flag initialization timing.  At parameter-file
  completion, including an early stop after rejection, engine applies
  existing compatibility adjustments when that file's flags require
  them.  Keep those flags separate from the overall startup result.
  Preserve interactive entry's existing debug-only compatibility path;
  do not apply file-completion rules to every command.
- `main_restart()`, `get_commands()`, and `get_cmd_string()` use the
  coordinator, including startup-configuration reprocessing.  The old
  whole-source `cmd_files()`, `load_commands()`, and textual `cmd_arg()`
  entry points are gone.  Diagnostic presentation and action execution
  remain at the outer UI boundaries.
- Validate all assembled owner tables in the coordinator before
  processing starts.  Do not retain an application-facing engine-only
  path that bypasses UI ownership.  Engine-handler unit tests may still
  invoke engine handlers directly.
- Source traversal is encapsulated in engine and stepped by UI.  The
  temporary runners used during extraction have been removed.
- Application startup, selected parameter sets, single-command entry, and
  fallback configuration processing all use the UI-first coordinator.
  No whole-source engine runner bypasses UI parameter ownership.

### Agreed Mixed-Parameter Ownership

- One handler owns each complete parameter.  UI owns `video`, `rds`, and
  `rds-texture`, including their validation and existing result flags.
  UI may configure public engine or geometry settings directly; the
  dependency remains UI-to-engine.  Do not introduce another handler,
  forwarded command text, duplicate validation, or a negotiation port.
- UI owns `cmd_video`.  Resolve the key name against UI-owned bindings
  and record the resolved mode index, not the key.  Parsing requests a
  mode; existing startup or parameter-set completion activates it at the
  current point.  Engine retains key-free active mode information.
- The pending request `g_init_mode` belongs to UI, unlike the active
  engine mode.  UI owns its storage, initialization, restart suppression,
  and parameter handler.  Follow the initialization agreement below,
  without engine-to-UI callbacks or forwarding wrappers.
- Preserve `video` rejection side effects: malformed key names leave the
  pending mode unchanged; valid but unbound names clear it.  Preserve
  `FRACTAL_PARAM | PARAM_3D` on success.  Do not activate a driver or
  change image dimensions while interpreting the parameter.
- UI owns `check_vid_mode_key_name`, including its use by configuration
  loading.  Engine has no key-name parser or UI key dependency.  Keep
  mode validation in engine and never restore `VideoInfo::key` or
  relocate UI binding storage there.
- UI owns `cmd_rds` and its calibration-bar parser.  Validate the
  complete parameter once into local values before changing settings.
  Depth, calibration bars, texture selection, and the texture filename
  remain geometry inputs; automatic conversion and filename reuse remain
  UI policy.  No additional geometry parameter handler is needed.
- `rds=no` disables automatic conversion only.  `yes` and `random`
  enable automatic conversion with random texture; `texture` enables it
  with image texture.  Preserve existing field-count, numeric, named-bar,
  and numeric-bar validation.  Omitted or empty depth and calibration
  fields retain their current values.  Reuse is enabled only when this
  parameter selects texture and a filename is already present.
- UI owns `cmd_rds_texture` and its asymmetric behavior.
  Always assign the filename; set reuse according to whether it is
  nonempty.  A nonempty filename also selects texture input.  An empty
  filename clears the filename and reuse, but leaves texture selection
  unchanged.  Neither form changes automatic conversion or opens a file.
- Preserve original parameter order and filename case.  For example,
  `rds-texture=map.gif rds=random` ends with random input and no reuse;
  the reverse order ends with texture input and reuse.  Do not replace
  these effects with a normalized combined mode or deferred actions.
  Geometry-only parameters such as `stereowidth` and `usegrayscale`
  remain engine-owned.
- The `video`, `rds`, and `rds-texture` handlers and key-name parsing are
  UI-owned.  Preserve existing diagnostics and change flags.
  Reuse current parameter, video, stereo, batch, and autokey coverage;
  extend it for rejection side effects, omitted fields, order-dependent
  reuse, and common routing through `@` sources.

### Agreed Initialization And Reset

- Extract appropriately named functions for initialization scopes before
  changing ownership: `init_process_defaults`,
  `reset_restart_defaults`, and `reset_fractal_defaults`.  Extract
  cohesive domain groups, such as viewport, orbit, and sound defaults,
  inside those functions where useful.  Preserve assignment order and
  the different settings each scope resets; do not create a universal
  reset or one forwarding function per assignment.
- UI initialization functions own the combined operation and call the
  corresponding engine initialization functions.  Use namespaces to
  distinguish the owners.  Engine functions initialize only engine
  settings; they never call UI initialization or receive UI callbacks.
  Introduce a UI counterpart when it owns actual initialization, not
  just to wrap an engine call.  Engine initializers remain independently
  callable without keyboard, mouse, or driver interaction.
- Keep run-level random-seed capture, command-comment initialization,
  and temporary-directory discovery under the existing `g_first_init`
  gate.  Preserve its debug override rather than replacing that gate
  with an unconditional once-per-process mechanism.
- Startup applies process initialization when gated, restart defaults,
  and fractal defaults before configuration and arguments.  `<Insert>`
  reapplies restart and fractal defaults, configuration, and arguments.
  Preserve its later suppression of startup mode and image requests.
  Keep the existing first-init bookkeeping and debug-override timing.
- Fallback after abandoned startup entry selection reapplies restart
  and fractal defaults and configuration only.  Do not rerun process
  initialization, arguments, all of `main_restart`, or ordinary startup
  completion suppression on that fallback path.
- `reset=` applies only fractal defaults and its compatibility version.
  Selected parameter sets and single commands apply no defaults unless
  an explicit `reset` occurs.  Preserve per-set parser-flag initialization
  separately; it is not a request to reset application settings.
- UI restart defaults set `g_ask_video` and `g_virtual_screens` to true
  and clear the pending mode to `-1`.  Explicit `reset=` does not change
  these settings.  `g_init_mode` storage and the `video` handler are
  UI-owned; restart initialization clears the pending mode before parsing.
- History capacity, autokey mode/name, automatic stereo conversion, and
  texture reuse have no reset assignments in these initializers.  Keep
  their process defaults in their owning modules and do not add resets.
  Existing UI startup groups, including browsing/evolution, disabling
  showdot, and the initial showorbit preference, stay in `main_restart`.
  History initialization keeps its current position after parameters.
- UI owns `exitnoask` handling, including startup eligibility, validation,
  diagnostics, and result flags.  UI owns `g_escape_exit` storage and its
  fractal-default reset.  Those defaults clear it to false, preserving
  both startup and explicit reset behavior.
- UI owns the complete `reset` command's orchestration.  Engine
  `parse_reset_version` validates and decodes its version using the shared
  parsed `Command` and existing `Version` type before mutation.  UI then
  invokes the combined fractal initializer once and calls engine
  `apply_reset_version`.  Preserve `g_version`, `g_file_version`,
  diagnostics, and `FRACTAL_PARAM | RESET` behavior without reparsing.
- Do not let an engine handler reset first and have UI react to an
  accumulated `RESET` flag by resetting again.  Initialization completes
  at the command's original position before processing another command,
  including inside `@` sources.  Rejected reset values change neither
  owner's defaults; later errors do not roll back an accepted reset.
- Keep configuration lookup/reading, mode activation, source completion,
  and action execution outside default initializers.  The UI source
  coordinator selects the appropriate initialization scope; engine
  source traversal does not hide a second initialization operation.
- UI owns restart defaults, pending-mode storage, exit-confirmation
  handling and storage, and explicit fractal-reset orchestration.

### Agreed Ordering And Failure

- Process one ordered stream.  Complete each command's UI or engine
  handling before reading another command.  Partition handler ownership,
  not execution into separate UI and engine passes.  Preserve repeated
  settings, order-dependent defaults, lazy lookup, and reset positions.
- Apply ordinary settings immediately, including UI preferences.  Keep
  changes preceding an error; do not introduce whole-input rollback,
  transactional command application, or success-only UI assignment.
  Preserve existing rejection side effects rather than assuming every
  rejected command leaves all settings unchanged.
- Source traversal owns continuation.  When `g_first_init` is true or
  batch mode is active, the first error stops all remaining startup
  sources.  This includes errors in configuration, an indirect file, or
  an ordinary argument; later arguments and actions do not run.  Accepted
  settings before the error remain applied.  Startup UI presents the
  first error through `stop_msg()` and returns failure.
- During a nonfatal interactive restart, argument errors can accumulate
  while later arguments run.  A rejected file command stops and closes
  that file; an enclosing startup processor can then continue with later
  arguments.  A selected set returns to its caller after rejection, and
  a single interactive command reports its result and returns.  UI does
  not impose a second, unconditional stop policy on the coordinator.
- Warnings alone neither stop processing nor suppress generation.
  For example, command-line arguments
  `askvideo=no maxiter=bad virtual=no` apply both UI preferences during
  a nonfatal interactive restart.  Initial startup, batch processing,
  and a rejected command in a parameter file stop before `virtual=no`.
- Finalize and close a parameter file after normal completion, early
  rejection, or a generation request.  Apply existing compatibility
  adjustments from that file's accepted change flags, independently of
  unrelated startup flags, before resuming an enclosing source where
  applicable.  Keep single-command completion distinct from file scope.
- Generation handlers record deferred actions, not immediate generation.
  An accepted generation request stops further source processing,
  including later command-line arguments after an indirect file.
  Earlier errors remain recorded and suppress action execution, but do
  not cause processing to resume after the request.  Ordinary settings
  already applied are not undone or recast as generation actions.
- Preserve `apply_command_effects` at the outer UI completion boundary.
  Errors suppress generation and device initialization; warnings do not.
  Keep existing action-failure diagnostics and exit handling.  Do not
  resume parsing or roll back settings after an action fails.
- Keep argument rejection distinct from exceptions.  Do not add a
  blanket coordinator catch that turns unrelated failures into bad
  arguments and continues.  Preserve existing propagation and cleanup.
- Startup processor and router tests cover first-error termination,
  nonfatal restart continuation, accepted-prefix changes, warning-only
  continuation, and action suppression.  Command-effect and diagnostic
  tests cover the outer UI response.

### Agreed Consequence Reporting

- Reuse `CommandParseResult`, `CmdArgFlags`, `CommandDiagnostics`, and
  `CommandAction` for both handler owners.  Each receives the same parsed
  `Command` and current result destination.  UI updates its own settings
  directly; do not add a UI-settings result, reporting interface, action
  queue, or settings transport into engine.
- Keep the agreed optional-result contract.  An empty optional means
  unrecognized; `NONE` is accepted without change flags, not proof that
  nothing happened.  Other nonnegative flags retain their meanings.
  `BAD_ARG` is rejection, not a change-bit combination.  Only the outer
  coordinator assigns the selected outcome to the current result flags.
  Use existing diagnostic helpers and do not report a rejection twice.
- Preserve each handler's existing flags independently of ownership.
  `maxhistory` still returns `FRACTAL_PARAM | PARAM_3D` after moving to UI.
  Do not silently reclassify regeneration consequences.  `RESET` reports
  an already-performed reset, not a request for another initialization;
  preserve existing menu interpretation of `RESET` and `YES_3D`.
- Keep one aggregation implementation in engine source processing.
  Incorporate each command result once before reusing its current-result
  storage.  OR nonnegative flags into accumulated flags, excluding
  `BAD_ARG`.  Append errors and warnings in encounter order within their
  existing separate collections.  Preserve nonempty-action assignment
  and OR `initialize_fm` requests.  Do not discard an entire rejected
  result: diagnostics and existing auxiliary requests still contribute.
- Keep failure separate from accumulated change flags.  A failed file
  returns its accepted-prefix flags plus diagnostics; a rejected single
  command retains `BAD_ARG`.  Do not collapse both contexts into a success
  boolean or let file-style aggregation erase single-command rejection.
- Keep file-local flags separate from startup totals.  Flags from UI
  handlers contribute to the same file-completion compatibility decision
  as before.  Moving `maxhistory` must not silently suppress that file's
  legacy adjustments.  Preserve completion after early stops and direct
  entry's separate debug-only compatibility path.
- Compute `needs_file_entry` from final engine settings at the existing
  startup-completion point, only when errors/actions permit that check.
  It is not a generically ORed command field.  UI performs selection;
  batch mode retains its missing-entry diagnostic without a UI callback.
- Keep diagnostic presentation, device initialization, generation, and
  exit handling at existing outer UI boundaries.  Reuse
  `apply_command_effects` with its current error gates, sound-device check,
  and failure handling.  Its return means exit requested, not command
  success.  Ordinary caller responses to change flags remain distinct
  from whether deferred actions were allowed to execute.
- Reporting uses the shared extraction, lookup, source, and coordinator
  types rather than a separate framework.  Tests cover aggregation,
  caller consequences, compatibility flags, diagnostics, deferred
  effects, and real application command paths.

## Implemented Render Boundary

The implemented controllers use the pattern from `ui::AntController`,
`ui::BifurcationController`, `ui::CellularController`, and similar types:

```text
ui polling loop
    construct or resume renderer
    poll keyboard input for the active UI context
    call suspend() or exit when the UI decides to stop
    call bounded iterate() steps until the renderer reports no more work

calculation object
    own calculation state
    expose iterate(), done() when useful, resume(), and suspend()
    do not poll keyboard or mouse input
```

Apply the same rule at the image-engine boundary.  Do not create an
`ImageCalculation` facade whose first `iterate()` call still runs the old
whole-image function to completion.  Do not move renderer classification
into `libid/ui`.  Do not add an interface, callback table, or adapter
merely to let the old control flow call through a new name.

An output port is a different boundary.  `SoundOutput` accepts calculated
data, like pixel output, without calling a controller or hiding a wait.
Do not reject that design merely because its interface is virtual.

The target is one active render operation selected from engine state and
dispatch data.  The operation owns render setup, bounded calculation
work, suspend/resume state, completion state, image timing, and cleanup.
Shared setup and teardown may use an implementation hierarchy, but the
base must own real common behavior rather than hide another synchronous
renderer call.

The current common lifecycle behavior is `ImageCalculation::start()` and
`ImageCalculation::finish()` in `engine/calcfrac.cpp`.  That behavior is
real shared render setup and teardown, not a renderer facade.  It owns
true-color setup and teardown, parser setup, log-map setup
and cleanup, orbit display defaults, inversion setup, resume cleanup,
calculation-time accounting, work-area cleanup, and disk image cleanup.

`StandardFractal` and `NonStandardFractal` use that shared
implementation directly.  Lyapunov uses it through its contained standard
calculation.  Standard resume performs common setup; bounded traversal
retains its existing return points.  Suspension, completion, and failure
cleanup finalize each elapsed interval once.  UI controllers acknowledge
setup warnings and prepare requested storage without owning calculation
setup, timing, or cleanup.

The concrete render operation objects are:

- `engine::StandardFractal`: owns standard escape-time traversal,
  `StandardPass`, standard pixel state, perturbation, pass dispatch,
  standard orbit-plot requests, and standard suspend/resume state.
  `ui::StandardFractalController` can start, step, and suspend a
  caller-owned StandardFractal.  It owns marker presentation, sound
  servicing, and the existing MainLoop/OrbitToggle handler scopes;
  completion and suspension release those scopes before returning.
- `fractals::Lyapunov`: owns Lyapunov render state by containing an
  `engine::StandardFractal`; Lyapunov-specific per-image setup remains in
  `lyapunov_per_image()` below the dispatch layer.
  `ui::LyapunovController` can start, step, and suspend a caller-owned
  Lyapunov, retaining marker presentation and MainLoop/OrbitToggle
  handler scopes.  It releases those scopes on completion, suspension,
  or failure.
- `engine::NonStandardFractal`: owns non-standard dispatch setup,
  symmetry defaults, plot defaults, `per_image()` invocation, and final
  status interpretation.  It shares common image setup, timing, and
  cleanup without entering UI render loops.
  Its bounded operation router owns bifurcation, Popcorn, standalone
  Diffusion, TestPoint, Ant, Cellular, Plasma, L-system, FrothyBasin, and
  IFS, 2D/3D orbit, dynamic, inverse-Julia, and standard-4D state without
  renderer callbacks.  Their UI controllers advance
  selected objects; setup, resume, and finish stay below UI.  L-system
  definition loading, startup, and terminal interpretation stay below
  UI; `ui::LSystemController` advances existing sizing/drawing increments
  and owns thinking-screen lifetime and the drawing poll throttle.
  Completion, suspension, and failure close its presentation; pending
  commands remain for the outer handler.  FrothyBasin retains
  orbit/pixel checkpoints, palette selection, and resume coordinates
  below UI;
  `ui::FrothyBasinController` advances existing increments with a
  caller-owned SoundSession.  It owns palette diagnostics, overlay
  interaction, sound servicing, and keyboard scopes, retaining current
  delay policy.
  The completed-image orbit tool uses setup-only FrothyBasin callbacks,
  independently of the selected image calculation's lifetime.
  `ui::PlasmaController` reports storage availability before engine
  initialization and advances existing subdivision increments.
  Cellular retains generation-sized preparation and next-screen
  initialization below UI; `ui::CellularController` advances
  one generation at a time and owns thinking-screen lifetime and error
  presentation.  It retains post-generation driver peeks, saved-row and
  next-screen continuation, and the legacy completion result.
  Popcorn's Julia setup and symmetry selection also stay below UI.
  `ui::PopcornController` advances existing image increments, retains
  post-step input checks and saved-row suspension, and owns its handler
  scope.  Popcorn does not use overlay pacing or sound servicing.
  Ant's Ctrl+A entry selects an operation on the existing
  image, without restarting common image setup, timing, or resource
  cleanup.  `ui::AntController` steps the selected Ant operation at
  existing batch boundaries and owns step/delay state, option-menu
  interaction, and help-mode lifetime.  It preserves the legacy zero
  return for completion or command interruption; the caller retains
  final cleanup after an option-menu transition.
  `ui::BifurcationController`, `ui::DiffusionController`, and
  `ui::TestPointController` step caller-owned selected operations at the
  existing increment boundaries.  They retain pre/post-step polling,
  disk-progress feedback, suspension, and result conventions; Diffusion
  owns its handler lifetime.
  Per-image callbacks no longer invoke renderers.  IFS selects 2D or 3D
  from loaded definition dimensions, including cached definitions;
  `type=ifs3d` remains compatibility syntax for `type=ifs`.  IFS retains
  unsymmetrized plotting and non-resumable interruption.  Selected 3D
  IFS and orbit objects own combined-eye state; their operations expose
  completed views and explicit next-view initialization.
  `geometry::StereoView` owns eye
  selection, projection setup, and full-screen stereo-pair restoration,
  including failure cleanup.  `ui::StereoController` drives both families
  and owns input, photographer prompts, palette setup, and sound
  servicing.  Its start/iterate/done/result/suspend operations advance
  caller-owned selected calculations one point at a time.  Eye
  transitions retain the modal PHOTO save prompt and prepare the next
  view before advancing a point.  Completion, suspension, and failure
  release its handler scope.
  Engine selection and sound-source classification do not depend on UI
  renderer pointers.  The legacy orbit/IFS rendering entry is gone.
  Orbit2D and Dynamic2D keep their existing increment and resume formats;
  `ui::OrbitController` steps both with a caller-owned SoundSession,
  retaining sound waits, menu interruption, disk-progress feedback, and
  completion drain.  Its handler scope ends on completion, suspension,
  or failure.
  `ui::IFS2DController` retains pre-point polling and non-resumable
  interruption.  `ui::Standard4DController` retains UI palette setup
  and post-increment polling after each unfinished symmetric pixel
  pair.  Both step caller-owned selected operations.  Standalone inverse
  Julia owns point completion and prepared-queue lifetime;
  `ui::InverseJuliaFractalController` steps that selected operation,
  retaining UI allocation retries, fallback notifications, pre-point
  input handling, and orbit toggling.  Its handler scope ends on
  completion, suspension, or failure; the completed-image picker is
  unchanged.

The input and presentation side starts, resumes, cancels, or paces the
active operation.  It does not classify standard versus Lyapunov versus
non-standard renderers, and it does not own orbit math, pass traversal,
or pixel plotting.

The outer boundary now drives bounded work: `big_while_loop` starts and
steps `ui::ImageRenderContext`, which owns an engine-selected
`ImageRenderOperation` and its stepwise presentation controller.
Renderer classification remains in the engine, not the UI.  Setup,
timing, calculation state, and cleanup remain in calculation objects;
input, sound servicing, markers, diagnostics, and storage preparation
belong to UI contexts.  The synchronous rendering entry points and
`calculate_image()` diagnostic bridge have been removed.

The indirect renderer paths are also closed: `FractalSpecific` has no UI
renderer pointers, calculation front ends do not enter controllers, and
per-image callbacks perform setup only.  Existing work increments and
controller input checkpoints are retained; outer stepping adds no polls.
Further consolidation to a single event-driven input loop remains
separate from the release 1.5 directory boundary.

## GIF File Operation Policy

Treat reading or writing a GIF as one atomic, run-to-completion I/O
operation.  Old DOS keyboard polling compensated for slow LZW work; it is
not a behavior to preserve when replacing the codecs with giflib.

- Remove direct and indirect keyboard polling, pending-input callbacks,
  and user-interruption results from GIF load/save during the conversion.
  Pending keys remain for the outer input loop to handle later.
- Use giflib's scanline APIs for both reading and writing.  The UI owner
  coordinates scanline feedback while completing the entire load/save
  command, without polling or dispatching keyboard input between lines.
  Atomic means non-interruptible by input, not feedback-free.
- During loading, process each decoded scanline for pixel output and
  optional sound before reading the next.  Keep per-pixel sound mapping
  and coordinated plotting; do not replace it with one tone per line.
  No full-image buffer, second traversal, or post-close playback stage
  is required.
- During saving, update on-screen progress after each completed scanline
  and call `driver_flush()` before writing the next.  This is a display
  flush, not a per-scanline disk flush.  Preserve progress restoration
  without modifying pixels that have not yet been saved.
- Keep codec work, pixel traversal, and pixel output below UI.  The UI
  owner services sound through `g_sound`, handles playback backpressure,
  and draws/flushes save feedback between scanlines.  The codecs do not
  poll input, service audio, or invoke UI feedback callbacks.
- Keep mode selection, overwrite decisions, busy-state setup/cleanup,
  error reporting, and completion notifications around file I/O.  Neither
  a codec nor scanline feedback may enter a modal UI or pump input during
  the operation.  Sound servicing may pace loading without making it
  keyboard-interruptible.
- Preserve I/O-error handling, resource cleanup, and temporary-file
  replacement rules.  Atomic here describes control flow, not new
  transactional filesystem guarantees.  Remove keyboard-interrupted
  save/keep-partial choices, not actual read/write failure reporting.
- Preserve saving an incompletely rendered image and its resume metadata.
  An interrupted calculation is not an interrupted GIF write.

The giflib-based codecs implement this boundary.  Scanline feedback is
part of the run-to-completion command; it is not a reason to restore the
old keyboard polling.

## [Sound Output Port](sound.md)

The separate design artifact defines `SoundOutput`, numerical samples,
session ownership, backpressure, and coordinated interactive playback.
Calculation submits directly to an output sink.  Do not thread plotted
coordinates upward through pass and renderer results solely for sound,
or add sound-notification mailboxes to calculation objects.

Follow the [dependency-injection contract](sound.md#dependency-injection):
`SoundOutputPtr` aliases `std::shared_ptr<SoundOutput>`.  The application
installs one shared instance in `g_sound`; calculation calls that
active port directly and UI services the same instance.  Do not thread
sound parameters through calculation call chains, add constructor
injection to every calculation object, or introduce per-pass sound fields
and forwarding functions.  Tests replace and restore this same binding.

Interactive plotting and audio proceed at the slower rate.  The engine
returns when output cannot accept the next coordinated unit; UI owns
waiting and input.  Silent output and recording without device playback
do not acquire playback waits.  Preserve normal synchronization cadence
when the sink is ready.

The UI services non-blocking `SoundOutput::update()` during rendering,
playback waits, and completion.  The implementation releases due voices;
calculation does not call soundoff or service playback.  Normal release
preserves other voices and release tails, whereas explicit `mute()`
immediately silences all voices and resets playback state.  Normal
completion drains release tails rather than muting them away.

No multimedia library has been selected.  `ui::DriverSoundOutput`
optionally records each accepted tone under `OSF_MIDI` before attempting
driver playback.  These sequential behaviors are one implementation, not
two port implementations or a composite.  Its non-blocking `update()`
releases a due tone; explicit mute resets playback immediately.

Do not promise audible continuous sound on drivers that currently return
`false`.  Preserve optional recording regardless of driver capability.
Future audio-file-only output, new audio libraries, WAV encoding, and
changed sound mappings are separate work.

## Orbit Display And Delay Policy

Orbit plotting is engine work done from inside each pixel's orbit
iteration.  The UI layer must not plot or scrub orbit points.  The
inversion work is to remove keyboard and mouse polling buried inside
orbit display mechanics by making the engine return after bounded
plot or scrub work.  The UI controller owns input polling and any
presentation delay between those engine increments.

Current behavior is retained, not a settled replacement for DOS display
semantics.  Standard and FrothyBasin controllers call
`pace_orbit_display()` after reported overlay output.  That UI helper
maps delays below 139 units to no display wait; values of 139 or more
retain 100-microsecond units.  Its input dispatcher is throttled to about
72 Hz.  This is not frame-based retention or batching of orbit points.

`ShowDotController` separately waits for every positive `g_orbit_delay`,
using saved marker pixels and the show-dot color/shape settings.
FrothyBasin still draws its simple `g_show_dot` marker inside
`start_pixel()` and does not use that controller.  The general pixel
kernel retains the legacy sound/delay-dependent chunk choice: 16
iterations with orbit sound and positive delay, otherwise 2048.  That
choice does not poll or wait inside calculation.

The driver-backed sound port separately interprets `g_orbit_delay` as
tone duration in 100-microsecond units.  Its duration is not rounded by
the overlay display threshold.  UI services and waits for playback.

The old DOS delay model may not fit a windowed application, and exact
FRACTINT microsecond compatibility is not necessarily the right goal.
Revisit the entire policy before adding display-delay changes.

The policy review must cover every relevant overlay or orbit-inspection
use:

- standard escape-time overlay display from `showorbit=yes`;
- FrothyBasin overlay display;
- Lorenz and other 2D or 3D orbit plotting paths;
- any remaining fractal type that uses `g_orbit_delay`, `plot_orbit`,
  visible point pacing, or sound output timing.

Questions to answer:

- Is `orbitdelay` still a compatibility parameter, a UI presentation
  cadence, or a legacy value that should be mapped to modern behavior?
- What should `orbitdelay=0`, small positive values, and large values
  mean for windowed display?
- Should visible orbit display use per-point delay, frame-rate-limited
  sampled presentation, single-step mode, or a separate animation UI?
- Should batch or headless rendering ignore visible orbit delay?
- What future sound-duration controls or `orbitdelay` compatibility
  mapping should replace the interim driver adapter's duration setting?

Done when:

- the chosen policy is documented before implementation;
- automated tests specify the chosen timing and state behavior; manual
  arguments and observations cover only remaining native display or input
  gaps, not every affected fractal type;
- new implementation slices separate engine plot work from UI polling,
  without mixing in unresolved delay-policy decisions.

The former shared orbit-plot path had two responsibilities.  They are
now separated:

- Overlay orbit display draws a temporary orbit over an existing image.
  `engine::OrbitPlot` saves covered pixels, XORs the orbit color, and
  later restores the saved pixels.  This is the `showorbit=yes` behavior
  for standard escape-time rendering and FrothyBasin.  Its interface has
  no image-color argument or `color == -1` sentinel.
- Image-orbit plotting makes the orbit point part of the rendered image.
  It writes an explicit color and must not save or scrub covered pixels.
  `Popcorn` owns this plotting directly, without `OrbitPlot`.

Popcorn is not a standard fractal renderer.  It uses screen pixels as
seed points, iterates a Popcorn map, and plots the produced orbit points
elsewhere on the screen.  It does not color the seed pixel by orbit
length, and it does not use StandardPass traversal.  `Popcorn` owns
scan state, orbit state, and image point plotting; its UI controller
advances the selected operation in `NonStandardFractal`.

Popcorn and PopcornJul are not part of the standard overlay-orbit
policy.  They hijacked the old orbit plotting machinery to draw image
pixels, but they are not showing temporary escape-time orbit overlays.
The false dependency on `OrbitPlot` has been removed.  Popcorn is not an
orbit-inspection feature.

The running orbit overlay may eventually be superseded by an orbit tool
window with an interactive cursor in the main image.  That tool would
let the UI request orbit calculation for a selected image point, then
animate, pause, single-step, or play sound independently of image
rendering.  In that future shape, `showorbit=yes` can become
compatibility syntax that opens or enables the orbit tool instead of
forcing rendering to draw, delay, and scrub temporary overlay points.

That direction reinforces the current inversion boundary: image
rendering should not own input polling or UI pacing, and the UI should
not own fractal orbit math.  The engine supplies bounded orbit work; the
UI decides when to request it, how to present it, and how to handle user
input while it is visible.

## Current Presentation Ownership

Calculation setup, memory operations, and command processing report
diagnostics without modal UI or shutdown callbacks.  Standard and
Lyapunov show-dot presentation belongs to `ShowDotController`.
FrothyBasin's simple marker and the general pixel kernel's legacy
sound/delay-dependent chunk choice remain as described above.  Neither
performs keyboard polling or a delay inside calculation.

Command parsing returns diagnostics, generation requests, and the need
for startup file-entry selection.  UI callers own action execution,
selection cancellation, device initialization, and application exits.
Image-save feedback and policy live in `ui/save_image.cpp`, which drives
scanline encoding through `io/ImageSave`.  Image-load setup choices live
in `ui/restore_image.cpp`.  Video selection, 3D settings, and Tab
information use pending values; the active image and stacked screen
remain unchanged when setup is cancelled.

The engine and fractal classes expose bounded rendering facts such as
"overlay point was drawn" or "scrub restored
pixels"; the UI controller decides whether that causes display delay or
input handling.  Sound samples go directly to `SoundOutput`; they are
not another reason to add UI-consumed flags to calculation objects.

Any future presentation-cleanup slice must move behavior to a named UI
controller, output adapter, or appropriately owned backend before
removing the engine-side leak.  Do not merely delete delay, pacing,
sound, mute, or input behavior.  The sound-port slices preserve
`OSF_MIDI` and driver delegation, not the old DOS polling/wait mechanism.

When a slice adds a temporary interface, method, member, or helper only
as a bridge to a later slice, mark the added declaration or definition
with this comment immediately before it:

```cpp
// TODO: removed later
```

The later slice must remove the marked wart.  Permanent interfaces must
not use this marker.

Each slice must fit a focused commit.  Split prerequisite state changes
from ownership changes when either needs independent review.  Do not
combine all diagnostics, all renderer families, or all backend extraction
into one cleanup commit.  Every temporary bridge must name the later
slice that removes it in the plan.

Validation has three complementary layers: image or semantic tests protect
calculation, autokey scripts protect real application interaction, and
controller tests cover precise transitions and injected failures.  A
grep-based test is not a substitute for exercising a callback path.
Source searches remain audit tools, not repository tests.

## Validation Audit

Existing coverage does not imply coverage of live interaction:

- `tests/cmake/AddImageTest.cmake` runs images with `batch=yes` and
  `sound=off`.  Pass, cellular, orbit, IFS, and restoration fixtures
  protect output, not calculation input, transient markers, or playback.
  `ImageTest.rotate` tests geometric rotation, not palette cycling.
- `tests/autokey/CMakeLists.txt` covers parameter/MIG generation, 3D-look
  Targa output, random-dot stereograms, palette cycling, standard
  one/two-pass interruption, and arbitrary-precision pass switching.
  Standard scripts save a resumable checkpoint, invoke `x` and Esc, then
  compare completed pixels exactly with an uninterrupted control for
  optimized and general Mandelbrot paths.
  Palette scripts verify cycling-context saves, single stepping, failed
  map selection, malformed-map warnings, valid-map retry, and return to
  the image.  Pass-switch scripts select BoundaryTrace and
  solid guessing through `x`, verify saved pass and precision metadata,
  and compare pixels exactly with fresh renders for `perturbation=auto`
  and `no`.  The show-dot script uses `ROWWAIT` to cross row boundaries,
  saves interrupted checkpoints around menu cancellation, and verifies
  the Maximum Iterations restart against a fresh render.  These scripts
  do not exercise the stereo photographer prompt.
  The cellular script delivers its interrupt only while the preparation
  thinking screen is active, saves a non-resumable checkpoint, then
  starts generation-zero rendering and compares it with a fresh image.
  The L-system script interrupts the sizing thinking screen, returns
  through the outer menu, and starts a completed drawing at a new order.
  Its validation script rejects an invalid definition, acknowledges the
  error, selects a valid definition, and verifies completed drawing.
  Cancelling another invalid selection preserves that completed image.
  Batch checks require one diagnostic and no saved image for rejected
  definitions.  These checks use metadata and blank/nonblank output,
  not drift-sensitive line goldens.
  The IFS script acknowledges a missing definition, rejects malformed
  selection, chooses a valid definition, and preserves the completed
  image when another invalid selection is cancelled.  Batch checks
  require one diagnostic and no saved image for each rejected definition.
  It verifies metadata and blank/nonblank output, not rasterized lines.
  The 2D and 3D IFS interruption scripts select the definition and type
  through the command UI, deliver `x` after 512 IFS iterations during
  active rendering, cancel the menu, and save a non-resumable checkpoint.
  Changing the iteration limit restarts calculation; saved parameters,
  completion state, and pixels are checked against a fresh seeded image.
  IFS does not support calculation resume.
  The PHOTO-mode IFS and Lorenz 3D scripts remain interaction regressions.
  They reach the first completed eye, acknowledge its notice, save from
  the photographer handler, and continue to the right eye.  Separate
  journeys complete or interrupt that eye after 512 iterations, cancel
  the options screen, and return to image commands.  Checkpoints verify
  type, PHOTO mode, iteration limit, status, dimensions, and nonblank
  output.  They do not compare drift-sensitive orbit pixels.
  The formula script acknowledges a missing definition, rejects invalid
  syntax, selects a valid formula, and preserves the completed image
  when another invalid selection is cancelled.  Batch checks reject
  missing, malformed, and empty formulas without saving an image.
  The Lyapunov setup script dismisses the inside-method warning with a
  key that would otherwise open options, verifies completed rendering
  with the substituted numeric color, then changes the inside color and
  compares the restarted image with a fresh calculation.
  The JIIM script checks picker movement and exact-point selection through
  saved Julia parameters, then reopens the orbit window around a menu and
  verifies restored Mandelbrot pixels.  It protects the UI-owned tool's
  behavior, not native cursor presentation.
- Keyboard-handler tests cover dispatch, interruption flags, and key
  requeueing.  Standard controller tests also cover mid-orbit suspension,
  continuation, scanline resume positions, and perturbation/Lyapunov gates.
  Palette controller tests verify speed and pause state without timing,
  plus file-selection failure, acknowledgement, and retry.
  Loader tests verify missing-file preservation, partial malformed-map
  loading, returned warnings, and legacy quantization without a driver.
  Thinking-screen tests cover text, spinner updates, and active state.
  Cellular controller tests cover preparation, transition to pixel
  output, exactly-once cleanup, interruption, and injected failure.
  L-system calculation tests cover bounded sizing, suspension, and
  transition to drawing without UI calls.  Parser tests cover missing
  definitions, malformed commands, missing axiom and angle operands,
  invalid angles, stale-definition cleanup, and successful reload after
  failure.  Controller tests cover thinking-screen entry, updates,
  exactly-once cleanup, interruption, initialization failure, and injected
  calculation/presentation failure.
- Formula parser tests run without a UI driver.  They cover source and
  caret diagnostics, syntax and jump failures, recoverable symmetry
  warnings, file cleanup, failed-program cleanup, successful recompilation,
  and parameter inspection.
- Lyapunov and Julibrot setup tests run without a UI driver.  They cover
  supported options, warning delivery, legacy normalization order, and
  rejection before Julibrot initialization.  Current display and disk
  drivers accept only 256-color modes, so Julibrot's low-color failure is
  covered directly rather than through an unreachable autokey path.
- Perturbation tests cover reference failure and replacement without
  retained history, caller-selected pixel order, bounded reference and
  pixel increments, deep-zoom BoundaryTrace completion, and palette-color
  consistency.  They also verify BoundaryTrace strategy selection.
  Pass-switch tests verify automatic strategy selection, disabled
  perturbation, fresh reference state, dispatcher restoration, and
  big-number stack cleanup between traversals.  Current precision tests
  cover bailout equality and tiny offsets on either side of the boundary,
  including Mandelbrot near `-2`.  Coloring, periodicity, inversion,
  synchronous-orbit, sticky-orbit, and output-backpressure regressions
  cover the later perturbation extensions.
- Non-standard operation tests cover 2D orbit, dynamic, inverse-Julia,
  and Standard4D selection, setup, completion, suspension, and queue
  ownership.  Autokey menu-return journeys compare Henon, dynamic,
  MandelCloud, inverse Julia, and Julibrot output with uninterrupted
  renders.  Non-resumable inverse Julia and Julibrot explicitly restart
  with Enter after cancelling the menu; resumable types continue.
- `TestStereo` covers random-line generation.  PHOTO-mode autokey tests
  cover calculated-view save/continue/interruption and restored-view
  save/advance with Enter or Escape.  `TestStereoView` and
  `TestStereoRestore` cover eye selection, separate GIF decodes, failure
  status, shared cleanup, and full-screen restoration without input
  polling during decode.
- `TestShowDotController` covers exact point and triangle restoration,
  symmetry copies, unfinished pixels, zero-delay batches, menu suspension,
  and restart.  Autokey verifies row progress and checkpoint contents;
  neither establishes transient native marker presentation.
- Mock drivers and timer callbacks do not exercise a real window's event
  queue, physical mouse tracking, or visible refresh cadence.  Current
  Win32 and X11 drivers do not support continuous tone playback; require
  fake-port, mock-driver, and tone-log tests, not audible manual checks.

The coverage described above is implemented.  Future slices should
automate menu results, preparation-screen lifetime, pass switching, and
failure cleanup rather than assigning them unspecified manual checks.

Manual checks are reserved for JIIM cursor presentation, show-dot
presentation, and native input during busy rendering.  Report any
still-required check when completing the corresponding slice.
If a slice adds automation for that same observation, remove the manual
requirement.  Do not repeat it for an unchanged caller or each fractal.

### Autokey Regression Rules

- Use `tests/autokey` for real application journeys through the driver-key
  interface, active handlers, menus, and render controllers.  Establish
  the relevant baseline script before changing its control flow, then
  retain it as a regression.  Record known defects separately; a test
  must not silently redefine the intended behavior around an old bug.
- Trace the actual context and key bindings before writing a script.
  Do not assume a control supported by one fractal applies to another.
  Reuse scenarios across slices; add cases for distinct interaction paths,
  not every fractal sharing the same controller or unchanged image test.
- Assert meaningful outcomes: selected parameters, generated files,
  completion, return to the outer UI, and relevant intermediate state.
  A correct final image alone does not prove a menu opened during work;
  its keys might have been processed only after calculation completed.
- For an in-progress interruption claim, observe the active calculation
  or preparation phase when the key is handled, the menu/cancel result,
  and the subsequent continuation or restart.  Use controller tests for
  exact yield points, resume bookkeeping, and handler lifetime as well.
- `CALCWAIT` waits for completion.  `THINKWAIT <key>` emits a character or
  named key only while a thinking screen is active; it does not assert
  arbitrary calculation phases.  `ITERWAIT <iteration> <key>` requires
  active calculation and the type-specific `g_color_iter` threshold;
  IFS increments that counter for each generated point.  Do not assume
  it counts image progress for other types.  If another intermediate
  observation is unavailable, add narrowly scoped UI/harness support in
  a separate prerequisite commit, or keep that assertion in a controller
  test and limit the script's claim accordingly.
- Prefer `CALCWAIT` for completion.  Fixed `WAIT` delays, a large workload,
  or a slow CI machine do not prove that input arrived during work.
  Give scripts bounded process timeouts so lost keys or hangs fail.
- `FAST` disables autokey's visual typing delay for the active script,
  including after `WAIT` or `CALCWAIT`.  Use it before phase-gated input
  so typing delays cannot defer an interruption until work has finished.
  It returns no key; a directive at a modal boundary also ends keyahead
  flushing before the script acknowledges the notice.  Normal scripts
  retain their existing typing delays unless they request `FAST`.
  Do not increase polling or yield frequency to make a script pass.
- Use isolated test homes, explicit save locations, deterministic inputs,
  and fresh artifacts.  Compare resumed output with an uninterrupted
  control where pixels are stable, reusing existing golds when suitable.
  Use semantic results for L-systems and other drift-sensitive types;
  do not add unstable image golds to test keyboard behavior.
- Scripts exercise injected keys, not native keyboard delivery, mouse
  tracking, blinking, refresh cadence, or responsiveness.  Keep the
  narrowly scoped manual observations above.  Do not assert exact poll
  counts, elapsed render times, or the old location of an input check.
- Keep calculation/codec tests and injected-error controller tests where
  a script cannot reliably exercise resource exhaustion or cleanup.
  GIF load/save remains atomic; autokey may test surrounding choices,
  not introduce or expect mid-file keyboard cancellation.

## Current Engine Boundary

The synchronous engine-to-UI render dispatch has been removed.
Non-standard operation selection and per-image setup no longer enter
renderers or UI.  Engine/fractal sources and headers no longer include
UI headers.  Parameter ownership and defaults are separated:

- `VideoInfo` is key-free.  UI owns the separate `g_video_mode_keys`
  associations, the selected-entry binding, key lookup, key-name parsing,
  and binding-name formatting.  Engine retains disk/display mode
  validation with no UI key dependency.  UI owns the `video` handler and
  pending request `g_init_mode`, including its storage and
  initialization/completion policy.
- Stereo calculation inputs and `CalibrationBars` are geometry-owned.
  Automatic conversion and texture-filename reuse remain UI policy.
  UI owns both the `rds` and `rds-texture` handlers; engine parameter
  handling no longer depends on UI stereo policy.
- UI owns `exitnoask`, `g_escape_exit`, and combined fractal-default
  initialization for startup, fallback, and explicit `reset` commands.
  Engine owns its defaults and reset-version validation/application.
  Accepted resets take effect inline, once, before the next command.
- GIF evolution records and comparisons are I/O-owned; public GIF metadata
  headers no longer import UI.  `io/loadfile.cpp` explicitly includes
  `ui/evolve.h` for its existing application of evolution runtime state.

GIF codec operations do not poll input or support keyboard-interrupted
saves.  Image-save progress, notifications, and exit policy are UI-owned;
`io/ImageSave` owns encoding and file cleanup.  Image-load setup choices
and cancellation are UI-owned and do not commit pending image parameters.

Command parsing reports errors, warnings, generation actions, and startup
selection requests without UI calls.  The command UI owns their effects;
MIG failures unwind to it rather than exiting from I/O code.

The implementation slices for the release 1.5 boundary are complete.
Future source audits must still trace UI helper and dispatch-pointer
calls as well as raw driver calls; an empty raw-key search alone does not
establish inversion.

This is not a claim that every non-UI module is free of UI state or
headers.  Current backend dependencies include:

- `geometry/line3d.cpp` includes `ui/big_while_loop.h` to register its
  output-cleanup function in `g_out_line_cleanup`.
- `io/encoder.cpp` and `io/ImageSave.cpp` include the big-loop and
  evolution headers.  `ImageSave` reads UI-owned evolution state when
  constructing metadata.
- `io/loadfile.cpp` includes evolution and parameter-file UI headers;
  applying saved extensions restores evolution runtime state and genes.
- `io/load_config.cpp` uses UI key-name parsing and writes the separate
  UI-owned video-mode bindings while loading the mode catalog.
- `io/save_timer.cpp` uses UI-owned `g_finish_row` and `read_ticker()` for
  autosave scheduling.

These dependencies do not poll keyboard or mouse input.  They are
distinct from the completed engine/fractal input boundary and from any
future cleanup of metadata, configuration, or autosave ownership.

The `OrbitPlot` wait loop, delay state, and sound mapping have been
removed.  It submits numerical samples through `g_sound`; standard and
FrothyBasin UI controllers own sound servicing and session completion.

## Keyboard Polling Still Outside `id_main`

The current release 1.5 boundary is narrower than the final
event-driven design.  For release 1.5, the key rule is that only
`libid/ui` talks directly to keyboard and mouse I/O.  The stricter
future target is that `id_main` or the top application loop is the only
code that polls the driver key queue.

To reach the stricter target, split keyboard dispatch from keyboard
polling:

- `KeyboardHandler.cpp` should hold the handler stack and interruption
  state, and dispatch keys that are passed to it.
- `KeyboardHandler.cpp` should not call `driver_key_pressed()` or
  `driver_get_key()`.
- The top application loop should read driver keys and feed them to the
  handler stack.
- Render controllers already expose bounded stepping.  Their input waits
  and modal interactions still need event-driven phases before the top
  loop can be the sole driver-key poller.

Simple render controllers that already wrap calculation objects but
still poll the driver queue directly:

- `libid/ui/BifurcationController.cpp`
- `libid/ui/CellularController.cpp`
- `libid/ui/IFS2DController.cpp`
- `libid/ui/LSystemController.cpp`
- `libid/ui/PlasmaController.cpp`
- `libid/ui/Standard4DController.cpp`
- `libid/ui/TestPointController.cpp`

Controllers with bespoke interaction or partial handler-stack use that
still read or write the driver queue directly:

- `libid/ui/AntController.cpp`
- `libid/ui/inverse_julia.cpp`
- `libid/ui/ImageRenderContext.cpp`
- `libid/ui/stereo.cpp`

Handler-based render controllers that still rely on
`KeyboardHandler.cpp` polling internally:

- `libid/ui/DiffusionController.cpp`
- `libid/ui/FrothyBasinController.cpp`
- `libid/ui/InverseJuliaFractalController.cpp`
- `libid/ui/LyapunovController.cpp`
- `libid/ui/OrbitController.cpp`
- `libid/ui/PopcornController.cpp`
- `libid/ui/SoundSession.cpp`
- `libid/ui/StandardFractalController.cpp`
- `libid/ui/standard_orbit_plot.cpp`
- `libid/ui/StereoController.cpp`

Blocking text screens, modal prompts, and utility UI paths also poll
directly.  They can remain for the release 1.5 directory boundary, but
must become application phases or dialogs before `id_main` can be the
only keyboard poller:

- `libid/ui/big_while_loop.cpp`
- `libid/ui/command_diagnostics.cpp`
- `libid/ui/editpal.cpp`
- `libid/ui/file_get_window.cpp`
- `libid/ui/flip_image.cpp`
- `libid/ui/full_screen_choice.cpp`
- `libid/ui/full_screen_prompt.cpp`
- `libid/ui/get_key_no_help.cpp`
- `libid/ui/help.cpp`
- `libid/ui/intro.cpp`
- `libid/ui/input_field.cpp`
- `libid/ui/main_menu.cpp`
- `libid/ui/main_menu_switch.cpp`
- `libid/ui/rotate.cpp`
- `libid/ui/slideshw.cpp`
- `libid/ui/starfield.cpp`
- `libid/ui/stop_msg.cpp`
- `libid/ui/temp_msg.cpp`
- `libid/ui/thinking.cpp`
- `libid/ui/zoom.cpp`

## Future Event-Driven Work

The following ideas are outside the completed release 1.5 input boundary.
Input polling is UI-owned, but the application is not yet event-driven.

- Convert `Frame`, `WinText`, and `Plot` to wxWidgets controls while the
  current polling code still pumps GUI events.
- Add menu-bar support, with blocking text screens still preventing normal
  menu interaction until they are migrated.
- Convert blocking text screens to modal dialogs incrementally.  Start
  with simple numeric input, then form-style prompts, then file-entry
  screens.
- After text screens are dialogs, remove the menu-bar blocking workaround.
- Replace polling-driven calculation with idle-processing callbacks first.
  Treat idle processing as a bridge to later worker-thread or distributed
  rendering.
- Plan worker-thread and UI-thread communication as separate follow-up
  work.  Do not mix thread ownership, cancellation, or result delivery
  into this polling-boundary refactor.
- Replace driver polling in the UI dispatcher with GUI-thread event
  dispatch to the same handler stack.
- Let event callbacks set calculation-interrupt state through the active
  handlers once the engine runs on a worker thread.
- Build a tool interface for image interaction.  A tool should receive
  mouse, keyboard, and modifier-key events, not only `MouseNotification`
  calls.
- Support a tool stack with a permanent default zoom tool.  Events should
  go to the top tool, with optional fall-through to lower tools.
- Allow multiple zoom tools over time, such as current zoom-box behavior,
  XaoS-style continuous zoom, or ManpWIN-style controls.
- Preserve accurate zoom-box area selection.  It is a required
  interaction, not an artifact to discard when adding other zoom modes.
- After blocking screens and tools own input, remove the `Driver` keyboard
  API, custom message-pumping path, and custom event-loop workarounds.

## Release 1.5 Boundary Checks

- Top-level image rendering is requested through `libid/ui`, not through
  an engine function that selects UI control paths.
- Standard escape-time rendering is driven by a UI polling loop around
  `StandardFractal`.
- `StandardFractal` owns standard renderer iteration, mode dispatch, and
  resume state.
- `libid/engine` and `libid/fractals` do not include headers from
  `libid/ui`.
- This keyboard-input audit returns no matches:

```text
rg "driver_(get_key|key_pressed|wait_key_pressed|unget_key|key_cursor)" \
  libid/engine libid/fractals libid/geometry libid/io libid/math \
  libid/misc
```

- `rg "MouseNotification|mouse_subscribe|mouse_unsubscribe" \
  libid/engine libid/fractals libid/geometry libid/io libid/math \
  libid/misc` returns no matches.
- `rg "g_look_at_mouse|g_cursor_mouse_tracking" \
  libid/engine libid/fractals libid/geometry libid/io libid/math \
  libid/misc` returns no matches.
- `libid/ui` is the only libid layer with direct keyboard and mouse I/O.
- Calculation setup, sizing, iteration, suspension, completion, and error
  handling cannot reach UI polling indirectly through `thinking()`,
  cursor waits, modal messages, or renderer dispatch pointers.
- Calculation and backend operations return failures and warnings to UI
  owners without selecting prompts or calling `goodbye()`.
- JIIM and stereo interaction are driven from UI controllers around
  bounded calculation operations.
- Pixel plotting, image storage, and calculation timing stay below UI;
  their backend operations do not own input or presentation decisions.
- Calculation may submit numerical samples directly to `SoundOutput`.
  The application injects one `SoundOutputPtr` through the active output
  binding, not through calculation call-chain parameters or per-pass
  fields; UI services that same instance.
  One concrete implementation preserves optional `OSF_MIDI` recording
  followed by driver output, without key polling or playback waits inside
  the port.  Interactive UI advances plotting and audio together at the
  slower rate; driver refusal does not delay or suppress recording.
- UI services non-blocking `update()` while rendering, waiting, and
  completing.  Normal voice release and release-tail completion do not
  use mute; interruption explicitly silences and resets playback.
- Standard and Lyapunov show-dot marker state belongs to UI, with no
  temporary marker bridge in their calculation paths.  FrothyBasin still
  draws its simple marker in calculation; it does not poll or wait there.
- Audit source calls and reachable dispatch targets in both release and
  debug configurations.  Ignore comment-only search matches; do not add
  grep-based tests that merely enforce source spelling.
- Autokey regressions protect changed keyboard journeys through the real
  application.  In-progress claims have intermediate evidence; controller
  tests cover precise transitions and failure cleanup, while native input
  and transient presentation retain their narrow manual observations.
- Calculation code reports interruption or decisions without owning input.
- Current behavior is preserved while longer-term architecture stays out
  of scope.
