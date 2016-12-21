#pragma once
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <ctime>
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
        tm created;
        tm lastModified;
        UINT32 firstCluster;
        UINT32 size;
        std::wstring filename;
    };


    std::pair<int, int> clusterToSectors(int cluster);
    DirectoryEntry findFile(std::vector<std::wstring> path);
    void printListOfClusters(DirectoryEntry de);
    void printFile(DirectoryEntry de);

private:
    struct LFNEntry {
        BYTE sequenceNumber;
        std::wstring name;
    };

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


        UINT16 ui16;
        readFromBuffer(offset + 0x0E, ui16);
        value.created.tm_hour = (ui16 & 0xF800) >> 11;
        value.created.tm_min = (ui16 & 0x07E0) >> 5;
        value.created.tm_sec = (ui16 & 0x1F) << 1;
        readFromBuffer(offset + 0x10, ui16);
        value.created.tm_year = ((ui16 & 0xFE00) >> 9) + 80;
        value.created.tm_mon = ((ui16 & 0x01E0) >> 5) - 1;
        value.created.tm_mday = (ui16 & 0x1F);
        value.created.tm_wday = 0;
        value.created.tm_isdst = 0;
        value.created.tm_yday = 0;

        readFromBuffer(offset + 0x16, ui16);
        value.lastModified.tm_hour = (ui16 & 0xF800) >> 11;
        value.lastModified.tm_min = (ui16 & 0x07E0) >> 5;
        value.lastModified.tm_sec = (ui16 & 0x1F) << 1;
        readFromBuffer(offset + 0x18, ui16);
        value.lastModified.tm_year = ((ui16 & 0xFE00) >> 9) + 80;
        value.lastModified.tm_mon = ((ui16 & 0x01E0) >> 5) - 1;
        value.lastModified.tm_mday = (ui16 & 0x1F);
        value.lastModified.tm_wday = 0;
        value.lastModified.tm_isdst = 0;
        value.lastModified.tm_yday = 0;

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
