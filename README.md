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
* [TODO] Automated triggering of imports on project start/preview start/cook etc.
* [TODO] Automatic population of row name references (below)

### Data repositories

* Data table wrapper to coordinate access to data table rows, see `UVulDataRepository`.
* A reference type in row structs that allow for convenient access to other rows, see `FVulDataPtr`.
  These pointers are initialized with enough information to fetch the referenced row without needing
  to go back through the repository.
  * `FVulDataPtr` and its typed version, `TVulDataPtr`, provides a bunch of features as a general-use
    pointer type for rows, so we use this as the only type returned from the repository.

### Number functionality

`TVulMeasure` and `TVulNumber` provide functionality for representing and modifying numeric
values in game logic, such as health and stats in an RPG system.

`TVulCharacterStat` is similar to above, but is simplified for stats that have modifications
bucketed by defined sources, which are each tracked independently.

### Level management

A `AVulLevelManager` can be dropped in to a root level and levels configured. Management of streaming the
data in/out will be handled for you and a loading screen can be displayed whilst this occurs. Additional
support is provided for useful functionality such as hooking in to when levels are loaded and automatically
spawning widgets when certain levels are shown.

### Centralization of random number generation

See `TVulRngManager` for more information.

### User Interface

#### UI Notifications

`FVulUiNotification` and `TVulNotificationCollection` provide an efficient creation and storage
of temporary notifications to a player. This provides functionality such as:
* Configuration for how long each notification displays for.
* Automatic widget creation & removal once the notification expires.
* Simple API to replace existing content.
* Extensibility for implementation of different notification types; `FVulHeadlineNotification` is
  the provided implementation.

#### MultiBorder widget

A reusable variation of a border widget that allows defining multiple, overlaid borders in its style definition.
See `UVulMultiBorder`.

#### Collapsed panel widget

Contains content that is shown/hidden when a connected button is pushed. See `UVulCollapsedPanel`.

#### Tooltip

`UVulTooltipSubsystem` provides a simple API for displaying a tooltip to the user as they interact
with the world/interface. This is implemented independently of Unreal's native tooltip support for now,
but maybe integrated later if a need arises.

To use this, create a widget class and implement `IVulTooltipWidget` in CPP, then create
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
  - If you want CommonUI's icon support:
    - Create a data table asset with `RowStruct=RichTextIconData`
    - Select this in your BP.
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