#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <Windows.h>

class FAT32Volume
{
public:
    FAT32Volume(std::wstring name);
    ~FAT32Volume();

    struct DirectoryEntry {
        BYTE shortFilename[8];
        BYTE shortFileExt[3];
        BYTE fileAttributes;
        UINT32 firstCluster;
        UINT32 size;
        std::wstring filename;
    };

    struct LFNEntry {
        BYTE sequenceNumber;
        std::wstring name;
    };

    DirectoryEntry findFile(std::vector<std::wstring> path);

private:
    DirectoryEntry findFile(std::vector<std::wstring> path,
        UINT32 directoryCluster, std::wstring longName, bool longNameSet);

    void readBytes(int offset, int amount, int bufferOffset = 0);
    void readSector(int sector, int bufferOffset = 0);
    void readCluster(int cluster);

    /*reads sector with fragment of FAT table containing entry at given index*/
    void readSectorWithFatIndex(int index);


    template<typename T>
    void readFromBuffer(int offset, T& value) {
        value = *reinterpret_cast<T*>(&buffer[offset]);
    }

    template<>
    void readFromBuffer<DirectoryEntry>(int offset, DirectoryEntry& value) {
        memcpy(value.shortFilename, &buffer[offset], 8);
        memcpy(value.shortFileExt, &buffer[offset + 0x08], 3);
        value.fileAttributes = buffer[offset + 0x0B];

        UINT16 firstClusterHigh;
        UINT16 firstClusterLow;
        readFromBuffer(offset + 0x14, firstClusterHigh);
        readFromBuffer(offset + 0x1A, firstClusterLow);
        value.firstCluster = ((UINT32)firstClusterHigh << 16) | firstClusterLow;

        readFromBuffer(offset + 0x1C, value.size);

        BYTE b;
        readFromBuffer(offset + 0x0C, b);
        bool lowercaseBase = (b & 0x08);
        bool lowercaseExt = (b & 0x10);

        for (int i = 0; i < 8; i++) {
            b = value.shortFilename[i];
            if (b == ' ') break;
            if (b == 0x05) b = 0xE5;
            if (lowercaseBase) b = tolower(b);
            value.filename.push_back(b);
        }
        if (value.shortFileExt[0] != ' ') value.filename.push_back('.');
        for (int i = 0; i < 3; i++) {
            b = value.shortFileExt[i];
            if (b == ' ') break;
            if (lowercaseExt) b = tolower(b);
            value.filename.push_back(b);
        }
    }

    template<>
    void readFromBuffer<LFNEntry>(int offset, LFNEntry& value) {
        readFromBuffer(offset, value.sequenceNumber);

        wchar_t c;
        int i = 0x01;
        while (i <= 0x1E) {
            readFromBuffer(offset + i, c);
            if (c == 0) {
                break;
            }
            value.name.push_back(c);

            if (i == 0x09) {
                i = 0x0E;
            }
            else if (i == 0x18) {
                i = 0x1C;
            }
            else {
                i += 2;
            }

        }
    }


    HANDLE hnd;
    std::vector<BYTE> buffer;
    std::vector<BYTE> FAT;

    UINT16 sectorSize = 512;
    UINT8 sectorsPerCluster;
    UINT16 reservedSectors;
    UINT32 sectorsPerFAT;
    UINT32 rootCluster;
};
