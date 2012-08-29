@echo off

set BOOST_ROOT=d:\usr\boost_1_47_0
set MYSQL_DIR=C:\Program Files\MySQL\Connector C 6.0.2

set BUILD_TYPE=Debug
if [%1]==[Release] (
	set BUILD_TYPE=Release
)
set BUILD_FOLDER=_build_%BUILD_TYPE%_64

if not exist %BUILD_FOLDER% (
	mkdir %BUILD_FOLDER%
)

cd %BUILD_FOLDER%
cmake -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBOOST_LIBRARYDIR=d:/usr/boost_1_47_0/stage_64/lib -DWITH_BOOST=ON -DSOCI_TESTS=ON -DWITH_MYSQL=ON -DWITH_SQLITE3=OFF -DWITH_ORACLE=OFF -DWITH_ODBC=OFF -DWITH_POSTGRESQL=OFF -DSOCI_MYSQL_TEST_CONNSTR:STRING="dbname=pseaker_example user=pseaker password=pseakerp host=127.0.0.1 port=3306" -G "Visual Studio 8 2005 Win64" ../ 
cd ../
echo "%BUILD_FOLDER%/soci.sln" > _start_msvc.bat