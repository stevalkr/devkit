#compdef sk

function _sk() {
  local ifs_bk="$IFS"
  local input=("${(Q)words[@]}")
  IFS=$'\n'
  local res=($(SK_COMPLETE_ARGS_NUM=$((CURRENT)) $input[1] _complete "${input[@]:1}" 2>/dev/null))
  IFS="$ifs_bk"
  local tpe="${${res[1]}%%>	*}"
  local -a suggestions
  declare -a suggestions
  for suggestion in ${res:1}; do
    local sugg="${suggestion%%	*}"
    local desc="${suggestion##*	}"
    suggestions+=("${sugg}:${desc}")
  done
  if [[ "$tpe" == filenames ]]; then
    _files
  fi
  _describe 'sk' suggestions
}

_sk "$@"
