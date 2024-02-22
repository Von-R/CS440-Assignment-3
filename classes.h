#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <string.h>
#include <sstream>
#include <bitset>
#include <cmath>
#include <cstring> 
#include <memory>

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

    // Function to insert a new record and slot directory entry into the block
void insertIntoBlock(std::ostringstream& outputStream, const std::string& recordString, const std::string& slotDirectoryEntry, int lastOffset, int lastLength, int slotDirStart) {
    std::string originalBlockData = outputStream.str(); // Get the current contents of the block

    cout << "insertIntoBlock\n";

    // Calculate where to insert the new record (immediately after the last record)
    int insertPositionForRecord = lastOffset + lastLength;
    // Calculate the new start position for the slot directory (taking into account the new record)
    int newSlotDirStart = slotDirStart + recordString.length() + slotDirectoryEntry.length();

    // Reconstruct the block with the new record and updated slot directory
    std::string updatedBlockData;
    updatedBlockData.reserve(BLOCK_SIZE); // Reserve enough space to avoid reallocations

    // Part before the new record
    updatedBlockData.append(originalBlockData.substr(0, insertPositionForRecord));
    // Insert the new record
    updatedBlockData.append(recordString);
    // Part between the new record and the old slot directory
    updatedBlockData.append(originalBlockData.substr(insertPositionForRecord, slotDirStart - insertPositionForRecord));
    // Insert the new slot directory entry
    updatedBlockData.append(slotDirectoryEntry);
    // Remaining part of the old slot directory
    updatedBlockData.append(originalBlockData.substr(slotDirStart));

    // Clear the stringstream and insert the updated block data
    outputStream.str("");
    outputStream << updatedBlockData;
}


    
bool canInsertRecord(int lastOffset, int lastLength, int slotDirStart, int newRecordSize) {
    return (lastOffset + lastLength + newRecordSize) <= (slotDirStart - 12); // Adjusted condition to fit refactored variables
}

std::string constructRecordString(const Record& record) {
    // Builds the record string
    return std::to_string(record.id) + '$' + record.name + '$' + record.bio + '$' + std::to_string(record.manager_id) + "$";
}

std::string constructSlotDirectoryEntry(int lastOffset, int lastLength, int recordLength) {
    // Calculate the starting position of the new record
    int newOffset = lastOffset + lastLength;

    // Format the slot directory entry, which typically includes the offset and length of the record
    // The specific format can vary depending on the system's requirements
    // Here, we'll simply separate them by a comma for illustration
    std::string slotEntry = std::to_string(newOffset) + "," + std::to_string(recordLength);

    return slotEntry;
}


void handleOverflow(std::istringstream& blockStream, int& currentReadPosition, int& nextFreeOverflow, const std::string& fName) {
    std::string inputBuffer;
    // Move the reading position to where the overflow pointer should be located
    blockStream.seekg(BLOCK_SIZE - 11);
    std::getline(blockStream, inputBuffer, '}');

    int overflowPointer = std::stoi(inputBuffer);

    std::ofstream outFile(fName, std::ios::in | std::ios::ate | std::ios::binary);
    if (overflowPointer == 0) { // No active overflow pointer
        // Calculate the new overflow pointer
        std::string newPointer = std::to_string(nextFreeOverflow);
        nextFreeOverflow += BLOCK_SIZE; // Update to the next free overflow position

        // Create and update the current block with the new overflow pointer
        // Assuming BLOCK_SIZE - (newPointer.length() + 1) is the correct position for the overflow pointer
        std::unique_ptr<char[]> currentBlockBuffer(new char[BLOCK_SIZE + 1]);
        blockStream.str().copy(currentBlockBuffer.get(), BLOCK_SIZE); // Copy current block data
        currentBlockBuffer[BLOCK_SIZE] = '\0';
        char* tempBuffer = currentBlockBuffer.get(); // Temporary char* pointing to the buffer
        // Correctly call stringWrite by passing a char** (address of tempBuffer)
        stringWrite(newPointer, &tempBuffer, BLOCK_SIZE - 11 - newPointer.length());

        outFile.seekp(currentReadPosition); // Go back to the start of the current block
        outFile.write(currentBlockBuffer.get(), BLOCK_SIZE); // Write the updated block with new overflow pointer

        // Prepare a new overflow block
        std::unique_ptr<char[]> newPageBuffer(new char[BLOCK_SIZE + 1]);
        std::fill_n(newPageBuffer.get(), BLOCK_SIZE, ' '); // Initialize the buffer with spaces
        newPageBuffer[BLOCK_SIZE] = '\0';

        std::string filePointer = "{0000000000}"; // Default file pointer for the new overflow block
        // Assuming we put the file pointer at the end of the new overflow block
        tempBuffer = newPageBuffer.get(); // Temporary char* pointing to the buffer
        // Correctly call stringWrite by passing a char** (address of tempBuffer)
        stringWrite(newPointer, &tempBuffer, BLOCK_SIZE - 11 - newPointer.length());
        //stringWrite(filePointer, newPageBuffer.get(), BLOCK_SIZE - filePointer.length() - 1);

        outFile.seekp(std::stoi(newPointer)); // Move to the new overflow block's position
        outFile.write(newPageBuffer.get(), BLOCK_SIZE); // Write the new overflow block
        
        currentReadPosition = std::stoi(newPointer); // Update the read location to the new overflow block
    } else {
        // If there's already an overflow pointer, move to that block for the next operation
        currentReadPosition = overflowPointer;
    }
    
    outFile.close();
}



    // Function to insert a record into an appropriate block within a file
void insertRecordIntoBlock(const Record& record) {
    cout << "insertRecordIntoBlock\n";
    auto currentReadPosition = j * BLOCK_SIZE; // Assuming 'j' is correctly defined and used elsewhere

    // Calculate total length of the new record dynamically
    auto recordSize = sizeof(long long) * 2 + record.bio.size() + record.name.size() + sizeof(int) + 12;

    for (;;) {
        cout << "for\n";
        std::ifstream fileStream(fName, std::ios::binary);
        if (!fileStream) {
            cout << "Failed to open file\n";
            break; // Exit if file cannot be opened
        }

        // Allocate memory for reading block data
        auto blockBuffer = std::make_unique<char[]>(BLOCK_SIZE + 1);
        fileStream.seekg(currentReadPosition).read(blockBuffer.get(), BLOCK_SIZE);
        blockBuffer[BLOCK_SIZE] = '\0'; // Ensure null-termination

        std::string blockData(blockBuffer.get());
        std::istringstream blockStream(blockData);

        int slotDirectoryStart = BLOCK_SIZE - 12, offsetLastRecord = 0, lengthLastRecord = 0;
        // Assuming canInsertRecord and other necessary logic is implemented correctly

        if (canInsertRecord(offsetLastRecord, lengthLastRecord, slotDirectoryStart, recordSize)) {
            std::ostringstream blockOutputStream;
            blockOutputStream.write(blockBuffer.get(), BLOCK_SIZE);

            // Construct new record string and slot directory entry
            std::string newRecordString = constructRecordString(record);
            std::string newSlotDirectoryEntry = constructSlotDirectoryEntry(offsetLastRecord, lengthLastRecord, newRecordString.length());

            // Insert new record and slot directory entry into block
            insertIntoBlock(blockOutputStream, newRecordString, newSlotDirectoryEntry, offsetLastRecord, lengthLastRecord, slotDirectoryStart);

            // Write modified block back to file
            std::ofstream outFile(fName, std::ios::in | std::ios::ate | std::ios::binary);
            outFile.seekp(currentReadPosition);
            outFile.write(blockOutputStream.str().c_str(), BLOCK_SIZE);

            cout << "Record inserted\n";
            break; // Successfully inserted the record, exit loop
        } else {
            handleOverflow(blockStream, currentReadPosition, nextFreeOverflow, fName);
            // After handling overflow, if you're not updating currentReadPosition to a new value that will eventually fail canInsertRecord, you will loop indefinitely.
            // Ensure handleOverflow properly updates conditions or states to prevent endless looping.
            cout << "Handled overflow\n";
            break; // Consider breaking after handling overflow or ensure the next iteration can exit.
        }
    }


    }

    // Insert new record into index
    void insertRecord(Record record) {
        cout << "insertRecord\n";
        
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
        insertRecordIntoBlock(record);

        
        
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

            //int allmyfellascounter = 1;

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
                                    stringWrite(blankRecord, &pageDataBuffer, recordPosition);
                                    //put the new slot in
                                    stringWrite(blankSlot, &pageDataBuffer, slotPosition);

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
                                    insertRecordIntoBlock(moveRecord);
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
                                    stringWrite(blankRecord, &pageDataBuffer, recordPosition);
                                    //put the new slot in
                                    stringWrite(blankSlot, &pageDataBuffer, slotPosition);

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
                                    insertRecordIntoBlock(moveRecord);
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
        stringWrite(filePointer, &newPage, 4084);

        //declare buffer and fill with spaces
        for(int i = 0; i < 256; i++){
            outFile.write(newPage, 4096);
        }

        delete[] newPage;
        outFile.close();
      
    }

    // Read csv file and add records to the index
    void createFromFile(string csvFName) {
        cout << "createFromFile\n";

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
                        int overflowPointer;
                        try {
                            overflowPointer = std::stoi(inputBuffer);
                            // Use overflowPointer as needed
                        } catch (const std::invalid_argument& e) {
                            // Handle error, e.g., log it, set overflowPointer to a default value, etc.
                            std::cerr << "Invalid overflow pointer: " << inputBuffer << std::endl;
                            // Consider setting overflowPointer to a default or error value
                        }


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