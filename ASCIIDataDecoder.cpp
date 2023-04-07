#include "TimeMeasuring.hpp"

#include <opencv2/opencv.hpp>
#include <zbar.h>

#include <iostream>
#include <fstream>

struct DecodedObject
{
  std::string type;
  std::string data;
  std::vector <cv::Point> location;
};

auto decode(cv::Mat const& imGray) -> std::vector<DecodedObject>
{
  std::vector<DecodedObject> decodedObjects;

  zbar::ImageScanner scanner;
  scanner.set_config(zbar::ZBAR_NONE, /*zbar::ZBAR_CFG_ENABLE*/zbar::ZBAR_CFG_BINARY, 1);

  zbar::Image image(imGray.cols, imGray.rows, "Y800", (uchar *)imGray.data, imGray.cols * imGray.rows);

  int n = scanner.scan(image);
  for(zbar::Image::SymbolIterator symbol = image.symbol_begin(); symbol != image.symbol_end(); ++symbol)
  {
    DecodedObject obj;

    obj.type = symbol->get_type_name();
    obj.data = symbol->get_data();

    std::cout << "Type : " << obj.type << std::endl;
    std::cout << "Data size : " << obj.data.size() << std::endl << std::endl;

    for(int i = 0; i< symbol->get_location_size(); i++)
    {
      obj.location.push_back(cv::Point(symbol->get_location_x(i),symbol->get_location_y(i)));
    }

    decodedObjects.push_back(obj);
  }
  return decodedObjects;
}

int main(int argc, char* argv[])
{
  auto file = std::ofstream(std::string(argv[1]) + "_reassembled.zip", std::ios::binary);
  cv::VideoCapture cap;
  cap.open(argv[1]);

  cv::VideoWriter video("outDecodingProcess.avi",
                        cv::VideoWriter::fourcc('M','J','P','G'),
                        10,
                        cv::Size(static_cast<int32_t>(cap.get(cv::CAP_PROP_FRAME_WIDTH)),
                                 static_cast<int32_t>(cap.get(cv::CAP_PROP_FRAME_HEIGHT))));

  std::vector<std::vector<uint8_t>> decodedDataChunks;

  cv::Mat frame;
  cv::Mat qrMat;
  auto i = 0;
  while (cv::waitKey(1) < 0)
  {
    //TAKEN_TIME();
    cap >> frame;
    if (frame.empty() || (cv::waitKey(1) == 27))
    {
      break;
    }
    cv::cvtColor(frame, qrMat, cv::COLOR_BGR2GRAY);
    auto decodedData = decode(qrMat);
    if(decodedData.size() != 1)
    {
      std::string frameText = "QRCode not found";
      cv::putText(frame, frameText, cv::Point(10, frame.rows/2), cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 0, 255), 5);
      std::cout << frameText << std::endl;
    }
    else
    {
      if (decodedDataChunks.empty() ||
          std::memcmp(decodedData[0].data.data(), decodedDataChunks.back().data(), decodedDataChunks.back().size()) != 0)
      {
        decodedDataChunks.emplace_back(decodedData[0].data.begin(), decodedData[0].data.end());
        std::string frameText = std::string("QRCode ok, chunk: ") + std::to_string(decodedDataChunks.size());
        std::cout << frameText << std::endl;
      }
      cv::rectangle(frame, cv::Rect{decodedData[0].location[0], decodedData[0].location[2]}, cv::Scalar(0, 255, 0), 3);
      std::string frameText = std::string("QRCode successfully found, gottenChunk: ") + std::to_string(decodedDataChunks.size());
      cv::putText(frame, frameText, cv::Point(10, frame.rows/2), cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 255), 5);
    }

    std::string frameText = std::string("frame: ") + std::to_string(i++);
    std::cout << frameText << std::endl;
    cv::putText(frame, frameText, cv::Point(0, 40), cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(255, 255, 255), 5);
    imshow("qrCaptured", frame);
    video.write(frame);
  }

  cv::destroyAllWindows();

  for (auto const& item : decodedDataChunks)
  {
    file.write((char const*)item.data(), item.size());
  }
  return 0;
}




