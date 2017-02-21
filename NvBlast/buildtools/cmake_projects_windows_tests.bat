@echo off

echo #############################################################################################
echo Starting %~n0 %date% %time%
echo #############################################################################################

echo PM_CMakeModules_VERSION %PM_CMakeModules_VERSION%

if NOT DEFINED PM_CMakeModules_VERSION GOTO DONT_RUN_STEP_2

IF NOT DEFINED PM_PACKAGES_ROOT GOTO PM_PACKAGES_ROOT_UNDEFINED

REM Now set up the CMake command from PM_PACKAGES_ROOT

SET CMAKECMD=%PM_cmake_PATH%\bin\cmake.exe

echo "Cmake: %CMAKECMD%"


REM Generate projects here

echo.
echo #############################################################################################
ECHO "Creating VS2015 Win64 TESTS"

SET CMAKE_OUTPUT_DIR=test\compiler\vc14win64-cmake\
IF EXIST %CMAKE_OUTPUT_DIR% rmdir /S /Q %CMAKE_OUTPUT_DIR%
mkdir %CMAKE_OUTPUT_DIR%
pushd %CMAKE_OUTPUT_DIR%
%CMAKECMD% %BLAST_ROOT_DIR%\test -G "Visual Studio 14 2015" -Ax64 -DTARGET_BUILD_PLATFORM=Windows -DSTATIC_WINCRT=1 -DBL_LIB_OUTPUT_DIR=%BLAST_ROOT_DIR%\lib\vc14win64-cmake\ -DBL_DLL_OUTPUT_DIR=%BLAST_ROOT_DIR%\bin\vc14win64-cmake\ -DBL_EXE_OUTPUT_DIR=%BLAST_ROOT_DIR%\bin\vc14win64-cmake\
popd
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%


echo.
echo #############################################################################################
ECHO "Creating VS2015 Win32 TESTS"

SET CMAKE_OUTPUT_DIR=test\compiler\vc14win32-cmake\
IF EXIST %CMAKE_OUTPUT_DIR% rmdir /S /Q %CMAKE_OUTPUT_DIR%
mkdir %CMAKE_OUTPUT_DIR%
pushd %CMAKE_OUTPUT_DIR%
%CMAKECMD% %BLAST_ROOT_DIR%\test -G "Visual Studio 14 2015" -AWin32 -DTARGET_BUILD_PLATFORM=Windows -DSTATIC_WINCRT=1 -DBL_LIB_OUTPUT_DIR=%BLAST_ROOT_DIR%\lib\vc14win32-cmake\ -DBL_DLL_OUTPUT_DIR=%BLAST_ROOT_DIR%\bin\vc14win32-cmake\ -DBL_EXE_OUTPUT_DIR=%BLAST_ROOT_DIR%\bin\vc14win32-cmake\
popd
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo.
echo #############################################################################################
ECHO "Creating VS2013 Win32 TESTS"

SET CMAKE_OUTPUT_DIR=test\compiler\vc12win32-cmake\
IF EXIST %CMAKE_OUTPUT_DIR% rmdir /S /Q %CMAKE_OUTPUT_DIR%
mkdir %CMAKE_OUTPUT_DIR%
pushd %CMAKE_OUTPUT_DIR%
%CMAKECMD% %BLAST_ROOT_DIR%\test -G "Visual Studio 12 2013" -AWin32 -DTARGET_BUILD_PLATFORM=Windows -DSTATIC_WINCRT=1 -DBL_LIB_OUTPUT_DIR=%BLAST_ROOT_DIR%\lib\vc12win32-cmake\ -DBL_DLL_OUTPUT_DIR=%BLAST_ROOT_DIR%\bin\vc12win32-cmake\ -DBL_EXE_OUTPUT_DIR=%BLAST_ROOT_DIR%\bin\vc12win32-cmake\
popd
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo.
echo #############################################################################################
ECHO "Creating VS2013 Win64 TESTS"

SET CMAKE_OUTPUT_DIR=test\compiler\vc12win64-cmake\
IF EXIST %CMAKE_OUTPUT_DIR% rmdir /S /Q %CMAKE_OUTPUT_DIR%
mkdir %CMAKE_OUTPUT_DIR%
pushd %CMAKE_OUTPUT_DIR%
%CMAKECMD% %BLAST_ROOT_DIR%\test -G "Visual Studio 12 2013" -Ax64 -DTARGET_BUILD_PLATFORM=Windows -DSTATIC_WINCRT=1 -DBL_LIB_OUTPUT_DIR=%BLAST_ROOT_DIR%\lib\vc12win64-cmake\ -DBL_DLL_OUTPUT_DIR=%BLAST_ROOT_DIR%\bin\vc12win64-cmake\ -DBL_EXE_OUTPUT_DIR=%BLAST_ROOT_DIR%\bin\vc12win64-cmake\
popd
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%


GOTO :End

:PM_PACKAGES_ROOT_UNDEFINED
ECHO PM_PACKAGES_ROOT has to be defined, pointing to the root of the dependency tree.
PAUSE
GOTO END

:DONT_RUN_STEP_2
ECHO Don't run this batch file directly. Run generate_projects_(platform).bat instead
PAUSE
GOTO END

:End
