# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
import os
from typing import Any
from conans import ConanFile, CMake


VALID_MAX_CONFIGS: dict[tuple[str, str], set[str]] = {
    ('Visual Studio', '15'): { '2022' },
    ('Visual Studio', '16'): { '2023' }
}

SETTINGS: dict[str, Any] = {
    'os': ['Windows'],
    'compiler': {
        'Visual Studio': {'version': ['15', '16']},
    },
    'build_type': None,
    'arch': 'x86_64'
}

TOOL_REQUIRES: list[str] = [
    'cmake/3.24.1',
    'thinkboxcmlibrary/1.0.0'
]

REQUIRES: list[str] = [
    'thinkboxlibrary/1.0.1',
    'thinkboxmxlibrary/1.0.0',
    'magma/1.0.1',
    'maxsdk/1.0.0',
    'tbb/2020.3',
    'tinyxml2/9.0.0'
]

class MagmaMXConan(ConanFile):
    name: str = 'magmamx'
    version: str = '1.0.0'
    description: str = 'Magma implementation for 3ds Max plugins.'
    settings: dict[str, Any] = SETTINGS
    generators: str | list[str] = 'cmake_find_package'
    tool_requires: list[str] = TOOL_REQUIRES
    options: dict[str, Any] = {
        'max_version': ['2022', '2023']
    }
    
    def requirements(self) -> None:
        for requirement in REQUIRES:
            self.requires(requirement)

    def configure(self) -> None:
        if self.options.max_version == None:
            self.options.max_version = '2023'
        self.options['thinkboxmxlibrary'].max_version = self.options.max_version
        self.options['maxsdk'].max_version = self.options.max_version

    def validate(self) -> None:
        if self.options.max_version != self.options['maxsdk'].max_version:
            raise Exception('Option \'max_version\' must be the same as maxsdk')
        compiler = str(self.settings.compiler)
        compiler_version = str(self.settings.compiler.version)
        compiler_tuple = (compiler, compiler_version)
        max_version = str(self.options.max_version)
        if max_version not in VALID_MAX_CONFIGS[compiler_tuple]:
            raise Exception(f'{str(compiler_tuple)} is not a valid configuration for 3ds Max {max_version}')

    def build(self) -> None:
        cmake = CMake(self)
        cmake.configure(defs={
            'MAX_VERSION': self.options.max_version
        })
        cmake.build()

    def export_sources(self) -> None:
        self.copy('**.h', src='', dst='')
        self.copy('**.hpp', src='', dst='')
        self.copy('**.cpp', src='', dst='')
        self.copy('**.cmake', src='', dst='')
        self.copy('**.rc', src='', dst='')
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
        self.cpp_info.libs = ["magmamx"]
