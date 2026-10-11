How to contribute to OpenMW
=======================

Non-Coding
==============

You can help by triaging the issues list. Check for unconfirmed bugs and try to reproduce them on your end, working out any details that may be necessary. [See our Bug Reporting Guidelines](https://wiki.openmw.org/index.php?title=Bug_Reporting_Guidelines) on how to improve bug reports.

Other non-coding options include:

- [Testing OpenMW](https://wiki.openmw.org/index.php?title=Testing)
- [Testing Morrowind Mods](https://wiki.openmw.org/index.php?title=Mod_Testing_Guidelines)
- [Researching Morrowind's formulas](https://wiki.openmw.org/index.php?title=Research)

Coding
========

To contribute code to OpenMW:

1. Create a GitLab account so you can open a merge request (MR). You can do this when you're ready to submit one.
2. Fork the [OpenMW repository](https://gitlab.com/OpenMW/openmw) on GitLab
3. Clone your fork locally
4. [Set up a development environment and build OpenMW](https://wiki.openmw.org/index.php?title=Development_Environment_Setup)
5. Make sure your build of OpenMW runs
6. Read the [Developer Reference](https://wiki.openmw.org/index.php?title=Developer_Reference), especially the [Policies and Standards](https://wiki.openmw.org/index.php?title=Policies_and_Standards), and the rest of this document.
7. Find an issue that interests you on the [issue tracker](https://gitlab.com/OpenMW/openmw/-/issues/). [The ones tagged "Beginner"](https://gitlab.com/OpenMW/openmw/-/issues/?scope=all&utf8=✓&state=opened&label_name=Beginner) may be good starting points.

Ideally, before you start coding, reach out on [Discord](https://discord.com/servers/openmw-260439894298460160), [IRC](https://web.libera.chat/#openmw), or [Matrix #openmw-space:openmw.org](#openmw-space:openmw.org) in the development channel (all three are bridged to each other) to discuss the issue and how you plan to tackle it. Alternatively, start the discussion in the GitLab issue itself.

Currently, we are focused on completing the Morrowind game experience and general polishing. Features outside this scope may be approved in some cases, but you should start a discussion first.

Note:
* Issues that have the 'Future' label are usually out of the current scope of the project. Corresponding submissions are unlikely to be merged or even properly reviewed.
* Newly reported bugs should be attempted to be reproduced on the latest code and on the latest available stable release. Both can be found [here](https://openmw.org/downloads/).
* Often, it's best to start a discussion about possible solutions before you jump into coding, especially for larger features.

Merge request guidelines
=======================

* Reference the bug or feature ticket(s) in your commit message or merge request description (e.g. `Bug #123`, `Fixes #123`) to make it easier to keep track of what we changed for what reason. Our issue tracker will show those commits next to the ticket. If your merge request's description includes 'Fixes #123', that issue will automatically be closed when your commit is merged.
* Avoid stuffing unrelated commits into one merge request. As a rule of thumb, each feature and each bugfix should go into a separate MR, unless they are closely related or dependent upon each other. Small merge requests are easier to review and are less likely to require further changes before we can merge them. A "mega" merge request with lots of unrelated commits in it is likely to get held up in review for a long time.
* Feel free to submit incomplete merge requests as [drafts](https://docs.gitlab.com/ee/user/project/merge_requests/drafts/). Even if the work cannot be merged yet, merge requests are a great place to collect early feedback.
* Make sure each of your changes has a clear objective. Unnecessary changes may lead to merge conflicts, clutter the commit history and slow down review. Code formatting 'fixes' should be avoided, unless you were already changing that particular line anyway.
* When pulling changes from master, prefer rebase over merge. Consider using a merge if there are conflicts or for long-running MRs.
* Don't rebase without a good reason. Some good reasons include cleaning up the commit history, resolving merge conflicts, and pulling in related changes or build pipeline changes.
* Review threads should be resolved by the thread author so that they can confirm that their concerns have been adequately dealt with. This rule does not apply to threads consisting solely of [suggested code changes](https://docs.gitlab.com/user/project/merge_requests/reviews/suggestions/)—if applied, they can be left as resolved.

Guidelines for original engine "fixes"
=================================

From time to time, you may be tempted to "fix" what you think was a "bug" in the original game engine.

Unfortunately, the definition of what is a "bug" is not so clear. Consider that your "bug" is actually a feature unless proven otherwise:

* We have no way of knowing what the original developers really intended (short of asking them, good luck with that).
* What may seem like an illogical mechanic can actually be part of an attempt to balance the game.
* Many people will actually <i>like</i> these "bugs" because that is what they remember the game for.
* Exploits may be part of the fun of an open-world game - they reward knowledge with power. There are too many of them to plug them all, anyway.

OpenMW, in its default configuration, is meant to be a faithful reimplementation of Morrowind, minus things like crash bugs, stability issues and severe design errors. However, we try to avoid touching anything that affects the core gameplay, the balancing of the game or introduces incompatibilities with existing mod content.

That said, we may sometimes evaluate such issues on an individual basis. Common exceptions to the above would be:

* Issues so glaring that they would severely limit the capabilities of the engine in the future (for example, the scripting engine not being allowed to access objects in remote cells).
* Bugs where the intent is very obvious, and that have little to no balancing impact (e.g. the bug where being tired made it easier to repair items, instead of harder).
* Bugs that were fixed in an official patch for Morrowind.

Feature additions policy
=====================

We get it: you have waited so long for feature XYZ to be available in Morrowind, and now that OpenMW is here, you cannot wait to implement your ingenious idea and share it with the world.

Unfortunately, since maintaining features comes at a cost and our resources are limited, we have to be a little selective in what features we allow into the main repository. Generally:

* Features should be as generic and non-redundant as possible.
* Any feature that is also possible with modding should be done as a mod instead.
* Through moving certain game logic into built-in scripting, OpenMW will expand the scope of what is possible with modding.
* Modders can edit OpenMW's GUI skins and layout XML files as well as create new widgets through the Lua API, but it is expected that existing C++ widgets will also be recreated through built-in scripting.
* If a feature introduces new game UI strings, you will need to become acquainted with OpenMW's YAML localisation system and expose them. Read about it [here](https://openmw.readthedocs.io/en/latest/reference/modding/localisation.html).

If you are in doubt of your feature being within our scope, it is probably best to start a forum discussion first. See the [settings documentation](https://openmw.readthedocs.io/en/stable/reference/modding/settings/index.html) and [Features list](https://wiki.openmw.org/index.php?title=Features) for some examples of features that were deemed acceptable.

Reviewing merge requests
=======================

We welcome any help in reviewing open MRs. You don't need to be a developer to comment on new features. We also encourage ["junior" developers to review senior's work](https://pagefault.blog/2018/04/08/why-junior-devs-should-review-seniors-commits/).

This review process is divided into two sections because complaining about code or style issues hardly makes sense until the functionality of the MR is deemed OK. Anyone can help with the **functionality review** while most parts of the **code review** require you to have programming experience.

In addition to the checklist below, make sure to check that the **merge request guidelines** were followed.

Functionality review
============

1. Ask for missing information or clarifications. Compare against the project's design goals and roadmap.
2. Check if the automated tests are passing. If they are not, make the MR author aware of the issue and potentially quote the error line. If the error appears unrelated to the MR and/or the master branch is failing with the same error, our CI might be broken and needs to be fixed independently of any open MRs. Raise this issue on one of the following resources:
   * [Discord](https://discord.com/servers/openmw-260439894298460160)
   * [IRC](https://web.libera.chat/#openmw)
   * [Issue tracker](https://gitlab.com/OpenMW/openmw/-/issues)
   * Our [forums](https://forum.openmw.org/)

3. Make sure that the new code has been tested thoroughly, either by asking the author or, preferably, testing yourself. In a complex project like OpenMW, it is easy to make mistakes, typos, etc. Therefore, prefer testing all code changes, no matter how trivial they look. When you have tested an MR that no one has tested so far, post a comment letting us know.
4. On long-running MRs, request the author to update its description with the current state or a checklist of things left to do.

Code review
===========

1. Carefully review each line for issues the author may not have thought of, paying special attention to 'special' cases. Often, people build their code with a particular mindset and forget about other configurations or unexpected interactions.
2. If any changes are workarounds for an issue in an upstream library, make sure the issue was reported upstream so we can eventually drop the workaround when the issue is fixed and the new version of that library is a build dependency.
3. Make sure MRs do not turn into arguments about hardly related issues. If the MR author disagrees with an established part of the project (e.g. supported build environments), they should open a forum discussion or bug report and in the meantime adjust the MR to adhere to the established way, rather than leaving the MR hanging on a dispute.
4. Check if the code matches our style guidelines.
5. Check to make sure the commit history is clean. Squashing should be considered if the review process has made the commit history particularly long. Commits that don't build should be avoided because they are a nuisance for ```git bisect```.

Merging
=======

To be able to merge MRs, commit privileges are required. If you do not have the privileges, just ping someone that does have them with a short comment like "Looks good to me @user".

In general case, you should not merge MRs prematurely even if you are sure they just work or if they receive a senior member's approval.
The rule of thumb is to give at least 24 hours to a couple days of a window for reviews to come through. For more technically involved MRs, 24 hours might not be enough.

Dealing with regressions
========================

The master branch should always be in a working state that is not worse than the previous release in any way. If a regression is found, the first and foremost priority should be to get the regression fixed quickly, either by reverting the change that caused it or finding a better solution. Please avoid leaving the project in the 'broken' state for an extensive period of time while proper solutions are found. If the solution takes more than a day or so then it is usually better to revert the offending change first and reapply it later when fixed.

Fork CI
==========

There are a few caveats to running CI jobs on a fork:

macOS runners will not be available so you will not be able to verify macOS jobs run/build.

For the Ninja jobs, you will need to [create a fine-grained personal access token (PAT)](https://docs.gitlab.com/auth/tokens/fine_grained_access_tokens/#create-a-fine-grained-personal-access-token) with following permissions: Commit Read, Pipeline Read and Job Run access. And then set it as a [CI Secret for your fork](https://docs.gitlab.com/ci/variables/#for-a-project) called `GITLAB_ACCESS_TOKEN`.

Other resources
===============

* [GitHub blog - how to write the perfect pull request](https://blog.github.com/2015-01-21-how-to-write-the-perfect-pull-request/)
* [Tools](https://wiki.openmw.org/index.php?title=Tools) that facilitate testing/development.
