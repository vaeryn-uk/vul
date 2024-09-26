# Navmesh

Unreal's navigation system uses [Recast](https://recastnav.com/) at its core. Navigation is 
enabled by placing a `NavMeshBoundsVolume` in a level. This additionally creates a 
`RecastNavMesh` actor if one is not already in place.

## Using navmeshes with `VulLevelManager`

Our level manager is based on the idea that levels are streamed-in to the world as levels
are changed. This means that standard Unreal behaviour, such as world-init hooks or level
blueprints are not executed. This effects nav meshes, as when a level starts, there seems
to be logic that runs that loads the relevant navigation data in to `UNavigationSystemV1`.

To ensure that navmeshes work correctly with our level manager, you must:
* Create one `NavMeshBoundsVolume` in the root level. The root level does not need any geometry, but this actor
  is required to ensure that nav mesh data is initialized when the game first boots (thus is available when
  streamed-levels are shown).
    * As a consequence, a `RecastNavMesh` actor should be created. This should also be in the root level.
* Create a `NavMeshBoundsVolume` in your playable level(s) as required and size them around your geometry.
    * Note that no additional `RecastNavMesh` will be created in this level; all nav data will sit in the root-level
      actor, which ensures its available as this is done when root world is initialized.
* TODO: Does this create any issues with swapping levels that both have navmeshes? 
  How does the navsystem know which navmesh to query against when a different level (thus different navmeshbounds) is used?

## Multiple meshes

Unreal has support for different navmeshes for use by different actors (agents). This appears to
be poorly documented, but the following setup worked for me:

* Before adding any Nav-related actors in your world, edit `Project Settings > Navigation System > Supported Agents`.
* Define the different types of agents you have in your game.
  * For example, you may want different navmeshes for agents of different sizes.
* Add necessary `NavMeshBoundsVolume`s in your world.
  * You'll notice that a `RecastNavMesh` actor is created (if not already) for each of the agents
    you define in project settings.
* Importantly, you do not have to tag individual controllers or pawns with which navmesh they will use explicitly.
  Instead, `UNavComponent`s on actors define their ideal properties, e.g. `NavAgentRadius`, `NavAgentStepHeight`,
  and the nav system will try to find the best matching navmesh.
  * `UNavigationSystemV1::GetNavDataForProps` is the engine code that is responsible for this selection.
* Note: multiple meshes are not displayed in the world view in the editor by default. Locate the `RecastNavMesh`(s)
  in the Outliner and tick their `Enable Drawing` setting, and you'll be able to see the meshes generated
  when enabling `Show > Navigation` in the viewport.
  