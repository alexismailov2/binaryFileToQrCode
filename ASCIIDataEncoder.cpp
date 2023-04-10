#include "TimeMeasuring.hpp"
#include "Header.hpp"
#include "QR-Code-generator/cpp/qrcodegen.hpp"

#include <opencv2/opencv.hpp>
#include <zbar.h>

#include <iostream>
#include <fstream>

namespace {

struct DecodedObject
{
  std::string type;
  std::string data;
  std::vector<cv::Point> location;
};

auto decode(cv::Mat const &imGray) -> std::vector<DecodedObject>
{
  TAKEN_TIME();
  std::vector<DecodedObject> decodedObjects;

  zbar::ImageScanner scanner;
  scanner.set_config(zbar::ZBAR_NONE, /*zbar::ZBAR_CFG_ENABLE*/zbar::ZBAR_CFG_BINARY, 1);

  zbar::Image image(imGray.cols, imGray.rows, "Y800", (uchar *) imGray.data, imGray.cols * imGray.rows);

  int n = scanner.scan(image);
  for (zbar::Image::SymbolIterator symbol = image.symbol_begin(); symbol != image.symbol_end(); ++symbol) {
    DecodedObject obj;

    obj.type = symbol->get_type_name();
    obj.data = symbol->get_data();

    std::cout << "Type : " << obj.type << std::endl;
    std::cout << "Data size : " << obj.data.size() << std::endl << std::endl;

    for (int i = 0; i < symbol->get_location_size(); i++) {
      obj.location.push_back(cv::Point(symbol->get_location_x(i), symbol->get_location_y(i)));
    }

    decodedObjects.push_back(obj);
  }
  return decodedObjects;
}

size_t filesize(std::string const &filename)
{
  TAKEN_TIME();
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

//void binaryFileReadingByChunk(std::string const &filePath,
//                              size_t chunkFileSize,
//                              std::function<void(std::vector<uint8_t> const &chunkData)> &&cb)
//{
//  TAKEN_TIME();
//  auto fileSize = filesize(filePath);
//  auto file = std::ifstream(filePath, std::ios::binary);
//  auto fileRead = 0;
//  while (fileRead < fileSize) {
//    const auto chunkSize = ((fileSize - fileRead) > chunkFileSize)
//                           ? chunkFileSize
//                           : (fileSize - fileRead);
//    //std::cout << "chunkSize: " << chunkSize << std::endl;
//    std::vector<uint8_t> dataFromFile(chunkSize);
//    file.read((char *) dataFromFile.data(), chunkSize);
//    cb(dataFromFile);
//    fileRead += chunkSize;
//  }
//}

void binaryFileReadingByChunk(std::string const &filePath,
                              size_t chunkFileSize,
                              std::function<void(std::vector<uint8_t> const &chunkData)> &&cb)
{
  TAKEN_TIME();
  auto fileSize = filesize(filePath);
  auto file = std::ifstream(filePath, std::ios::binary);
  auto fileRead = 0;
  auto i = 0;
  while (fileRead < fileSize) {
    const auto chunkSize = ((fileSize - fileRead) > chunkFileSize)
                           ? chunkFileSize
                           : (fileSize - fileRead);
    //std::cout << "chunkSize: " << chunkSize << std::endl;
    std::vector<uint8_t> dataFromFile(chunkFileSize + sizeof(Header)); //chunkSize
    Header header{};
    header.chunkId = i++;
    header.chunksCount = (fileSize / chunkFileSize) + ((fileSize % chunkFileSize) ? 1 : 0);
    header.payloadSize = chunkSize;
    memcpy(dataFromFile.data(), &header, sizeof(Header));
    file.read((char *) dataFromFile.data() + sizeof(Header), chunkSize);
    cb(dataFromFile);
    fileRead += chunkSize;
  }
}

void printQr(qrcodegen::QrCode const &qr)
{
  TAKEN_TIME();
  int border = 4;
  for (int y = -border; y < qr.getSize() + border; y++) {
    for (int x = -border; x < qr.getSize() + border; x++) {
      std::cout << (qr.getModule(x, y) ? "##" : "  ");
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}

void qrToMat(cv::Mat &output, qrcodegen::QrCode const &qr, int border = 4)
{
  TAKEN_TIME();
  output = cv::Mat::ones(qr.getSize() + border * 2, qr.getSize() + border * 2, CV_8UC1);
  for (int y = -border; y < qr.getSize() + border; y++) {
    for (int x = -border; x < qr.getSize() + border; x++) {
      output.at<uint8_t>(y + border, x + border) = qr.getModule(x, y) ? 0 : 0xFF;
    }
  }
}

uint8_t constexpr NOT_HEX = -1;

uint8_t char_to_hex(char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return NOT_HEX;
}

std::string hex_to_string(uint8_t n)
{
  std::string res;

  res += "0123456789ABCDEF"[n % 16];
  n >>= 4;
  res += "0123456789ABCDEF"[n % 16];

  return std::string(res.rbegin(), res.rend());
}

void fileToHex(std::string const &inputFilePath, std::string const &outputFilePath)
{
  TAKEN_TIME();
  auto outFile = std::ofstream(outputFilePath);
  binaryFileReadingByChunk(inputFilePath, 4096, [&](std::vector<uint8_t> const &chunkData) {
    for (auto const &item: chunkData) {
      outFile << hex_to_string(item);
    }
  });
}

void hexToFile(std::string const &inputFilePath, std::string const &outputFilePath)
{
  TAKEN_TIME();
  auto outFile = std::ofstream(outputFilePath, std::ios::binary);
  binaryFileReadingByChunk(inputFilePath, 4096, [&](std::vector<uint8_t> const &chunkData) {
    for (auto i = 0; i < chunkData.size(); i += 2) {
      uint8_t binaryValue = (char_to_hex(chunkData[i]) << 4) + char_to_hex(chunkData[i + 1]);
      outFile.write((const char *) &binaryValue, 1);
    }
  });
}

void encodeBinaryFileToQRCodes(std::string const &inputFilePath,
                               std::function<void(cv::Mat &qrMat, std::vector<uint8_t> const &chunkData)> &&cb)
{
  TAKEN_TIME();
  binaryFileReadingByChunk(inputFilePath, 2048, [&](std::vector<uint8_t> const &chunkData) {
    auto const qr = qrcodegen::QrCode::encodeBinary(chunkData, qrcodegen::QrCode::Ecc::LOW);
    //printQr(qr);
    cv::Mat qrMat;
    qrToMat(qrMat, qr);
    cb(qrMat, chunkData);
  });
}

} // anonymous

int main(int argc, char* argv[])
{
  //fileToHex(argv[1], std::string(argv[1]) + ".hex");
  //hexToFile(std::string(argv[1]) + ".hex", std::string(argv[1]) + ".hex.zip");
  if (argc < 2)
  {
    printf("Usage: \n\t%s <FilePathInput> [<scale> [<repeat count> [<framerateMs> [<testNeeded>]]]]\n\tExample:\n\t%s TestData.zip 4 4 100 1", argv[0], argv[0]);
    return 0;
  }
  uint8_t scale = (argc == 3) ? std::atoi(argv[2]) : 4;
  uint8_t repeatCount = (argc == 4) ? std::atoi(argv[3]) : 2;
  uint8_t framerateMs = (argc == 5) ? std::atoi(argv[4]) : 100;
  uint8_t testNeeded = (argc == 6) ? std::atoi(argv[5]) : 0;

  for(int repeatLoop = 0; repeatLoop < repeatCount; ++repeatLoop)
  {
    auto i = 0;
    encodeBinaryFileToQRCodes(argv[1], [&](cv::Mat& qrMat, std::vector<uint8_t> const& chunkData) {
      cv::resize(qrMat, qrMat, cv::Size(qrMat.rows * scale,qrMat.cols * scale), 0, 0, cv::INTER_NEAREST);
      if (testNeeded)
      {
        auto decodedData = decode(qrMat);
        if(decodedData.empty())
        {
          throw std::runtime_error("fatal error not detected");
        }
//        for (auto const& item : decodedData[0].data)
//        {
//          printf("%02X ", item);
//        }
//        printf("\n");
//        for (auto const& item : chunkData)
//        {
//          printf("%02X ", item);
//        }
//        printf("\n");
        if (std::memcmp(decodedData[0].data.data(), chunkData.data(), chunkData.size()) != 0)
        {
          throw std::runtime_error("fatal error not equal");
        }
      }

      cv::imwrite(std::string("./QrEncoded/output_") + std::to_string(i++) + ".png", qrMat);
      cv::imshow("capture me", qrMat);
      cv::waitKey(framerateMs);
    });
  }
  return 0;
}
