/*BasePVByteArray.h*/
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvDataCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
#ifndef BASEPVBYTEARRAY_H
#define BASEPVBYTEARRAY_H
#include <cstddef>
#include <cstdlib>
#include <string>
#include <cstdio>
#include "pvData.h"
#include "factory.h"
#include "AbstractPVScalarArray.h"
#include "serializeHelper.h"

using std::min;

namespace epics { namespace pvData {

    PVByteArray::~PVByteArray() {}

    PVByteArray::PVByteArray(PVStructure *parent,ScalarArrayConstPtr scalar)
    : PVScalarArray(parent,scalar) {}

    class BasePVByteArray : public PVByteArray {
    public:
        BasePVByteArray(PVStructure *parent,ScalarArrayConstPtr scalarArray);
        virtual ~BasePVByteArray();
        virtual void setCapacity(int capacity);
        virtual int get(int offset, int length, ByteArrayData *data) ;
        virtual int put(int offset,int length,ByteArray from,
           int fromOffset);
        virtual void shareData(ByteArray value,int capacity,int length);
        // from Serializable
        virtual void serialize(ByteBuffer *pbuffer,SerializableControl *pflusher) ;
        virtual void deserialize(ByteBuffer *pbuffer,DeserializableControl *pflusher);
        virtual void serialize(ByteBuffer *pbuffer,
             SerializableControl *pflusher, int offset, int count) ;
        virtual bool operator==(PVField& pv) ;
        virtual bool operator!=(PVField& pv) ;
    private:
        int8 *value;
    };

    BasePVByteArray::BasePVByteArray(PVStructure *parent,
        ScalarArrayConstPtr scalarArray)
    : PVByteArray(parent,scalarArray),value(new int8[0])
    { } 

    BasePVByteArray::~BasePVByteArray()
    {
        delete[] value;
    }

    void BasePVByteArray::setCapacity(int capacity)
    {
        if(PVArray::getCapacity()==capacity) return;
        if(!PVArray::isCapacityMutable()) {
            std::string message("not capacityMutable");
            PVField::message(message, errorMessage);
            return;
        }
        int length = PVArray::getLength();
        if(length>capacity) length = capacity;
        int8 *newValue = new int8[capacity]; 
        for(int i=0; i<length; i++) newValue[i] = value[i];
        delete[]value;
        value = newValue;
        PVArray::setCapacityLength(capacity,length);
    }

    int BasePVByteArray::get(int offset, int len, ByteArrayData *data)
    {
        int n = len;
        int length = PVArray::getLength();
        if(offset+len > length) {
            n = length-offset;
            if(n<0) n = 0;
        }
        data->data = value;
        data->offset = offset;
        return n;
    }

    int BasePVByteArray::put(int offset,int len,
        ByteArray from,int fromOffset)
    {
        if(PVField::isImmutable()) {
            PVField::message("field is immutable",errorMessage);
            return 0;
        }
        if(from==value) return len;
        if(len<1) return 0;
        int length = PVArray::getLength();
        int capacity = PVArray::getCapacity();
        if(offset+len > length) {
            int newlength = offset + len;
            if(newlength>capacity) {
                setCapacity(newlength);
                newlength = PVArray::getCapacity();
                len = newlength - offset;
                if(len<=0) return 0;
            }
            length = newlength;
        }
        for(int i=0;i<len;i++) {
           value[i+offset] = from[i+fromOffset];
        }
        PVArray::setLength(length);
        PVField::postPut();
        return len;
    }

    void BasePVByteArray::shareData(ByteArray shareValue,int capacity,int length)
    {
        delete[] value;
        value = shareValue;
        PVArray::setCapacityLength(capacity,length);
    }

    void BasePVByteArray::serialize(ByteBuffer *pbuffer,
                SerializableControl *pflusher) {
        serialize(pbuffer, pflusher, 0, getLength());
    }

    void BasePVByteArray::deserialize(ByteBuffer *pbuffer,
            DeserializableControl *pcontrol) {
        int size = SerializeHelper::readSize(pbuffer, pcontrol);
        if(size>=0) {
            // prepare array, if necessary
            if(size>getCapacity()) setCapacity(size);
            // retrieve value from the buffer
            int i = 0;
            while(true) {
                int toRead = min(size-i, pbuffer->getRemaining());
                pbuffer->get((char *)value, i, toRead);
                i += toRead;
                if(i<size)
                    pcontrol->ensureData(1); // TODO: is there a better way to ensureData?
                else
                    break;
            }
            // set new length
            setLength(size);
            postPut();
        }
        // TODO null arrays (size == -1) not supported
    }

    void BasePVByteArray::serialize(ByteBuffer *pbuffer,
            SerializableControl *pflusher, int offset, int count) {
        // cache
        int length = getLength();

        // check bounds
        if(offset<0)
            offset = 0;
        else if(offset>length) offset = length;
        if(count<0) count = length;

        int maxCount = length-offset;
        if(count>maxCount) count = maxCount;

        // write
        SerializeHelper::writeSize(count, pbuffer, pflusher);
        int end = offset+count;
        int i = offset;
        while(true) {
            int maxIndex = min(end-i, pbuffer->getRemaining())+i;
            for(; i<maxIndex; i++)
                pbuffer->putByte(value[i]);
            if(i<end)
                pflusher->flushSerializeBuffer();
            else
                break;
        }
    }

    bool BasePVByteArray::operator==(PVField& pv)
    {
        return getConvert()->equals(this, &pv);
    }

    bool BasePVByteArray::operator!=(PVField& pv)
    {
        return !(getConvert()->equals(this, &pv));
    }
}}
#endif  /* BASEPVBYTEARRAY_H */