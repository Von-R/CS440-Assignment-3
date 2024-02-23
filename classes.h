#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <fstream> // Add missing include statement for <fstream>
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
    int nextOverflowBucket;
    string fName;      // Name of index file
    int hashID;
    
    void stringWrite(const string& toCopy, char** copyHere, int location) {
        memcpy(*copyHere + location, toCopy.c_str(), toCopy.length());
    }

    void insertRecordIntoIndexIntoBlock(Record record){

      
        int readBlock = j*BLOCK_SIZE;
        string offsetBuffer;

        //the size needed to store the new record and its corresponding slot dir slot
        int recordSize = 32 + record.bio.length() + record.name.length();

        while(true)
        {
            ifstream inputFile;
            inputFile.open(fName, ios::binary);
            inputFile.seekg(readBlock);

            string stringBuffer;
            stringBuffer.resize(BLOCK_SIZE);
            inputFile.read(&stringBuffer[0], BLOCK_SIZE);
            inputFile.close();

            //set the buffer to a stringstream and delete the normal string
            stringstream strStream;
            strStream.str(stringBuffer);
            stringBuffer.clear();

            int recordDirectoryOffset = BLOCK_SIZE - 12;
            int recDirSize = 0;
            int recentOffset = 0;
            int recentLength = 0;

            //determine how much space is currently available in the determined block, go to the slot dir and add up the offsets
            if (getline(strStream, offsetBuffer, '[')){
                //if the getline was valid
                if(strStream.tellg() != -1){
                    recordDirectoryOffset = strStream.tellg();
                    recordDirectoryOffset--;
                    recDirSize = BLOCK_SIZE - recordDirectoryOffset;

                    getline(strStream, offsetBuffer, '*');
                    recentOffset = stoi(offsetBuffer);

                    getline(strStream, offsetBuffer, ',');
                    getline(strStream, offsetBuffer, '*');
                    recentLength = stoi(offsetBuffer);
                    
                }
                //if the getline was invalid and cause a stringstream fail, reset the stringstream and move on
                else{
                    ifstream inputFile(fName, ios::binary);
                    inputFile.seekg(readBlock);

                    string stringBuffer;
                    stringBuffer.resize(BLOCK_SIZE);
                    inputFile.read(&stringBuffer[0], BLOCK_SIZE);
                    inputFile.close();

                    stringstream strStream(stringBuffer);
                    // No need to clear stringBuffer since we're reassigning it in the next line

                }
            }

            //if space, insert
            if((recentOffset + recentLength + recDirSize + recordSize) <= BLOCK_SIZE){
                //put the record in the given block, then push it back onto the file

                //turn the stringstream back into a c string
                stringBuffer = strStream.str();
                strStream.clear();
                char* blockBuffer = new char[BLOCK_SIZE + 1];
                blockBuffer[BLOCK_SIZE] = '\0';
                strcpy(blockBuffer, (stringBuffer).c_str());
               // stringBuffer.clear();

                //build record
                string newRecord = to_string(record.id) + '$' + record.name + '$' + record.bio + '$' + to_string(record.manager_id) + "$";
                string newSlot = "[*****,****]";
                //build slot
                newSlot.replace(1, to_string(recentOffset + recentLength).length(), to_string(recentOffset + recentLength));
                newSlot.replace(7, to_string(newRecord.length()).length(), to_string(newRecord.length()));
                //cout << newSlot << endl;

                //put the new record in
                stringWrite(newRecord, &blockBuffer, (recentOffset + recentLength));
                //put the new slot in
                stringWrite(newSlot, &blockBuffer, (recordDirectoryOffset - 12));

                //send the data into the file
                ofstream outputFile;
                outputFile.open(fName, ios::in | ios::ate | ios::binary);
                outputFile.seekp(readBlock);
                outputFile.write(blockBuffer, 4096);
                outputFile.close();

                delete[] blockBuffer;

                break;

            }
            //if no space, insert into overflow
            else{
                //check if we currently already have a valid file pointer
                strStream.seekg(BLOCK_SIZE-11);
                //getline(strStream, offsetBuffer, '{');
                getline(strStream, offsetBuffer, '}');

                int ptrOverflowBucket = stoi(offsetBuffer);

                //No active pointer on this page so the next one needs to be created
                if(ptrOverflowBucket == 0)
                {
                    //create an overflow page and store it there
                    stringBuffer = strStream.str();
                    strStream.clear();
                    
                    //create a buffer to write the new page
                    char* blockBuffer = new char[BLOCK_SIZE + 1];
                    blockBuffer[BLOCK_SIZE] = '\0';
                    strcpy(blockBuffer, (stringBuffer).c_str());
                    stringBuffer.clear();

                    
                    //update the pointer in the current page, and write it to the file

                    string tmpPtr = to_string(nextOverflowBucket);
                    nextOverflowBucket += BLOCK_SIZE;

                    int overflowBlockDist = (BLOCK_SIZE-(tmpPtr.length()+1));

                    stringWrite(tmpPtr, &blockBuffer, overflowBlockDist);

                    //seek to the correct position in the file
                    //Write contents to the file
                    ofstream outputFile;
                    outputFile.open(fName, ios::in | ios::ate | ios::binary);
                    outputFile.seekp(readBlock);
                    outputFile.write(blockBuffer, BLOCK_SIZE);

                    string filePointer = "{0000000000}";
                    memset(blockBuffer, ' ', BLOCK_SIZE);
                    stringWrite(filePointer, &blockBuffer, 4084);

                    outputFile.seekp(stoi(tmpPtr));
                    outputFile.write(blockBuffer, BLOCK_SIZE);

                    outputFile.close();
                    //update read location
                    readBlock = stoi(tmpPtr);

                    delete[] blockBuffer;
                }
                
                else {
                    //move past the current page
                    readBlock = ptrOverflowBucket; 
                }
            }
            
        }

    }

    // Insert new record into index
    void insertRecordIntoIndex(Record record) {
        
        /*
        //the size needed to store the new record and its corresponding slot dir slot
        int recordSize = 8 + 8 + record.bio.length() + record.name.length() + 4 + 12;
        */

        // No records written to index yet
        if (numRecords == 0) {
            // Initialize index with first blocks (start with 4)
            blockDirectory.push_back(00);
            blockDirectory.push_back(01);
            blockDirectory.push_back(10);
            blockDirectory.push_back(11);

            nextFreeBlock = 4*BLOCK_SIZE;
            nextOverflowBucket = 4096*256;

        }

        // Add record to the index in the correct block, creating a overflow block if necessary
        numRecords++;

        //compute the hash function "id mod 216"
        int hashID = record.id%216;
        int binResult = 0;
        int modResult = 0;
        int divResult = hashID;

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
            binResult = binResult - xorFella;
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
        insertRecordIntoIndexIntoBlock(record);

        
        
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
                    ifstream inputFile;
                    inputFile.open(fName, ios::binary);

                    if(overFlowCounter == 0){
                        inputFile.seekg(BLOCK_SIZE*l);
                    }
                    else{
                        inputFile.seekg(overFlowCounter);
                    }

                    //convert the c string to a silly stringstream teehee
                    char* blockBuffer = new char[BLOCK_SIZE + 1];
                    inputFile.read(blockBuffer, BLOCK_SIZE);
                    blockBuffer[BLOCK_SIZE] = '\0';
                    string stringBuffer = blockBuffer;
                    delete[] blockBuffer;
                    stringstream strStream;
                    strStream.str(stringBuffer);
                    stringBuffer.clear();

                    inputFile.close();

                    //establish the first slot position
                    if(slotCounter == 0){
                        if (getline(strStream, inputBuffer, '[')){
                            //we found a slot
                            if(strStream.tellg() != -1){


                                //take account of the position
                                slotCounter = strStream.tellg();
                                int slotPosition = strStream.tellg();
                                slotPosition--;


                                //operate on the record

                                //grab the record offset and seek to the record
                                getline(strStream, inputBuffer, '*');
                                int recordPosition = stoi(inputBuffer);
                                strStream.seekg(stoi(inputBuffer));
                                //grab the id
                                getline(strStream, inputBuffer, '$');
                                string id = inputBuffer;
                                //grab the name
                                getline(strStream, inputBuffer, '$');
                                string name = inputBuffer;
                                //grab the bio
                                getline(strStream, inputBuffer, '$');
                                string bio = inputBuffer;
                                //grab the managerid
                                getline(strStream, inputBuffer, '$');
                                string manid = inputBuffer;

                                //make the record and send it into the file in the new location
                                vector<string> myVector{id, name, bio, manid};
                                Record moveRecord(myVector);

                                //hash it
                                int hashID = moveRecord.id%216;
                                int binResult = 0;
                                int modResult = 0;
                                int divResult = hashID;

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
                                    binResult = binResult - xorFella;
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
                                    stringBuffer = strStream.str();
                                    strStream.clear();
                                    char* blockBuffer = new char[BLOCK_SIZE + 1];
                                    blockBuffer[BLOCK_SIZE] = '\0';
                                    strcpy(blockBuffer, (stringBuffer).c_str());
                                    stringBuffer.clear();

                                    //build blank record and slot
                                    int blankRecordLength = 8 + 8 + 4 + moveRecord.name.length() + moveRecord.bio.length();
                                    string blankRecord;
                                    blankRecord.append(blankRecordLength, ' ');
                                    string blankSlot = "            ";

                                    //put the new record in
                                    stringWrite(blankRecord, &blockBuffer, recordPosition);
                                    //put the new slot in
                                    stringWrite(blankSlot, &blockBuffer, slotPosition);

                                    //send the data into the file
                                    ofstream outputFile;
                                    outputFile.open(fName, ios::in | ios::ate | ios::binary);
                                    if(overFlowCounter == 0){
                                        outputFile.seekp(BLOCK_SIZE*l);
                                    }
                                    else{
                                        outputFile.seekp(overFlowCounter);
                                    }
                                    outputFile.write(blockBuffer, 4096);
                                    outputFile.close();

                                    delete[] blockBuffer;

                                    //move the record
                                    insertRecordIntoIndexIntoBlock(moveRecord);
                                }
                                

                            }
                            //there are no slots
                            else{
                                //reset the stringstream since the error bit got set
                                strStream.clear();
                                ifstream inputFile;
                                inputFile.open(fName, ios::binary);
                                //find the correct place to reset at
                                if(overFlowCounter == 0){
                                    inputFile.seekg(BLOCK_SIZE*l);
                                }
                                else{
                                    inputFile.seekg(overFlowCounter);
                                }
                                //convert the c string to a silly stringstream teehee
                                char* blockBuffer = new char[BLOCK_SIZE + 1];
                                inputFile.read(blockBuffer, BLOCK_SIZE);
                                blockBuffer[BLOCK_SIZE] = '\0';
                                string stringBuffer = blockBuffer;
                                delete[] blockBuffer;
                                stringstream strStream;
                                strStream.str(stringBuffer);
                                stringBuffer.clear();

                                inputFile.close();
                                //grab the overflow pointer
                                strStream.seekg(BLOCK_SIZE-11);
                                getline(strStream, inputBuffer, '}');
                                int ptrOverflowBucket = stoi(inputBuffer);

                                //check for overflow page
                                if(ptrOverflowBucket == 0){
                                    strStream.clear();
                                    break;
                                }
                                else{
                                    overFlowCounter = ptrOverflowBucket;
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
                            strStream.seekg(BLOCK_SIZE-11);
                            getline(strStream, inputBuffer, '}');
                            int ptrOverflowBucket = stoi(inputBuffer);

                            if(ptrOverflowBucket == 0){
                                strStream.clear();
                                break;
                            }
                            else{
                                overFlowCounter = ptrOverflowBucket;
                                slotCounter = 0;
                            }
                        }
                        else{
                            //check if it's spaces
                            strStream.seekg(slotCounter);

                            //if not spaces, grab the record and operate on it
                            if((strStream.str())[slotCounter] != ' '){
                                //hash it

                                int slotPosition = strStream.tellg();
                                slotPosition--;
                                
                                //operate on the record

                                //grab the record offset and seek to the record
                                getline(strStream, inputBuffer, '*');
                                strStream.seekg(stoi(inputBuffer));
                                int recordPosition = stoi(inputBuffer);
                                //grab the id
                                getline(strStream, inputBuffer, '$');
                                string id = inputBuffer;
                                //grab the name
                                getline(strStream, inputBuffer, '$');
                                string name = inputBuffer;
                                //grab the bio
                                getline(strStream, inputBuffer, '$');
                                string bio = inputBuffer;
                                //grab the managerid
                                getline(strStream, inputBuffer, '$');
                                string manid = inputBuffer;

                                //make the record and send it into the file in the new location
                                vector<string> myVector{id, name, bio, manid};
                                Record moveRecord(myVector);

                                //hash it
                                int hashID = moveRecord.id%216;
                                int binResult = 0;
                                int modResult = 0;
                                int divResult = hashID;

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
                                    binResult = binResult - xorFella;
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
                                    stringBuffer = strStream.str();
                                    strStream.clear();
                                    char* blockBuffer = new char[BLOCK_SIZE + 1];
                                    blockBuffer[BLOCK_SIZE] = '\0';
                                    strcpy(blockBuffer, (stringBuffer).c_str());
                                    stringBuffer.clear();

                                    //build blank record and slot
                                    int blankRecordLength = 8 + 8 + 4 + moveRecord.name.length() + moveRecord.bio.length();
                                    string blankRecord;
                                    blankRecord.append(blankRecordLength, ' ');
                                    string blankSlot = "            ";

                                    //put the new record in
                                    stringWrite(blankRecord, &blockBuffer, recordPosition);
                                    //put the new slot in
                                    stringWrite(blankSlot, &blockBuffer, slotPosition);

                                    //send the data into the file
                                    ofstream outputFile;
                                    outputFile.open(fName, ios::in | ios::ate | ios::binary);
                                    if(overFlowCounter == 0){
                                        outputFile.seekp(BLOCK_SIZE*l);
                                    }
                                    else{
                                        outputFile.seekp(overFlowCounter);
                                    }
                                    outputFile.write(blockBuffer, 4096);
                                    outputFile.close();

                                    delete[] blockBuffer;

                                    //move the record
                                    insertRecordIntoIndexIntoBlock(moveRecord);
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
        ofstream outputFile;
        outputFile.open(indexFileName, ios::binary);

        //make a new page, and put the empty pointer at the end
        char* newPage = new char[4096];
        string filePointer = "{0000000000}";
        memset(newPage, ' ', 4096);
        stringWrite(filePointer, &newPage, 4084);

        //declare buffer and fill with spaces
        for(int i = 0; i < 256; i++){
            outputFile.write(newPage, 4096);
        }

        delete[] newPage;
        outputFile.close();
      
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
    
                //send the record to insertRecordIntoIndex
                insertRecordIntoIndex(myRecord);
                
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
        int hashID = searchId%216;
        int binResult = 0;
        int modResult = 0;
        int divResult = hashID;

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
            binResult = binResult - xorFella;
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
            ifstream inputFile;
            inputFile.open(fName, ios::binary);

            if(overFlowCounter == 0){
                inputFile.seekg(BLOCK_SIZE*j);
            }
            else{
                inputFile.seekg(overFlowCounter);
            }

            //convert the c string to a silly stringstream teehee
            char* blockBuffer = new char[BLOCK_SIZE + 1];
            inputFile.read(blockBuffer, BLOCK_SIZE);
            blockBuffer[BLOCK_SIZE] = '\0';
            string stringBuffer = blockBuffer;
            delete[] blockBuffer;
            stringstream strStream;
            strStream.str(stringBuffer);
            stringBuffer.clear();

            inputFile.close();

            string inputBuffer;

            //establish the first slot position
            if(slotCounter == 0){

                if (getline(strStream, inputBuffer, '[')){
                    //we found a slot
                    if(strStream.tellg() != -1){

                        //take account of the position
                        slotCounter = strStream.tellg();
                        int slotPosition = strStream.tellg();
                        slotPosition--;

                        //operate on the record

                        //grab the record offset and seek to the record
                        getline(strStream, inputBuffer, '*');
                        int recordPosition = stoi(inputBuffer);
                        strStream.seekg(stoi(inputBuffer));
                        //grab the id
                        getline(strStream, inputBuffer, '$');
                        string id = inputBuffer;
                        //grab the name
                        getline(strStream, inputBuffer, '$');
                        string name = inputBuffer;
                        //grab the bio
                        getline(strStream, inputBuffer, '$');
                        string bio = inputBuffer;
                        //grab the managerid
                        getline(strStream, inputBuffer, '$');
                        string manid = inputBuffer;

                        //print
                        if(stoi(id) == searchId){
                            //print
                            vector<string> myVector{id, name, bio, manid};
                            Record myRecord(myVector);

                            cout << endl;
                            myRecord.print();
                            cout << endl;
                        }

                    }
                    //there are no slots
                    else{
                        //reset the stringstream since the error bit got set
                        strStream.clear();
                        ifstream inputFile;
                        inputFile.open(fName, ios::binary);
                        //find the correct place to reset at
                        if(overFlowCounter == 0){
                            inputFile.seekg(BLOCK_SIZE*j);
                        }
                        else{
                            inputFile.seekg(overFlowCounter);
                        }
                        //convert the c string to a silly stringstream teehee
                        char* blockBuffer = new char[BLOCK_SIZE + 1];
                        inputFile.read(blockBuffer, BLOCK_SIZE);
                        blockBuffer[BLOCK_SIZE] = '\0';
                        string stringBuffer = blockBuffer;
                        delete[] blockBuffer;
                        stringstream strStream;
                        strStream.str(stringBuffer);
                        stringBuffer.clear();

                        inputFile.close();
                        //grab the overflow pointer
                        strStream.seekg(BLOCK_SIZE-11);
                        getline(strStream, inputBuffer, '}');
                        int ptrOverflowBucket = stoi(inputBuffer);

                        //check for overflow page
                        if(ptrOverflowBucket == 0){
                            strStream.clear();
                            break;
                        }
                        else{
                            overFlowCounter = ptrOverflowBucket;
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
                    strStream.seekg(BLOCK_SIZE-11);
                    getline(strStream, inputBuffer, '}');
                    int ptrOverflowBucket = stoi(inputBuffer);

                    if(ptrOverflowBucket == 0){
                        strStream.clear();
                        break;
                    }
                    else{
                        overFlowCounter = ptrOverflowBucket;
                        slotCounter = 0;
                    }
                }
                else{
                    //check if it's spaces
                    strStream.seekg(slotCounter);

                    //if not spaces, grab the record and operate on it
                    if((strStream.str())[slotCounter] != ' '){
                        //hash it

                        int slotPosition = strStream.tellg();
                        slotPosition--;
                        
                        //operate on the record
                        //grab the record offset and seek to the record
                        getline(strStream, inputBuffer, '*');
                        strStream.seekg(stoi(inputBuffer));
                        int recordPosition = stoi(inputBuffer);
                        //grab the id
                        getline(strStream, inputBuffer, '$');
                        string id = inputBuffer;
                        //grab the name
                        getline(strStream, inputBuffer, '$');
                        string name = inputBuffer;
                        //grab the bio
                        getline(strStream, inputBuffer, '$');
                        string bio = inputBuffer;
                        //grab the managerid
                        getline(strStream, inputBuffer, '$');
                        string manid = inputBuffer;

                        if(stoi(id) == searchId){
                            //print
                            vector<string> myVector{id, name, bio, manid};
                            Record myRecord(myVector);
                            cout << endl;
                            myRecord.print();
                            cout << endl;
                        }

                    }

                }

            }

        }
    }
};