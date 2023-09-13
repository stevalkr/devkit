project('devcpp', 'cpp',
  default_options: [
    'buildtype=release',
    'cpp_std=c++20',
    'cpp_args=-march=native -fopenmp',
    ]
  )

if get_option('buildtype') == 'debug'
  sanitize_arg = [
    '-fsanitize=address',
    '-fsanitize=undefined',
    '-fno-omit-frame-pointer',
    ]
endif



incdir = include_directories('include')

boost = dependency('boost')
rosbag = dependency('rosbag')

python = find_program('python3')
gen = generator(python,
  output: '@BASENAME@_test.cpp',
  arguments: ['../tests/generator.py', '@INPUT@', '@OUTPUT@']
  )

type_test = executable('type_test',
  gen.process('include/devcpp/type.hpp'),
  include_directories : incdir,
  cpp_args: sanitize_arg,
  link_args: sanitize_arg,
  )
test('type_test', type_test)

bag_writer_test = executable('bag_writer_test',
  gen.process('./include/devcpp/writer/bag_writer.hh'),
  include_directories : incdir,
  dependencies: [
    rosbag
    ],
  )
test('bag_writer_test', bag_writer_test)

bag_reader_test = executable('bag_reader_test',
  gen.process('./include/devcpp/reader/bag_reader.hh'),
  include_directories : incdir,
  dependencies: [
    rosbag
    ],
  )
test('bag_reader_test', bag_reader_test)
