# SPDX-License-Identifier: BSD-3-Clause

project('debugcc',
  'c',
  license: ['BSD-3-Clause'],
  meson_version : '>= 0.61.0', # for install_symlink
  default_options: [
    'warning_level=2',	# sets -Wextra
    'buildtype=release',
    ]
  )

# Set advanced compiler flags
compiler = meson.get_compiler('c')

compiler_cflags = ['-Wno-unused-parameter',
		   '-Wno-unused-result',
		   '-Wno-missing-field-initializers',
		   '-Wno-sign-compare',
		   '-Wundef',
		   '-Wnull-dereference',
		   '-Wdouble-promotion',
		   '-Wshadow',
		   '-Wwrite-strings',
		   '-Wstrict-overflow=4']

# TODO add clang specific options
if compiler.get_id() == 'gcc'
	compiler_cflags += ['-Wformat-signedness',
			    '-Wduplicated-cond',
			    '-Wduplicated-branches',
			    '-Wvla-larger-than=1',
			    '-Walloc-zero',
			    '-Wunsafe-loop-optimizations',
			    '-Wcast-align',
			    '-Wlogical-op',
			    '-Wjump-misses-init']
endif

add_global_arguments(compiler.get_supported_arguments(compiler_cflags),
                     language: 'c')

platforms = [
  'ipq8064',
  'msm8936',
  'msm8994',
  'msm8996',
  'msm8998',
  'qcs404',
  'sc8280xp',
  'sdm845',
  'sm6115',
  'sm6125',
  'sm6350',
  'sm6375',
  'sm8150',
  'sm8250',
  'sm8350',
  'sm8450',
  'sm8550',
  'sm8650',
  ]

debugcc_srcs = [
  'debugcc.c',
  ]

platform_defs = []
platform_array = []

foreach p: platforms
  debugcc_srcs += p + '.c'
  platform_defs += 'extern struct debugcc_platform ' + p + '_debugcc;'
  platform_array += '\t&' + p + '_debugcc,'

  install_symlink(p + '-debugcc',
    install_dir: get_option('bindir'),
    pointing_to: 'debugcc')
endforeach

platforms = configuration_data()
platforms.set('PLATFORM_DEFS', '\n'.join(platform_defs))
platforms.set('PLATFORM_ARRAY', '\n'.join(platform_array))

debugcc_srcs += configure_file(
  input: 'platforms.c.in',
  output: 'platforms.c',
  configuration: platforms)

executable('debugcc',
  debugcc_srcs,
  install: true)
