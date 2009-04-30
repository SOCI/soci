@ECHO OFF
SET PROJECT=soci_dll
SET TARGET=Build
SET TARGETCONFIG=Debug
SET TARGETPLATFORM=Win32
SET CLP=PerformanceSummary;NoSummary


ECHO ===============================================
ECHO   Build '%PROJECT%' project
ECHO ===============================================

msbuild %PROJECT%.sln /t:%TARGET% /p:Configuration=%TARGETCONFIG% /p:Platform=%TARGETPLATFORM% /clp:%CLP% /v:normal /nologo
