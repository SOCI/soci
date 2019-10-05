# Releasing

The releasing guide for the SOCI maintainers.

**NOTICE:** The `4.0` and `4.0.0` version numbers are used below
(e.g. as in `release/4.0`) to keep the instructions more concrete,
but it is a placeholder similar to `X.Y` or `X.Y.Z`.
It should be replaced with the version number of the new release.

## Update CHANGES file

Update the list of important changes in the [CHANGES](CHANGES) file.

It is helpful to generate complete log of changes in Markdown format:

```console
git log 3.2.3..master --pretty=format:"1. [%h](http://github.com/soci/soci/commit/%H) - %s"
```

Then, filter it out from trivial changes keeping only the important ones.
Next, group changes in sections for core and backends, sort within sections,
edit and update if necessary. Finally, copy to the `CHANGES` file.

## Update version numbers

Update the version number of the new release in the following places:

- [include/soci/version.h](include/soci/version.h)
- [mkdocs.yml](mkdocs.yml) - in `release/X.Y` branch set `site_name` with **X.Y** and keep **master** in the `master` branch.
- [docs/index.md](docs/index.md)
- [appveyor.yml](appveyor.yml)

Search through the source tree looking for other places
that use current version number and may require an update.
Depending on type of release, update major, minor and micro version numbers.

The version number also has to be updated in number of places
on the website, see [update website](#update-website) section.

## Add build status to README.md

Add row for `release/X.Y` to the CI build status table in [README.md](README.md).

## Update website content

Update version of the new release on the [www/index.html](www/index.html) page.

Add date and version of the new release on the [www/events.html](www/events.html) page.

Add link to the folder with documentation for the new release
on the [www/doc.html](www/doc.html) page.

## Upload website content

### Continuous deployment

There is [CircleCI workflow](https://circleci.com/gh/SOCI/workflows/soci)
configured in [.circleci/config.yml](.circleci/config.yml)
with dedicated jobs to deploy content website and documentation.

For `master` and `release/X.Y` branches, the jobs do:

- lint all Markdown files
- run [MkDocs](https://www.mkdocs.org) to generate HTML pages from source in [doc/](doc)
- upload generated HTML files to branch-specific documentation folder (e.g. [doc/release/4.0](http://soci.sourceforge.net/doc/release/4.0/))
- upload static HTML files with website pages

See [manual update](#manual-update) for details about SFTP connection.

### Manual update

The website is hosted on SourceForge.net at
[soci.sourceforge.net](https://soci.sourceforge.net)
and in order to upload updated files there, you will need:

- Your SourceForge account credentials with administration rights for SOCI project (used for SFTP authentication)
- [SFTP client to connect to SourceForge host](https://sourceforge.net/p/forge/documentation/SFTP/).

For example, you can use `lftp` to upload website and documentation
files similarly to our CircleCI workflow configured to generate and
deploy documentation (see [.circleci/config.yml](.circleci/config.yml)):

```console
lftp sftp://${DEPLOY_DOCS_USER}:${DEPLOY_DOCS_PASS}@web.sourceforge.net -e "set ftp:ssl-force true; set ftp:ssl-protect-data true; set ssl:verify-certificate no; set sftp:auto-confirm yes; mirror -v -R <source> <target directory>; quit"
```

where `<source>` denotes a file or folder to be copied
and `<target directory>` is a directory under Web content folder
of SOCI on `web.sourceforge.net` i.e. `/home/project-web/soci/htdocs`.

## Create release branch

```console
git checkout master
git branch release/4.0
```

## Create source archive

**NOTICE:** Run this step in Linux environment only.

Run [scripts/release.sh](scripts/release.sh) script to build release archives
with content from `release/X.Y` branch.

```console
cd soci
./scripts/release.sh --help
```

First, you will want to build number of release candidates (typically 1-3, rarely 5-6):

```console
./scripts/release.sh --rc=1 release/4.0
./scripts/release.sh --rc=2 release/4.0
...
```

Eventually, once release candidates have been reviewed and verified,
you will build packages ready for the final release:

```console
./scripts/release.sh release/4.0
```

This will output the release candidate packages:

- `soci-4.0.0-rc1.zip`
- `soci-4.0.0-rc1.tar.gz`

or the final release packages:

- `soci-4.0.0.zip`
- `soci-4.0.0.tar.gz`

where `4.0.0` is placeholder for the major, minor and micro version
determined from the current value of the `SOCI_LIB_VERSION` macro
in [include/soci/version.h](include/soci/version.h).

## Publish release packages on SourceForge.net

## Create annotated tag for final release and push to GitHub

Since we use SourceForge.net to publish release packages, we do not use
GitHub releases, so there is no need to publish a new release via GitHub.
Creating and pushing Git annotated tag is sufficient.

```console
git checkout release/4.0
git tag -a 4.0.0 -m "Releasing SOCI 4.0.0"
git push origin 4.0.0
```

## Announce new release

Post the new release announcement to the project mailing lists:

- [soci-devel](https://sourceforge.net/p/soci/mailman/soci-devel/)
- [soci-users](https://sourceforge.net/p/soci/mailman/soci-users/)

The End.
