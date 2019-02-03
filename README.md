# CDPL: Concurrent and Distributed Programming Library

Collection of useful classes and utilities to kick-start your latest parallel project.

Includes implementations of widely used abstractions like *semaphores*, *mutexes*, *monitors*, *threads*,
*message passing interface (mailbox)* and revamped coordination language *linda* which integrates seamlessly into the modern c++ environment 

Library is *multiplatform* and can be used interchangeably on any operating system as long as compiler is c++1x and STL compliant,
although build script for examples and tests is given only for the Linux environment

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes under **Linux**.
See deployment for notes on how to deploy the project on a live system.

### Prerequisites

C++1x and STL compliant compiler, GNU Make

### Installing

1. Start by cloning this git repository

```
git clone https://github.com/aleksailic/CDPL
```
2. Change your path to newly created folder

```
cd CDPL
```
3. *Optional:* Use the build script to build all/particular example or try creating your own in the designated folder 
using the obvious naming pattern.

```
./build.sh all|project_dir
```
*Note:* Make sure to delete the trailing slash character that bash automatically adds.
## Running the tests/examples

After building all/particular test/examples you can run them from project root directory with command
```
./project_dir/project_name
```
Each example represents a particular CDP problem and is a self-contained executable once compiled.

Note: Each subsystem has its own DEBUG directive to write verbose output for debugging purposes to the DEBUG_STREAM (stdout by default).
This can be very useful when creating your own solution.

## Deployment

The only requirement is the library file contained in CDPL.h that can be statically or dynamically linked (once we release an official version).

Make sure to include necessary licensing/copyright files when deploying your project.

## Contributing

Feel free to submit a pull request or contact me at [aleksa.d.ilic@gmail.com](mailto:aleksa.d.ilic@gmail.com).

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/aleksailic/CDPL/tags). 

## Authors

* **Aleksa Ilić** - *Core work* - [aleksailic](https://github.com/aleksailic)

See also the list of [contributors](https://github.com/aleksailic/CDPL/contributors) who participated in this project.

## License

Library and example/test files are licensed under the MPL 2.0 License - see the [LICENSE](LICENSE) file for details.

## Resources
* Z. Radivojević, I. Ikodinović and Z. Jovanović, Konkurentno i distribuirano programiranje, 2nd ed. Belgrade: Akademska misao, 2018.
* Various StackOverflow threads that provided insight when c++ templating got rough
