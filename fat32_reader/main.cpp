#include <algorithm>
#include <regex>
#include "FAT32Volume.h"

std::vector<std::wstring> split(const std::wstring& str, wchar_t delim) {
    std::vector<std::wstring> items;
    std::wstringstream ss(str);
    std::wstring item;
    while (std::getline(ss, item, delim)) {
        items.push_back(item);
    }
    return items;
}


int main() {
    while (true) {
        std::wstring input;
        char c;
        std::cout << "type full path to file:\n";
        std::getline(std::wcin, input);
        std::replace(input.begin(), input.end(), '\\', '/');

        std::wregex reg(L"[a-zA-Z]:(/[^/]+)+/?");
        if (!std::regex_match(input, reg)) {
            std::cout << "wrong path\n";
            continue;
        }

        auto path = split(input, '/');

        FAT32Volume volume(path[0]);
        path.erase(path.begin());
        auto de = volume.findFile(path);
        if (de.filename.empty()) {
            std::cout << "file not found\n";
        }
        else {
            std::cout << "\n";
            std::wcout << de.filename << "\n";

            std::cout << "attributes:\t";
            if (de.fileAttributes & 0x01) std::cout << "R";
            if (de.fileAttributes & 0x20) std::cout << "A";
            if (de.fileAttributes & 0x04) std::cout << "S";
            if (de.fileAttributes & 0x02) std::cout << "H";
            std::cout << "\n";

            std::cout << "created:\t" << std::put_time(&de.created, "%Y-%m-%d %H:%M:%S") << "\n";
            std::cout << "last modified:\t" << std::put_time(&de.lastModified, "%Y-%m-%d %H:%M:%S") << "\n";
            auto sectors = volume.clusterToSectors(de.firstCluster);
            std::cout << "first cluster:\t" << de.firstCluster <<
                " (sectors " << sectors.first << "-" << sectors.second << ")\n";
            std::cout << "size:\t\t" << de.size << " B\n";

            std::cout << "\nprint list of clusters containing file? (y/n)\n";
            std::cin >> c;
            if (c == 'y') {
                volume.printListOfClusters(de);
            }
            std::cout << "\nprint file data? (y/n)\n";
            std::cin >> c;
            if (c == 'y') {
                volume.printFile(de);
            }
        }
        std::cout << "\nchoose another file? (y/n)\n";
        std::cin >> c;
        if (c != 'y') {
            break;
        }
        std::cout << "\n";
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
    }

    return 0;
}
