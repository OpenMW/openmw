==============
Install OpenMW
==============

Direct Download
===============

If you're not sure what any of the different methods mean, you should probably stick to this one.
Simply download the latest version for your operating system from
`github.com/OpenMW/openmw/releases <https://github.com/OpenMW/openmw/releases>`_
and run the install package once downloaded. It's now installed!

.. note::
	There is no need to uninstall previous versions
	as OpenMW automatically installs into a separate directory for each new version.
	Your saves and configuration are compatible and accessible between versions.

From Source
===========

Visit the `Development Environment Setup <https://wiki.openmw.org/index.php?title=Development_Environment_Setup>`_
section of the Wiki for detailed instructions on how to build the engine.

Ubuntu
======

A `Launchpad PPA <https://launchpad.net/~openmw/+archive/openmw>`_ is available.
Add it and install OpenMW.

.. code-block:: console

	$ sudo add-apt-repository ppa:openmw/openmw
	$ sudo apt update
	$ sudo apt install openmw

Arch Linux
==========

The binary package is available in the official [community] Repositories.
To install, simply run the following as root (or in sudo).

.. code-block:: console

	$ pacman -S openmw

Void Linux
==========

The binary package is available in the official Repository
To install simply run the following as root (or in sudo).

.. code-block:: console

	$ xbps-install openmw

Debian
======

OpenMW is available from the unstable (sid) repository of Debian contrib
and can be easily installed if you are using testing or unstable.
However, it depends on several packages which are not in stable,
so it is not possible to install OpenMW in Wheezy without creating a FrankenDebian.
This is not recommended or supported.

Fedora
======

OpenMW is available in the official repository of Fedora for versions 41 and up.
To install simply run the following as root (or in sudo), depending on what packages
you want.

``openmw`` includes the launcher, install wizard, iniimporter and the engine itself.

``openmw-cs`` includes the construction set.

``openmw-tools`` includes ``bsatool``, ``esmtool`` and ``niftest``.

.. code-block:: console

	$ dnf install openmw
	$ dnf install openmw-cs
	$ dnf install openmw-tools

Flatpak
=======

OpenMW is available as a flatpak. With flatpak installed, run the command below. It should show up on your desktop.

.. code-block:: console

	$ flatpak install openmw
