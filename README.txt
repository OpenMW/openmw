Welcome to Mangle v0.1
----------------------

Written by:      Nicolay Korslund (korslund@gmail.com)
License:         zlib/png (see LICENSE.txt)
WWW:             http://asm-soft.com/mangle/
Documentation:   http://asm-soft.com/mangle/docs



Mangle is the project name for a small set of generic interfaces for
various game middleware libraries, such as sound, input, graphics, and
so on. You can imagine that it stands for "Minimal Abstraction Game
Layer", if you like. It will consist of several more or less
independent modules, one for each of these areas. These may be used
together to build an entire game engine, or they can be used
individually as separate libraries.

However, Mangle does NOT actually implement a game engine, or any new
fundamental functionality. More on that below.

Currently there's modules for sound and streams / archives (virtual
file systems.) More will come in the future (including input, 2D/3D
graphics, GUI, physics, and more.)


Main idea
---------

The idea behind Mangle is to provide a uniform, consistent interface
to other game libraries. The library does not provide ANY
functionality on its own. Instead it connects to a backend
implementation of your choice (or of your making.)

The Sound module, for example, currently has backends for OpenAL
(output only), FFmpeg (input only) and for Audiere. Hopefully we'll
add IrrKlang, FMod, DirectSound, Miles and more in the future It can
combine libraries to get more complete functionality (like using
OpenAL for output and FFmpeg to decode sound files), and it's also
easy to write your own backend if you're using a different (or
home-brewed) sound system.

Regardless of what backend you use, the front-end interfaces (found
eg. in sound/output.h) is identical, and as a library user you
shouldn't notice much difference at all if you swap one backend for
another at a later point. It should Just Work.

The interfaces themselves are also quite simple. Setting up a sound
stream from FFmpeg or other decoder into OpenAL can be quite hairy -
but with Mangle the hairy parts have already been written for you. You
just plug the parts together.

The goal in the long run is to support a wide variety of game-related
libraries, and as many backend libraries (free and commercial) as
possible, so that you the user will have to write as little code as
possible.



What is it good for
-------------------

The main point of Mangle, as we said above, is that it connects to any
library of your choice "behind the scenes" but provides the same,
super-simple interface front-end for all of them. There can benefit
you in many ways:

- If you want to use a new library that Mangle support. You don't have
  to scour the net for tutorials and usage examples, since much of the
  common usage code is already included in the implementation classes.

- If you don't want to pollute your code with library-specific code.
  The Mangle interfaces can help you keep your code clean, and its
  user interface is often simpler than the exteral library one.

- If you want to quickly connect different libraries together, it
  really helps if they have speak a common language. The Mangle
  interfaces are exactly that. Need to load Audiere sounds from a
  weird archive format only implemented for PhysFS, all channeled
  through the OGRE resource system? No problem!

- If you are creating a library that depends on a specific feature
  (such as sound), but you don't want to lock your users into any
  specific sound library. Mangle works as an abstraction that lets
  your users select their own implementation.

- If you want to support multiple backends, or make it possible to
  easily switch backends later. You can select backends at compile
  time or even at runtime. For example you might want to switch to to
  a commercial sound library at a later stage in development, or you
  may want to use a different input library on console platforms than
  on PC.

The Mangle implementations are extremely light-weight - often just one
or two cpp/h pairs per module. You can plug them directly into your
program, there's no separate library building step required.

Since the library aims to be very modularly put together, you can
also, in many cases, just copy-and-paste the parts you need and ignore
the rest. Or modify stuff without fearing that the whole 'system' will
come crashing down, because there is no big 'system' to speak of.


Past and future 
---------------

Mangle started out as (and still is) a spin-off from OpenMW, another
project I am personally working on ( http://openmw.sourceforge.net ).
OpenMW is an attempt to recreate the engine behind the commercial game
Morrowind, using only open source software.

The projects are still tightly interlinked, and they will continue to
be until OpenMW is finished. Most near-future work on Mangle will be
focused chiefly on OpenMW at the moment. However I will gladly
implement external contributions and suggestions that are not
OpenMW-related.


Conclusion
----------

As you might have guessed, Mangle is more a concept in development
than a finished library right now.

All feedback, ideas, concepts, questions and code are very
welcome. Send them to: korslund@gmail.com

I will put up a forum later as well if there's enough interest.
