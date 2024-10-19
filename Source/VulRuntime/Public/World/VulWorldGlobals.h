#pragma once

#include "CoreMinimal.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/Object.h"

/**
 * Provides utility accessors to classes & functionality via a UWorld.
 */
namespace VulRuntime::WorldGlobals
{
	/**
	 * Gets a UWorld from the given UObject, checking that it exists/is valid.
	 */
	VULRUNTIME_API UWorld* GetWorldChecked(const UObject* Object);

	/**
	 * Gets a GameInstance subsystem of the requested type from the given UObject's world, checking that it exists/is valid.
	 */
	template <typename Subsystem>
	Subsystem* GetGameInstanceSubsystemChecked(const UObject* Ctx)
	{
		auto SS = GetWorldChecked(Ctx)->GetGameInstance()->GetSubsystem<Subsystem>();
		checkf(IsValid(SS), TEXT("Cannot find GameInstance subsystem"))
		return SS;
	}

	/**
	 * Gets a GameState of the requested type from the given UObject's world, checking that it exists/is valid.
	 */
	template <typename GameState>
	GameState* GetGameStateChecked(UObject* Ctx)
	{
		auto GS = GetWorldChecked(Ctx)->GetGameState<GameState>();
		checkf(IsValid(GS), TEXT("Cannot find GameState"))
		return GS;
	}

	/**
	 * Gets the first player controller of the requested type from the given UObject's world, checking that it exists/is valid.
	 */
	template <typename PlayerController = APlayerController>
	PlayerController* GetFirstPlayerController(UObject* Ctx)
	{
		auto PC = Cast<PlayerController>(GetWorldChecked(Ctx)->GetFirstPlayerController());
		checkf(IsValid(PC), TEXT("Cannot find player controller"))
		return PC;
	}

	/**
	 * Attempts to resolve a player controller that is viewing the given object.
	 */
	template <typename PlayerController = APlayerController>
	PlayerController* GetViewPlayerController(const UObject* Ctx)
	{
		for (int I = 0; I < UGameplayStatics::GetNumPlayerControllers(Ctx); I++)
		{
			const auto PC = UGameplayStatics::GetPlayerController(Ctx, I);
			const auto LP = PC->GetLocalPlayer();

			if (GEngine && GEngine->GameViewport.Get() && LP && GEngine->GameViewport.Get() == LP->ViewportClient.Get())
			{
				return PC;
			}
		}

		return nullptr;
	}

	/**
	 * Gets the first local player of the requested type from the given UObject's world, checking that it exists/is valid.
	 */
	template <typename LocalPlayer = ULocalPlayer>
	ULocalPlayer* GetFirstLocalPlayer(UObject* Ctx)
	{
		auto PC = Cast<APlayerController>(GetWorldChecked(Ctx)->GetFirstPlayerController());
		if (ensureAlwaysMsgf(IsValid(PC), TEXT("Cannot get LocalPlayer from no player contoller")))
		{
			return nullptr;
		}
		const auto LP = Cast<LocalPlayer>(PC->GetLocalPlayer());
		if (ensureAlwaysMsgf(IsValid(LP), TEXT("No LocalPlayer found on player controller")))
		{
			return nullptr;
		}

		return LP;
	}

	/**
	 * Returns the provided subsystem, checking it exists (if the player controller is a local player).
	 *
	 * If not a local player controller, this returns nullptr.
	 */
	template <typename Subsystem>
	Subsystem* GetLocalPlayerSubsystem(const APlayerController* PlayerController)
	{
		auto Player = PlayerController->GetLocalPlayer();
		if (!Player)
		{
			return nullptr;
		}

		auto Ret = Player->GetSubsystem<Subsystem>();
		checkf(IsValid(Ret), TEXT("Could not resolve local player controller"));
		return Ret;
	}

}