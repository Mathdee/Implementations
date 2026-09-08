#include <iostream>
#include <vector>
#include <cstdlib>
#include <chrono>
#include <new>

class Arena{
public:
    explicit Arena(size_t size_bytes);
    ~Arena();

    // Arenas own a unique block of memory, copying would double-free at destruction so deleting the operation. 
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&& other) noexcept;
    Arena& operator=(Arena&& other) noexcept;


    // returns a pointer with no type attached, called decides what type to construct.
    void* allocate(size_t size, size_t align);


    //Constructs a T directly inside arena.
    template <typename T, typename... Args>
    T* create(Args&&... args);

    //Instead of deallocate, use reset which throws everything away at once.
    void reset();

    


private:
    char* buffer_;    // The one big block, raw bytes.
    size_t capacity_; // total size of the block.
    size_t offset_;   // how many bytes from the start are already handed out.

};

/*
// malloc gives raw, untyped memory, new[] implies an array of a specific type.
// malloc returns void*, so static_cast<char*> converts it to the pointer type we need.
// capacity_ stores the total number of bytes available.
// offset_ starts at 0 because nothing has been allocated yet.
// free releases the entire raw memory block when the arena is destroyed.
*/

//Constructor:
Arena::Arena(size_t size_bytes)
    :buffer_(static_cast<char*>(std::malloc(size_bytes))),
    capacity_(size_bytes),
    offset_(0)
{}

//Destructor:
Arena::~Arena(){
    std::free(buffer_);
}


/*
// current is the current bump-pointer address, represented as a number for bit math.
// aligned rounds current up to the required alignment boundary.
// padding is the number of bytes skipped to reach that aligned address.
// If padding + size won't fit, return nullptr to signal allocation failure.
// Otherwise, return the aligned address and bump offset_ past padding + size.
*/
void* Arena::allocate(size_t size, size_t align){
    size_t current = reinterpret_cast<size_t>(buffer_ + offset_);
    size_t aligned = (current + align-1) & ~(align-1);
    size_t padding = aligned - current;

    if(offset_ + padding + size > capacity_){
        return nullptr;
    }

    void* result = buffer_ + offset_ + padding;
    offset_ += padding + size;
    return result;
}


//Construct an object inside the arena:
template<typename T, typename... Args>
T* Arena::create(Args&&... args){

    void* raw = allocate(sizeof(T), alignof(T));


    //Arena is out of memory
    if(!raw){
        return nullptr;
    }

    // Construct T inside the Arena
    return new(raw) T(std::forward<Args>(args)...);
}



void Arena::reset(){
    offset_ = 0;
}

//Move Constructor:
Arena::Arena(Arena&& other) noexcept
    :buffer_(other.buffer_),
    capacity_(other.capacity_),
    offset_(other.offset_)
{
    other.buffer_ = nullptr;
    other.capacity_ = 0;
    other.offset_ = 0;
}

//Move Assignment:
Arena& Arena::operator=(Arena&& other) noexcept{

    if(this != &other){
        std::free(buffer_);

        buffer_ = other.buffer_;
        capacity_ = other.capacity_;
        offset_ = other.offset_;

        other.buffer_ = nullptr;
        other.capacity_ = 0;
        other.offset_ = 0;
    }
    
    return *this;

}


struct SmallObj{int a, b, c, d; };

int main(){

    constexpr size_t N = 1'000'000;

    auto t0 = std::chrono::steady_clock::now();
    std::vector<SmallObj*> ptrs;
    ptrs.reserve(N);
    for(size_t i =0; i < N; ++i){
        ptrs.push_back(new SmallObj{1,2,3,4});
    }
    for(auto p: ptrs){
        delete p;
    }

    auto t1 = std::chrono::steady_clock::now();

    Arena arena(N * sizeof(SmallObj) + N * alignof(SmallObj)); //headroom for padding
    auto t2 = std::chrono::steady_clock::now();
    for(size_t i = 0; i < N; ++i){
        void* raw = arena.allocate(sizeof(SmallObj), alignof(SmallObj));
        new (raw) SmallObj{1,2,3,4};
    }

    arena.reset();
    auto t3 = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> new_delete_ms = t1-t0;
    std::chrono::duration<double, std::milli> arena_ms = t3-t2;

    std::cout << "new/delete: " << new_delete_ms.count() << " ms\n";
    std::cout << "arena:      " << arena_ms.count() << " ms\n";
    std::cout << "speedup:    " << new_delete_ms.count() / arena_ms.count() << "x\n";

}

// RESULTS:
//  new/delete: 45.1368 ms
//  arena:      1.20984 ms
//  speedup:    37.3082x
