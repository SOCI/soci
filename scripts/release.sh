#!/usr/bin/env bash
# This script is part of the release procedure of the SOCI library based
# on current state of given release branch or master (see RELEASING.md).
#
# Copyright (c) 2019 Mateusz Loskot <mateusz@loskot.net>
#
# This script performs the following steps:
# 1. If given release/X.Y exists
#    - on remote as origin/release/X.Y, check it out;
#    - locally, then pass option --use-local-branch to check it out;
#    and use it as ready for packaging.
# 3. Determine the full release version number from `SOCI_LIB_VERSION` value in include/soci/version.h.
# 4. Build HTML documentation
# 4.1. Create Python virtual environment
# 4.2. Install MkDocs
# 4.3. Build documentation
# 5. Filter unwanted content
#    - Website files
#    - Markdown documentation and configuration files
#    - CI configuration files
#    - Development auxiliary files
#    - Git auxiliary files
# 6. Create archives (.zip, .tar.gz)
#
usage()
{
    echo "Usage: `realpath $0` [OPTIONS] <release/X.Y branch>"
    echo "  --rc <N>            N is number of release candidate (e.g. from 1 to 9)"
    echo "  --use-local-branch  Use existing local release/X.Y branch instead of checking out origin/release/X.Y"
    echo "  --help, -h          Displays this message"
    exit 1
}
ME=`basename "$0"`
MSG_TAG="| $ME"

UNAME_S="$(uname -s)"
case "${UNAME_S}" in
    Linux*)     THIS_SYS=Linux;;
    Darwin*)    THIS_SYS=Mac;;
    CYGWIN*)    THIS_SYS=Cygwin;;
    MINGW*)     THIS_SYS=MinGW;;
    *)          THIS_SYS="UNKNOWN:${UNAME_S}"
esac
if [[ "$THIS_SYS" != "Linux" ]]; then
    echo "${MSG_TAG} ERROR: This script requires Linux. Yours is '$THIS_SYS'. Aborting."
    exit 1
fi
if [[ ! -d "$PWD/.git" ]] || [[ ! -r "$PWD/include/soci/version.h" ]]; then
    echo "${MSG_TAG} ERROR: Directory '$PWD' is not Git repository. Aborting."
    exit 1
fi

OPT_USE_LOCAL_RELEASE_BRANCH=0
OPT_GIT_RELEASE_BRANCH=""
OPT_RC_NUMBER=""
while [[ $# -gt 0 ]];
do
    case $1 in
        --rc) test ! -z $2 && OPT_RC_NUMBER=$2; shift; echo "${MSG_TAG} INFO: Setting --rc=$OPT_RC_NUMBER, building release candidate archive";;
        --use-local-branch) OPT_USE_LOCAL_RELEASE_BRANCH=1; echo "${MSG_TAG} INFO: Setting --use-local-branch on, using existing local release/X.Y branch";;
        -h|--help) usage;;
        *) OPT_GIT_RELEASE_BRANCH=$1;;
    esac;
    shift
done

if [[ -n "$OPT_RC_NUMBER" ]] && [[ ! "$OPT_RC_NUMBER" =~ ^[1-9]+$ ]]; then
    echo "${MSG_TAG} ERROR: Release candidate must be single digit integer from 1 to 9, not '$OPT_RC_NUMBER'. Aborting."
    exit 1
fi

GIT_RELEASE_BRANCH=$OPT_GIT_RELEASE_BRANCH
if [[ -z "$GIT_RELEASE_BRANCH" ]] || [[ ! "$GIT_RELEASE_BRANCH" =~ ^release/[3-9]\.[0-9]$ ]]; then
    echo "${MSG_TAG} ERROR: Missing valid 'release/X.Y' branch name i.e. release/3.0 or later"
    usage
    exit 1
fi

GIT_CURRENT_BRANCH=$(git branch | grep \* | cut -d ' ' -f2)
echo "${MSG_TAG} INFO: Current branch is $GIT_CURRENT_BRANCH"
echo "${MSG_TAG} INFO: Releasing branch $GIT_RELEASE_BRANCH"

echo "${MSG_TAG} INFO: Fetching branches from origin"
git fetch origin

# Checkout branch release/X.Y
GIT_LOCAL_RELEASE_BRANCH=$(git branch -a | grep -Po "(\*?\s+)\K$GIT_RELEASE_BRANCH")
if [[ $OPT_USE_LOCAL_RELEASE_BRANCH -eq 1 ]]; then
    if [[ -n "$GIT_LOCAL_RELEASE_BRANCH" ]]; then
        echo "${MSG_TAG} INFO: Checking out branch '$GIT_RELEASE_BRANCH'"
        git checkout $GIT_RELEASE_BRANCH || exit 1
        echo "${MSG_TAG} INFO: Updating branch '$GIT_RELEASE_BRANCH' from origin"
        git pull --ff-only origin $GIT_RELEASE_BRANCH || exit 1
    else
        echo "${MSG_TAG} ERROR: Local release branch '$GIT_LOCAL_RELEASE_BRANCH' does not exists. Aborting."
        exit 1
    fi
else
    if [[ -n "$GIT_LOCAL_RELEASE_BRANCH" ]]; then
        echo "${MSG_TAG} ERROR: Local release branch '$GIT_LOCAL_RELEASE_BRANCH' already exists. Aborting."
        echo "${MSG_TAG} INFO: Delete the local branch and run again to checkout 'origin/$GIT_RELEASE_BRANCH'."
        exit 1
    fi
fi

# Checkout branch origin/release/X.Y as release/X.Y
if [[ $OPT_USE_LOCAL_RELEASE_BRANCH -eq 0 ]]; then
    GIT_REMOTE_RELEASE_BRANCH=$(git branch -a | grep -Po "(\*?\s+)\Kremotes/origin/$GIT_RELEASE_BRANCH")
    if [[ -n "$GIT_REMOTE_RELEASE_BRANCH" ]]; then
        echo "${MSG_TAG} INFO: Release branch 'origin/$GIT_RELEASE_BRANCH' does exist. Checking it out."
        git checkout -b $GIT_RELEASE_BRANCH --no-track origin/$GIT_RELEASE_BRANCH || exit 1
        git pull --ff-only origin $GIT_RELEASE_BRANCH || exit 1
    else
        echo "${MSG_TAG} ERROR: Release branch 'origin/$GIT_RELEASE_BRANCH' does not exist. Aborting"
        echo "${MSG_TAG} INFO: Create release branch 'origin/$GIT_RELEASE_BRANCH' and run again."
        exit 1
    fi
fi

GIT_CURRENT_RELEASE_BRANCH=$(git branch -a | grep -Po "(\*\s+)\K$GIT_RELEASE_BRANCH")
if [[ "$GIT_CURRENT_RELEASE_BRANCH" != "$GIT_RELEASE_BRANCH" ]]; then
    echo "${MSG_TAG} ERROR: Current branch is not '$GIT_RELEASE_BRANCH' but '$GIT_CURRENT_RELEASE_BRANCH'. Aborting."
    exit 1
fi

SOCI_VERSION=$(cat "$PWD/include/soci/version.h" | grep -Po "(.*#define\s+SOCI_LIB_VERSION\s+.+)\K([3-9]_[0-9]_[0-9])" | sed "s/_/\./g")
if [[ ! "$SOCI_VERSION" =~ ^[4-9]\.[0-9]\.[0-9]$ ]]; then
    echo "${MSG_TAG} ERROR: Invalid format of SOCI version '$SOCI_VERSION'. Aborting."
    exit 1
else
    echo "${MSG_TAG} INFO: Releasing version $SOCI_VERSION"
fi

SOCI_FULL_VERSION=$SOCI_VERSION
if [[ -n $OPT_RC_NUMBER ]];then
    SOCI_FULL_VERSION=$SOCI_VERSION-rc$OPT_RC_NUMBER
fi
SOCI_ARCHIVE=soci-$SOCI_FULL_VERSION

if [[ -d "$SOCI_ARCHIVE" ]]; then
    echo "${MSG_TAG} ERROR: Directory '$SOCI_ARCHIVE' already exists. Aborting."
    echo "${MSG_TAG} INFO: Delete it and run again."
    exit 1
fi
if [[ -f "${SOCI_ARCHIVE}.zip" ]]; then
    echo "${MSG_TAG} ERROR: Archive '${SOCI_ARCHIVE}.zip' already exists. Aborting."
    echo "${MSG_TAG} INFO: Delete it and run again."
    exit 1
fi
if [[ -f "${SOCI_ARCHIVE}.tar.gz" ]]; then
    echo "${MSG_TAG} ERROR: Archive '${SOCI_ARCHIVE}.tar.gz' already exists. Aborting."
    echo "${MSG_TAG} INFO: Delete it and run again."
    exit 1
fi

if [[ -d $PWD/.venv ]] && [[ ! -f $PWD/.venv/bin/activate ]]; then
    echo "${MSG_TAG} ERROR: Directory '$PWD/.venv' already exists. Can not create Python environment. Aborting."
    exit 1
fi

if [[ ! -f $PWD/.venv/bin/activate ]]; then
    MASTER_PY=""
    if command -v python3 &>/dev/null; then
        MASTER_PY=$(which python3)
    fi
    if [[ -z "$MASTER_PY" ]] && command -v python &>/dev/null; then
        MASTER_PY=$(which python)
    fi
    if [[ -z "$MASTER_PY" ]] || [[ ! $($MASTER_PY --version) =~ ^Python.+3 ]]; then
        echo "${MSG_TAG} ERROR: Python 3 not found. Aborting."
        exit 1
    else
        echo "${MSG_TAG} INFO: Creating Python virtual environment using $MASTER_PY (`$MASTER_PY --version`)"
    fi
    $MASTER_PY -m venv $PWD/.venv || exit 1
fi

if [[ ! -f $PWD/.venv/bin/activate ]]; then
    echo "${MSG_TAG} ERROR: Python virtual environment script '$PWD/.venv/bin/activate' not found. Aborting."
    exit 1
fi
source $PWD/.venv/bin/activate
echo "${MSG_TAG} INFO: Using Python from `which python` (`python --version`)"
python -m pip --quiet install --upgrade pip
python -m pip --quiet install --upgrade mkdocs

echo "${MSG_TAG} INFO: Building documentation with `mkdocs --version`"
mkdocs build --clean

echo "${MSG_TAG} INFO: Exiting Python virtual environment"
deactivate

echo "${MSG_TAG} INFO: Preparing release archive in '$SOCI_ARCHIVE'"
mkdir $SOCI_ARCHIVE
cp -a cmake $SOCI_ARCHIVE
cp -a include $SOCI_ARCHIVE
cp -a languages $SOCI_ARCHIVE
cp -a src $SOCI_ARCHIVE
cp -a tests $SOCI_ARCHIVE
cp -a AUTHORS CMakeLists.txt LICENSE_1_0.txt README.md Vagrantfile $SOCI_ARCHIVE/
mv site $SOCI_ARCHIVE/docs

# Add git SHA-1 to version in CHANGES file
RELEASE_BRANCH_SHA1=$(git show-ref --hash=8 origin/$GIT_RELEASE_BRANCH)
echo "${MSG_TAG} INFO: Appending '$RELEASE_BRANCH_SHA1' hash to version in '$SOCI_ARCHIVE/CHANGES'"
cat CHANGES | sed "s/Version $SOCI_VERSION.*differs/Version $SOCI_FULL_VERSION ($RELEASE_BRANCH_SHA1) differs/" > $SOCI_ARCHIVE/CHANGES

echo "${MSG_TAG} INFO: Building release archive '$SOCI_ARCHIVE.zip'"
zip -q -r $SOCI_ARCHIVE.zip $SOCI_ARCHIVE
if [[ $? -ne 0 ]]; then
    echo "${MSG_TAG} ERROR: zip failed. Aborting."
    exit 1
fi

echo "${MSG_TAG} INFO: Building release archive '$SOCI_ARCHIVE.tar.gz'"
tar -czf $SOCI_ARCHIVE.tar.gz $SOCI_ARCHIVE
if [[ $? -ne 0 ]]; then
    echo "${MSG_TAG} ERROR: tar failed. Aborting."
    exit 1
fi

echo "${MSG_TAG} INFO: Cleaning up"
rm -rf "${SOCI_ARCHIVE}"
git checkout $GIT_CURRENT_BRANCH

if [[ $OPT_USE_LOCAL_RELEASE_BRANCH -eq 0 ]]; then
    echo "${MSG_TAG} INFO: Deleting '$GIT_RELEASE_BRANCH' checked out from 'origin/$GIT_RELEASE_BRANCH'."
    git branch -D $GIT_RELEASE_BRANCH
fi

echo "${MSG_TAG} INFO: Done"
