project('devkit', 'cpp',
  default_options: {
    'buildtype': 'release',
    'cpp_std'  : 'c++20',
    'cpp_args' : '-O3',
    'cpp_eh'   : 'none',
    }
  )

if get_option('buildtype') == 'debug'
  test_args = [
    '-fsanitize=address',
    '-fsanitize=undefined',
    '-fno-omit-frame-pointer',
    ]
else
  test_args = []
endif

comp_args = test_args + [
  # '-DCXXOPTS_NO_EXCEPTIONS',
  '-DDOCTEST_CONFIG_DISABLE',
  ]


# ---------
#  build
# ---------

deps = [
  dependency('fmt'),
  dependency('lua'),
  # dependency('cxxopts'),
  ]

srcs = [
  './src/args.hh',
  './src/defer.hh',
  './src/lua.hh',
  # './src/options.hh',
  './src/task.hh',
  ]

inc_dirs = include_directories('./src')

executable('sk', [
    './app/sk.cpp'
    ] + srcs,
  dependencies: deps,
  include_directories: inc_dirs,
  cpp_args: comp_args,
  link_args: comp_args,
  install: true,
  )


# ---------
#  test
# ---------

python = find_program('python3')
gen = generator(python,
  output: '@BASENAME@_test.cpp',
  arguments: [ '@SOURCE_DIR@/test/generator.py', '@INPUT@', '@OUTPUT@']
  )
test_dep = deps + [ dependency('doctest') ]

foreach file : srcs
  name = '@0@_test'.format(
    file.split('/')[-1].split('.')[0]
    )
  exec = executable(name,
    gen.process(file),
    dependencies: test_dep,
    include_directories: inc_dirs,
    cpp_args: test_args,
    link_args: test_args,
    )
  test(name, exec)
endforeach

