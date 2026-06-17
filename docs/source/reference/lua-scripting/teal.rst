####
Teal
####

What is Teal?
=============

Teal is a typed dialect of Lua. `Teal's Github repository <https://github.com/teal-language/tl>`_.

Teal compiles into Lua, so you can use it in any Lua 5.1+ runtime. If you are familiar with TypeScript, Teal is to Lua what TypeScript is to JavaScript.
You can learn the syntax in the `Teal book <https://teal-language.org/book/>`_.

Teal's syntax is mostly the same as Lua, but with additional type declarations and annotations.
It will help you catch many mistakes before even running a script, and provide confidence about large code changes.

Using the type checker
======================

To compile your ``.tl`` files into ``.lua`` files, install `Cyan, Teal's build system <https://github.com/teal-language/cyan>`_.

Create a directory for your project, with a ``tlconfig.lua`` file inside.
All of your scripts (i. e. the ``scripts`` directory) should be within this directory.
``tlconfig.lua`` configures the Teal build system and compiler, see the `complete list here <https://teal-language.org/book/compiler_options.html>`_.

.. note::
  You can use ``cyan init`` to set up a directory for a Teal project automatically.

In addition to setting up a build process, you will need the `declaration files for the OpenMW API <https://gitlab.com/OpenMW/openmw/-/jobs/artifacts/master/raw/teal_declarations.zip?job=Teal>`_.
Unpack them into a directory of your choice, and add that path to the ``include_dir`` option in your ``tlconfig.lua``. Alternatively, you can add ``-I <my-dcelaration-directory-path>`` as an agument to ``Cyan`` commands.

After everything is ready, run ``cyan build`` in the same directory as ``tlconfig.lua``. It will find all the ``.tl`` files in the ``source_dir``, and put compiled ``.lua`` files at the same relative paths inside ``build_dir``.
Running ``cyan build`` will also perform a type check, notifying you of any mismatches or mistakes.

.. note::
  ``source_dir`` and ``build_dir`` can be the same directory. In fact, that is the recommended arrangement, so that it's convenient to include the original sources with your scripts.

IDE support
===========

Work on `Teal Language Server <https://github.com/teal-language/teal-language-server>`_ is still ongoing, so for now the only supported IDE is `Visual Studio Code <https://code.visualstudio.com/>`_.
It's available on Windows, Linux and Mac, so most likely you can run it too.
Teal's extension can be found here: `VSCode Marketplace <https://marketplace.visualstudio.com/items?itemName=pdesaulniers.vscode-teal>`_ (or simply search for "Teal" in the extension UI).

.. note::
  VSCode also has a web version, but the Teal extension isn't available there.
