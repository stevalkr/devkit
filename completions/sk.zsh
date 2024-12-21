#compdef sk

function _sk() {
  local ifs_bk="$IFS"
  local input=("${(Q)words[@]}")
  IFS=$'\n'
  local res=($(SK_COMPLETE_ARGS_NUM=$((CURRENT)) $input[1] _complete ${input[@]:2} 2>/dev/null))
  IFS="$ifs_bk"
  local tpe="${${res[1]}%%>	*}"
  local -a suggestions
  declare -a suggestions
  for suggestion in ${res:1}; do
    suggestions+=("${suggestion%%	*}")
  done
  local -a args
  if [[ "$tpe" == filenames ]]; then
    args+=('-f')
  fi
  compadd -J nix "${args[@]}" -a suggestions
}

_sk "$@"
