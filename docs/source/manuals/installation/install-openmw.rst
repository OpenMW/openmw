==============
Install OpenMW
==============

The (easier) Binary Way
=======================

If you're not sure what any of the different methods mean, you should probably stick to this one.
Simply download the latest version for your operating system from
`github.com/OpenMW/openmw/releases <https://github.com/OpenMW/openmw/releases>`_
and run the install package once downloaded. It's now installed!

	.. note::
		There is no need to uninstall previous versions
		as OpenMW automatically installs into a separate directory for each new version.
		Your saves and configuration are compatible and accessible between versions.

The (bleeding edge) Source Way
==============================

Visit the `Development Environment Setup <https://wiki.openmw.org/index.php?title=Development_Environment_Setup>`_
section of the Wiki for detailed instructions on how to build the engine.

The Ubuntu Way
==============

A `Launchpad PPA <https://launchpad.net/~openmw/+archive/openmw>`_ is available.
Add it and install OpenMW::

	$ sudo add-apt-repository ppa:openmw/openmw
	$ sudo apt-get update
	$ sudo apt-get install openmw openmw-launcher

.. note::
	OpenMW-CS must be installed separately by typing::

		$ sudo apt-get install openmw-cs

The Arch Linux Way
==================

The binary package is available in the official [community] Repositories.
To install, simply run the following as root (or in sudo)::

	# pacman -S openmw

The Void Linux Way
==================

The binary package is available in the official Repository
To install simply run the following as root (or in sudo)::

	# xbps-install openmw

The Debian Way
==============

OpenMW is available from the unstable (sid) repository of Debian contrib
and can be easily installed if you are using testing or unstable.
However, it depends on several packages which are not in stable,
so it is not possible to install OpenMW in Wheezy without creating a FrankenDebian.
This is not recommended or supported.

