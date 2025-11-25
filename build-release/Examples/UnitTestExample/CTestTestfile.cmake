# CMake generated Testfile for 
# Source directory: D:/project/training/Qt/Q1ORM/Examples/UnitTestExample
# Build directory: D:/project/training/Qt/Q1ORM/build-release/Examples/UnitTestExample
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(Q1ORM_UnitTests "D:/project/training/Qt/Q1ORM/build-release/bin/Debug/UnitTestExample.exe")
  set_tests_properties(Q1ORM_UnitTests PROPERTIES  _BACKTRACE_TRIPLES "D:/project/training/Qt/Q1ORM/Examples/UnitTestExample/CMakeLists.txt;69;add_test;D:/project/training/Qt/Q1ORM/Examples/UnitTestExample/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(Q1ORM_UnitTests "D:/project/training/Qt/Q1ORM/build-release/bin/Release/UnitTestExample.exe")
  set_tests_properties(Q1ORM_UnitTests PROPERTIES  _BACKTRACE_TRIPLES "D:/project/training/Qt/Q1ORM/Examples/UnitTestExample/CMakeLists.txt;69;add_test;D:/project/training/Qt/Q1ORM/Examples/UnitTestExample/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(Q1ORM_UnitTests "D:/project/training/Qt/Q1ORM/build-release/bin/MinSizeRel/UnitTestExample.exe")
  set_tests_properties(Q1ORM_UnitTests PROPERTIES  _BACKTRACE_TRIPLES "D:/project/training/Qt/Q1ORM/Examples/UnitTestExample/CMakeLists.txt;69;add_test;D:/project/training/Qt/Q1ORM/Examples/UnitTestExample/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(Q1ORM_UnitTests "D:/project/training/Qt/Q1ORM/build-release/bin/RelWithDebInfo/UnitTestExample.exe")
  set_tests_properties(Q1ORM_UnitTests PROPERTIES  _BACKTRACE_TRIPLES "D:/project/training/Qt/Q1ORM/Examples/UnitTestExample/CMakeLists.txt;69;add_test;D:/project/training/Qt/Q1ORM/Examples/UnitTestExample/CMakeLists.txt;0;")
else()
  add_test(Q1ORM_UnitTests NOT_AVAILABLE)
endif()
