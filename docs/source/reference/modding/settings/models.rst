Models Settings
###############

load unsupported nif files
--------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

At the moment OpenMW's NIF loader is only tailored to load models
that can also load in Morrowind.

However, you can enable its limited and experimental support for updates in
the definitions of record types from later NIF revisions by toggling on
this setting.

You must keep in mind that loading unsupported NIF files may fail,
and the degree of this failure may vary. In milder cases, OpenMW will reject
the file anyway because it lacks a definition for a certain record type
that the file may use. In more severe cases OpenMW's
incomplete understanding of a record type can lead to memory corruption,
crashes or even freezes. Don't enable this if you're not sure that
you know what you're doing.

To help debug possible issues OpenMW will log its progress in loading
every file that uses an unsupported NIF version.
