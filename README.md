# Student to Topic assignment CLI (using linear optimization)

This C++ program can be used to find an assignment of students to topics using a linear optimization, i.e. a Linear Program (LP).
Students can choose preferences on topics and the program will find the assignment that is globally optimal.
To be more precise, a student that gets assigned their first priority adds e.g. 10 points to the overall score, one that get assigned their second choice only adds 8 points etc. The program then calls the LP solver ```lp_solve``` to maximize the overall score under constraints like that every student gets only assigned one topic. 

## Installation

- **Step 1:** To solve the LP we use an existing program called ```lp_solve```. So first you have to install the lp_solve cli. Using a Debian based system you can e.g. use the following command.
```
sudo apt install lp-solve
```

- **Step 2:** Clone the repository, e.g. with a ssh-based authentification use the command 
```
git clone git@github.com:bencabrera/student-topic-assignment-lp.git
```

- **Step 3:** Go to the cloned folder and run make (of course build-essentials / g++ have to be installed) 
```
make
```

- **Step 4:** Go to the ```bin``` folder to find the binary. To show the help run 
```
./topic_assignment -h
```
or to run the provided example run
```
./topic_assignment ../example_instance/topics.txt ../example_instance/student_preferences.txt  ../example_instance/weights
```


## License

The code is provided free of all warranty via the MIT license (see LICENSE.md).