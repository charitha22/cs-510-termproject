# Provenance tracking tool using valgrind

This is a minimal implementation of a data provenance tracking algorithm considering only dynamic data dependances. The provenance sources considered are stdin or files. More generally any reads from a given file descriptor is considered a provenance source. The provenance targets are all the program variables. The tool is capable of tracking the data provenance from sources to targets across registers and VEX IR based temporary variables. 

## How to use the tool?

* Install valgrind
* Follow the instructions described in [here](http://www.valgrind.org/docs/manual/writing-tools.html) on setting up new valgrins tools

## Implemenation

The implementation is based on hash maps. We have used seperate hash maps for storing data dependances on memory addreses and on temporary variables. For registers we have used valgrind's set_shadow_reg_area and get_shadow_reg_area platforms.

### How provenance sources are handled? 

Since the sources are reads from file descriptors we detect them using syscall APIs in valgrind. When ever there is a read syscall we update the corresponding buffer address of the read as tainted by the same address.

### How shadow registers are updated? 

Whenever there is a register write we update its corresponding shadow location with all the addresses the write depends on. Similarly for a register read we will return the corresponding address list to the reader to update their dependences.

### How loads and stores are handled?

Loads are essentially reading some memory location and updating a temp variable with its content. The corresponding abstract state update for this would be to access the shadow memory and pass the corresponding taint address list to the temp shadow map. A store would be updating a memory address with the content of some temp variable. In this case first we pass the provenance from the temp variable to store address and after that we output (final result of the tool) the address list.

### How ALU operations are handled? 

ALU operations are arithmetic operations on top of temp variables and constants. The result is then assigned to another temp variable. For this we can simply pass the provenance from arguments of the ALU operation to the resultant temp variable.


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


