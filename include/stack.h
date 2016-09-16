#ifndef STACK_H
#define STACK_H

#include "stdint.h"
#include "stdio.h"

#define INITIAL 1000

/*
 * Container for passing elements between stages
 * Minimal implementation to optimize for performance
 */
template <class T> 
class Stack {

protected:
    uint32_t top;
    T *stack;
    uint32_t max_size;

public:
    Stack();
    ~Stack();
    void push_back(T const &);
    void pop_back();
    T back() const;
    uint32_t size();
    bool empty() const {
        return (top == 0);
    }
};

/*
 *
 */
template <class T>
Stack<T>::Stack() {

    top = 0;
    max_size = INITIAL;
    stack = new T[INITIAL];

}

/*
 *
 */
template <class T>
Stack<T>::~Stack() {


}

template <class T>
inline void Stack<T>::push_back(T const& el) {

    stack[top] = el;
    top++;

    //Double size of stack if necessary
    if(top == max_size){

        //printf("RESIZING!\n");

        T *stack_temp = new T[max_size*2];
        // copy contents
        for(uint32_t i = 0; i < max_size; i++){
            stack_temp[i] = stack[i];
        }

        max_size *=2;
        delete stack;
        stack = stack_temp;
    }
}

template <class T>
inline void Stack<T>::pop_back(){
    top--;
}

template <class T>
inline T Stack<T>::back() const{
    return stack[top - 1];
}

template <class T>
inline uint32_t Stack<T>::size() {
    return top;
}

#endif
