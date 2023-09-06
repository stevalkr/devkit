# devkit

devkit is a set of tools for ros/ros2 development, used by myself.

## Prerequisites

- ~~nix~~
- docker

## Usage

### devdocker

`etherswangel/ros` and `etherswangel/dev` is available in `dockerhub`.

`xproj` is a simple script to help use devdocker containers, modify the script to mount volumes.

Files under `_etc` will override default ones, currently `etc/init`(empty) and `etc/bashrc`. Add `_etc/init` to add custom commands, it will be sourced at the end of bashrc.

```bash
./xproj build   # build docker image
./xproj run     # run docker container
./xproj conn    # connect to existing container
./xproj clean   # remove _build and _install
```

For more details, please see `devdocker/dev:noetic/example` and `devdocker/dev:humble/example`.

### devpy

not ready

### devcpp

not ready

## License

Distributed under the MIT License. See LICENSE for more information.
