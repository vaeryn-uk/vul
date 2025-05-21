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
* Define known types  via `VUL_FIELD_` macros.
* Supports polymorphism via discriminator fields (e.g. a `type` field).