Welcome to GOOI v0.1
--------------------

Written by:      Nicolay Korslund (korslund@gmail.com)
License:         zlib/png (see LICENSE.txt)
WWW:             http://asm-soft.com/gooi/
Documentation:   http://asm-soft.com/gooi/docs



GOOI stands for Game-Oriented Object Interfaces. It is meant to become
a small set of generic interfaces for various game middleware
libraries, such as sound, input, graphics, and so on. It consists of
several independent modules, one for each of these areas. These may be
used together to build an entire game engine, or they can be used
individually as separate libraries.

However, GOOI does NOT actually implement a game engine, or any new
fundamental functionality. More on that below.

Currently there is only the Sound module, but more will come in the
future (including input, 2D/3D graphics, GUI, physics, file
system/archive access, and more.)


Main idea
---------

The idea behind to provide a uniform, consistent interface to other
game libraries. The library does not provide ANY functionality on its
own. Instead it connects to a backend implementation of your choice.

The Sound module, for example, currently has backends for OpenAL
(output only), FFmpeg (input only) and for Audiere. Hopefully we'll
soon add IrrKlang, FMod, DirectSound and Miles to that. It can combine
libraries to get more complete functionality (like using OpenAL for
output and FFmpeg to decode sound files), and it's also easy to write
your own backend if you're using a different (or home-brewed) sound
system.

Regardless of what backend you use, the front-end interface (found in
sound/sound.h) is identical, and as a library user you shouldn't
notice much difference at all if you swap one backend for another at a
later point.

The goal in the long run is to support a wide variety of game-related
libraries, and as many backend libraries (free and commercial) as
possible, so that you the user will have to write as little code as
possible.



What is it good for
-------------------

The main point of GOOI, as we said above, is that it connects to any
library of your choice "behind the scenes" but provides the same,
super-simple interface front-end for all of them. There can benefit
you in many ways:

- If you want to use a new library that GOOI support. You don't
  have to scour the net for tutorials and usage examples, since much
  of the common usage code is already included in the implementation
  classes.

- If you don't want to pollute your code with library-specific code.
  The GOOI interfaces can help you keep your code clean, and its user
  interface is often simpler than the exteral library one.

- If you are creating a library that depends on a specific feature
  (such as sound), but you don't want to lock your users into any
  specific sound library. GOOI works as an abstraction that lets your
  users select their own implementation. My own Monster scripting
  language ( http://monsterscript.net ) will use this tactic, to
  provide native-but-generic sound, input and GUI support, among other
  features.

- If you want to support multiple backends, or make it possible to
  easily switch backends later. You can select backends at compile
  time or even at runtime. Maybe you decide to switch to to a
  commercial library at a late stage in development, or you discover
  that your favorite backend doesn't work on all the platforms you
  want to reach.

The GOOI implementations are extremely light-weight - often just one
or two cpp/h pairs. You plug them directly into your program, there's
no separate build step required.

Since the library aims to be very modularly put together, you can
also, in many cases, just copy-and-paste the parts you need and ignore
the rest. Or modify stuff without fearing that the whole 'system' will
come crashing down, because there is no big 'system' to speak of.


Past and future 
---------------

GOOI started out as a spin-off from OpenMW, another project of mine
( http://openmw.sourceforge.net ). OpenMW is an attempt to recreate
the engine behind the commercial game Morrowind, using only open
source software.

The projects are still tightly interlinked, and the will continue to
be until OpenMW is finished. That means that all near-future work on
GOOI for my part will be more or less guided by what OpenMW needs. But
I'll gladly accept external contributions that are not OpenMW-related.


Conclusion
----------

As you might have guessed, GOOI is more a concept in development than
a finished library right now.

All feedback, ideas, concepts, questions and code are very
welcome. Send them to: korslund@gmail.com

I will put up a forum later as well if there's enough interest.
