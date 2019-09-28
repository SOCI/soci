# Releasing

The releasing guide for the SOCI maintainers.

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

...for RC1, 2, ... and final release

## Publish release

- GitHub
- SourceForge

## Issue announcement

Post the new release announcement to the project mailing lists:

- [soci-devel](https://sourceforge.net/p/soci/mailman/soci-devel/)
- [soci-users](https://sourceforge.net/p/soci/mailman/soci-users/)
