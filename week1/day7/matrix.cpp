#include <iostream>
#include <cassert>
#include <cstddef>
#include <chrono>

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
    const T& operator[](size_t index) const; // <--- Add for Day7 Matrix implementation
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

// DAY 7, CONST OPERATOR IMPLEMENTATION FOR MATRIX
template < typename T>
const T& Vec<T>::operator[](size_t index) const{
    return data_[index];
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

//========================================================
//========================================================
//                DAY 7 IMPLEMENTATION
//========================================================
//========================================================

class Matrix{
public:
    Matrix(size_t rows, size_t cols);
    double& operator()(size_t row, size_t col);
    const double& operator()(size_t row, size_t col) const;
    size_t rows() const;
    size_t cols() const;

    


private:
    Vec<double> data_;
    size_t rows_;
    size_t cols_;


};

//Constructor
Matrix::Matrix(size_t rows, size_t cols): rows_(rows), cols_(cols){
    size_t total = rows * cols;

    for(size_t i = 0; i < total; ++i){
        data_.push_back(0.0);
    }
} 

//Non-const Operator()
double& Matrix::operator()(size_t row, size_t col){
    return data_[row * cols_ + col];
}

//Const Operator()
const double& Matrix::operator()(size_t row, size_t col) const{
    return data_[row * cols_ + col];
}


//GETTERS
size_t Matrix::rows() const{
    return rows_;
}
size_t Matrix::cols() const{
    return cols_;
}


Matrix multiply_naive(const Matrix& A, const Matrix& B){
    Matrix C(A.rows(), B.cols());

    for(size_t i = 0; i < A.rows(); ++i){
        for(size_t j = 0; j < B.cols(); ++j){
            double sum = 0.0;
            for(size_t k = 0; k < A.cols(); ++k){
                sum+= A(i,k) * B(k,j);
            }
            C(i,j) = sum;
        }

    }
    return C;
}
Matrix multiply_reordered(const Matrix& A, const Matrix& B){
    Matrix C(A.rows(), B.cols());

    for(size_t i = 0; i < A.rows(); ++i){
        for(size_t k = 0; k < A.cols(); ++k){
            double value_saved = A(i,k);
            for(size_t j = 0; j < B.cols(); ++j){
                C(i,j) += value_saved * B(k,j);
            }
        }

    }
    return C;
}


int main(){
    size_t n = 512;
    Matrix A(n,n), B(n,n);


    // FILLING A AND B WITH VALUES
    for(size_t i = 0; i < n; ++i){
        for(size_t j = 0; j < n; ++j){
            A(i,j) = static_cast<double>(i+j);
            B(i,j) = static_cast<double>(i-j);
        }
    }

    auto t0 = std::chrono::steady_clock::now();
    Matrix C1 = multiply_naive(A,B);
    auto t1 = std::chrono::steady_clock::now();
    Matrix C2 = multiply_reordered(A,B);
    auto t2 = std::chrono::steady_clock::now();

    std::chrono::duration<double> naive_time = t1 - t0;
    std::chrono::duration<double> reordered_time = t2-t1;

    bool match = true;
    for (size_t i = 0; i < n && match; ++i) {
        for (size_t j = 0; j < n; ++j) {
            if (std::abs(C1(i,j) - C2(i,j)) > 1e-9) {
                match = false;
                break;
            }
        }
    }
    std::cout << "results match: " << (match ? "yes" : "NO - BUG") << "\n";   

    std::cout << "naive: " << naive_time.count() << "s\n";
    std::cout << "reordered: " << reordered_time.count() << "s\n";

    std::cout << "speedup: " << naive_time.count() / reordered_time.count() << " times\n";

    return 0;
}