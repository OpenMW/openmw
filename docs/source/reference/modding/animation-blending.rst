Animation blending
##################

Animation blending introduces smooth animation transitions between essentially every animation in the game without affecting gameplay. Effective if ``smooth animation transitions`` setting is enabled in the launcher or the config files.

Animation developers can bundle ``.yaml``/``.json`` files together with their ``.kf`` files to specify the blending style of their animations. Those settings will only affect the corresponding animation files.

The default OpenMW animation blending config file (the global config) affects actors only, that restriction doesn't apply to other animation blending config files; they can affect animated objects too.
Do not override the global config file in your mod, instead create a ``your_modded_animation_file_name.yaml`` file and put it in the same folder as your ``.kf`` file.

For example, if your mod includes a ``newAnimations.kf`` file, you can put a ``newAnimations.yaml`` file beside it and fill it with your blending rules.
Animation config files shipped in this fashion will only affect your modded animations and will not meddle with other animations in the game. 

Local (per-kf-file) animation rules will only affect transitions between animations provided in that file and transitions to those animations; they will not affect transitions from the file animation to some other animation.

Editing animation config files
------------------------------

In examples below ``.yaml`` config file will be used. You can provide ``.json`` files instead of ``.yaml`` if you adhere to the same overall structures and field names.

Animation blending config file is a list of blending rules that look like this:

```
blending_rules:
  - from: "*"
    to: "*"
    easing: "sineOut"
    duration: 0.25
  - from: "*"
    to: "idlesneak*"
    easing: "springOutWeak"
    duration: 0.4
```

See ``OpenMW\files\data\animations\animation-config.yaml`` for an example of such a file.

Every blending rule should include a set of following fields:

``from`` and ``to`` are rules that will attempt to match animation names; they usually look like ``animationGroupName:keyName`` where ``keyName`` is essentially the name of a specific action within the animation group. 
Examples: ``"weapononehanded: chop start"``, ``"idle1h"``, ``"jump: start"`` e.t.c.

.. note::

    ``keyName`` is not always present and if omitted - the rule will match any ``keyName``.
    The different animation names the game uses can be inspected by opening ``.kf`` animation files in Blender.


Both ``animationGroupName`` and ``keyName`` support wildcard characters either at the beginning, the end of the name, or instead of the name:

- ``"*"`` will match any name.
- ``"*idle:sta*"`` will match an animationGroupName ending with ``idle`` and a keyName starting with ``sta``.
- ``"weapon*handed: chop*attack"`` will not work since we don't support wildcards in the middle.

``easing`` is an animation blending function, i.e., a style of transition between animations, look below to see the list of possible easings.

``duration`` is the transition duration in seconds, 0.2-0.4 are usually reasonable transition times, but this highly depends on your use case.

.. note::

    The bottom-most rule takes precedence in the animation config files.


List of possible easings
------------------------

- "linear"
- "sineOut"
- "sineIn"
- "sineInOut"
- "cubicOut"
- "cubicIn"
- "cubicInOut"
- "quartOut"
- "quartIn"
- "quartInOut"
- "springOutGeneric"
- "springOutWeak"
- "springOutMed"
- "springOutStrong"
- "springOutTooMuch"

``"sineOut"`` easing is usually a safe bet. In general ``"...Out"`` easing functions will yield a transition that is fast at the beginning of the transition but slows down towards the end, that style of transitions usually looks good on organic animations e.g. humanoids and creatures.

``"...In"`` transitions begin slow but end fast, ``"...InOut"`` begin fast, slowdown in the middle, end fast.

Its hard to give an example of use cases for the latter 2 types of easing functions, they are there for developers to experiment.

The possible easings are largely ported from `here <https://easings.net/>`__ and have similar names. Except for the ``springOut`` family, those are similar to ``elasticOut`` from `easings.net <https://easings.net/>`__, with ``springOutWeak`` being almost identical to ``elasticOut``.

Don't be afraid to experiment with different timing and easing functions!