function _sk_complete
  set -l sk_args (commandline --current-process --tokenize --cut-at-cursor)
  set -l sk_current_token (commandline --current-token --cut-at-cursor)
  set -l sk_args_num (math (count $sk_args) + 1)

  env SK_COMPLETE_ARGS_NUM=$sk_args_num $sk_args[1] _complete $sk_args[2..-1] $sk_current_token
end

function _sk_accepts_files
  set -l response (_sk_complete)
  test $response[1] = 'filenames'
end

function _sk
  set -l response (_sk_complete)
  string collect -- $response[2..-1]
end

complete --command sk --condition 'not _sk_accepts_files' --no-files
complete --command sk --arguments '(_sk)'
