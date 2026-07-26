#include <iostream>

template <typename T>
class UniquePtr{
public:
    explicit UniquePtr(T* ptr = nullptr);
    ~UniquePtr();

    // copy operations deleted, bcz ownership is unique, copy violates that.
    UniquePtr(const UniquePtr& other) = delete;
    UniquePtr& operator=(const UniquePtr& other) = delete;


    // move operations, bcz ownership can transfer.
    UniquePtr(UniquePtr&& other) noexcept;
    UniquePtr& operator=(UniquePtr&& other) noexcept;

    T& operator*() const;
    T* operator->() const;
    T* get() const;
    T* release();
    void reset(T* new_ptr = nullptr);


private:
    T* ptr_;

};

template <typename T>
UniquePtr<T>::UniquePtr(T* ptr) : ptr_(ptr){

}

template <typename T>
UniquePtr<T>::~UniquePtr(){
    delete ptr_;
}

template <typename T>
UniquePtr<T>::UniquePtr(UniquePtr&& other) noexcept : ptr_(other.ptr_){
    other.ptr_ = nullptr;
}

template <typename T>
UniquePtr<T>& UniquePtr<T>::operator=(UniquePtr&& other) noexcept{
    if(this != &other){
        delete ptr_;
        ptr_ = other.ptr_;
        other.ptr_ = nullptr;
    }
    return *this;
}

template <typename T>
T& UniquePtr<T>::operator*() const{
    return *ptr_;
}

template <typename T>
T* UniquePtr<T>::operator->() const{
    return ptr_;
}

template <typename T>
T* UniquePtr<T>::get() const{
    return ptr_;
}

template <typename T>
T* UniquePtr<T>::release(){
    T* old_ptr = ptr_;
    ptr_ = nullptr;
    return old_ptr;
}

template <typename T>
void UniquePtr<T>::reset(T* new_ptr){
    T* old_ptr = ptr_;
    ptr_ = new_ptr;
    delete old_ptr;
}

//make_unique<Node>(5) calls new Node(5) and wraps it, 
//so you never write a naked new at the call site.
template <typename T, typename... Args>
UniquePtr<T> make_unique(Args&&... args){
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
struct Node{
    T value;
    UniquePtr<Node> next;
    Node(T v) : value(v), next(nullptr){}
};

int main(){
    UniquePtr<Node<int>> head = make_unique<Node<int>>(1);
    head->next = make_unique<Node<int>>(2);
    head->next->next = make_unique<Node<int>>(3);

    //UniquePtr<Node<int>> copy = head; triggers delete function. good!, cannot copy.

    Node<int>* cur = head.get();
    while(cur != nullptr){
        std::cout << cur->value << " ";
        cur = cur->next.get();
    }

    std::cout<< '\n';
    return 0;
}