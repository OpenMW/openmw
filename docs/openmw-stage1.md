# Preamble

This document outlines the development of OpenMW after the 1.0 release.

For the first couple of feature releases after 1.0 we should focus on giving mod developers additional tools so they can produce interesting mods with features not supported in vanilla Morrowind. If we look at the use of Morrowind (e.g. on Twitch) we see that people still overwhelmingly make use of the vanilla engine instead of OpenMW. We want to make OpenMW the default choice when playing Morrowind-content. The way to achieve this goal are interesting and desirable mods that require OpenMW.

Now is not the time for sweeping architectural changes. With 1.0 we acquired an enormous advantage over vanilla Morrowind (full source access) and now we should cash in on that advantage by pushing out large numbers of features that are directly visible to the user.

At the same time we want to stay true to our origins. While we want to broaden the scope of OpenMW and make it into a more general purpose engine (in the sense that it is used for more than just playing Morrowind), it should still be a Morrowindesque engine. The general direction should be to widen and to generalise. We aim to evolve, not to revolutionise.

Our goal here is to make OpenMW into a general purpose engine, but a general purpose 1st/3rd person real-time RPG engine. We do not attempt to support other genres or other flavours of RPG.

The development of OpenMW will hopefully continue for a long time and we can not reasonably hope to sketch out its entire future development in a single design document. Therefore this document should be seen as stage 1 of the post 1.0 development only. It may last us 6 months or a year or several years, depending on how much development activity we can achieve. But eventually there will be a stage 2 design document.

# Definitions

## Instances & Objects

The terminology regarding instances and objects is somewhat confusing since there are several variants of it that are incompatible with each other. Throughout this document we will use the following variant:

* Instance: The sub-records placed within a cell record. Mostly called references in the vanilla Morrowind terminology. In C++ OOP we would rather call this an object or an instance.
* Object: Records like doors, activators, containers and so on. Mostly called IDs in the vanilla Morrowind terminology. In C++ OOP we would rather call this a class.

In a few cases the term "object" will be used in the classical OOP sense. Any such mentioning will be accompanied by a statement about the different use.

## Deprecation

This document declares some features and functions as deprecated. This usage is somewhat different from the usual meaning in software development. Usually something being deprecated not only means that its use is discouraged but also that it may be removed in later versions. In OpenMW we can not ever do the later because that would break compatibility with older content.

Instead a feature being deprecated shall mean the following:

* We discourage the use of the feature (and may reinforce this position by non-intrusive warning messages at some point)
* We will not extend the feature to work with other enhancements of OpenMW.

A hypothetical example: We deprecate the MessageBox instruction. If we introduce new variable types, the MessageBox text formatting will not be extended to cover these.

## Script Instructions and Script Functions

We define a script instruction as a statement that does not have any return value and therefore can't be used as part of an expression. We define script functions as a function that does have a return value. Sometimes the term script instruction is used to mean both instructions and functions (since we do not have a term that covers both).

We also use the term script function for user defined scripts that are used as functions.

The meaning should be clear from context.

# Organisation & Procedures

This document describes the core enhancements for the next couple of feature releases after 1.0. This does not mean that these releases are limited to what is described here. We have a whole feature request forum full of ideas and many issues on the bug-tracker. But we need to be careful here about what we add and what not.

With 1.0 we have a clear goal in front of us. After 1.0 things are less clear. We need to consider that any feature we add now and that influences the file formats will stay forever. We can not ever remove features of this kind because that would break compatibility with older content files. Even with features that don't influence the file format we should consider carefully so the engine does not end up as a bloated mess.

## Versioning

We continue with the semantic versioning scheme, meaning the next feature releases will be 1.1.0, 1.2.0, 1.3.0 and so on.

We will only jump to 2.0.0 if a fundamental change is made (e.g. multiplayer). Otherwise we will continue, even if that means we end up with version 1.314159.0 eventually.

OpenMW and OpenMW-CS will be kept in sync after 1.0.

**Important**: If OpenMW reaches 1.0 before OpenMW-CS, we need to wait for OpenMW-CS to catch up before we continue with feature development.

## Bug Fix Releases

We may decide to have regular bug fix releases in parallel to the feature release development, i.e. while we work on 1.2.0 there could be 1.1.1, 1.1.2, 1.1.3 and so on. If OpenMW is adopted more widely and the development of feature releases takes a longer time this approach might be beneficial to the end users. We would then continue to develop on the release branch of the feature release which then becomes a maintenance branch. This could either happen via (i) merging bug fix commits into the maintenance branch and then periodically merge the maintenance branch into master or (ii) merge bug fix commits both into the maintenance branch and master.

However this approach means additional work and would be of limited usefulness if we have very quick feature releases. We should hear the people involved in the release process on this topic before we make a decision, because they would be the ones having to do most of the additional work.

## Roadmap

We will continue with the revised roadmap scheme. This means we add a new label (stage1). After we are finished discussing this design document and have made necessary adjustments we will cut it up into individual tasks and add them to the tracker with the stage1 label. This label will then take the role of the current 1.0 label.
Confirmed bug reports also get tagged with stage1. Other issues (feature requests and tasks) only after we have approved them for near future development.
We will most likely not have a separate stage1 label for the editor, since for the bulk of the changes (The Grand De-hardcoding) most tasks we will require changes to both OpenMW and the editor.

# Content File Format & Namespaces

We will continue with the same general structure of the content file format. All changes will be specific to individual record types.

We already have a new version counter in the omwaddon and omwgame file formats. We will continue to increase this counter with every new feature release that requires additions or modifications to the file format.

The ability to read files of older formats must be maintained. The ability of the editor to write older file formats is optional and unless there is strong demand and enough manpower we will most like not support this feature.

## Records

Records from older file formats are adjusted and converted during the load process (both in OpenMW and OpenMW-CS). The data structures in OpenMW and OpenMW-CS will always match the most up to date version.

The following cases of changes to the record format have been identified:

* New optional records: No action on load required since the older formats won't have this kind of record.
* New mandatory records: Records are build from cfg values or/and GMST values. Typically such a record will merge several values from these sources. We create these records only if the omwgame file is of a version prior to the introduction of this record. We need to modify the record when omwaddon files provide their own record for the value sources (i.e. GMST records).
* Existing records with new optional fields: No action on load required. We already support this.
* Existing records with new mandatory fields: Same as with new mandatory records.
* Existing record that needs to be fully converted into a different record type: Conversion needs to be performed on load.
* Existing record that is switched over from integer-based to ID-based indexing (this is a special case of the case above)
* Existing records is split into two records: Split needs to be performed on load.

There have been some concerns regarding the efficiency of string based IDs (vs. integer). While there hasn't been an evidence indicating that this is actually a problem in OpenMW, it is important to point out that the move from integer to string does not mean that such a problem could not be addressed.

We already have plans to introduce a new string type for IDs, which deals with the case-issue (current std::string based implementation is error-prone and results in some very verbose code).

If we are careful when designing this new string class it will be easy to swap out the implementation from a string based one to an integer based one, where the actual strings are stored in a hash-table (similar to the FName class in Unreal Engine 4).

If we allow older format content files to depend on newer format content files the loading process will become more complicated (see the example given in the "New mandatory records" case). We could avoid these complications by forbidding content files from depending on content files with newer format. This limitation however would have a noticeable usability impact. This issue needs to be discussed further.

All new records will be identified by a unique string ID. We will not add any new record types that are indexed by integer values (e.g. skills) or other schemes. Descriptions of new record type in this document will usually leave out the ID field.

## Namespaces

Within the editor we already have limited pre-1.0 support for namespaces (using C++ syntax). We will continue to expand on that. Namespaces are case-insensitive.

Namespaces will serve as a tool to avoid naming conflicts between content files and to organise records within a content file.

We encourage a clean use of namespaces, meaning there will be no equivalent of 'using namespace'.

We will not enforce the use of a single namespace (not accounting for sub-namespaces) per content file. But we will strongly encourage this approach.

There are four special namespaces:

* sys: This is equivalent to std in C++. In general only we are allowed to add new IDs to it (e.g. new GMSTs).
* project: Records within this namespace are saved in a local project file, but may also be presented to OpenMW when starting it through OpenMW-CS.
* session: Records with this namespace exist only for the duration of a single editing sessions and are currently not presented to OpenMW.
* default: Previously called sys::default (see section about cells and worldspaces)

To encourage at least some degree of stylistic uniformity we will use only all lower case names for namespaces in all official documentation.

Having a record with the same ID as a namespace is allowed (see section about cells & worldspaces for an example).

# Meta-Data & File Management

## Identifying Meta-Data

Identifying meta-data specifies the identity of the content file in a formal way that can be utilised by various tools.

There are four parts to this meta data:

* Name: A unique name that identifies the content file across all versions and all languages
* Version: A unique version identifier. A uniform versioning scheme is required here to avoid unnecessary complexity in the tools. Most likely we will use the same semantic scheme as with OpenMW itself.
* Language: see section about localisation
* Multiplayer: see subsection about multiplayer in misc section

There are two possible ways to encode this meta data:

1. In a new record or sub-record
2. In the file name

The record option is more flexible and allows for extensions at a later point with the least amount of disruption. It also avoids unnecessarily cryptic file names. On the other hand we need unique file names anyway and storing the information in the file name avoids redundancy. This needs to be discussed further.

There is a large number of possible uses for this kind of meta data. Here are some examples:

* Simplifying the content file selection within the launcher: All entries for the same content file can be grouped into a single row. We can then add two columns for version and language that contain a combo box with the available options. That should reduce the amount of clutter significantly and also should help to avoid user errors because it is now impossible to select two versions of the same file at once.
* We can add a language setting to the launcher and make it automatically select the preferred language for content files (where available).
* Tools for porting content files from one version of a dependency to another will benefit from being able to automatically identify that two different dependencies are actually just different versions of the same file.

## Non-Identifying Meta-Data

### License

We provide the user with a reasonable list of license choices (including a custom one where the user can provide his own license text). When the content file is packaged by the editor, a copy of the license text is included automatically. For newly created content files this field should be set to a sensible default (definitely not custom). Furthermore the merge tool can check for possible license conflicts.

This enhancement will hopefully cut down on the license wild-growth within the Morrowind modding community. If we want to get fancy with this feature we could also add a tool that helps the user to decide on a license.

### List of Authors

Currently we have a single text field for the authors. This is suboptimal for community projects. With the current solution the best a community can do is to insert 'The x Team' and then maintain an external list.

A better solution is to replace this field with a list of names that can be automatically merged (including checking for duplicates) when merging content files.
The editor can be configured with a user name, which is automatically inserted when creating a new content file or modifying an existing content file.

## Resources Files

### Problem

Currently all resources files of all installed content files are dumped into a single directory. This poses several problems:

* A content file that comes with resources files that override other resources files can not be simple switched off by disabling it in the launcher.
* Without external record keeping there is no way of knowing which resources files to include when packaging a content file for release. This is a particularly big problem if we want to automate this process.
* Automated tools for checking for missing resources won't be able to look at the resources for the currently selected stack of content files only, which means some problems may remain undetected.

### Data Paths

Some people have started to put different content files into separate directories, each with its own data path. This is not a solution.

The selection of data path is a separate stage from the selection of content files. More precisely the available selection of content files depends on the selection of data paths. This results in a two stage content selection mechanism, which is undesirable from a usability perspective.

### Solution

The solution is to group resources files within the data directory structure.

The first proposed solution (putting all resources files into a single archive with the same name as the content file) received a lot of criticism during forum discussions.

As an alternative we could create a single content directory (per content file) within the data directory. This content directory would then contain the content file and the usual directory structure (Icons, Meshes, Music, ...).

We may offer one of these solutions or both. This needs to be discussed further.

In the forum the following flaw in the archive solution has been brought up (applies equally to the directory solution):

In some cases one may want to use only the resources from a content file without the actual content file and license reason may stop the repackaging in a form that allows this kind of usage within the new system.

There is an easy fix for this issue: We add a flag to the dependency list in each content file. If this flag is set for a dependency, only the resources are used and the actual content file is ignored.

### Importing & Backwards-Compatibility

Most of the current content exists in a form that is not compatible with the new system.

In most cases this can easily be resolved by adding an importer tool to the launcher, that takes the content and copies it into the desired organisation scheme (archive or directory).

For other content (pre-existing data directories and Morrowind, since Morrowind and its add-ons are installed into the same directory) we need to maintain a level of compatibility with the old approach, that can be switched on and off as needed.

### Resources Packages

During the discussion about the new content file organisation scheme (omwgame, omwaddon) there was some criticism about the scheme not allowing plug-ins with no dependencies.

This is not a problem, because such plug-ins are generally useless. A plug-in without any dependency can not modify or reference any record. That was not such a large issue with vanilla Morrowind, because in this case the plug-in could have still have depended on things like attributes or dynamic stats. However in OpenMW these will all turn into records and therefore can not be accessed from within an omwaddon file without dependencies. This will only get more extreme as the de-hardcoding progresses further. An omwaddon file without dependencies can do literally nothing, which makes it useless by definition.

But there is one exception. Some members of the mod community have provided resources packages (meshes and similar) that can be used by other content developers. Under our new resources scheme these would have to be accompanied by an omwaddon file. This is not a good solution because of two reasons:

* a omwgame file may need to depend on such a resource pack (currently impossible)
* a resource pack can be used by omwaddon files that depend on any omwgame file (also currently impossible)

Therefore we will introduce resources packs as a third kind of content type. Resources packs do not have dependencies and they can be placed in any position in the load order (provided the load order fulfils the dependency requirements of the content files depending on the resources pack). This will require some adjustments to the content file selector GUI.

Note that resources from a resources pack can still override resources from other resources packs and other content files earlier in the load order. Therefore the load order is highly relevant for resources packs too.

A resources pack consist of an omwres content file and the associated resources files. If we choose the file-name variant for identifying meta-data and the archive implementation path (vs sub-directory), we could theoretically skip the omwres content file and just make the resources archive the omwres file instead. However this is not a good idea, because that would leave us no way to carry along author and license information.

## Packaging

The editor will be enhanced with a packaging tool that can take a content file and its associated resources files and package them into a form suitable for distribution.

The question now is what is a suitable form. For Linux the first option that comes to mind are the respective software package management systems. This of course does not help Windows and Mac users. While there is the obvious and very reasonable answer ("Get a real operating system!") this seems unnecessarily harsh to Mac users and is overall not helpful.

Therefore we will develop our own cross-platform package format for OpenMW content. The exact scope still needs to be determined, but it could be as simple as an archive that holds the content file and the associated resources files. Giving this archive type a unique file extension (e.g. omwpack) would then allow the launcher to pick up opening requests (i.e. double click on the file) and install the content (optionally including a check for dependencies).

# Scripting

**Note**: Extensions to the scripting language in form of new instructions and functions are distributed over this entire document. In some cases, features may require additional functions that return the value of certain fields of certain records. These functions are usually not listed explicitly and are left as an exercise to the reader.

## Language Version

The version of the language used in a script is determined by a single integer number which is stored in a new subrecord within the script record. Note that this is different from earlier concepts, which used a more complicated version identifier and stored it within the actual script.

Scripts that are missing this subrecord are considered version 0 (legacy). New scripts (when created in OpenMW-CS) default to the most up-to-date version. The version can be selected from a combo box in the script subview. We may add a user setting that hides the combo box UI if the version is the most up to date one.

The version number is incremented whenever an addition to the scripting language is made (at most once per feature release). A new feature release is not required to increment the version number, if no feature changes have been made to the language in this release.

From version 1 on, all workarounds for bad scripts will be disabled. This should not break anything for well-written scripts. In general, moving to a new language version should usually at most require minor fixes.

Since old scripts will remain with the old language version (unless there is a need to modify them and use newer features), this scheme will not break compatibility for existing content.

## Error Handling

We will continue with the existing runtime error handling scheme (i.e., stopping the script execution on a runtime error). Introducing exception handling instead would be overkill.

We will do our best to avoid crashes as a result of broken scripts. That means (among others) that we need to put an artificial (configurable) limit on function call depth to avoid crashes and lock-ups from endless script recursion. We may also decide to put a similar limit on loops.

## Variable Types

The types `long` and `float` remain unchanged.

We will flag `short` as deprecated. Currently `short` has no advantage over `long` (it uses the same amount of memory in OpenMW). For anything but a very specific kind of bit-cutting the `short` type has no use at all.
If we ever decide that we do want a shorter integer type we can always de-deprecate `short`.

We will introduce several new types:

### Strings

The name of the new string type is `String`, and string literals are marked by placing them in quotation marks. Example:

````
String a
Set a to "Some Text"
````

Comparison operators for strings are provided in the usual way.

Because we are discontinuing bad script workarounds, most ambiguity regarding string literals should have been removed already (we will not accept local variable names and instructions within quotation marks any more). Only a single problem case remains:

If a function requires an ID, is it a literal ID or is it the name of a string variable that contains an ID? Example:

````
String Fargoth
Set Fargoth to "player"
Fargoth -> AddItem "Gold_001", 100
````

We cannot make a rule that forbids the use of local variable names that are also IDs, because that would allow random content files to break scripts in other, unrelated content files.

Therefore, the solution here is to simply give the local variable precedence over the ID. If the script author intended to give money to `Fargoth`, he should not have created a local variable with the same name.

Note that global variables are not an issue here, because global variables are IDs and IDs need to be unique (with certain exceptions that are not relevant here).

To remove even the last remaining potential problems with ambiguity, we will also introduce a new string literal that can only be a literal and never a string variable name. These literals are preceded by an `L` character. Utilising this feature, the aforementioned code block could be rewritten as is shown below, if the script author absolutely insists on having a local variable named `Fargoth`:

````
String Fargoth
Set Fargoth to "player"
L"Fargoth" -> AddItem "Gold_001", 100
````

### Instance References

The name of the new reference type is `Ref`. It can reference an instance in the world or in a container or be a null reference.

Since we currently have no way to reference an instance persistently, the use of the `Ref` type is limited for the time being (see section *Variable Scope* for further details).

References can be used in any place that would otherwise allow an ID that stand for an existing reference. The rules for strings regarding ambiguity apply to references in the same way.

A reference will implicitly cast to 0 (a null reference) or 1 (not a null reference) when used in a numeric expression.

**Note**: A reference pointing to the contents of a container points to a stack of items and not a single item.

We introduce the following keywords:

* `Self`: A value of type `Ref` (the instance the script is currently running on, only available in local scripts and dialogue scripts)
* `NullRef`: A literal of type `Ref` (null reference)
* `GetId(r)`: Returns the ID of the reference `r` as a string value
* ``GetContainer(r)`: Returns the reference of the container `r` is in (or a null reference if `r` is not in a container)
* `GetCell(r)`: Returns the ID string of the cell `r` is in (either directly or via a container)
* `SearchActive(id, container = 0)`: Returns a reference with the given ID within the active cells. If `container != 0`, also check in containers; returns a null reference if no reference is found
* `SearchIn(id, id2)`: Returns a reference with the given ID within something (`id2`) that can contain references; this can be a cell, a worldspace, a container, a creature or an NPC. If `id2` represents an instance, a `Ref` variable can be given instead; returns a null reference if no reference is found

### Lists

A single-type list will be available for the following types:

* `long`
* `float`
* `String`
* `Ref`

We will not support mixed-type lists because that would require a major change to the type system. Internally and functionally, a list will work like an array. The type names for lists are:

* `LongList`
* `FloatList`
* `StringList`
* `RefList`

List literals are given as a comma-separated list of values in square brackets. Empty list literals are allowed. In the context of lists, we generally allow implicit promotion of integer to float types.

We support the following list operations:

* `l1[i]`: Returns member `i` (indexing begins at 0)
* `GetSize(l1)`: Returns the number of elements in `l1` as an integer
* `l1 + l2`: Returns a concatenated list consisting of the members of `l1` followed by the members of `l2`
* `GetSubset(l1, l2)`: Returns the common subset of the elements of `l1` and `l2`
* `HasElement(l1, e)`: Returns 0 (if `e` is not element of `l1`) or 1 (otherwise)
* `GetIndex(l1, e)`: Returns the index of the first occurrence of `e` in `l1` (or -1 if none)
* `Minimum(l1)`: Returns the minimum of the elements in `l1` (only works with numerical types)
* `Maximum(l1)`: Returns the maximum of the elements in `l1` (only works with numerical types)

and the following list instructions:

* `Set l1[i] To e`: Sets list element `i` of list `l1` to value `e` (indexing begins at 0)
* `Sort(l1, cf="")`: Sorts list elements into ascending order (if `cf` is an empty string) or via comparison function script `cf`
* `Filter(l1, ff)`: Filters list `l1` by filter function `ff` (only keeps elements that do not return 0)
* `Reverse(l1)`: Reverses order of list elements of list `l1`
* `Append(l1, e)`: Appends element `e` to list `l1`
* `Append(l1, l2)`: Appends elements of list `l2` to list `l1`
* `Remove(l1, e)`: Removes all elements equal to `e` from list `l1`
* `Remove(l1, l2)`: Removes all elements from list `l1` which are also in list `l2`
* `InsertAt(l1, e, i)`: Inserts element `e` into list `l1` at position `i` (moving the element at index `i` one position upwards and extending the size of the list by one)
* `RemoveAt(l1, i)`: Removes the element at position `i` from list `l1` (moving elements with an index higher than `i` down by one and reducing the size of the list by one)
* `Resize(l1, n)`: Set size of list `l1` to `n`; if the list is extended, additional elements are initialised with default values

Variable names used in the lists above:

* `l1` and `l2` are lists / list literals of the same element type
* `i` is an integer value and is used as list index
* `n` is an integer value
* `e` is a variable/literal of the element type of `l1` and `l2`
* `cf` is the ID of a comparison function script that takes two element-type arguments and returns an integer (-1, 0, 1)
* `ff` is the ID of a filter function script that takes one element-type argument and returns an integer (0, 1)

### List Aliases

We define three more type aliases:

* `Vector3`: A `FloatList` of 3 members, used for location or rotation
* `Vector4`: A `FloatList` of 4 members (location and rotation around z-axis)
* `Vector6`: A `FloatList` of 6 members (location and rotation)

These aliases are not separate types.

In the specification of additional functions and instructions, values of these types are specified as `v3`, `v4`, and `v6` respectively. Passing shorter lists is allowed and will set the missing elements to 0.

**Note**: All rotations are specified in degrees.

## Variable Scope

All variable types will be available at all scopes with the exception of reference variables. Reference variables will be limited to true local scope (see below), because all other scopes require writing the variable to a saved game file and we currently don't have a general method to serialise instance references. This is a task for stage 2 or more likely OpenMW 2.0.

The global and the local scope remain unchanged. We will add two more scopes:

### True Local Variables

What the Morrowind scripting language describes as local variables does not match the concept of local variables in other languages. In case of local scripts, Morrowind local variables function as member variables. In case of global scripts, Morrowind local variables function as static variables.

There are several reasons for the introduction of true local variables:

* Less pollution of local dialogue variable names
* Less state that needs to be saved
* Prevention of potential scripting errors when the state of a local variable is maintained across multiple script executions and the script does not re-initialise the variable at the beginning of a new run

We will call this scope **true local variable** to distinguish it from old local variables.

Declaration of true local variables will look like this:

````
Local long a = 1
Local long b
````

If no default value is given, the variable is default-initialised (value 0, empty string, empty list, null reference).

### Record Variables

A common problem (especially relevant for large projects with complex functionality) is that there is no place other than global variables to store additional state that is not specific to an instance. A global function with multiple variables can be used instead, but that is just a workaround.

Furthermore, there is currently no way to add new state to other objects (object in the OOP sense, not in the Morrowind sense) other than instances.

Therefore, we introduce record variables, which are a generalisation of old local variables in local scripts.

The respective records will be enhanced by an optional list of subrecords that can declare record variables and their default values.

Obvious candidates for this feature are:

* Objects
* Cells
* Regions
* Worldspaces
* Factions

**Note**: Object record variables are essentially the same as old local variables declared in a local script - only that they are declared in an object record instead.

We will introduce the following record variable functions:

* `Has{Type}(vn)`: Returns 1 if record/reference has a long/float/string/list record variable of name `vn`, 0 otherwise
* `Get{Type}(vn)`: Returns the value of record/reference variable `vn`. Note that we don't use the x.y syntax here, because we need to explicitly specify the type to allow proper compile-time error checking for reference variables.

and the following record variable instructions:

* `Set{Type}(vn, val)`: Sets variable `vn` to value `val`. It is considered an error if this variable does not exist.

Within all functions and instructions, the record/reference is specified via the `->` operator in the usual way. When using object records (via ID), the first instance found is used instead of the object record.

Variable names used in the lists above:

* `id` is a `String`
* `vn` is a `String`
* `ref`is a reference of type `Ref`
* `val` is a value according to `{Type}`

Finally, `{Type}` stands for `long`, `float`, `String`, `LongList`, `FloatList`, or `StringList`.

We can fold the access of local variables in global scripts into the new record variable system, even though global scripts do not have explicit record variables. The new functions for record variable access make the old x.y syntax obsolete and, therefore, we declare it as deprecated.

## Control Structures

We will add a `For` loop that works on lists. To avoid unnecessary complexity in the language and to encourage the use of lists, we will not have an index-based `For` loop (these can easily be simulated via `While`).

Example:

````
For x In l
  do something with x
EndFor
````

We will add the `break` and `continue` keywords both for `For` and `While` loops.

We may add a `switch-case` construct, but this is most likely a stage-2 task since there are many different ways to do `switch-case` and no obviously superior solution. This will most likely require extended discussion and design work.

## General Additions

The vanilla scripting language is missing a few pieces of basic functionality. Therefore, we need to add the following kinds of instructions:

* Trigonometric functions (`Cos`, `Sin`, `Tan`, `Acos`, `Asin`, `Atan`, `Atan2`, `DegToRad`, `RadToDeg`)
* Logical boolean function (`And`, `Or`, `Not`, `Xor`)
* Other mathematical functions (`Abs`, `Floor`, `Ceil`, `Clamp`, `Lerp`, `Sign`)

## Object Management

We will add a function that moves an instance into the world. This feature (especially the `move` function) preludes a concept of persistent instance identity, which is most likely a stage-2 or OpenMW-2.0 feature. For now, these functions are required to deal with state attached to the instance in the form of record variables or old local variables respectively.

* `MoveObjectToWorld(target, v6)`: Moves the instance from current location to world location `target` at coordinates `v6`
* `CloneObjectToWorld(target, v6)`: Adds a copy of the instance `target` to world location `target` at coordinates `v6`

**Note**: In all cases, reference specification for objects to be moved/cloned can be done via the `->` operator in the usual way.

**Note**: Moving or cloning out of a container will only move or clone a single item, not the whole stack.

The `target` value represents the target cell or worldspace (see section *References to Cells in Data Structures* for details)

We will add replacement functions for object-placement-related functions without declaring the original functions deprecated.

Instructions:

* `SetRotation(v3)`: Sets the instance's rotation; actors ignore the first two elements (replaces `SetAngle`)
* `SetPosition(v3)`: Sets the instance's position (replaces `SetPos`)
* `Teleport(target, v6, strict = 0)`: Teleports the instance to location specified by `target` and `v6`; actors ignore the first two elements; if `strict != 0`, no corrections are made to the specified location, otherwise the location may be adjusted for safety (replaces `Position` and `PositionCell`)
* `Spawn(objectId, target, v6, strict = 0)`: Same as `Teleport`, but spawns a new object `objectId` (replaces `PlaceItem` and `PlaceItemCell`)

Functions:

* `GetRotation`: Returns the instance's rotation as a `Vector3` (replaces `GetAngle`)
* `GetPosition`: Returns the instance's position as a `Vector3` (replaces `GetPos`)

## Object-Type-Specific Additions

We will add functions to modify and query other non-item-specific state that is already present in the cellref class and therefore already written to saved game files.

Instructions:

* `SetTrap(trapId)`: Sets the instance's trap ID to `trapId`
* `SetKey(trapId)`: Sets the instance's key ID to `keyId`
* `SetTeleport(target, v4)`: Sets the instance's teleport location to the location specified by `target` and `v4` (does not affect teleport flag)

Functions:

* `GetTrap`: Return the instance's trap ID
* `GetKey`: Return the instance's key ID
* `GetTeleportTarget`: Return the instance's teleport target
* `GetTeleportPosition`: Returns the instance's teleport location and zrot as `Vector4`

## Object-Types

We introduce compile-time constants for object types (Weapons, Activators and so on). These have integer values. The constants must be named consistently in such a way that name collisions are unlikely (e.g. `TypeWeapon`, `TypeActivator`).

We add a function that returns the type of an object or instance:

* `GetObjectType`

## Queries

A query returns a list of instances within a certain area. Along with the description of the area, the query takes a list of object types (`t`) as an additional argument.

* `QuerySphere t, v3, r`
* `QueryBox t, v3_tl, v3_br` (this is an axis-aligned box)
* `QueryCylinder t, v3, r, height` (the cylinder is aligned along the z-axis, `v3` specifies the middle point of the bottom circle)

## Text Formatting

The vanilla scripting language provides text formatting in a single place (`MessageBox` instruction). This is insufficient:

1. We will need text formatting in many places, independently of message boxes.
2. The `MessageBox` text formatting sucks.

We will deprecate `MessageBox`. As a replacement, we will introduce new instructions for creating message boxes (see following subsection and *GUI* section) without a formatting feature and separate text formatting functions.

* `Str(v, p = -1)`: Returns a `String` containing the string representation of the numeric value `v`. `p` specifies the number of digits after the decimal point. If `p == -1`, a suitable value depending on the type of `v` will be chosen
* `LFormat(s, a)`: Returns a `String` equal to the `String` `s` with all occurrences of %x replaced with the respective element indexed x from the string list `a`
* `Join(l, s)`: Returns a `String` consisting of the members of the `StringList` `l` separated by the `String` `s`

## Multiple-Choice Message Boxes

We add two new functions for showing multiple-choice message boxes:

* `MakeChoice t, sl, f`
* `MakeBranchChoice t, sl, fl`

Arguments are as follows:

* `t` is the message box's text
* `sl` is a list of strings, defining the available options
* `f` is the ID of a script function that is called when an option is selected; the function takes one `long` argument (the option index, starting at 0)
* `fl` is a list of script function names with the same length as `sl`; the functions do not take any arguments; empty strings in the list are allowed, which makes OpenMW ignore the respective choice

## Functions

We extend scripts to become callable functions with argument passing and a return type.

### Declaration

The syntax of the begin statement is extended in the following way:

````
Begin <function-name> { ( <list-of-arguments> { -> return-type } ) }
````

**Note**: `{}` denotes optional parts.

Argument lists are allowed to be empty and are comma-separated. Elements in argument lists are type-name pairs. Elements can be given default values in a C++ fashion.

Arguments and return values are for all intents and purposes true local variables. Therefore, the `Ref` type is available.

### Calling

A function can be called by its name followed by parentheses. Arguments are listed inside the parentheses separated by commas.

We may at some point add the use of named arguments for function calling (comparable to Python), but this is most likely a stage-2 feature.

If the function has a return type, the function is evaluated as an expression with a type corresponding to the return type.

Calling a function pauses the execution of the calling script, executes the called script and then resumes the execution of the calling script. This is different from the `StartScript` and `StopScript` instructions. The `StartScript` instruction is not modified (i.e., no argument passing).

A function call must be performed via a function name literal. The function name cannot be given as a string variable, since this would make it impossible to check for the correctness of the function signature at compile time.

Local and global scripts must not have any arguments without default values.

## Script Hooks

We introduce two new record types (hook, hooked script). The hooked script is a generalisation of the concept of a start-up script. We may decide to fold the start-up script record type into the hook record type.

### Hooked Scripts

A hooked script record binds a script to a hook record. More than one hooked script record per hook record is allowed.

The ID of a hook record is built from the hook record ID followed by a `$` followed by the script name.

Example:

````
Startup$KillFargoth
````

By building hook IDs in this way, we allow content files to delete hooked script records in dependencies without the need for an additional ID that identifies individual hook records.

**Note**: If more than one script is attached to a hook, the order in which the scripts are executed is, at this point, implementation-defined and, thus, scripts must not depend on a specific order.

### Hooks

A hook record declares a new hook. Each hook record contains a list of argument types with optional default values (again comparable to C++ function calling).

There will be no return values (problematic since there can be multiple scripts per hook).

We provide system hooks within the namespace `sys` that are called in specific situations (see the section about de-hardcoding for some examples). Content developers may also provide their own hooks (user hooks).
System hooks are not added to content files, since they cannot be modified by content developers anyway. We will instead inject the system hook records on content-file load.

System hooks are triggered by the engine. System hooks and user hooks can be triggered explicitly with a new script instruction.

## Script Slots

We define the term script slot as an optional subrecord in an object record that contains the name of a script. The default script slot for most object types is the "Run-Once-Per-Frame" slot, a.k.a. local script.

The default slot does not take any function arguments, but other slots can. The function arguments of a script attached to a slot need to match the hardcoded argument list of the slot.

**Note**: If two scripts attached to the same object both define a (non-true) local variable with the same name, there will be only one variable. It is an error if the type of these variables don't match.

### Additional Slots

We add slots for `OnX`-type keywords to object types where applicable. The relevant keywords are:

* `OnActivate`
* `OnDeath`
* `OnKnockout`
* `OnMurder`
* `OnPCAdd`
* `OnPCDrop`
* `OnPcEquip`
* `OnHitMe`
* `OnPCRepair`
* `OnPCSoulGemUse`
* `OnRepair`

### Custom Slots

We may allow the addition of custom slots (defined by the content developer), though this is an advanced feature more likely to be implemented in stage 2. Further details need to be explored.

## Namespaces

The namespace of a script is determined by its ID. For example a script with the ID `foo::Bar` would be placed in the namespace `foo`.

IDs in a script refer to the local namespace by default, meaning the ID `a` in the script `b::c` would refer to `b::a` if such an ID exists, or to `::a` otherwise.

## Other Script Instructions

This is a collection of script instructions that fit nowhere else:

* `GetPcTarget`: Returns the reference of the instance the player is currently looking at (crosshair); can be a null reference
* `GetMultiplayer`: Always returns 0
* `GetPlayers`: Returns a list of references to all player-controlled instances; this list contains a single reference to the instance of object `Player`

## Other Script Hooks

This is a collection of script hooks that fit nowhere else:

* `sys::NewGameStarted`: Executed when a new game is started

# Cells, Worldspaces & Areas

## Interior vs. Exterior

The distinction between interior and exterior cells stopped making sense with the release of the Tribunal add-on. Therefore, we should drop this terminology and replace it with **Unpaged Cells** (interior) and **Paged Cells** (exterior).

## Unpaged Cells

Unpaged cells take the place of old interior cells. The only difference between old interior cells and unpaged cells is the worldspace.

By default, each unpaged cell has an associated worldspace record with the same ID. Furthermore, the unpaged-cell record is enhanced by an optional subrecord that specifies an alternative worldspace (if present).

Most of the cell configuration data is moved out of the cell record and into the worldspace record.

By moving out the worldspace into a separate record, we can unify worldspace data between interior and exterior cells. We can also associate multiple interior cells with the same worldspace or interior cells with an exterior worldspace. This should reduce duplicated data significantly.

## Paged Cells

Paged cells remain essentially unchanged from old exterior cells, the only major difference being the ID and the worldspace.

Currently, there is no uniform ID scheme for cells. Interior cells are named via an ID, exterior cells are named via a cell index (x, y). This needs to be changed, since we need a more consistent way to address cells.

The editor already gives exterior cells an ID (currently in the form of `#x,y`). The new scheme (relevant at least for the editor and scripting) will be `worldspace::x,y` (the cell will have the ID `x,y` in the namespace `worldspace`). We may also consider to replace the two integer coordinates in the cell record with the string ID. The performance impact of this change should be insignificant.

The default exterior worldspace will be called `default` (previously called `sys::default`). Therefore, the old exterior cell 0, 0 will be called `default:0,0`. We are moving `default` out of `sys` to avoid creating an exception to the rule that content files are not allowed to create records in `sys`.

## Worldspace Records

Worldspace records are shared by paged and unpaged cells (i.e., there are no interior and exterior worldspaces).

Worldspace records contain the following fields:

* Water: Default Water Level, Water ID (see de-hardcoding). **Note**: Script instructions that modify the water level will now modify the water level of a worldspace instead the one of a cell.
* Ambient Light: 4 values
* Sky: Sky ID (analogous to Water ID, most likely not part of stage 1). **Note**: A worldspace cannot have both an ambient-light and a sky subrecord.
* Terrain: No data (might be better implemented as a flag); presence indicates that worldspace has terrain; ignored by unpaged cells

All fields are optional. If a field is missing, the respective cell/worldspace element is not present.

## References to Cells in Data Structures

Vanilla references to cells are generally based on the ID of the cell. There are no fields in vanilla Morrowind's data structures that reference individual exterior cells.

We keep these reference fields, but change their meaning:

1. Check whether it is the ID of an unpaged cell. If yes, use this cell.
2. Check whether it is a worldspace. If yes, use the paged cells in this worldspace.
3. Otherwise, error.

## Scripts

### Water Level

The existing script instructions that deal with water level can be safely extended to the new system. We will add an optional argument at the end of the argument list of each instruction (`GetWaterLevel`, `ModWaterLevel`, and `SetWaterLevel`) which specifies the worldspace the instruction is acting on. If the argument is missing, the instruction affects the current worldspace instead.

**Note**: This behaviour is different from vanilla Morrowind in regards to exterior cells.

### New Instructions

We add the following script instructions:

* `EnumerateActiveCells`: Returns a `StringList` containing the IDs of all cells currently active; paged cells are listed individually
* `GetWorldspace(c)`: Returns the ID of the worldspace the cell with the ID `c` is in
* `GetWater(w)`: Returns the ID of the water used in the worldspace with the ID `w`
* `GetSky(w)`: Returns the ID of the sky used in the worldspace with the ID `w`

### Script hooks

We add four new script hooks:

* `RegionEntered`
* `RegionExited`
* `WorldspaceEntered`
* `WorldspaceExited`

All four hooks take the ID of the region or worldspace as arguments and are executed when the player enters or exits a region or a worldspace.

## Areas

The ability to designate a section of a worldspace with a specific ID that can be checked or referenced has many uses. A few examples:

* Give an NPC certain dialogue topics only in specific places
* Limit wandering NPCs to an area
* Add special/magical effects to a location

Currently, our ability to do that is largely limited to cells. This is a problem because of two reasons:

* The fixed-size, square nature of cells makes areas unreasonably inflexible.
* We can't have overlapping areas.

### Record

We introduce a new record ("Area") that consists of the following fields:

* Worldspace ID
* Polygon: A list of 2D coordinates defining a surface on the xy-plane
* Min-Height: z-coordinate at which the area starts
* Max-Height: z-coordinate at which the area ends
* Enter script ID (string, optional): Script function that is called when the player enters the area
* Exit script ID (string, optional): Script function that is called when the player exits the area (must also be called in case of teleportation)
* Inside script ID (string, optional): Script function that is called while the player is in the area.
* Inside script delta (float, optional): Minimum time between two executions of the inside-script function.
* Composite (integer, optional): If this flag is set, the area will be ignored by OpenMW except as a part of a joined area. **Note**: Composite areas are not accessible by script instructions and do not run any scripts of their own.

All script functions take the ID of the area as a parameter.

Whenever a cell or a worldspace is referenced to check or define locations, the ID of an area can be used instead.

### Script Functions

We add the following script functions:

* `GetAreas(v3)`: Returns a `StringList` containing the IDs of all areas `v3` is in
* `InArea(id)`: Returns 1 (instance is in area `id`) or 0 (otherwise)

## Joined Areas

A joined area is an area that consists of multiple unjoined areas. Joined areas can contain both composite and non-composite areas. A joined area can stretch across multiple worldspaces. Scripts do not distinguish between areas and joined areas.

### Record

We introduce a new record ("Joined Area") that consists of the following fields:

* Enter script ID (string, optional): Script function that is called when the player enters the area
* Exit script ID (string, optional): Script function that is called when the player exits the area (must also be called in case of teleportation)
* Inside script ID (string, optional): Script function that is called while the player is in the area.
* Inside script delta (float, optional): Minimum time between two executions of the inside-script function.

# Item Interaction & Item Management

## Deletion

We will add an instruction to delete instances via a reference variable. Current solutions are insufficient because they can not target specific items (when in a container) and require different approaches depending on whether an instance is in a container or in the world. Self-deletion must be safe. For the sake of consistency, we extend this instruction to work on non-item objects too.

* `Delete` (reference specification for instance to be deleted via the `->` operator in the usual way)

## Container

We will add a function that returns the contents of a container as a list of references. **Note**: In this case, the term "container" also includes creatures and NPCs.

* `GetContents` (reference specification for container via the `->` operator in the usual way)

We will add a function that moves/clones an item instance into a container (actual container or actor). This feature (especially the `move` function) preludes a concept of persistent instance identity, which is most likely a stage-2 or OpenMW-2.0 feature. For now, these functions are required to deal with state attached to the instance in the form of record variables / old local variables. (In all cases, the reference specification for the item to be moved/cloned works via the `->` operator in the usual way.)

* `MoveItemToContainer(ref/id, count = 1)`: Move item from current location to container specified by `ref/id`
* `CloneItemToContainer(ref/id, count = 1)`: Add a copy of item to container specified by `ref/id`

The `count` argument is ignored when the original item is not in a container.

## Other Item-Related Instructions

Instructions:

* `SetItemHealth(health)`: Sets item's current health
* `SetItemCharge(charge)`: Sets item's current charge
* `SetItemOwner(owner)`: Sets item's owner ID
* `SetItemSoul(soul)`: Sets item's soul ID (soul gems only)
* `SetItemFaction(faction)`: Sets item's faction ID
* `SetItemFactionRank(rank)`: Sets item's faction rank

Functions:

* `IsItem`: Returns 1 if reference is an item, 0 otherwise
* `GetItemHealth`: Returns item's current health
* `GetItemMaxHealth`: Returns item's maximum health
* `GetItemCharge`: Returns item's current charge
* `GetItemMaxCharge`: Returns item's maximum charge
* `GetItemOwner`: Returns item's owner ID
* `GetSoulItem`: Returns item's soul ID (soul gems only)
* `GetItemFaction`: Returns item's faction ID
* `GetItemFactionRank`: Returns item's faction rank

## Item Tags

Currently, there is no customisable way of categorising items. To compensate for this shortcoming, we introduce item tags.

An item tag is a string that is attached to an object record. An object record can have multiple tags.

Objects also have implicit item tags that are determined by their type (e.g., every weapon object has a tag "Weapon" even without the object record explicitly containing this tag). We may introduce other kinds of implicit tags (e.g. weapon types, enchanted items).

The de-hardcoding introduces additional item tags.

Item tags are immutable at runtime and can be queried via script instructions:

* `GetItemTags(id)`: Returns a `StringList` of tags for the object `id`; as usual, instead of an ID a reference variable can be used
* `HasitemTag(id, tag)`: Returns 1 or 0 depending on `id` having the tag `tag`

Using these instructions on non-item objects returns an empty list or 0 respectively.

Item tags can be used to organise container windows (see *GUI* section).

A few examples of tags that content developers may come up with:

* "Quest": A quest item
* "Vendor": An item that has no other use than to sell it to a vendor

We may suggest some default tags in the documentation (or even the editor itself) to encourage more consistent tag use by content developers.

## Interactions

We enhance the way how the player interacts with the world, especially via items.

### Methods of Interactions

There are three methods of interaction:

* The player presses the attack button while holding an interaction item in his hand and targeting an object in the world. This feature exists in vanilla Morrowind only within the Security skill. We generalise this kind of interaction (see *Held Items* in the *De-Hardcoding* section). We also allow this feature for weapons held in the main hand: If an interaction is possible, the interaction takes the place of the attack. If the interaction is ignored or refused, the attack proceeds.
* The player drags an item from a container onto an instance in the world. This kind of interaction does not exist in vanilla Morrowind.
* The player drags an item from a container onto another item in another (or the same) container. This kind of interaction does not exist in vanilla Morrowind.

### Interaction Subrecords

All item-object record types are enhanced by a new optional subrecord that holds the name of the ItemItemInteraction script.

All object record types (including items) are enhanced by a new optional subrecord that holds the name of the ItemObjectInteraction script.

### Interaction Chain

Custom interactions are handled by running through a sequence of scripts. Each script can either ignore the interaction, accept it, or refuse it.

If the action is accepted or refused, the chain stops. If the action is refused or the chain runs to its end without any script accepting it, the player will be notified via a sound effect. We may either re-purpose an existing sound effect or acquire a new one.

We add two new script instructions:

* `AcceptInteraction`
* `RefuseInteraction`

Using any of these instructions outside of an interaction chain is an error. We use these instructions instead of return values, because a part of the interaction chain works via hooks which do not provide return values.

All interaction chain functions share the following signature:

* reference to item A
* reference to item/object B
* reference to actor performing the interaction (player for now, we may extend that later)
* integer: 0 if interaction was initiated via drag & drop, 1 if interaction was initiated via attack button

### Item-Item Chain:

* Hook `sys::PreItemItemInteraction`
* `ItemItemInteraction script of item A
* `ItemItemInteraction script of item B
* Hook `sys::ItemItemInteraction`

### Item-Object Chain:

If the object is also an item the Item-Item-Chain is used instead.

* Hook `sys::PreItemObjectInteraction`
* `ItemObjectInteraction` script of item A
* `ItemObjectInteraction` script of object B
* Hook `sys::ItemObjectInteraction`

# De-Hardcoding

This section describes the first batch of de-hardcoding tasks. This is the core part of stage 1 (*The Grand De-Hardcoding*). We are aiming mostly for low-hanging, but highly profitable fruits here. More complex de-hardcoding (especially tasks that require more extensive script support) will follow in stage 2.

## GMSTs

Many GMSTs will be integrated into other records, which will effectively remove the GMST. We may consider actually removing the GMST record during the load process.

We will add three new types of GMSTs:

* `IntegerList`
* `FloatList`
* `StringList`

We may consider allowing content files to create new GMSTs outside of the `sys` namespace. This would make the GMST record type more consistent with other record types. To make this addition useful, we need to add script functions to read GMSTs:

* `Get{Type}Gmst()`

`{Type}` can be of type `long`, `float`, `String`, `Longlist`, `FloatList`, or `StringList`.

## Fallback Values

The *openmw.cfg* file contains a set of fallback values. These were extracted from the *Morrowind.ini* file. As the name indicates, we consider these values as a fallback for legacy-format content files only. Our goal is to move all these values to content file format. In some cases, we may also decide to declare a fallback value obsolete and not use it at all.

All usable values should be migrated to new GMST records, unless they are already covered by other parts of the de-hardcoding.

## General Scripting Enhancements

We will introduce new script functions:

* `Enumerate{x}`: Returns a `StringList` of the IDs of all records of the respective type; `{x}` is either "Skills", "Races", "Classes", "DynamicStats", "Attributes", "WeaponTypes", "ArmorTypes", "Specialisations", "MagicSchools", "Factions" or "Birthsigns"

## Dynamic Stats

We will unify dynamic stats and make them configurable. For stage 1, we will not allow the deletion of existing dynamic stats (they are used in too many parts of the engine). But we will allow new dynamic stats.

We add a new record type for dynamic stats. Records for Health (`sys::Health`), Fatigue (`sys::Fatigue`), and Magicka (`sys::Magicka`) will be created by the engine when loading older omwgame files (these records are mandatory for newer omwgame files).

A dynamic-stat record contains the following information:

* Name (taken from GMSTs for default stats)
* Tooltip Text (taken from GMSTs for default stats)
* Colour
* (optional) PrevID: ID of the stat the engine will try to subsequently sort this stat in (similar to info records)
* (optional) NextID: ID of the stat the engine will try to antecedently sort this stat in (similar to info records)
* (optional) Vital: Flag which specifies whether the player dies if the stat drops to 0
* (optional) Level-Up Function: ID of the stat's level-up function, which takes the reference of the character as argument and returns an `long` value representing the maximum stat change achievable with a level-up
* (optional) Time-Passed Function: ID of the stat's time-passed functionm, which takes the reference of the character and the duration as arguments and returns a `float` that is used to modify the current stat value (the function is not used when waiting or sleeping)
* (optional) Wait Function: ID of the stat's wait function, which takes the reference of the character, the duration, and a sleep flag (integer) as arguments and returns a `float` that is used to modify the current stat value
* Default Value: Value that is used as the maximum value of the stat if an actor does not have this stat (possible if the stat was defined in an add-on and not in the base game)

Scripts for default stats are injected as necessary. Some of these scripts require access to GMST values. We need to figure out how to implement this: either grab the value from the GMST when adding the script or adding a script function to read GMST values. The latter is easier but will not allow us to remove the redundant GMST records easily.

The currently existing `Get`, `Set`, and `Mod` instructions are flagged as deprecated and replaced by a single instruction for each operation that takes the stat ID as an argument. We need separate instructions to deal with base, modified and current values.

## Attributes

We will unify attributes and make them configurable. For stage 1 we will not allow the deletion of existing attributes (they are used in too many parts of the engine). But we will allow new attributes.

We add a new record type for attributes. Records for the existing attributes (`sys::NameOfAttribute`) will be created by the engine when loading older omwgame files (these records are mandatory for newer omwgame files).

An attribute record contains the following information:

* Name (taken from GMSTs for default attributes)
* Tooltip Text (taken from GMSTs for default attributes)
* (optional) PrevID: ID of the attribute the engine will try to subsequently sort this attribute in (similar to info records)
* (optional) NextID: ID of the attribute the engine will try to antecedently sort this attribute in (similar to info records)
* Default Value: Value that is used as the maximum value of the attribute if an actor does not have this attribute (possible if the attribute was defined in an add-on and not in the base game)

**Note**: All records that reference attributes need the respective fields to be changed from integers to strings.

The currently existing `Get`, `Set`, and `Mod` instructions are flagged as deprecated and replaced by a single instruction for each operation that takes the attribute ID as an argument. We need separate instructions to deal with base, modified and current values.

Additionally, we will add a new GMST `sys::ClassAttributes` of type `long`. This GMST specifies the number of favoured attributes that a class has. The value defaults to 2.

## Weapon Types

We will unify weapon types and make them configurable.

We add a new record type for weapon types. Records for the existing weapon types (`sys::NameOfType`) will be created by the engine when loading older omwgame files.

A weapon type record contains the following information:

* Name (taken from GMSTs for default types)
* Handling Type: Defines how the weapon is held and what animations play when attacking
* Hit Test Script: Name of the hit test script used (see *Combat* subsection)

For stage 1, the weapon type record is still very basic. Its purpose is mostly to allow better de-hardcoding of skills. We may expand on this in the future. However, there are already possible uses, e.g., a weapon type "Ancient Blades" which requires a separate skill.

We add a new script function:

* `GetWeaponTypeId id`: Returns the weapon type ID of an instance or an object with the ID `id`

**Note**: `GetWeaponType` is already taken by Tribunal and we can not repurpose this name without breaking compatibility. 

## Armour Types

In vanilla Morrowind, armour types exist only implicitly. There is no record and no option to select armour types. Armour types are determined by weight only.

We will keep implicit armour types as an option, but we'll also add armour types explicitly as a new type of record.

Records for the existing types (`sys::LightArmorType`, `sys::MediumArmorType`, `sys::HeavyArmorType`) will be created by the engine when loading older omwgame files (these records are mandatory for newer omwgame files).

An armour type record contains the following information:

* Name (taken from GMSTs for default types)
* (optional) Minimum weight

Additionally, armour object records are extended by the optional field "Armour type".

If an armour object has no armour type specified, its type is determined by its weight according to the following algorithm:

1. Consider all armour types with a minimum weight field.
2. Exclude types that have a minimum weight larger than the weight of the object.
3. Pick the type with the largest minimum weight.

For stage 1, the armour type record is still very basic. Its purpose is mostly to allow better de-hardcoding of skills. We may expand on this in the future. However, there are already possible uses, e.g., imitating TES IV: Oblivion by eliminating medium armour or adding a new armour type such as "Ancient Armour" which requires a separate skill.

We add another script function:

* `GetArmorTypeId id`: Returns the armour type ID of an instance or an object with the ID `id`

**Note**: For the sake of consistency, we've adopted the naming scheme introduced for weapon types.

## Specialisation

We will unify specialisations and make them configurable.

We add a new record type for specialisations. Records for Combat (`sys::CombatSpec`), Stealth (`sys::StealthSpec`), and Magic (`sys::MagicSpec`) will be created by the engine when loading older omwgame files.

A specialisation record contains the following information:

* Name (taken from GMSTs for default types)

We add a new script instruction:

* `GetSpecialization id`: Returns the specialisation of a class or a skill with the given ID `id`

## Magic Schools

We will unify magic schools and make them configurable.

We add a new record type for magic schools. Records for the existing magic schools (`sys::AlterationMagicSchool`, `sys::ConjurationMagicSchool`, etc.) will be created by the engine when loading older omwgame files.

A magic school record contains the following information:

* Name (taken from GMSTs for default schools)
* Specialisation (`sys::MagicSpec` for default schools)
* Resource (`sys::Magicka` for default schools)

A use for the specialisation field would be, e.g., dividing magic into the two categories "Divine Magic" and "Arcane Magic" and having characters specialise accordingly.

## Skills

Skills are a complex topic and we won't achieve full de-hardcoding in stage 1 - and most likely there will never be a 100% complete de-hardcoding. But there is still a lot we can do with skills.

Currently, skills exist as indexed records. We need to move these over to ID-based records. The exiting indices are translated to IDs of the form `sys::NameOfSkill`.

We add a new subrecord type (see subsection *Function Subrecords* below). Each skill can have zero, one, or multiple function subrecords.

The following skills are too tightly bound into the engine to allow their easy removal:

* Acrobatics
* Alchemy
* Armorer
* Athletics
* Block
* Enchant
* Hand-to-hand
* Mercantile
* Sneak
* Speechcraft
* Unarmored

Therefore, we will (for now) forbid the deletion of the respective skill records. All other default skills can be deleted.

### Script Instructions

The currently existing `Get`, `Set`, and `Mod` instructions are flagged as deprecated and replaced by a single instruction for each operation that takes the skill ID as an argument. We need separate instructions to deal with base and modified values.

We also introduce new script instructions:

* `UseSkill id, v`: Progresses the skill `id` by a value of `v` (`float`).
* `UseValueSkill Id, i`: Progresses the skill `id` by the value given by the use-value field with index `i` (0, 1, 2 or 3)
* `Get{x}SkillLevel s_id`: Returns the skill level of a function subrecord with `TargetId` `s_id` (`String`). `{x}` is either "Armor", "Weapon", or "Magic"; see the next subsection for details.

**Note**: We do not de-hardcode the fixed number of use-value fields. The usefulness of these fields for new skills is most likely limited. The `UseSkill` instruction, which bypasses the use-value fields, should be sufficient in most cases. We only add the `UseValueSkill` instructions to better support script interactions with default skills.

### Function Subrecords

A function subrecord describes how a skill interacts with hardcoded engine functions. A function record consists of a function ID (we use an index here, because the list of functions will definitely not be extensible by content files) and additional arguments.

For stage 1, we require only one additional argument:

* `TargetId` (single `String` value)

Stage 1 will introduce three function IDs:

* Armour Skill
* Weapon Skill
* Magic Skill

We do not forbid non-unique function subrecords, i.e., two skills may have identical function subrecords, e.g., two skills governing light armour. If there is more than one skill with the same function subrecord and the skill level for this function needs to be considered, we use the maximum of all relevant skill levels.

### Armour Skills

We make armour skills fully customisable by introducing a function ID for armour types. Any skill that has a function subrecord with this ID is an armour skill. For default armour skills loaded from older omwgame files, we will inject function subrecords during the loading process.

The `TargetId` of an armour function subrecord indicates the armour type that the skill is governing. 

The default skills are:

* "Heavy Armor"
* "Light Armor"
* "Medium Armor"

### Weapon Skills

We make weapon skills fully customisable by introducing a function ID for weapon types. Any skill that has a function subrecord with this ID is a weapon skill. For default weapon skills loaded from older omwgame files, we will inject function subrecords during the loading process.

The `TargetId` of a weapon function subrecord indicates the weapon type that the skill is governing. 

The default skills are:

* "Axe"
* "Blunt Weapon"
* "Long Blade"
* "Marksman"
* "Short Blade"
* "Spear"

### Magic Skills

We make magic skills fully customisable by introducing a function ID for magic schools. Any skill that has a function subrecord with this ID is a magic skill. For default magic skills loaded from older omwgame files, we will inject function subrecords during the loading process.

The `TargetId` of a magic school function subrecord indicates the magic school that the skill is governing. 

The default skills are:

* "Alteration"
* "Conjuration"
* "Destruction"
* "Illusion"
* "Mysticism"
* "Restoration"

## Weather

We make weather types customisable by moving from hardcoded, index-based weather effects to ID-based weather types stored in content files. To achieve this, we introduce a new record type ("Weather"). When loading older omwgame files, we will inject weather records for the ten default weather types (`sys::WeatherAsh`, `sys::WeatherBlight`, etc.).

A weather record is made up of subrecords, each describing a weather effect. A weather record can have any number of weather effect subrecords.

There are four types of weather effect subrecords:

* "Magic Effect": A magic effect that is applied to all actors within active cells while the respective weather type is active
* "Sky": Modifications to the visuals of the sky (e.g, clouds)
* "Particles": Particle effects (e.g., rain)
* "Event": Events that can happen with a certain probability while the weather type is active (e.g., lightning); events happen at random locations

Magic effect subrecords contain the ID of the respective magic effect. All other subrecords contain an integer ID specifying the effect and (if necessary) additional parameters.

All effects described in these subrecords affect active cells only.

### Sky

TODO:
- Should cover all existing sky effects and possibly new ones

### Particles

TODO:
- Should cover all existing particle effects and possibly new ones

### Event

We will add the following event IDs:

* "0": Scripted event; additional data consists of the name of the script to be run when the event is triggered; the script takes the following arguments: worldspace (`String`), location (`Vector3`), weather ID (`String`)
* "1": Lightning

In addition to the event ID and event-ID-specific data, event subrecords have an "Event Trigger" block:

* Chance: Floating number in the range between 0 and 1; the chance per unit of time that the event happens (we may want to make the unit of time globally configurable with a new GMST)
* (optional) Min-Distance: The minimum distance from the player at which the effect can take place (0 by default)
* (optional) Max-Distance: The maximum distance from the player at which the effect can take place (infinite by default)

### Scripts

We need to introduce new script instructions regarding the weather, since the existing instructions are based on a fixed number of weather types or integer-indexed weather types. These instructions can not be salvaged and we therefore declare them deprecated.

New script instructions:

* `SetWeather(regionId, weatherId)`: Sets the current weather for the region with the ID `regionId` to `weatherId` (replaces `ChangeWeather`)
* `GetWeather(regionId)`: Returns the current weather ID for the region with the ID `regionId` (replaces `GetCurrentWeather`)
* `SetRegionWeather(regionId, weatherId, chance)`: Modifies the entry `weatherId` in the weather table of the region with the ID `regionId`; `chance` is of type `long`; we relax the requirement that the sum of the chance values needs to be 100 (replaces `ModRegion`)
* `UpdateRegionWeather(regionId)`: Forces a new roll on the weather table of the region with the ID `regionId`

## Water

We currently have but a single water type that is hardcoded. We name this water type `sys::Water` and inject a suitable record of the new type (see below) during loading of older omwgame files.

We generalise the concept of water to the concept of liquid by introducing a new record type ("Liquid"). Liquid records can be used both for different-looking water types (e.g., more swampy water, water with different colouration) but also for completely different liquid types (e.g. lava).

**Note**: Morrowind only uses one liquid type, which we refer to as "water". (While lava does exist in the game, it is handled with activators, rather than as a liquid type)

To support the water type `sys::Water`, we also need to add a new magic effect (`sys::SuffocationEffect`).

Liquid records are referenced both in worldspace records (see *Cells, Worldspaces & Areas* section) and in the body of liquid records (see *Misc* section).

A liquid record consists of the following fields:

* Effects: IDs of zero or more magic effects which are applied to actors while in the liquid
* Submerged Effects: IDs of zero or more magic effects that are applied to actors while completely submerged in the liquid
* Liquid Type ID: `long`, hardcoded
* Additional parameters, specific to liquid type

We need to support at least one liquid type (ID 0: Water) from the start. Other types can be added.

TODO:
- Are there other ways to handle visuals for liquid types that require less hardcoding?
- Use of shaders?

## Magic Effects

Currently, we have a fixed number of magic effects that are referenced by an integer index.

We move over to effect records with a string-ID-based scheme.

We also introduce a new effect (`sys::SuffocationEffect`) by injecting a record when loading from older omwgame files. This effect triggers the normal procedure for running out of air.

**Note**: This will require creating a new effect icon, which will be stored internally along with other resources that are part of the game engine.

Effect records can be deleted without restrictions. A content developer can add new effect records - again without restrictions.

An effect record consists of the following fields:

* Name (currently stored in a GMST)
* (optional) Effect Function: Contains the ID of a script function that is called when the magic effect takes effect; the function takes two arguments: the reference of the effects source (this would be a spellcaster in case of a spell or an item in case of an enchantment) and a list of target references
* (optional) Wear-Off Function: Contains the ID of a script function that is called when the magic effect ends (only relevant for non-instant effects); the function takes the same two arguments as the "Effect Function" field

For the existing effects, we will inject scripts that match the previously hardcoded effects when loading from an older omwgame file. This will require the addition of a significant number of new script instructions that are all trivial, since they will just call existing engine functions.

It is important to generalise as much as possible when creating these new script functions, e.g., we won't have a `DivineIntervention` script instruction. Instead we add a `GetClosestMarker` function, use existing functions for querying position and cell and, then, use the new `Teleport` instruction.

## Input

We allow the addition of customisable input functions that can be bound to keyboard buttons in the usual way and can trigger the execution of scripts. Existing input bindings are not affected by this.

To this end, we introduce a new record type ("Input"):

* Label
* (optional) Tooltip: `String` value
* Default Key: `long` value
* (optional) Key-Press Event Script: Contains the ID of a script that is executed when the key is pressed
* (optional) Key-Release Event Script: Contains the ID of a script that is executed when the key is released
* (optional) Key-Hold Event Script: Contains the ID of a script that is executed when the key is pressed and held
* Key-Hold Delta: `float` value describing the minimum time between two executions of the "Key-Hold Event Script"; defaults to a reasonable value

## Held Items

We merge the probe and lockpick object types into a new object type: "Held Item". The Security-skill-related functions are handled via the `ItemObjectInteraction` script subrecord. When loading older omwgame files, a suitable script in the `sys` namespace is injected for probing and lock picking each. When transforming probe and lockpick records into held-item records, an `ItemObjectInteraction` subrecord referencing the respective script is injected.

**Note**: If we enhance the animation system, we may need to add additional subrecords that specify the pose for holding the item or the animation for using it.

## Pickpocketing

We add a new GMST (`sys::ReversePickpocketing`) of type `long` that indicates if reverse pickpocketing is allowed (value other than 0) or not (value 0).

We move the function for checking if pickpocketing succeeded from the C++ code to a new script function (`sys::PickpocketTest`). We inject this function when loading older omwgame files.

The function takes the following arguments:

* thiefId: Reference to the thief (actor)
* victimId: Reference to the victim (actor)
* stolenItemId: Reference to the item to be stolen
* reversePickpocket: `long` flag; set to 0 for regular pickpocketing, set to 1 for reverse pickpocketing

The function returns 1 if the pickpocket attempt succeeded and 0 if it failed.

Items with the `noPick` tag are excluded from pickpocketing.

## Combat

De-hardcoding combat is a monumental task that we can not hope to complete within the time frame of stage 1. We will de-hardcode some parts that do not depend on large-scale improvements to animation and can be handled with the scripting improvements in stage 1. Any further enhancements are left for stage 2 or, more likely, OpenMW 2.0.

### Hits

We move the hit test check from C++ code to script functions. The following functions will be used:

* `<NameOfMeleeHitTest>(weaponId, targetId)`: Function for melee weapons defined in the corresponding WeaponType record; `weaponId` refers to the weapon used, `targetId` refers to the ID of the target
* `<NameOfRangedHitTest>(weaponId, targetId, ammunitionId)`: Function for ramged weapons defined in the corresponding WeaponType record; `weaponId` refers to the weapon used and `targetId` refers to the ID of the target, and `ammunitionId` refers to the ammunition used by the ranged weapon
* `sys::UnarmedHitTest(weaponId, targetId)`: Default function for unarmed attacks; `weaponId` refers to the weapon used and `targetId` refers to the ID of the target

All functions return 0 if the attack misses and a value other than 0 on a successful hit.

`sys::UnarmedHitTest` and default functions for armed-hit tests (`sys::MeleeHitTest` and `sys::RangedHitTest`) are injected when loading older omwgame files.

### Script Enhancements

We add a new script function:

* `GetTargetRef`: Returns the current target of an actor; returns a null reference if the actor is not in combat or if it is used on an instance that is not an actor

We add two new script instructions:

* `DeclareWar l`: Similar to `StartCombat`, but specifies a whole group of enemies; `l` is a list of references
* `DeclarePeace l`: Ends the hostilities resulting from previous combat towards the actors listed in the reference list `l`. **Note**: We may need to add a grace period after this instruction during which attacks do not cause hostility again, so we can handle non-instant attacks.

### Script Hooks

We add three new script hooks:

* `sys::Kill`: Executed whenever an actor is killed; scripts for this hook take the following two arguments: reference to the target and reference to the actor who killed the target (may be a null reference)
* `sys::CombatStarted`: Executed when the player enters combat
* `sys::CombatEnded`: Executed when the player exits combat

## Music

### Playlists

We add a new record type: PlayList

The playlist record has the following fields:

* Probability: Float [0.0-1.0]
* Music Directory (optional): String
* Title (can appear more than once): String

The titles on the playlist are all the titles in the music directory and the titles listed explicitly.

When playing a playlist, first the probability is considered to randomly decide if a track is played or not. Then a track from the list is chosen.

If a playlist is played in loop mode the process above is repeated whenever:

* The current track ends
* No track is playing at reasonably chosen time intervals

### Background and Event Music

We distinguish between two types of music:

* Background Music
* Event Music

Background music is music that (potentially) runs all the time, unless explicitly stopped. Background music is always looping.

Event music is played only on certain events. Event music can be played looped or only a single time. In the former case event music needs to be stopped explicitly. Once event music ends background music is resumed exactly where it was interrupted.

Background and event music persists through save-load-cycles.

### Script Enhancements

We add several new script instructions:

* PlayBackgroundMusic (t)
* PlaylistBackgroundMusic (pl)
* StopBackgroundMusic
* PlayEventMusic (t, priority, loop=0, force=0)
* PlaylistEventMusic (pl, priority, loop=0, force=0)
* StopEventMusic (priority)

Arguments:

* t (string): a single title (specified by its location in the data directory)
* pl (string): a playlist (specified by its ID)
* loop (integer): looping or not
* force (integer): Terminate any other running event music first. If this flag is 0 and there is already event music playing the new event music is ignored. Event music provided by content developers will usually not set this flag.
* priority (integer): Priority for event music. If there are multiple instances of active event music only the one with the highest priority will play. There can only ever be one instance of event music for any priority level. If event music of higher priority stops and there is event music of lower priority the event music of lower priority starts to play.

Note: An attempt to play a track or playlist that is currently playing is ignored. In this case the track or playlist does not restart. Changes in looping behaviour are considered though.

New games starting with a new format omwgame file can use priorities as the developer sees fit. For old games where automatic injection of scripts and music records take place we define the following priorities:

* 10: Combat music
* 0: Basic event music

### Transition of Existing System

We completely replace the old music system with a new script-based one. The StreamMusic script instruction is flagged as deprecated.

New records are injected when loading from an older omwgame file.

We add two playlists (sys::ExplorePlaylist and sys::CombatPlaylist) which are pointed to the usual directories and play with a probability of 1.0.

We add a GMST (sys::DefaultBackgroundMusic) with a default value of "sys::ExplorePlaylist". Whenever starting a new game or loading a game with no background music running the playlist specified in this GMST is played as background music. If the GMST holds an empty string no background music is played by default.

We add a script to the sys::CombatStarted and sys::CombatEnded hook each. The former plays sys::CombatPlaylist looped and forced. The latter stops event music.

### Location-Based Background Music

We add an optional string sub-record to the records listed below. This field contains a single track or a playlist that determine the background music for the respective location.

* Worldspace
* Region
* Area
* Cell

More specific entries (towards the bottom of the list) take precedence over more general entries. Global background music (controlled via script instructions) has the lowest priority.

## Character Creation

The process of character creation currently allows for very little customisation. We redo the character creation process with the following goals in mind:

* Allow more control over how the individual steps are chained together
* Allow additional steps (both future hardcoded enhancements like additional visual customisation and custom steps entirely defined in content files)
* Allow replacement of existing steps with alternative methods

Currently the character creation process runs through these steps:

1. Choose name
2. Choose race and appearance
3. Choose a method for class selection and perform class selection (3 methods)
4. Choose birthsign
5. Review

### Chaining

We add two new GMSTs to control how the steps of character creation are chained together:

* sys::NewCharBack: Integer; allow going back to previous steps (not being able to go back also implies that there is no going forward)
* sys::NewCharSteps: String-List: List of scripts associated with the steps of the character creation process. The scripts are listed ordered according to the sequence they are executed in.

Each character creation step has a an associated script that is called when the step is reached (either via a EnableX script instruction or via the player going back or forth).

### Script Instructions

We add several new script instructions.

* CharacterCreationRun (n): Opens the respective GUI/window
* SetPlayerName (name)
* SetPlayerRace (id)
* SetPlayerFace (id)
* SetPlayerHair (id)
* SetPlayerSex (i): 0 or 1
* SetPlayerClass (id)
* SetPlayerBirthsign (id)
* CharacterCreationBack: Go back to previous step
* CharacterCreationNext: Go to next step (ignored if the next step had not been accessed before)

n is an integer argument with one of several hardcoded integer values:

* 0: Name GUI
* 1: Race GUI
* 2: Class selection via picking an existing class
* 3: Class selection via creating a class
* 4: Birthsign GUI
* 5: Review GUI

### Transition of Existing System

When loading older omwgame files we inject two GMSTs (sys::NewCharBack and sys::NewCharSteps) as well as several scripts.

For each step we add a new script that consists of only a single line:

````
CharacterCreationRun n
````

where n is the appropriate index.

Only steps #3 will be handled differently than described above.

The class selection method GUI can be implemented entirely with a message box. Therefore we scrap the hard-coded method selection window entirely.

Likewise the character selection method of answering a set of quests can be implemented via a set of message boxes. Therefore we scrap this window too.

## Level-Up

The vanilla (character-)level-up system allows almost no modification. We keep the existing hard-coded system, but make it more configurable, and we also allow content developers to completely replace the existing system.

### State Unveiling

Checks for a level up are based on the skill progression since the last level up. In vanilla this state is not accessible to content developers or players.

We address this issue by turning these hidden variables into record variables attached to the player character. The variables have the type long and a name matching the ID of the skill, and are created on the fly whenever the engine detects a skill level up. Note that regular script instructions that change skill level do not count towards level ups.

We also add another record variable (LevelUpCount) that counts the number of major and minor skill level-ups.

By binding these stats to a character instance instead of using global stats we are also making preparations for optional future level up mechanics for NPCs (companions or otherwise). This is most likely a task for stage 2 though.

### Trainers

Some NPCs are flagged as skill trainers. These trainers can train the player in their three best skills. Skill levels gained via training count towards level-up. We make the following enhancements to this mechanism:

* New GMST (sys::SkillTrainCount, integer): Replaces the hard-coded value of 3 skills.
* New script (sys::SkillTrainingTest): Tests if NPC can train player in a skill (returns 0 or 1). This test is performed after the faction standing test and after the skill list is trimmed down via sys::SkillTrainCount. The function takes the following arguments: ref to player, ref to NPC, ID of skill
* New script (sys::SkillTrainingApplied): Executed after training has been completed. Arguments are the same as with sys::SkillTrainingTest.

When loading from an old omwgame file, GMST and scripts are injected. sys::SkillTrainCount defaults to 3, sys::SkillTrainingTest performs the usual tests on skill level and attributes and sys::SkillTrainingApplied defaults to an empty script.

### Level-Up Test

The level-up test is performed when either the player rests (by default via T key) or when the ShowRestMenu function is called.

The test is performed by calling the function sys::LevelUpTest, which returns either 0 or 1. If 0 is returned no further action is taken. If a value other than 0 is returned the usual level-up procedure continues. Note that this function is allowed to initiate a custom level-up procedure and return 0.

The function takes two arguments:

* A ref to the player
* An integer that indicates what started the level-up test (-1 for resting)

We enhance the ShowRestMenu instruction with an optional long argument (defaults to 0) that is passed to the sys::LevelUpTest function as second argument.

When loading from an older omwgame file we inject a sys::LevelUpTest script that matches the hardcoded version of the level-up test.

### Level-Up Procedure

We keep the level-up procedure but replace several hardcoded values with GMSTs.

* Number of major skills: sys::MajorSkillCount (default: 5)
* Number of minor skills: sys::MinorSkillCount (default: 5)
* Number of attributes to increase during level-up: sys::AttributeLevelUp (default: 3)

We add a new function (sys::AttributeIncrease) that determines the number of points an attribute can be increased. The function takes a reference to the player and the ID of the attribute as argument and returns the number of attribute points. As usual we inject a matching function that matches the hardcoded implementation.

We add localisation records (see section about localisation) of the form default::LevelUpMessageX, where X is a positive integer number. When loading older omwgame files these records are injected based on level up fallback values. We use the default namespace instead of sys, because content developers need to be able to add their own records.

We add a script (sys::LevelUpFinished) that is called after the level-up procedure has been completed. The script takes a reference to the player character as its only argument.

The default implementation (injected when loading from an older omwgame file) resets the skill record variables and reduces the LevelUpCount variable by 10.

### Level-Up Image

The level up dialogue contains an image that depends on how the skill increase is distributed among specialisations. The function that decides about this image is very simple. However, since specialisations won't be hardcoded any more, we can not simply pass these values into the function without first establishing more involved script support than planned for stage 1.

This is actually not a problem since we can simply track the increase values in record variables, which may also prove useful for other purposes.

With this modification the C++ function (MWGui::LevelupDialog::getLevelupClassImage) can be directly translated into a script function (sys::LevelUpImage).

Note that this change does not allow for easy addition of new classes to the scheme. This is not a feature present in vanilla MW either. We may decide to investigate other options than simply translating the C++ function into a script function.

## Trading

### Pricing

The algorithm for trading (implemented in MWMechnics::Trading::haggle) can be translated straightforward into a script function once all stage 1 script enhancements are in place.

### Restocking Money

Currently the frequency of money restocking is controlled by a the GMST fBarterGoldResetDelay. Money is always reset to the original value.

We make all of these aspects configurable by adding a new field to all actor record types (NPC, creature) that contains a script ID.

The script takes a reference to the actor as its single argument.

When loading older omwgame files we inject a script (sys::UpdateGold) and inject this ID into all actor records that are flagged for merchant services. We also add a record variable (last restock).

The script checks the current time against the last restock variable and the GMST and resets the gold if necessary.

### Restocking Items

Currently items are restocked after every trade. Items in the merchant's inventory and in all containers owned by the merchant are restocked to their original count.

This process is complicated by the existence of levelled item lists, which makes it too complex to re-implement in a script.

We will make several changes to increase the flexibility of item restocking.

First we move the refill operation from after a trade is completed to before a trade is completed. This should not affect existing content.

We add three more scripts (optional, specified via new string fields in the actor's record) that hook into the refill process.

1. Check if a refill should be performed; called at the beginning of every trade cycle. Arguments: ref to actor, integer (1 if first trade since talking to the merchant, 0 otherwise). Returns an integer (0 no refill required, otherwise refill is required). If this script is not present, a refill is performed).
2. Called for every item that the refill algorithm flags for removal. Arguments: ref to actor, ref to item. Returns an integer (0 if item should be kept, otherwise item is removed). If this script is not present, the removal takes place.
3. Called at the end of refill procedure. This gives content authors the opportunity to make other modifications. Arguments: ref to actor. No return value.

## Fast-Travel

Morrowind offers a wide selection of fast travel options. Each of these fall into one of three categories:

1. Spells and teleport items
2. Travel to fixed locations via paying a travel NPC (boat, slit strider, gondola, guild guide)
3. The propylon chamber system

With vanilla Morrowind and other stage 1 changes #1 and #3 are already sufficiently configurable. #2 is not.

In stage 1 we will improve the flexibility of the travel network system and also its usability both for content creators and for players.

### Travel Network Record

We introduce a new record type (Travel Network) that consists of the following fields:

* Colour (integer)
* ID of Cost-Script (string): Takes three arguments (ref to travelling character, ID of source location record, ID of destination location record) and returns a long (number of coins). This is the value before bartering and followers are taken into consideration.
* ID of TravelTime-Script (string): Takes three arguments (ref to travelling character, ID of source location record, ID of destination location record) and returns a float (duration in hours)
* ID of TravelCompleted-Script (string, optional): Takes three arguments (ref to travelling character, ID of source location record, ID of destination location record). Executed when a character has finished travelling.
* Auto-Knowledge (integer, flag): Is the travel network fully known from the beginning of the game (!=0) or do locations need to be discovered to be usable (0).
* Visible (integer, flag): Is the travel network visible on the map?

### Travel Location Record

We introduce a new record type (Travel Location) that consists of the following fields:

* Travel Network ID
* Location worldspace (String)
* Location position and orientation (Vector4)
* Parent worldspace (String, optional)
* Parent position (Vector3, optional)
* List of IDs for travel locations that can be reached from this travel location

By formalising travel locations in this way we make it easier to modify travel locations later on (currently a modder would have to go through all travel NPCs of the same network, if he wanted to move a travel location) and to build visualisation features (see below).

### Travel Sub-records in NPC records

We keep the existing sub-record structures for travel since there is no automated way of transiting them to the new scheme. This task will be left to mod developers.

NPCs supporting the new scheme will have a single field that specifies the ID of the associated travel location record. 

### Transition of Existing System

When loading older omwgame files we inject suitable travel network records. Unfortunately there is no procedural method to distinguish between boat, slit strider and gondola, so these will all have to be grouped together into an sys::TravelOthers network. It will be up to mod developers to untangle these.
The travel network records will be created with the visible flag set to 0, because old-style travel data isn't available at load time and therefore visualisation can not work.

Travel locations can not be automatically transitioned to the new system. This task is left to mod developers.

### Block List

The layout of a travel network is determined by the data in the content files and can not be changed during the game. However we do offer several methods to configure a network during the game.

* Location Block List: Individual locations can be dynamically blocked, either for incoming or outgoing connections or both.
* Connection Block List: Connections between two locations can be blocked dynamically. This blocking is directed, meaning blocking the connection from A to B does not automatically block the connection from B to A.

### Visualisation

We offer visualisation somewhat similar to [this](http://en.uesp.net/wiki/Morrowind:Transport).

Each travel network has a colour associated with it that will be used to mark connections. If a travel location has a parent section, the parent section will be used instead to determine the location (e.g. exterior location for guild guides inside a building).

Only networks flagged as visible are drawn. We may add a GUI element that allows showing/hiding of visible networks either as a group or individually.

### Script Enhancements

We introduce several new script functions:

* GetTravelLocationWorldspace (id): Returns String
* GetTravelLocation (id): Returns Vector4
* GetTravelParentLocationWorldspace (id): Returns String
* GetTravelParentLocation (id): Returns Vector3
* GetTravelLocationList (id, ignore_blocked=0): Returns a list containing the IDs of all locations of the travel network id
* GetTravelDestinationList (id, ignore_blocked=0): Return list of IDs of destination travel locations for the travel location id

We also introduce several new script instructions:

* SetTravelVisibility (id, visibility): Sets the visibility (flag) of a travel network (id)
* RevealTravelLocation (id): Flag a travel location as known
* BlockTravelConnection (source-id, destination-id, blocked): Block or unblock a connection between two locations
* BlockTravelLocation (id, incoming-blocked, outgoing-blocked): Block a travel location.

# NPCs & AI

## Navmeshes

The current system of waypoints is limited. It is unlikely that we can improve it much. This makes pathfinding one of the few cases were we seriously need to consider completely abandoning an existing system in favour of a new one (navmeshes in this case).

## Waypoints

Waypoints can still be useful, but in a different way. Content developers will often designate a specific location for a NPC to stand at or walk to. Currently the easiest method to achieve this is to place an instance at the location, note down the coordinates and then enter them into a script or a field in a record. This is not an efficient approach and also causes maintainability issues, when a location that is used in multiple parts of a content file needs to be moved.

We introduce a new record type (Waypoint) with the following fields:

* Position (Vector3)
* Worldspace/Cell Id (String; cell takes precedence over worldspace)

We also add a new script function:

* GetWaypointPosition (id): Returns a Vector3

Waypoints are not part of the cell data structures because they need to be accessed independently of cells and we don't want to search through (potentially) every single cell every time a waypoint is referenced.

## NPC Schedules

NPC schedules are an advanced topic and as such would be a better fit for stage 2. However schedules are also a very iconic feature with a large impact on the playing experience. Therefore we will consider them for stage 1. The following is a preliminary proposal that is in need of more design work.

### Active and Passive Cells

We run schedules only in active cells. But we also need to consider passive cells.

Here are a few examples. For simplicity let's consider a one-dimensional cell grid with the usual pattern of one cell around of the player cell being active.

For this example let the player be in cell 0. Now consider NPC A, who is in cell 2 (not active). He has a schedule that takes him to cell 1 sometimes. As a player we would expect to see him from the distance then.

Another example with the same setup as above: While A is moving within cell 2 towards cell 1, the player relocates to cell 2. He would to see A make his way towards cell 1. Moreover if the player moves back to cell 0 and then returns to cell 2, he would expect to see A having made some progress.

Conclusion: We must track all actors whose schedules can take them into active cells.

It is not clear yet how to achieve this goal best. In the most simple implementation we could just estimate the time it takes to perform a task in a schedule and spawn the actor into active cells accordingly. This would however not cover the second example.

Alternatively we could invent a semi-active cell state and apply it to all cells that contain NPCs whose schedules can touch the active cells. This semi-active state would not run any scripts, nor would it render or run animations, and it may use a simplified/limited form of physics. This approach would cover all cases. However, it has the potential for a severe performance impact.

Again alternatively, we may offer both implementations and switch between them base on settings/available processing power.

### Actor Tracking

Independently from the chosen implementation, we need to track all actors whose schedules are relevant to the active cells. This is a problem because with the default data structures we would at least need to scan every single cell on start-up for relevant actors. The performance impact of such an approach is most likely not acceptable.

Therefore information about which actor's schedule is relevant to which cell needs to be stored in a separate data structure that exists outside of the cell records.

### Schedule Record

We add a new record type (Schedule). A schedule consists of a list of task sub-records.

Each task sub-record consists of:

* A time of the day range
* An AI package
* Ignore flag (see next section)

The time table of a schedule is allowed to have holes (e.g schedule 1 ending at 10:00 and schedule 2 beginning at 10:15).

Tasks may also overlap partially, but no two tasks may start at the same time.

When picking a task from the schedule, all tasks which contain the current time are considered. Of these the task with the earliest start time is chosen.

A new task is picked only when the current one has expired.

### Managing Actor Schedules

Schedules can be assigned to actors either in the actor record or with a new script instruction.

When explicitly switching from one schedule to another, tasks with the ignore flag are ignored. This feature is intended for transitory tasks, that are pointless if the previous task hasn't been performed.

We add script instructions that allow for querying the current schedule of an actor. We also add a script instruction to prematurely end a task.

## AI Packages

To accommodate the NPC schedule feature and other enhancements to the AI system properly we need to redo the AI package system. We will deprecate the old packages.

This is an early draft of the new system and will almost certainly require more design work before we can cut it up into individual tasks.

### Idle

Idle activities are currently bound to the Wander package. This is suboptimal, both because other packages could benefit from it and because storing the idle function in a package instead of the NPC makes it hard to give the NPC a characteristic idle pattern.

We add a list of idle chances for each available idle animation to the NPC record. Ideally this list should be extensible, but first we need to improve the animation system so that the number of idle animations isn't hard-coded any more. This may be a stage 2 task.

Each package contains two fields related to idling behaviour:

* Chance: A value between 0 and 1 that specifies the chance (per reasonable unit of time) of an idle animation to be executed.
* Dialogue Chance: A value between 0 and 1 that specifies the chance of an idle dialogue if an idle animation is executed.

We may decide to use a scale of 0 to 100 instead of 0 to 1 to increase usability for less mathematically minded content developers.

The old wander package keeps its old idle list. When active it disables the new idle system.

If a NPC does not have any active packages the idle function is still active. In this case Chance and Dialogue Chance are determined by two new GMSTs, which default to 0 when loading older omwgame files.

### Package Roles

We categorise package into four roles:

* Legacy: Old packages, generated either via script instructions or via AI package list in the actor record. Removed once they run out (if not flagged for repeat). Deprecated.
* Schedule: Inserted by the schedule system. Can not be directly manipulated via script instructions.
* Manual: New packages, generated via new script instructions.
* Situation: Inserted via other game mechanics (e.g. combat, breath). Can not be directly manipulated via script instructions.

Packages of a type lower in the list take precedence over packages higher in the list. For the roles schedule and situation there can be only one package per actor at a time.

We add new script instructions to query the package state of an actor and to insert and remove manual packages.

### Duration

Each package has two fields that describe its duration:

* Duration itself: Package will self-delete once the current time is larger than the start time plus the duration
* Repeat flag: Package adds itself again at the end of the end of the duration

Packages lacking these fields persist indefinitely unless explicitly removed.

Some legacy packages already have these fields. Schedule packages will not have these fields since the removal is controlled entirely by the schedule system.

### Updates Packages

We translate the packages AiFollow, AiActivate and AiTravel into the new format.

AiFollow and AiTravel specify a location by being given a set of coordinates and an optional cell ID. We will replace part of the package with a different format.

There are two possible locations sub-records of which each package may only contain one:

* A set of coordinates and a worldspace/cell ID (not optional)
* The ID of a (new type) waypoint (no worldspace ID required, since the waypoint includes that)

The other old-style packages require more extensive modifications.

### AiEscort

We can largely keep AiEscort with the following modifications:

* We use the new format to describe the target location (see above)
* Instead of a single location the package lists a sequence of locations, which will allow content developers to make actors escort the player along certain paths and around dangerous areas without resorting to multiple AI packages.

### AiWander

Instead of a location and distance based AI wander, we add a new package that limits the wandering to a single area.

There shouldn't be much need for this package, since AiPatrol and AiVisit offer better alternatives. But we offer it anyway to cover cases where neither of the new packages is seen as useful.

### AiPatrol

This is a new package with no direct equivalent in the old system. The package contains the following fields:

* A list of locations (in the new format)
* A loop flag

The actor will walk from one location to the next. Once he has reached the last location he will either continue with the first location (if the loop flag is set) or walk down the list of locations in opposite direction and then start over (if the loop flag is not set).

The obvious use case for this package is a guard patrolling a perimeter or a place.

### AiVisit

This is a new package with no direct equivalent in the old system. The package contains a list where each list item contains the following fields:

* Location
* Duration (float)
* Chance (float)

An actor with this package will randomly choose a location from the list according to the chance field (locations with higher chance are picked more often), then move to this location and stay there until the duration has expired and then continue from the top.

This package is intended as an alternative to the old AiWander. Hopefully (by giving the NPCs actual places to go to instead of just wandering around aimlessly) it can achieve NPC behaviour that is less silly looking and at least gives the appearance of making sense.

# Localisation

## Problems

There are two fundamental problems with the way localisation is handled in vanilla MW:

* Localised strings are used as internal identifiers that are referenced by other parts of the content
* Localised strings are intermixed with other records

This results in multiple issues:

* When localising the developer must hunt down all references to the localised strings and change them manually (cell names and topics).
* When updating a localisation to a new version of the content file the translator has to manually pierce together his old translation and the new content file.
* Changing a string that is used as an internal reference breaks the connection to the referenced records in relation to the non-localised file, which in turn makes it impossible to utilise automated tools that help for the localisation process.
* Content files of mismatching languages generally can't be combined in a content file list. Mixing languages is generally not desirable for playing, but may sometimes be unavoidable. The ability to mix languages is highly desirable for developers (e.g. debugging).

## Encoding

We already use UTF-8 internally. For post-1.0 versions of content file formats we will also use UTF-8 exclusively.

## Localisation Content Files

Localisation will be contained in separate content files (omwlang) that are to be loaded on top of the content file to be localised.

The launcher will treat a content file and its localisation file as a single entity (see section about Identifying Meta-Data).

We may add a launcher language setting that determines the default language chosen by the launcher. This needs to be a separate option from the legacy encoding setting, which is currently also called language setting.

## Localisation Records

A common localisation scheme is to use the English text as a key. This way non-localised text does not need to bother with the localisation mechanism at all.

This is not a good idea, both in general and particularly in our case:

* The assumption that English is the base language fails in certain areas of the MW community.
* Text does not always translate 1-to-1 between languages. A popular example is the term Free Software. The free has two meanings: free as in free speech and free as in free beer. But in other languages (e.g. German) there are separate words for these two meanings. By using the word free as a key in multiple places with different meanings it would be impossible to correctly localise the text to German.
* The original text might change through an update (fixed spelling mistake is an obvious use-case here). If the text is used as key that breaks the connection between the text in the original content file and the updates content file and therefore forces the translator to figure out what is going on and then make manual corrections.

Instead we use an ID based scheme. We add a new type of record (localisation) that contains as a single field the localised text.

In legacy content files user visible text is contained in other records. On loading of these files we move this text out of these records and into separate localisation files. The ID of the localisation record will be generated from the ID of the other record with a suitable suffix (e.g. $name).

By factoring out all user visible text into a separate record type we make it possible for the editor to present the text to the translator in a single table.

We will allow the editing of the localisation record within the UI (table or dialogue) of the main record, so that for a content developer who is not involved in localisation the workflow is not being made more complicated.

## Fallback-Strings

The openmw.cfg file contains a collection of fallback strings that were taken from Morrowind.ini. No localisation mechanism is provided (other than swapping out Morrowind.ini files).

OpenMW uses these strings only when loading older content files. However "older" is a less specific term here than throughout the rest of this document. As OpenMW progresses through feature releases we will continue to add new user visible strings (e.g. labels for new user settings). Therefore we will need a fully functional localisation system for fallback strings alongside the primary record-based localisation system.

We will allow an optional language-code prefix (lp) in the key:

````
fallback-lp-somekey=Some Text
````

When looking up fallback strings the preferred language is chosen by the default language setting in the launcher (we may decide to provide a separate setting for fallback strings if this doesn't result in an overly complicated settings UI).

If a fallback string with this language-code is available it will be used. If no such fallback string is available, the default fallback string (without language-code prefix) will be used. The default fallback string must exist for all fallback strings with language codes. Otherwise the cfg file must be considered defective.

We add an option to the importer tool to import strings only, which then will be merged (with a user-specified language-code prefix) into an existing cfg file.

Non-legacy fallback strings (added after 1.0) will be added to the global cfg file instead. We will accept localised versions of these strings provided by the community. Since these strings are not part of vanilla MW, there are no legal issues with including these strings in the official repository.

## Cell Names

The handling of cell names in vanilla MW is one of the major localisation problems that need to be addressed post 1.0. Cell names are used both as the text that is presented to the user and as the internal identifier by which cells are referenced. This is further complicated by a system that allows the use of partial names.

We turn the name field (the ID) into an ID-only field. The actual user visible name is moved into an optional localisation record.

We allow referencing of cells by both ID and name. If the name is used, we emit an unobtrusive warning message.

This change does not fix the problem of already existing localisations that have modified the names of cells. See the section about localisation mapping below for a workaround.

## Topics

The situation with dialogue topics is similar to the cell situation and we will apply the same fix, though with a minor modification.

When referencing a topic by the localised text (instead of the ID) from within a dialogue text (this includes journal entries) we do not emit a warning message. Requiring the use of IDs within the dialogue text would be too much of a burden to the content developers.

## Scripts

Vanilla scripts can contain user visible string literals. This interferes with localisation in an unacceptable way.

User visible string literals within scripts exist in three forms:

1. MessageBox strings
2. Topics (in AddTopic instruction)
3. Cell Names (in various location related instructions)

There is nothing we can do to enforce a solution for #1 without causing unreasonable complications for script authors. But we can at least offer options.

* We add a variant of the new LFormat script function (called just Format) which takes as its first parameter the ID of a localisation record instead of a string literal.
* We add a script function called Text which takes the ID of a localisation record as argument and returns the matching string.

Issues #2 and #3 can be enforced at least partially. We declare the use of user visible strings (instead of IDs) deprecated. For all new script instructions using the user visible string instead of the ID results in an error. Old instructions produce a warning instead.

## Localisation Mapping

The new localisation scheme leaves existing localisations unaddressed. Localisations of Morrowind.esm break the connection of cell names and topics in relation to the un-localised Morrowind.esm (English).

We provide a workaround for this problem by introducing localisation mapping. Localisation mapping is a language-specific list of ID/user visible text pairs, provided as a text file. A similar scheme exists for the Russian localisation. We may decide to simple extend the existing implementation of this feature in OpenMW to provide localisation mapping.

Regarding the localisation mapping files two important points need to be considered:

* The files can not be generated automatically. Theoretically we could develop a tool that checks for cell and topic records in two Morrowind.esm files that match in fields other than ID and other user visible text. However this would still require manual checking and seems overall a lot of work for little gain.
* We should not generate and distribute these files within the OpenMW project because of copyright reasons. This task would be left to the modding community.

# GUI

## Argument against general GUI customisation

Under general GUI customisation we understand a fully featured GUI framework that allows content developers to build their own GUI. This is an argument against such an approach.

What are the use cases for a full-scale GUI framework? They seem to fall into two general categories:

1. Alternatives for existing GUI elements (e.g. new inventory system, new load/save GUI, new alchemy GUI)
2. GUI for a completely new game feature that does not exist in vanilla Morrowind

We have handled some instances of case #1 in the OpenMW engine itself already. These seem to do their job and have been well received. It makes sense to continue with this approach and keep this kind of GUI enhancements in the core project instead of handing it over to mod developers. The needs of total conversions in this regard can be met with a skinning system and additional configuration options (see below).

The second case is a little different. If we want enable mod developers to add new skills in the same style as for example alchemy we need a full GUI framework. The question is do we want to encourage this kind of content? Morrowind and also OpenMW are already very heavy on the GUI. A feature set that encourages a lighter use of GUI elements (in favour of more world interactivity instead) could result in overall better content quality.

At a smaller scale the need for interactive GUI features can be fulfilled by individual GUI elements (see below) that can be used from a regular script, similar to multiple choice message boxes.

Note that by going down this route we do not block the addition of a full-scale GUI framework at a later time, if we should decide that we need one after all. All the features described here are useful in general and can be translated into a full-scale solution.

## Skinning

Under the term skinning we understand changes to the look of a GUI without affecting the functionality. Vanilla Morrowind already provides a certain level of skinning-capacity though textures and Morrowind.ini settings. We are going to expand on these and clean up the skinning process. After 1.0 all skinning should be done exclusively through content files and associated resources files. Skinning does not belong in ini/cfg files.

### Windows

In general we do not need to provide skinning options for individual windows. Instead we aim for a single set of options that are applied to all windows. This will ensure consistency throughout the GUI.

This is a preliminary list of elements that require customisation:

* Window border/furniture textures and sizes
* Text (Font, Colour, Size)
* Window background
* Other Widgets (e.g. Frames)

### HUD

Morrowind (and OpenMW 1.0) has only 4 HUD elements:

* Mini-map
* Active effect list
* Dynamic stats bars with enemy health bar and selected weapon and spell display
* Notifications (non-interactive message boxes)

For all these elements we need the ability to customise the the size and the position (Notifications are a special case here, see below). But we also need to give attention to individual HUD elements (see subsections below).

### Mini-Map

At the very least we should make the width and the height of the mini-map configurable independently from each other. But we also should consider allowing other shapes (maybe defined via a texture).

### Active effect list

We need to make the orientation of the effect list configurable (left, right, down, up).

### Dynamic Stats

Dynamic stats are more complicated since they merge three elements:

* Dynamic stats
* Active weapon and spell
* Enemy health bar

The first two go together well enough, but we should provide an option to decouple the enemy health bar and turn it into its own HUD elements.

We also need to consider that after 1.0 we won't always have three dynamic stats. There could be more or (in stage 2) less. The HUD element must be built in a way that it can adjust its layout accordingly without manual layouting.

### Notifications

Currently we have a single hard-coded notification area (bottom centre of screen), where up to a fixed number of text paragraphs can be displayed in a message box style GUI element.

We introduce a new type of record (NotificationArea) with the following fields:

* Position (top/centre/bottom, left/centre/right)
* Scale
* Maximum number of messages
* Duration factor (scales how long a message stays on screen, depending on the length of the message)
* Message box layout (size, textures)
* Text (horizontal alignment, size)
* Text-Wrap (on or off)
* Default style (see below)

When loading older omwgame files we inject a NotificationaArea record (sys::Messages) that matches the old hard-coded notification area.

We add a new script instructions that replaces the non-interactive variant of the old MessageBox instruction.

* Message text, style = "", area = "sys::Messages"): If style is an empty string, the default style for the notification area is used.

### Notification styles

We introduce a new type of record (NotificationStyle) with the following fields:

* Colour
* Font (including options for bold and italic)

When loading older omwgame files we inject a NotificationStyle record (sys::DefaultMessageStyle) that matches the old hard-coded notification area.

## New Input-Elements

We add a couple of new input elements that are each represented as a separate window:

* Item selection: Takes a list of references and returns the list and an index. List is presented as a horizontal icon view of the items.
* List selection: Takes a list of strings and returns the list and an index. List is presented as a vertical list of text items.
* Numerical input: Takes a range, a precision value and a default value and returns a float.
* Text input: Takes a default value and returns a string.

Additionally each window has a title bar, an okay button and and optional cancel button.

For each element we add a new script instruction that brings up the respective element. These instructions take the following arguments:

* The input values listed above.
* The ID of a script function that is called when the okay button is pressed
* (optional) The ID of a script function that is pressed when the cancel button is pressed. If this argument is missing (or the ID string is empty) no cancel button is shown.
* Window title text

## Party HUD Element

We add an optional HUD element that shows the status of companions. This HUD element can be switched on/off and configured in the user settings.

For multiplayer we can also show the status of other players. There need to be visual distinction between companions and other players.

## Container Windows

The vanilla Morrowind container windows (inventory, NPC inventory on loot/trade, actual container) has a very poor usability.

Main issues are:

* Icons are not distinguishable enough. Users need to regularly utilise tooltips to identify an item which defeats the purpose of the icon. This problems is caused both by the large number of different items in Morrowind and by the small icon size which does not leave much room for visual differentiation.
* The available tools (item category tabs) are insufficient to manage a large inventory.

We need to improve upon this issue, but we also need to stay close to the original look and feel. We will make three improvements in a way that is as unobtrusive as possible:

### View Modes

We offer three view modes similar to classical file browser:

* Small Icon Grid: The vanilla Morrowind mode. If no small icon is available, we use a down-scaled version of the large icon.
* Large Icon Grid: Like the vanilla Morrowind mode, but with larger icons (at least twice, but probably three times the size). We add a new field to all item type records that contains name of the large icon texture. If no large icon is available an up-scaled version of the small icon is used.
* Table: One line per item (or item-stack). The columns shown are configurable in the user-settings.

The view mode is selected by three buttons that could be placed in the same row as the category tabs.

### Search

We add a search box (either fixed if we can make space for it or as a pop-up). If the search box is not empty only those items are shown those names contain the search string.

### Categories

We make item categories configurable via content files by introducing a new record (ItemCategory) with the following fields:

* Category name
* Item filter

The item filter will be based on item tags (see section Item-Interactions & -Management). These need to be boolean expressions (e.g. tag1 and tag2 or tag3 or not tag4) since a single tag won't allow for enough customisation. Since this is an issue sensitive to scaling (people have large inventories) we should aim for an implementation that is simple and fast.

When loading older omwgame files we inject ItemCategory records for the vanilla categories.

We may explore an alternative GUI layout consisting of a vertical bar (maybe on the right side of the window) that contains an icon for each category instead of text.

The current horizontal text-based category selector is problematic for two reasons:

* Since we hand over the creation of categories to mod developers we can expect a large number of them and therefore scaling becomes an issue. Horizontally arranged text items scale terribly because text items also primarily extend horizontally. There are workarounds for this problem but these are either bad or add alternative interface (e.g. a pull-down menu listing all tabs) which isn't a great solution either. Under no circumstances will we use the workaround that adds two arrow buttons left and right of the tab bar to change the visible section of the tab bar. This is an anti-pattern and everyone who has ever committed the horrendous crime of implementing this GUI-atrocity deserves to end up in usability hell.
* In table mode we already have a bar at the top of the window. Two bars on the same window border give a cluttered, unclean appearance. 

If we do use an alternative layout, we need to add an icon field to the ItemCategory record and we also need to consider whether we drop the vanilla one completely or let the user choose. Unless there is strong opposition (this is a significant change from Vanilla after all) we should choose the former option. If we opt for a less radical change we still should switch from text to icons to counter the scaling problem, even if we stick with the horizontal layout.

## Character State

Currently there is no way to attach additional status information to items or the player character and show these in the appropriate GUI elements.

With record variables we already have the means to store additional stats. We only need a way to indicate to OpenMW that we want to display these stats.

To this end we add a new record type (CustomStat) with the following fields:

* Type (item or character)
* Label
* Tooltip (optional)
* Name of the record variable

Item stats will be shown in the item tooltip. Character stats will be shown on the right side of the character stats window.

We fold bounty value and reputation into this system and inject suitable CustomStat records when loading older omwgame files.

We will also fold skill progression since level up (the number of skill gains for major and minor skills) into this system but without proving a CustomStat record (since this stat isn't displayed in Vanilla). This task is left to content developers (see below).

### Skill Increases

The number of skill increases is an important value for the levelling process. Unfortunately these values are not presented to the player in Vanilla.

We add a new GMST (sys::TrackSkills) of type integer. When loading older omwgame files we inject this record with a default value of 0.

If the GMST is not 0 we show skill progression in two places:

* The tooltip of the skill (number of skill levels gains)
* The tooltip of the attribute (total number of level gains for all skills associated with this attribute)

# Graphics

TODO

Random collection of ideas that may be feasible or not:

idea? simple text on instance via a new object type, changeable both during content development and during gameplay via script (e.g. sign post)

idea? celestial de-hardcoding (number of moons, number of suns, custom objects, configurable movement across the sky)

idea? animation de-hardcoding/allow additional animations; possible use-cases:

* more idle animations
* sitting animations
* item handling animations (take a drink from a mug, hold a fishing rod, carry a box)

# Editor

## Scripting

We will use Python as a scripting language for OpenMW-CS. Adding a scripting language is a major task and for stage 1 we will most likely be limited to the basics. More advanced support can be added later.

Sandboxing isn't an issue here, since if we want editor extensions to be useful they can't be sandboxed anyway.

### API

We expose the following API:

* Read-Access to all records
* Read-Access to the content file list
* Read-Access to the resources lists
* Command issuing 

### Operation Scripts

We let scripts to run as operations (exclusively for now). In the context of OpenMW-CS the term operation means the following:

* An operation runs in a separate thread and has read only access to the content data
* Multiple operations can run concurrently, but only one operation of each type (e.g. saving, verifier, python script) at a time
* While at least one operation is running, the content data can not be modified
* The operation can be terminated by the user at any time
* The operation reports back to the main thread via messages (shown in a table like with the verifier) and progress state (represented as progress bar)

A script operation may issue commands. These commands are queued up and then executed in the main thread once the operation has finished.

The operations script defines the GUI element it is triggered from; either through an API or an associated resource (depending on how we decide to manage Python script in OpenMW-CS).

For stage 1 we allow operation scripts to add menu items to the main menu and to context menus. We will not add a tools main menu item under which scripts are grouped, because this is akin to a misc category, the most useless category of all. We may consider allowing operation scripts to create their own top-level main menu items.

## Debugging

We will expand on the use of OpenMW-CS as a debugging tool. To support this feature we require an easy to use IPC method (preferably platform-independent). Performance is of less importance since we are unlikely to send large amounts of data between OpenMW and OpenMW-CS.

The potential for debugging tools is almost unlimited. But for stage 1 we most likely will only implement some basic tools. Two easily implementable tools with great usability value are listed below.

Note that all debugging tools will require that OpenMW is started from OpenMW-CS.

### Marking

We add a new keyboard shortcut to OpenMW (only available when started from OpenMW-CS). When used OpenMW will look at the instance under the mouse pointer (or the crosshair if the mouse pointer is hidden). If this instance is part of a content file (i.e. not created during play) OpenMW will send the cell and RefID of the instance to OpenMW-CS. OpenMW-CS inserts the instance info into a table.

This table functions in a similar way to the verifier table; allowing the content developer to jump directly to records that have been identified as being in need of attention.

### Script Error Reporting

OpenMW currently reports script errors and warnings to the console only. When started from OpenMW-CS OpenMW will also send these errors back to OpenMW-CS which will present it in a verifier-like table to the user.

## Namespaces

We will encourage the use of a single main namespace per content file (the content developer is free to add sub-namespaces within this namespace). To this end we will add a namespace field to the content file header.

When adding new IDs the namespace from the header will be inserted into the input field as a default.

We also add function that produces a warning if the user tries to create an ID outside this namespace and outside the special namespaces project, session and default. This function can be disabled via the user-settings. If the content file doesn't have a main namespace (the default for all older content files) this function is inactive.

## Help

OpenMW-CS currently has very little in-application help. Improving this will be an ongoing process. For stage 1 we will focus on scripting.

We will add a keyboard shortcut and a context menu item to the script editor. When activated while the cursor is on a keyword we show a help text that explains the syntax and the function.

Ideally we would want to use the same help text within the application and in our documentation. A way to handle this needs to be determined.

The location where we show the help text needs to be determined. For script editor views we could reuse the widget that displays errors. Or we could add a separate subview type for help text. We may decide to add a help main menu item from which the help system can be navigated.

## Spell Checker

We will add a spell checker by utilising an existing spell check solution. All names (e.g. items, cells, races) will be implicitly added to the list of known words.

## Porting Tools

Under the term porting we understand the process of moving a content file from one version of a dependency to another version of the same dependency.

At the simplest level this requires only changing the dependency information but we will add additional functions that help with the process.

For the porting of localisation content files (i.e. a new version of a content file is released and the localisation needs to adapt to it) we will add the following functions:

* OpenMW-CS will look up all localisation records that have been added in the new version and present these to the user in a list
* OpenMW-CS will look up all localisation records that have been modified in the new version and present these to the user in a list that shows both the old and the new value

# Misc

This is a random collection of other enhancements that don't fit anywhere else.

## Content User Settings

Currently there is no clean way for content files to provide their own user settings.

To compensate for this shortcoming we introduce a new record type (UserSetting) and a script hook  and we rearrange the user settings window.

### UserSettings Record

We add a new record type that describes user settings options. This record type offers several kinds of user settings but does not specify the layout of the GUI elements that are used to represent the settings. We strictly separate the data and the representation (skinning).

User settings data is stored in GMSTs.

A user settings record consists of the following fields:

* ID of the GMST used to store the record
* Category ID
* Label (string)
* Tooltip (string, optional)
* Type of setting (integer, hard-coded)
* Additional data

The following user setting types are available:

* bool (type 0): GMST is an integer; GUI is a check box; additional data: default value
* numeric, integer (type 1): GMST is an integer; GUI is a spin box; additional data: upper bound, lower bound, default value
* numeric, float (type 2): GMST is a float; GUI is a spin box; additional data; upper bound, lower bound, precision, default value
* list (type 3) GMST is an integer; GUI is a combo box; additional data; list of strings for combo box text, default value
* slider (type 4): GMST is a float; GUI is a slider; additional data; upper bound, lower bound, default value

### Categories

User settings categories are represented as separate pages/tabs in the GUI. We specify categories via category IDs (strings). Currently there does not appear to be a need for a separate user settings category record, since this record would have no data.

Content files can create new categories (simply by referencing them in a user settings record) or add to existing categories (including the vanilla ones). We should consider adding a couple of additional default categories (including localised labels, but empty and invisible until populated by content files) to help content developers organise their settings in a coherent way.

The categories in vanilla MW are General, Audio, Controls and Graphics. Content files can add settings to these too, but we should consider reorganisation. General is not a good category. We should consider splitting it up. We should also consider moving the key bindings into their own category. A separate key bindings category would have to be the only exception to the rule that content files are allowed to add settings to pre-existing categories.

### Hook

We add a new hook (sys::SettingsChanged) that is triggered whenever a change to the user settings is mode. The hook function takes a list of strings as argument, containing the IDs of user settings that have been changed.

### Settings Window

The current design of the user settings window in OpenMW is not scalable (it can not accommodate more settings without getting significantly worse). Therefore a complete redesign is required. We should aim for something that resembles the OpenMW-CS user settings window.

This also provides an opportunity to add a GUI to many cfg file only settings that got added prior to 1.0. But we should consider if some of these wouldn't be better suited for bare GMSTs without user setting.

## Launcher Skinning

Currently our launcher has a generic look with some Morrowind branding. This is not desirable for total conversions.

To compensate for this shortcoming we introduce launcher skinning. The launcher skinning is determined by the currently selected omwgame file. We add an new, optional record type that is allowed only in omwgame files.

The exact scope of this new record remains to be determined, but icons and backgrounds are obvious candidates.

## Body of Liquid

Currently we at most have a single body of liquid in each cell that covers the entire cell. This makes it impossible to create additional lakes that are not at sea level.

To compensate for this shortcoming we introduce a new object type: LiquidBody.

Actors within a LiquidBody instance behave in the same way as actors in the regular cell water (drowning, swimming).

A LiquidBody object record has the following fields:

* Liquid ID (see de-hardcoding section, water sub-section)
* Height (float)
* Shape

Shape is a closed Bézier curve that can be edited in the editor. This shape defines a surface (the liquid surface). The depths of liquid body is defined by the height value.

We can imply that only the surface of the LiquidShape instance is exposed. The other sides do not require rendering.

## Dagoth Ur Fix Un-Fix

We introduced a fix for a defect in Morrowind.esm by blocking remote access to instances of the object dagoth_ur_1 via mod instructions for dynamic stats. We will now bind this fix to a new integer GMST in the sys namespace. This will allow content developers to disable this fix in their mods (hopefully after giving poor old Dagoth Ur a bit more health).

## Comment Subrecords

We add a new optional sub-record to all top-level records. This subrecord contains a string that can be used for writing down notes and additional documentation by content developers.

OpenMW does not need to read these records. If we find that comment subrecords add too much bloat to the record data structures we may decide to skip loading them in OpenMW and even keep them out of the data structures used by OpenMW.

OpenMW-CS will add the comment sub-record as a regular data field. We will provide an option to show the comments as tooltips in various places.

Note that the vanilla esm format already contains a similar field in the land texture record. This field needs to be folded into the new command sub-record.

## No Pause Mode

We add four new GMSTs of type integer (default value 1):

* sys::StatusPause
* sys::DialogPause
* sys::ReadingPause
* sys::CraftingPause

These decide if the game is paused when in status mode (right-click), dialogue mode, journal mode or any of the crafting modes (e.g. alchemy). To handle sys::DialogPause==0 properly we ignore  dialogues initiated by NPCs or via script, if the player is already in a dialogue.

## Instance Persistence

We add a new optional field to all object records and instance subrecords, a single enum-like integer.

The value in the object record is considered by OpenMW-CS when creating new instances of that object only (it functions as a default value). OpenMW considers the value in the object record when spawning a new instance during gameplay.

The possible values of the new field are as follows:

* Persistent: Instance can be changed and is included in the save file (this matches the vanilla/OpenMW 1.0 behaviour).
* Transient: Instance can be changed, but is not stored in the save file
* TransientNoSync: Same as Transient; in a multiplayer environment the instance does not need to be synchronised across multiple clients
* Immutable: Instance can not be changed and is not stored in the save file. An attempt to manipulate the instance via a script results in an error. Attempting to spawn an immutable instance into the world also results in an error.

If not present the field defaults to Persistent, except for static objects and instances for which it defaults to Immutable.

The DontSaveObject script instructions (ignored in OpenMW 1.0) can be used to switch a persistent instance to transient state. This function is meant primarily for (marginally) improved backwards compatibility. We do not encourage the use of DontSaveObject for new content.

Immutable statics are a marginal departure from vanilla Morrowind, since statics can be manipulated to some degree. However since that never worked properly in the first place it is not very likely that we break anything and even in case of breakage a quick and simple fix to the content is available. Combined with the large benefits of this feature (drastically reduced save game file size, for one thing) taking the risk of introducing an incompatibility seems worthwhile.

## Multi-Player

OpenMW currently does not support multi-player and is unlikely to do so in the near or intermediate future. But we still can make improvements that aid the multi-player project alongside OpenMW and especially content developers that wish to improve the multi-player experience of their content.

Our goal for multi-player (both in regards to the separate project and if/when multi-player gets integrated into OpenMW) is for content to be usable both in single and in multi-player. This goal isn't fully achievable without special attention from content developers. No matter how we approach multi-player some assumption made by content developers for single-player will be broken, which will then result in some broken content. The best we can hope for is to minimise the breakage.

To this end we will introduce features that will allow content developers to include support for multi-player; mostly in the form of script instructions. These feature must be designed in a way that does not interfere with single-player. Several examples can be found throughout this document.

In the same vein we also provide optimisation options for multi-player, i.e. ways to flag state as not requiring synchronisation across server and clients.

### Content Files

There will always be content files that do not take multi-player into consideration (Morrowind.esm being the obvious case, but some or most current content developers may fall into the same category). This can easily be addressed by putting another content file with fixes on top of the content file (provided the original file had a suitable license).

We will mark up these content files in a similar way as we mark localisations, so that the launcher can automatically match them to their respective main content file and automatically enable them when running multi-player.

### Scripting

Scripting is the biggest issue in regards to multi-player. The current implementation of the multi-player project executes scripts client-side. This is unlikely to work. Anything related to random numbers is an obvious source for (potentially) game-breaking de-syncs. If we dig deeper we will probably find more sources of problems.

On the other hand scripts that contain the ID player can not be executed server-side, because there would be no way to know which player is meant.

The changes required to address these issues are substantial. This is not a task for near future post-1.0 development or even intermediate future development.

What we can do now is to help content developers create their content multi-player ready, so that when multi-player becomes available in OpenMW their content will work out of the box (minus the unavoidable bugs that come from not being able to test content).

### Player

We can have the script engine detect if a script contains the ID player and if that is the case let it execute on the client instead of the server. However that is only a workaround and it is not reliable. Obvious cases where this could fail are:

* Function calls where one of the involved scripts including the ID player and the other doesn't
* The ID player being used not explicitly but via an ID variable.

It would still make sense to implement this workaround as a fallback.

For a proper solution to the problem we need to engineer the scripting language in a specific way and provide guidance to content developers how to use it in a multi-player compatible style.

In particular we need to provide features that allow for the avoidance of the ID literal *Player*.

In some cases the solution is obvious. For example the script slot for OnActivate can pass the activating actor as an argument to the script. Such a script may be able to operate in a more generalised way that does not require an explicit reference to the player. If the script needs to distinguish between player actors and non-player actors it can check the received actor reference against the list of player references.

There are also many script functions that imply the player (e.g. EnableBirthMenu). These must be changed to work on an actor instead.

This is a complex issue that will require more design and deliberation before we can act on it.
