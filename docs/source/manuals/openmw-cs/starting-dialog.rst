OpenMW CS Starting Dialog
#########################

In this chapter we will cover starting up OpenMW CS and the starting interface.
Start the CS the way intended for your operating system and you will be
presented with a window and three main buttons and a small button with a
wrench-icon. The wrench will open the configuration dialog which we will cover
later. The three main buttons are the following:

Create A New Game
   Choose this option if you want to create an original game that does not
   depend on any other content files. The distinction between game and addon in
   the original Morrowind engine was somewhat blurry, but OpenMW is very strict
   about it: regardless of how large your addon is, if it depends on another
   content file it is not an original game.

Create A New Addon
   Choose this option if you want to create an extension to an existing game.
   An addon can depend on other addons as well optionally, but it *must* depend
   on a game.

Edit A Content File
   Choose this option is you wish to edit an existing content file, regardless
   of whether it is a game or an addon.

Whether you create a game or an addon, a data file and a project file will be
generated for you in you user directory.

You will have to choose a name for your content file and if you chose to create
an addon you will also have to chose a number of dependencies. You have to
choose exactly one game and you can choose an arbitrary amount of addon
dependencies.  For the sake of simplicity and maintainability choose only the
addons you actually want to depend on. Also keep in mind that your dependencies
might have dependencies of their own, you have to depend on those as well. If
one of your dependencies needs something it will be indicated by a warning sign
and automatically include its dependencies when you choose it.

If you want to edit an existing content file you will be presented with a
similar dialog, except you don't get to choose a file name (because you are
editing files that already exist).
