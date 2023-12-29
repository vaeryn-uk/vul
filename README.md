# Vaeryn's Unreal Library

A collection of generic, utility functionality for my personal projects in Unreal Engine 5.

## Installation

This library is intended to be checked out as a git submodule in your project's `Source` directory.

```
git submodule add github.com/vaeryn-uk/vul Source/Vul
```

Then add this to your project's `.uproject` file:

```
// ...
  "Modules": [
    // ...
    {
        "Name": "VulRuntime",
        "Type": "Runtime",
        "LoadingPhase": "Default"
    }
  ]
// ...
```

And `<MyProject>.Target.cs` & `<MyProject>Editor.Target.cs`:

```csharp
    // ...
    ExtraModuleNames.AddRange(new string[] { "VulRuntime" });
    // ...
```

## New projects

### Git

The included `.gitattributes` and `.gitignore` files are a useful starting point for Unreal projects using Git as a version control system.

The ignore files prevents committing big/unintended files, such as as engine cache files or built binaries.

The attributes file configures asset files, such as images or models, to be tracked by Git LFS to reduce storage requirements.
