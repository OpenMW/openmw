Models Settings
###############

load unsupported nif files
--------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Allow the engine to load arbitrary NIF files as long as they appear to be valid.

OpenMW has limited and **experimental** support for NIF files
that Morrowind itself cannot load, which normally goes unused.

If enabled, this setting allows the NIF loader to make use of that functionality.

.. warning::
	You must keep in mind that since the mentioned support is experimental,
	loading unsupported NIF files may fail, and the degree of this failure may vary.
	
	In milder cases, OpenMW will reject the file anyway because
	it lacks a definition for a certain record type that the file may use.
	
	In more severe cases OpenMW's incomplete understanding of a record type
	can lead to memory corruption, freezes or even crashes.
	
	**Do not enable** this if you're not so sure that you know what you're doing.

To help debug possible issues OpenMW will log its progress in loading
every file that uses an unsupported NIF version.
