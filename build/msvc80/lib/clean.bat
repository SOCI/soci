@ECHO OFF
SET PROJECT=soci_lib
SET TARGET=Clean
SET TARGETCONFIG=Debug
SET TARGETPLATFORM=Win32
SET CLP=PerformanceSummary;NoSummary

REM
REM Clean intermediate Visual C++ project files
REM

IF EXIST *.vcproj.*.user DEL /Q *.vcproj.*.user
IF EXIST *.ncb DEL /Q *.ncb

ECHO ===============================================
ECHO   Clean '%PROJECT%' project binaries
ECHO ===============================================

msbuild %PROJECT%.sln /t:%TARGET% /p:Configuration=%TARGETCONFIG% /p:Platform=%TARGETPLATFORM% /clp:%CLP% /v:normal /nologo
