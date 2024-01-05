#include "VulEditorUtil.h"
#include "EditorDialogLibrary.h"
#include "VulEditor.h"

void FVulEditorUtil::Output(
	const FText& Title,
	const FText& Message,
	const EAppMsgCategory& Category,
	bool ShowDialog,
	UObject* Details)
{
	if (Category == EAppMsgCategory::Warning) {
		UE_LOG(LogVulEditor, Warning, TEXT("%s: %s"), *Title.ToString(), *Message.ToString());
	} else if (Category == EAppMsgCategory::Info || Category == EAppMsgCategory::Success)
	{
		UE_LOG(LogVulEditor, Display, TEXT("%s: %s"), *Title.ToString(), *Message.ToString());
	} else
	{
		UE_LOG(LogVulEditor, Error, TEXT("%s: %s"), *Title.ToString(), *Message.ToString());
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
