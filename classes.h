#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <string.h>
#include <sstream>
#include <bitset>
#include <cmath>

using namespace std;

class Record {
public:
    int id, manager_id;
    std::string bio, name;

    Record(vector<std::string> fields) {
        id = stoi(fields[0]);
        name = fields[1];
        bio = fields[2];
        manager_id = stoi(fields[3]);
    }

    void print() {
        cout << "\tID: " << id << "\n";
        cout << "\tNAME: " << name << "\n";
        cout << "\tBIO: " << bio << "\n";
        cout << "\tMANAGER_ID: " << manager_id << "\n";
    }
};


class LinearHashIndex {

private:
    const int BLOCK_SIZE = 4096;

    vector<int> blockDirectory; // Map the least-significant-bits of h(id) to a bucket location in EmployeeIndex (e.g., the jth bucket)
                                // can scan to correct bucket using j*BLOCK_SIZE as offset (using seek function)
								// can initialize to a size of 256 (assume that we will never have more than 256 regular (i.e., non-overflow) buckets)
    int n;  // The number of indexes in blockDirectory currently being used
    int i;	// The number of least-significant-bits of h(id) to check. Will need to increase i once n > 2^i
    int j;  // Used to search the blockdir and find the right page for a given record id
    int numRecords;    // Records currently in index. Used to test whether to increase n
    int nextFreeBlock; // Next place to write a bucket. Should increment it by BLOCK_SIZE whenever a bucket is written to EmployeeIndex
    int nextFreeOverflow;
    string fName;      // Name of index file
    int hashResult;
    string pageBuffer;

    void stringWrite(const std::string& toCopy, char** copyHere, int location) {
    std::copy(toCopy.begin(), toCopy.end(), *copyHere + location);
    }


    // Put a record into the correct block
    void putRecord(Record record){

        string inputBuffer;

        int currReadLocation = j*BLOCK_SIZE;

        // Dynamically calculate the size of the new record
        int newRecordLength = 8 + 8 + record.bio.length() + record.name.length() + 4 + 12;

        while(true)
        {
            ifstream inFile;
            inFile.open(fName, ios::binary);
            // Seek to the correct block to read from
            inFile.seekg(currReadLocation);

            //declare the c style buffer to read in the block
            char* pageDataBuffer = new char[BLOCK_SIZE + 1];
            // Read the block into the buffer
            inFile.read(pageDataBuffer, BLOCK_SIZE);
            // Null terminate the buffer
            pageDataBuffer[BLOCK_SIZE] = '\0';

            inFile.close();

            // Convert to string
            string stringBuffer = pageDataBuffer;
            // Free memory
            delete[] pageDataBuffer;

            //set the buffer to a stringstream and delete the normal string
            stringstream myStrStrm;
            myStrStrm.str(stringBuffer);
            // stringBuffer.clear();

            int slotDirStart = BLOCK_SIZE - 12;
            int sizeOfSlotDir = 0;
            int lastOff = 0;
            int lastLen = 0;

            //determine how much space is currently available in the determined block, go to the slot dir and add up the offsets
            if (getline(myStrStrm, inputBuffer, '[')){
                //if the getline was valid
            
                if(myStrStrm.tellg() != -1){
                    // GEt the position of the slot dir
                    slotDirStart = myStrStrm.tellg();
                    slotDirStart--;
                    sizeOfSlotDir = BLOCK_SIZE - slotDirStart;

                    getline(myStrStrm, inputBuffer, '*');
                    //cout << inputBuffer << endl;
                    lastOff = stoi(inputBuffer);

                    getline(myStrStrm, inputBuffer, ',');
                    getline(myStrStrm, inputBuffer, '*');
                    //cout << inputBuffer << endl;
                    lastLen = stoi(inputBuffer);
                    
                }
                //if the getline was invalid and cause a stringstream fail, reset the stringstream and move on
                /*
                else{
                    myStrStrm.clear();
                    ifstream inFile;
                    inFile.open(fName, ios::binary);
                    inFile.seekg(currReadLocation);

                    //declare the c style buffer to read in the block
                    char* pageDataBuffer = new char[BLOCK_SIZE + 1];
                    inFile.read(pageDataBuffer, BLOCK_SIZE);
                    pageDataBuffer[BLOCK_SIZE] = '\0';

                    inFile.close();

                    //set the buffer to a normal string and delete the c string
                    string stringBuffer = pageDataBuffer;
                    delete[] pageDataBuffer;

                    //set the buffer to a stringstream and delete the normal string
                    stringstream myStrStrm;
                    myStrStrm.str(stringBuffer);
                    stringBuffer.clear();
                }
                */
            }

            //if space, insert
            if((lastOff + lastLen + sizeOfSlotDir + newRecordLength) <= BLOCK_SIZE){
                //put the record in the given block, then push it back onto the file

                //turn the stringstream back into a c string
                stringBuffer = myStrStrm.str();
                myStrStrm.clear();
                char* pageDataBuffer = new char[BLOCK_SIZE + 1];
                pageDataBuffer[BLOCK_SIZE] = '\0';
                strcpy(pageDataBuffer, (stringBuffer).c_str());
                stringBuffer.clear();

                //build record
                string newRecord = to_string(record.id) + '$' + record.name + '$' + record.bio + '$' + to_string(record.manager_id) + "$";
                string newSlot = "[*****,****]";
                //build slot
                newSlot.replace(1, to_string(lastOff + lastLen).length(), to_string(lastOff + lastLen));
                newSlot.replace(7, to_string(newRecord.length()).length(), to_string(newRecord.length()));
                //cout << newSlot << endl;

                //put the new record in
                replaceString(newRecord, &pageDataBuffer, (lastOff + lastLen));
                //put the new slot in
                replaceString(newSlot, &pageDataBuffer, (slotDirStart - 12));

                //send the data into the file
                ofstream outFile;
                outFile.open(fName, ios::in | ios::ate | ios::binary);
                outFile.seekp(currReadLocation);
                outFile.write(pageDataBuffer, 4096);
                outFile.close();

                delete[] pageDataBuffer;

                break;

            }
            //if no space, insert into overflow
            else{
                //check if we currently already have a valid file pointer
                myStrStrm.seekg(BLOCK_SIZE-11);
                //getline(myStrStrm, inputBuffer, '{');
                getline(myStrStrm, inputBuffer, '}');

                int overflowPointer = stoi(inputBuffer);

                //No active pointer on this page so the next one needs to be created
                if(overflowPointer == 0)
                {
                    //create an overflow page and store it there
                    stringBuffer = myStrStrm.str();
                    myStrStrm.clear();
                    
                    //create a buffer to write the new page
                    char* pageDataBuffer = new char[BLOCK_SIZE + 1];
                    pageDataBuffer[BLOCK_SIZE] = '\0';
                    strcpy(pageDataBuffer, (stringBuffer).c_str());
                    stringBuffer.clear();

                    
                    //update the pointer in the current page, and write it to the file

                    string newPointer = to_string(nextFreeOverflow);
                    nextFreeOverflow += BLOCK_SIZE;

                    int placementLength = (BLOCK_SIZE-(newPointer.length()+1));

                    replaceString(newPointer, &pageDataBuffer, placementLength);

                    //seek to the correct position in the file
                    //Write contents to the file
                    ofstream outFile;
                    outFile.open(fName, ios::in | ios::ate | ios::binary);
                    outFile.seekp(currReadLocation);
                    outFile.write(pageDataBuffer, BLOCK_SIZE);

                    string filePointer = "{0000000000}";
                    memset(pageDataBuffer, ' ', BLOCK_SIZE);
                    replaceString(filePointer, &pageDataBuffer, 4084);

                    outFile.seekp(stoi(newPointer));
                    outFile.write(pageDataBuffer, BLOCK_SIZE);

                    outFile.close();
                    //update read location
                    currReadLocation = stoi(newPointer);

                    delete[] pageDataBuffer;
                }else
                {
                    //move past the current page
                    currReadLocation = overflowPointer; 
                }
            }
            
        }

    }

    // Insert new record into index
    void insertRecord(Record record) {
        
        /*
        //the size needed to store the new record and its corresponding slot dir slot
        int newRecordLength = 8 + 8 + record.bio.length() + record.name.length() + 4 + 12;
        */

        // No records written to index yet
        if (numRecords == 0) {
            // Initialize index with first blocks (start with 4)
            blockDirectory.push_back(00);
            blockDirectory.push_back(01);
            blockDirectory.push_back(10);
            blockDirectory.push_back(11);

            nextFreeBlock = 4*BLOCK_SIZE;
            nextFreeOverflow = 4096*256;

        }

        // Add record to the index in the correct block, creating a overflow block if necessary
        numRecords++;

        //compute the hash function "id mod 216"
        int hashResult = record.id%216;
        int binResult = 0;
        int modResult = 0;
        int divResult = hashResult;

        //convert to binary and grab i least significant bits
        for(int k = 0; k < i; k++){
            //build an int that looks like binary
            modResult = divResult%2;
            divResult = divResult/2;

            binResult += pow(10,k) * modResult;

        }

        //check for bit flip
        if(binResult > blockDirectory.at(n-1)){
            //perform the bit flip
            int xorFella = pow(10, i-1);
            binResult ^= xorFella;
        }

        //based on the binary result, determine which block to go to, j represents the block number
        for(int k = 0; k < n; k++){
            if(blockDirectory.at(k) == binResult){
                j = k;
                // cout << "I go in page: " << j << endl;
            }
        }

        //open the file and seek to the desired block
       
        //for processing small bits of data;
        string inputBuffer;

        //put the record in the correct page
        putRecord(record);

        
        
        // Take neccessary steps if capacity is reached:
		// increase n; increase i (if necessary); place records in the new bucket that may have been originally misplaced due to a bit flip

        //check whether the average number of records per page exceeds 70% capacity, if so, increase n
        //730 represents the max size of a record with its data plus slot dir
        if((numRecords * 730) > (.7 * n * 4096)){
            n++;
            nextFreeBlock += BLOCK_SIZE;

            //check if i needs to increase
            if(pow(2,i) < n){
                i++;
            }
            //update the blockdir
            divResult = n-1;
            binResult = 0;
            int k = 0;

            while(divResult != 0){
                //build an int that looks like binary
                modResult = divResult%2;
                divResult = divResult/2;

                binResult += pow(10,k) * modResult;
                k++;

            }
            
            //update the block dir
            blockDirectory.push_back(binResult);

            int allmyfellascounter = 1;

            //for each current page in the file
            for(int l = 0; l < n-1; l++)
            {
                int slotCounter = 0;
                int overFlowCounter = 0;

                while(true){
                    //open the file and snatch a page
                    ifstream inFile;
                    inFile.open(fName, ios::binary);

                    if(overFlowCounter == 0){
                        inFile.seekg(BLOCK_SIZE*l);
                    }
                    else{
                        inFile.seekg(overFlowCounter);
                    }

                    //convert the c string to a silly stringstream teehee
                    char* pageDataBuffer = new char[BLOCK_SIZE + 1];
                    inFile.read(pageDataBuffer, BLOCK_SIZE);
                    pageDataBuffer[BLOCK_SIZE] = '\0';
                    string stringBuffer = pageDataBuffer;
                    delete[] pageDataBuffer;
                    stringstream myStrStrm;
                    myStrStrm.str(stringBuffer);
                    stringBuffer.clear();

                    inFile.close();

                    //establish the first slot position
                    if(slotCounter == 0){
                        if (getline(myStrStrm, inputBuffer, '[')){
                            //we found a slot
                            if(myStrStrm.tellg() != -1){


                                //take account of the position
                                slotCounter = myStrStrm.tellg();
                                int slotPosition = myStrStrm.tellg();
                                slotPosition--;


                                //operate on the record

                                //grab the record offset and seek to the record
                                getline(myStrStrm, inputBuffer, '*');
                                int recordPosition = stoi(inputBuffer);
                                myStrStrm.seekg(stoi(inputBuffer));
                                //grab the id
                                getline(myStrStrm, inputBuffer, '$');
                                string id = inputBuffer;
                                //grab the name
                                getline(myStrStrm, inputBuffer, '$');
                                string name = inputBuffer;
                                //grab the bio
                                getline(myStrStrm, inputBuffer, '$');
                                string bio = inputBuffer;
                                //grab the managerid
                                getline(myStrStrm, inputBuffer, '$');
                                string manid = inputBuffer;

                                //make the record and send it into the file in the new location
                                vector<string> myVector{id, name, bio, manid};
                                Record moveRecord(myVector);

                                //hash it
                                int hashResult = moveRecord.id%216;
                                int binResult = 0;
                                int modResult = 0;
                                int divResult = hashResult;

                                //convert to binary and grab i least significant bits
                                for(int k = 0; k < i; k++){
                                    //build an int that looks like binary
                                    modResult = divResult%2;
                                    divResult = divResult/2;

                                    binResult += pow(10,k) * modResult;

                                }

                                //check for bit flip
                                if(binResult > blockDirectory.at(n-1)){
                                    //perform the bit flip
                                    int xorFella = pow(10, i-1);
                                    binResult ^= xorFella;
                                }

                                //check if it needs to be moved
                                if(binResult != blockDirectory.at(j)){
                                    //find the correct block
                                    for(int k = 0; k < n; k++){
                                        if(blockDirectory.at(k) == binResult){
                                            j = k;
                                        }
                                    }

                                    //delete the record and slot from the original location

                                    //turn the stringstream back into a c string
                                    stringBuffer = myStrStrm.str();
                                    myStrStrm.clear();
                                    char* pageDataBuffer = new char[BLOCK_SIZE + 1];
                                    pageDataBuffer[BLOCK_SIZE] = '\0';
                                    strcpy(pageDataBuffer, (stringBuffer).c_str());
                                    stringBuffer.clear();

                                    //build blank record and slot
                                    int blankRecordLength = 8 + 8 + 4 + moveRecord.name.length() + moveRecord.bio.length();
                                    string blankRecord;
                                    blankRecord.append(blankRecordLength, ' ');
                                    string blankSlot = "            ";

                                    //put the new record in
                                    replaceString(blankRecord, &pageDataBuffer, recordPosition);
                                    //put the new slot in
                                    replaceString(blankSlot, &pageDataBuffer, slotPosition);

                                    //send the data into the file
                                    ofstream outFile;
                                    outFile.open(fName, ios::in | ios::ate | ios::binary);
                                    if(overFlowCounter == 0){
                                        outFile.seekp(BLOCK_SIZE*l);
                                    }
                                    else{
                                        outFile.seekp(overFlowCounter);
                                    }
                                    outFile.write(pageDataBuffer, 4096);
                                    outFile.close();

                                    delete[] pageDataBuffer;

                                    //move the record
                                    putRecord(moveRecord);
                                }
                                

                            }
                            //there are no slots
                            else{
                                //reset the stringstream since the error bit got set
                                myStrStrm.clear();
                                ifstream inFile;
                                inFile.open(fName, ios::binary);
                                //find the correct place to reset at
                                if(overFlowCounter == 0){
                                    inFile.seekg(BLOCK_SIZE*l);
                                }
                                else{
                                    inFile.seekg(overFlowCounter);
                                }
                                //convert the c string to a silly stringstream teehee
                                char* pageDataBuffer = new char[BLOCK_SIZE + 1];
                                inFile.read(pageDataBuffer, BLOCK_SIZE);
                                pageDataBuffer[BLOCK_SIZE] = '\0';
                                string stringBuffer = pageDataBuffer;
                                delete[] pageDataBuffer;
                                stringstream myStrStrm;
                                myStrStrm.str(stringBuffer);
                                stringBuffer.clear();

                                inFile.close();
                                //grab the overflow pointer
                                myStrStrm.seekg(BLOCK_SIZE-11);
                                getline(myStrStrm, inputBuffer, '}');
                                int overflowPointer = stoi(inputBuffer);

                                //check for overflow page
                                if(overflowPointer == 0){
                                    myStrStrm.clear();
                                    break;
                                }
                                else{
                                    overFlowCounter = overflowPointer;
                                    slotCounter = 0;
                                }

                            }
                        }
                    }
                    //we already found a slot in this page
                    else{
                        //change the slot counter by 12, 
                        slotCounter += 12;
                        //if we're at 4084, we're at the end of the page
                        if(slotCounter == 4085){
                            //grab the overflow pointer
                            myStrStrm.seekg(BLOCK_SIZE-11);
                            getline(myStrStrm, inputBuffer, '}');
                            int overflowPointer = stoi(inputBuffer);

                            if(overflowPointer == 0){
                                myStrStrm.clear();
                                break;
                            }
                            else{
                                overFlowCounter = overflowPointer;
                                slotCounter = 0;
                            }
                        }
                        else{
                            //check if it's spaces
                            myStrStrm.seekg(slotCounter);

                            //if not spaces, grab the record and operate on it
                            if((myStrStrm.str())[slotCounter] != ' '){
                                //hash it

                                int slotPosition = myStrStrm.tellg();
                                slotPosition--;
                                
                                //operate on the record

                                //grab the record offset and seek to the record
                                getline(myStrStrm, inputBuffer, '*');
                                myStrStrm.seekg(stoi(inputBuffer));
                                int recordPosition = stoi(inputBuffer);
                                //grab the id
                                getline(myStrStrm, inputBuffer, '$');
                                string id = inputBuffer;
                                //grab the name
                                getline(myStrStrm, inputBuffer, '$');
                                string name = inputBuffer;
                                //grab the bio
                                getline(myStrStrm, inputBuffer, '$');
                                string bio = inputBuffer;
                                //grab the managerid
                                getline(myStrStrm, inputBuffer, '$');
                                string manid = inputBuffer;

                                //make the record and send it into the file in the new location
                                vector<string> myVector{id, name, bio, manid};
                                Record moveRecord(myVector);

                                //hash it
                                int hashResult = moveRecord.id%216;
                                int binResult = 0;
                                int modResult = 0;
                                int divResult = hashResult;

                                //convert to binary and grab i least significant bits
                                for(int k = 0; k < i; k++){
                                    //build an int that looks like binary
                                    modResult = divResult%2;
                                    divResult = divResult/2;

                                    binResult += pow(10,k) * modResult;

                                }

                                //check for bit flip
                                if(binResult > blockDirectory.at(n-1)){
                                    //perform the bit flip
                                    int xorFella = pow(10, i-1);
                                    binResult ^= xorFella;
                                }

                                //check if it needs to be moved
                                if(binResult != blockDirectory.at(j)){
                                    //find the correct block
                                    for(int k = 0; k < n; k++){
                                        if(blockDirectory.at(k) == binResult){
                                            j = k;
                                        }
                                    }

                                    //delete it from the original location

                                    //turn the stringstream back into a c string
                                    stringBuffer = myStrStrm.str();
                                    myStrStrm.clear();
                                    char* pageDataBuffer = new char[BLOCK_SIZE + 1];
                                    pageDataBuffer[BLOCK_SIZE] = '\0';
                                    strcpy(pageDataBuffer, (stringBuffer).c_str());
                                    stringBuffer.clear();

                                    //build blank record and slot
                                    int blankRecordLength = 8 + 8 + 4 + moveRecord.name.length() + moveRecord.bio.length();
                                    string blankRecord;
                                    blankRecord.append(blankRecordLength, ' ');
                                    string blankSlot = "            ";

                                    //put the new record in
                                    replaceString(blankRecord, &pageDataBuffer, recordPosition);
                                    //put the new slot in
                                    replaceString(blankSlot, &pageDataBuffer, slotPosition);

                                    //send the data into the file
                                    ofstream outFile;
                                    outFile.open(fName, ios::in | ios::ate | ios::binary);
                                    if(overFlowCounter == 0){
                                        outFile.seekp(BLOCK_SIZE*l);
                                    }
                                    else{
                                        outFile.seekp(overFlowCounter);
                                    }
                                    outFile.write(pageDataBuffer, 4096);
                                    outFile.close();

                                    delete[] pageDataBuffer;

                                    //move the record
                                    putRecord(moveRecord);
                                }
                                
                            }

                        }

                    }

                }

            }
            
        }
        
    }

public:
    LinearHashIndex(string indexFileName) {
        n = 4; // Start with 4 buckets in index
        i = 2; // Need 2 bits to address 4 buckets
        numRecords = 0;
        nextFreeBlock = 0;
        fName = indexFileName;
        // Create your EmployeeIndex file and write out the initial 4 buckets
        // make sure to account for the created buckets by incrementing nextFreeBlock appropriately

        //open the index file
        ofstream outFile;
        outFile.open(indexFileName, ios::binary);

        //make a new page, and put the empty pointer at the end
        char* newPage = new char[4096];
        string filePointer = "{0000000000}";
        memset(newPage, ' ', 4096);
        replaceString(filePointer, &newPage, 4084);

        //declare buffer and fill with spaces
        for(int i = 0; i < 256; i++){
            outFile.write(newPage, 4096);
        }

        delete[] newPage;
        outFile.close();
      
    }

    // Read csv file and add records to the index
    void createFromFile(string csvFName) {

        //open the input file
        ifstream inputFile;
        inputFile.open(csvFName);

        //start reading from the file
        string curLine;
        string id;
        string manid;
        string name;
        string bio;

        
        //loop until all records have been sent to the insert function
        while(true){
            
            //process the id element
            if(getline(inputFile, curLine, ',')){
                id = curLine;
                
                //process the name element
                getline(inputFile, curLine, ',');
                name = curLine;
    
                //process the bio element
                getline(inputFile, curLine, ',');
                bio = curLine;
    
                //process the manager id element
                getline(inputFile, curLine);
                manid = curLine;
                
                vector<string> myVector{id, name, bio, manid};
                Record myRecord(myVector);
    
                //send the record to insertRecord
                insertRecord(myRecord);
                
            }
            //if the getline is invalid, that means there is no new line, break out and close the input file
            else{
                break;
            }

        }

        //close the input file
        inputFile.close();
    }

    // Given an ID, find the relevant record and print it
    void findRecordById(int searchId) {

        //hash the id
        //compute the hash function "id mod 216"
        int hashResult = searchId%216;
        int binResult = 0;
        int modResult = 0;
        int divResult = hashResult;

        //convert to binary and grab i least significant bits
        for(int k = 0; k < i; k++){
            //build an int that looks like binary
            modResult = divResult%2;
            divResult = divResult/2;

            binResult += pow(10,k) * modResult;

        }

        //check for bit flip
        if(binResult > blockDirectory.at(n-1)){
            //perform the bit flip
            int xorFella = pow(10, i-1);
            binResult ^= xorFella;
        }

        //based on the binary result, determine which block to go to, j represents the block number
        for(int k = 0; k < n; k++){
            if(blockDirectory.at(k) == binResult){
                j = k;
            }
        }

        //traverse the pages until you find the end
        int slotCounter = 0;
        int overFlowCounter = 0;

        while(true){
            //open the file and snatch a page
            ifstream inFile;
            inFile.open(fName, ios::binary);

            if(overFlowCounter == 0){
                inFile.seekg(BLOCK_SIZE*j);
            }
            else{
                inFile.seekg(overFlowCounter);
            }

            //convert the c string to a silly stringstream teehee
            char* pageDataBuffer = new char[BLOCK_SIZE + 1];
            inFile.read(pageDataBuffer, BLOCK_SIZE);
            pageDataBuffer[BLOCK_SIZE] = '\0';
            string stringBuffer = pageDataBuffer;
            delete[] pageDataBuffer;
            stringstream myStrStrm;
            myStrStrm.str(stringBuffer);
            stringBuffer.clear();

            inFile.close();

            string inputBuffer;

            //establish the first slot position
            if(slotCounter == 0){

                if (getline(myStrStrm, inputBuffer, '[')){
                    //we found a slot
                    if(myStrStrm.tellg() != -1){

                        //take account of the position
                        slotCounter = myStrStrm.tellg();
                        int slotPosition = myStrStrm.tellg();
                        slotPosition--;

                        //operate on the record

                        //grab the record offset and seek to the record
                        getline(myStrStrm, inputBuffer, '*');
                        int recordPosition = stoi(inputBuffer);
                        myStrStrm.seekg(stoi(inputBuffer));
                        //grab the id
                        getline(myStrStrm, inputBuffer, '$');
                        string id = inputBuffer;
                        //grab the name
                        getline(myStrStrm, inputBuffer, '$');
                        string name = inputBuffer;
                        //grab the bio
                        getline(myStrStrm, inputBuffer, '$');
                        string bio = inputBuffer;
                        //grab the managerid
                        getline(myStrStrm, inputBuffer, '$');
                        string manid = inputBuffer;

                        //print
                        if(stoi(id) == searchId){
                            //print
                            vector<string> myVector{id, name, bio, manid};
                            Record myRecord(myVector);

                            myRecord.print();
                        }

                    }
                    //there are no slots
                    else{
                        //reset the stringstream since the error bit got set
                        myStrStrm.clear();
                        ifstream inFile;
                        inFile.open(fName, ios::binary);
                        //find the correct place to reset at
                        if(overFlowCounter == 0){
                            inFile.seekg(BLOCK_SIZE*j);
                        }
                        else{
                            inFile.seekg(overFlowCounter);
                        }
                        //convert the c string to a silly stringstream teehee
                        char* pageDataBuffer = new char[BLOCK_SIZE + 1];
                        inFile.read(pageDataBuffer, BLOCK_SIZE);
                        pageDataBuffer[BLOCK_SIZE] = '\0';
                        string stringBuffer = pageDataBuffer;
                        delete[] pageDataBuffer;
                        stringstream myStrStrm;
                        myStrStrm.str(stringBuffer);
                        stringBuffer.clear();

                        inFile.close();
                        //grab the overflow pointer
                        myStrStrm.seekg(BLOCK_SIZE-11);
                        getline(myStrStrm, inputBuffer, '}');
                        int overflowPointer = stoi(inputBuffer);

                        //check for overflow page
                        if(overflowPointer == 0){
                            myStrStrm.clear();
                            break;
                        }
                        else{
                            overFlowCounter = overflowPointer;
                            slotCounter = 0;
                        }

                    }
                }
            }
            //we already found a slot in this page
            else{
                //change the slot counter by 12, 
                slotCounter += 12;
                //if we're at 4084, we're at the end of the page
                if(slotCounter == 4085){
                    //grab the overflow pointer
                    myStrStrm.seekg(BLOCK_SIZE-11);
                    getline(myStrStrm, inputBuffer, '}');
                    int overflowPointer = stoi(inputBuffer);

                    if(overflowPointer == 0){
                        myStrStrm.clear();
                        break;
                    }
                    else{
                        overFlowCounter = overflowPointer;
                        slotCounter = 0;
                    }
                }
                else{
                    //check if it's spaces
                    myStrStrm.seekg(slotCounter);

                    //if not spaces, grab the record and operate on it
                    if((myStrStrm.str())[slotCounter] != ' '){
                        //hash it

                        int slotPosition = myStrStrm.tellg();
                        slotPosition--;
                        
                        //operate on the record
                        //grab the record offset and seek to the record
                        getline(myStrStrm, inputBuffer, '*');
                        myStrStrm.seekg(stoi(inputBuffer));
                        int recordPosition = stoi(inputBuffer);
                        //grab the id
                        getline(myStrStrm, inputBuffer, '$');
                        string id = inputBuffer;
                        //grab the name
                        getline(myStrStrm, inputBuffer, '$');
                        string name = inputBuffer;
                        //grab the bio
                        getline(myStrStrm, inputBuffer, '$');
                        string bio = inputBuffer;
                        //grab the managerid
                        getline(myStrStrm, inputBuffer, '$');
                        string manid = inputBuffer;

                        if(stoi(id) == searchId){
                            //print
                            vector<string> myVector{id, name, bio, manid};
                            Record myRecord(myVector);

                            myRecord.print();

                        }

                    }

                }

            }

        }

    }
};