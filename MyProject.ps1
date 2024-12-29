<#
.SYNOPSIS
Example Unreal Project powershell script

.DESCRIPTION
Automates common operations needed for developing Unreal projects outside of the Unreal Editor.
This is akin to a `Makefile` in a Linux project, allowing you to define common tasks, such as
cooking & building the project.

Copy this file in to the root of your project then rename and adapt for your project.
The commands provided here serve as a useful starting point & reference for some common UE commands for
building on Windows.

In powershell, run the following for help on usage syntax:

  Get-Help -Full .\MyProject.ps1

.PARAMETER Action
Which operation to perform.

Cook          - Cook project content. Required to run the game outside the editor.
CookAndBuild  - Cook and build the project and write the game files into `Builds` for a standalone version.
Zip           - Takes the latest build project and zips it up as the given `-Version`.
Profile       - Starts Unreal Insights for profiling
#>

param (
    [Parameter(Mandatory = $true)]
    [String]$Action,
    [Parameter(Mandatory = $false)]
    [String]$Version
)

### Review these common variables for your project. ###
# Unreal installation directory.
$UeDir = "C:\Program Files\Epic Games\UE_5.5"
# Your project name. Should match the .uproject file name.
$ProjectName = "MyProject"
# Where your project lives. We assume this file is in it root.
$ProjectDir = $PSScriptRoot
# The editor version we initiate. Must exist ing $UeDir.
$EditorExe = "UnrealEditor-Win64-DebugGame-Cmd.exe"

$DateTime = Get-Date -Format "yyyy.MM.dd-HH.mm.ss"

if ($Action -eq "Cook")
{
    Start-Process `
         -FilePath "$UeDir\Engine\Binaries\Win64\$EditorExe" `
         -ArgumentList (
    "`"$ProjectDir\$ProjectName.uproject`" -run=Cook  -TargetPlatform=Windows  -unversioned -fileopenlog",
    "-abslog=`"$UeDir\Engine\Programs\AutomationTool\Saved\Cook-$DateTime.txt`" -stdout -CrashForUAT",
    "-unattended -NoLogTimes",
    "-UTF8Output"
    ) `
         -NoNewWindow -Wait

    Write-Output "-------------------"
    Write-Output "   Cook complete   "
    Write-Output "-------------------"
}
elseif ($Action -eq "CookAndBuild")
{
    Start-Process `
        -FilePath "$UeDir\Engine\Build\BatchFiles\RunUAT.bat" `
        -ArgumentList (
    "-ScriptsForProject=`"$ProjectDir\$ProjectName.uproject`"",
    "Turnkey -command=VerifySdk -platform=Win64 -UpdateIfNeeded -EditorIO",
    "-EditorIOPort=54155  -project=`"$ProjectDir\$ProjectName.uproject`" BuildCookRun",
    "-nop4 -utf8output -nocompileeditor -skipbuildeditor -cook",
    "-project=`"$ProjectDir\$ProjectName.uproject`" -target=$ProjectName",
    "-unrealexe=`"$UeDir\Engine\Binaries\Win64\$EditorExe`"",
    "-platform=Win64 -installed -stage -archive -package -build -clean -pak -iostore ",
    "-compressed -prereqs -archivedirectory=`"$ProjectDir/Builds`" -clientconfig=Development",
    "-nocompile -nocompileuat"
    ) `
        -NoNewWindow -Wait

    Write-Output "-------------------"
    Write-Output "   Build complete  "
    Write-Output "-------------------"
}
elseif ($Action -eq "Zip")
{
    if ( [string]::IsNullOrEmpty($Version))
    {
        Write-Host "Cannot zip without a version string (-Version)"
        exit 1
    }

    $ZipPath = "$ProjectDir\Builds\$ProjectName_$Version.zip"

    if (Test-Path $ZipPath)
    {
        Write-Output "Cannot zip. $ZipPath already exists"
        exit 1
    }

    $ToCompress = "$ProjectDir\Builds\Windows"

    if (-not (Test-Path $ToCompress))
    {
        Write-Output "Cannot zip. Build $ToCompress does not exist"
        exit 1
    }

    Write-Output "Compressing to: $ZipPath"

    Compress-Archive -Path $ToCompress -DestinationPath $ZipPath

    Write-Output "-------------------"
    Write-Output "   Zip complete  "
    Write-Output "-------------------"
}
elseif ($Action -eq "Profile")
{
    Start-Process -Path "$UeDir\Engine\Binaries\Win64\UnrealInsights.exe"

    Write-Output "-------------------------------"
    Write-Output "   Unreal Insights started"
    Write-Output "-------------------------------"
}
else
{
    Write-Output "Unrecognized usage"
    Write-Output ""
    Get-Help "$ProjectDir\$ProjectName.ps1" -Full
}


