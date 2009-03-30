#include <GnuVersion.h>

#include <ElfFile.h>

void GnuVerneed::print(){
    if (isAuxiliaryEntry()){
        printVernaux();
    } else {
        printVerneed();
    }
}

void GnuVerneed::printVernaux(){
    ASSERT(isAuxiliaryEntry() && "Cannot call this method on a non-auxiliary entry");
    PRINT_INFOR("Gnu Vernaux (%d)", index);
    PRINT_INFOR("\tHash  : %x", GET(vna_hash));
    PRINT_INFOR("\tFlags : %hx", GET(vna_flags));
    PRINT_INFOR("\tOther : %hd", GET(vna_other));
    PRINT_INFOR("\tName  : %d", GET(vna_name));
    PRINT_INFOR("\tNext  : %d", GET(vna_next));
}

void GnuVerneed::printVerneed(){
    ASSERT(!isAuxiliaryEntry() && "Cannot call this method on an auxiliary entry");
    PRINT_INFOR("Gnu Verneed (%d)", index);
    PRINT_INFOR("\tVersion   : %hd", GET(vn_version));
    PRINT_INFOR("\tCount     : %hd", GET(vn_cnt));
    PRINT_INFOR("\tFile      : %d", GET(vn_file));
    PRINT_INFOR("\tAuxiliary : %d", GET(vn_aux));
    PRINT_INFOR("\tNext      : %d", GET(vn_next));
}


uint32_t GnuVerneedTable::findVersion(uint32_t ver){
    for (uint32_t i = 0; i < numberOfVerneeds; i++){
        if (verneeds[i]->isAuxiliaryEntry() && verneeds[i]->GET(vna_other)){
            return i;
        }
    }
    return numberOfVerneeds;
}

GnuVerneedTable::GnuVerneedTable(char* rawPtr, uint32_t size, uint16_t scnIdx, ElfFile* elf)
    : RawSection(ElfClassTypes_GnuVerneedTable,rawPtr,size,scnIdx,elf)
{
    if (elfFile->is64Bit()){
        entrySize = Size__64_bit_Gnu_Verneed;
        ASSERT(entrySize == Size__64_bit_Gnu_Vernaux && "Version needs and auxiliary types must be same size");
    } else {
        entrySize = Size__32_bit_Gnu_Verneed;
        ASSERT(entrySize == Size__32_bit_Gnu_Vernaux && "Version needs and auxiliary types must be same size");
    }

    ASSERT(size % entrySize == 0 && "This size of the section should be divisible by entry size");
    numberOfVerneeds = size / entrySize;
    verneeds = new GnuVerneed*[numberOfVerneeds];
}

GnuVerneedTable::~GnuVerneedTable(){
    if (verneeds){
        for (uint32_t i = 0; i < numberOfVerneeds; i++){
            delete verneeds[i];
        }
        delete[] verneeds;
    }
}

void GnuVerneedTable::print(){

    PRINT_INFOR("Gnu Version Need Table: section %d with %d entries", sectionIndex, numberOfVerneeds);

    for (uint32_t i = 0; i < numberOfVerneeds; i++){
        verneeds[i]->print();
    }
}

void GnuVerneedTable::dump(BinaryOutputFile* binaryOutputFile, uint32_t offset){
    uint32_t currByte = 0;
    for (uint32_t i = 0; i < numberOfVerneeds; i++){
        binaryOutputFile->copyBytes(verneeds[i]->charStream(),entrySize,offset+currByte);
        currByte += entrySize;
    }

    ASSERT(currByte == sizeInBytes && "Size written to file does not match theoretical size");
}

uint32_t GnuVerneedTable::read(BinaryInputFile* binaryInputFile){
    binaryInputFile->setInPointer(getFilePointer());
    uint32_t totalBytesRead = 0;
    uint32_t remainingAux = 0;

    for (uint32_t i = 0; i < numberOfVerneeds; i++){
        if (remainingAux){
            if (elfFile->is64Bit()){
                verneeds[i] = new GnuVernaux64(i);
            } else {
                verneeds[i] = new GnuVernaux32(i);
            }
            binaryInputFile->copyBytesIterate(verneeds[i]->charStream(),entrySize);
            remainingAux--;
        } else {
            if (elfFile->is64Bit()){
                verneeds[i] = new GnuVerneed64(i);
            } else {
                verneeds[i] = new GnuVerneed32(i);
            }
            binaryInputFile->copyBytesIterate(verneeds[i]->charStream(),entrySize);
            remainingAux = verneeds[i]->GET(vn_cnt);
        }

        totalBytesRead += entrySize;
    }

    ASSERT(totalBytesRead == sizeInBytes && "Size read from file does not match theoretical size");
    return totalBytesRead;
}

uint32_t GnuVersymTable::addSymbol(uint16_t val){
    ASSERT(versyms && "symbols array should be initialized");
    uint16_t* newsyms = new uint16_t[numberOfVersyms+1];

    for (uint32_t i = 0; i < numberOfVersyms; i++){
        newsyms[i] = versyms[i];
    }
    newsyms[numberOfVersyms] = val;

    delete[] versyms;
    versyms = newsyms;
    numberOfVersyms++;

    sizeInBytes += entrySize;

    ASSERT(sizeInBytes == entrySize*numberOfVersyms && "Section size does not match data size");
}


GnuVersymTable::GnuVersymTable(char* rawPtr, uint32_t size, uint16_t scnIdx, ElfFile* elf)
    : RawSection(ElfClassTypes_GnuVersymTable,rawPtr,size,scnIdx,elf)
{
    if (elfFile->is64Bit()){
        entrySize = Size__64_bit_Gnu_Versym;
    } else {
        entrySize = Size__32_bit_Gnu_Versym;
    }

    ASSERT(entrySize == sizeof(uint16_t) && "The size of the entries is different than expected");
    ASSERT(size % entrySize == 0 && "This size of the section should be divisible by entry size");
    numberOfVersyms = size / entrySize;
    ASSERT(size == numberOfVersyms*entrySize && "Section size does not match data size");

    versyms = new uint16_t[numberOfVersyms];
}

GnuVersymTable::~GnuVersymTable(){
    if (versyms){
        delete[] versyms;
    }
}

void GnuVersymTable::print(){
    PRINT_INFOR("Gnu Version Symbol Table: section %d with %d entries", sectionIndex, numberOfVersyms);
    for (uint32_t i = 0; i < numberOfVersyms; i++){
        PRINT_INFOR("\tVersym (%d)\t: %hd", i, versyms[i]);
    }
}

void GnuVersymTable::dump(BinaryOutputFile* binaryOutputFile, uint32_t offset){
    uint32_t currByte = 0;

    for (uint32_t i = 0; i < numberOfVersyms; i++){
        binaryOutputFile->copyBytes(getFilePointer()+entrySize*i,entrySize,offset+currByte);
        currByte += entrySize;
    }
    ASSERT(currByte == sizeInBytes && "Size written to file does not match theoretical size");
}

uint32_t GnuVersymTable::read(BinaryInputFile* binaryInputFile){
    binaryInputFile->setInPointer(getFilePointer());
    uint32_t totalBytesRead = 0;

    for (uint32_t i = 0; i < numberOfVersyms; i++){
        binaryInputFile->copyBytesIterate(charStream()+entrySize*i,entrySize);
        totalBytesRead += entrySize;
    }
    ASSERT(totalBytesRead == sizeInBytes && "Size read from file does not match theoretical size");
    return totalBytesRead;
}