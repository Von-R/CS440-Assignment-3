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

const int MAX_RECORD_SIZE = 730;

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
    int n;                      // The number of indexes in blockDirectory currently being used
    int i;	                    // The number of least-significant-bits of h(id) to check. Will need to increase i once n > 2^i
    int j;                      // Used to search the blockdir and find the right page for a given record id
    int numRecords;             // Records currently in index. Used to test whether to increase n
    int nextFreeBlock;          // Next place to write a bucket. Should increment it by BLOCK_SIZE whenever a bucket is written to EmployeeIndex
    int nextOverflowBucket;     // Next place to write an overflow bucket. Should increment it by BLOCK_SIZE whenever an overflow bucket is written to EmployeeIndex
    string fName;               // Name of index file
    int hashID;                 // The hash value of the record id
    
    // Helper function to write a string to a c string
    void stringWrite(const string& toCopy, char** copyHere, int location) {
        memcpy(*copyHere + location, toCopy.c_str(), toCopy.length());
    }

    // Helper function to convert a number to binary and grab the i least significant bits
    int leastSigBits(int bitMask, int xorResult, int divResult) {
        for(int k = 0; k < i; k++){
            //build an int that looks like binary
            xorResult = divResult%2;
            divResult = divResult/2;
            bitMask += pow(10,k) * xorResult;
            }
            return bitMask;
        }

    // Inserts a record into a block
    void insertRecordIntoIndexIntoBlock(Record record){
        int readBlock = j*BLOCK_SIZE;
        string offsetBuffer;

        // Dynamically calculate the size of the record, variable length fields
        int recordSize = 32 + record.bio.length() + record.name.length();

        // Loop until the record is inserted
        while(true)
        {
            // Open the file and seek to the desired block
            ifstream inputFile;
            inputFile.open(fName, ios::binary);
            inputFile.seekg(readBlock);

            // Read the block into a buffer
            string stringBuffer;
            stringBuffer.resize(BLOCK_SIZE);
            inputFile.read(&stringBuffer[0], BLOCK_SIZE);
            inputFile.close();

            //set the buffer to a stringstream and delete the normal string
            stringstream strStream;
            strStream.str(stringBuffer);
            stringBuffer.clear();

            // The offset of the record directory and the size of the record directory
            int recordDirectoryOffset = BLOCK_SIZE - 12;
            int recDirSize = 0;
            int recentOffset = 0;
            int recentLength = 0;

            // Get the offset and length of the most recent record
            if (getline(strStream, offsetBuffer, '[')){
                if(strStream.tellg() != -1){
                    recordDirectoryOffset = strStream.tellg();
                    recordDirectoryOffset--;
                    recDirSize = BLOCK_SIZE - recordDirectoryOffset;

                    // Get the offset of the most recent record
                    getline(strStream, offsetBuffer, '*');
                    recentOffset = stoi(offsetBuffer);

                    // Get the length of the most recent record
                    getline(strStream, offsetBuffer, ',');
                    getline(strStream, offsetBuffer, '*');
                    recentLength = stoi(offsetBuffer);
                    
                }
                // If getline failure, reset the stringstream and move on
                else{
                    // Reset stringstream
                    ifstream inputFile(fName, ios::binary);
                    inputFile.seekg(readBlock);

                    string stringBuffer;
                    stringBuffer.resize(BLOCK_SIZE);
                    inputFile.read(&stringBuffer[0], BLOCK_SIZE);
                    inputFile.close();

                    stringstream strStream(stringBuffer);
                }
            }

            // If there is sufficient space after all data and overhead in the block, insert the record
            if((recentOffset + recentLength + recDirSize + recordSize) <= BLOCK_SIZE){
                // Turn the stringstream back into a c string
                stringBuffer = strStream.str();
                strStream.clear();
                char* blockBuffer = new char[BLOCK_SIZE + 1];
                blockBuffer[BLOCK_SIZE] = '\0';
                strcpy(blockBuffer, (stringBuffer).c_str());
        
                // Construct the new record string from the record object fields
                string newRecord = to_string(record.id) + '%' + record.name + '%' + record.bio + '%' + to_string(record.manager_id) + "%";
                string newSlot = "[*****,****]";

                // Construct the new slot string
                newSlot.replace(1, to_string(recentOffset + recentLength).length(), to_string(recentOffset + recentLength));
                newSlot.replace(7, to_string(newRecord.length()).length(), to_string(newRecord.length()));

                // Write the new record and slot to the block buffer
                stringWrite(newRecord, &blockBuffer, (recentOffset + recentLength));
                stringWrite(newSlot, &blockBuffer, (recordDirectoryOffset - 12));

                // Write the block buffer to the file
                ofstream outputFile;
                outputFile.open(fName, ios::in | ios::ate | ios::binary);
                outputFile.seekp(readBlock);
                outputFile.write(blockBuffer, 4096);
                outputFile.close();

                // Tidy up
                delete[] blockBuffer;

                break;

            }

            // If there is not enough space in the block, move to overflow
            else{

                // Get the overflow pointer
                strStream.seekg(BLOCK_SIZE-11);
                getline(strStream, offsetBuffer, '}');

                // Get the overflow pointer as int
                int ptrOverflowBucket = stoi(offsetBuffer);

                // If there is no overflow page, create one and store the record there
                if(ptrOverflowBucket == 0)
                {
                    // Turn the stringstream back into a c string
                    stringBuffer = strStream.str();
                    strStream.clear();
                    
                    // Create a new block buffer
                    char* blockBuffer = new char[BLOCK_SIZE + 1];
                    blockBuffer[BLOCK_SIZE] = '\0';
                    strcpy(blockBuffer, (stringBuffer).c_str());
                    stringBuffer.clear();

                    // Write the overflow pointer to the end of the block
                    string tmpPtr = to_string(nextOverflowBucket);
                    nextOverflowBucket += BLOCK_SIZE;
                    int overflowBlockDist = (BLOCK_SIZE-(tmpPtr.length()+1));
                    stringWrite(tmpPtr, &blockBuffer, overflowBlockDist);

                    // Write the block buffer to the file
                    ofstream outputFile;
                    outputFile.open(fName, ios::in | ios::ate | ios::binary);
                    outputFile.seekp(readBlock);
                    outputFile.write(blockBuffer, BLOCK_SIZE);

                    // Write file pointer to the new overflow block
                    string filePointer = "{0000000000}";
                    memset(blockBuffer, ' ', BLOCK_SIZE);
                    stringWrite(filePointer, &blockBuffer, 4084);

                    // Write the block buffer to the file
                    outputFile.seekp(stoi(tmpPtr));
                    outputFile.write(blockBuffer, BLOCK_SIZE);

                    outputFile.close();
                    // Update readBlock to the new overflow block
                    readBlock = stoi(tmpPtr);

                    // Tidy up
                    delete[] blockBuffer;
                }
                
                else {
                    // Advance to the overflow block
                    readBlock = ptrOverflowBucket; 
                }
            } 
        }
    }

    // Insert new record into index
    void insertRecordIntoIndex(Record record) {

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

        // Calculate the hash function value for record
        int hashID = record.id%216;
        int divResult = hashID;
        int xorResult = 0;
        int bitMask = 0;
    
        // Get the i least significant bits of the hash value
        bitMask = leastSigBits(bitMask, xorResult, divResult);

        // Bit flip
        if (bitMask > blockDirectory.back()) {
            bitMask -= pow(10, i-1);
        }

        // Find the correct block for the record
        for(int i = 0; i < n; i++){
            if(blockDirectory.at(i) == bitMask){
                j = i;
            }
        }

        string stringBuffer;

        // Call the function to insert the record into the block
        insertRecordIntoIndexIntoBlock(record);

        // If the average number of records per page exceeds 70% capacity, increase n
        if((n * 4096 * 0.7) < (numRecords * MAX_RECORD_SIZE)){
            nextFreeBlock += BLOCK_SIZE;
            n++;
    
            // If n exceeds 2^i, increase i
            if(pow(2,i) < n){
                i++;
            }

            bitMask = 0;
            divResult = n-1;
            int k = 0;

            // bitmask calc
            while(divResult != 0){
                xorResult = divResult%2;
                divResult = divResult/2;
                bitMask += pow(10,k) * xorResult;
                k++;
            }
            
            // Add the new block bitmask to the block directory
            blockDirectory.push_back(bitMask);

            // Loop through the blocks and reassign records to new blocks
            for(int l = 0; l < n-1; l++)
            {
                int overflowBucketIndex = 0;
                int slotPosn = 0;

                // Loop through the pages of the block
                while(true){
                    
                    ifstream inputFile;
                    inputFile.open(fName, ios::binary);

                    // If there is no overflow, seek to the block
                    if(overflowBucketIndex == 0){
                        inputFile.seekg(BLOCK_SIZE*l);
                    }
                    // If there is overflow, seek to the overflow block
                    else{
                        inputFile.seekg(overflowBucketIndex);
                    }

                    // Convert the c string to a stringstream
                    char* blockBuffer = new char[BLOCK_SIZE + 1];
                    inputFile.read(blockBuffer, BLOCK_SIZE);
                    blockBuffer[BLOCK_SIZE] = '\0';
                    string stringBuffer = blockBuffer;
                    delete[] blockBuffer;
                    stringstream strStream;
                    strStream.str(stringBuffer);
                    stringBuffer.clear();

                    inputFile.close();

                    // Determine position of the first slot
                    if(slotPosn == 0){
                        if (getline(strStream, stringBuffer, '[')){
                            
                            if(strStream.tellg() != -1){

                                // Get the position of the slot
                                slotPosn = strStream.tellg();
                                int slotPosition = strStream.tellg();
                                slotPosition--;

                                // Get line of the record
                                getline(strStream, stringBuffer, '*');
                                int posnOfRecord = stoi(stringBuffer);
                                strStream.seekg(stoi(stringBuffer));
                                
                                // Get fields
                                getline(strStream, stringBuffer, '%');
                                string id = stringBuffer;
                            
                                getline(strStream, stringBuffer, '%');
                                string name = stringBuffer;
                            
                                getline(strStream, stringBuffer, '%');
                                string bio = stringBuffer;
                            
                                getline(strStream, stringBuffer, '%');
                                string managerID = stringBuffer;

                                // Create a record object
                                vector<string> fieldArray{id, name, bio, managerID};
                                Record tmpRecord(fieldArray);

                                int hashID = tmpRecord.id%216;
                                int divResult = hashID;
                                int bitMask = 0;
                                int xorResult = 0;
                               

                                // Bitmask calc
                                bitMask = leastSigBits(bitMask, xorResult, divResult);

                                // Bit flip inspection
                                if(bitMask > blockDirectory.at(n-1)){
                                    // Flip bit
                                    int xorRes = pow(10, i-1);
                                    bitMask = bitMask - xorRes;
                                }

                                // Determine if bitflipped record must be moved
                                if(bitMask != blockDirectory.at(j)){
                                    // Determine correct block to move bit flipped record to
                                    for(int k = 0; k < n; k++){
                                        if(blockDirectory.at(k) == bitMask){
                                            j = k;
                                        }
                                    }

                                    // Convert the stringstream to a c string
                                    stringBuffer = strStream.str();
                                    strStream.clear();
                                    char* blockBuffer = new char[BLOCK_SIZE + 1];
                                    blockBuffer[BLOCK_SIZE] = '\0';
                                    strcpy(blockBuffer, (stringBuffer).c_str());
                                    stringBuffer.clear();

                                    // Construct the new template record
                                    int templateRecordLen = 8 + 8 + 4 + tmpRecord.name.length() + tmpRecord.bio.length();
                                    string templateRecord;
                                    templateRecord.append(templateRecordLen, ' ');
                                    string slotEmpty = "            ";

                                    stringWrite(templateRecord, &blockBuffer, posnOfRecord);
                                    stringWrite(slotEmpty, &blockBuffer, slotPosition);

                                    // Write the block buffer to the file
                                    ofstream outputFile;
                                    outputFile.open(fName, ios::in | ios::ate | ios::binary);
                                    if(overflowBucketIndex == 0){
                                        outputFile.seekp(BLOCK_SIZE*l);
                                    }
                                    else {
                                        outputFile.seekp(overflowBucketIndex);
                                    }
                                    outputFile.write(blockBuffer, 4096);
                                    outputFile.close();

                                    // Tidy up
                                    delete[] blockBuffer;

                                    // Transfer record to new block
                                    insertRecordIntoIndexIntoBlock(tmpRecord);
                                    numRecords++;
                                }
                            }

                            // Otherwise if there are no slots available
                            else{
                                
                                // Convert the stringstream to a c string for error bit
                                strStream.clear();
                                ifstream inputFile;
                                inputFile.open(fName, ios::binary);

                                // Determine correct block to move bit flipped record to
                                if(overflowBucketIndex == 0){
                                    inputFile.seekg(BLOCK_SIZE*l);
                                }
                                else{
                                    inputFile.seekg(overflowBucketIndex);
                                }
                                
                                // Convert the c string to a stringstream
                                char* blockBuffer = new char[BLOCK_SIZE + 1];
                                inputFile.read(blockBuffer, BLOCK_SIZE);
                                blockBuffer[BLOCK_SIZE] = '\0';
                                string stringBuffer = blockBuffer;
                                delete[] blockBuffer;
                                stringstream strStream;
                                strStream.str(stringBuffer);
                                stringBuffer.clear();
                                inputFile.close();

                                // Retrieve the pointer to the overflow block
                                strStream.seekg(BLOCK_SIZE-11);
                                getline(strStream, stringBuffer, '}');
                                int ptrOverflowBucket = stoi(stringBuffer);

                                // Determine if there is an overflow page
                                if(ptrOverflowBucket == 0){
                                    strStream.clear();
                                    break;
                                }
                                else{
                                    overflowBucketIndex = ptrOverflowBucket;
                                    slotPosn = 0;
                                }
                            }
                        }
                    }
                    
                    // Slot found
                    else{

                        // Increment slot position
                        slotPosn += 12;

                        // End of page reached
                        if(slotPosn == 4085){

                            // Retrieve the pointer to the overflow block
                            strStream.seekg(BLOCK_SIZE-11);
                            getline(strStream, stringBuffer, '}');
                            int ptrOverflowBucket = stoi(stringBuffer);

                            if(ptrOverflowBucket == 0){
                                strStream.clear();
                                break;
                            }
                            else{
                                overflowBucketIndex = ptrOverflowBucket;
                                slotPosn = 0;
                            }
                        }

                        else {
                            strStream.seekg(slotPosn);

                             // If not empty space, get record
                            if((strStream.str())[slotPosn] != ' '){

                                int slotPosition = strStream.tellg();
                                slotPosition--;
                                
                                // From record offset, seek to record position in stream
                                getline(strStream, stringBuffer, '*');
                                strStream.seekg(stoi(stringBuffer));
                                int posnOfRecord = stoi(stringBuffer);
                               
                                getline(strStream, stringBuffer, '%');
                                string id = stringBuffer;
                        
                                getline(strStream, stringBuffer, '%');
                                string name = stringBuffer;
                              
                                getline(strStream, stringBuffer, '%');
                                string bio = stringBuffer;
                                
                                getline(strStream, stringBuffer, '%');
                                string managerID = stringBuffer;

                                // Create record
                                vector<string> fieldArray{id, name, bio, managerID};
                                Record tmpRecord(fieldArray);

                                int hashID = tmpRecord.id%216;
                                int bitMask = 0;
                                int xorResult = 0;
                                int divResult = hashID;

                                // BitMask calc
                                bitMask = leastSigBits(bitMask, xorResult, divResult);

                                // Bit flip inspection
                                if(bitMask > blockDirectory.at(n-1)){
                                    // Flip bit
                                    int xorRes = pow(10, i-1);
                                    bitMask = bitMask - xorRes;
                                }
                                // Determine if bitflipped record must be moved
                                if(bitMask != blockDirectory.at(j)){

                                    // Determine correct block to move bit flipped record to
                                    for(int k = 0; k < n; k++){
                                        if(blockDirectory.at(k) == bitMask){
                                            j = k;
                                        }
                                    }

                                    // Convert the stringstream to a c string
                                    stringBuffer = strStream.str();
                                    strStream.clear();
                                    char* blockBuffer = new char[BLOCK_SIZE + 1];
                                    blockBuffer[BLOCK_SIZE] = '\0';
                                    strcpy(blockBuffer, (stringBuffer).c_str());
                                    stringBuffer.clear();

                                    // Construct the new template record
                                    int templateRecordLen = 8 + 8 + 4 + tmpRecord.name.length() + tmpRecord.bio.length();
                                    string templateRecord;
                                    templateRecord.append(templateRecordLen, ' ');
                                    string slotEmpty = "            ";

                                    stringWrite(templateRecord, &blockBuffer, posnOfRecord);              
                                    stringWrite(slotEmpty, &blockBuffer, slotPosition);

                                    // Write the block buffer to the file
                                    ofstream outputFile;
                                    outputFile.open(fName, ios::in | ios::ate | ios::binary);
                                    if(overflowBucketIndex == 0){
                                        outputFile.seekp(BLOCK_SIZE*l);
                                    }
                                    else{
                                        outputFile.seekp(overflowBucketIndex);
                                    }
                                    outputFile.write(blockBuffer, 4096);
                                    outputFile.close();

                                    delete[] blockBuffer;

                                    // Transfer record to new block
                                    insertRecordIntoIndexIntoBlock(tmpRecord);
                                    numRecords++;
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
        ofstream outputFile;

        outputFile.open(indexFileName, ios::binary);

        // Initialize the index file with 256 pages of empty blocks
        char* newPage = new char[4096];
        string filePointer = "{0000000000}";
        memset(newPage, ' ', 4096);
        stringWrite(filePointer, &newPage, 4084);

        for(int i = 0; i < 256; i++){
            outputFile.write(newPage, 4096);
        }

        delete[] newPage;
        outputFile.close();
      
    }

    // Read csv file and add records to the index
    void createFromFile(string csvFName) {

        ifstream inputFile;
        inputFile.open(csvFName);
        string currentLine;
        string eid;
        string managerID;
        string name;
        string bio;

        // Loop and insert records into index
        while(true){
            
            if(getline(inputFile, currentLine, ',')){
                eid = currentLine;

                getline(inputFile, currentLine, ',');
                name = currentLine;

                getline(inputFile, currentLine, ',');
                bio = currentLine;

                getline(inputFile, currentLine);
                managerID = currentLine;
                
                vector<string> fieldArray{eid, name, bio, managerID};
                Record createRecord(fieldArray);
    
                // Pass the record to the insert function
                insertRecordIntoIndex(createRecord);
                
            }
            // No newline, break
            else{
                break;
            }
        }
        inputFile.close();
    }

    // Given an ID, find the relevant record and print it
    void findRecordById(int searchId) {

        int hashID = searchId%216;
        int bitMask = 0;
        int xorResult = 0;
        int divResult = hashID;

        // BitMask calc
        bitMask = leastSigBits( bitMask,  xorResult,  divResult); 

        // Bit flip inspection
        if(bitMask > blockDirectory.at(n-1)){
            // Flip bit
            int xorRes = pow(10, i-1);
            bitMask = bitMask - xorRes;
        }

        // Use bitmask to find the correct block
        for(int k = 0; k < n; k++){
            if(blockDirectory.at(k) == bitMask){
                j = k;
            }
        }

        int slotPosn = 0;
        int overflowBucketIndex = 0;

        while(true){
            ifstream inputFile;
            inputFile.open(fName, ios::binary);

            if(overflowBucketIndex == 0){
                inputFile.seekg(BLOCK_SIZE*j);
            }
            else{
                inputFile.seekg(overflowBucketIndex);
            }

             // Convert the c string to a stringstream
            char* blockBuffer = new char[BLOCK_SIZE + 1];
            inputFile.read(blockBuffer, BLOCK_SIZE);
            blockBuffer[BLOCK_SIZE] = '\0';
            string stringBuffer = blockBuffer;
            delete[] blockBuffer;
            stringstream strStream;
            strStream.str(stringBuffer);
            stringBuffer.clear();
            inputFile.close();

            // Determine position of the first slot
            if(slotPosn == 0){
                if (getline(strStream, stringBuffer, '[')){
                    if(strStream.tellg() != -1){

                        slotPosn = strStream.tellg();
                        int slotPosition = strStream.tellg();
                        slotPosition--;

                        // From record offset, seek to record position in stream
                        getline(strStream, stringBuffer, '*');
                        int posnOfRecord = stoi(stringBuffer);
                        strStream.seekg(stoi(stringBuffer));
                       
                        getline(strStream, stringBuffer, '%');
                        string id = stringBuffer;
                
                        getline(strStream, stringBuffer, '%');
                        string name = stringBuffer;
                      
                        getline(strStream, stringBuffer, '%');
                        string bio = stringBuffer;
                        
                        getline(strStream, stringBuffer, '%');
                        string managerID = stringBuffer;

                        if(stoi(id) == searchId){
                            vector<string> fieldArray{id, name, bio, managerID};
                            Record createRecord(fieldArray);
                            cout << endl;
                            createRecord.print();
                            cout << endl;
                        }

                    }
                    else {

                         // Convert the stringstream to a c string for error bit
                        strStream.clear();
                        ifstream inputFile;
                        inputFile.open(fName, ios::binary);

                         // Determine correct block to move bit flipped record to
                        if(overflowBucketIndex == 0){
                            inputFile.seekg(BLOCK_SIZE*j);
                        }
                        else{
                            inputFile.seekg(overflowBucketIndex);
                        }

                         // Convert the c string to a stringstream
                        char* blockBuffer = new char[BLOCK_SIZE + 1];
                        inputFile.read(blockBuffer, BLOCK_SIZE);
                        blockBuffer[BLOCK_SIZE] = '\0';
                        string stringBuffer = blockBuffer;
                        delete[] blockBuffer;
                        stringstream strStream;
                        strStream.str(stringBuffer);
                        stringBuffer.clear();
                        inputFile.close();

                        // Retrieve the pointer to the overflow block
                        strStream.seekg(BLOCK_SIZE-11);
                        getline(strStream, stringBuffer, '}');
                        int ptrOverflowBucket = stoi(stringBuffer);

                        // Determine if there is an overflow page
                        if(ptrOverflowBucket == 0){
                            strStream.clear();
                            break;
                        }
                        else{
                            overflowBucketIndex = ptrOverflowBucket;
                            slotPosn = 0;
                        }

                    }
                }
            }

            // Slot found
            else {

                // Increment slot position
                slotPosn += 12;

                 // End of page reached
                if(slotPosn == 4085){

                    // Retrieve the pointer to the overflow block
                    strStream.seekg(BLOCK_SIZE-11);
                    getline(strStream, stringBuffer, '}');
                    int ptrOverflowBucket = stoi(stringBuffer);

                    if(ptrOverflowBucket == 0){
                        strStream.clear();
                        break;
                    }
                    else{
                        overflowBucketIndex = ptrOverflowBucket;
                        slotPosn = 0;
                    }
                }
                else {
                    strStream.seekg(slotPosn);

                     // If not empty space, get record
                    if((strStream.str())[slotPosn] != ' '){
                        int slotPosition = strStream.tellg();
                        slotPosition--;
                      
                        // From record offset, seek to record position in stream
                        getline(strStream, stringBuffer, '*');
                        strStream.seekg(stoi(stringBuffer));
                        int posnOfRecord = stoi(stringBuffer);
                       
                        getline(strStream, stringBuffer, '%');
                        string id = stringBuffer;
                
                        getline(strStream, stringBuffer, '%');
                        string name = stringBuffer;
                      
                        getline(strStream, stringBuffer, '%');
                        string bio = stringBuffer;
                        
                        getline(strStream, stringBuffer, '%');
                        string managerID = stringBuffer;

                        if(stoi(id) == searchId){
                            vector<string> fieldArray{id, name, bio, managerID};
                            Record createRecord(fieldArray);
                            cout << endl;
                            createRecord.print();
                            cout << endl;
                        }
                    }
                }
            }
        }
    }
};