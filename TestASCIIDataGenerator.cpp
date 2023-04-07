#include <iostream>
#include <fstream>

int main(int argc, char* argv[])
{
  auto fileCount = std::atoi(argv[1]);
  auto byesCount = std::atoi(argv[2]);
  for (auto i = 0; i < fileCount; ++i)
  {
    auto file = std::ofstream(std::string("./TestData/") + std::to_string(i) + ".txt");
    auto testSymbolSet = std::string("abcdefghijklmnopqrstuvwxyz123456789 _!@#$%^&*()~\n");
    for(auto j = 0; j < byesCount; ++j)
    {
      file << testSymbolSet[std::rand() % testSymbolSet.size()];
    }
  }
  return 0;
}
