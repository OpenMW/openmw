@echo off

rem See COMPILE-win32.txt for instructions.

rem This file assumes it can find Ogre in ..\ogre and that ffmpeg
rem has been downloaded and compiled in ..\ffmpeg

echo Compiling C++ files
g++ -c sound\cpp_avcodec.cpp -I..\ffmpeg
g++ -c ogre\cpp_ogre.cpp -I..\ogre\include

copy ..\ogre\bin\debug\ogremain_d.dll .
copy ..\ogre\bin\debug\ois_d.dll .
copy ..\ogre\bin\debug\cg.dll .
copy ..\ogre\bin\debug\RenderSystem*.dll .
copy ..\ogre\bin\debug\Plugin*.dll .
copy ..\ffmpeg\libavcodec\avcodec-51.dll .
copy ..\ffmpeg\libavformat\avformat-52.dll .
copy ..\ffmpeg\libavdevice\avdevice-52.dll .
copy ..\ffmpeg\libavutil\avutil-49.dll .
copy \windows\system32\d3dx9_30.dll d3dx9d_30.dll

echo Compiling main program (openmw.exe)
gdc -Wall -g openmw.d bsa\*.d core\*.d esm\*.d input\*.d nif\*.d ogre\*.d scene\*.d sound\*.d util\*.d cpp_ogre.o cpp_avcodec.o monster\util\*.d avcodec-51.dll avformat-52.dll avdevice-52.dll avutil-49.dll openal32.dll ogremain_d.dll OIS_d.dll -lstdc++ -o openmw.exe
