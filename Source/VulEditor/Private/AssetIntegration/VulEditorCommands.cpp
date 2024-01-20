#include "AssetIntegration/VulEditorCommands.h"

#define LOCTEXT_NAMESPACE "FVulEditorModule"

void FVulEditorCommands::RegisterCommands()
{
	UI_COMMAND(
		ImportAllConnectedSources,
		"Import connected data sources",
		"Run a fresh import on all data sources connected to this repository",
		EUserInterfaceActionType::Button,
		FInputChord()
	)
}

#undef LOCTEXT_NAMESPACE
