# Custom depth & Stencils

Custom depth and stencil values allow meshes to pass values to either translucent
or post process materials. These can be used to render objects even when they are
obstructed by objects in the world.

![img/obstructed-character.png](img/obstructed-character.png)

## Obstructed Visibility Effect Using Stencils

This approach uses a single post-process material, which saves having to customize materials
on any objects in the game world (including any Overlay materials).

This uses the stencil bits as a mask, using the write mask to allow multiple objects
to combine values.

![img/stencil-write-masks.png](img/stencil-write-masks.png)

The general approach is:
* All objects you want your render to appear over the top of as Stencil Value=255, Write Mask=Eight bit
* Any objects that should appear over this will have values lower than 128
  * Any write mask is fine for these as it's just the foreground object that needs to merge the value.
* Create a material that checks the bit values, if we subtract 128 and still have values, we know this
  is a pixel that has our object we want to indicate/highlight.
  ![img/stencil-postprocess-material.png](img/stencil-postprocess-material.png)Tips to get a clean render of the effect:
  * PP Material's `Blendable Location` property = `Scene Color After DOF`. This gets rid of a jitter effect.
  * Ensure that motion blur is off; this does not work well when a character moves around.
    * Can set this in a PP Volume.
  * There is also an "Enable Stencil Test" in the material which might be useful for performance, or at
    at least cleaning up the material code to save having to do the check in there.
* Create a Post-process volume and add the PP material to it.
* You can select different effects for different objects by configuring the first 7 bits then checking for
  them in the PP material.

