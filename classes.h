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

struct FileHeader {
    
    int totalBuckets; // Total number of buckets, including overflow
    int nextBucketToSplit // Next bucket in line for splitting
    int currentLevel // Current level of the hash table
    int totalRecords // Total number of records in the index
    int firstFreeOverflowBucketOffset // Offset for the first free overflow bucket (for quick allocation)
    ... // Other global parameters like hash table parameters, performance metrics, etc.
};

// Bucket Header Structure
struct BucketHeader {
    int bucketID; // Unique identifier for the bucket
    int recordsInBucket; // Number of records currently stored in the bucket
    int overflowBucketOffset; // Offset to the overflow bucket, if any
    int bucketFillRatio; // Fill ratio of the bucket to manage splitting
    // Other local parameters like modification timestamps, local performance metrics, etc.
};


class LinearHashIndex {

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
    // Successive calls will read successive records, as long as read pointer isn't moved 
    // by other methods
    Record readEmployeeRecord(ifstream &file){
        string line;
        getline(file, line);
        stringstream ss(line);
        vector<string> fields;
        string field;
        while (getline(ss, field, ',')) {
            fields.push_back(field);
        }
        return Record(fields);
    }

    // Insert new record into index
    void insertRecord(Record record) {

        // No records written to index yet
        if (numRecords == 0) {
            // Initialize index with first blocks (start with 4)

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

        // Take necessary steps if capacity is reached:
		// increase n; increase i (if necessary); place records in the new bucket that may have been originally misplaced due to a bit flip


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
        // make sure to account for the created buckets by incrementing nextFreeBlock appropriately
      
    }

    // Read csv file and add records to the index
    void createFromFile(string csvFName) {
        ifstream file(csvFName);

        if (!file.is_open()) {
            throw invalid_argument("Could not open file");
        }


        
    }

    // Given an ID, find the relevant record and print it
    Record findRecordById(int id) {
        
    }
};
