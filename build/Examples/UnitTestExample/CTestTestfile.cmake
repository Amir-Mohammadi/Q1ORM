# CMake generated Testfile for 
# Source directory: C:/Users/ARM/Desktop/Q1ORM/Examples/UnitTestExample
# Build directory: C:/Users/ARM/Desktop/Q1ORM/build/Examples/UnitTestExample
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(Q1ORM_UnitTests "C:/Users/ARM/Desktop/Q1ORM/build/bin/Debug/UnitTestExample.exe")
  set_tests_properties(Q1ORM_UnitTests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/ARM/Desktop/Q1ORM/Examples/UnitTestExample/CMakeLists.txt;76;add_test;C:/Users/ARM/Desktop/Q1ORM/Examples/UnitTestExample/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(Q1ORM_UnitTests "C:/Users/ARM/Desktop/Q1ORM/build/bin/Release/UnitTestExample.exe")
  set_tests_properties(Q1ORM_UnitTests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/ARM/Desktop/Q1ORM/Examples/UnitTestExample/CMakeLists.txt;76;add_test;C:/Users/ARM/Desktop/Q1ORM/Examples/UnitTestExample/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(Q1ORM_UnitTests "C:/Users/ARM/Desktop/Q1ORM/build/bin/MinSizeRel/UnitTestExample.exe")
  set_tests_properties(Q1ORM_UnitTests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/ARM/Desktop/Q1ORM/Examples/UnitTestExample/CMakeLists.txt;76;add_test;C:/Users/ARM/Desktop/Q1ORM/Examples/UnitTestExample/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(Q1ORM_UnitTests "C:/Users/ARM/Desktop/Q1ORM/build/bin/RelWithDebInfo/UnitTestExample.exe")
  set_tests_properties(Q1ORM_UnitTests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/ARM/Desktop/Q1ORM/Examples/UnitTestExample/CMakeLists.txt;76;add_test;C:/Users/ARM/Desktop/Q1ORM/Examples/UnitTestExample/CMakeLists.txt;0;")
else()
  add_test(Q1ORM_UnitTests NOT_AVAILABLE)
endif()
