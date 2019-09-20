# Releasing

The releasing guide for the SOCI maintainers.

## Update CHANGES file

Update the list of important changes in the [CHANGES](https://github.com/SOCI/soci/blob/master/CHANGES) file.

It is helpful to generate complete log of changes in Markdown format:

```
git log 3.2.3..master --pretty=format:"1. [%h](http://github.com/soci/soci/commit/%H) - %s"
```

Then, filter it out from trivial changes keeping only the important ones.
Next, group changes in sections for core and backends, sort within sections,
edit and update if necessary. Finally, copy to the `CHANGES` file.

## Update version numbers

Update the version numbers in the following places:

- [include/soci/version.h](https://github.com/SOCI/soci/blob/master/include/soci/version.h)
- [mkdocs.yml](https://github.com/SOCI/soci/blob/master/mkdocs.yml)
- [docs/index.md](https://github.com/SOCI/soci/blob/master/docs/index.md)
- [appveyor.yml](https://github.com/SOCI/soci/blob/master/appveyor.yml)

Search through the source tree looking for other places that use current
version number and may require an update.

## Update website

Link docs for new version.
Build docs and upload or update docs deployment via CI.

*TODO*

## Create source archive

...for RC1, 2, ... and final release

*TODO*

## Publish release

- GitHub
- SourceForge

## Issue announcement

- soci-devel
- soci-users

