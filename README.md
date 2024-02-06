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

### UI Notifications

`FVulUiNotification` and `TVulNotificationCollection` provide an efficient creation and storage
of temporary notifications to a player. This provides functionality such as:
* Configuration for how long each notification displays for.
* Automatic widget creation & removal once the notification expires.
* Simple API to replace existing content.
* Extensibility for implementation of different notification types; `FVulHeadlineNotification` is
  the provided implementation.