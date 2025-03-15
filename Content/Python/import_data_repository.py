import os
import unreal

"""
Loads a UVulDataRepository and imports all connected sources.

Requires a UE_VUL_DATA_REPO environment variable containing the repo's asset path,
e.g. "/Game/MyProject/Data/DTR_GameData.DTR_GameData"
"""

asset_path = os.getenv("UE_VUL_DATA_REPO") or ""

if not asset_path.startswith("/Game/"):
    raise RuntimeError("env UE_VUL_DATA_REPO is not a valid asset path")

expected_class = unreal.VulDataRepository

unreal.log(f"loading asset {asset_path}")

asset = unreal.EditorAssetLibrary.load_asset(asset_path)

if not asset:
    raise RuntimeError(f"asset {asset_path} not found")

if not isinstance(asset, expected_class):
    raise RuntimeError(f"asset {asset_path} was {type(asset)}, not {type(expected_class)}")

unreal.log(f"beginning import")

unreal.VulEditorBlueprintLibrary.import_connected_data_sources(asset)

unreal.log(f"import complete")