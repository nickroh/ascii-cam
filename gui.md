# AGENTS.md --- Rust egui Virtual Camera Controller

## Project Goal

Build a small Windows desktop controller in Rust using `eframe` and
`egui` for an existing virtual-camera implementation.

The app should have a minimal, CLI-inspired interface and control: -
Virtual camera ON/OFF through the existing C++ trigger. - Runtime video
filter selection through Windows shared memory. - Clear status and error
reporting.

An existing C++ trigger and camera DLL already exist. Reuse them. Do not
rewrite the camera implementation or rebuild the DLL unless required by
an identified compatibility issue or an explicit request.

## Confirmed Architecture

-   **GUI/controller:** Rust + `eframe`/`egui`.
-   **Camera lifecycle:** invoke the existing C++ trigger for
    start/stop.
-   **Shared-memory creator and owner:** the Rust application.
-   **Shared-memory consumer:** the C++ camera process/DLL.
-   **Filter changes:** Rust writes a filter selection to shared memory;
    C++ reads it and applies the corresponding filter.
-   **Platform:** Windows.
-   **Visual direction:** compact, dark, minimal, CLI-inspired.

### Data Flow

1.  Rust initializes the shared-memory control block.
2.  Rust starts or stops the camera using the existing C++ trigger.
3.  When the user changes the filter, Rust updates the shared-memory
    control block.
4.  C++ reads the shared-memory value and updates the active filter.
5.  The UI distinguishes the requested state from the confirmed runtime
    state whenever confirmation is available.

## Shared-Memory Contract

The Rust app creates the named shared-memory object using Windows APIs,
such as `CreateFileMappingW`, and maps it with `MapViewOfFile`. The C++
side opens the same named object and maps it for reading.

Before implementation, inspect the existing C++ code and agree on: -
Exact, stable mapping name, including whether the `Local\\` or
`Global\\` namespace is needed. - Exact mapping size and byte layout. -
Filter ID values and meaning. - Initialization behavior and how C++
detects that the control block is ready. - Synchronization and
memory-ordering requirements. - Whether the mapping persists while
either process remains alive. - What happens if the GUI restarts while
the camera process is still running. - How C++ reports that a requested
filter was actually applied, if confirmation is required.

Do not invent or silently change the mapping name or binary layout if
the C++ implementation already expects specific values. Update both
sides together when changing the contract.

### Suggested Initial Protocol

Use a small, fixed-layout control block rather than strings or
Rust-specific data structures. A conceptual layout could contain: -
Protocol version. - Initialization/readiness marker. - Requested filter
ID. - Optional applied filter ID or acknowledgement counter. - Optional
error/status field.

This is a design proposal, not a claim about the existing DLL's
implementation. Confirm field widths, alignment, atomicity, and offsets
before coding. Avoid placing Rust `String`, `Vec`, references, or other
process-local pointers inside shared memory.

Prefer fixed-width integer fields such as `u32` and a documented
C-compatible layout. If concurrent access is possible, use an explicitly
designed synchronization mechanism or platform-supported atomic
operations that are compatible across Rust and C++; do not assume that
arbitrary concurrent reads and writes are automatically safe. A
mutex/event may be appropriate if the existing C++ side supports it.
Keep the hot video-processing path non-blocking where possible.

### Creator and Initialization Rules

-   Rust is responsible for creating and initializing the mapping.
-   C++ should open the existing mapping and consume its contents; it
    should not independently initialize or overwrite Rust-owned control
    fields.
-   Ensure C++ does not read uninitialized or partially initialized
    values. Define a readiness protocol.
-   Decide how the controller handles an already-existing mapping,
    including whether it is a newly created mapping or an existing
    object.
-   Define restart behavior explicitly. If the Rust app exits while C++
    remains alive, the mapping object can remain available while another
    process retains a handle, but a fresh Rust process must handle that
    case safely.
-   Do not rely on `CreateFileMappingW` always creating a brand-new
    object; Windows may return an existing object with the same name.
    Check the creation result appropriately and validate
    protocol/version before overwriting data.
-   Use correct cleanup for mapped views and handles. Unmap the view and
    close the Rust handle on shutdown; do not attempt to forcibly
    destroy a mapping still in use by another process.

## Filter Behavior

Represent supported filters with a Rust enum, for example: - `None` -
`Ascii` - `Edge`

Only expose filters actually supported by the C++ implementation. The
names above are examples; inspect the existing filter implementation for
the real supported set.

Use a stable numeric mapping, documented in both languages. For example,
`0 = None`, `1 = ASCII`, `2 = Edge` is acceptable only if both sides
agree.

When the user selects a filter: 1. Update the requested filter in Rust
state. 2. Write the agreed filter ID to shared memory. 3. Let the C++
side detect and apply the change. 4. If the protocol supports
acknowledgement, show the applied filter separately from the requested
filter. 5. Handle unsupported IDs or backend errors without pretending
that the filter was applied.

Filter changes should not restart the camera unless the existing
implementation specifically requires it. The intended design is runtime
switching through shared memory.

## Camera ON/OFF Behavior

-   Reuse the existing C++ trigger for camera start and stop.
-   Inspect how the trigger accepts commands: executable arguments,
    standard input, named IPC, exported functions, or another mechanism.
-   Do not assume it is a short-lived command or a long-running process
    until the code is inspected.
-   Avoid launching duplicate camera instances.
-   Handle start/stop errors and unexpected process termination.
-   Disable or guard controls during transitional states when
    appropriate.
-   Do not equate a button click with a confirmed camera state.
    Represent states such as `Off`, `Starting`, `Running`, `Stopping`,
    and `Error` if the trigger provides enough information.
-   On application shutdown, do not accidentally leave a camera process
    running unless that is the established intended behavior.

## UI Design

Keep the UI minimal and CLI-inspired: - Dark, low-contrast neutral
background. - Monospace font where practical. - One restrained accent
color. - Compact labels and separators. - No unnecessary cards, shadows,
gradients, animations, or decorative icons. - Clear ON/OFF state. - A
compact filter selector, such as a `ComboBox` or selectable list. -
Optional concise status/error line.

Suggested controls: - `Virtual Camera`: ON/OFF. - `Filter`: supported
filter selection. - `Status`: actual state when known. - Error details
only when needed.

Use standard egui widgets first: - `toggle_value` or `checkbox` for
Boolean state. - `ComboBox` and `selectable_value` for one-of-many
filter selection. - `separator` for grouping. - `horizontal` for compact
rows. - `ScrollArea` only if content requires it.

## Recommended Rust Structure

Start with the smallest structure that fits the existing codebase. A
possible layout is:

``` text
src/
├── main.rs
├── app.rs
├── shared_memory.rs
├── camera_controller.rs
├── settings.rs
└── ui.rs
```

Responsibilities: - `main.rs`: initialize and launch `eframe`. -
`app.rs`: application state and `eframe::App::update`. -
`shared_memory.rs`: Windows mapping creation, mapped access,
synchronization, and cleanup. - `camera_controller.rs`: invoke and
supervise the existing C++ trigger. - `settings.rs`: filter enum and
protocol constants. - `ui.rs`: rendering helpers if `app.rs` becomes too
large.

Do not split files merely to match this example if the current project
is still small. Preserve the project's existing conventions where
sensible.

## Dependencies and Windows API

-   Use `eframe`/`egui` for the GUI.
-   Use the `windows` crate for Windows APIs if appropriate.
-   Keep crate versions compatible with the project's current Rust
    toolchain.
-   Enable only the Windows API features actually needed.
-   Common APIs may include `CreateFileMappingW`, `MapViewOfFile`,
    `UnmapViewOfFile`, and `CloseHandle`; the C++ consumer uses
    `OpenFileMapping` and `MapViewOfFile`.
-   Verify function signatures, handle types, error handling, and API
    feature names against the selected `windows` crate version.
-   Do not write a guessed API wrapper without compiling and testing it.

## Error Handling and Diagnostics

Provide concise, useful errors for: - Shared-memory creation or mapping
failure. - Protocol version or layout mismatch. - Camera trigger
launch/start/stop failure. - Unknown or unsupported filter ID. - C++
consumer unable to open the mapping. - Camera process termination or
filter application failure, if detectable.

Do not silently ignore failures. Do not report `Running` or an applied
filter without evidence from the trigger or protocol.

## Implementation Workflow for AI Coding Assistants

1.  Inspect the repository, `Cargo.toml`, existing C++ trigger, DLL
    exports, and filter-selection code.
2.  Identify the current ON/OFF mechanism and the actual supported
    filters.
3.  Locate any existing shared-memory name, struct layout, and
    synchronization logic. If none exists, define a cross-language
    contract before implementing both sides.
4.  Implement Rust-owned shared-memory creation and initialization.
5.  Implement or update the C++ consumer to open and read the same
    mapping.
6.  Connect the egui filter control to the shared-memory write path.
7.  Connect ON/OFF to the existing C++ trigger.
8.  Add lifecycle handling, state reporting, cleanup, and meaningful
    errors.
9.  Build the Rust app and C++ component, then test the two processes
    together on Windows.
10. Summarize modified files, protocol decisions, build/run commands,
    and any unverified behavior.

## Acceptance Criteria

-   The app compiles on the intended Windows environment.
-   Rust creates and initializes the shared-memory mapping.
-   C++ opens the same mapping and reads the agreed layout.
-   Changing the UI filter updates shared memory and causes the C++ side
    to apply a supported filter without restarting the camera, if
    runtime switching is supported by the processing code.
-   ON/OFF invokes the existing camera trigger correctly.
-   Actual status is reported honestly; errors are visible.
-   Shared-memory access is synchronized and the layout is documented.
-   Resources are released safely during shutdown.
-   The UI remains compact, readable, and CLI-inspired.
-   Existing DLL functionality is preserved unless a specific change is
    necessary.
