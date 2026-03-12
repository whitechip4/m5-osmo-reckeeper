# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/blacktip/esp/esp-idf/components/bootloader/subproject"
  "/home/blacktip/repos/osmo-reckeeper/build/bootloader"
  "/home/blacktip/repos/osmo-reckeeper/build/bootloader-prefix"
  "/home/blacktip/repos/osmo-reckeeper/build/bootloader-prefix/tmp"
  "/home/blacktip/repos/osmo-reckeeper/build/bootloader-prefix/src/bootloader-stamp"
  "/home/blacktip/repos/osmo-reckeeper/build/bootloader-prefix/src"
  "/home/blacktip/repos/osmo-reckeeper/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/blacktip/repos/osmo-reckeeper/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/blacktip/repos/osmo-reckeeper/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
