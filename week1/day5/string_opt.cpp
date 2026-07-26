#include <iostream>
#include <cstring>
#include <algorithm>

class Mystring{
public:
    static int heap_alloc_count;
    static constexpr size_t INLINE_CAPACITY = 15;

    Mystring(const char* s); //constructor
    Mystring(const Mystring& other); //copy constructor
    Mystring& operator=(const Mystring& other); //copy assignment
    Mystring(Mystring&& other) noexcept; //Move constructor
    Mystring& operator=(Mystring&& other) noexcept; // Move assignment

    ~Mystring(){
        if(is_heap_){
            delete[] heap_.data_;
        }
    }


    const char* c_str() const; // return usable C-string pointer
    size_t size() const;
    bool operator==(const Mystring& other) const; 
    Mystring& operator+=(const char* suffix);


private:

    union{
        char inline_data_[INLINE_CAPACITY + 1];
        struct {
            char* data_;
            size_t capacity_;
        } heap_;


    };
    size_t size_ ;
    bool is_heap_;

};

int Mystring::heap_alloc_count = 0;

Mystring::Mystring(const char* s){
    size_t length = std::strlen(s);
    size_ = length;
    if(length <= INLINE_CAPACITY){
        is_heap_ = false;
        std::memcpy(inline_data_, s,  length );
        inline_data_[length] = '\0'; 
    }
    else{
        is_heap_ = true;
        heap_.capacity_ = length+1;
        heap_.data_ = new char[length + 1];

        std::memcpy(heap_.data_, s, length + 1);
        heap_.data_[length] = '\0';
        ++Mystring::heap_alloc_count;
    }
}

//Copy constructor implementation.
Mystring::Mystring(const Mystring& other){
    size_ = other.size_;
    is_heap_ = other.is_heap_;

    if(!other.is_heap_){
        std::memcpy(inline_data_, other.inline_data_, other.size_ + 1);
    }
    else{
        heap_.capacity_ = other.heap_.capacity_;
        heap_.data_ = new char[heap_.capacity_];
        std::memcpy(heap_.data_, other.heap_.data_, other.size_ + 1);
        ++Mystring::heap_alloc_count;
    }
}

//Copy assignment implementation.
Mystring& Mystring::operator=(const Mystring& other){
    if(this == &other){
        return *this;
    }

    if(is_heap_){
        delete[] heap_.data_;
    }

    size_ = other.size_;
    is_heap_ = other.is_heap_;
    
    if(!other.is_heap_){
        std::memcpy(inline_data_, other.inline_data_, other.size_+1);
    }
    else{
        heap_.capacity_ = other.heap_.capacity_;
        heap_.data_ = new char[heap_.capacity_];
        std::memcpy(heap_.data_, other.heap_.data_, other.size_+1);
        ++Mystring::heap_alloc_count;
    }
    return *this;

}

//Move constuctor Implementation.
Mystring::Mystring(Mystring&& other) noexcept{
    size_ = other.size_;
    is_heap_ = other.is_heap_;

    if(!other.is_heap_){
        std::memcpy(inline_data_, other.inline_data_, other.size_ + 1);
    }
    else{
        heap_.data_ = other.heap_.data_;
        heap_.capacity_ = other.heap_.capacity_;

        other.heap_.data_ = nullptr;
        other.heap_.capacity_ = 0;
        other.size_ = 0;
        other.is_heap_ = false;
    }

}


//Move assignment Implementation.
Mystring& Mystring::operator=(Mystring&& other) noexcept{

    if(this == &other){
        return *this;
    }

    if(is_heap_){
        delete[] heap_.data_;
    }

    size_ = other.size_;
    is_heap_ = other.is_heap_;

    if(!other.is_heap_){
        std::memcpy(inline_data_, other.inline_data_, other.size_ + 1);
    }
    else{
        heap_.data_ = other.heap_.data_;
        heap_.capacity_ = other.heap_.capacity_;

        other.heap_.data_ = nullptr;
        other.heap_.capacity_ = 0;
        other.size_ = 0;
        other.is_heap_ = false;
    }
    
    return *this;
}

const char* Mystring::c_str() const{
    if(is_heap_){
        return heap_.data_;
    }
    return inline_data_;
}

size_t Mystring::size() const{
    return size_;
}

bool Mystring::operator==(const Mystring& other) const{
    if(size_ != other.size_){
        return false;
    }
    return std::memcmp(c_str(), other.c_str(), size_) == 0;
} 

Mystring& Mystring::operator+=(const char* suffix){
    size_t suffix_len = strlen(suffix);
    size_t new_size = size_ + suffix_len;

    if(!is_heap_){
        // Case 1: currently inline, fits inline
        if(new_size <= INLINE_CAPACITY){
            std::memcpy(inline_data_ + size_, suffix, suffix_len);
            inline_data_[new_size] = '\0';
        }
        // Case 2: currently inline, must transition to heap
        else{
            char* new_data = new char[new_size+1];

            std::memcpy(new_data, inline_data_, size_);
            std::memcpy(new_data + size_, suffix, suffix_len);
            new_data[new_size] = '\0';

            heap_.data_ = new_data;
            heap_.capacity_ = new_size + 1;
            is_heap_ = true;
            ++Mystring::heap_alloc_count;
        }
    }
    else{
        //Case 3: Currently heap, existing capacity has room
        if(new_size + 1<= heap_.capacity_){
            std::memcpy(heap_.data_ + size_, suffix, suffix_len);
            heap_.data_[new_size] = '\0';
        }
        else{
            //Case 4: Currently Heap, must reallocate to larger block
            size_t double_capacity = heap_.capacity_ * 2;
            size_t required_capacity = new_size + 1;
            size_t new_capacity = std::max(double_capacity,  required_capacity);
            char* new_data = new char[new_capacity];

            std::memcpy(new_data, heap_.data_, size_);
            std::memcpy(new_data + size_, suffix, suffix_len);

            new_data[new_size] = '\0';
            delete[] heap_.data_;

            heap_.data_ = new_data;
            heap_.capacity_ = new_capacity;
            ++Mystring::heap_alloc_count;
        }
    }



    size_ = new_size;
    return *this;
}


int main(){
    
    //-----Basix checks-----
    Mystring a("hello");
    Mystring b("world");
    std::cout << a.c_str() << " " << b.c_str() << "\n";
    std::cout << "a == b: " << (a == b) << "\n";

    Mystring c = a; //copy constructor
    c += " there"; // stays inline if short enough
    std::cout << "c: " << c.c_str() << " size=" << c.size() << "\n";

    Mystring d = std::move(c); // move constuctor
    std::cout << "d: " << d.c_str() << "\n";


    // ----- Force Heap Transition -----
    Mystring e("short");
    e += " but this suffix makes it defo longer than fifteen characters";
    std::cout << "e: " << e.c_str() << " size=" << e.size() << '\n';

    std::cout << "heap allocations so far: " << Mystring::heap_alloc_count << "\n";

    // ------ SBO stress test: short strings should allocate ZERO ------

    int before = Mystring::heap_alloc_count;
    for(int i = 0; i < 100000; ++i){
        Mystring s1("abc");
        Mystring s2 = s1; // copy
        Mystring s3 = std::move(s2); // move
        s3 += "xy";  // stays inline (5 chars);
        Mystring s4("zz");
        s4 = s3; // copy assignment;
    }
    std::cout<<"allocations during short-string stress: " << (Mystring::heap_alloc_count - before) << " (should be 0)\n";

    // ------ Mixed stress test,  for ASan -----
    before = Mystring::heap_alloc_count;
    for(int i = 0; i < 50000; ++i){
        Mystring s1(i%2 == 0 ? "short" : "this string is intentionally over fifteen characters long");
        Mystring s2 = s1;
        Mystring s3 = std::move(s1);
        s3 += "-suffix-that-might-push-over-the-inline-limit";
        Mystring s4 = s3;
        s4 = std::move(s3);
    }
    std::cout << "allocations during mixed stress: " << (Mystring::heap_alloc_count - before) << '\n';


    return 0;
}