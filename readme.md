# devkit

devkit is a set of tools for development.

## ShortKut

### Installation

```bash
nix profile install github:stevalkr/devkit#sk
```

Auto-completion is available for `zsh` and `fish`, add the following to your config file:

```
# zsh
fpath=($HOME/.nix-profile/share/zsh/site-functions $fpath)
autoload -U compinit; compinit

# fish
set fish_complete_path $fish_complete_path $HOME/.nix-profile/share/fish/vendor_completions.d
```

### Prerequisites

- fmt
- lua
- meson
- doctest [optional]

### Usage

ShortKut is a customizable alias, currently using Lua as scripting language. Write your own alias following the template:

```lua
-- ${store}/apps/sk.lua

local M = {}
local h = {}

h['alias'] = [[
Usage:
    sk alias [options]

Options:
    -o, --option <opt>  Option description
]]

M.alias = function(cwd, subcommands, options, rest_args, extra_args)
    if Confirmed then
        return {
            use_shell = 'false',
            new_process = 'false',
            search_path = 'true',
            command = 'echo hello ' .. options['option']
        }
    else
        return { command = 'echo Not Confirmed && exit 1' }
    end
end

h['help'] = [[
Usage:
    sk <command> [subcommands] [options] [--] <extra arguments>

Commands:
    alias  Manage alias
    help   Show help
]]

-- function to show help message
M.help = function(command)
    local general_options = [[

General Options:
  -h, --help                           Print help message
  -y, --confirm                        Confirm all prompts
  --store <dir>                        Store directory, default to ~/.devkit ]]

    if #command == 0 then
        command = 'help'
    end

    return (h[command] or '') .. general_options
end

return M
```

For more details, please see [app/sk.cpp](https://github.com/stevalkr/devkit/blob/main/app/sk.cpp) and [dot_devkit](https://github.com/stevalkr/dot_devkit/blob/main/apps/sk.lua).

## devdocker

### Prerequisites

- docker

### Usage

`stevalkr/ros` and `stevalkr/dev` are available on `dockerhub`.

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
