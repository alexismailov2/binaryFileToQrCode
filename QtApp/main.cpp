#include <QApplication>

#include "BinaryFileToQrCodeWindow.h"

int main(int argc, char *argv[])
{
    auto pn = std::string(argv[0]);
    if ((argc < 2) || (std::string(argv[1]) != pn + ".config"))
    {
      printf("Usage: \n\t%s <FilePathInput> [<scale> [<repeat count> [<framerateMs> [<testNeeded>]]]]\n\tExample:\n\t%s TestData.zip 4 4 100 1", argv[0], argv[0]);
      return 1;
    }
    std::string inputFile = "./TestData.zip";//(argc >= 2) ? argv[1] : "./TestData.zip";
    uint8_t scale = (argc >= 3) ? std::atoi(argv[2]) : 4;
    uint8_t repeatCount = (argc >= 4) ? std::atoi(argv[3]) : 2;
    uint16_t framerateMs = (argc >= 5) ? std::atoi(argv[4]) : 100;
    bool testNeeded = (argc == 6) && (bool)std::atoi(argv[5]);

    QApplication a(argc, argv);
    BinaryFileToQrCodeWindow w(inputFile, "", scale, repeatCount, framerateMs, testNeeded, false);
    w.show();
    return QApplication::exec();
}

