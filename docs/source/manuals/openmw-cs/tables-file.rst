###########
File Tables
###########

Tables found in the File menu.


Verification Results
********************

This table shows reports created by the verify command found in the file menu.
Verify will go over the whole project and output errors / warnings when records
don't conform to the requirements of the engine. The offending records can be
accessed directly from the verification results table. The table will not update
on its own once an error / warning is fixed. Instead, use the *Refresh* option
found in the right click menu in this table. 

Type
    The type of record that is causing the error / warning.

ID
    ID value of the offending record.
    
Severity
    Whether the entry is an error or merely a warning.
    The game can still run even if not all errors are fixed.
    
Description
    Information on what exactly is causing the error / warning.


Error Log
*********

The Error Log table shows any errors that occured while loading the game files
into OpenMW-CS. These are the files that come in ``.omwgame``, ``.omwaddon``,
``.esm``, and ``.esp`` formats.
