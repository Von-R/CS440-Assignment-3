# ifndef PAGE_H
# define PAGE_H

# include <string>
# include <vector>
# include <algorithm>
# include "classes.h"

class Page {
        static const int page_size = BLOCK_SIZE; // Define the size of each page (4KB)
        int pageNumber; // Identifier for the page
        Page *nextPage; // Pointer to the next page in the list
        // These are static members of the Page class because the offsetArray and dataVector sizes will be shared by all instances of the Page class
        int static offsetArraySize;
        int static dataVectorSize;
        static const char sentinelValue = '\0'; // Sentinel value to indicate empty space in the data vector
        vector<int> offsetArray; // Vector to store the offsets of the records in the page
        vector<char> data; // Vector to store the data in the page

        bool dataVectorEmpty() {
            for (int i = 0; i < data.size(); i++) {
                if (data[i] != sentinelValue) {
                    return false;
                }
            }
            return true;
        }

        // Returns the size of the data vector by counting non-sentinel values
        // Does not modify data
        int dataSize(const std::vector<char>& data, char sentinelValue) {
            //cout << "dataSize begin" << endl;
            return std::count_if(data.begin(), data.end(), [sentinelValue](char value) {
            return value != sentinelValue;
            });
            //cout << "dataSize end" << endl;
        }

        bool offsetArrayEmpty() {
            for (int i = 0; i < offsetArray.size(); i++) {
                if (offsetArray[i] != -1) {
                    return false;
                }
            }
            return true;
        }

    public:
        struct PageHeader {
            int recordsInPage; // Number of records in the page
            int spaceRemaining; // Updated to be initialized later in Page constructor

            PageHeader() : recordsInPage(0) {} // Constructor simplified
        } pageHeader;

        // Constructor for the Page class
        Page(int pageNum) : pageNumber(pageNum), nextPage(nullptr) {
            cout << "Page constructor begin" << endl;

            // Initialize the offsetArray to its maximum potential size first; sentinel value is -1
            offsetArray = get<0>(initializationResults); // Instance-specific offsetArray is initialized with 0's
            cout << "Value of offsetArray[0]: " << offsetArray[0] << endl;

            
            // size of offset array, 2nd element of tuple returned by initializeValues
            offsetArraySize = get<1>(initializationResults); // Update this based on actual logic
            cout << "Value of offsetArraySize: " << offsetArraySize << endl;

            // Size of data vector; calculated based on the size of the offset array and the size of the page header
            dataVectorSize = page_size - offsetArraySize - sizeof(PageHeader);
            cout << "Value of dataVectorSize: " << dataVectorSize << endl;
            // Initialize data vector to its maximum potential size first; sentinel value is -1
            data.resize(dataVectorSize, sentinelValue);
            cout << "Value of data[0]: " << data[0] << endl;
            cout << "Size of resized data vector: " << data.size() << endl;
            // Initialize space remaining in the page
            pageHeader.spaceRemaining = dataVectorSize;
            cout << "Value of spaceRemaining: " << pageHeader.spaceRemaining << endl;
            cout << "Ensure all members are initialized correctly\n";
            cout << "Page constructor end" << endl;
        }

        // Returns the size of the offset array by count non-sentinel values
        // Does not modify offsetArray
        int offsetSize() {
            int count = std::count_if(offsetArray.begin(), offsetArray.end(), [](int value) {
                return value != -1;
            });
            return count;
        }

        int dataSize(const std::vector<char>& data) {
            int count = std::count_if(data.begin(), data.end(), [](int value) {
                return value != sentinelValue;
            });
            //cout << "Elements in data array: " << count << endl;
            return count;
        }

        int calcSpaceRemaining() {
            cout << "calcSpaceRemaining entered:" << endl;
            // Calculate the space remaining based on current usage. PageHeader, page_size and offsetArraySize are all static members/fixed
            int newSpaceRemaining = page_size - dataSize(data, sentinelValue) - offsetArraySize - sizeof(PageHeader);
            cout << "calcSpaceRemaining::newSpaceRemaining: " << newSpaceRemaining << endl;
            return newSpaceRemaining;
        }


            // Getters
            int getPageNumber() {return pageNumber;}
            Page * getNextPage() {return nextPage;}
            int getDataSize() {return data.size();}
            int getOffsetArraySize() {return offsetArraySize;}
            int getDataVectorSize() {return dataVectorSize;}
            bool checkDataEmpty() {return data.empty();}
            // Getter method to return the next page in the list
            Page * goToNextPage() {
                return this->nextPage;
            }
            

            // Setters
            // Reinitialize data vector elements to sentinel value '\0'
            void emptyData() {fill(data.begin(), data.end(), sentinelValue);}
            // Reinitialize offsetArray elements to sentinel value -1
            void emptyOffsetArray() {fill(offsetArray.begin(), offsetArray.end(), -1);}
            // Method to link this page to the next page
            void setNextPage(Page* nextPage) {
                this->nextPage = nextPage;
            }

            

            // Test function to print the contents of a page as indexed by their offsets
            void printPageContentsByOffset() {

                // Check if page is empty. If so simply return
                if (offsetArrayEmpty() && dataVectorEmpty()) {
                    cout << "Page is empty. No records to print.\n";
                    return;
                };

                // Error check: If offsetArray is empty but data vector is not, print error and exit
                if (offsetArrayEmpty() && !dataVectorEmpty()) {
                    cerr << "Error: Offset array is empty but data vector not empty!\n";
                    exit (-1);
                };
                
                // Error check: If offsetArray is not empty but data vector is empty, print error and exit
                if (!offsetArrayEmpty() && dataVectorEmpty()) {
                    cerr << "Error: Data vector is empty but offset array not empty!\n";
                    exit (-1);
                };

                // Initial print statements
                cout << "printPageContentsByOffset begin" << endl;
                cout << "Page Number: " << pageNumber << endl;
                cout << "Records in Page: " << pageHeader.recordsInPage << endl;
                cout << "Space Remaining: " << pageHeader.spaceRemaining << endl;
                cout << "Offset\t\tBeginning of record\t\tRecord Size\n";

                int elementsInOffsetArray = offsetSize();
                cout << "elements in offset array: " << elementsInOffsetArray << endl;

                for (size_t i = 0; i <  elementsInOffsetArray; ++i) {
                    // Validate the current offset
                    if (offsetArray[i] < 0 || offsetArray[i] >= static_cast<int>(dataSize(data))) {
                        cerr << "Error: Invalid offset " << offsetArray[i] << " at offsetArray index " << i << ". Skipping record.\n";
                        continue;
                    }

                    // Calculate the start and end offsets for the current record
                    int startOffset = offsetArray[i];
                    int endOffset = (i + 1 < elementsInOffsetArray) ? offsetArray[i + 1] : dataSize(data);

                    if (endOffset == dataSize(data)) { cout << "endOffset currently equal to dataSize(data)" << dataSize(data) << endl;}

                    // Validate the end offset
                    if (endOffset > static_cast<int>(dataSize(data))) {
                        cerr << "Error: End offset " << endOffset << " exceeds data vector size. Adjusting to data size.\n";
                        endOffset = dataSize(data);
                    }

                    // Print the offset in hexadecimal and the record's contents
                    cout << "0x" << setw(3) << setfill('0') << hex << startOffset << "\t";
                    for (int j = startOffset; j < endOffset; ++j) {
                        // Ensure printing of printable characters only
                        cout << (isprint(data[j]) ? data[j] : '.');
                    }
                    
                    cout << "\t\t" << dec << endOffset - startOffset << "\n\n";
                    cout << "endoffset: " << endOffset << "\nstartOffset: "<< startOffset << endl;
                }
                cout << "printPageContentsByOffset end" << endl;
            }

            // Method to find the offset of the next record in the data vector
            // Searches for the last non-sentinel character and returns the next position
            int findOffsetOfNextRecord(const std::vector<char>& data, char sentinelValue) {
                // Start from the end of the vector and move backwards
                for (int i = data.size() - 1; i >= 0; --i) {
                    if (data[i] != sentinelValue) {
                        // Found the last non-sentinel character, return the next position
                        return i + 1;
                    }
                }
                // If all characters are sentinel values or the vector is empty, return 0
                return 0;
            }

            // Method to add an offset to the end of offsetArray
            // Bespoke method due to array being filled with sentinel values
            bool addOffsetToFirstSentinel(std::vector<int>& offsetArray, int newOffset) {
                for (size_t i = 0; i < offsetArray.size(); ++i) {
                    if (offsetArray[i] == -1) { // Sentinel value found
                        offsetArray[i] = newOffset; // Replace with new offset
                        return true; // Indicate success
                    }
                }
                return false; // Indicate failure (no sentinel value found)
            }

            // Method to add a record to the page
            bool addRecord(const Record& record) {
                cout << "addRecord begin" << endl;
                bool offsetAdd = false;
                auto recordString = record.toString();

                // This should print out a properly formatted record string with not trailing periods
                cout << "addRecord:: TEST PRINT: Record string: " << recordString << "\n";

                size_t recordSize = recordString.size();
                
                // Check if there's enough space left in the page
                if (recordSize > static_cast<size_t>(pageHeader.spaceRemaining)) {
                    std::cerr << "addRecord::Error: Not enough space in the page to add the record.\n";
                    return false;
                }
                
                // Calculate the offset for the new record
                int offsetOfNextRecord = findOffsetOfNextRecord(data, sentinelValue);

                // Ensure the insertion does not exceed the vector's predefined max size
                if (offsetOfNextRecord + recordSize <= data.size()) {
                    cout << "addRecord:: Adding record to page: " << this->pageNumber << "\n";
                    std::copy(recordString.begin(), recordString.end(), data.begin() + offsetOfNextRecord);
                    pageHeader.recordsInPage += 1;
                    cout << "addRecord:: spaceRemaining - recordSize: " << pageHeader.spaceRemaining << " - " << recordSize << " = " << pageHeader.spaceRemaining - recordSize << endl;
                    pageHeader.spaceRemaining -= recordSize;
                    //offsetArray.push_back(offsetOfNextRecord);

                    if (addOffsetToFirstSentinel(offsetArray, offsetOfNextRecord)) {
                        cout << "addRecord:: Offset added to offsetArray\n";
                    } else {
                        std::cerr << "addRecord:: Error: Unable to add offset to offsetArray.\n";
                        return false;
                    }

                    cout << "addRecord::Test: Confirm record added to page:\nPrinting stored record from main memory: \n";
                    
                    cout << "0x" << setw(3) << setfill('0') << hex << offsetArray.back() << "\t" << dec;
                    for (int i = offsetOfNextRecord; i < offsetOfNextRecord + recordSize; ++i) {
                        cout << data[i];
                    }

                    return true;

                } else if (offsetOfNextRecord + recordSize > data.size()) {
                    std::cerr << "addRecord:: Error: Attempt to exceed predefined max size of data vector.\n";
                    cout << "addRecord failed" << endl;
                    return false;
                } else {
                    std::cerr << "addRecord:: Error: Unknown error occurred while adding record to page.\n";
                    cout << "addRecord failed" << endl;
                    return false;
                }
                cout << "addRecord successful" << endl;
            }

            // Method to write the contents of this page to a file
            int writeRecordsToFile(std::ofstream& outputFile, int offsetSize, int pageID) {
                if (!outputFile.is_open()) {
                    std::cerr << "Error: Output file is not open for writing.\n";
                    return -1; // Indicate error
                }

               

                for (int i = 0; i < offsetSize - 1; ++i) { // -1 to prevent accessing beyond the last valid index
                    int startOffset = offsetArray[i];
                    int endOffset = offsetArray[i + 1]; // Get the end offset for the current segment

                    // Write each character in the range [startOffset, endOffset) to the file
                    for (int j = startOffset; j < endOffset; ++j) {
                        if (data[j] != sentinelValue) {
                            outputFile.write(reinterpret_cast<const char*>(&data[j]), sizeof(data[j]));
                        }
                    }
                }

                // Optionally, return the new offset after writing, if needed
                return outputFile.tellp();
            }

        }; // End of Page definition

#endif