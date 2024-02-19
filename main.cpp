/*
Skeleton code for linear hash indexing
*/

#include <string>
#include <ios>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include "classes.h"
using namespace std;


int main(int argc, char* const argv[]) {

    // Create the index
    LinearHashIndex emp_index("EmployeeIndex");
    emp_index.createFromFile("Employee.csv");
    
    // Loop to lookup IDs until user is ready to quit
<<<<<<< HEAD
    while (true) {
        string input;
        cout << "Enter an ID to lookup: ";
        cin >> input;
        if (input == "q") {
            break;
        }
        int id = stoi(input);
        emp_index.findRecordById(id);
    }
=======
>>>>>>> 48b8bff184732904d1f989bdffdca3939e697ae2
    

    return 0;
}
