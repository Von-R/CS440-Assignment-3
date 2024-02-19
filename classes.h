#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <bitset>
using namespace std;

const int BLOCK_SIZE = 4096;

class Record {
public:
    bitset<8> hashID;
    int id, manager_id;
    std::string bio, name;

    Record(vector<std::string> fields) {
        id = stoi(fields[1]);
        name = fields[2];
        bio = fields[3];
        manager_id = stoi(fields[4]);
        hashID = bitset<8>( id % 216);
    }

    Record () {};

    void print() {
        cout << "\tHASH_ID: " << hashID.to_string() << "\n"; // "to_string" converts bitset to "human-readable" string
        cout << "\tID: " << id << "\n";
        cout << "\tNAME: " << name << "\n";
        cout << "\tBIO: " << bio << "\n";
        cout << "\tMANAGER_ID: " << manager_id << "\n";
    }

    // $ marks beginning of record
    // # marks end of field
    // % marks end of record
    string toString() const {
        return "$" + hashID.to_string() + "#" + to_string(id) + "#" + name + "#" + bio + "#" + to_string(manager_id) + "%";
    }

    bitset<8> getHashID() const {
        return bitset<8>(id % 216);
    }
};

struct FileHeader {
    vector<BucketHeader> bucketHeaders; // Vector of bucket headers
    int totalBuckets;                   // Total number of buckets, including overflow
    int nextBucketToSplit;              // Next bucket in line for splitting
    float currentLevel;                   // Current level of the hash table
    int totalRecords;                   // Total number of records in the index
    int firstFreeOverflowBucketOffset;  // Offset for the first free overflow bucket (for quick allocation)
    int fileHeaderSize;                 // Size of the file header: calculate at compile time, if static
    // Other global parameters like hash table parameters, performance metrics, etc.

    // Initialize the file header
    FileHeader() {
        totalBuckets = 0;
        nextBucketToSplit = 0;
        currentLevel = 0;
        totalRecords = 0;
        firstFreeOverflowBucketOffset = -1;
    }

    // Methods

    // Getters
    float getCurrentLevel() {
        calculateIndexFillLevel();
        return currentLevel;
    }

    // This method requires that the bucket headers are already populated and up to date
    float calculateIndexFillLevel() {
        // Calculate the fill ratio of the index
        currentLevel = 0;
        for (BucketHeader bucket : bucketHeaders) {
            currentLevel += bucket.getBucketFillRatio();
            currentLevel /= totalBuckets;
        }
        return currentLevel;
    }


    // Write the file header to the index file
    void serializeHeader() {
        // What members will I write, exactly?
        // Anything that needs to persist between creating the index file and reading it back in for searching

        // for serializing the bucketHeader vector, first write the size of the vector, then write each bucketHeader
        // this will allow you to read the size of the vector first, then read that many bucketHeaders, knowing when to stop
        // when deserializing
    }

    // Read the file header from the index file
    void deserializeHeader() {

        // for deserializing the bucketHeader vector, first read the size of the vector, then read each bucketHeader
        // this will allow you to read the size of the vector first, then read that many bucketHeaders, knowing when to stop


    }

    // Setters
    void setTotalBuckets(int numBuckets) {
        totalBuckets = numBuckets;
    }

    void incrementTotalBuckets() {
        totalBuckets++;
    }

    void incrementTotalRecords() {
        totalRecords++;
    }

    void setNextBucketToSplit(int nextBucket) {
        nextBucketToSplit = nextBucket;
    }


    

};

// Bucket Header Structure
struct BucketHeader {
    int bucketID; // Unique identifier for the bucket. 1 through 216
    int digits; // Number of bits used to address the bucket. 1 through 8
    int recordsInBucket; // Number of records currently stored in the bucket
    int overflowBucketOffset; // Offset to the overflow bucket, if any
    float bucketFillRatio; // Fill ratio of the bucket to manage splitting
    int bucketSpaceRemaining;
    // Other local parameters like modification timestamps, local performance metrics, etc.

    BucketHeader(int id) {
        bucketID = id;
        recordsInBucket = 0;
        overflowBucketOffset = -1;
        bucketFillRatio = 0;
        bucketSpaceRemaining = BLOCK_SIZE - sizeof(BucketHeader) ;
    }

    // Getters
    int getBucketID() {
        return bucketID;
    }

    int getDigits() {
        return digits;
    }

    int getRecordsInBucket() {
        return recordsInBucket;
    }

    int getOverflowBucketOffset() {
        return overflowBucketOffset;
    }

    int getBucketSpaceRemaining() {
        return bucketSpaceRemaining;
    }

    float getBucketFillRatio() {
        calculateBucketFillRatio();
        return bucketFillRatio;
    }

    // Setters
    void setRecordsInBucket(int recordNum) {
        recordsInBucket = recordNum;
    }

    void setHashID(bitset<8> hashID) {
        // Set the hash ID of the bucket
        bucketID = hashID.to_ulong();
    }

    void incrementSignificantBits() {
        // Increment the number of significant bits
        digits++;
    }

    void calculateBucketFillRatio() {
        // Calculate the fill ratio of the bucket
        bucketFillRatio = (BLOCK_SIZE - sizeof(BucketHeader) - bucketSpaceRemaining) / BLOCK_SIZE;
    }




};


class LinearHashIndex {

    FileHeader * fileHeader;
    ofstream outputFile;


private:
    

    vector<int> blockDirectory; // Map the least-significant-bits of h(id) to a bucket location in EmployeeIndex (e.g., the jth bucket)
                                // can scan to correct bucket using j*BLOCK_SIZE as offset (using seek function)
								// can initialize to a size of 256 (assume that we will never have more than 256 regular (i.e., non-overflow) buckets)
    int n;  // The number of indexes in blockDirectory currently being used
    int i;	// The number of least-significant-bits of h(id) to check. Will need to increase i once n > 2^i
    int numRecords;    // Records currently in index. Used to test whether to increase n
    int nextFreeBlock; // Next place to write a bucket. Should increment it by BLOCK_SIZE whenever a bucket is written to EmployeeIndex
    string fName;      // Name of index file

    // Read a record from the file
    // Successive calls will read successive records, as long as read pointer isn't moved by other methods
    // Option 1: Return a boolean indicating success/failure
    bool readEmployeeRecord(ifstream &file, Record &record) {
        string line;
        if (!getline(file, line)) {
            return false; // Indicate failure to read due to EOF or other error
        }
        stringstream ss(line);
        vector<string> fields;
        string field;
        while (getline(ss, field, ',')) {
            fields.push_back(field);
        }
        record = Record(fields);
        return true; // Indicate success
    }

    // Insert new record into index
    // Do not fill memory with records, simply write one at a time to the index file
    bool insertRecord(Record record) {

        // No records written to index yet
        if (numRecords == 0) {
            // Initialize index with first blocks (start with 4)
            initializeIndexFile(outputFile, fileHeader);
        }

        // Add record to the index in the correct block, creating a overflow block if necessary

        /*
        Check file header to get offset of correct bucket
        Check bucket header to determine if there's room, a split is needed or an overflow
            bucket already exists for this hash value

            1. If there's room, write the record to the bucket
            2. If a split is needed, split the bucket and write the record to the correct bucket
            3. If overflow bucket exists, seek to it, read bucket header and repeat process
        
        */
        numRecords++;
        // Take necessary steps if capacity is reached:
		// increase n; increase i (if necessary); place records in the new bucket that may have been originally misplaced due to a bit flip
        return true;

    }

public:
    LinearHashIndex(string indexFileName) {
        n = 4; // Start with 4 buckets in index
        i = 2; // Need 2 bits to address 4 buckets
        numRecords = 0;
        nextFreeBlock = 0;
        fName = indexFileName;
        blockDirectory = vector<int>(256, -1);

        // Create your EmployeeIndex file and write out the initial 4 buckets
        outputFile.open(indexFileName, ios::out | ios::binary);
        // make sure to account for the created buckets by incrementing nextFreeBlock appropriately
        for (int j = 0; j < n; j++) {
            outputFile.seekp(FileHeader::size() + j * BLOCK_SIZE, ios::beg);

            blockDirectory[j] = nextFreeBlock;
            nextFreeBlock += BLOCK_SIZE;
        }
      
    }

    void initializeIndexFile(string indexFileName, FileHeader * fileHeader) {
        // Create the index file and write out the initial 4 buckets
        ofstream indexFile;
        indexFile.open(indexFileName, ios::out | ios::binary);
        if (!indexFile.is_open()) {
            throw invalid_argument("Could not open file");
        }
        //
        fileHeader->serializeHeader();
        // make sure to account for the created buckets by incrementing nextFreeBlock appropriately
    }   

    // Read csv file and add records to the index
    void createFromFile(string csvFName) {
        ifstream inputFile(csvFName);

        if (!inputFile.is_open()) {
            throw invalid_argument("Could not open file");
        }

        

        // Declare/initialize variables
        ofstream indexFile;
        Record record;
        bitset<8> hashID;
        // Initialize PageList
        PageList * pageList = new PageList();

        // Initialize index file: Consult index file structure to determine how to initialize
        initializeIndexFile();
        // Read records from csv file and insert into index
            // Read line: parse record into string
            
            
        while(readEmployeeRecord(inputFile, record)){
            // record now is not populated with a single record, including the hash ID.
            // Load string into memory page, make sure to update index page
            // If page is full, write to disk and update index file header
            if (!insertRecord(record)) {
                // If record insertion fails, print error message
                cout << "Error inserting record" << endl;
            }
            // rinse/repeat until end of file
        }
        // Close csv file
        inputFile.close();

        // Result: 


        
    }

    // Given an ID, find the relevant record and print it
    Record findRecordById(int id) {
        
    }
};
