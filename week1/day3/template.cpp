/*
  DAY 3: 
  - Template
  - begin()/end()
  - verify range
  - verify std::sort, std::find
  - Add emplace_back
  - Write tests
*/
#include <iostream>
#include <cassert>
#include <cstddef>

template <typename T> class Vec{
public:

    // invariant: size_ <= capacity_
    // invariant: data_ is nullptr iff capacity_ == 0, else points to capacity_ ints

    Vec();
    ~Vec(); //Destructor Declarator

    Vec(const Vec& other); // Copy Constructor Declaration
    //Vec& operator=(const Vec& other); // Copy Assignment Declaration
    Vec(Vec&& other) noexcept; // Move Constructor Declaration
    //Vec& operator=(Vec&& other) noexcept;  // Move Assignment Declaration
    Vec& operator=(Vec other);

    T* begin();
    T* end();

    // Friend swap function, noexcept.
    friend void swap(Vec& first, Vec& second) noexcept{
        using std::swap;
        swap(first.data_, second.data_);
        swap(first.size_, second.size_);
        swap(first.capacity_, second.capacity_);

    }

    void push_back(T value);
    void pop_back();
    T& operator[](size_t index);
    size_t size() const;
    size_t capacity() const;

private:
    T* data_;
    size_t size_;
    size_t capacity_;
    void grow_ (size_t new_capacity);

};

template <typename T>
Vec<T>::Vec(): data_(nullptr), size_(0), capacity_(0){

}

// Destructor Implementation
template <typename T>
Vec<T>::~Vec(){
    delete[] data_;
}

// Copy Constructor Implementation
template <typename T>
Vec<T>::Vec(const Vec& other){
    std::cout << "copy constructor\n";

    T* new_data = new T[other.capacity_];
    for(size_t i =0; i < other.size_; ++i){
        new_data[i] = other.data_[i];
    }
    this->data_ = new_data;
    this->size_ = other.size_;
    this->capacity_= other.capacity_;

}

// Copy Assignment Implementation
// Vec& Vec::operator=(const Vec& other){ 
//     std::cout << "copy assignment\n";
//     if(this == &other){
//         return *this;
//     }
//     int* new_data = new int[other.capacity_];
//     for(size_t i =0; i < other.size_; ++i){
//         new_data[i] = other.data_[i];
//     }
//     delete[] this->data_;
//     this->data_ = new_data;

//     this->size_ = other.size_;
//     this->capacity_ = other.capacity_;

//     return *this;
// }

// Move Constructor Implementation
// '&&' means "this argument is something disposable (a temporary, or something wrapped in std::move), so it's safe to steal from."
// using noexcept promises it won't throw so it will use the faster move instead of copy
template <typename T>
Vec<T>::Vec(Vec&& other) noexcept{
    std::cout << "move constructor\n";
    this->data_ = other.data_;
    this->capacity_ = other.capacity_;
    this->size_ = other.size_;
    
    other.data_ = nullptr;
    other.capacity_ = 0;
    other.size_ = 0;
}

// Move Assignment Implementation
// Vec& Vec::operator=(Vec&& other) noexcept{
//     std::cout << "move assignment\n";
//     if(this == &other){
//         return *this;
//     }

//     delete[] this->data_;
//     this->data_ = other.data_;
//     this->capacity_ = other.capacity_;
//     this->size_ = other.size_;

    
//     other.data_ = nullptr;
//     other.capacity_ = 0;
//     other.size_ = 0;

//     return *this;
// }

//====================
// With Copyy-and-Swap Idiom, I replaced Move/Copy assignment with the following function.
/*==================================================
Explanation:
By passing the parameter by value, a single assignment operator replaces both manual copy and move assignments. 
The compiler automatically copy-constructs or move-constructs the temporary parameter, which is then swapped with the current object's resources. 
This unified approach eliminates the need for self-assignment checks because the old data 
is safely destroyed when the temporary goes out of scope.
*/
template <typename T>
Vec<T>& Vec<T>::operator=(Vec other){
    swap(*this, other);
    return *this;
}
template <typename T>
size_t Vec<T>::capacity() const{
    return capacity_;
}
template <typename T>
size_t Vec<T>::size() const{
    return size_;

}

template <typename T>
T& Vec<T>::operator[](size_t index){
    assert(index < size_);
    return data_[index];
}

template <typename T>
void Vec<T>::grow_ (size_t new_capacity){
    T* new_data = new T[new_capacity];

    for(size_t i = 0; i < size_; ++i){
        new_data[i] = data_[i];
    }

    delete [] data_;
    
    data_ = new_data;
    capacity_ = new_capacity;
}


template <typename T>
void Vec<T>::push_back(T value){
    if(size_ == capacity_){
        grow_(size_ == 0? 1: capacity_*2);
    }
    data_[size_] = value;
    size_++;
}
template <typename T>
void Vec<T>::pop_back(){
    if(size_ > 0){
        size_--;
    }
}
template <typename T>
Vec<T> f() {
    Vec<T> v;
    v.push_back(1);
    return v;
}

template <typename T>
T* Vec<T>::begin(){
    return data_;
}

template <typename T>
T* Vec<T>::end(){
    return data_ + size_;
}

int main(){
    size_t count = 1'000'000;
    Vec<int> v1;
    for(size_t i = 0; i < count; ++i){
        v1.push_back(i);
    }
    std::cout << v1[0] << ", " << v1[1] << ", " << v1[2] << ", " << v1[3]  << std::endl;
    std::cout << "Size: " << v1.size() << ", Capacity: " << v1.capacity() << std::endl;

    v1.pop_back();
    std::cout << "Size: " << v1.size() << ", Capacity: " << v1.capacity() << std::endl;
    
    v1 = v1;

    Vec<int> v2 = std::move(v1); 
    //Vec v2 = v1; //Causes double-free error(Without the copy constructor)

    // TEST FOR MOVE CONSTRUCTOR:
    Vec<int> a;
    a.push_back(1);
    a.push_back(2);
    Vec<int> b = std::move(a);
    a.push_back(99);
    std::cout << "a.size()=" << a.size() << " b.size()=" << b.size() << "\n";
    std::cout << "b[0]=" << b[0] << " b[1]=" << b[1] << "\n";

    b = std::move(a);

    //Named Return Value Optimization, the compiler optimizes the code
    // It constructs the object v directly inside the memory space reserved for result in the main function.
    Vec<int> result = f<int>();
    for(size_t i = 0; i < result.size(); ++i){
            std::cout << result[i] << std::endl;
    }
    
    
    return 0;
}

