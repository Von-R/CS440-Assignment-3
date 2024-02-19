# ifndef PAGE_H
# define PAGE_H

# include <string>
# include <vector>
# include <algorithm>
#include <iomanip>
#include <fstream> // Include the necessary header file
# include "classes.h"

const int MAX_PAGES = 4;

class Page {
        //static const int BLOCK_SIZE = BLOCK_SIZE; // Define the size of each page (4KB)
        int pageNumber; // Identifier for the page
        Page *nextPage; // Pointer to the next page in the list
        Page *lastPage; // Pointer to the last page in the list
        // These are static members of the Page class because the offsetArray and dataVector sizes will be shared by all instances of the Page class
        int static dataVectorSize;
        static const char sentinelValue = '\0'; // Sentinel value to indicate empty space in the data vector
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
        int elemsInDataVector(const std::vector<char>& data, char sentinelValue) {
            //cout << "elemsInDataVector begin" << endl;
            return std::count_if(data.begin(), data.end(), [sentinelValue](char value) {
            return value != sentinelValue;
            });
            //cout << "elemsInDataVector end" << endl;
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
            int indexPage = false;

            if (pageNum == 0) {
                indexPage = true;
            }
            // Size of data vector; calculated based on the size of the offset array and the size of the page header
            dataVectorSize = BLOCK_SIZE - sizeof(PageHeader);
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

        int elemsInDataVector(const std::vector<char>& data) {
            int count = std::count_if(data.begin(), data.end(), [](int value) {
                return value != sentinelValue;
            });
            //cout << "Elements in data array: " << count << endl;
            return count;
        }

        int calcSpaceRemaining() {
            cout << "calcSpaceRemaining entered:" << endl;
            // Calculate the space remaining based on current usage. PageHeader, BLOCK_SIZE and offsetArraySize are all static members/fixed
            int newSpaceRemaining = BLOCK_SIZE - elemsInDataVector(data, sentinelValue) - sizeof(PageHeader);
            cout << "calcSpaceRemaining::newSpaceRemaining: " << newSpaceRemaining << endl;
            return newSpaceRemaining;
        }


            // Getters
            int getPageNumber() {return pageNumber;}
            Page * getNextPage() {return nextPage;}
            int getelemsInDataVector() {return data.size();}
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
            // Method to link this page to the next page
            void setNextPage(Page* nextPage) {
                this->nextPage = nextPage;
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

            // Method to add a record to the page
            bool addRecordToMemory(const Record& record) {
                cout << "addRecord begin" << endl;
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
                    
                    bitset<8> hashID = record.getHashID();
                    cout << "0x" << setw(3) << setfill('0') << "\t" << dec;

                    return true;

                } else if (recordSize > data.size()) {
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
            /*
            Concept: 
            Iterate through data vector, capture each record 
            For each record, consult index to find the appropriate bucket
            Read bucket header in memory, deserialize and determine if there's room, a split is needed or an overflow bucket already exists for this hash value

                1. If there's room, write the record to the bucket

                If there's no room: 
                2. If a split is needed, split the bucket and write the record to the correct bucket
                3. If overflow bucket exists, seek to it, read bucket header and repeat process
                4. If split is not needed and no overflow bucket exists, create a new overflow bucket and write the record to it. Update index

            Update bucket header in memory and write to file
            
            */
            int writeRecordsToBucket(std::fstream& outputFile, int offsetSize, int blockOffset, int pageID) {
                

                // ...

                                std::ofstream outputFile; // Change the type to std::ofstream

                                if (!outputFile.is_open()) {
                                    std::cerr << "Error: Output file is not open for writing.\n";
                                    return -1; // Indicate error
                                }
                int i = 0;

                while (data[i] != sentinelValue) {
                    outputFile.write(reinterpret_cast<const char*>(&data[i]), sizeof(data[i]));
                    i++;
                }

                // Optionally, return the new offset after writing, if needed
                // May be unnecessary for this project
                return outputFile.tellp();
            }

}; // End of Page definition

class indexPage : public Page  {
        //static const int BLOCK_SIZE = BLOCK_SIZE; // Define the size of each page (4KB)
        int pageNumber; // Identifier for the page
        Page *nextPage; // Pointer to the next page in the list
        // These are static members of the Page class because the offsetArray and dataVector sizes will be shared by all instances of the Page class
        int static dataVectorSize;
        static const char sentinelValue = '\0'; // Sentinel value to indicate empty space in the data vector
        vector<char> indexVector; // Vector to store the data in the page

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
        int elemsInDataVector(const std::vector<char>& data, char sentinelValue) {
            //cout << "elemsInDataVector begin" << endl;
            return std::count_if(data.begin(), data.end(), [sentinelValue](char value) {
            return value != sentinelValue;
            });
            //cout << "elemsInDataVector end" << endl;
        }


    public:
        struct PageHeader {
            int recordsInPage; // Number of records in the page
            int spaceRemaining; // Updated to be initialized later in Page constructor

            PageHeader() : recordsInPage(0) {} // Constructor simplified
        } pageHeader;

        // Constructor for the Page class
        indexPage(int pageNum) : pageNumber(pageNum), nextPage(nullptr) {
            cout << "Page constructor begin" << endl;
            int indexPage = false;

            if (pageNum == 0) {
                indexPage = true;
            }
            // Size of data vector; calculated based on the size of the offset array and the size of the page header
            dataVectorSize = BLOCK_SIZE - sizeof(PageHeader);
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

        int elemsInDataVector(const std::vector<char>& data) {
            int count = std::count_if(data.begin(), data.end(), [](int value) {
                return value != sentinelValue;
            });
            //cout << "Elements in data array: " << count << endl;
            return count;
        }

        int calcSpaceRemaining() {
            cout << "calcSpaceRemaining entered:" << endl;
            // Calculate the space remaining based on current usage. PageHeader, BLOCK_SIZE and offsetArraySize are all static members/fixed
            int newSpaceRemaining = BLOCK_SIZE - elemsInDataVector(data, sentinelValue) - sizeof(PageHeader);
            cout << "calcSpaceRemaining::newSpaceRemaining: " << newSpaceRemaining << endl;
            return newSpaceRemaining;
        }


            // Getters
            int getPageNumber() {return pageNumber;}
            Page * getNextPage() {return nextPage;}
            int getelemsInDataVector() {return data.size();}
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
            // Method to link this page to the next page
            void setNextPage(Page* nextPage) {
                this->nextPage = nextPage;
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

            // Method to add a record to the page
            bool addRecordToMemory(const Record& record) {
                cout << "addRecord begin" << endl;
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
                    
                    bitset<8> hashID = record.getHashID();
                    cout << "0x" << setw(3) << setfill('0') << "\t" << dec;

                    return true;

                } else if (recordSize > data.size()) {
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
            /*
            Concept: 
            Iterate through data vector, capture each record 
            For each record, consult index to find the appropriate bucket
            Read bucket header in memory, deserialize and determine if there's room, a split is needed or an overflow bucket already exists for this hash value

                1. If there's room, write the record to the bucket

                If there's no room: 
                2. If a split is needed, split the bucket and write the record to the correct bucket
                3. If overflow bucket exists, seek to it, read bucket header and repeat process
                4. If split is not needed and no overflow bucket exists, create a new overflow bucket and write the record to it. Update index

            Update bucket header in memory and write to file
            
            */
            int writeRecordsToBucket(std::fstream& outputFile, int offsetSize, int blockOffset, int pageID) {
                

                // ...

                                std::ofstream outputFile; // Change the type to std::ofstream

                                if (!outputFile.is_open()) {
                                    std::cerr << "Error: Output file is not open for writing.\n";
                                    return -1; // Indicate error
                                }
                int i = 0;

                while (data[i] != sentinelValue) {
                    outputFile.write(reinterpret_cast<const char*>(&data[i]), sizeof(data[i]));
                    i++;
                }

                // Optionally, return the new offset after writing, if needed
                // May be unnecessary for this project
                return outputFile.tellp();
            }

}; // End of indexPage definition

 class PageList {
            public:
            Page *head = nullptr;
            //static const int maxPages = 3; 
            // Constructor for the PageList class
            // Head is initialized as page 0 and linked list created from it
            PageList() {
                cout << "PageList constructor begin" << endl;
                head = initializePageList();
                cout << "PageList constructor end" << endl;
            }

            // Create 'maxPages' number of pages and link them together
            // Initialization of head member attirbutes done in head constructor
            Page* initializePageList() {
                cout << "initializePageList begin" << endl;
                Page *tail = nullptr;
                for (int i = 0; i < maxPages; ++i) {
                    cout << "Creating page " << i << endl;
                    Page *newPage = new Page(i); // Create a new page
                    cout << "Page " << i << " created" << endl;

                    if (!head) {
                        cout << "Setting head to page " << i << endl;
                        head = newPage; // If it's the first page, set it as head
                    } else {  
                        cout << "Linking page " << i << " to page " << i - 1 << endl;
                        tail->setNextPage(newPage); // Otherwise, link it to the last page
                    }
                    cout << "Setting tail to page " << i << endl;
                    tail = newPage; // Update the tail to the new page
                }
                cout << "initializePageList end" << endl;
                return head; // Return the head of the list
            };

              // Destructor for the PageList class
            ~PageList() {
                cout << "PageList destructor begin" << endl;
                Page* current = head;
                while (current != nullptr) {
                    Page* temp = current->getNextPage(); // Assuming nextPage points to the next Page in the list
                    delete current; // Free the memory of the current Page
                    current = temp; // Move to the next Page
                };
                cout << "PageList destructor end" << endl;
            };

            // Method to reset the data and offsetArray of each page in the list to initial values
            void resetPages() {
                cout << "resetPage begin" << endl;
                Page * current = head;
                while (current != nullptr) {
                    current->emptyData();
                    current->pageHeader.recordsInPage = 0;
                    current->pageHeader.spaceRemaining = current->calcSpaceRemaining();
                    current = current->getNextPage();
                }
                cout << "resetPage end" << endl;
            }

            bool dumpPages(ofstream& file, int& pagesWrittenToFile) {
                cout << "Dumping pages to file...\n";

                if (!file.is_open()) {
                    cerr << "File is not open for writing.\n";
                    return false;
                }

                Page* currentPage = head;
                int pageOffset = 0;
                while (currentPage != nullptr) {
                    // Assume currentPage->writeRecordsToFile() handles serialization of page data
                    if (!currentPage->writeRecordsToBucket(file, currentPage->getPageNumber()) {
                        cerr << "Failed to write page records to file.\n";
                        return false;
                    }

                    // Update the page directory with new entry
                    int currentPageOffset = /* Calculate current page offset in the file */;
                    int currentPageRecordCount = currentPage->getRecordCount();
                    // Add the new directory entry
                    // Assuming you have a method to handle adding and serializing the page directory entry
                    addPageDirectoryEntry(currentPageOffset, currentPageRecordCount);

                    pagesWrittenToFile++;
                    currentPage = currentPage->getNextPage();
                }

                // Serialize and write the updated page directory and file header here
                // Reset the pages for reuse
                resetPages();
                cout << "Page dumping complete.\n";
                return true;
            }

            

            /*
            Test function: Use to confirm page is being filled properly. 
            */
            void printMainMemory() {
                cout << "Printing main memory contents...\n";
                Page* page = head;
                while (page != nullptr) {
                    // Print the page number as a header for each page's content
                    cout << "\n\nPrinting contents of Page Number: " << page->getPageNumber() << endl;
                    // Now, use the printPageContentsByOffset method to print the contents of the current page
                    //page->printPageContentsByOffset();
                    // Move to the next page in the list
                    page = page->getNextPage();
                }
                cout << "End of main memory contents.\n";
            }

        }; // End of PageList definition

#endif