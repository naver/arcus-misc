arcus-management-toolkit
========================

## Introduction

Arcus-management-toolkit is a simple management application to provide some useful tools on Arcus cache service.

## Usage

- Unpack and run.
```
$ tar xvf arcus-management-toolkit-$VERSION.tgz
$ cd arcus-management-toolkit-$VERSION/bin
$ ./arcus-management-toolkit
```
- Connect to the server.
```
http://localhost:8080
```

## Building And Packaging

- Install gradle first.
- Then,
```
$ gradle build
$ gradle distTar
```
- You can see the tgz package in build/distributions

## Issues

If you find a bug, please report it via the GitHub issues page.

https://github.com/naver/arcus-management-toolkit/issues

## License

Licensed under the Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0

