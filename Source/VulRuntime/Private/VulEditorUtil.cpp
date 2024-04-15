#if WITH_EDITOR
#include "VulEditorUtil.h"
#include "VulRuntime.h"
#include "EditorDialogLibrary.h"

void FVulEditorUtil::Output(
	const FText& Title,
	const FText& Message,
	const EAppMsgCategory& Category,
	bool ShowDialog,
	UObject* Details)
{
	if (Category == EAppMsgCategory::Warning) {
		UE_LOG(LogVul, Warning, TEXT("%s: %s"), *Title.ToString(), *Message.ToString());
	} else if (Category == EAppMsgCategory::Info || Category == EAppMsgCategory::Success)
	{
		UE_LOG(LogVul, Display, TEXT("%s: %s"), *Title.ToString(), *Message.ToString());
	} else
	{
		UE_LOG(LogVul, Error, TEXT("%s: %s"), *Title.ToString(), *Message.ToString());
	}

	// TODO: Print details object in log message?

	if (!ShowDialog)
	{
		return;
	}

	if (IsValid(Details))
	{
		FEditorDialogLibraryObjectDetailsViewOptions Opts;
		UEditorDialogLibrary::ShowObjectDetailsView(Title, Details, Opts);
	} else
	{
		UEditorDialogLibrary::ShowMessage(
			Title,
			Message,
			EAppMsgType::Ok,
			EAppReturnType::No,
			Category
		);
	}
}

void FVulEditorUtil::Output(
	const FString& Title,
	const FString& Message,
	const EAppMsgCategory& Category,
	bool ShowDialog,
	UObject* Details
) {
	return Output(FText::FromString(Title), FText::FromString(Message), Category, ShowDialog, Details);
}

#endif