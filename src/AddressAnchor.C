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

#include <AddressAnchor.h>

#include <BinaryFile.h>
#include <X86Instruction.h>
#include <RawSection.h>

uint64_t AddressAnchor::getLinkOffset(){
    ASSERT(linkedParent);
    ASSERT(link);
    switch (linkClass){
    case LinkClass_InstructionRelative:
        return link->getBaseAddress() - linkedParent->getBaseAddress() - linkedParent->getSizeInBytes();
        break;
    case LinkClass_InstructionImmediate:
    case LinkClass_DataReference:
        return link->getBaseAddress();
        break;
    default:
        __SHOULD_NOT_ARRIVE;
    }
    __SHOULD_NOT_ARRIVE;
    return 0;
}

int searchLinkBaseAddressExact(const void* arg1, const void* arg2){
    uint64_t key = *((uint64_t*)arg1);
    AddressAnchor* a = *((AddressAnchor**)arg2);

    ASSERT(a && "AddressAnchor should exist");

    uint64_t val = a->linkBaseAddress;

    if (key < val)
        return -1;
    if (key > val)
        return 1;
    return 0;
}

int searchLinkBaseAddress(const void* arg1, const void* arg2){
    uint64_t key = *((uint64_t*)arg1);
    AddressAnchor* a = *((AddressAnchor**)arg2);

    ASSERT(a && "AddressAnchor should exist");
    uint64_t val = a->linkBaseAddress;

    PRINT_DEBUG_ANCHOR("searching for key %llx ~~ [%#llx,%#llx) in anchors", key, val, val + a->getLink()->getSizeInBytes());

    if (key < val)
        return -1;
    if (key >= val + a->getLink()->getSizeInBytes())
        return 1;
    return 0;
}


int compareLinkBaseAddress(const void* arg1, const void* arg2){
    AddressAnchor* a1 = *((AddressAnchor**)arg1);
    AddressAnchor* a2 = *((AddressAnchor**)arg2);

    return a1->linkBaseAddress - a2->linkBaseAddress;
    if(a1->linkBaseAddress < a2->linkBaseAddress)
        return -1;
    if(a1->linkBaseAddress > a2->linkBaseAddress)
        return 1;
    return 0;
}

void AddressAnchor::refreshCache(){
    linkBaseAddress = link->getBaseAddress();
}


Base* AddressAnchor::updateLink(Base* newLink){
    ASSERT(newLink->containsProgramBits());
    Base* oldLink = link;
    PRINT_DEBUG_ANCHOR("updating link: %#llx -> %#llx", linkBaseAddress, linkedParent->getBaseAddress());
    link = newLink;

    refreshCache();
    verify();

    return oldLink;
}

void AddressAnchor::dump(BinaryOutputFile* binaryOutputFile, uint32_t offset){
    if (linkClass == LinkClass_DataReference){
        dumpDataReference(binaryOutputFile, offset);
    } else {
        dumpInstruction(binaryOutputFile, offset);
    }
}

uint64_t AddressAnchor::getLinkValue(){
    return getLinkOffset();
}

void AddressAnchor::dump8(BinaryOutputFile* binaryOutputFile, uint32_t offset, uint8_t value){
    ASSERT((uint8_t)(getLinkValue() - value) == 0 && "Need more than 8 bits for relative immediate");
    binaryOutputFile->copyBytes((char*)&value,sizeof(uint8_t),offset);
}

void AddressAnchor::dump16(BinaryOutputFile* binaryOutputFile, uint32_t offset, uint16_t value){
    ASSERT((uint16_t)(getLinkValue() - value) == 0 && "Need more than 16 bits for relative immediate");
    binaryOutputFile->copyBytes((char*)&value,sizeof(uint16_t),offset);
}

void AddressAnchor::dump32(BinaryOutputFile* binaryOutputFile, uint32_t offset, uint32_t value){
    ASSERT((uint32_t)(getLinkValue() - value) == 0 && "Need more than 32 bits for relative immediate");
    binaryOutputFile->copyBytes((char*)&value,sizeof(uint32_t),offset);
}

void AddressAnchor::dump64(BinaryOutputFile* binaryOutputFile, uint32_t offset, uint64_t value){
    ASSERT((uint64_t)(getLinkValue() - value) == 0 && "Need more than 64 bits for relative immediate");
    binaryOutputFile->copyBytes((char*)&value,sizeof(uint64_t),offset);
}

void AddressAnchor::dumpDataReference(BinaryOutputFile* binaryOutputFile, uint32_t offset){
    ASSERT(linkClass == LinkClass_DataReference);
    ASSERT(linkedParent->getType() == PebilClassType_DataReference);
    DataReference* dataReference = (DataReference*)linkedParent;
    if (dataReference->is64Bit()){
        uint64_t value = getLinkValue();
        dump64(binaryOutputFile, offset + dataReference->getSectionOffset(), value);
    } else {
        uint32_t value = (uint32_t)getLinkValue();
        dump32(binaryOutputFile, offset + dataReference->getSectionOffset(), value);
    }
}

void AddressAnchor::dumpInstruction(BinaryOutputFile* binaryOutputFile, uint32_t offset){
    ASSERT(linkedParent);
    ASSERT(linkedParent->getType() == PebilClassType_X86Instruction);
    ASSERT(linkClass == LinkClass_InstructionRelative || linkClass == LinkClass_InstructionImmediate);

    X86Instruction* linkedInstruction = (X86Instruction*)linkedParent;

    for (uint32_t i = 0; i < MAX_OPERANDS; i++){
        OperandX86* op = linkedInstruction->getOperand(i);
        if (op){
            bool overwrite = false;
            if (op->isRelative() && linkClass == LinkClass_InstructionRelative){
                overwrite = true;
            }
            if (op->GET(type) == UD_OP_IMM && linkClass == LinkClass_InstructionImmediate){
                overwrite = true;
            }

            if (overwrite){
                if (op->getBytesUsed() == sizeof(uint8_t)){
                    uint8_t value = (uint8_t)getLinkValue();
                    dump8(binaryOutputFile, offset + op->getBytePosition(), value);
                } else if (op->getBytesUsed() == sizeof(uint16_t)){
                    uint16_t value = (uint16_t)getLinkValue();
                    dump16(binaryOutputFile, offset + op->getBytePosition(), value);
                } else if (op->getBytesUsed() == sizeof(uint32_t)){
                    uint32_t value = (uint32_t)getLinkValue();
                    dump32(binaryOutputFile, offset + op->getBytePosition(), value);
                } else if (op->getBytesUsed() == sizeof(uint64_t)){
                    uint64_t value = (uint64_t)getLinkValue();
                    dump64(binaryOutputFile, offset + op->getBytePosition(), value);
                } else {
                    print();
                    PRINT_ERROR("an operand cannot use %d bytes", op->getBytesUsed());
                    __SHOULD_NOT_ARRIVE;
                }
            }
        }
    }
}

AddressAnchor::AddressAnchor(Base* lnk, Base* par, bool imm){
    link = lnk;
    linkedParent = par;
    
    linkBaseAddress = link->getBaseAddress();
    if (linkedParent->getType() == PebilClassType_DataReference){
        linkClass = LinkClass_DataReference;
    } else {
        ASSERT(linkedParent->getType() == PebilClassType_X86Instruction);
        if (imm){
            linkClass = LinkClass_InstructionImmediate;
        } else {
            linkClass = LinkClass_InstructionRelative;
        }
    }

    verify();    
}

AddressAnchor::AddressAnchor(Base* lnk, Base* par){
    link = lnk;
    linkedParent = par;
    
    linkBaseAddress = link->getBaseAddress();
    if (linkedParent->getType() == PebilClassType_DataReference){
        linkClass = LinkClass_DataReference;
    } else {
        ASSERT(linkedParent->getType() == PebilClassType_X86Instruction);
        linkClass = LinkClass_InstructionRelative;
    }

    verify();
}

AddressAnchor::~AddressAnchor(){
}

bool AddressAnchor::verify(){
    if (!link->containsProgramBits()){
        PRINT_ERROR("Address link not allowed to be type %d", link->getType());
        return false;
    }
    if (!linkedParent->containsProgramBits()){
        PRINT_ERROR("Address link base not allowed to be type %d", linkedParent->getType());
        return false;
    }

    if (link->getType() == PebilClassType_X86Instruction){
    } else if (link->getType() == PebilClassType_DataReference){
    } else {
        PRINT_ERROR("Address link cannot have type %d", link->getType());
        return false;
    }

    if (linkBaseAddress != link->getBaseAddress()){
        PRINT_ERROR("Link base address %#lx cached does not match actual value %#llx", linkBaseAddress, link->getBaseAddress());
        return false;
    }

    if (linkClass <= LinkClass_Undefined || linkClass >= LinkClass_TotalTypes){
        PRINT_ERROR("Invalid link class");
        return false;
    }

    return true;
}

void AddressAnchor::print(){
    PRINT_INFOR("AnchorRef: addr %#llx, link offset %#llx, link value %#llx", linkBaseAddress, getLinkOffset(), getLinkValue());
    if (linkedParent){
        linkedParent->print();
    }
    if (link){
        link->print();
    }
}
