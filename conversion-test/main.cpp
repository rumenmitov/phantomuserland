#include <cstring>
#include <iostream>
#include <climits>

int main(int argc, char* argv[]) {
    std::cout << "Int limit: " << INT_MAX << std::endl;
    std::cout << "Size of int: " << sizeof(int) << std::endl;

    char charArray[] = "hello world";
    int* intArray = nullptr;
    intArray = (int*)charArray;

    for (int i = 0; i < sizeof(charArray) / sizeof(int); i++) {
        std::cout << intArray[i] << ", ";
    }

    std::cout << "\nnow charArray:" << std::endl;
    char *charArray2 = (char*) intArray;

    for (int i = 0; i < sizeof(charArray2); i++) {
        std::cout << charArray2[i] << ", ";
    }
    
    return 0;
}
