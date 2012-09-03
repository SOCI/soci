
#export BOOST_ROOT=d:\usr\boost_1_47_0
#export MYSQL_DIR=C:\Program Files\MySQL\Connector C 6.0.2
export MYSQL_INCLUDE_DIR=/opt/local/include/mysql5/mysql
export MYSQL_DIR=/opt/local/lib/mysql5


export BUILD_TYPE=Debug
if [[ $1 == "Release" ]];then
    export BUILD_TYPE=Release
fi

export BUILD_FOLDER=_build_"$BUILD_TYPE"_32

if [  ! -d "$BUILD_FOLDER" ];then
	mkdir $BUILD_FOLDER
fi

cd $BUILD_FOLDER
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DWITH_BOOST=ON -DSOCI_TESTS=ON -DWITH_MYSQL=ON -DWITH_SQLITE3=OFF -DWITH_ORACLE=OFF -DWITH_ODBC=OFF -DWITH_POSTGRESQL=OFF -DSOCI_MYSQL_TEST_CONNSTR:STRING="dbname=pseaker_example user=pseaker password=pseakerp host=127.0.0.1 port=3306" -G "Unix Makefiles" ../ 
cd ../

