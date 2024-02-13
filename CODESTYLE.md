OpenMW Codestyle Guide
=======================
This file consists of compiled wiki pages documenting the OpenMW code style and rules; read more on the policies and standards [here](https://wiki.openmw.org/index.php?title=Policies_and_Standards).

For information regarding contribution guidelines, please check [here](https://gitlab.com/OpenMW/openmw/-/blob/master/CONTRIBUTING.md?ref_type=heads).

For bug reporting guidelines, please check [here](https://wiki.openmw.org/index.php?title=Bug_Reporting_Guidelines#Explanation_of_issue_states).

For bug tracker referencing guidelines, please check [here](http://www.redmine.org/projects/redmine/wiki/RedmineSettings#Referencing-issues-in-commit-messages).


**Table of Contents:**
* [Naming Conventions](#naming-conventions)
    * [Files](#files)
    * [Names](#names)
    * [Acronyms](#acronyms)
    * [Abbreviations](#abbreviations)
* [Code Formatting](#code-formatting)
    * [Layout & Indention](#layout-&-indention) 
    * [General Code Formatting](#general-code-formatting)
    * [Includes](#includes)
    * [Doxygen Comments](#doxygen-comments)
* [Architecture](#architecture)
    * [Structure](#structure)
    * [World Model](#world-model)
    * [Rendering](#rendering)
    * [GUI](#gui)
    * [ESM/ESP Formats](#esm/esp-formats)
* [Depency Policy](#depency-policy)
    * [Apps](#apps)
    * [Components](#components)
    * [Extern](#extern)
* [Unit Tests](#unit-tests)
* [General Advice](#general-advice)

Naming Conventions
=================================
In the beginning of OpenMW there were not any naming conventions, which resulted in substantial inconsistencies. This section tries to codify the conventions that are most commonly used in the code base. Please stick to them, even if the code in the neighbourhood of what you are working on is not.

Files
---------------------------------
No CamelCase and no underscores, please.

* **header files** | somefile.hpp
* **source files** | somefile.cpp
 
Names
---------------------------------
* **Namespaces**  | ThisIsANamespace
    * Namespaces in apps/openmw will have a MW-prefix (MWThisIsANamespace). Obviously you should try to keep these names short, but don't use hard to read abbreviations.
    * Fully qualified names preferred (e.g. std::cout).
    * Using directives (e.g. "using namespace std") are not welcome. Using directives in headers will result in the code being rejected.
    * Using declarations (e.g. "using std::cout") are tolerated, but should be used only in the innermost possible scope (i.e. usually function scope). Fully qualified names are still preferred.
    * Do not declare any names in the global namespace (the only exception to this rule is the file which contains the main() function of an executable).
* **Classes** | ThisIsAClass
* **Functions** | thisIsAFunction
* **Local Variables & Parameters** | thisIsAVariable
* **Member Variables** | Non-Static: mThisIsAVariable, Static: sThisIsAVariable
    * Note that formally the name of the variable would still be thisIsAVariable. The m and the s are scope sigils. Some other languages (e.g. Ruby) provide these as part of the syntax. C doesn't, so we have to encode them in the name. And no, this is not Hungarian notation.
* **Global Variables** | Shame on you!
* **Enums** | TypePrefix_SomeName, TypePrefix_SomeOtherName, TypePrefix_AndAnotherOne
    * The TypePrefix should be chosen appropriately for the purpose of the enum. Don't use all uppercase names. These would make the enums look like preprocessor constants.
* **Preprocessor Constants** | THISISACONSTANT
    * Since preprocessor constants are the only part of the language that does not respect namespaces, these must be clearly distinguished from everything else.
    * Don't use these for anything but include guards and conditional compiling.

Acronyms
---------------------------------
Acronyms in names should follow the guidelines on this page too, even if that is not how you would normally write them, e.g.: a class representing a NPC would be called Npc. Otherwise it would look like a preprocessor macro.

Abbreviations
---------------------------------
Please avoid cryptic abbreviations. iter and i are okay. Well known acronyms from the RPG terminology are okay too (but see above for advice on capitalisation). Other abbreviations should be avoided. Don't be afraid of long names!

Code Formatting
=================================
In the beginning of OpenMW we did not have any code formatting conventions, which resulted in substantial inconsistencies. This page tries to codify the conventions that are most commonly used in the code base. Please stick to them, even if the code in the neighbourhood of what you are working on is not.

Layout & Indention
---------------------------------
This is an example of the layout & indention style we encourage.
```cpp
// somefile.hpp
namespace SomeSpace
{
    class SomeClass
    {
        private:
            int mSomeVar;
  
        public:
            float someFunction(float p1, float p2) const;

            int getSomeVar() const { return mSomeVar; }  // Allowed only for one-liner getters and setters.
        void setSomeVar(int v) { mSomeVar = v; }     // All other functions should be implemented in somefile.cpp.
    };
}

// somefile.cpp
namespace SomeSpace
{
    float SomeClass::someFunction(float p1, float p2) const
    {
        return p1 + p2;
    }
}
```
Includes
---------------------------------
An implementation file (cpp) should always start by including its own header.

Further includes should be grouped. Groups must be separated by an empty line.

* **Standard C and C++ Headers**
```cpp
#include <string>
```

* **External Libraries Headers**

If more than one external library is used, each library's headers should be put into a separate group. 
```cpp
#include <osg/Vec3f>
```

* **Component Headers**
```cpp
#include <components/compiler/context.hpp>
```

* **Apps Headers**'
```cpp
#include <mwworld/world.hpp>
```

* **Local Headers**
```cpp
#include "world.hpp"
``` 

Doxygen Comments
---------------------------------
Class definitions and function declarations should have doxygen comments. We aren't very strict about this though (especially for trivial functions).

A class should be documented like this:

```cpp
/// \brief short description
///
/// Longer description.
class SomeClass
``` 

The longer description can be skipped, if there is nothing more to say.

A function should be documented like this:

```cpp
/// Description.
void someFunction();
```
Here is a link to the [Doxygen Documentation](http://www.stack.nl/~dimitri/doxygen/commands.html). Please make plenty of use of the listed commands, especially the following:
* \a
* \attention
* \brief
* \note
* \p
* \param
* \return
* \todo

Architecture
=================================
Below includes the style relevant aspects of the OpenMW architecture, extra information will be linked at the end of each subsection if you would like to read further.

Structure
---------------------------------
The OpenMW project is made up of three distinct subsystems (apps, components, & extern), please click the links below to learn more about them, as well as the important dependency policies surrounding them.
* [Apps](#apps)
* [Components](#components)
* [Extern](#extern)

World Model
---------------------------------
* **Terminology** | The current state of OpenMW (code and comments) is an unhealthy mix between two terminologies:
    * The original terminology (MW), which is rather unintuitive and partially overlaps with the C++ language terminology, which creates additional ambiguity.
    * The new terminology (NEW), which is reasonable, but incompatible with MM and overlaps even stronger with the C++ language terminology, which creates even more additional ambiguity.
    * To learn more about other aspects of the world model that may not be entirely relevant to code style, please read [here](https://wiki.openmw.org/index.php?title=Architecture). 

Rendering
---------------------------------
Our rendering code is based on [OpenSceneGraph](https://github.com/openscenegraph/openscenegraph), which is based on OpenGL. Because OpenSceneGraph is a very powerful framework, it is best to decide on some common usage practices, in order to avoid any problems.

* **Threading Considerations**
    * The default threading model in OpenSceneGraph, DrawThreadPerContext, requires some care when dealing with modifications to rendering data (Drawables and StateSets). Any such data that is currently in (or has been in) the scene graph (referred to as 'live' data) must be assumed as unsafe to modify because it could be in use by the drawing thread.
    * What is *not* allowed:
        *  Modify a live StateSet (e.g. by adding/removing StateAttributes or modifying a contained StateAttribute).
        *  Modify a live Drawable in a way that affects its drawing implementation.
    * What *is* allowed:
        *  Remove a Drawable from, or add it to the scene graph (the scene graph itself is not part of rendering data).
        *  (un)assign a StateSet from a node - the StateSets used for a rendering traversal are stored separately, so this is fine.
        *  Make harmless modifications that don't affect the renderer, like adding callbacks, changing names, etc. Still be very careful, though.
    * When making actual modifications, one of the following techniques should be followed:
        * Clone the original object and modify that copy. This is perfect for infrequent modifications when we don't care about the performance overhead of object cloning/deletion. [Example](https://github.com/OpenMW/openmw/blob/ada85aa1ee24e32435083cf644faece988ba9a27/apps/openmw/mwrender/characterpreview.cpp#L94).
        * Use a double buffering technique to manage access. We have a [StateSetUpdater](https://github.com/OpenMW/openmw/blob/master/components/sceneutil/statesetupdater.hpp) class that implements this technique for StateSets. [Example](https://github.com/OpenMW/openmw/blob/ada85aa1ee24e32435083cf644faece988ba9a27/apps/openmw/mwrender/sky.cpp#L109).
        * Rather than changing the object itself, inject the change on the fly where it's needed, e.g. into the CullVisitor by using a CullCallback. [Example](https://github.com/OpenMW/openmw/blob/ada85aa1ee24e32435083cf644faece988ba9a27/apps/openmw/mwrender/sky.cpp#L742).
        * Set the object's dataVariance to DYNAMIC, so that the draw traversal knows that it has to synchronize that object. Never do this in performance critical areas, or at all, really. Just one DYNAMIC object will make the threading useless and probably halve your frame rate.
    
    
* **Relevant Important Components**
    *  Scene Manager | Where game object's rendering nodes are loaded, prepared, optimized, and instanced, then stored in the cache for future use.
        * SceneManager employs the previously mentioned SharedStateManager to improve efficiency bt sharing StateAttributes and StateSets. 
        * But it contains a mechanism (state sorting), that causes unsafe modofications of StateSets of objects loaded through the SceneManager as they can be shared by another object. To made modifications, follow an approach described above in "Threading Considerations".
        * Be aware that cloning a StateSet has performance implications beyond the clone itself - the clone will negate the substantial benefit of the state sharing mechanism employed earlier. If at all possible, do *not* use this approach for objects used many times in the scene. There are plenty of alternative approaches as described earlier.
    * Optimizer | Restructures a scenegraph in a way that is functionally equivalent yet increases the rendeirng speed. 

Please read [here](https://wiki.openmw.org/index.php?title=Rendering_Architecture) for information on other aspects of the rendering architecture.

GUI
---------------------------------
OpenMW's game UI is built on [MyGUI](http://mygui.info/). To learn about MyGUI visit the [code documentation](http://mygui.info/docs/), its [wiki](http://www.ogre3d.org/tikiwiki/MyGUI), the [source code](https://github.com/mygui/mygui) and more specifically the examples contained in the source code.

* **Layout System** | MyGUI's XML format for layouts/skins has been used for the majority of the OpenMW GUI. The resulting files can be found in this [subdirectory](https://gitlab.com/OpenMW/openmw/-/tree/master/files/data/mygui) of source code repo (or its [mirror](https://github.com/OpenMW/openmw/tree/master/files/data/mygui)).  
    * Please note that most of OpenMW's skin files were written using a deprecated syntax. The now preferred and more powerful syntax is the ResourceLayout, for which an example can be found [here](https://gitlab.com/OpenMW/openmw/-/blob/master/files/data/mygui/openmw_resources.xml). This syntax more closely resembles the one used for layout files.
* **Escape Sequences**
    * As the MyGUI [wiki](http://www.ogre3d.org/tikiwiki/tiki-index.php?page=MyGUI+text+formatting) explains, text widgets support escape sequences starting with '#', for example:
        * **#ff0000text** results in a red 'text'.
        * **#{foobar}** will be replaced with the value for 'foobar' in the MyGUI::LanguageManager.
    * Colour-codes (like **#ff0000**) are always used regardless of whether 'setCaption' or 'setCaptionWithReplacing' is used. To use escape sequences in a layout one must use the 'Caption' property.
    * To avoid '#' characters in a caption being treated as an escape sequence, this character needs to be escaped by adding another '#' character: '##' produces '#'. MyGUI provides the function MyGUI::TextIterator::toTagsString to do this.

Please read [here](https://wiki.openmw.org/index.php?title=GUI_Architecture) for further information on other aspects of the GUI architecture.

ESM/ESP Formats
---------------------------------
Up to version 1.0 we will use the original esm/esp formats nearly unchanged. Currently OpenMW ignores any pre-compiled scripts and does JIT-compiling instead. The new CS will not generate pre-compiled scripts either.

A documentation of the original format can be found [here](http://www.uesp.net/text.shtml?morrow/tech/mw_esm.txt).

*Please note that this documentation is not fully complete and in some cases not fully correct either.*

Dependency Policy
=================================
OpenMW consists of three distinct, vertically arranged subsystem:

Apps
---------------------------------
The OpenMW project consists of multiple applications/tools:
* **openmw** | This is the actual game.
* **openmw-launcher** | A launcher for openmw.
* **[esmtool](https://wiki.openmw.org/index.php?title=Esmtool)** | A esm/esp file analysis tool (command line only).
* **bsatool** | A bsa file analysis tool (command line only).
* **openmw-cs** | A replacement for the Morrowind Construction Set.
* **openmw-iniimporter** | An importer for Morrowind.ini files.
* **openmw-essimporter** | An importer for vanilla Morrowind save files. This is currently in early stages - imported saves will be missing functionality, see the [bug tracker](https://bugs.openmw.org/projects/openmw/issues?utf8=%E2%9C%93&set_filter=1&f%5B%5D=status_id&op%5Bstatus_id%5D=o&f%5B%5D=category_id&op%5Bcategory_id%5D=%3D&v%5Bcategory_id%5D%5B%5D=24&f%5B%5D=&c%5B%5D=project&c%5B%5D=tracker&c%5B%5D=status&c%5B%5D=priority&c%5B%5D=subject&c%5B%5D=assigned_to&c%5B%5D=updated_on&group_by=&t%5B%5D=).
* **openmw-wizard** | An installation wizard for the game files that are required to run OpenMW.

Components
---------------------------------
A collection of reusable components. Most of these components are used by more than one application within the OpenMW project.

See the source [tree](https://github.com/OpenMW/openmw/tree/master/components) for a list of components.

An ideal component is self-contained with no additional dependencies on the OpenMW codebase. In some cases depending on another component is OK.

Extern
---------------------------------
Each subsystem may only depend on the subsystems below it. This policy is currently not enforced by the build system (but we have a [task](https://bugs.openmw.org/issues/20) on the issue tracker for changing that).

Unit Tests
=================================
OpenMW unit tests are [here](https://gitlab.com/OpenMW/openmw/-/tree/master/apps/openmw_test_suite). It uses [gtest](https://github.com/google/googletest) framework.

If you add a new component, consider to add unit tests for it as well.

**How to Run:**
* Install dependencies (example for Ubuntu):
```console
sudo apt install libgtest-dev libgmock-dev
``` 
* Configure OpenMW to build tests:
```console
mkdir build && cd build
cmake .. -DBUILD_UNITTESTS=ON
make
``` 
* Run all tests:
```console
./openmw_test_suite
``` 
* Run some tests:
```console
./openmw_test_suite --gtest_filter='SettingsFileParserTest*'
``` 

General Advice
=================================
* Read further information on topics above in the OpenMW wiki, a good place to start for beginners is the [FAQ](https://openmw.org/faq/).
* Prefer C++ means over C means. In particular:
    * Do not use the preprocessor for purposes other than conditional compiling (this includes include guards)
    * Use C++ streams instead of C I/O
    * Use new/delete instead of malloc/free (but usage of new/delete should also be limited, see below)
* Prefer STL-container over raw arrays and new[]/delete[].
* Use new/delete only when necessary. Prefer automatic storage duration.
* Throw exceptions instead of retuning error codes.
* When returning a pointer, don't return a 0-pointer in case of an error (returning a 0-pointer is still valid, if it does not represent an error situation).
* Remember the [Rule of Three](https://en.wikipedia.org/wiki/Rule_of_three_%28C%2B%2B_programming%29).
* Use osg means where it is applicable. In particular:
    * Use osg::PI instead of M_PI;
    * Use osg::Vec3d instead of double[3];
    * Consider to use osg::clampBetween(v, 0.f, 1.f) instead of std::max(0.f, std::min(v, 1.f)).
