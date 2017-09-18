@echo off
rem Runs CMake to configure SOCI for Visual Studio 2017.
rem Runs MSBuild to build the generated solution.
rem
rem Usage:
rem 1. Copy build.bat to build.locale.bat (git ignored file)
rem 2. Make your adjustments in the CONFIGURATION section below
rem 3. Run build.local.bat 32|64
rem 4. Optionally, run devenv.exe SOCI{32|64}.sln from command line

rem ### CONFIGURATION #####################################
rem ### Connection strings for tests (alternatively, use command line-c option)
rem ### For example, SQL Server LocalDB instance, MySQL and PostgreSQL on the Vagrant VM.
set TEST_CONNSTR_MSSQL=Driver={ODBC Driver 11 for SQL Server};Server=(localdb)\MSSQL13DEV;Integrated Security=True;Database=vagrant;MARS_Connection=Yes;
set TEST_CONNSTR_MYSQL=Driver={MySQL ODBC 5.3 Unicode Driver};Database=vagrant;Server=localhost;User=vagrant;Password=vagrant;Option=3;
set TEST_CONNSTR_PGSQL=Driver={PostgreSQL Unicode(x64)};Server=localhost;Database=vagrant;UID=vagrant;PWD=vagrant;
setlocal
set BOOST_ROOT=C:/local/boost_1_59_0
rem #######################################################

set U=""
if /I "%2"=="U"  set U=U
if [%1]==[32] goto :32
if [%1]==[64] goto :64
goto :Usage

:32
set P=32
set MSBUILDP=Win32
set GENERATOR="Visual Studio 15 2017"
goto :Build

:64
set P=64
set MSBUILDP=x64
set GENERATOR="Visual Studio 15 2017 Win64"
goto :Build

:Build
set BUILDDIR=_build%P%%U%
mkdir %BUILDDIR%
pushd %BUILDDIR%
cmake.exe ^
    -G %GENERATOR% ^
    rem -DBOOST_ROOT:PATH=%BOOST_ROOT% ^
    rem -DBOOST_LIBRARYDIR:PATH=%BOOST_ROOT%/lib%P%-msvc-14.0 ^
    ..
move SOCI.sln SOCI%P%%U%.sln
rem msbuild.exe SOCI%P%%U%.sln /p:Configuration=Release /p:Platform=%MSBUILDP%
popd
goto :EOF

:Usage
@echo build.bat
@echo Usage: build.bat [32 or 64]
exit /B 1
