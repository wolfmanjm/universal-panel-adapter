/*
      This file is based on RingBuffer.h in Smoothie (http://smoothieware.org/).
      With chucks taken from http://en.wikipedia.org/wiki/Circular_buffer, see licence there also
*/

#ifndef RINGBUFFER_H
#define RINGBUFFER_H


template<class kind, char length> class RingBuffer {
    public:
        RingBuffer();
        char          size();
        char          capacity();
        void         pushBack(kind object);
        void         popFront(kind &object);
        bool         isFull();
        bool         isEmpty();
        void         clear();

    private:
        kind         buffer[length];
        volatile char tail;
        volatile char head;
};

template<class kind, char length>  char RingBuffer<kind, length>::capacity(){
    return length;
}

template<class kind, char length>  char RingBuffer<kind, length>::size(){
        char i = head - tail + ((tail > head)?length:0);
        return i;
}

template<class kind, char length> RingBuffer<kind, length>::RingBuffer(){
    this->tail = this->head = 0;
}

template<class kind, char length> void RingBuffer<kind, length>::clear(){
    this->tail = this->head = 0;
}

template<class kind, char length>  bool RingBuffer<kind, length>::isFull(){
    return  ((head+1)&(length-1)) == tail;
}

template<class kind, char length>  bool RingBuffer<kind, length>::isEmpty(){
    return  head == tail;
}

template<class kind, char length> void RingBuffer<kind, length>::pushBack(kind object){
    this->buffer[this->head] = object;
    this->head = (head+1)&(length-1);
}

template<class kind, char length> void RingBuffer<kind, length>::popFront(kind &object){
    object = this->buffer[this->tail];
    this->tail = (this->tail+1)&(length-1);
}

#endif
