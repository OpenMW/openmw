Building yojimbo
================

## Building on Windows

Download [premake 5](https://premake.github.io/download.html) and copy the **premake5** executable somewhere in your path.

You need Visual Studio to build the source code. If you don't have Visual Studio 2019 you can [download the community edition for free](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&rel=16).

Once you have Visual Studio installed, go to the command line under the yojimbo directory and type:

    premake5 vs2019

Open the generated yojimbo.sln file.

You can now build the library and run individual test programs as you would for any other Visual Studio solution.

## Building on MacOS and Linux

First, download and install [premake 5](https://premake.github.io/download.html).

Next, install libsodium.

Linux:

    sudo apt install libsodium-dev

Mac:

    brew install libsodium

Now go to the command line under the yojimbo directory and enter:

    premake5 gmake

This creates makefiles which you can use to build the source via:

    make -j

Then run the built executables:

    ./bin/test           // run unit tests
    ./bin/server         // run a server on localhost on UDP port 40000
    ./bin/client         // run a client that connects to the local server

cheers

 - Glenn
