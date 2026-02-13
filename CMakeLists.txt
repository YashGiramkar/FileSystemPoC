#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

# Sysbuild main-app path: inject app pm.yml early so Partition Manager
# sees it when it collects PM_SUBSYS_PATHS during find_package(Zephyr).
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/pm.yml)
  get_filename_component(APP_PM_YML ${CMAKE_CURRENT_SOURCE_DIR}/pm.yml REALPATH)
  set_property(GLOBAL APPEND PROPERTY PM_SUBSYS_PATHS ${APP_PM_YML})
  set_property(GLOBAL APPEND PROPERTY PM_SUBSYS_OUTPUT_PATHS ${CMAKE_CURRENT_BINARY_DIR}/pm.yml.app)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(FileSystemPoC)

target_sources(app PRIVATE ./ASW/main.c)
