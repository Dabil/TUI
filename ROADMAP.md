TUI MASTER ROADMAP
REVISED: 03-17-26 DETAILED ROADMAP
========================================

PROJECT GOAL
Build a clean, extensible Text UI engine that:

1. feels natural to author in C++
2. supports page-building without Visual Studio later
3. preserves the best ideas from the old TUI "Rogue-Like" Engine
4. avoids baking old API mistakes into the new architecture
5. supports future windows, widgets, menus, panels, popups, and alternate renderers

The engine should be built in layers so that later features are built on stable foundations instead of temporary solutions.



============================================================
0. CORE DESIGN PRINCIPLES
============================================================

These principles govern the whole roadmap.

0.1 Keep the renderer core low-level and Unicode-native
- ScreenBuffer must remain a logical cell grid (not a text engine)
- Internal text representation must use:
  - char32_t
  - std::u32string
- The renderer operates on display cells, not bytes or UTF-8 strings
- Rendering/presentation must stay separate from composition and layout
- Platform-specific code belongs strictly in backend implementations

----------------------------------------

0.2 Separate text model, layout, and encoding layers
- The system must clearly separate:
  1) Internal text model (U32 code points)
  2) Text layout (width, grapheme segmentation, wrapping)
  3) Backend encoding (UTF-8, UTF-16, etc.)
- Encoding conversion must be centralized in a dedicated UnicodeConversion layer
- No UTF-8/UTF-16 logic should exist inside:
  - ScreenBuffer
  - widgets
  - layout/composition systems
- Backends consume already-laid-out cells and handle only transport encoding

----------------------------------------

0.3 Treat text as display cells, not characters or bytes
- The engine must be cell-aware, not byte-aware
- One code point does not always equal one cell
- The system must support:
  - width 0 (combining marks)
  - width 1 (standard characters)
  - width 2 (CJK, emoji by policy)
- ScreenBuffer stores final placed cells only
- Text layout systems determine how code points map to cells

----------------------------------------

0.4 Keep ScreenBuffer simple, but correct
- ScreenBuffer is responsible for:
  - storing cells
  - bounds safety
  - overwrite behavior
  - placement of text runs
- ScreenBuffer is NOT responsible for:
  - grapheme segmentation
  - encoding conversion
  - font capability decisions
- Cells may include lightweight metadata (e.g., continuation / wide trailing)
- The buffer must preserve visual correctness for wide and combining characters

----------------------------------------

0.5 Build authoring API on top of the rendering core
- Do not force page-building behavior into ScreenBuffer
- Build a PageComposer / PageBuffer layer on top
- That layer is responsible for:
  - layout
  - alignment
  - wrapping
  - higher-level text behavior
- This keeps the core reusable and predictable

----------------------------------------

0.6 Build interpreter on top of the same API
- The interpreter must not be a separate rendering engine
- It must call the same PageComposer and text APIs as C++
- This guarantees consistent behavior between:
  - programmatic UI
  - interpreted UI
- Unicode behavior must be identical across both paths

----------------------------------------

0.7 Preserve intent, not implementation quirks
- Legacy APIs (ANSI-style, ASCII assumptions) are references only
- Preserve:
  - authoring ergonomics
  - semantic meaning
- Redesign:
  - ASCII-only assumptions
  - byte-oriented APIs
  - renderer-driven text decisions
- Unicode correctness takes priority over backward quirks

----------------------------------------

0.8 Use explicit compositing semantics
- Avoid unclear naming like:
  - writeTransparentObject
  - writeFullyTransparentObject
- Use clear compositing policies:
  - overwrite
  - merge
  - preserve
- Compositing must operate on fully-formed cells, not raw text

----------------------------------------

0.9 Keep backend renderers pure and dumb
- Backends (e.g., ConsoleRenderer) must:
  - render already-laid-out cells
  - map styles to platform output
  - handle encoding conversion (e.g., UTF-16 for Win32)
  - skip continuation cells
- Backends must NOT:
  - determine character width
  - perform grapheme segmentation
  - decide layout or wrapping
- Backend limitations must be exposed via capability reporting

----------------------------------------

0.10 Design for real-world terminal limitations
- Unicode correctness in the engine does not guarantee correct display
- Rendering depends on:
  - terminal host (conhost, Windows Terminal, etc.)
  - font support
  - fallback behavior
- The system must expose backend capabilities (e.g., emoji support, combining marks)
- Higher-level systems may adapt behavior based on these capabilities

----------------------------------------

0.11 Build foundations in the correct order
- rendering (cell grid + Unicode correctness)
- style system
- text layout (width + grapheme awareness)
- object model
- compositing
- page API (PageComposer)
- interpreter
- input/events/focus
- panels/windows
- widgets
- polish/backends



These principles align with the original roadmap emphasis on foundational layers first, clean separation of screen composition from output, and building reusable systems before higher-level UI features. 

============================================================
UPDATED GLOBAL RULE (CRITICAL)
============================================================

The engine must follow this invariant:

- Input boundary: UTF-8
- Internal model: UTF-32 (char32_t / std::u32string)
- Layout: grapheme + width aware
- Storage: ScreenCell grid
- Output: backend-specific encoding (UTF-16 for Win32)

NO EXCEPTIONS.

FORMAT SUPPORT POLICY

1. Unified object rule
- All reusable loaded/generated visual text content must normalize into TextObject

2. Loader/exporter split
- Parsers/loaders interpret source formats
- Exporters/save systems write supported target formats
- Neither changes the rendering core model

3. Encoding boundary rule
- Text file input must decode at the boundary
- Internal model remains UTF-32
- Legacy code-page handling belongs only in conversion/loader utilities

4. Terminal-art rule
- ANSI-style sources are imported as structured glyph/style data
- Do not treat raw escape replay as the engine’s primary representation

5. Lossy export rule
- Saving/exporting to older formats is allowed only with explicit approximation/clamping behavior
- Never silently discard unsupported style or layout semantics

6. Future declarative format rule
- JSON / YAML / INI and similar structured formats belong to later phases as page/layout/widget declarations
- They must compile down to the same engine APIs, not create a second UI system

7. Future animation format rule
- Multi-frame text/ANSI formats belong in animation-oriented phases once timing/invalidation systems exist


============================================================
1. DEPENDENCY GRAPH
============================================================

This is the actual dependency order for implementation.

1. Core types
   ↓
2. Rendering core
   ↓
3. Style system
   ↓
4. Object/asset system
   ↓
5. Compositing / WritePolicy system
   ↓
6. PageComposer API 1.0
   ↓
7. Screen templates + regions + alignment
   ↓
8. Page interpreter
   ↓
9. Application input/event system
   ↓
10. Focus system
   ↓
11. Layering / surfaces / compositor
   ↓
12. Panels / popups / windows
   ↓
13. Menu system
   ↓
14. Widget system
   ↓
15. Scrolling / viewports
   ↓
16. Animation / timers
   ↓
17. Alternate render backends
   ↓
18. Tooling / page editor / advanced scripting

The roadmap below follows this order intentionally.



============================================================
2. PHASE 1 — CORE TYPES AND RENDERING FOUNDATION
============================================================

PURPOSE
Build the stable, Unicode-correct rendering core that everything else depends on.

WHY THIS PHASE COMES FIRST
Without this phase, every later system becomes harder to design and may require refactoring. This phase establishes:
- logical screen representation
- diff-based redraw
- strict separation of rendering vs composition
- a Unicode-native internal text model

This ensures all future systems operate on correct display semantics rather than ASCII assumptions.

IMPLEMENT IN THIS PHASE

2.1 Core types
- Point
- Size
- Rect
- Alignment enums
- orientation / anchor enums if needed
- basic utility enums for composition and layout

2.2 ScreenCell
Each cell should be able to represent:
- glyph (char32_t)
- style/theme reference
- cell metadata for display correctness (priority, flags, etc.)

Required metadata:
- CellKind:
  - Empty
  - Glyph
  - WideTrailing
  - CombiningContinuation

  Goal:
- support wide characters and combining marks without breaking layout
- remain lightweight and value-type

2.3 ScreenBuffer
Responsibilities:
- logical width/height
- resize
- clear
- get/set cell (cell-safe)
- inBounds
- Unicode-native writing
- text placement (not text parsing)
- fillRect
- drawFrame
- debug export (Unicode-safe)

PRIMARY API (Unicode-native):
- writeCodePoint(int x, int y, char32_t glyph, const Style&)
- writeText(int x, int y, const std::u32string&, const Style&)
- fillRect(const Rect&, char32_t glyph, const Style&)
- renderToU32String()
- renderToUtf8String()

LEGACY WRAPPERS (forwarding only):
- writeChar(..., char, ...)
- writeString(..., std::string, ...)
→ must decode UTF-8 and forward

IMPORTANT RULES:
- ScreenBuffer operates on display cells, not bytes
- must handle:
  - width 0 (combining)
  - width 1
  - width 2 (CJK/emoji)
- must manage trailing cells for wide glyphs
- must clean up overwritten continuation cells

NOT RESPONSIBLE FOR:
- UTF-8 decoding logic
- grapheme segmentation
- font capability decisions

2.4 Unicode utilities (NEW - REQUIRED)
Utilities/Unicode/

- UnicodeConversion
  - utf8ToU32
  - u32ToUtf8
  - u32ToUtf16
  - codePointToUtf16

- UnicodeWidth
  - measureCodePointWidth(char32_t)

- UnicodeGrapheme (Phase 1 basic / Phase 5 expanded)
  - segmentGraphemeClusters(...)


2.5 Surface
Responsibilities:
- wrap a ScreenBuffer
- serve as an off-screen composition target
- later support layering/window composition

2.6 IRenderer
Responsibilities:
- initialize
- shutdown
- present(ScreenBuffer)
- resize
- textCapabilities()

2.7 ConsoleRenderer
Responsibilities:
- all Win32 console-specific work
- UTF-16 output (not UTF-8 logic internally)
- live console size detection
- diff-based presentation
- resize polling

STRICT RULES:
- must NOT:
  - compute character width
  - perform grapheme segmentation
  - interpret text meaning
- must:
  - render already-laid-out cells
  - skip continuation cells
  - map Style → console attributes
  - use UnicodeConversion utilities

2.8 FrameDiff
Responsibilities:
- compare previous vs current frame
- dirty row/span detection
- support efficient redraw

Must compare:
- glyph
- style
- cell metadata (CellKind)

2.9 Application loop
Responsibilities:
- initialize renderer
- create main surface
- per-frame update
- per-frame render
- live resize polling

2.10 Resize handling
- startup size detection
- live resize detection
- resize logical surface and previous frame on size changes

OUTPUT OF PHASE 1
A Unicode-correct logical rendering engine with:
- proper cell semantics
- correct wide/combining behavior
- backend-independent text model
- efficient redraw

FILES / FOLDERS NEEDED

App/
  Application.h
  Application.cpp

Core/
  Point.h
  Size.h
  Rect.h
  Enums.h

Rendering/
  ScreenCell.h
  ScreenBuffer.h
  ScreenBuffer.cpp
  Surface.h
  Surface.cpp
  FrameDiff.h
  FrameDiff.cpp
  IRenderer.h
  ConsoleRenderer.h
  ConsoleRenderer.cpp

  Utilities/
  Unicode/
    UnicodeConversion.h
    UnicodeConversion.cpp
    UnicodeWidth.h
    UnicodeWidth.cpp
    UnicodeGrapheme.h
    UnicodeGrapheme.cpp

main.cpp



============================================================
3. PHASE 2 — STYLE SYSTEM (ANSI-LIKE AUTHORING MODEL)
============================================================

PURPOSE
Create a real style system that preserves the authoring feel of ansi graphics with Themes.h, without tying the engine to raw ANSI transport strings.

WHY THIS PHASE COMES EARLY
Page composition depends on style semantics. The user should be able to think in “console styling” terms while the backend stays abstract. This directly preserves the semantic theme-building pattern from Themes and the composable styling vocabulary of ansi graphics.

IMPLEMENT IN THIS PHASE

UPDATE NOTES:
- No Unicode conflicts
- Must remain backend-agnostic
- Style must not assume ANSI transport
- Capability detection must inform rendering decisions without changing authored style data
- Diagnostic reporting must explain backend adaptation without coupling page code to backend internals

ADDITION:
- Style system must survive round-trip across:
  - ScreenBuffer
  - Renderer
  - alternate backends
- Output capability detection must determine what the active backend can actually render
- Capability limits must not destroy or downgrade authored style intent inside the logical model
- The renderer must be able to record what output adjustments were made so authors can understand display differences

3.1 Color abstraction
Support:
- standard colors
- bright colors
- 256 color representation
- future-capable RGB 24-bit true color representation

Requirements:
- color model must be backend-independent
- color model must distinguish logical color intent from backend realization
- authored colors must remain representable even if active backend cannot fully display them

3.2 Style struct
Support:
- fg color
- bg color
- bold
- dim
- underline
- slow blink
- fast blink
- reverse
- invisible
- strike

Requirements:
- Style is a logical description of appearance
- Style must not encode transport strings
- Style must be storable in ScreenCell / ScreenBuffer without loss of intent
- unsupported fields must still remain preserved in the logical model

3.3 Policy and Emulation
Define backend behavior for unsupported style features.

Support:
- direct support when backend can render a style field
- graceful downgrade when backend cannot render a style field
- optional emulation policy where appropriate
- explicit separation between:
  - logical style intent
  - backend capability
  - output mapping decision

Examples:
- true color may downgrade to nearest 256 or 16-color approximation
- underline may be dropped if backend does not support it
- blink may be ignored, mapped according to backend policy, or emulated by the renderer.
- unsupported effects must not corrupt other style fields

3.4 Style composition
Support authoring like:
- style::Bold + style::Fg(Color::Red)
- style::Bold + style::Fg(Color::Red) + style::Bg(Color::Green)

Requirements:
- composition must remain clean and author-friendly
- composition must operate on logical Style objects
- composed styles must remain independent of backend capability limits

3.5 Style merge behavior
Support:
- replace style
- preserve destination style
- merge style fields

Requirements:
- merge semantics must be explicit and reusable
- merge behavior must preserve authored style intent
- preserve-style behavior must remain available even when backend output is downgraded

3.6 Semantic theme namespaces
Examples:
- AppThemes
- UIThemes
- BannerThemes
- GameThemes later
- etc

Requirements:
- themes produce logical styles, not backend escape sequences
- themes must not depend on ConsoleRenderer internals
- themes should remain portable across future render backends

3.7 Backend mapping
ConsoleRenderer maps Style to backend capabilities.

Requirements:
- initial backend may support only a subset of Style richness
- model must still preserve future richness
- backend mapping must use capability information rather than hardcoded assumptions
- style-to-output conversion must be isolated from style authoring APIs

3.8 Preserve-style semantics
Needed because earlier docs explicitly relied on preserving destination formatting when no style override was supplied.

Requirements:
- preserve-style behavior must work at the logical buffer level
- renderer downgrade must not alter stored logical styles
- absence of a new style override must keep the existing stored style intact

3.9 Console capability model
Add a capability model that records what the active output backend can actually support.

Purpose:
- detect available console output features at startup
- store backend capability information in a reusable structured form
- allow renderer mapping code to choose direct output, downgrade, or omission
- keep authored style richer than currently available output when necessary

Support:
- VT / ANSI processing available or not
- standard 16-color support
- bright color support
- 256-color support
- RGB / true color support
- bold support
- dim support
- underline support
- reverse support
- invisible support
- strike support
- blink support
- preserve-style-safe fallback behavior
- optional future flags for alternate backends

Requirements:
- capability model must describe backend output ability, not authoring limits
- capability detection must be separate from Style and Color definitions
- capability information must be queryable by renderer mapping code
- capability information must be stable enough to cache after initialization
- capability detection must fail safely when a feature cannot be confirmed
- unknown support should prefer conservative fallback behavior

3.10 Console capability detection
Provide detection logic for the active console backend.

Responsibilities:
- inspect console mode / backend state during renderer initialization
- determine whether VT processing is enabled or can be enabled
- determine broad color/output tiers supported by the backend
- populate ConsoleCapabilities for later rendering decisions
- expose detection results to renderer code without spreading platform checks throughout the codebase

Requirements:
- detection logic must live in backend-specific code, not in Style or theme code
- detection should identify capability tiers rather than over-promise exact rendering behavior
- unsupported or uncertain features should be marked conservatively
- detection must not require the style authoring layer to know Windows console details
- future backends must be able to supply their own capability implementations

3.11 Renderer downgrade and adaptation behavior
Use Style + ConsoleCapabilities together to determine actual output behavior.

Support:
- render exact style when supported
- approximate color when partial support exists
- omit unsupported effects safely
- preserve logical style in ScreenBuffer regardless of rendered downgrade

Requirements:
- adaptation must occur during backend mapping/output
- downgrade rules must be centralized and predictable
- output adaptation must not mutate authored theme/style definitions
- output adaptation must not mutate stored ScreenCell styles unless explicitly requested by a separate transformation step
- an output file must be created that users can inspect to see what was set 

3.12 Capability diagnostics model
Add a diagnostics/reporting model that records capability state and renderer adaptation decisions.

Purpose:
- explain why authored styles may not appear exactly as intended
- expose backend limitations to page authors and developers
- record what the renderer did when exact output was not possible
- provide a stable debugging aid without requiring source-level backend knowledge

Support:
- record detected backend capabilities
- record renderer output policy decisions
- record style fields that were:
  - rendered directly
  - downgraded
  - approximated
  - omitted
  - emulated
  - preserved logically but not rendered physically
- record summary statistics and representative examples where helpful

Requirements:
- diagnostics must report backend realization decisions, not alter them
- diagnostics must remain optional and non-invasive
- diagnostics must not become required for normal rendering
- diagnostics data should be structured enough for future tooling
- diagnostics output should remain understandable to page authors

3.13 Renderer diagnostics logging
Provide a file-based diagnostics output that authors can inspect after startup or render activity.

Purpose:
- save capability and adaptation information to a human-readable file
- help page authors understand why display output differs from authored intent
- make backend limitations visible during development and testing

Possible contents:
- renderer/backend identity
- console mode / VT status
- detected capability flags
- supported color tier
- supported decoration features
- downgrade rules currently in effect
- emulation rules currently in effect
- examples of unsupported style requests encountered during rendering
- summary of omitted or approximated style effects

Examples of reported events:
- RGB foreground requested, downgraded to nearest 16-color value
- underline requested, omitted because backend does not support underline
- blink requested, not emitted because blink is disabled or unsupported
- reverse requested, emitted directly
- logical style preserved in ScreenBuffer, physical output adapted in renderer

Requirements:
- diagnostics logging must be separate from normal render output
- diagnostics file generation must not clutter console output
- diagnostics logging should be optional or configurable
- diagnostics logging must not require page authors to change how they author styles
- diagnostics file format should be simple, readable, and stable
- renderer should be able to append events or write a fresh report per run according to configuration

3.14 Author-facing rendering hints
Use diagnostics output to help page authors improve pages without exposing backend internals everywhere.

Purpose:
- help authors choose styles that are more portable
- help identify when a theme depends on features unavailable in the current console
- support future validation and tooling

Support:
- capability summary section
- warnings about unsupported theme/style usage
- suggestions for portable alternatives where reasonable
- distinction between:
  - authored logical style
  - backend-supported style
  - actual rendered result

Requirements:
- hints must be advisory only
- hints must not force authors to target one backend
- hints must not weaken the backend-agnostic style model

OUTPUT OF PHASE 2
A backend-agnostic style system with console-native authoring feel, plus a capability-aware backend mapping layer that can detect available console output features, preserve logical style richness, safely adapt rendered output to the active backend, and emit diagnostics explaining why authored styles may be displayed differently.

FILES / FOLDERS NEEDED

Rendering/Styles/
  Color.h
  Style.h
  Style.cpp
  StyleBuilder.h
  StyleBuilder.cpp
  StyleMerge.h
  StyleMerge.cpp
  BannerThemes.h
  UIThemes.h

Rendering/Capabilities/
  ConsoleCapabilities.h
  ConsoleCapabilities.cpp
  CapabilityReport.h
  CapabilityReport.cpp

Rendering/Diagnostics/
  RenderDiagnostics.h
  RenderDiagnostics.cpp
  RenderDiagnosticsWriter.h
  RenderDiagnosticsWriter.cpp

Rendering/Backends/
  ConsoleCapabilityDetector.h
  ConsoleCapabilityDetector.cpp

Optional if kept outside Rendering:
UI/Styles/
  FrameStyle.h
  UIStyle.h

DEFINITION OF DONE

- Color abstraction supports logical standard, bright, 256, and RGB color representations
- Style represents logical appearance without transport coupling
- Style composition API supports ansi-like authoring syntax
- Style merge behavior supports replace, preserve, and merge semantics
- Semantic themes produce reusable backend-agnostic logical styles
- ConsoleCapabilities records active backend output capabilities in structured form
- Console capability detection runs during renderer initialization
- ConsoleRenderer maps Style through capability-aware backend conversion
- Unsupported output features are downgraded, omitted, approximated, or emulated safely
- Diagnostics model records backend capability state and renderer adaptation behavior
- Diagnostics writer can save a readable report file for authors and developers
- ScreenBuffer and ScreenCell preserve logical style intent even when active output is limited
- Architecture remains portable to alternate render backends



============================================================
4. PHASE 3 — ASCII OBJECT / ASSET MODEL
============================================================

PURPOSE
Create reusable visual objects for composition. 

WHY THIS PHASE COMES EARLY
Page composition depends on style semantics. The user should be able to think in “console styling” terms while the backend stays abstract. This directly preserves the semantic theme-building pattern and the composable styling vocabulary from ansi.h. 

The page API must have stable primitives to place. The user must be able to rely on authored screens that depend on reusable text assets, text blocks, boxes, ANSI art, and generated banner content.

This phase must define a single reusable text-asset object model, while plain text, ANSI art, binary text art, and banner/font formats are handled by dedicated loaders or generators that normalize content into that shared object.

UPDATE NOTES:
- No Unicode conflicts
- Must remain backend-agnostic
- Style must not assume ANSI transport
- All text loaded from files must be decoded at the boundary and normalized immediately
- UTF-8 text sources must be converted to std::u32string immediately
- TextObject is the unified internal object model for reusable placed text assets
- Do NOT split the object model into:
  - AsciiObject
  - AnsiObject
  - UnicodeObject
- Those distinctions belong to loaders/parsers, not to the core composition type

ADDITION:
- Style system must survive round-trip across:
  - ScreenBuffer
  - Renderer
  - alternate backends

ARCHITECTURE RULE

Use this pipeline:

File or generator format
    ↓
Loader / parser
    ↓
Unified internal object model
    ↓
ScreenBuffer

Examples:
- .txt / .asc / .nfo / .diz → PlainTextLoader → TextObject
- .ans / .bin / .xbin / .adf → AnsiLoader / BinaryArtLoader → TextObject
- FIGlet / TOIlet → AsciiBanner → TextObject

WHY THIS IS THE CORRECT MODEL

ASCII is mostly an input/content constraint
ANSI is a formatting/control-sequence format
Unicode is the engine’s internal text representation model

If the project creates separate core object types like:
- AsciiObject
- AnsiObject
- UnicodeObject

then it mixes different concerns into the type system and creates:
- duplicated width/height/storage logic
- duplicated draw logic
- awkward conversions
- confusion about what PageComposer should accept
- difficulty supporting mixed-format content later

Instead:
- one object type
- many loaders/parsers
- one rendering target

That is the cleanest and most extensible design.

TEXT OBJECT STORAGE MODEL

TextObject should internally store normalized display content suitable for placement into ScreenBuffer.

Minimum responsibilities:
- own normalized internal content
- width
- height
- loaded state
- optional source metadata
- optional per-cell style information
- optional future metadata for compositing / layering

RECOMMENDED STORAGE SHAPE

Because this roadmap now includes:
- .ans
- .bin
- .xbin
- .adf
- styled loaded assets

TextObject should prefer a cell-grid-backed internal representation rather than a simple vector of plain strings.

That means TextObject should be able to represent:
- glyph
- style
- empty cells
- alignment-preserving spaces
- optional metadata needed for imported art

This keeps imported terminal-art assets first-class and avoids losing style/layout fidelity.

IMPLEMENT IN THIS PHASE
4.1 TextObject core
Support:
- FromFile
- FromTextBlock
- width / height
- loaded state
- draw into ScreenBuffer
- normalized internal cell/text storage
- style-aware imported content support

IMPORTANT:
- TextObject is a reusable visual object
- TextObject is NOT a renderer
- TextObject should contain no backend-specific output logic

4.2 Factory helpers
Support:
- Rectangle
- Bordered box
- optional line helpers
- optional fill/block helpers
- optional FIGlet text helper if it fits cleanly

Rules:
- factory helpers return TextObject
- factory helpers remain renderer-independent
- generated objects must be valid Unicode-aware object content

4.3 Plain text loader family
Support loading:
- .asc
- .txt
- .nfo
- .diz

Responsibilities:
- decode file text at the boundary
- support UTF-8 text input
- support legacy text-art encodings when explicitly needed
- normalize content into TextObject
- preserve intended line breaks and spacing

Notes:
- .nfo and .diz may require optional legacy code page support for classic art compatibility
- code-page handling must remain in loader/conversion utilities, never in ScreenBuffer or PageComposer

4.4 Text asset saving support
Support saving where it makes architectural sense:
- save TextObject to:
  - .txt
  - .asc
  - .diz
- optional save to .nfo when encoding policy is defined
- export plain text representations from TextObject

Rules:
- plain-text save paths must be explicit about encoding
- UTF-8 should be the default outbound text format unless a legacy format explicitly requires another encoding
- save/export logic must not bypass the unified object model

4.5 ANSI / terminal art loader family
Support loading:
- .ans
- .bin
- .xbin
- .adf

Responsibilities:
- parse source format into:
  - glyphs
  - styles
  - positioning/layout semantics as supported
- normalize imported content into TextObject
- never replay raw ANSI directly to the backend as the primary model

Rules:
- ANSI/control-sequence semantics must be translated into internal Style semantics
- imported content must become engine-owned structured data
- unknown or unsupported commands must fail gracefully without crashing

4.6 ANSI / terminal art saving support
Support saving where it makes architectural sense:
- save/export TextObject to:
  - .ans
  - .bin
- optional later:
  - .xbin
  - .adf

Rules:
- saving to terminal-art formats is an exporter responsibility
- exporter behavior must be explicit about format limitations
- if a TextObject contains styling/features the target format cannot express, exporter must:
  - clamp
  - approximate
  - or fail clearly
- never hide lossy conversion

4.7 Asset loading / caching layer
Goal:
- load text assets from Assets directory
- centralize lookup
- avoid repeated disk work
- centralize format detection and dispatch

Responsibilities:
- asset path resolution
- extension/type dispatch
- cache previously loaded normalized TextObject content
- support future asset catalogs

4.8 AsciiBanner integration
Support:
- FIGlet loading
- TOIlet loading
- renderLines
- create TextObject output
- draw into ScreenBuffer through TextObject flow
- keep this as an object/text-generation helper, not a renderer

Rules:
- banner generation returns reusable object content
- banner generation must not directly write to the backend
- banner output should normalize into TextObject

4.9 Font loader support for custom bitmap/pseudo-graphical fonts
Support loading:
- .fon / custom bitmap font mapping formats

Purpose:
- custom UI font themes
- pseudo-graphical rendering in console
- character-to-glyph mapping support for generated text objects

Rules:
- this is a font/asset loading concern, not a renderer concern
- font loaders may feed:
  - AsciiBanner-like generators
  - future glyph/theme systems
- do not turn ScreenBuffer into a font engine

OUTPUT OF PHASE 3
Reusable visual assets that the page layer can place and compose, built around one unified TextObject model with format-specific loaders/parsers and optional exporters.

By the end of this phase the engine should support:
- one primary reusable text object type
- plain text asset loading
- ANSI / terminal art loading
- banner/font-generated text objects
- centralized asset lookup and caching
- saving/export where format fidelity makes sense

FILES / FOLDERS NEEDED

Rendering/Objects/
  TextObject.h
  TextObject.cpp
  AsciiBanner.h
  AsciiBanner.cpp
  ObjectFactory.h
  ObjectFactory.cpp
  PlainTextLoader.h
  PlainTextLoader.cpp
  AnsiLoader.h
  AnsiLoader.cpp
  BinaryArtLoader.h
  BinaryArtLoader.cpp
  TextObjectExporter.h
  TextObjectExporter.cpp
  FontLoader.h
  FontLoader.cpp

Assets/
  ASCII/
  ANSI/
  Fonts/
    FIGlet/
    TOIlet/
    Bitmap/

Utilities/
  AssetPaths.h
  AssetPaths.cpp

Optional later:
Data/Assets/
  AssetLibrary.h
  AssetLibrary.cpp



============================================================
5. PHASE 4 — COMPOSITING / WRITE POLICY SYSTEM
============================================================

PURPOSE
Replace the old confusing write mode names with a clear and scalable compositing model.

WHY THIS PHASE IS FOUNDATIONAL
The page API should not be built on ambiguous concepts like “transparent” and “fully transparent.” The writing/compositing model must be correct first so page-authoring functions stay intuitive. This directly addresses the weakness discovered in the old API and preserves the old functionality without the naming confusion. The old docs clearly show that multiple write modes were essential, but the names were not ideal. 

UPDATE NOTES:

- All compositing operates on fully-resolved ScreenCell objects
- Must respect:
  - continuation cells
  - wide glyph boundaries
- Must NOT split wide glyphs

NEW RULE:
- compositing must treat a wide glyph as an atomic visual unit

DEFINE IN THIS PHASE

5.1 Source cell parts
Each source cell can contribute:
- Glyph
- Space
- Style

5.2 Source mask
- AllCells
- GlyphCellsOnly
- SpaceCellsOnly

5.3 Overwrite rules
For glyph:
- Always
- IfDestinationEmpty
- Never

For style:
- Always
- IfDestinationUnstyled
- Never

5.4 WritePolicy struct
Should support:
- writeGlyph
- writeStyle
- writeSpaces
- sourceMask
- glyphOverwrite
- styleOverwrite

5.5 Core compositing function
General form:
- writeObject(..., WritePolicy)

5.6 Named convenience presets
These are the API names authors should actually use most often:
- writeSolidObject
- writeVisibleObject
- writeGlyphsOnly
- writeStyleMask
- writeStyleBlock

5.7 Optional depth-aware convenience helpers
- writeVisibleObjectBehind
- writeGlyphsOnlyBehind
- writeStyleMaskBehind

5.8 Placement variants
All preset functions should eventually support:
- exact coordinates
- region + alignment
- full-screen alignment

OUTPUT OF PHASE 4
A complete and understandable object writing/compositing vocabulary.

FILES / FOLDERS NEEDED

Rendering/Composition/
  WritePolicy.h
  WritePolicy.cpp
  SourceMask.h
  OverwriteRule.h
  ObjectWriter.h
  ObjectWriter.cpp

Optional:
Rendering/Composition/
  WritePresets.h
  WritePresets.cpp



============================================================
6. PHASE 5 — PAGE COMPOSER API 1.0
============================================================

PURPOSE
Build the real authoring API for creating TUI pages in C++.

WHY THIS PHASE COMES AFTER COMPOSITING
The page API should expose good author-facing names and placement features, but it should sit on top of stable lower-level rendering, style, object, and compositing primitives.

IMPORTANT DESIGN DECISION
Do not put all authoring behavior directly onto low-level ScreenBuffer.
Build a higher-level PageComposer or PageBuffer layer that writes into ScreenBuffer.

UPDATE NOTES:

TEXT API MUST CHANGE TO:

- writeText(x, y, std::u32string, style)
- writeTextUtf8(...) → wrapper only

Text layout responsibilities:
- must use UnicodeWidth + grapheme segmentation
- must NOT assume 1 char = 1 column

render() MUST BECOME:
- renderToUtf8String()

API 1.0 SHOULD INCLUDE

6.1 Page clearing
- clear(style)

6.2 Screen template loading
- createScreen(filename)
Behavior:
- load at (0,0)
- do not auto-clear first
This preserves the old layering workflow described in the docs. 

6.3 Named regions
- createRegion(x, y, w, h, name)
- hasRegion(name)
- getRegion(name)
- clearRegions()

6.4 Alignment vocabulary
- HorizontalAlign: Left, Center, Right
- VerticalAlign: Top, Center, Bottom

6.5 Text writing
- writeText(x, y, text, style)
- writeTextBlock(x, y, block, style)

6.6 Object writing by coordinate
- writeSolidObject(...)
- writeVisibleObject(...)
- writeGlyphsOnly(...)
- writeStyleMask(...)
- writeStyleBlock(...)
- plus generic writeObject(..., WritePolicy)

6.7 Object writing by region + alignment
- same operations using region name and alignments

6.8 Full-screen aligned placement
- same operations using implicit full screen as region

6.9 Render/debug export
- render() -> std::string

6.10 Optional future-friendly API helpers
- centerX()
- centerInRegion()
- anchor helpers
- named placement helpers

6.11 Structured data-driven object creation hooks
Prepare PageComposer to support future structured formats without coupling this phase to those parsers.

Future object/layout formats should be able to resolve into:
- TextObject
- PageComposer placement calls
- region/alignment-based placement

This keeps later declarative formats aligned with the same composition model rather than creating a second UI system.

OUTPUT OF PHASE 5
The first complete authoring API for building pages cleanly in C++.

FILES / FOLDERS NEEDED

Composition/
  PageComposer.h
  PageComposer.cpp
  RegionRegistry.h
  RegionRegistry.cpp
  Alignment.h
  Placement.h
  Placement.cpp

Optional alias layer if you prefer:
Page/
  PageBuffer.h
  PageBuffer.cpp



============================================================
7. PHASE 6 — PAGE FILE FORMAT AND INTERPRETER
============================================================

PURPOSE
Allow people to build pages without Visual Studio.

WHY THIS PHASE COMES HERE
The interpreter should map onto an already-stable PageComposer API. It should not define the engine primitives; it should consume them.

UPDATE NOTES:

- ALL file input is UTF-8
- Interpreter must:
  - decode → U32 immediately
  - never operate on std::string internally

IMPLEMENT IN THIS PHASE

7.1 Define page language syntax
Commands should cover:
- CLEAR
- SCREEN
- REGION
- TEXT
- TEXTBLOCK
- OBJECT
- STYLE
- MODE
- ALIGN
- AT
- optional comments

7.2 Page parser
- tokenize commands
- parse arguments
- report line/file errors

7.3 Page interpreter
- resolve assets
- resolve themes/styles
- call PageComposer methods

7.4 Page asset references
- resolve ASCII assets
- resolve screen templates
- resolve fonts if needed later

7.5 Style/theme name lookup
- e.g. "Field", "Helmets", "Title"

7.6 Page execution result
- build final page into ScreenBuffer / Surface

7.7 Testing/examples
- create sample page files
- verify parity with equivalent C++ page definitions

7.8 Structured page/config input formats (initial declarative layer)
Support later parsing/import for:
- .json
- .yaml
- .ini

Purpose:
- declarative page descriptions
- configuration-driven authored screens
- future style/theme/layout definitions

Rules:
- these formats must compile down to the same PageComposer / TextObject / style APIs
- they must not introduce a second rendering or composition model
- use them for page/layout description, not as a replacement for ScreenBuffer or TextObject

7.9 Structured asset references
Structured page files may reference:
- TextObject assets
- fonts
- themes
- regions
- screen templates

All references must resolve through centralized asset/path systems.

OUTPUT OF PHASE 6
A first interpreted page system that uses the same engine as the C++ API.

FILES / FOLDERS NEEDED

Interpreter/
  Token.h
  Lexer.h
  Lexer.cpp
  Parser.h
  Parser.cpp
  PageScriptAst.h
  PageScriptInterpreter.h
  PageScriptInterpreter.cpp
  PageCommand.h
  PageCommand.cpp
  ThemeResolver.h
  ThemeResolver.cpp

Assets/
  Pages/



============================================================
8. PHASE 7 — APPLICATION INPUT / EVENTS / COMMANDS
============================================================

PURPOSE
Move from passive page rendering to interactive TUI behavior.

WHY THIS PHASE COMES AFTER PAGE AUTHORING
You wanted page composition to feel simple first. Input and interactivity should be layered on once the display model is stable.

IMPLEMENT IN THIS PHASE

8.1 InputManager
- raw key polling
- normalization

8.2 KeyCode abstraction
- platform-independent keys

8.3 Command abstraction
- MoveUp
- MoveDown
- Confirm
- Cancel
- NextFocus
- PreviousFocus
- OpenHelp
etc.

8.4 Event model
- KeyEvent
- CommandEvent
- ResizeEvent
- TickEvent
- later MouseEvent

8.5 CommandMap
- key to command mapping

8.6 Event dispatch path
- application to active page/screen
- later to focused widget/window

OUTPUT OF PHASE 7
A clean event/input layer ready for menus and widgets.

FILES / FOLDERS NEEDED

Input/
  KeyCodes.h
  Command.h
  Event.h
  InputManager.h
  InputManager.cpp
  CommandMap.h
  CommandMap.cpp

============================================================
8–15 PHASES Rules
============================================================

GLOBAL UPDATE (applies to ALL REMAINING PHASES):

1. All text input:
   - UTF-8 at boundaries only
   - internal = std::u32string

2. All layout:
   - must use UnicodeWidth
   - must support width 0 / 1 / 2

3. No system may:
   - assume fixed-width characters
   - assume ASCII

4. Rendering:
   - always operates on ScreenCell grid

5. Widgets / UI:
   - must use grapheme-aware layout
   - must not break wide glyphs across boundaries

6. Debug / Testing:
   - must use UTF-8 snapshot output
   - must not truncate to char


============================================================
9. PHASE 8 — LAYERING / COMPOSITOR / PANELS / WINDOWS
============================================================

PURPOSE
Support layered UI, panels, popups, and future overlapping windows.

WHY THIS PHASE COMES BEFORE WIDGETS
Widgets and dialogs need a layered composition environment. This follows the original architecture guidance around surfaces/layers, panels/windows, and reusable popup structure. 

IMPLEMENT IN THIS PHASE

9.1 Layer/Surface instances
- position
- visibility
- z-order

9.2 Compositor
- compose multiple surfaces into final frame
- clipping
- ordering

9.3 UIElement base
- bounds
- visible
- enabled
- draw/update hooks

9.4 Panel
- titled rectangular region
- background/frame rendering

9.5 Window
- top-level panel semantics
- modal flag

9.6 PopupWindow
- open/close behavior
- modal display

9.7 WindowManager
- window list
- z-order
- show/hide
- modal routing

OUTPUT OF PHASE 8
Reusable layered UI foundation for windows and overlays.

FILES / FOLDERS NEEDED

Rendering/
  Compositor.h
  Compositor.cpp
  LayerInstance.h
  LayerInstance.cpp

UI/Base/
  UIElement.h
  UIElement.cpp
  WindowManager.h
  WindowManager.cpp

UI/Panels/
  Panel.h
  Panel.cpp
  Window.h
  Window.cpp
  PopupWindow.h
  PopupWindow.cpp



============================================================
10. PHASE 9 — FOCUS, MENU SYSTEM, STATUS BARS
============================================================

PURPOSE
Make interactive authored pages practical and consistent.

IMPLEMENT IN THIS PHASE

10.1 FocusManager
- current focused element
- set/next/previous focus
- modal awareness later

10.2 StatusBar
- command hints
- page-level instructions

10.3 Menu primitives
- MenuItem
- MenuModel
- MenuView
- MenuController

10.4 Navigation patterns
- vertical navigation
- horizontal navigation
- confirm/cancel
- wrap/clamp
- disabled item skipping

10.5 First interactive authored page
- title menu
- settings page
or
- help/about popup

10.6 Structured configuration support for menus and status content
Optional use of:
- .json
- .yaml
- .ini

for menu definitions, command hints, and authored interactive metadata, provided they resolve into the same runtime UI model.

This should remain data-driven configuration on top of the existing engine, not a competing architecture.

OUTPUT OF PHASE 9
The first genuinely usable interactive TUI pages.

FILES / FOLDERS NEEDED

UI/Base/
  FocusManager.h
  FocusManager.cpp

UI/Menus/
  MenuItem.h
  MenuItem.cpp
  MenuModel.h
  MenuModel.cpp
  MenuView.h
  MenuView.cpp
  MenuController.h
  MenuController.cpp

UI/Panels/
  StatusBar.h
  StatusBar.cpp



============================================================
11. PHASE 10 — VIEWPORTS / SCROLLING / HELP PANELS
============================================================

PURPOSE
Support content larger than the visible area.

IMPLEMENT IN THIS PHASE

11.1 Viewport
- content size
- view size
- scroll offsets

11.2 ScrollablePanel
- clipping-aware rendering
- keyboard scrolling

11.3 TextView / read-only panels
- manuals
- reports
- help text
- logs

11.4 Tile / grid-based content formats
Support grid-oriented asset/layout loading for larger structured content areas.

Candidate formats:
- .csv grid layouts
- custom tile/grid definitions
- Cogmind-style tileset-inspired mapping formats later if needed

Purpose:
- maps
- dashboards
- structured panels
- tile-oriented authored layouts

Rules:
- tile/grid data must normalize into engine-owned structures
- visible rendering must still end in ScreenCell / ScreenBuffer composition
- no special bypass around the rendering core

OUTPUT OF PHASE 10
A reusable scrolling system for complex text-heavy screens.

FILES / FOLDERS NEEDED

UI/Base/
  Viewport.h
  Viewport.cpp

UI/Panels/
  ScrollablePanel.h
  ScrollablePanel.cpp

UI/Widgets/
  TextView.h
  TextView.cpp



============================================================
12. PHASE 11 — WIDGET SYSTEM 1.0
============================================================

PURPOSE
Move from page composition + menus to reusable full UI controls.

IMPLEMENT IN THIS PHASE

12.1 Widget base
- bounds
- focusable
- draw
- update
- handleEvent

12.2 ContainerWidget
- owns child widgets
- child drawing
- child dispatch
- child focus traversal

12.3 Initial widgets
- Label
- Button
- ListBox
- TextView if not already placed earlier

12.4 Widget style integration
- use Style / UITheme roles
- support focused/disabled/selected states

OUTPUT OF PHASE 11
The first real reusable widget framework.

FILES / FOLDERS NEEDED

UI/Widgets/
  Widget.h
  Widget.cpp
  ContainerWidget.h
  ContainerWidget.cpp
  Label.h
  Label.cpp
  Button.h
  Button.cpp
  ListBox.h
  ListBox.cpp
  TextView.h
  TextView.cpp



============================================================
13. PHASE 12 — PAGE LANGUAGE EXTENSIONS FOR INTERACTIVE LAYOUTS
============================================================

PURPOSE
Extend interpreted pages beyond static composition.

IMPLEMENT IN THIS PHASE

13.1 Widget declarations in page files
- LABEL
- BUTTON
- MENU
- LIST
- PANEL

13.2 Interactive page binding
- commands
- actions
- selection IDs
- focus order

13.3 Interpreter support for widget trees
- parse widgets
- instantiate widgets
- attach to page/screen

13.4 Structured widget/layout import
Allow future import of widget/layout definitions from:
- .json
- .yaml
- .ini

These must instantiate the same widget trees and PageComposer placement semantics used by native code and interpreted page scripts.

OUTPUT OF PHASE 12
Interpreted interactive page layouts, not just static screens.

FILES / FOLDERS NEEDED

Interpreter/
  WidgetScriptAst.h
  WidgetFactory.h
  WidgetFactory.cpp
  ActionResolver.h
  ActionResolver.cpp



============================================================
14. PHASE 13 — ANIMATION / TIMERS / INVALIDATION
============================================================

PURPOSE
Add polish without breaking the architecture.

IMPLEMENT IN THIS PHASE

14.1 TickClock
- delta time
- timing helpers

14.2 Tick events
- periodic UI updates

14.3 Animation model
- duration
- elapsed
- progress
- target properties

14.4 Invalidation system
- invalidate rect
- invalidate whole page
- invalidate element/window
This was part of the earlier architecture guidance and belongs here once richer UI is present. 

14.5 Animated / multi-frame text asset formats
Support later loading/playback of time-based text assets such as:
- multi-frame .ans animation
- .txt frame sequences
- custom .anim format

Purpose:
- animated banners
- splash/title effects
- lightweight TUI motion systems

Rules:
- animations must be modeled as frame sequences or time-based asset sets
- playback must compose through existing surfaces/buffers
- animation formats must not bypass compositing or PageComposer placement semantics

OUTPUT OF PHASE 13
Stable time-based polish features layered on top of a finished UI model.

FILES / FOLDERS NEEDED

Animation/
  TickClock.h
  TickClock.cpp
  Animation.h
  Animation.cpp
  Animator.h
  Animator.cpp

Rendering/
  InvalidationRegion.h
  InvalidationRegion.cpp



============================================================
15. PHASE 14 — ALTERNATE BACKENDS / BACKEND-NEUTRAL RENDERING
============================================================

PURPOSE
Future-proof the engine for non-console output.

IMPLEMENT IN THIS PHASE

15.1 Strengthen IRenderer seam
- backend-neutral contract
- style mapping abstraction if needed

15.2 Optional ANSI terminal backend

15.3 Optional SDL/SFML fake-terminal backend

15.4 Optional image/debug export backend

OUTPUT OF PHASE 14
Renderer flexibility without disturbing page API or interpreter.

FILES / FOLDERS NEEDED

Rendering/Backends/
  AnsiRenderer.h
  AnsiRenderer.cpp
  SDLRenderer.h
  SDLRenderer.cpp
  DebugImageRenderer.h
  DebugImageRenderer.cpp



============================================================
16. PHASE 15 — TOOLING / TESTING / PAGE AUTHORING QUALITY-OF-LIFE
============================================================

PURPOSE
Make the engine practical to maintain and use.

IMPLEMENT IN THIS PHASE

16.1 Render snapshot tests
- compare renderToString output
- verify layout regressions

16.2 Theme/style preview tools

16.3 Asset preview tools

16.4 Page linting / script validation

16.5 Optional page editor later

OUTPUT OF PHASE 15
A maintainable and author-friendly ecosystem around the engine.

FILES / FOLDERS NEEDED

Tools/
  ThemePreviewTool/
  PageLintTool/
  AssetPreviewTool/

Tests/
  RenderingTests/
  PageComposerTests/
  InterpreterTests/
  WidgetTests/



============================================================
17. UPDATED VISUAL STUDIO FOLDER / FILE LAYOUT
============================================================

This updates the earlier architecture/file roadmap to support API 1.0 and interpreter work.

TUI/
│
├── App/
│   ├── Application.h
│   └── Application.cpp
│
├── Core/
│   ├── Point.h
│   ├── Size.h
│   ├── Rect.h
│   └── Enums.h
│
├── Rendering/
│   ├── ScreenCell.h
│   ├── ScreenBuffer.h
│   ├── ScreenBuffer.cpp
│   ├── Surface.h
│   ├── Surface.cpp
│   ├── FrameDiff.h
│   ├── FrameDiff.cpp
│   ├── InvalidationRegion.h
│   ├── InvalidationRegion.cpp
│   ├── IRenderer.h
│   ├── ConsoleRenderer.h
│   ├── ConsoleRenderer.cpp
│   ├── Compositor.h
│   ├── Compositor.cpp
│   ├── LayerInstance.h
│   ├── LayerInstance.cpp
│   │
│   ├── Styles/
│   │   ├── Color.h
│   │   ├── Style.h
│   │   ├── Style.cpp
│   │   ├── StyleBuilder.h
│   │   ├── StyleBuilder.cpp
│   │   ├── StyleMerge.h
│   │   ├── StyleMerge.cpp
│   │   ├── BannerThemes.h
│   │   └── UIThemes.h
│   │
│   ├── Objects/
│   │   ├── TextObject.h
│   │   ├── TextObject.cpp
│   │   ├── AsciiBanner.h
│   │   ├── AsciiBanner.cpp
│   │   ├── ObjectFactory.h
│   │   ├── ObjectFactory.cpp
│   │   ├── PlainTextLoader.h
│   │   ├── PlainTextLoader.cpp
│   │   ├── AnsiLoader.h
│   │   ├── AnsiLoader.cpp
│   │   ├── BinaryArtLoader.h
│   │   ├── BinaryArtLoader.cpp
│   │   ├── TextObjectExporter.h
│   │   ├── TextObjectExporter.cpp
│   │   ├── FontLoader.h
│   │   └── FontLoader.cpp
│   │
│   └── Composition/
│       ├── SourceMask.h
│       ├── OverwriteRule.h
│       ├── WritePolicy.h
│       ├── WritePolicy.cpp
│       ├── ObjectWriter.h
│       ├── ObjectWriter.cpp
│       ├── WritePresets.h
│       └── WritePresets.cpp
│
├── Composition/
│   ├── PageComposer.h
│   ├── PageComposer.cpp
│   ├── RegionRegistry.h
│   ├── RegionRegistry.cpp
│   ├── Alignment.h
│   ├── Placement.h
│   └── Placement.cpp
│
├── Input/
│   ├── KeyCodes.h
│   ├── Command.h
│   ├── Event.h
│   ├── InputManager.h
│   ├── InputManager.cpp
│   ├── CommandMap.h
│   └── CommandMap.cpp
│
├── Interpreter/
│   ├── Token.h
│   ├── Lexer.h
│   ├── Lexer.cpp
│   ├── Parser.h
│   ├── Parser.cpp
│   ├── PageScriptAst.h
│   ├── PageScriptInterpreter.h
│   ├── PageScriptInterpreter.cpp
│   ├── PageCommand.h
│   ├── PageCommand.cpp
│   ├── ThemeResolver.h
│   ├── ThemeResolver.cpp
│   ├── WidgetScriptAst.h
│   ├── WidgetFactory.h
│   ├── WidgetFactory.cpp
│   ├── ActionResolver.h
│   └── ActionResolver.cpp
│
├── UI/
│   ├── Base/
│   │   ├── UIElement.h
│   │   ├── UIElement.cpp
│   │   ├── FocusManager.h
│   │   ├── FocusManager.cpp
│   │   ├── WindowManager.h
│   │   ├── WindowManager.cpp
│   │   ├── Viewport.h
│   │   └── Viewport.cpp
│   │
│   ├── Panels/
│   │   ├── Panel.h
│   │   ├── Panel.cpp
│   │   ├── Window.h
│   │   ├── Window.cpp
│   │   ├── PopupWindow.h
│   │   ├── PopupWindow.cpp
│   │   ├── StatusBar.h
│   │   ├── StatusBar.cpp
│   │   ├── ScrollablePanel.h
│   │   └── ScrollablePanel.cpp
│   │
│   ├── Menus/
│   │   ├── MenuItem.h
│   │   ├── MenuItem.cpp
│   │   ├── MenuModel.h
│   │   ├── MenuModel.cpp
│   │   ├── MenuView.h
│   │   ├── MenuView.cpp
│   │   ├── MenuController.h
│   │   └── MenuController.cpp
│   │
│   └── Widgets/
│       ├── Widget.h
│       ├── Widget.cpp
│       ├── ContainerWidget.h
│       ├── ContainerWidget.cpp
│       ├── Label.h
│       ├── Label.cpp
│       ├── Button.h
│       ├── Button.cpp
│       ├── ListBox.h
│       ├── ListBox.cpp
│       ├── TextView.h
│       └── TextView.cpp
│
├── Screens/
│   ├── Screen.h
│   ├── Screen.cpp
│   ├── TitleScreen.h
│   ├── TitleScreen.cpp
│   ├── MainMenuScreen.h
│   └── MainMenuScreen.cpp
│
├── Animation/
│   ├── TickClock.h
│   ├── TickClock.cpp
│   ├── Animation.h
│   ├── Animation.cpp
│   ├── Animator.h
│   └── Animator.cpp
│
├── Game/
├── Data/
├── Utilities/
│   ├── Unicode/
│   │   ├── UnicodeConversions.h
│   │   ├── UnicodeConversions.cpp
│   │   ├── UnicodeWidth.h
│   │   ├── UnicodeWidth.cpp
│   │   ├── UnicodeGrapheme.h
│   │   ├── UnicodeGrampheme.cpp
│   ├── AssetPaths.h
│   ├── AssetPaths.cpp
│   ├── StringUtils.h
│   ├── StringUtils.cpp
│   ├── Logger.h
│   └── Logger.cpp
│
├── Assets/
│   ├── ASCII/
│   ├── ANSI/
│   ├── Fonts/
│   │   ├── FIGlet/
│   │   ├── TOIlet/
│   │   └── Bitmap/
│   ├── Pages/
│   ├── Screens/
│   └── Themes/
│
├── Tests/
│   ├── RenderingTests/
│   ├── PageComposerTests/
│   ├── InterpreterTests/
│   └── WidgetTests/
│
└── Tools/
    ├── ThemePreviewTool/
    ├── PageLintTool/
    └── AssetPreviewTool/



============================================================
18. WHAT SHOULD BE BUILT NEXT, SPECIFICALLY
============================================================

Do not jump to widgets or old page ports.

Build next in this exact order:

1. Finish Phase 1 cleanly if anything remains unstable
2. Finish Phase 2 style system cleanly
3. Build Phase 3 TextObject / asset system around one unified object model
4. Add plain text loaders first (.txt / .asc / .nfo / .diz)
5. Add ANSI / terminal art loaders next (.ans / .bin first, .xbin / .adf after core import path is stable)
6. Add TextObject export/save support where fidelity is clear
7. Add banner/font loading and generation support
8. Build Phase 4 WritePolicy/compositing system
9. Build Phase 5 PageComposer API 1.0
10. Test authored pages in C++
11. Then build Phase 6 interpreter

That is the correct path to avoid temporary solutions.



============================================================
19. API 1.0 MINIMUM LOCK-IN CHECKLIST
============================================================

Before starting interpreter work, API 1.0 should support all of this:

- clear(style)
- createScreen(filename)
- createRegion(...)
- hasRegion / getRegion / clearRegions
- writeText
- writeTextBlock
- writeSolidObject
- writeVisibleObject
- writeGlyphsOnly
- writeStyleMask
- writeStyleBlock
- writeObject with explicit WritePolicy
- coordinate placement
- region + alignment placement
- render() to string
- semantic named styles like BannerThemes
- style preservation / merge semantics

If any of these are missing, interpreter work should wait.



============================================================
20. FINAL PROJECT DIRECTION
============================================================

The engine is now intentionally aimed at supporting:

1. clean C++ page composition
2. data-driven page composition
3. later widgets and interactive layouts
4. later alternate backends
5. long-term maintainability without large refactors

The key idea is:
build the page-composition vocabulary first,
then let both C++ and interpreted pages use it.

============================================================
OUTPUT OF UPDATED ROADMAP 03/21/26
============================================================

A fully Unicode-correct TUI engine that:

- preserves clean architecture
- avoids future refactors
- supports real-world text (emoji, CJK, combining marks)
- remains backend-neutral
- maintains authoring simplicity