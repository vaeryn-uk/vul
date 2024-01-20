#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

class VULEDITOR_API FVulEditorCommands : public TCommands<FVulEditorCommands>
{
public:
	TSharedPtr< FUICommandInfo > ImportAllConnectedSources;
	FVulEditorCommands()
		: TCommands( TEXT("VulEditor"), NSLOCTEXT("Contexts", "VulEditor", "Vul Editor Actions"), NAME_None, FAppStyle::GetAppStyleSetName() )
	{}

	virtual void RegisterCommands() override;
};
