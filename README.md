# upd

👟 Update files, fast.

## Description

`upd` is a command-line utility that automatically determines what files in a
directory need to be updated as resulting from the transformation of other
files, and issues the commands to update them. In that regard it is similar to
the `make` utility. `upd` focuses on convenience and performance. It can
automatically identify sets of source files based on globbing patterns. It can
automatically track transitive dependencies of a file to be updated.

For example, `upd` can be used to manage conveniently the compilation of a C++
project. I will run the process to compile all the `*.cpp` source files of a
specified directory, then run the process to link the object files. It will
track the header dependencies of each source files automatically, so that
changes to a header file triggers a new compilation. At the same time, unrelated
header or source changes will not cause a new compilation, making the update as
fast as possible. Finally, `upd` take care of creating output directories before
they get generated.

## Get Started

The most convenient way to install `upd` is by using either
[`npm`](https://www.npmjs.com/) or [`yarn`](https://yarnpkg.com/). The utility
is split in two main packages. [`upd-cli`](https://www.npmjs.com/package/@jeanlauliac/upd-cli)
is the global CLI tool, while [`upd`](https://www.npmjs.com/package/@jeanlauliac/upd)
can be installed locally and separately for each project. This allows you to
use completely different versions of `upd` on different project easily, in
constrast to utilities such as `make` which are generally global.

For installing with `npm`:

    npm install -g @jeanlauliac/upd-cli    # only if not installed already on your machine
    npm install @jeanlauliac/upd --save-dev

Then you can run the utility from anywhere in the project, ex:

    upd --help
