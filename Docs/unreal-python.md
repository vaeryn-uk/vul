﻿# Python Scripting in UE

These steps describe how to get a Python workflow set up in your IDE which
utilizes Unreal's built-in Python interpreter. This is aimed at writing & running
python outside the editor.

## Editor Setup

* Ensure your project has the Python plugin enabled.
  ![img/python-plugin.png](img/python-plugin.png)
* Review Python configuration in Project Settings:
  * Disable `Pip Strict Hash Check` if you'll be installing your own Python Dependencies.
* Review Python configuration in Editor Settings:
  * Enable `Developer Mode` to have UE generate python stubs for the `unreal` Python library.
    This will be useful for adding auto-complete to your IDE. Note this could also be enabled
    in project settings, but in editor settings, its specific to you.
* Restart the editor

## IDE setup (Rider)

* Add a `System Interpreter` which points at the embedded Python executable, e.g.
  ![img/rider-ue-python-interpreter.png](img/rider-ue-python-interpreter.png)
  * Do not use a Virtualenv interpreter as this will not resolve dependencies correctly in Rider.
  * Dependencies do not need to be located here via custom Interpreter paths, as we won't be
    executing Python from the IDE directly; it'll be running inside Unreal.
* For auto-complete, you'll need to `Attach` directories with python code in your Rider solution:
  * `MyProjectDir/Intermediate/PythonStubs` - this will only exist if you've followed the Editor Setup above.
    This defines the unreal functionality via `import unreal`.
  * `MyProjectDir/Intermediate/PipInstall/Lib/site-packages` - contains the pip-installed libraries installed
    from plugins.
* The Unreal stubs are provided in 1 file which is very long. You'll need to increase Rider's max file size
  for it to be scanned by Rider:
  * 2x`Shift` > `Edit Custom Properties...`
  * Add the following line to the config file that opens:
    ```
    idea.max.intellisense.filesize = 25000
    ```
  * Restart Rider.

The end result is auto-complete for the `unreal` module:

![img/rider-python-autocomplete.png](img/rider-python-autocomplete.png)

## Dependencies

Unreal supports declaring your own Python dependencies in Plugins (`.uplugin`), but seemingly
not projects directly (`.uproject`). A workaround is to create a plugin in your project solely
to specify the dependencies and have them accessible from the embedded UE python interpreter.
Your Python code could live in this plugin to keep your project organized. 

A plugin does not need to contain any source code or content; it can simply be a `.uplugin` file at
`MyProjectDir/Plugins/MyProjectPython/MyProjectPython.uplugin`, which looks something like:

```
{
	"FileVersion": 3,
	"FriendlyName": "HavenPython",
	"PythonRequirements": [
		{
			"Platform": "All",
			"Requirements": [
				"requests==2.32.3"
			]
		}
	],
	"Modules": []
}
```

Each string entry in the `Requirement` array is a line of pip requirements.txt file.

You can add an Unreal module in the usual way (`Source/MyProjectPython.Build.cs` etc) and include
it in the `Modules` definition above to introduce CPP code. This may be a natural place for some binding
code.

## Executing your code

See the `PythonScript` action in [MyProject.ps1](../MyProject.ps1) which contains an 
example of how to run a script from the CLI quickly without needing to boot the editor.