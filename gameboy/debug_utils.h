//
// Created by jc on 30/09/23.
//

#ifndef DEBUG_UTILS
#define DEBUG_UTILS


#include <iostream>
#include <cstdint>
#include <map>
#include <vector>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

std::map<u8, std::string> cartridgeTypeDescr =
        {{0x00, "ROM ONLY"},
         {0x13, "MBC3+RAM+BATTERY"},
         {0x01, "MBC1"},
         {0x15, "MBC4"},
         {0x02, "MBC1+RAM"},
         {0x16, "MBC4+RAM"},
         {0x03, "MBC1+RAM+BATTERY"},
         {0x17, "MBC4+RAM+BATTERY"},
         {0x05, "MBC2"},
         {0x19, "MBC5"},
         {0x06, "MBC2+BATTERY"},
         {0x1A, "MBC5+RAM"},
         {0x08, "ROM+RAM"},
         {0x1B, "MBC5+RAM+BATTERY"},
         {0x09, "ROM+RAM+BATTERY"},
         {0x1C, "MBC5+RUMBLE"},
         {0x0B, "MMM01"},
         {0x1D, "MBC5+RUMBLE+RAM"},
         {0xC,  "MMM01+RAM"},
         {0x1E, "MBC5+RUMBLE+RAM+BATTERY"},
         {0x0D, "MMM01+RAM+BATTERY"},
         {0xFC, "POCKET CAMERA"},
         {0x0F, "MBC3+TIMER+BATTERY"},
         {0xFD, "BANDAI TAMA5"},
         {0x10, "MBC3+TIMER+RAM+BATTERY"},
         {0xFE, "HuC3"},
         {0x11, "MBC3"},
         {0xFF, "HuC1+RAM+BATTERY"},
         {0x12, "MBC3+RAM"}};


struct CartridgeHeader {
    u8 entryPoint[3];
    u8 nintendoLogo[48];
    char title[0x143 - 0x134 + 1];
    char licenseCode[0x145 - 0x144 + 1];
    u8 sgbFlag[1];
    u8 cartridgeTyp;
    u8 sizeType;
    u8 ramSizeType;
    u8 destCode;
    u8 oldLicenseCode;
    u8 maskROMVersionNo;
    u8 headerChecksum;
    u16 globalChecksum;
};

std::map<u8, std::string> romSizeDescr = {
        {0x00, " 32KByte (no ROM banking)"},
        {0x01, " 64KByte (4 banks)"},
        {0x02, "28KByte (8 banks)"},
        {0x03, "56KByte (16 banks)"},
        {0x04, "12KByte (32 banks)"},
        {0x05, "  1MByte (64 banks)  - only 63 banks used by MBC1"},
        {0x06, "  2MByte (128 banks) - only 125 banks used by MBC1"},
        {0x07, "  4MByte (256 banks)"},
        {0x52, ".1MByte (72 banks)"},
        {0x53, ".2MByte (80 banks)"},
        {0x54, ".5MByte (96 banks)"}
};

std::map<u8, std::string> ramSizeDescr = {
        {0x00, "None"},
        {0x01, "2 KBytes"},
        {0x02, "8 Kbytes"},
        {0x03, "32 Kbytes"}
};


void dumpCartridgeHeader(std::vector<u8> &ram) {

    auto header = *reinterpret_cast<CartridgeHeader *>(&ram[0x100]);
    printf("entryPoint: %x%x%x", header.entryPoint[0], header.entryPoint[1], header.entryPoint[2]);
    std::cout << std::endl;
    std::cout << "nintendoLogo: " << header.nintendoLogo << std::endl;
    std::cout << "title: " << header.title << std::endl;
    std::cout << "licenseCode: " << header.licenseCode << std::endl;
    std::cout << "sgbFlag: " << header.sgbFlag << std::endl;
    std::cout << "cartridgeTyp: " << cartridgeTypeDescr[header.cartridgeTyp] << std::endl;
    std::cout << "sizeType: " << romSizeDescr[header.sizeType] << std::endl;
    std::cout << "ramSizeType: " << ramSizeDescr[header.ramSizeType] << std::endl;
    std::cout << "destCode: " << header.destCode << std::endl;
    std::cout << "oldLicenseCode: " << header.oldLicenseCode << std::endl;
    std::cout << "maskROMVersionNo: " << header.maskROMVersionNo << std::endl;
    std::cout << "headerChecksum: " << header.headerChecksum << std::endl;
    std::cout << "globalChecksum: " << header.globalChecksum << std::endl;
}


#endif