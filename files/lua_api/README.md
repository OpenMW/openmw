Files in this directory describe OpenMW API for [Lua Development Tools](https://www.eclipse.org/ldt/) (LDT).

`*.doclua` files are taken (with some modifications) from LDT distribution and are distributed under `MIT` license.
Openmw-specific docs (`openmw/*.lua`) are under `GPLv3` license.

To get a Lua IDE with integrated OpenMW documentation and code autocompletion do the following:

1. Install and run [LDT](https://www.eclipse.org/ldt/#installation).
2. Press `File` / `New` / `Lua Project` in menu.
3. Specify project name (for example the title of your omwaddon)
4. Set `Targeted Execution Environment` to `No Execution Environment`, and `Target Grammar` to `lua-5.1`.
5. Press `Next`, choose the `Libraries` tab, and click `Add External Source Folder`. Then specify there the path to this directory.
6. Press `Finish`.
