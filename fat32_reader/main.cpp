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
    std::wstring path = L"cstv\\test\\cotam\\8845.txt";
    //std::getline(std::cin, path);

    auto items = split(path, '\\');

    FAT32Volume volume(L"J:");
    auto out = volume.findFile(items);

    return 0;
}
