@echo off

rem See COMPILE-win32.txt for instructions.

echo Compiling C++ files
g++ -c sound\cpp_avcodec.cpp -I.\includes\ffmpeg\
g++ -c ogre\cpp_ogre.cpp -I.\includes\ogre\
g++ -c bullet\cpp_bullet.cpp -I.\includes\bullet\

echo Compiling main program (openmw.exe)
gdc -Wall -g openmw.d bsa\*.d core\*.d esm\*.d input\*.d nif\*.d ogre\*.d scene\*.d sound\*.d util\*.d bullet\*.d cpp_ogre.o cpp_avcodec.o cpp_bullet.o libbulletdynamics.a libbulletcollision.a libbulletmath.a mscripts\object.d monster\compiler\*.d monster\vm\*.d monster\util\*.d avcodec-51.dll avformat-52.dll avdevice-52.dll avutil-49.dll openal32.dll ogremain_d.dll OIS_d.dll -lstdc++ -o openmw.exe
