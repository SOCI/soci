@ECHO OFF
SET PROJECT=soci_dll
SET TARGET=Build
SET TARGETCONFIG=Debug
SET TARGETPLATFORM=Win32
SET CLP=PerformanceSummary;NoSummary


ECHO ===============================================
ECHO   Build '%PROJECT%' project
ECHO ===============================================

IF NOT EXIST %PROJECT%.sln GOTO no_sln_file

msbuild %PROJECT%.sln /t:%TARGET% /p:Configuration=%TARGETCONFIG% /p:Platform=%TARGETPLATFORM% /clp:%CLP% /v:normal /nologo
EXIT /B 0

:no_sln_file
ECHO ERROR: Can not find %PROJECT%.sln file!
EXIT /B 1