include(FetchContent)

FetchContent_Declare(Soci
    GIT_REPOSITORY https://github.com/Leon0402/soci.git
    GIT_TAG        origin/feature/modernCmake
)
# Keep in mind that this only works due to the fact that these cache variables have been declared 
# with option in soci (inconsistency of set(.. CACHE ..) and option() )
set(SOCI_WITH_FIREBIRD ON)
set(SOCI_WITH_CXX11 ON)
FetchContent_MakeAvailable(Soci)