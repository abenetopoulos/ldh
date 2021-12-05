# `ldh`: A package manager for LiveHD projects

## How to build

### Prerequisites

* Python 3: Version >= 3.7
    * Needed for the `bootstrap.py` utility.
* `git`: tested with v2.33.0
* Prerequisites for building [libgit2](https://github.com/libgit2/libgit2/tree/043f3123e3c63b634d9bfd7276e70d86b8accadc#quick-start)
    * CMake
    * Python
    * C compiler

### Building

As soon as your system meets the above prerequisites, run:

```bash
$ pip install -r requirements.txt
$ ./bootstrap.py
```

from the project's root folder. The bootstrap script should set up all development dependencies
appropriately.
You will also have to update your `LD_LIBRARY_PATH` environment variable to include the `libgit2` libraries.

To do this for the current session, run
```bash
$ export LD_LIBRARY_PATH=<LDH_PATH>/libgit2/build/:$LD_LIBRARY_PATH
```
inside a shell, where `LDH_PATH` is the path where you cloned `ldh`. If you want this change to persist,
you'll have to update your shell config (`zshrc`, `bashrc`, etc.) to correctly update the environment
variable.
**Alternatively**, you can create links for the generated libraries into a folder that is already
part of `$LD_LIBRARY_PATH`.

After the above steps are done, you can build `ldh` using either
```bash
$ make debug
```

or
```bash
$ make
```

from the project's root directory. The `debug` variant will output an un-optimized binary with debug symbols,
which also includes the dependencies' full source code (useful for development).


### Current Limitations

As of the time of writing this, when specifying something in semver format (using the `version` key in our TOML),
we can only handle:
* exact versions, like `1.2.3`
* ranges, like `>=1.2.0 && < 2.0.0`

due to limitations of the library we use for semantic version parsing and operations. Any other format should be converted
to one of the two forms above for now.


## Bootstrapping

To make the development process a little bit easier, a python utility called `bootstrap.py` is provided, alongside a
configuration file `submodules.yml`. The purpose of `bootstrap.py` is to make it easy for anyone cloning this repository
to get to compiling as quickly as possible, without `ldh` having to rely on complex systems like CMake to achieve
something as simple as placing dependencies in the right place.

The `bootstrap.py` utility is not meant to be:
- a full fledged dependency/package manager. It assumes that any entry listed in the `submodules.yml` file already
  exists in the repository in the form of a git submodule.
- a build system. We still rely on other build systems (like `cmake`, if the dependency uses it) to do the heavy lifting
  for us.


All external dependencies live under the `external-libs` folder.

### `submodules.yml` structure explanation

Entries in `submodules.yml` follow the structure below:

```yaml
- name: SUBMODULE_NAME
  actions:
    - type: link-include-dir | link-file
      src: RELATIVE_SRC_PATH
      dst: RELATIVE_DST_PATH
```

where:
* `name` is the name of the dependency, as it's found on disk, i.e. the git submodule's name. For instance,
  the entry for the `spdlog` logging library has `SUBMODULE_NAME = spdlog`.
* `actions.[].type` is one of `link-include-dir` or `link-file`. As we're dealing with C++ dependencies, there's two
  types of things we might need to do in order to compile:
    - expose the dependency's `include` directory to our application
    - expose a file (header file, library file, etc.) to our application
  For these purposes, we use an "action" that will essentially create links somewhere where our application can consume
  them. This (the path our application expects the link to be) is specified with the `actions.[].dst` value, which is a
  path **relative** to the application's root directory. The target of the link is specified with the `actions.[].src`
  value, which is a **relative** path inside the dependency's local folder. For instance, to expose the `toml.hpp` file
  from the `tomlplusplus` dependency to our application, we specify

  ```yaml
  actions:
    - type: link-file
      src: "toml.hpp"
      dst: "include/toml.hpp"
  ```

  since the `toml.hpp` file is found at `external-libs/tomlplusplus/toml.hpp`, and we want it "exposed" at
  `LDH_ROOT/include/toml.hpp`.

There may be multiple actions to be performed for a particular dependency, like for `libgit2`.

Dependencies are resolved in the order they appear in the `.yml` file, and actions for each dependency are performed in
the specified order.

Optionally, a dependency may contain the following (top-level) attributes:
```yaml
pre-install:
  - "shell_command_1"
  - "shell_command_2"
  ...
  - "shell_command_n"
pre-remove:
  - "shell_command_1'"
  - "shell_command_2'"
  ...
  - "shell_command_n'"
```

which outline sets of steps that need to be performed before the actions for the dependency are executed.


### Adding a new development dependency

The steps that a contributor will need to follow when adding a new dependency are as follows:
1) Add the new dependency under `external-libs`, either as a git submodule (using `git submodule add ...`) or as a
source directory retrieved in some other way.
2) Add a new entry to `submodules.yml` for the dependency, using the folder name under `external-libs` as the new
entry's `name`, and listing the types of actions needed to expose its supporting files to `ldh`.
3) Run `bootstrap.py`.

The new dependency should now be available for use through `ldh`.
