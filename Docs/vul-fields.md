# Vul Fields: Reflection-based Serialization

WIP: To be migrated here from main `README.md`.

## Metadata & Schemas

_Highly experimental. Likely doesn't work for complex object structures not described below._

* Can infer metadata about field structures to generate JSON schema & typescript describing
  the serialized data format of your types.
* Accessed via `FVulFieldSerializationContext.Describe()`
  * Attached to context as serialization options will influence output schemas.
* Mostly hooked in around `FVulFieldSet VulFieldSet() const` definitions so custom
  metadata implementations are less likely to be needed.
* Non-VulFieldSet types will need their own `TVulMeta::Describe()`. This plugin
  provides this for common types, such as strings, numeric and UE pointers & containers.
* Define known types  via `VULFLD_` macros.
* Supports polymorphism via discriminator fields (e.g. a `type` field).
  ```
  Event: { id: string, type: enum }
  DamageDoneEvent: { amount: number } // extends Event, type=DamageDone.
  PlayerJoinedEvent: { playerId: string } // extends Event, type=PlayerJoined.
  ```
  Where Event is our base type, and "type" is the discriminator field.
* You must export types from your game code via `MYGAMEPROJECT_API` macros when using `VULFLD_` macros.
  This includes `DECLARE_ENUM_TO_STRING()`; should be `MYGAMEPROJECT_API DECLARE_ENUM_TO_STRING()`.
* Referencing is supported in TS and JSON schemas. They will export a special type, `VulFieldRef`.
* Unlike JSON schema Typescript will only export known types via `VULFLD_`. 

### TODOs

* Use new meta system to automatically deserialize based on discriminator.