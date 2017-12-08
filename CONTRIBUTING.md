How to contribute to OpenMW
=======================

Not sure what to do with all your free time? Pick out a task from here:

http://bugs.openmw.org/

Currently, we are focused on completing the MW game experience and general polishing. Features out of this scope may be approved in some cases, but you should probably start a discussion first.

Note:
- Tasks set to 'openmw-future' are usually out of the current scope of the project and can't be started yet.
- Bugs that are not 'Confirmed' should be confirmed first.
- Larger Features should have a discussion before you start implementing.
- In many cases, it's best to have a discussion about possible solutions before you jump into coding.

Aside from coding, you can also help by triaging the issues list. Check for bugs that are 'Unconfirmed' and try to confirm them on your end, working out any details that may be necessary. Check for bugs that do not conform to [Bug reporting guidelines](https://wiki.openmw.org/index.php?title=Bug_Reporting_Guidelines) and improve them to do so!

There are various [Tools](https://wiki.openmw.org/index.php?title=Tools) to facilitate testing/development.

Pull Request Guidelines
=======================

Thought of a change? Great! To facilitate the review process, your pull request description should include the following (if applicable):

* A link back to the bug report or forum discussion that prompted the change
* Summary of the changes made
* Reasoning / motivation behind the change
* What testing you have carried out to verify the change

Furthermore, we advise to:

* Separate your work into multiple pull requests whenever possible. As a rule of thumb, each feature and each bugfix should go into a separate PR, unless they are closely related or dependent upon each other. Small pull requests are easier to review, and are less likely to require further changes before we can merge them. A "mega" pull request with lots of unrelated commits in it is likely to get held up in review for a long time.
* Feel free to submit incomplete pull requests. Even if the work can not be merged yet, pull requests are a great place to collect early feedback. Just make sure to mark it as *[Incomplete]* or *[Do not merge yet]* in the title.
* If you plan on contributing often, please read the [Developer Reference](https://wiki.openmw.org/index.php?title=Developer_Reference) on our wiki, especially the [Policies and Standards](https://wiki.openmw.org/index.php?title=Policies_and_Standards).
* Make sure each of your changes has a clear objective. Unnecessary changes may lead to merge conflicts, clutter the commit history and slow down review. Code formatting 'fixes' should be avoided, unless you were already changing that particular line anyway.
* Reference the bug / feature ticket(s) in your commit message (e.g. 'Bug #123') to make it easier to keep track of what we changed for what reason. Our bugtracker will show those commits next to the ticket. If your commit message includes 'Fixes #123', that bug/feature will automatically be set to 'Closed' when your commit is merged.

Guidelines for original engine "fixes"
=================================

From time to time you may be tempted to "fix" what you think was a "bug" in the original game engine.

Unfortunately, the definition of what is a "bug" is not so clear. Consider that your "bug" is actually a feature unless proven otherwise:

* We have no way of knowing what the original developers really intended (short of asking them, good luck with that).
* What may seem like an illogical mechanic can actually be part of an attempt to balance the game. 
* Many people will actually <i>like</i> these "bugs" because that is what they remember the game for.
* Exploits may be part of the fun of an open-world game - they reward knowledge with power. There are too many of them to plug them all, anyway.

OpenMW, in its default configuration, is meant to be a faithful reimplementation of Morrowind, minus things like crash bugs, stability issues and design errors. However, we try to avoid touching anything that affects the core gameplay, the balancing of the game or introduces incompatibilities with existing mod content.

That said, we may sometimes evaluate such issues on an individual basis. Common exceptions to the above would be:

* Issues so glaring that they would severely limit the capabilities of the engine in the future (for example, the scripting engine not being allowed to access objects in remote cells)
* Bugs where the intent is very obvious, and that have little to no balancing impact (e.g. the bug were being tired made it easier to repair items, instead of harder)
* Bugs that were fixed in an official patch for Morrowind 

Feature additions policy
=====================

We get it, you have waited so long for feature XYZ to be available in Morrowind and now that OpenMW is here you can not wait to implement your ingenious idea and share it with the world.

Unfortunately, since maintaining features comes at a cost and our resources are limited, we have to be a little selective in what features we allow into the main repository. Generally:

- Features should be as generic and non-redundant as possible.
- Any feature that is also possible with modding should be done as a mod instead.
- In the future, OpenMW plans to expand the scope of what is possible with modding, e.g. by moving certain game logic into editable scripts.
- Currently, modders can edit OpenMW's GUI skins and layout XML files, although there are still a few missing hooks (e.g. scripting support) in order to make this into a powerful way of modding.
- If a feature introduces new game UI strings, that reduces its chance of being accepted because we do not currently have any way of localizing these to the user's Morrowind installation language.

If you are in doubt of your feature being within our scope, it is probably best to start a forum discussion first. See the [settings documentation](https://openmw.readthedocs.io/en/stable/reference/modding/settings/index.html) and [Features list](https://wiki.openmw.org/index.php?title=Features) for some examples of features that were deemed acceptable.
