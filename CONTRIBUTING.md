[Inkscape Developer Documentation](doc/readme.md) /

Contributing to Inkscape - Getting started with Inkscape Development
====================================================================

Inkscape welcomes your contributions to make it an even better
drawing program for the Open Source community.

You can help improve Inkscape with and without programming knowledge. We need both to get a good result.

As a **non-programmer**, you can e.g. help with support, testing, documentation, translations or outreach.
Please see our [Contributing page](https://inkscape.org/contribute/).

If you want to help as a **programmer**, then please follow the rest of this page. We suggest the following steps:

1. Know how to contact us
2. Get the source code
3. Compile and run
4. Look at the developer documentation
5. Set up a GitLab account
6. Make changes
7. Submit as Merge Request

Contact
-------

Feel free to reach out to us.

- Chat: The development team channel is available via [Web Chat](https://chat.inkscape.org/channel/team_devel) or via IRC: irc://irc.libera.chat/#inkscape-devel
- Mailing list: [inkscape-devel@lists.inkscape.org](mailto:inkscape-devel@lists.inkscape.org). Most of the developers are subscribed to the mailing list.
- Bug tracker:
  - Report issues in the [inbox](https://gitlab.com/inkscape/inbox/-/issues).
  - Once issues are confirmed, the developers move them to the [development issue tracker](https://gitlab.com/inkscape/inkscape/-/issues)
- Video conference: We regularly meet by video. Please ask in the chat for details.
- Real life: About once a year there is an Inkscape summit. We also take part in events like Libre Graphics Meeting. This is announced in the chat and mailing list.



Source Code
-----------

Inkscape uses the Git version control system, with the code hosted on GitLab.

 * Inkscape core: https://gitlab.com/inkscape/inkscape
 * Further tools and parts: https://gitlab.com/inkscape/

How to get the sourcode is described in the next step.

Get the Source, Compile and Install
-----------------------------------

[Compile Inkscape from source](doc/building/readme.md).

(If you only want to run Inkscape, but not modify the code, you can also [download a prebuilt Inkscape version](INSTALL.md).)


Developer Documentation
-----------------------

This page is part of the [Inkscape Developer Documentation](doc/readme.md). Here you should find everything you need for programming Inkscape. If not, please contact us or submit a merge request for improvement.


Setting up a GitLab account
---------------------------

To report bugs and submit changes, you need an account for GitLab. This is free. [Sign up on gitlab.com](https://gitlab.com/users/sign_up). You can find more information in the [Gitlab user tutorial](https://docs.gitlab.com/ee/user/profile/account/create_accounts.html).


Submitting Improvements
-----------------------

Changes to Inkscape can be submitted as merge requests on GitLab. If you know how to use GitLab, you can skip this section, *except* for the part about "Changes to CI settings".

The following sections are a rough guide to introduce you to the topic. They should get you started, but are no in-depth guide and provide only some indications of the required steps. If you are new to Git you will likely need to lookup some of the commands and terms on your own. Feel free to ask in the chat and look at [GitLab's tutorials](https://docs.gitlab.com/tutorials/learn_git/).

### Workflow

Once you have implemented new features or fixed bugs, you may want to contribute the changes back to the official Inkscape source code, such that other people can also benefit from your efforts.

Our motto for changes to the codebase is "Patch first, ask questions
later". When someone has an idea, rather than endlessly debating it, we
encourage folks to go ahead and code something up (even prototypish).

You would make this change in your own fork of Inkscape (see GitLab docs about
how to fork the repository), in a development branch of the code, which can be
submitted to GitLab as a merge request (MR). Once in an MR it is convenient for other
folks to try out, poke and prod, and tinker with. We figure, the best
way to see if an idea fits is to try it on for size.

So if you have an idea, go ahead:

### Creating a fork

A *fork* is your own copy of a GitLab repository on GitLab. Contrary to the official repository of Inkscape, you can push changes to your fork (you have write access) and thereby make them publicly available.

Fork the inkscape project (https://gitlab.com/inkscape/inkscape): Create a fork by clicking the Fork icon in the upper right corner of [Inkscape's main GitLab page](https://gitlab.com/inkscape/inkscape). See the [GitLab documentation](https://docs.gitlab.com/ee/user/project/repository/forking_workflow.html#creating-a-fork) on this topic for more information. You then work with your fork instead of the official repository, i.e. clone it to your local storage.

### Changes to CI settings
When you push changes, automatic builds and tests on the GitLab servers are initiated. The default timeout of GitLab is too short for the Inkscape build.

**Important**:
Go to your fork > Settings > CI/CD > General Pipelines > Timeout. Change the Pipeline Timeout to `3 hours`. If you can not find the setting, check the [GitLab documentation](https://docs.gitlab.com/ci/pipelines/settings/#set-a-limit-for-how-long-jobs-can-run).

### Creating a branch

Merge requests operate on branches, so it necessary to create a new branch for the changes you want to contribute. Use a separate branch for each bug/feature you want to work on. Assume you are going to fix a nasty bug. Create a branch with an appropriate name, e.g., `fix-for-bug-xyz`, by running
```
git checkout -b fix-for-bug-xyz
```
on the local clone of your fork. Make your changes (the bugfix) and commit them.

Find more information in the [GitLab documentation](https://docs.gitlab.com/topics/git/branch/).

### Commiting changes

Make the changes and `git commit` them.

See the [Commit Style](#commit-style) for how to write good commit messages.

### Pushing changes

When you are done with your changes, it is usually a good idea to take a few moments and review the status of your local Git repository and your work to make sure everything is the way you want. Pushing the branch to your GitLab fork repository will make it publicly available.

To push the branch to your fork of Inkscape on GitLab, run

```
git push origin fix-for-bug-xyz
```

This also produces a notification like

```
remote: To create a merge request for fix-for-bug-xyz, visit:
remote:   https://gitlab.com/userxxx/inkscape/-/merge_requests/new?merge_request%5Bsource_branch%5D=fix-for-bug-xyz
```

with a link for creating a merge request (where `userxxx` is your username). This message is only output for newly created branches.

### Creating a merge request

There are multiple ways to create a merge request. For example, you can use the above link printed by git push to create a merge request. Alternatively, you can select your branch on the GitLab web interface and click Create merge request in the upper right corner. See the GitLab documentation for more information on creating merge requests.

In GitLab's merge request form, enter a title, a meaningful description and attach pictures or files if appropriate.

It is recommended to tick the *Allow commits from members who can merge to the target branch* checkbox. This allows the core developers to push changes directly to your branch and thereby simplifies the integration of your code into Inkscape.

Try to keep your MR current instead of creating a new one. You can always push new changes to your branch to update the existing MR.

Rebase your MR sometimes.


### Merge request review

Merge requests are reviewed. Other developers will look at your MR and give feedback. Please check regularly if there were new comments on your MR.

Once everyone is happy, the MR is approved.

Repository access
-----------------

Any change (MR) to Inkscape must be approved by at least one other person with write access.
We give write access out to people with proven interest in helping develop
the codebase. Proving your interest is straightforward:  Make two
contributions and request access.

Coding Style
------------

Please refer to the [Coding Style Guidelines](https://inkscape.org/en/develop/coding-style/) if you have specific questions
on the style to use for code. If reading style guidelines doesn't interest
you, just follow the general style of the surrounding code, so that it is at
least consistent.

Before making big changes, we recommend to discuss your idea with us. Someone else might already have plans you can build upon and we will try to guide you!

Commit Style
------------

Write informative commit messages ([check this](https://chris.beams.io/posts/git-commit/)).

Use the full URL of the bug (`https://gitlab.com/inkscape/inkscape/-/issues/1234`) instead of mentioning just the number in messages and discussions.

Documentation
-------------

Code needs to be documented. Future Inkscape developers will really
appreciate this. New files should have one or two lines describing the
purpose of the code inside the file.

Debugging
---------

Inkscape can be debugged with `gdb`. This works best with a debug build, for which you add `-DCMAKE_BUILD_TYPE=Debug` to the CMake arguments (see [Compiling Inkscape](doc/building/readme.md)).

To diagnose memory access issues, building with the Address Sanitizer `-DWITH_ASAN=ON` is recommended. Note that this will slow down inkscape by about 3x. <!-- 2x slowdown according to https://github.com/google/sanitizers/wiki/AddressSanitizerPerformanceNumbers -->

Profiling
---------

[Profiling](https://en.wikipedia.org/wiki/Profiling_(computer_programming)) is a technique to find out which parts of the Inkscape sourcecode take very long to run.

1. Add `-DWITH_PROFILING=ON` to the CMake command (see [Compiling Inkscape](doc/building/readme.md)).
2. Compile Inkscape (again).
3. Run Inkscape and use the parts that you are interested in.
4. Quit Inkscape. Now a `gmon.out` file is created that contains the profiling measurement.
5. Process the file with `gprof` to create a human readable summary:
```
gprof install_dir/bin/inkscape gmon.out > report.txt
```
6. Open the `report.txt` file with a text editor.

Testing
-------

Before landing a patch, the unit tests should pass. You can run them with

```bash
ninja check
```

GitLab will also check this automatically when you submit a merge request.

If tests fail and you have not changed the relevant parts, please ask in the [chat](#contact).

Extensions
----------

All Inkscape extensions have been moved into their own repository. They
can be installed from there and should be packaged into builds directly.
Report all bugs and ideas to that sub project.

[Inkscape Extensions](https://gitlab.com/inkscape/extensions/)

They are available as a sub-module which can be updated independently:

```sh
git submodule update --remote
```

This will update the module to the latest version and you will see the
extensions directory is now changes in the git status. So be mindful of that.


Submodules / Errors with missing folders
----------------------------------------
Make sure you got the submodules code when fetching the code 
(either by using `--recurse-submodules` on the git clone command or by running `git submodule init && git submodule update --recursive`)

