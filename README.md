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
