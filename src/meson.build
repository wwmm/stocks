qt5_dep = dependency('qt5', modules: ['Core', 'Gui', 'Widgets', 'Sql', 'Charts'])

mheaders = [
    'main_window.hpp', 
    'table.hpp', 
    'compare.hpp',
    'correlation.hpp',
    'pca.hpp'
]

mui_files = [
    'ui/main_window.ui', 
    'ui/table.ui', 
    'ui/compare.ui',
    'ui/correlation.ui',
    'ui/pca.ui'
]

moc_files = qt5.preprocess(moc_headers : mheaders, ui_files: mui_files,
                           moc_extra_arguments: ['-DMAKES_MY_MOC_HEADER_COMPILE'],
                           dependencies: qt5_dep)

mysources = [
    'main.cpp', 
    'main_window.cpp', 
    'table.cpp',
    'model.cpp',
    'compare.cpp',
    'correlation.cpp',
    'pca.cpp',
    'chart_funcs.cpp',
    'callout.cpp',
    'effects.cpp',
    moc_files, 
    resources
]

deps = [
    qt5_dep, 
    dependency('eigen3', version: '>=3.3.7'), 
    dependency('openmp')
]

compilar_args = ['-msse2', '-mfpmath=sse', '-ftree-vectorize']

executable(meson.project_name(), mysources,  dependencies : deps, cpp_args:compilar_args)
