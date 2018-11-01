This directory contains source for the documentation.  It is also the 
build target for doxygen output.

Directory Layout

. (Docs root)

	High level content and format files. (E.g. css, header, footer.)

./Conceptual
	
	Conceptual (non-api) documentation such as overviews, how-to's, etc.
	The main index page content is also in this directory.

./Extern

	API documentation that is located outside the source files.
	
	When the API documentation gets too big or complex for the header
	and source files, it goes in this directory.
	
./Images

	Images related to the documentation.
	
./html

    The target for the Doxygen build.  (Created during the build process.)
    
Miscellany

One of the requirements for the API documentation is that it
has the minimum possible impact on the declarations in the
header files.  So, in general, the header file declarations only
contain summary documentation.  The detail documentation
is placed as follows:

1.  If an element is defined in a cpp file, then place
    the detail documentation in the source file.
2.  If an element does not have an associated cpp file, then
    place the detail documentation at the end of the header file.
3.  If there is a lot of detail documentation cluttering up
    the end of a header file, then the content is moved to 
	a separate file in the Extern directory.
    
Building the Documentation

1.  Download and install the appropriate Doxygen version.  (See the first 
    line in the Doxyfile for the current version.)
2.  Run "doxygen" in the project root directory. (The location of the Doxyfile.)
    No arguments are required.

The generated html files will be located in the /Docs/html directory.

If you want to "version" the documentation, you can set the PROJECT_NUMBER
setting in the Doxyfile.  E.g. PROJECT_NUMBER = "(2014-04-23)".  The project
number will be added to the header of the documentation.  
E.g. "Recast Navigation (2014-04-23)" 


 



