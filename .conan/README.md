# Conan package for SOCI

## Test the package creation
How to test the package from the current dir (`... soci/.conan/`):

    conan create . conan/stable

Expected output (when executed `example`):

```
Hola SOCI
sqlite3_statement_backend::prepare: no such table: test1 while preparing "DROP TABLE test1".
id: 7
name: John
adiós SOCI
```

## Notes:  
To install conan use `python > 3`

    pip3 install --user conan

Conan version used: `1.31.4`

## Example recipe
`conanfile.txt` using `sqlite3` as backend: 

```
[requires]
sqlite3/3.33.0
soci/4.0.1

[options]
soci:cxx11   = True
soci:sqlite3 = True
soci:empty   = True

[generators]
cmake
```
