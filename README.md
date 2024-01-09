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
* Test operation to verify what will happen before performing a real import.
* [TODO] Automated triggering of imports on project start/preview start/cook etc.
* [TODO] Automatic population of row name references (below)

### Data table rows

* [TODO] A data table row base class that always includes its RowName
* Data table wrapper to coordinate access to data table rows, see `UVulDataRepository`.
* A reference type in row structs that allow for convenient access to other rows, see `UVulDataRef`.
  These references are initialized with enough information to fetch the referenced row without needing
  to go back through the repository.