# SPDX-License-Identifier: BSD-3-Clause

project('debugcc',
  'c',
  license: ['BSD-3-Clause'],
  meson_version : '>= 0.61.0', # for install_symlink
  default_options: [
    'buildtype=release',
    ],
  version: '0.1.0',
)

platforms = [
  'ipq8064',
  'msm8936',
  'msm8974',
  'msm8994',
  'msm8996',
  'msm8998',
  'qcm2290',
  'qcs404',
  'sc7180',
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

debugcc_link_args = []
if not get_option('dynamic-linking')
  debugcc_link_args += ['-static', '-static-libgcc']
endif

executable('debugcc',
  debugcc_srcs,
  link_args: debugcc_link_args,
  install: true)
