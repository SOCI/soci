proc findBoost {} {
    # candidate directories for local Boost:
    set includeDirs {
        "/usr/local/include"
        "/usr/include"
    }
    set libDirs {
        "/usr/local/lib"
        "/usr/lib"
    }

    set includeDir ""
    foreach I $includeDirs {
        set header "${I}/boost/version.hpp"
        if {[file exists $header]} {
            set includeDir $I
            break
        }
    }
    if {$includeDir == ""} {
        return {}
    }

    set libDir ""
    foreach L $libDirs {
        set library "${L}/libboost_date_time.a"
        if {[file exists $library]} {
            set libDir $L
            break
        }
    }
    if {$libDir == ""} {
        return {}
    }

    return [list $includeDir $libDir]
}
