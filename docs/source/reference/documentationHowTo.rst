#######################################
So you want to help with documentation?
#######################################

Or a beginner's guide to writing docs without having to deal with more techie stuff than you have to.
#####################################################################################################

Intro
=====

The premise of this guide is that you would like to help out the OpenMW project beyond play-testing for bugs and such, 
*buuuuut* you're like me and don't really know how to code. 
This has the rather pesky side effect of you not really knowing about all the tools like GitHub and such. 
While many of these tools are super handy and great to know how to use, 
not everyone has the actual need and desire to learn the ins and outs of them. 
Since we would like as much help fleshing out the user documentation as possible, 
I wrote this guide to lower the barrier of entry into contributing to the project.

*However*, as much as I will try to guide you through all the tedious setup and day-to-day stuff, 
you will eventually have to learn to write using ReST (reStructuredText) formatting. 
Since you're probably like me when I started helping and don't know wtf ReST is, never fear. 
It's an incredibly simple language that is easy to read in plain text form that can then be converted automatically 
into different types of documents like PDFs and html for webpages.

Baby Steps
==========

Create an account on GitLab (https://gitlab.com), and sign in.
(Github probably works too, though some details will differ. More details later – maybe.)

Go to the OpenMW project on GitLab (https://gitlab.com/OpenMW/openmw)
Navigate to whatever documentation you want to tackle.
Choose Repository and Files in the menu on the left, then docs and source in the tree in the centre.

Don’t overlook the tutorial-style-guide.txt there for some tips to get you started.

Open whichever file you want to tackle – probably within the manuals or reference directories.
There’s also a dropdown box to the right of edit, at the top of the left menu, 
which offers options such as new file or directory, or upload file, with “+” to close that dropdown box.

Click on "Edit" towards top right which will reveal the underlying version, 
rather than the version displayed to normal readers. Use "Write" and "Preview" to switch between the two views.

When you have made the appropriate changes, and checked them in Preview mode, click the Green "Commit changes" button at the bottom.
This should add a branch, with a default name such as patch-1, to your own repository, and add a Merge Request to the main OpenMW Project.

[More details on the commit and review process]

Changes between submission and acceptance of the Merge Request - just make them in your branch, then press the Commit button there.

Going Deeper
============
So here's what you're gonna be learning how to set up:

1.	`GitHub`_
2.	`PyCharm`_
3.	`Sphinx`_
4.	`Sample PR`_

GitHub
======

GitHub is the website the OpenMW project is hosted on. It utilizes Git, which is a version control system, 
meaning it helps us all collaborate on the project without interfering with each others' work. 
The commands are a little annoying because there is a certain amount of undescriptive jargon, 
but for the most part, what you need to know is very simple and I'll walk you through it. 
There are three main parts that you should know:

1.	The OpenMW repository
2.	Your online repository
3.	Your local repository

The master OpenMW repository is where all of our work comes together and where the most current version of the source code resides. 
A repository, also called repo, is a directory or the main folder that holds a project. 
You will need to create your own account on GitHub so you can *fork* the OpenMW repository. 
Forking is just when you clone a project into a repository on your own account so you can make changes however you like 
without accidentally messing up the original project. 
Now, you could add and edit files on GitHub.com directly through your online repository, 
however it's much easier to work on them on your own computer in your local repository. 
Local just refers to the fact that it's physically stored on your computer's hard drive. Here are the easy steps for doing all this:

1.	Go to GitHub.com and sign up for a free account.
2.	Navigate to the master OpenMW repo at: https://github.com/OpenMW/openmw
3.	In the upper right corner, click on the button that says "Fork". This should take you to the newly created fork in your own account ``<username>/openmw``.

Now you have an online repository that is the exact copy of the OpenMW master. To set up your local repository, we're going to use PyCharm.

If you want more info I recommend reading this guide: https://readwrite.com/2013/09/30/understanding-github-a-journey-for-beginners-part-1/

PyCharm
=======

PyCharm is what's known as an IDE, which stands for integrated development environment. 
All this means is that it's for writing code and has a bunch of built-in features that make it easier to do so. 
In this case, PyCharm is made for the language Python, which is what Sphinx is written in. 
We won't actually be touching any of the Python, but some of the built-in features are extremely useful. 
Let's start setting it up:

1.	Go to https://www.jetbrains.com/pycharm/download/
2.	Select your OS, then download the free Community version.
3.	Locate and install.
4.	Run the program and let it load.
5.	Now we're going to connect it to our GitHub account and let it create the local repository by itself. In the welcome menu, go to the bottom right where it says configure and select Settings/Preferences.
6.	Click Version Control and select GitHub.
7.	Click Create API Token and enter your GitHub username and password in the dialogue box, then click Login.
8.	This should allow PyCharm to automatically connect to GitHub, but go ahead and click Test just to be sure.
9.	Click Apply and OK to save the settings.
10.	Back in the welcome window, click "Check out from version control" and select GitHub.

	.. note::
			After this step, it should log in to your GitHub. If not, you probably messed up the Token creation. 
			If you're on Mac, you may come across and error complaining about XCode and admin priviledges. If this happens, 
			open Terminal and type: ``sudo xcodebuild -license`` Read through the license and agree. 
			This should fix the error and allow you to log in.

11.	In Git Repository URL, select your OpenMW repository and click Clone

Congrats! You now have the OpenMW source code on your computer and you can begin making changes and contributing. 
If you're reading this guide though, you probably won't have any idea how to do that, 
so let's go through setting up Sphinx, then I'll go through it.

Sphinx
======

So far I've mentioned ReST (reStructuredText) a couple times, but what is it, and what is Sphinx? 
The most basic explanation is that ReST is the markup language (like HTML is the markup language for webpages) 
and Sphinx is the program that goes through and builds the actual document so you can read it in a more visually pleasing way. 
For a much more detailed explanation, I recommend: https://coderwall.com/p/vemncg/what-is-the-difference-rest-docutils-sphinx-readthedocs

This will be the most technical section as we have to use the command prompt or terminal to install Python and Sphinx. 
I had intended to give you a universal explanation on how to install both, 
but it would drastically increase the length of this guide. 
The tutorial on the Sphinx website is really just going to be better than anything I write here, 
so please refer to their guide here: https://www.sphinx-doc.org/en/stable/install.html

Hopefully you now have Python and Sphinx installed. ...

Now you should have everything installed and running so you can collaborate on documentation properly. 
Let's go through a few more brief GitHub basics. There are really only 4 things you will be using regularly:

1.	Rebase
2.	Commit
3.	Push
4.	Pull request (PR)

Rebasing means you're taking all changes in one branch and applying them directly on top of another branch. 
This is slightly different than a merge which compares the two branches and makes another state combining the two. 
The difference is slight, but we use the rebase because it keeps the history cleaner. 
You will always rebase your local repository from the OpenMW master repository. 
This ensures you have all the most up to date changes before working on stuff so there is less chance of conflicts that 
need to be resolved when your branch is merged back into the master. 
A commit is basically just stating which files you want to mark as ready to be "pushed" to your online repository. 
A push is just copying those "committed" changes to your online repo.
(Commit and push can be combined in one step in PyCharm, so yay) 
Once you've pushed all the changes you need to contribute something to the project, you will then submit a pull request, 
so called because you are *requesting* that the project maintainers "pull"
 and merge the changes you've made into the project master repository. One of the project maintainers will probably ask 
 you to make some corrections or clarifications. Go back and repeat this process to make those changes, 
 and repeat until they're good enough to get merged.

So to go over all that again. You rebase *every* time you start working on something to ensure you're working on the most 
updated version (I do literally every time I open PyCharm). Then make your edits. 
You commit and push from your local repo to your online repo. 
Then you submit a pull request and people can review your changes before they get merged into the project master! 
Or in list form:

1.	Rebase local repo from OpenMW master
2.	Make your edits
3.	Commit and push your local edits to your online repo
4.	Go online and submit a pull request
5.	Repeat steps 1-4 until someone approves and merges your PR

Preview Documentation
*********************

You will probably find it helpful to be able to preview any documentation you've made. 
I often forget necessary syntax and this allows me to double check my work before submitting a PR. 
Luckily, PyCharm has a handy built-in feature that allows you to easily generate the docs.

1.	In the top right corner of the PyCharm window, select the drop-down menu and select `Edit Configurations`.
2.	In the `Run/Debug Configurations` dialogue, click the green plus button in the top left and select `Python Docs > Sphinx Tasks`.
3.	Under the Configuration tab, make sure the following are filled out:
		:Name:		<whatever will help you remember what this is, just make sure you name it or it won't save>
		:Command:	html
		:Input:		<path-to-your-PyCharm-openmw-directory/docs/source>
		:Output:	<wherever you want the build files to be>
4.	Click `Apply`, then `OK`.

Now in order to generate the documentation on your computer to preview them, 
just click the green play button in the top right, next to the drop down menu with the name you chose above selected. 
Sphinx will run and you can view the resulting documentation wherever you chose Output to be, above. 
The window that Sphinx runs in will also show any errors that occur during the build in red, 
which should help you find typos and missing/incorrect syntax.

GitLab integration in PyCharm
=============================

As most of the hosting of OpenMW has moved to Gitlab, we should encourage the use of GitLab, 
though GitHub will continue to be supported.

Add a couple of plugins to PyCharm - see general instructions at https://www.jetbrains.com/help/pycharm/installing-updating-and-uninstalling-repository-plugins.html

For Linux/Windows - (MacOS is a little different)

1. File/Settings/Plugins 
2. Browse Repositories
3. Filter with “GitLab”
4. Install “GitLab Integration Plugin”, 
5. Follow the accompanying instructions to register your GitLab account (after restarting PyCharm) - File/Settings/Other Settings/Gitlab Integration
6. Install “GitLab Projects”

Within your account on GitLab

1. Fork OpenMW if you haven’t already done so
2. Select Settings from the dropdown box in your Avatar (top right)
3. Select Access Tokens from the list on the left 
4. Enter a name for application that will use it – say “PyCharm”
5. Set an expiry date
6. Check  the “api” box
7. Create the token, and use it to complete the setup of the "GitLab Integration Plugin" above.


Sample PR
=========

Coming soon...
