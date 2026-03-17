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

0.1 Keep the renderer core low-level
- ScreenBuffer should remain a logical cell grid
- renderer/presentation should stay separate
- platform-specific code belongs in backend code only

0.2 Build authoring API on top of the rendering core
- do not force page-building behavior directly into the lowest-level cell store
- build a PageComposer/PageBuffer layer on top of ScreenBuffer

0.3 Build interpreter on top of the same API
- the interpreter should not be its own separate engine
- it should call the same PageComposer functions the C++ API uses

0.4 Preserve intent, not old implementation quirks
- old APIs are references
- if an old feature fits cleanly, preserve it
- if an old feature causes architectural confusion, redesign it

0.5 Use explicit compositing semantics
- avoid unclear names like writeTransparentObject / writeFullyTransparentObject
- use clear write intent and policy-based compositing

0.6 Build foundations first
- rendering
- style
- object model
- compositing
- page API
- interpreter
- input/events/focus
- panels/windows
- widgets
- polish/backends

These principles align with the original roadmap emphasis on foundational layers first, clean separation of screen composition from output, and building reusable systems before higher-level UI features. 



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
Build the stable rendering core that everything else depends on.

WHY THIS PHASE COMES FIRST
Without this phase, every later system becomes harder to design and may require refactoring. This matches the earlier roadmap’s emphasis on logical screen representation, diff redraw, and keeping output separate from composition. 

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
- glyph
- style/theme reference
- optional future metadata (priority, flags, etc.)

2.3 ScreenBuffer
Responsibilities:
- logical width/height
- resize
- clear
- get/set cell
- inBounds
- writeChar
- writeString
- fillRect
- drawFrame
- renderToString for debugging/testing

2.4 Surface
Responsibilities:
- wrap a ScreenBuffer
- serve as an off-screen composition target
- later support layering/window composition

2.5 IRenderer
Responsibilities:
- initialize
- shutdown
- present(ScreenBuffer)
- resize

2.6 ConsoleRenderer
Responsibilities:
- all Win32 console-specific work
- UTF-8/output setup
- live console size detection
- diff-based presentation
- resize polling

2.7 FrameDiff
Responsibilities:
- compare previous vs current frame
- dirty row/span detection
- support efficient redraw

2.8 Application loop
Responsibilities:
- initialize renderer
- create main surface
- per-frame update
- per-frame render
- live resize polling

2.9 Resize handling
- startup size detection
- live resize detection
- resize logical surface and previous frame on size changes

OUTPUT OF PHASE 1
A stable logical rendering engine with live resizing and efficient redraw.

FILES / FOLDERS NEEDED

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

App/
  Application.h
  Application.cpp

main.cpp



============================================================
3. PHASE 2 — STYLE SYSTEM (ANSI-LIKE AUTHORING MODEL)
============================================================

PURPOSE
Create a real style system that preserves the authoring feel of ansi.h + BannerThemes.h, without tying the engine to raw ANSI transport strings.

WHY THIS PHASE COMES EARLY
Page composition depends on style semantics. The user should be able to think in “console styling” terms while the backend stays abstract. This directly preserves the semantic theme-building pattern from BannerThemes and the composable styling vocabulary from ansi.h. 
IMPLEMENT IN THIS PHASE

3.1 Color abstraction
Support:
- standard colors
- bright colors
- future-capable 256 / RGB representation

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

3.3 Style composition
Support authoring like:
- style::Bold + style::Fg(Color::Red)
- style::Bold + style::Fg(Color::Red) + style::Bg(Color::Green)

3.4 Style merge behavior
Support:
- replace style
- preserve destination style
- merge style fields

3.5 Semantic theme namespaces
Examples:
- BannerThemes
- UIThemes
- GameThemes later

3.6 Backend mapping
ConsoleRenderer maps Style to backend capabilities
- initial backend may support only a subset
- model should still preserve future richness

3.7 Preserve-style semantics
Needed because earlier docs explicitly relied on preserving destination formatting when no style override was supplied. 

OUTPUT OF PHASE 2
A backend-agnostic style system with console-native authoring feel.

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

Optional if kept outside Rendering:
UI/Styles/
  FrameStyle.h
  UIStyle.h



============================================================
4. PHASE 3 — ASCII OBJECT / ASSET MODEL
============================================================

PURPOSE
Create reusable visual objects for composition.

WHY THIS PHASE COMES BEFORE PAGE API
The page API must have stable primitives to place. The old docs show that authored screens heavily depended on reusable ASCII assets, text blocks, boxes, and generated banner content. 

IMPLEMENT IN THIS PHASE

4.1 AsciiObject core
Support:
- FromFile
- FromTextBlock
- width / height
- loaded state

4.2 Factory helpers
Support:
- Rectangle
- Bordered box
- optional line/block helpers
- optional FIGlet text helper if it fits cleanly

4.3 Asset loading / caching layer
Goal:
- load ASCII assets from Assets directory
- centralize lookup
- avoid repeated disk work

4.4 Optional AsciiBanner integration
- support FIGlet and TOIlet loading
- support renderLines
- support draw into ScreenBuffer
- keep this as an object/text-generation helper, not a renderer

OUTPUT OF PHASE 3
Reusable visual assets that the page layer can place and compose.

FILES / FOLDERS NEEDED

Rendering/Objects/
  AsciiObject.h
  AsciiObject.cpp
  AsciiBanner.h
  AsciiBanner.cpp
  ObjectFactory.h
  ObjectFactory.cpp

Assets/
  ASCII/
  Fonts/
    FIGlet/
    TOIlet/

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
│   │   ├── AsciiObject.h
│   │   ├── AsciiObject.cpp
│   │   ├── AsciiBanner.h
│   │   ├── AsciiBanner.cpp
│   │   ├── ObjectFactory.h
│   │   └── ObjectFactory.cpp
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
│   ├── AssetPaths.h
│   ├── AssetPaths.cpp
│   ├── StringUtils.h
│   ├── StringUtils.cpp
│   ├── Logger.h
│   └── Logger.cpp
│
├── Assets/
│   ├── ASCII/
│   ├── Fonts/
│   │   ├── FIGlet/
│   │   └── TOIlet/
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
2. Build Phase 2 style system
3. Build Phase 3 object/asset system cleanly
4. Build Phase 4 WritePolicy/compositing system
5. Build Phase 5 PageComposer API 1.0
6. Test authored pages in C++
7. Then build Phase 6 interpreter

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