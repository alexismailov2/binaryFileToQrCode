#include <QApplication>

#include "BinaryFileToQrCodeWindow.h"

#include <iostream>
#include <stdio.h>
#include <termios.h>

ssize_t my_getpass(char **lineptr, size_t* n, FILE *stream)
{
  // Turn echoing off and fail if we can't.
  struct termios old;
  if (tcgetattr (fileno (stream), &old) != 0)
  {
    return -1;
  }
  auto new_ = old;
  new_.c_lflag &= ~ECHO;
  if (tcsetattr (fileno (stream), TCSAFLUSH, &new_) != 0)
  {
    return -1;
  }

  // Read the password.
  int nread = getline (lineptr, n, stream);

  // Restore terminal.
  (void) tcsetattr (fileno (stream), TCSAFLUSH, &old);

  return nread;
}

int main(int argc, char *argv[])
{
    char * linePtr = nullptr;
    size_t lineLength = 0;
    if (my_getpass(&linePtr, &lineLength, stdin) == -1)
    {
      printf("Usage: \n\t%s <FilePathInput> [<scale> [<repeat count> [<framerateMs> [<testNeeded>]]]]\n\tExample:\n\t%s TestData.zip 4 4 100 1", argv[0], argv[0]);
      return 1;
    }
    auto pn = std::string(argv[0]);
    auto pe = std::string(linePtr);
    if ((pe != pn + ".config\n"))
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

