@echo off
rem See INSTALL.txt for instructions.

rem This batch file assumes you have run nodsss_monster.bat in the
rem Monster directory. It assumes Monster is located in ..\monster,
rem Ogre in ..\ogre and Audiere in ..\audiere

echo Compiling C++ files
g++ -c sound\cpp_audiere.cpp -I..\audiere\include
g++ -c ogre\cpp_ogre.cpp -I..\ogre\include

copy ..\ogre\bin\debug\ogremain_d.dll .
copy ..\ogre\bin\debug\ois_d.dll .
copy ..\ogre\bin\debug\cg.dll .
copy ..\audiere\bin\audiere.dll .
copy \windows\system32\d3dx9_30.dll d3dx9d_30.dll

echo Compiling main program (openmw.exe)
gdc openmw.d bsa\*.d core\*.d esm\*.d input\*.d nif\*.d ogre\*.d scene\*.d sound\*.d util\*.d cpp_audiere.o cpp_ogre.o monster\util\*.d ogremain_d.dll ..\audiere\lib\audiere.lib OIS_d.dll -lstdc++ -o openmw.exe
