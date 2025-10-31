# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "Examples\\CRUDExampleExample\\CMakeFiles\\CRUDExampleExample_autogen.dir\\AutogenUsed.txt"
  "Examples\\CRUDExampleExample\\CMakeFiles\\CRUDExampleExample_autogen.dir\\ParseCache.txt"
  "Examples\\CRUDExampleExample\\CRUDExampleExample_autogen"
  "Examples\\DatabaseInstallExample\\CMakeFiles\\DatabaseInstallExample_autogen.dir\\AutogenUsed.txt"
  "Examples\\DatabaseInstallExample\\CMakeFiles\\DatabaseInstallExample_autogen.dir\\ParseCache.txt"
  "Examples\\DatabaseInstallExample\\DatabaseInstallExample_autogen"
  "Examples\\DbContextExample\\CMakeFiles\\DbContextExample_autogen.dir\\AutogenUsed.txt"
  "Examples\\DbContextExample\\CMakeFiles\\DbContextExample_autogen.dir\\ParseCache.txt"
  "Examples\\DbContextExample\\DbContextExample_autogen"
  "Src\\CMakeFiles\\Src_autogen.dir\\AutogenUsed.txt"
  "Src\\CMakeFiles\\Src_autogen.dir\\ParseCache.txt"
  "Src\\Src_autogen"
  "Tools\\ExampleProjectModifier\\CMakeFiles\\ExampleProjectModifier_autogen.dir\\AutogenUsed.txt"
  "Tools\\ExampleProjectModifier\\CMakeFiles\\ExampleProjectModifier_autogen.dir\\ParseCache.txt"
  "Tools\\ExampleProjectModifier\\ExampleProjectModifier_autogen"
  "Tools\\ReleaseInstaller\\CMakeFiles\\ReleaseInstaller_autogen.dir\\AutogenUsed.txt"
  "Tools\\ReleaseInstaller\\CMakeFiles\\ReleaseInstaller_autogen.dir\\ParseCache.txt"
  "Tools\\ReleaseInstaller\\ReleaseInstaller_autogen"
  )
endif()
