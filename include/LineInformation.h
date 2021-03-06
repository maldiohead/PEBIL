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

#ifndef _LineInformation_h_
#define _LineInformation_h_

#include <Base.h>
#include <CStructuresDwarf.h>
#include <Vector.h>
#include <defines/LineInformation.d>

class BasicBlock;
class DwarfLineInfoSection;
class Function;
class LineInfoTable;

static char* currentDirectory = ".";
   

class LineInfo : public Base {
private:

protected:
    DWARF4_LineInfo_Registers entry;

    Vector<uint8_t> instructionBytes;
    LineInfoTable* header;

    uint64_t addressSpan;
public:
    LineInfo(LineInfoTable* header);
    LineInfo(const LineInfo& other);

    //LineInfo(uint32_t idx, char* instruction, LineInfoTable* hdr);
    ~LineInfo();

    void initializeWithDefaults();

    LINEINFO_MACROS_CLASS("For the get_X/set_X field macros check the defines directory");    

    void print();
    char* charStream() const { return (char*)&entry; }
    //uint32_t getInstructionSize() { return instructionBytes.size(); }

    char* getFileName();
    char* getFilePath();

    bool verify();
    LineInfoTable* getHeader() { return header; }
    uint64_t getAddressSpan() { return addressSpan; }
    void setAddressSpan(uint64_t spn) { addressSpan = spn; }

    //uint32_t getIndex() { return index; }
    const char* briefName() { return "LineInfo"; }
};

class LineInfoTable : public Base {
private:
    uint32_t executeInstruction(char* instruction);
    uint32_t executeSpecialOpcode(char* instruction);
    uint32_t executeStandardOpcode(char* instruction);
    uint32_t executeExtendedOpcode(char* instruction);
    void appendRowToMatrix();

protected:
    uint32_t index;
    char* rawDataPtr;

    DWARF4_Internal_LineInfo entry;

    Vector<LineInfo*> lineInformations;
    Vector<uint8_t> opcodes;
    Vector<char*> includePaths;
    Vector<DWARF4_FileName> fileNames;

    DebugFormats format;
    
    LineInfo* registers;
    DwarfLineInfoSection* dwarfLineInfoSection;

public: 
    LineInfoTable(uint32_t idx, char* raw, DwarfLineInfoSection* dwarf);
    ~LineInfoTable();

    void print();
    uint32_t read(BinaryInputFile* b);
    void dump(BinaryOutputFile* b, uint32_t offset);
    bool verify();

    LINEINFOTABLE_MACROS_CLASS("For the get_X/set_X field macros check the defines directory");

    const char* breifName() { return "LineInfoTable"; }

    LineInfo* getRegisters() { return registers; }
    uint32_t getOpcodeLength(uint32_t idx);

    uint32_t getNumberOfLineInfos() { return lineInformations.size(); }
    LineInfo* getLineInfo(uint32_t idx) { return lineInformations[idx]; }


    void addFileName(DWARF4_FileName file);
    char* getFileName(uint32_t idx);
    char* getIncludePath(uint32_t idx);

    uint32_t getAddressSize();
    void wedge(uint32_t shamt);
};

class LineInfoFinder {
protected:
    Vector<LineInfo*> sortedLineInfos;
    DwarfLineInfoSection* dwarfLineInfoSection;
public:
    LineInfoFinder(DwarfLineInfoSection* dwarfLineSection);
    ~LineInfoFinder();

    LineInfo* lookupLineInfo(Function* f);
    LineInfo* lookupLineInfo(BasicBlock* bb);
    LineInfo* lookupLineInfo(X86Instruction* ins);
    LineInfo* lookupLineInfo(uint64_t addr);

    bool verify();
};


#endif /* _LineInformation_h_ */

