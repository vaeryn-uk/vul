# Blender Notes

A place for quick tips & resources I've found useful when modeling in Blender.

## IK Setups

https://www.youtube.com/watch?v=Pt3-mHBCoQk

Inverse kinematics are really effective for animating legs. It's worth spending the time
getting these setup to more efficiently produce better animations.

![](img/ik-setup.png)

* Control - bone that is moved to move the entire leg (disable `Deform` in Bone properties).
* Pole target - bone that the IK chain is rotated toward (disable `Deform` in Bone properties).
  * Once setting this in the IK modifier, change the `Pole Angle` to face in the right 
    direction, usually `+90`/`-90` degrees.
* Foot - parented to the Control bone (not connected)
  * This bone is also constrained to the location of the lower leg bone (accessed in Pose Mode > Bone Constraints)
  * Set the `Head/Tail` = `1.0` to ensure the foot is connected to the end of the lower leg.
  * This prevents the foot following the control bone away from the leg, allowing more freedom with
    the control bone in animation.

![](img/ik-setup-foot-relations.png)
![](img/ik-setup-foot-constraints.png)

* Lower leg - bone with the IK constraint (accessed in Pose Mode > Bone Constraints)
  * Target: control bone
  * Pole: pole target
  * Chain length: this should be set to the number of bones (including self) that should be included
    in the IK chain. In this example 3: Lower leg, Middle of Leg, Top of Leg.

![](img/ik-setup-lower-leg-constraint.png)

## Animating quadrupeds

![](img/bear-walk-cycle.gif)

https://www.youtube.com/watch?v=CNd7nqUgnj0 explains how to achieve a simple walk cycle
for creature legs.

* Ensure a mirrored bone structure on left and right (`BoneName_L`, then Right Click > Symmetrize).
* Create an animation, define a frame range, e.g. 100. Keyframe all bones at 1.
* Make sure the body is at a height that allows some leg bending (i.e. legs should not be fully stretched).
* At frame 1, left legs should be at forward extreme and right leg at back extreme, on the ground.
* With both front IK bones selected, Copy this to end frame.
* Copy this to 50% mirrored (`Ctrl`+`Shift`+`V`)
* At 75% frame, take the left leg off the floor & rotate paw/foot.
* Copy this to 25% mirrored (`Ctrl`+`Shift`+`V`)
* Once the front legs are satisfactory, repeat this process for the back legs.
  * Front and back legs should now move in sync.
  * Note in the above animation the bear has shoulder bones which are animated up & down slightly
    in sync with legs at the same animation points.
* Apply a cycles modifier for all bones. This allows moving keyframes outside the animation bounds
  and have the animation cycle.
  * Select all bones
  * Graph Editor
  * Select all keyframes (`A`)
  * `Shift`-`E`, `Make Cyclic`
* Select the two back leg IK bones and shift them forward ~15% of the animation.
  * If the curve modifiers are applied correctly, the animation will now have the back legs in a cycle
    where the back leg leaves the floor slightly ahead of the front leg.

This can likely be applied to bipeds too.