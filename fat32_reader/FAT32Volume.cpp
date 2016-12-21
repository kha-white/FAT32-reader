#include "FAT32Volume.h"


FAT32Volume::FAT32Volume(std::wstring name) {
    std::wstring volume(L"\\\\.\\");
    volume.append(name);

    hnd = CreateFile(
        volume.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );

    buffer.resize(512);

    readSector(0);
    readFromBuffer(0x0B, sectorSize);
    readFromBuffer(0x0D, sectorsPerCluster);
    readFromBuffer(0x0E, reservedSectors);
    readFromBuffer(0x24, sectorsPerFAT);
    readFromBuffer(0x2C, rootCluster);

    buffer.resize(sectorSize*sectorsPerCluster);
}


FAT32Volume::~FAT32Volume() {
    CloseHandle(hnd);
}


void FAT32Volume::readBytes(int offset, int amount, int bufferOffset) {
    DWORD bytesRead;
    SetFilePointer(hnd, offset, NULL, FILE_BEGIN);
    ReadFile(
        hnd,
        reinterpret_cast<BYTE*>(&buffer[bufferOffset]),
        amount,
        &bytesRead,
        NULL
        );
}

void FAT32Volume::readSector(int sector, int bufferOffset) {
    readBytes(sector*sectorSize, sectorSize, bufferOffset);
}

void FAT32Volume::readCluster(int cluster) {
    if (cluster >= rootCluster) {
        for (int i = 0; i < sectorsPerCluster; i++) {
            readSector(reservedSectors + 2 * sectorsPerFAT +
                (cluster - rootCluster)*sectorsPerCluster + i, i*sectorSize);
        }
    }
}

void FAT32Volume::readSectorWithFatIndex(int index) {
    readSector(reservedSectors + index * 4 / sectorSize);
}


FAT32Volume::DirectoryEntry FAT32Volume::findFile(std::vector<std::wstring> path,
    UINT32 directoryCluster, std::wstring longName, bool longNameSet) {

    readCluster(directoryCluster);

    for (int offset = 0; offset < sectorsPerCluster*sectorSize; offset += 32) {
        DirectoryEntry de;
        readFromBuffer(offset, de);

        if (de.shortFilename[0] == 0x00 || de.shortFilename[0] == 0xE5) {
            //entry empty or deleted
            longNameSet = false;
            continue;
        }

        if (de.fileAttributes == 0x0F) {
            //LFN entry
            LFNEntry lfne;
            readFromBuffer(offset, lfne);

            if (lfne.sequenceNumber & 0x40) {
                //first physical entry of LFN (last logical)
                longName.clear();
                longNameSet = false;
            }
            longName.insert(0, lfne.name);
            if ((lfne.sequenceNumber & 0x1F) == 1) {
                //last physical entry of LFN (first logical)
                longNameSet = true;
            }
            continue;
        }

        if (longNameSet) {
            de.filename = longName;
            longNameSet = false;
        }

        if (de.fileAttributes == 0x10) {
            //subdirectory
            if (de.filename.compare(path[0]) == 0) {
                if (path.size() == 1) {
                    //directory found
                    return de;
                }
                path.erase(path.begin());
                return findFile(path, de.firstCluster, L"", false);
            }
            continue;
        }

        //regular directory entry
        if (de.filename.compare(path[0]) == 0) {
            if (path.size() == 1) {
                //file found
                return de;
            }
            else {
                //wrong path
                return {};
            }
        }
    }

    //end of cluster
    readSectorWithFatIndex(directoryCluster);
    UINT32 nextCluster;
    readFromBuffer((directoryCluster * 4) % sectorSize, nextCluster);
    if (nextCluster == 0x0FFFFFFF) {
        //end of directory
        //file not found
        return {};
    }
    else {
        //continue on next cluster
        return findFile(path, nextCluster, longName, longNameSet);
    }
}

FAT32Volume::DirectoryEntry FAT32Volume::findFile(std::vector<std::wstring> path) {
    return findFile(path, rootCluster, L"", false);
}