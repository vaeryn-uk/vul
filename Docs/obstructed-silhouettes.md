﻿# Obstructed Silhouettes

Custom depth and stencil values allow meshes to pass values to either translucent
or post process materials. These can be used to render objects even when they are
obstructed by objects in the world.

![img/character-silhouette.png](img/character-silhouette.png)

## Using a post-process material

This approach uses a single post-process material, which saves having to customize materials
on any objects in the game world (including any Overlay materials).

_Warning: In practice, I could not get this approach to work nicely. The un-solved problem is
getting two objects' stencil values to combine only when A overlaps B; not when B overlaps A.
The result was that even when a character walked in front of a wall, the silhouette appeared.
I experimented with different stencil values & masks, but could not get the desired behaviour,
and the Visualize Stencil Buffer setting in the viewport seemed to behave inconsistently when
switching back and forth between two settings. Not sure if this is an engine bug or just a 
misunderstanding of the stencil feature. For my use-case, I switched to the overlay material
solution described below._

This uses the stencil bits as a mask, using the write mask to allow multiple objects
to combine values.

![img/stencil-write-masks.png](img/stencil-write-masks.png)

The general approach is:
* All objects you want your render to appear over the top of as Stencil Value=255, Write Mask=Eight bit
* Any objects that should appear over this will have values lower than 128
  * Any write mask is fine for these as it's just the foreground object that needs to merge the value.
* Create a material that checks the bit values, if we subtract 128 and still have values, we know this
  is a pixel that has our object we want to indicate/highlight.
  ![img/silhouette-pp-material.png](img/silhouette-pp-material.png)
  * Tips to get a clean render of the effect:
    * PP Material's `Blendable Location` property = `Scene Color After DOF`. This gets rid of a jitter effect.
    * Ensure that motion blur is off; this does not work well when a character moves around.
      * Can set this in a PP Volume.
    * There is also an "Enable Stencil Test" in the material which might be useful for performance, or at
      at least cleaning up the material code to save having to do the check in there.
* Create a Post-process volume and add the PP material to it.
* You can select different effects for different objects by configuring the first 7 bits then checking for
  them in the PP material.

## Using a translucent material

This alternative solution involves modifying the material(s) applied to the object themselves. This
can be added as a single material as an Overlay material on the relevant meshes you want to silhouette.
Note that overlay materials trigger an addition drawcall per object they're applied to.

The upside to this approach vs PP is that the material is rendered in 3D space, resulting in visuals
that can provide more detail of the obstructed mesh, as well as easily applying opacity to blend with
the obstructing object.

The following material is designed to be applied as an overlay material. It works by querying if the
stencil value is >=1. On our obstructing objects (e.g. a wall), we set a stencil value of 2. For objects
with a silhouette, we set a stencil value of 1. If the silhouette object is in front of an obstructing
object, the stencil value is 1 and we render opacity as 0 (no silhouette), if the obstructing object is 
in front, the stencil value is >1 and we render a silhouette.

![img/silhouette-overlay-material.png](img/silhouette-overlay-material.png)

Note that because this material is only drawn on the mesh, we can always output a single color as its
the opacity that drives which silhouette pixels are actually drawn. As an overlay material, a `0.0` opacity
means that the object's standard, opaque materials are rendered normally.

Tips:
* Disable "Custom Depth with TemporalAA Jitter" to avoids jittering pixels around the silhouettes.
* The material's Translucency Pass works well when set to "After Motion Blur"; you will need to not
  have motion blur in PP for this to look okay.

## Querying neighbouring depths

Materials can query their neighbouring pixels' custom depth & stencil channels. This can be useful to apply
outlining to silhouettes. For example, this is from a PP material that checks nearby pixels' stencil values:

![img/stencil-pixel-query.png](img/stencil-pixel-query.png)

