#include <iostream>
#include <cassert>
#include <cstddef>

class Vec{
public:

    // invariant: size_ <= capacity_
    // invariant: data_ is nullptr iff capacity_ == 0, else points to capacity_ ints

    Vec();
    ~Vec();

    void push_back(int value);
    void pop_back();
    int& operator[](size_t index);
    size_t size() const;
    size_t capacity() const;

private:
    int* data_;
    size_t size_;
    size_t capacity_;
    void grow_ (size_t new_capacity);

};

Vec::Vec(): data_(nullptr), size_(0), capacity_(0){

}

Vec::~Vec(){
    delete[] data_;
}

size_t Vec::capacity() const{
    return capacity_;
}
size_t Vec::size() const{
    return size_;

}

int& Vec::operator[](size_t index){
    assert(index < size_);
    return data_[index];
}

void Vec::grow_ (size_t new_capacity){
    int* new_data = new int[new_capacity];

    for(size_t i = 0; i < size_; ++i){
        new_data[i] = data_[i];
    }

    delete [] data_;
    
    data_ = new_data;
    capacity_ = new_capacity;
}

void Vec::push_back(int value){
    if(size_ == capacity_){
        grow_(size_ == 0? 1: capacity_*2);
    }
    data_[size_] = value;
    size_++;
}

void Vec::pop_back(){
    if(size_ > 0){
        size_--;
    }
}

int main(){
    size_t count = 1'000'000;
    Vec v1;
    for(size_t i = 0; i < count; ++i){
        v1.push_back(i);
    }
    std::cout << v1[0] << ", " << v1[1] << ", " << v1[2] << ", " << v1[3]  << std::endl;
    std::cout << "Size: " << v1.size() << ", Capacity: " << v1.capacity() << std::endl;

    v1.pop_back();
    std::cout << "Size: " << v1.size() << ", Capacity: " << v1.capacity() << std::endl;

    Vec v2 = v1; //Causes double-free error
    
    
    return 0;
}