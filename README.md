# upd

👟 Update files, fast.

[![npm version](https://badge.fury.io/js/%40jeanlauliac%2Fupd.svg)](https://badge.fury.io/js/%40jeanlauliac%2Fupd)
[![Build Status](https://travis-ci.org/jeanlauliac/upd.svg?branch=master)](https://travis-ci.org/jeanlauliac/upd)

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
use completely different versions of `upd` on different projects easily. This
constrasts with utilities such as `make`, which are generally global.

For installing with `npm`:

```sh
npm install -g @jeanlauliac/upd-cli    # only if not installed already on your machine
npm install @jeanlauliac/upd --save-dev
```

Then you can initialize your project by running the following in the
project's root directory:

```sh
upd --init
```

This creates a single file `.updroot` that is used by `upd` to know what is the
root directory of the project. Only files in that directory or in any of its
children directories can be managed and updated by `upd` for that particular
project.

## Manifest

To know which files to update and how, `upd` needs a document describing the
relationship between files. For example, it can describe that all `*.cpp` files
should be compiled into object files. This document is called the manifest, and
should be contained in a file at the root directory of your project, under the
name `updfile.json`. The manifest is in the JSON format, that is fairly readable
by humans but tedious to maintain by hand. On the other hand it is easy to
generate from a script. As such it is recommended, but not required, to use
[`upd-configure`](https://www.npmjs.com/package/@jeanlauliac/upd-configure) to
generate the manifest.

Here's is an example of manifest that compiles all files matching `src/*.cpp`
into their respective object files in the `output` directory.

```json
{
  "command_line_templates": [
    {
      "binary_path": "clang++",
      "arguments": [
        {
          "literals": ["-c", "-o"],
          "variables": ["output_file"]
        },
        {
          "literals": ["-I", "/usr/local/include"],
          "variables": ["input_files"]
        }
      ]
    }
  ],
  "source_patterns": ["src/(*).cpp"],
  "rules": [
    {
      "command_line_ix": 0,
      "inputs": [{"source_ix": 0}],
      "output": "output/$1.o"
    }
  ]
}
```

`command_line_templates` describes the commands that compile, transform, link,
etc. files. In that example, we only have 1 command. This command is `clang++`,
a C++ compiler. Its arguments are component of several parts. Each part has
literal arguments, and variable arguments. The variables are replaced by the
correct file names when it tries to update a given file. In the example, if we
have a file `src/foo.cpp`, then `upd` will instantiate the following command
line:

```sh
clang++ -c -o output/foo.o -I /usr/local/include src/foo.cpp
```

`source_patterns` describes what are the source files of the project. In this
example, only the C++ source files are considered. `upd` crawls the filesystem
to find all the files matching each of the patterns. Each pattern can
specify capture groups using parentheses. For example `src/(*).cpp` captures
the basename of the file without extensions. For `src/foo.cpp`, it would be
`foo`. These capture groups can be refered to later in rules.

`rules` describes the relationship from source files to generated files, and
between different generated files. It is possible to build a chain of
generated files. Each rule need to specify the following:

* `command_line_ix`: this is the index in base zero of one of the command
  line templates specified earlier.
* `inputs`: this is the indices of the source patterns or the rules that give us
  this rule input files. In this example, a single source pattern is used as
  input. We could use a rule by using the field `rule_ix` instead of
  `source_ix`.
* `output`: this is a substitution pattern that indicates the name of the
  output file for this or these inputs. In that example, it is
  `output/$1.o`. `$1` refers to the first capture group of the input
  pattern. It is replaced by the corresponding captured string. For example,
  if the source pattern is `src/(*).cpp` and we found a file `src/foo.cpp`,
  the captured group is `foo`. So, the resulting output file name will be
  `output/foo.o`.

## Contribute

To get started on developing `upd`:

    # install dependencies (alternative: npm install)
    # and bootstrap a compiled version of `upd`
    yarn

    # setup update manifest
    ./configure.js

    # compile `upd` using itself!
    dist/upd dist/upd

    # run unit+e2e tests
    yarn test
