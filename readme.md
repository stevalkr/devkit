# devkit

devkit is a set of tools for development.

## ShortKut

### Prerequisites

- fmt
- lua
- meson
- doctest

### Usage

ShortKut is a customizable alias, currently using Lua as scripting language.

```bash
sk build
sk flake
...
```

Write your own alias following the template:

```lua
local M = {}

-- ~ sk alias
-- hello
M.alias = function()
    return 'echo hello'
end

return M
```

## devdocker

### Prerequisites

- docker

### Usage

`etherswangel/ros` and `etherswangel/dev` are available on `dockerhub`.

`xproj` is a simple script to help use devdocker containers, modify the script to mount volumes.

Files under `_etc` will override default ones, currently `etc/init`(empty) and `etc/bashrc`. Add `_etc/init` to add custom commands, it will be sourced at the end of bashrc.

```bash
./xproj build   # build docker image
./xproj run     # run docker container
./xproj conn    # connect to existing container
./xproj clean   # remove _build and _install
```

For more details, please see `devdocker/dev:noetic/example` and `devdocker/dev:humble/example`.

## devpy

not ready

## devcpp

not ready

## License

Distributed under the MIT License. See LICENSE for more information.
