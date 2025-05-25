# Vaeryn's Unreal Library

A collection of generic, utility functionality & useful info for my personal projects in Unreal Engine 5.

Code is currently implemented in a series of Unreal modules, all provided under one plugin.
If any single piece of functionality proves to be useful and worth making standalone, it may
be moved to its own plugin & module in the future.

## Installation

Clone this repository in to your `Plugins` directory

```
git submodule add github.com/vaeryn-uk/vul Plugins/Vul
```

Then add this to your project's `.uproject` file:

```
// ...
  "Plugins": [
    // ...,
    {
        "Name": "Vul",
        "Enabled": true
    }
  ]
// ...
```

### Runtime Type Information

The Vul plugin provides some template classes that use `dynamic_cast` outside of the UE UObject system.
You will need to enable RTTI in your game's module(s) for this to compile.

```
// in MyGameModule.build.cs

    // ...
    bUseRTTI = true;
    // ...
```

This requirement is introduced by Vul's tooltip implementation. If you don't use this, you may not need
to enable RTTI in your modules.

### UnrealYAML

You will also need the UnrealYAML project: https://github.com/jwindgassen/UnrealYAML

```
git submodule add https://github.com/jwindgassen/UnrealYAML Plugins/UnrealYAML
```

_Note: extra functionality we need is currently added to this library in 
a fork: https://github.com/vaeryn-uk/UnrealYAML._

`.uproject` file:

```
    {
        "Name": "UnrealYAML",
        "Enabled": true,
        "TargetAllowList": [
            "Editor"
        ]
    },
```

## New projects

### Style Guide

The widely followed style guide by Allar that I recommend: https://github.com/Allar/ue5-style-guide

### Git

The included `.gitattributes` and `.gitignore` files are a useful starting point for Unreal projects using Git as a version control system.

The ignore files prevents committing big/unintended files, such as as engine cache files or built binaries.

The attributes file configures asset files, such as images or models, to be tracked by Git LFS to reduce storage requirements.

## Functionality

An overview of some of the functionality provided by this plugin.

### Data table sources

Data table sources allow for importing files into Unreal's Data Tables. This offers the following
features over & above native UE data table functionality:

* YAML data files, which tend to be more user-friendly & more concise than JSON.
* Support for multiple files feeding in to one data table, rather than needing a huge, single JSON file.
  * Also support for a single file feeding in to to different data tables, allowing for game-semantic structuring
    of YAML files.
* Test operation to verify what will happen before performing a real import.
* Specify `meta="VulRowName"` in a `UPROPERTY` to have the import automatically populate the row name
  in to the struct directly. Useful for identity/equality checks on row structs.

### Data repositories

* Data table wrapper to coordinate access to data table rows, see `UVulDataRepository`.
* A reference type in row structs that allow for convenient access to other rows, see `FVulDataPtr`.
  These pointers are initialized with enough information to fetch the referenced row without needing
  to go back through the repository.
  * `FVulDataPtr` and its typed version, `TVulDataPtr`, provides a bunch of features as a general-use
    pointer type for rows, so we use this as the only type returned from the repository.
* See `MyProject.ps1`, which contains the `ImportGameData` action demonstrating how repositories can be
  synchronized from a Python script to save booting the editor & needing to manually reimport.

### Number functionality

`TVulMeasure` and `TVulNumber` provide functionality for representing and modifying numeric
values in game logic, such as health and stats in an RPG system.

`TVulCharacterStat` is similar to above, but is simplified for stats that have modifications
bucketed by defined sources, which are each tracked independently.

### Enum functionality

The `VulRuntime::Enum` namespace provides some utility functionality for working with UEnums in your
code.

`VulEnumTable` allows you to combine a config/data-table-driven approach connected to explicit
enum definitions in your code to yield benefits of generic config-driven functionality with the 
freedom to safely hard-code functionality to well-known enum values.

There are two implementations available:
* `TVulDataPtrEnumTable` (recommended) which integrates with `UVulDataRepository` functionality which supports
  references to other tables in your enum rows.
* `TVulEnumDataTable` is simpler and integrates with `UDataTable`s directly.

### Level management

A `UVulLevelManager` can be configured from Vul's project settings, see `UVulRuntimeSettings::LevelSettings`.
This subsystem will manage levels on game start, and it will coordinate the streaming of
data in and out, with a loading screen that will be displayed whilst this occurs. Additional
support is provided for useful functionality such as hooking in to when levels are loaded and automatically
spawning widgets when certain levels are shown, based on the `LevelData` assets that are configured in level
settings.

`UVulLevelManager` makes a best attempt to support loading non-root levels from the editor. If it detects
that you're booting a level that's specified in `UVulRuntimeSettings::LevelSettings` (i.e. a non-root/persistent
level), it will initialize actors and hooks in the same way as if it were being loaded during normal gameplay.

Levels can be marked as cinematic levels, see `UVulLevelData::SequenceSettings`. In this case, the level
manager will play the cinematic and immediately move on to another level when it's completed.

### Enhanced Developer Settings

`UVulDeveloperSettings` provides an extension to project-specific settings for your game with the ability
to mark certain settings' values ignored if a "developer mode" flag is enabled. Useful for quickly switching
values for something you're developing now, and reverting back to the normal settings without needing to reconfigure
everything.

You will need the `DeveloperSettings` module enabled in your `Project.Build.cs` file to extend & use this
feature.

### Centralization of random number generation

See `TVulRngManager` for more information.

### Actor Utility

* See `FVulActorUtil` for a bunch of C++ helpers for writing actor-interacting code. This serves both as code that
  can be used, but also a form of documentation of how to work with actors.
* `TVulComponentCollection` is a data type for conveniently working with zero or more components that share tag.
  `FVulNiagaraCollection` can be used for Niagara components, but also serves as example of our collection's use.

### User Interface

#### UI Notifications

`FVulUiNotification` and `TVulNotificationCollection` provide an efficient creation and storage
of temporary notifications to a player. This provides functionality such as:
* Configuration for how long each notification displays for.
* Automatic widget creation & removal once the notification expires.
* Simple API to replace existing content.
* Extensibility for implementation of different notification types; `FVulHeadlineNotification` is
  the provided implementation.

##### Text Notification Component

`UVulTextNotificationComponent` can be attached to actor to render text to the screen at their position.
This uses the `TVulNotificationCollection` above, using a standard notification implementation, `FVulTextNotification`,
which can render text & icons.

#### MultiBorder widget

A reusable variation of a border widget that allows defining multiple, overlaid borders in its style definition.
See `UVulMultiBorder`.

#### Animated Highlight widget

`UVulAnimatedHighlight` wraps content, providing a highlighting effect when the content is hovered.
Intended to be used alongside tooltip functionality.

#### Collapsed panel widget

Contains content that is shown/hidden when a connected button is pushed. See `UVulCollapsedPanel`.

#### Tooltip

`UVulTooltipSubsystem` provides a simple API for displaying a tooltip to the user as they interact
with the world/interface. This is implemented independently of Unreal's native tooltip support for now,
but maybe integrated later if a need arises.

To use this, create a widget class and implement `IVulTooltipWidget` in C++, then create
a widget UMG blueprint that extends it and select the UMG in Vul Runtime's project settings as the
widget class to use.

Once configured, `UVulTooltipSystem` can be called to show and hide a tooltip per player. When
showing the tooltip, you must provide a tooltip data instance (which extends `FVulTooltipData`).
This allows you to define what data can be displayed in a tooltip and provides a structured system
for consistent tooltips across your game.

`UVulTooltipUserWidget` is provided to save writing tooltip-triggering logic for widgets that
should trigger a tooltip when hovered. If you extend this, all you need to do is define the tooltip
data that widget will trigger.

This tooltip system also integrates with Vul's Rich Text support.

#### Rich Text

`UVulRichTextBlock` seeks to simplify customization and workflows when using Unreal's rich text system.
This encapsulates a bunch of boilerplate code and allows your project to extend this widget class
to customize available rich text support across your entire project. Use your extending class in all
places where text is used in the UI.

The approach is an extension of Unreal's
[CommonUI](https://docs.unrealengine.com/5.0/en-US/common-ui-plugin-for-advanced-user-interfaces-in-unreal-engine/).

See the code in `UVulRichTextBlock` for customization documentation, but as a quick setup overview:

- As per CommonUI, configure relevant settings:
  - `Common UI Editor` -> `Template Text Style`. This will be applied as the rich text default style override
    for newly created rich text blocks.
  -  Create a BP that extends `UCommonUIRichTextData`
  - Select this in `Common UI Framework` -> `Default Rich Text Data Class`
  - If you want icon support, create a data table with a row structure of `FVulRichTextIconDefinition`.
    - Set this table in Vul library settings, `IconSet`.
    - Use syntax `<vi i=\"<row-name>\"</>` to render icons
    - Optionally override `UVulRichTextIcon` to further customize appearance, then select your override 
      in Vul library settings. Note currently widget blueprints extensions of this icon widget do not work;
      they must be in C++.
    - These icons extend Rich Text Icons with support for color and background configuration.
  - You may define a rich text styles table as per CommonUI, although the Vul rich text support
    doesn't utilize or enhance this in any way.
- Create a widget blueprint with parent `UVulRichTextTooltipWrapper`. This is used whenever
  we replace rich text markup with something with a tooltip.
  - Set this Vul's project settings' `Rich Text Tooltip Wrapper`.
- Review `UVulRichTextBlock` comments, and override relevant methods to add rich text support
  specific for your project.

#### Generic Horizontal/Vertical Box Spacing

A property that allows you to control spacing between elements in a container that can
be seamlessly switched between horizontal and vertical boxes. See `FVulElementSpacer`.

#### Style Generation

This editor functionality aims to improve the creation & maintenance of a consistent
set of styles for CommonUI UI components. For example, your project may have primary,
secondary and danger buttons. These three styles would be defined as variations in
a generator, then you can use the in-editor `Generate` functionality to create or
update the corresponding button styles. This takes the manual work out of updating 
all of your styles, and provides a single editor pane where all your style diffferences
are listed in place.

Generators provided:
- `VulButtonStyleGenerator`
- `VulTextStyleGenerator`
- `VulBorderStyleGenerator`

Note that generators only allow variation of a small subset of available properties right
now. These may be expanded as use-cases arise.

### `FVulField` Serialization

`FVulField` and its related components provide an alternative approach to parts of the Unreal Header Tool 
and the reflection system. They make heavy use of templates and modern C++ concepts.

Use these components to describe and serialize your C++ types, allowing more flexibility than the 
constraints typically associated with `USTRUCT`, `UCLASS`, and similar macros.

[Documentation is here](./Docs/vul-fields.md).

### Copy on write pointer

`TVulCopyOnWritePtr` is useful to avoid unnecessary copying of data in scenarios where you are often reading
but sometimes copying.

### Loot model

`TVulAdaptiveLootModel` provides an implementation of rolling random rewards. The implementation provides
information allowing you to easily implement loot systems that tend towards coherent builds. I.e. as a player
acquires loot of a particular build or strategy, complimentary loot to that style is more likely to come.