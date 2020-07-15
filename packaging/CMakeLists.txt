include(InstallSoci)

INSTALL (DIRECTORY ${PROJECT_SOURCE_DIR}/include/soci DESTINATION include)

install_component(NAME Soci TARGETS soci_core soci_version soci_backend_config)

if(SOCI_WITH_DB2)
  install_component(NAME SociDb2 TARGETS soci_db2)
endif() 

if(SOCI_WITH_EMPTY)
  install_component(NAME SociEmpty TARGETS soci_empty)
endif() 

if(SOCI_WITH_FIREBIRD)
  install_component(NAME SociFirebird TARGETS soci_firebird)
endif() 

if(SOCI_WITH_MYSQL)
  install_component(NAME SociMySQL TARGETS soci_mysql)
endif() 

if(SOCI_WITH_ODBC)
  install_component(NAME SociOdbc TARGETS soci_odbc)
endif() 

if(SOCI_WITH_ORACLE)
  install_component(NAME SociOracle TARGETS soci_oracle)
endif() 

if(SOCI_WITH_POSTGRESQL)
  install_component(NAME SociPostgresql TARGETS soci_postgresql)
endif() 

if(SOCI_WITH_SQLITE3)
  install_component(NAME SociSqlite3 TARGETS soci_sqlite3)
endif() 
