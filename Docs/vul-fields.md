# Vul Fields: Reflection-based Serialization

This requires the Unreal `Json` module. C++ `concept` compiler support is also required (C++20).

This header provides a simplified entry point to the Vul Field system. Include it in external modules 
for access to core functionality:

```c++
include "Field/VulFieldSystem.h";
```

The field system is designed to support automatic serialization and deserialization of C++
classes outside the Unreal reflection system. This template-driven feature allows serialization
of both your native C++ and Unreal C++ types (`USTRUCT`, `UOBJECT`, etc.) with minimal
boilerplate code.

At the heart of this system is `FVulField`, which wraps pointers that will be read from/written
to. As we're generally defining types that include properties that themselves need to be
serialized, a `FVulFieldSet` is used to conveniently describe how your objects should be
serialized.

```c++
int I = 13;
FString Str = "hello world";

// Define a field set; this yields a serialized object.
FVulFieldSet Set;
Set.Add(FVul::Create(&I), "int");
Set.Add(FVul::Create(&Str), "str");

// Ctx can be used to configure options and report detailed errors that may occur.
FVulFieldSerializationContext Ctx;
TSharedPtr<FJsonValue> Result;
bool Ok = Set.Serialize(Result, Ctx);
```

Result (as a JSON string for demonstration):

```json
{ "int": 13, "str": "hello world" }
```

[See the tests for more examples](../Source/VulRuntime/Private/Field/Tests/TestVulField.cpp).

To plug your types into the system, serialization and deserialization template specializations
based on `TVulFieldSerializer<T>` must exist. This can be done in several ways:

* Implement `FVulFieldSet VulFieldSet() const` on your type to return object-based representations.
  This is the recommended method and should handle most use-cases for object-like types.
* Implement `FVulField VulField() const` for types that can be represented as a single field
  (e.g. a string).
* Implement your own `TVulFieldSerializer<T>` specialization for your type to implement custom
  (de)serialization logic.
* Some types will need special consideration, such as those derived from `UObject`;
  these are described below.

There are definitions for common types already in
[VulFieldCommonSerializers.h](../Source/VulRuntime/Public/Field/VulFieldCommonSerializers.h).
This includes container types such as `TArray`, `TMap`, `TOptional`, and `TSharedPtr`, so you only
need to define serializers for your concrete types; containerized and pointer versions will be
inferred.

Importantly, while the field system uses `FJsonValue` and associated types, it is not explicitly
limited to JSON. `FJsonValue` is chosen as a portable, standard data representation target since
it leverages existing Unreal Engine infrastructure.

#### Polymorphic types

Polymorphic classes are supported for serialization and deserialization by providing a custom
`TVulFieldSerializer`. An example is included in the tests:
`TVulFieldSerializer<TSharedPtr<FVulFieldTestTreeBase>>`, which demonstrates serializing a
recursive tree structure where each node is a different subclass of the node base class. This
requires `TSharedPtr` instances and a custom `type`-based discriminator in the serialized data so
the deserializer knows which instance to create.

#### UObject

UCLASS objects can be serialized and deserialized by implementing `IVulFieldSetAware`. Unlike
non-UObjects, implementing your own serializers is not recommended, as UObject construction is
already handled and requires careful management.

#### TScriptInterface<>

Properties of this type are supported if used with UObjects. For deserialization, the input must
be a shared reference to a previously described `UObject` which we link to. Internally, a check
ensures that the resolved object satisfies the specified interface.

For serialization, this behaves similarly to a `UObject*` for the underlying pointer; the object
must implement `IVulFieldSetAware` to produce a useful serialized representation.

#### UEnum

Enums are serialized and deserialized as their string form. Your enums must implement
`EnumToString` to be compatible with the provided serializer. Use the `DECLARE_ENUM_TO_STRING`
and `DEFINE_ENUM_TO_STRING` macros provided by UE for this purpose.

#### Shared references

Shared references provide two main features:

1. When serializing, repeated appearances of the same object are replaced with a string
   reference, reducing duplication.
2. When deserializing, references resolve to the same object instances.

This behavior is enabled by default but can be disabled using the
`VulFieldSerializationFlag_Referencing` flag in the `FVulFieldSerializationFlags` context.

This can be set for the entire serialization, or specific to certain paths:

```c++
	FVulFieldSerializationContext Ctx;
	// Disable any referencing for objects in the path `.foo.bar.*` in your object graph.
	// The path param is optional.
	Ctx.Flags.Set(VulFieldSerializationFlag_Referencing, false, ".foo.bar.*");
```

Here's what serialized data might look like for an array of characters where we have the same
character twice:

```json
[
  { "name": "Thor", "health": 13, "strength": 5, "weapon": "hammer" },
  "Thor" // referenced.
]
```

`FVulFieldSet` can have a field added as an identifier. This becomes the string reference that's
used when an object is repeated. For example:

```c++
	virtual FVulFieldSet VulFieldSet() const 
	{
		FVulFieldSet Set;
		Set.Add(FVulField::Create(&Name), "str", true); // This is the value that'll be used as a reference.
		Set.Add(FVulField::Create(&Health), "health");
		Set.Add(FVulField::Create(&Weapon), "weapon");
		Set.Add(FVulField::Create(&Strength), "strength");
		return Set;
	}
```

If needed, you can implement a custom `TVulFieldRefResolver`, which can be specialized for your types.

**Extracting References**

An `FVulSerializationContext` can have its `ExtractReferences` property set to true ahead of serialization
to extract all references to a known location. This will change root of the serialization output:

```
{
  "refs": {
    // ref => object map of things that are referenced.
  },
  "data": {
    // your serialized data, with references back to "refs"
  }
```

The example above would become:

```json
{
  "refs": {
    "Thor": { "name": "Thor", "health": 13, "strength": 5, "weapon": "hammer" }
  },
  "data": [
    "Thor", // referenced.
    "Thor"  // referenced.
  ]
}
```

The advantage of this approach is that consumers of serialized output know where they will
always find the real data which simplifies resolving references.

Note that deserialization support for extracted references is not yet implemented.

## Metadata & Schemas

*This subsystem is experimental and subject to change. It currently handles simple and
moderately nested structures well, but complex polymorphic graphs may require additional
customization.*

The metadata system is designed to infer descriptive schemas from existing `FVulField`
definitions. These schemas can be exported as:

* **JSON Schema** – for validation, tooling integration, and cross-language compatibility.
* **TypeScript Definitions** – for use in web-based tools or game logic written in TypeScript.

### How It Works

Metadata is inferred primarily from implementations of `FVulFieldSet VulFieldSet() const`.
These provide a complete view of an object's field layout and types. At runtime, metadata
generation is triggered via:

```c++
FVulFieldSerializationContext Context;
TSharedPtr<FVulFieldDescription> Description = MakeShared<FVulFieldDescription>();

if (!Context.Describe<MyType>(Description)) {
    UE_LOG(LogTemp, Display, TEXT("Metadata generation failed"));
}
```

Which can then be output:
```c++
// JSON schema.
TSharedPtr<FJsonValue> Schema = Description->JsonSchema();
FString Schema = VulRuntime::Field::JsonToString(Schema);

// Typescript definitions.
FString Definitions = Description->TypescriptDefinitions();
```

The context must be configured similarly to how it would be for actual serialization, as schema
generation respects context-specific flags and options.

While `VulFieldSet()` can technically include runtime logic, this is discouraged because the metadata 
system relies on default-constructed instances, where such logic may not be safe or meaningful.

For non-`FVulFieldSet` types, metadata must be explicitly defined by specializing
`TVulMeta::Describe<T>()`. The plugin includes default implementations for:

* Primitive types (e.g. `int`, `FString`)
* Common Unreal Engine types & containers (`FGuid`, `TArray`, `TMap`, `TOptional`, `TSharedPtr`)
* Other Vul types, such as `TVulDataPtr` and `TVulCopyOnWritePtr`

Custom types not based on `FVulFieldSet` must have a corresponding `TVulMeta::Describe`
specialization to be compatible.

### Sample Output

Here’s a sample of the JSON Schema that might be generated for a simple settings struct (taken from the
[tests](../Source/VulRuntime/Private/Field/Tests/TestVulFieldMeta.cpp)).

```json
{
  "type": "object",
  "properties": {
    "UVulTestFieldReferencing": {
      "oneOf": [
        {
          "$ref": "#definitions/VulTestFieldReferencing"
        },
        {
          "$ref": "#definitions/VulFieldRef"
        }
      ]
    },
    "UVulTestFieldReferencingContainer1": {
      "$ref": "#definitions/VulTestFieldReferencingContainer1"
    },
    "UVulTestFieldReferencingContainer2": {
      "$ref": "#definitions/VulTestFieldReferencingContainer2"
    }
  },
  "definitions": {
    "VulTestFieldReferencing": {
      "type": "object",
      "properties": {
        "name": {
          "type": "string"
        }
      },
      "x-vul-typename": "VulTestFieldReferencing"
    },
    "VulTestFieldReferencingContainer1": {
      "type": "object",
      "properties": {
        "child": {
          "oneOf": [
            {
              "$ref": "#definitions/VulTestFieldReferencing"
            },
            {
              "$ref": "#definitions/VulFieldRef"
            }
          ]
        }
      },
      "x-vul-typename": "VulTestFieldReferencingContainer1"
    },
    "VulTestFieldReferencingContainer2": {
      "type": "object",
      "properties": {
        "child": {
          "oneOf": [
            {
              "$ref": "#definitions/VulTestFieldReferencing"
            },
            {
              "$ref": "#definitions/VulFieldRef"
            }
          ]
        }
      },
      "x-vul-typename": "VulTestFieldReferencingContainer2"
    },
    "VulFieldRef": {
      "type": "string",
      "description": "A string reference to another object in the graph."
    }
  }
}
```

And sample generated Typescript:

```ts
// A string reference to an existing object of the given type
// @ts-ignore
export type VulFieldRef<T> = string;

export interface VulTestFieldReferencing {
    name: string;
}

export interface VulTestFieldReferencingContainer1 {
    child: (VulTestFieldReferencing | VulFieldRef<VulTestFieldReferencing>);
}

export interface VulTestFieldReferencingContainer2 {
    child: (VulTestFieldReferencing | VulFieldRef<VulTestFieldReferencing>);
}
```

Note with TypeScript, we only export named types, so usage of `VULFLD_` macros are key.

### Macros and Type Registration

Types intended for metadata export should be registered using the `VULFLD_` macro set. These handle
boilerplate generation and ensure the type is included in a global registry for export.

This allows the metadata system to give a name to your CPP types and describe the inheritance
relationships between them.

If your types live in a module (such as a game project), use the appropriate export macros
(`MYGAMEPROJECT_API`) to expose them as the template system will need to access them from
the Vul plugin module.

### Inheritance Support

Base & derived types are supported using discriminator fields. For example, consider a project
that describes game events, such as:

```json
{
  "type": "DamageDone",
  "amount": 10
}
```

```json
{
  "type": "PlayerJoined",
  "name": "Steve"
}
```

These might be representations of an `FMyGameProjectEvent` type, which has `FMyGameProjectDamageDoneEvent`
and `FMyGameProjectPlayerJoinedEvent` derived types. Here, the `type` field indicates which derived type
the schema represents. During deserialization, this can be used to instantiate the correct subclass.

The type structure for this example might look like:

```c++
UENUM()
enum class EMyGameProjectEventType : uint8
{
    None,
    PlayerJoined,
    DamageDone,
}

// Note MYGAMEPROJECT_API is important here otherwise the Vul module won't link properly.
MYGAMEPROJECT_API DECLARE_ENUM_TO_STRING(EMyGameProjectEventType); // And corresponding DEFINE_ENUM_TO_STRING in the .cpp file 

USTRUCT()
struct MYGAMEPROJECT_API FMyGameProjectEvent
{
    GENERATED_BODY()
    
    VULFLD_TYPE(FMyGameProjectEvent, "MyGameProjectEvent")
    VULFLD_DISCRIMINATOR_FIELD(FMyGameProjectEvent, "type")
    
    FVulFieldSet VulFieldSet() const
    {
        FVulFieldSet Set;
        // A "virtual" field, in that is doesn't directly map to a property (and is serialize-only, no deserialize).
        Set.Add<EMyGameProjectEventType>([this]{ return GetEventType(); }, "type");
        return Set;
    }
    
    virtual EMyGameProjectEventType GetEventType() const { return EMyGameProjectEventType::None; }
}

USTRUCT()
struct MYGAMEPROJECT_API FMyGameProjectDamageDoneEvent : public FMyGameProjectEvent
{
    GENERATED_BODY()
    
    VULFLD_DERIVED_TYPE(FMyGameProjectDamageDoneEvent, "MyGameProjectDamageDoneEvent", FMyGameProjectEvent)
    VULFLD_DERIVED_DISCRIMINATOR(FMyGameProjectDamageDoneEvent, EMyGameProjectEventType::DamageDone)
    
    FVulFieldSet VulFieldSet() const
    {
        FVulFieldSet Set = Super::VulFieldSet();
        Set.Add(FVulField::Create(&Amount), "amount");
        return Set;
    }
    
    int Amount;
    
    virtual EMyGameProjectEventType GetEventType() const override { return EMyGameProjectEventType::DamageDone; } 
}

USTRUCT()
struct MYGAMEPROJECT_API FMyGameProjectPlayerJoinedEvent : public FMyGameProjectEvent
{
    GENERATED_BODY()
    
    VULFLD_DERIVED_TYPE(FMyGameProjectPlayerJoinedEvent, "MyGameProjectPlayerJoinedEvent", FMyGameProjectEvent)
    VULFLD_DERIVED_DISCRIMINATOR(FMyGameProjectPlayerJoinedEvent, EMyGameProjectEventType::PlayerJoined)
    
    FVulFieldSet VulFieldSet() const
    {
        FVulFieldSet Set = Super::VulFieldSet();
        Set.Add(FVulField::Create(&PlayerName), "name");
        return Set;
    }
    
    FString PlayerName;
    
    virtual EMyGameProjectEventType GetEventType() const override { return EMyGameProjectEventType::PlayerJoined; } 
}
```

### Referencing in Schemas

If reference tracking is enabled, exported schemas (both TypeScript and JSON) will include
support for shared instances via the `VulFieldRef` type. This ensures compatibility with
deserialization behaviors where objects are shared or cyclic.

### Limitations

* TypeScript export only includes known types registered via `VULFLD_` macros.
* JSON Schema export may be verbose due to nested objects and limited flattening.
* Highly dynamic types or runtime-polymorphic behaviors may not produce meaningful schemas.

## TODOs

* Use the new metadata system to automatically deserialize based on the discriminator.
* Integration with UE's reflection system to automatically serialize and deserialize UPROPERTY chains.
* Enum support with integer representation for more efficient serialized output.
* Deserialization support for extracted refs.
