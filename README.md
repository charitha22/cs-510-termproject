# Provenance tracking tool using valgrind

This is a minimal implementation of a data provenance tracking algorithm considering only dynamic data dependances. The provenance sources considered are stdin or files. More generally any reads from a given file descriptor is considered a provenance source. The provenance targets are all the program variables. The tool is capable of tracking the data provenance from sources to targets across registers and VEX IR based temporary variables. 

## How to use the tool?

* Install valgrind
* Follow the instructions described in [here](http://www.valgrind.org/docs/manual/writing-tools.html) on setting up new valgrins tools

## Implemenation

The implementation is based on hash maps. We have used seperate hash maps for storing data dependances on memory addreses and on temporary variables. Fore registers we have used valgrind's set_shadow_reg_area and get_shadow_reg_area platforms.

## Author

[Charitha Saumya](https://sites.google.com/site/charithasaumya/)

## Example

For the folowing c program,

#include <stdio.h> \\
#include <unistd.h> \\
int main(){
int a, b, r, x, y, z;
    r = read(STDIN_FILENO, &a, 4);
    r = read(STDIN_FILENO, &b, 4);
    x = b*3;
    y = x-a;
    z = x+y;
    return 0;
}

Output of this tool is,

==22063== ddtector, dynamic data dependance detector
==22063== Copyright (C) 2002-2015, and GNU GPL’d, by Charitha Saumya.
==22063== Using Valgrind-3.12.0 and LibVEX; rerun with -h for copyright info
==22063== Command: ./tc2
==22063==
0xfee01d8c [DD]:
0xfee01d88 [DD]:
0xfee01d84 [DD]:
0xfee01d7c [DD]:
0xfee01d54 [DD]:
0xfee01d44 [DD]:
0xfee01d48 [DD]:
22222222
0xfee01d64 [DD]: \[0xfee01d64:00000032\]
0xfee01d65 [DD]: \[0xfee01d65:00000032\]
0xfee01d66 [DD]: \[0xfee01d66:00000032\]
0xfee01d67 [DD]: \[0xfee01d67:00000032\]
0xfee01d6c [DD]:
0xfee01d54 [DD]:
0xfee01d48 [DD]:
0xfee01d68 [DD]: \[0xfee01d68:00000032\]
0xfee01d69 [DD]: \[0xfee01d69:00000032\]
0xfee01d6a [DD]: \[0xfee01d6a:00000032\]
0xfee01d6b [DD]: \[0xfee01d6b:00000032\]
0xfee01d6c [DD]:
0xfee01d70 [DD]: \[0xfee01d68:00000032\]
0xfee01d74 [DD]: \[0xfee01d68:00000032\] [0xfee01d64:00000032]
0xfee01d78 [DD]: \[0xfee01d68:00000032\] [0xfee01d64:00000032]
0xfee01da0 [DD]:
==22063==

<!--These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.-->

<!--### Prerequisites-->

<!--What things you need to install the software and how to install them-->

<!--```-->
<!--Give examples-->
<!--```-->

<!--### Installing-->

<!--A step by step series of examples that tell you have to get a development env running-->

<!--Say what the step will be-->

<!--```-->
<!--Give the example-->
<!--```-->

<!--And repeat-->

<!--```-->
<!--until finished-->
<!--```-->

<!--End with an example of getting some data out of the system or using it for a little demo-->

<!--## Running the tests-->

<!--Explain how to run the automated tests for this system-->

<!--### Break down into end to end tests-->

<!--Explain what these tests test and why-->

<!--```-->
<!--Give an example-->
<!--```-->

<!--### And coding style tests-->

<!--Explain what these tests test and why-->

<!--```-->
<!--Give an example-->
<!--```-->

<!--## Deployment-->

<!--Add additional notes about how to deploy this on a live system-->

<!--## Built With-->

<!--* [Dropwizard](http://www.dropwizard.io/1.0.2/docs/) - The web framework used-->
<!--* [Maven](https://maven.apache.org/) - Dependency Management-->
<!--* [ROME](https://rometools.github.io/rome/) - Used to generate RSS Feeds-->

<!--## Contributing-->

<!--Please read [CONTRIBUTING.md](https://gist.github.com/PurpleBooth/b24679402957c63ec426) for details on our code of conduct, and the process for submitting pull requests to us.-->

<!--## Versioning-->

<!--We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/your/project/tags). -->

<!--## Authors-->

<!--* **Billie Thompson** - *Initial work* - [PurpleBooth](https://github.com/PurpleBooth)-->

<!--See also the list of [contributors](https://github.com/your/project/contributors) who participated in this project.-->

<!--## License-->

<!--This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details-->

<!--## Acknowledgments-->

<!--* Hat tip to anyone who's code was used-->
<!--* Inspiration-->
<!--* etc-->


