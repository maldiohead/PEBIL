/* 
 * This file is part of the pebil project.
 * 
 * Copyright (c) 2010, University of California Regents
 * All rights reserved.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _BitSet_h_
#define _BitSet_h_

#include <Base.h>

template <class T=uint32_t>
class BitSet {
private:
    const static uint8_t DivideLog = 3;
    const static uint32_t ModMask = ~((uint32_t)0xffffffff << DivideLog);

    uint32_t maximum;
    uint8_t* bits;
    uint32_t cardinality;
    T* elements;

    uint32_t internalCount(){
        return (maximum >> DivideLog) + (maximum & ModMask ? 1 : 0);
    }

public:
    BitSet(uint32_t maxVal,T* arr=NULL) : maximum(maxVal),cardinality(0),elements(arr) {
        uint32_t count = internalCount();
        bits = new uint8_t[count];
        bzero(bits,count);
    }

    //! copy constructor
    BitSet(BitSet& src){
        maximum = src.maximum;
        uint32_t count = src.internalCount();
        bits = new uint8_t[count];
        memcpy(bits,src.bits,count);
        cardinality = src.cardinality;
        elements = src.elements;
    }

    ~BitSet() { delete[] bits; }

    BitSet& operator-=(BitSet& src){
        ASSERT((maximum == src.maximum) && "FATAL: Two sets with different max numbers are SUBed");

        uint32_t count = internalCount();
        for(uint32_t i=0;i<count;i++){
            bits[i] = bits[i] & ~(src.bits[i]);
            if (src.contains(i)){
                remove(i);
                cardinality--;
            }
        }        
        for(uint32_t i=0;i<count;i++){
            if (src.bits[i] & bits[i]){
                PRINT_ERROR("bits dont match");
            }
        }
        return *this;
    }

    bool isSubsetOf(BitSet& src){
        ASSERT((maximum == src.maximum) && "FATAL: Two sets with different max numbers are subsetted");
        uint32_t count = internalCount();
        for (uint32_t i = 0; i < count; i++){
            if (bits[i] & ~(src.bits[i])){
                return false;
            }
        }
        return true;
    }

    BitSet& operator&=(BitSet& src){
        ASSERT((maximum == src.maximum) && "FATAL: Two sets with different max numbers are ANDed");

        uint32_t count = internalCount();
        for(uint32_t i=0;i<count;i++){
            bits[i] &= src.bits[i];
        }
        cardinality = 0;
        for(uint32_t i=0;i<maximum;i++){
            if(contains(i)){
                cardinality++;
            }
        }
        return *this;
    }

    bool operator==(BitSet& src){
        ASSERT(maximum == src.maximum && "FATAL: Two sets with different max numbers are EQed");
        
        uint32_t count = internalCount();
        for(uint32_t i=0;i<count;i++){
            if (bits[i] != src.bits[i]){
                return false;
            }
        }
        return true;
    }

    BitSet& operator|=(BitSet& src){

        ASSERT((maximum == src.maximum) && "FATAL: Two sets with different max numbers are ORed");

        uint32_t count = internalCount();
        for(uint32_t i=0;i<count;i++){
            bits[i] |= src.bits[i];
        }
        cardinality = 0;
        for(uint32_t i=0;i<maximum;i++){
            if(contains(i)){
                cardinality++;
            }
        }
        return *this;
    }

    BitSet& operator=(BitSet& src){
        ASSERT((maximum == src.maximum) && "FATAL: Two sets with different max numbers are CPed");

        uint32_t count = internalCount();
        for(uint32_t i=0;i<count;i++){
            bits[i] = src.bits[i];
        }
        cardinality = src.cardinality;
        return *this;
    }

    //! Complement operator
    /*!
      Complements the set.
      \return The complement of the operand. 
    */
    BitSet& operator~(){
        uint32_t count = internalCount();
        for (uint32_t i = 0; i < count; i++){
            bits[i] = ~bits[i];
        }
        cardinality = maximum-cardinality;
        return *this;
    }

    inline void clear() {
        uint32_t count = internalCount();
        bzero(bits,count);
        cardinality = 0;
    }

    inline void setall() {
        uint32_t count = internalCount();
        memset(bits,0xff,count);
        cardinality = maximum;
    }

    void print() {
        uint32_t count = internalCount();
        fprintf(stdout, "[BitSet] %d bits: ", maximum);
        for (uint32_t i = 0; i < count; i++){
            fprintf(stdout, "%02x", bits[i]);
        }
        fprintf(stdout, "\n");
    }

    inline void insert(uint32_t n){
        if(n >= maximum)
            return;
        uint32_t index = n >> DivideLog;
        uint32_t mask = 1 << (n & ModMask);
        if(!(bits[index] & mask))
            cardinality++;
        bits[index] |= mask;
    }

    inline void remove(uint32_t n){
        if(n >= maximum)
            return;
        uint32_t index = n >> DivideLog;
        uint32_t mask = 1 << (n & ModMask);
        if(bits[index] & mask)
            cardinality--;
        bits[index] &= ~mask;
    }

    inline bool contains(uint32_t n){
        uint32_t index = n >> DivideLog;
        uint32_t mask = 1 << (n & ModMask);
        if(n >= maximum)
            return false;
        return (bits[index] & mask);
    }

    inline uint32_t size() { return cardinality; }
    inline bool empty() { return !cardinality; }
    
    //! generates an array of elements which are currently included in the set
    //   NOTE: this is not the same as the 'elements' member
    //variable 
    // \return an array of elements included in the set - the
    //size of the elements is the same as the return value of size().
    T* duplicateMembers(){
        if(!size())
            return NULL;
        
        T* ret = new T[size()];
        
        uint32_t arrIdx = 0;
        uint32_t idx = 0;
        for(uint32_t i=0;idx<maximum;i++){
            uint8_t value = bits[i];
            for(uint32_t j=0;(j<(1 << DivideLog)) && (idx<maximum);j++,idx++){
                uint8_t mask = 1 << j;
                if(value & mask){
                    ret[arrIdx++] = elements[idx];
                }
            }
        }
        
        ASSERT(size() == arrIdx);
        
        return ret;
    }
};

#endif
