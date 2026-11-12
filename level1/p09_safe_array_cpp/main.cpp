#include <cstddef>
#include <exception>
#include <iostream>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

// One class covers all three requirements: int arrays, operator[], and templates.
// std::vector manages storage and gives copied arrays independent contents.
template <typename T = int>
class SafeArray {
public:
    using reference = typename std::vector<T>::reference;
    using const_reference = typename std::vector<T>::const_reference;

    // Elements start with their type's default value (for example, 0 for int).
    explicit SafeArray(std::ptrdiff_t length = 0)
        : data_(checkedLength(length)) {}

    std::size_t size() const noexcept {
        return data_.size();
    }

    bool empty() const noexcept {
        return data_.empty();
    }

    // Requirement 1: checked reading and writing without operator overloading.
    const_reference get(std::ptrdiff_t index) const {
        return data_[checkedIndex(index)];
    }

    void set(std::ptrdiff_t index, const T& value) {
        data_[checkedIndex(index)] = value;
    }

    // Requirement 2: both writable and read-only subscripts check bounds.
    reference operator[](std::ptrdiff_t index) {
        return data_[checkedIndex(index)];
    }

    const_reference operator[](std::ptrdiff_t index) const {
        return get(index);
    }

private:
    std::vector<T> data_;

    static std::size_t checkedLength(std::ptrdiff_t length) {
        if (length < 0) {
            throw std::invalid_argument("Array length cannot be negative.");
        }
        return static_cast<std::size_t>(length);
    }

    std::size_t checkedIndex(std::ptrdiff_t index) const {
        if (index < 0 || static_cast<std::size_t>(index) >= data_.size()) {
            throw std::out_of_range(
                "Index " + std::to_string(index)
                + " is out of range for array size "
                + std::to_string(data_.size()) + ".");
        }
        return static_cast<std::size_t>(index);
    }
};

int main() {
    try {
        std::cout << "1. Integer array: set() and get()\n";
        SafeArray<int> numbers(5);
        for (std::ptrdiff_t i = 0; i < 5; ++i) {
            numbers.set(i, static_cast<int>((i + 1) * 10));
        }
        for (std::ptrdiff_t i = 0; i < 5; ++i) {
            std::cout << numbers.get(i) << ' ';
        }
        std::cout << "\nSize: " << numbers.size() << "\n\n";

        std::cout << "2. Checked operator[]\n";
        numbers[2] = 99;
        const SafeArray<int>& readOnly = numbers;
        std::cout << "After numbers[2] = 99: " << readOnly[2] << '\n';

        SafeArray<int> copied = numbers;
        copied[0] = 777;
        std::cout << "Independent copy: original[0] = " << numbers[0]
                  << ", copy[0] = " << copied[0] << "\n\n";

        std::cout << "3. Template arrays\n";
        SafeArray<double> decimals(2);
        decimals[0] = 1.5;
        decimals[1] = 2.75;
        std::cout << "double: " << decimals[0] << ' ' << decimals[1] << '\n';

        SafeArray<std::string> words(2);
        words.set(0, "Hello");
        words[1] = "SafeArray";
        std::cout << "string: " << words[0] << ' ' << words[1] << "\n\n";

        std::cout << "4. Bounds errors (caught so the program can continue)\n";
        for (const std::ptrdiff_t index : {-1, 5}) {
            try {
                std::cout << readOnly[index] << '\n';
            } catch (const std::out_of_range& error) {
                std::cout << "Error: " << error.what() << '\n';
            }
        }

        try {
            numbers.set(5, 100);
        } catch (const std::out_of_range& error) {
            std::cout << "Write error: " << error.what() << '\n';
        }

        SafeArray<int> emptyArray;
        std::cout << "Empty array size: " << emptyArray.size() << '\n';
        try {
            emptyArray[0] = 1;
        } catch (const std::out_of_range& error) {
            std::cout << "Empty array error: " << error.what() << '\n';
        }

        try {
            SafeArray<int> invalidArray(-3);
        } catch (const std::invalid_argument& error) {
            std::cout << "Length error: " << error.what() << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << "Unexpected error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
