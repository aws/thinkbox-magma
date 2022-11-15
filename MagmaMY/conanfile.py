# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
import os
from typing import Any
from conans import ConanFile, CMake


VALID_MAYA_CONFIGS: dict[tuple[str, str], set[str]] = {
    ('Visual Studio', '16'): { '2022', '2023' },
    ('gcc', '7'): { '2022', '2023' },
    ('gcc', '9'): { '2022', '2023' },
    ('apple-clang', '10.0'): { '2022', '2023' }
}

SETTINGS: dict[str, Any] = {
    'os': ['Windows', 'Linux', 'Macos'],
    'compiler': {
        'Visual Studio': {'version': ['16']},
        'gcc': {'version': ['7', '9']},
        'apple-clang': {'version': ['10.0']}
    },
    'build_type': None,
    'arch': 'x86_64'
}

TOOL_REQUIRES: list[str] = [
    'cmake/3.24.1',
    'thinkboxcmlibrary/1.0.0'
]

REQUIRES: list[str] = [
    'thinkboxlibrary/1.0.0',
    'thinkboxmylibrary/1.0.0',
    'magma/1.0.0',
    'nodeview/1.0.0',
    'mayasdk/1.0.0',
    'tbb/2020.3',
    'tinyxml2/9.0.0'
]

MAYA_QT_VERSIONS: dict[str, str] = {
    '2022': '5.15.2',
    '2023': '5.15.2'
}

DEFAULT_OPTIONS: dict[str, Any] = {
    'qt:shared': True,
    'qt:openssl': False,
    'qt:with_pcre2': False,
    'qt:with_harfbuzz': False,
    'qt:with_sqlite3': False,
    'qt:with_pq': False,
    'qt:with_odbc': False,
    'qt:with_openal': False,
    'qt:with_zstd': False,
    'qt:with_md4c': False
}


class MagmaMYConan(ConanFile):
    name: str = 'magmamy'
    version: str = '1.0.0'
    license: str = 'Apache-2.0'
    description: str = 'Magma implementation for Maya plugins.'
    settings: dict[str, Any] = SETTINGS
    generators: str | list[str] = 'cmake_find_package'
    tool_requires: list[str] = TOOL_REQUIRES
    options: dict[str, Any] = {
        'maya_version': ['2022', '2023']
    }
    
    def requirements(self) -> None:
        for requirement in REQUIRES:
            self.requires(requirement)
        self.requires(f'qt/{MAYA_QT_VERSIONS[str(self.options.maya_version)]}')

    def configure(self) -> None:
        if self.options.maya_version == None:
            self.options.maya_version = '2022'
        self.options['nodeview'].maya_version = self.options.maya_version
        self.options['thinkboxmylibrary'].maya_version = self.options.maya_version
        self.options['mayasdk'].maya_version = self.options.maya_version
        self.default_options = DEFAULT_OPTIONS

    def validate(self) -> None:
        if self.options.maya_version != self.options['mayasdk'].maya_version:
            raise Exception('Option \'maya_version\' must be the same as mayasdk')
        compiler = str(self.settings.compiler)
        compiler_version = str(self.settings.compiler.version)
        compiler_tuple = (compiler, compiler_version)
        maya_version = str(self.options.maya_version)
        if maya_version not in VALID_MAYA_CONFIGS[compiler_tuple]:
            raise Exception(f'{str(compiler_tuple)} is not a valid configuration for Maya {maya_version}')

    def build(self) -> None:
        cmake = CMake(self)
        cmake.configure(defs={
            'MAYA_VERSION': self.options.maya_version
        })
        cmake.build()

    def export_sources(self) -> None:
        self.copy('**.h', src='', dst='')
        self.copy('**.hpp', src='', dst='')
        self.copy('**.cpp', src='', dst='')
        self.copy('**.cmake', src='', dst='')
        self.copy('CMakeLists.txt', src='', dst='')
        self.copy('../NOTICE.txt', src='', dst='')
        self.copy('../LICENSE.txt', src='', dst='')

    def package(self) -> None:
        cmake = CMake(self)
        cmake.install()

        with open(os.path.join(self.source_folder, 'NOTICE.txt'), 'r', encoding='utf8') as notice_file:
            notice_contents = notice_file.readlines()
        with open(os.path.join(self.source_folder, 'LICENSE.txt'), 'r', encoding='utf8') as license_file:
            license_contents = license_file.readlines()
        os.makedirs(os.path.join(self.package_folder, 'licenses'), exist_ok=True)
        with open(os.path.join(self.package_folder, 'licenses', 'LICENSE'), 'w', encoding='utf8') as cat_license_file:
            cat_license_file.writelines(notice_contents)
            cat_license_file.writelines(license_contents)

    def deploy(self) -> None:
        self.copy('*', dst='bin', src='bin')
        self.copy('*', dst='lib', src='lib')
        self.copy('*', dst='include', src='include')

    def package_info(self) -> None:
        self.cpp_info.libs = ["magmamy"]
