#if BUILD_WITH_X11
#include <opencv2/opencv.hpp>

#include <memory>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

struct ScreenShot
{
  ScreenShot(cv::Rect rect)
    : rect(rect)
  {
    display = XOpenDisplay(nullptr);
    if (!display)
    {
      std::runtime_error("Display not gotten");
    }
    root = DefaultRootWindow(display);
    if (!root)
    {
      std::runtime_error("DefaultRootWindow");
    }
    if (XGetWindowAttributes(display, root, &window_attributes)  == 0)
    {
        std::runtime_error("window might not be valid any more");
    }
    std::cout << "window_attributes.width: " << window_attributes.width << std::endl;
    std::cout << "window_attributes.height: " << window_attributes.height << std::endl;

    int scr = XDefaultScreen(display);

    shminfo = std::make_shared<XShmSegmentInfo>();

    ximg = XShmCreateImage(display,
                           DefaultVisual(display, scr),
                           DefaultDepth(display, scr),
                           ZPixmap,
                           NULL,
                           shminfo.get(),
                           window_attributes.width,//Width(SelectedMonitor),
                           window_attributes.height);//Height(SelectedMonitor));
    shminfo->shmid = shmget(IPC_PRIVATE, ximg->bytes_per_line * ximg->height, IPC_CREAT | 0777);
    shminfo->shmaddr = ximg->data = (char*)shmat(shminfo->shmid, 0, 0);
    shminfo->readOnly = False;
    if(shminfo->shmid < 0)
    {
      throw std::runtime_error("Fatal shminfo error!");
    }
    Status s1 = XShmAttach(display, shminfo.get());
    if (!s1)
    {
      throw std::runtime_error("XShmAttach() failure!");
    }
//    root = DefaultRootWindow(display);
//
//    if (XGetWindowAttributes(display, root, &window_attributes)  == 0)
//    {
//        std::std::runtime_error("window might not be valid any more");
//    }
//    std::cout << "window_attributes.width: " << window_attributes.width << std::endl;
//    std::cout << "window_attributes.height: " << window_attributes.height << std::endl;
//
//    Screen* screen = window_attributes.screen;
//    ximg = XShmCreateImage(display,
//                           DefaultVisualOfScreen(screen),
//                           DefaultDepthOfScreen(screen),
//                           ZPixmap,
//                           NULL,
//                           &shminfo,
//                           window_attributes.width,
//                           window_attributes.height);
//
//    shminfo.shmid = shmget(IPC_PRIVATE, ximg->bytes_per_line * ximg->height, IPC_CREAT|0777);
//    shminfo.shmaddr = ximg->data = (char*)shmat(shminfo.shmid, 0, 0);
//    shminfo.readOnly = False;
//    if(shminfo.shmid < 0)
//    {
//      throw std::runtime_error("Fatal shminfo error!");
//    }
//    Status s1 = XShmAttach(display, &shminfo);
//    if (!s1)
//    {
//      throw std::runtime_error("XShmAttach() failure!");
//    }
  }

  void operator() (cv::Mat& cv_img)
  {
    if(!XShmGetImage(display,
                     RootWindow(display, DefaultScreen(display)),
                     ximg,
                     0,//OffsetX(SelectedMonitor),
                     0,//OffsetY(SelectedMonitor),
                     AllPlanes))
    {
      throw std::runtime_error("XShmGetImage() failure!");
    }
    //ProcessCapture(Data->ScreenCaptureData, *this, SelectedMonitor, (unsigned char*)XImage_->data, XImage_->bytes_per_line);
    //return Ret;
    //XShmGetImage(display, root, ximg, 0, 0, 0x00ffffff);
    cv_img = cv::Mat(window_attributes.height, window_attributes.width, CV_8UC4, ximg->data);
    cv::cvtColor(cv_img, cv_img, cv::COLOR_BGRA2GRAY);
  }

  ~ScreenShot()
  {
    if(shminfo)
    {
      shmdt(shminfo->shmaddr);
      shmctl(shminfo->shmid, IPC_RMID, 0);
      XShmDetach(display, shminfo.get());
    }
    if(ximg)
    {
      XDestroyImage(ximg);
    }
    if(display)
    {
      XCloseDisplay(display);
    }
//
//    XDestroyImage(ximg);
//    XShmDetach(display, &shminfo);
//    shmdt(shminfo.shmaddr);
//    XCloseDisplay(display);
  }

  Display* display{};
  Window root;
  XWindowAttributes window_attributes;
  XImage* ximg{};
  std::shared_ptr<XShmSegmentInfo> shminfo;

  cv::Rect rect;
};
#endif

#include "TimeMeasuring.hpp"
#include "Header.hpp"

#include <opencv2/opencv.hpp>
#include <zbar.h>

#include <iostream>
#include <fstream>
#include <map>

#ifdef __APPLE__
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#ifndef BUILD_WITH_X11
#include <ApplicationServices/ApplicationServices.h>
#include <sys/stat.h>

cv::Mat cvMatWithGrayImage(CGImageRef imageRef)
{
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
  CGFloat cols = CGImageGetWidth(imageRef);
  CGFloat rows = CGImageGetHeight(imageRef);
  cv::Mat cvMat = cv::Mat(rows, cols, CV_8UC1); // 8 bits per component, 1 channel
  CGContextRef contextRef = CGBitmapContextCreate(cvMat.data,                 // Pointer to backing data
                                                  cols,                       // Width of bitmap
                                                  rows,                       // Height of bitmap
                                                  8,                          // Bits per component
                                                  cvMat.step[0],              // Bytes per row
                                                  colorSpace,                 // Colorspace
                                                  kCGImageAlphaNone |
                                                  kCGBitmapByteOrderDefault); // Bitmap info flags

  CGContextDrawImage(contextRef, CGRectMake(0, 0, cols, rows), imageRef);
  CGContextRelease(contextRef);
  CGColorSpaceRelease(colorSpace);
  CGImageRelease(imageRef);
  return cvMat;
}

cv::Mat ScreenCapturingCropped(cv::Rect rect = {0, 0, 0, 0})
{
  TAKEN_TIME();
  CGImageRef screenShot = nullptr;
  if (!rect.empty())
  {
    CGRect cgRect;
    cgRect.origin.x = (CGFloat)rect.x;
    cgRect.origin.y = (CGFloat)rect.y;
    cgRect.size.width = (CGFloat)rect.width;
    cgRect.size.height = (CGFloat)rect.height;
    screenShot = CGDisplayCreateImageForRect(CGMainDisplayID(), cgRect);
//  CGImageRef screenShot = CGWindowListCreateImage(rect.empty() ? CGRectInfinite : cgRect,
//                                                  kCGWindowListOptionOnScreenOnly,
//                                                  kCGNullWindowID,
//                                                  kCGWindowImageDefault);
  }
  else
  {
    screenShot = CGDisplayCreateImage(CGMainDisplayID());
  }
  return cvMatWithGrayImage(screenShot);
}
#else
cv::Mat ScreenCapturingCropped(cv::Rect rect = {0, 0, 1920, 1080})
{
  TAKEN_TIME();
  static ScreenShot screen(rect);
  cv::Mat img;
  screen(img);
  return img;
}
#endif

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
  scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_BINARY, 1);

  zbar::Image image(imGray.cols, imGray.rows, "Y800", (uchar *)imGray.data, imGray.cols * imGray.rows);
  scanner.scan(image);
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

size_t filesize(std::string const &filename)
{
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

auto readChunkIndexes(std::string const& filename) -> std::set<uint32_t>
{
  std::set<uint32_t> chunkIndexes;
  std::string className;
  auto fileWithClasses{std::ifstream(filename)};
  while (std::getline(fileWithClasses, className))
  {
    if (!className.empty())
    {
      chunkIndexes.insert(std::atoi(className.c_str()));
    }
  }
  return chunkIndexes;
}

int main(int argc, char* argv[])
{
  std::string videoFile = (argc > 1) ? argv[1] : "";
  bool isOutputDebugResult = (argc > 2) ? std::atoi(argv[2]) : 0;
  if (isOutputDebugResult)
  {
    fs::create_directory("./ResultImages");
  }
  fs::create_directory("./GottenChunks");
  cv::VideoCapture cap;
  if (!videoFile.empty())
  {
    cap.open(videoFile);
  }

  std::vector<std::vector<uint8_t>> decodedDataChunks;
  std::set<uint32_t> successPacketIndexes;

  cv::Mat frame;
  cv::Mat qrMat;
  auto i = 0;

//  cv::VideoWriter video("outDecodingProcess.avi",
//                        cv::VideoWriter::fourcc('M','J','P','G'),
//                        10,
//                        cv::Size(static_cast<int32_t>(cropRoi.width),
//                                 static_cast<int32_t>(cropRoi.height)));

  successPacketIndexes = readChunkIndexes("./gottenChunks.txt");

  auto gottenChunksFile = std::ofstream("./gottenChunks.txt", std::ios::app);
  bool isCapturing = true;
  while (isCapturing && (cv::waitKey(1) < 0))
  {
    //TAKEN_TIME();
    if (!videoFile.empty())
    {
      cap >> frame;
    }
    else
    {
      qrMat = ScreenCapturingCropped(/*cropRoi*/);
    }
    cv::cvtColor(qrMat, frame, cv::COLOR_GRAY2BGR);
    if (frame.empty() || (cv::waitKey(1) == 27))
    {
      break;
    }
    std::string progressString;
    auto decodedData = decode(qrMat);
    if(decodedData.size() < 1)
    {
      std::string frameText = "QRCode not found";
      cv::putText(frame, frameText, cv::Point(10, frame.rows/2), cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 0, 255), 5);
      std::cout << frameText << std::endl;
    }
    else
    {
      for (auto j = 0; j < decodedData.size(); ++j)
      {
        Header header;
        memcpy(&header, decodedData[j].data.data(), sizeof(Header));
        if (!successPacketIndexes.count(header.chunkId))
        {
          auto outFile = std::ofstream(std::string("./GottenChunks/") + std::to_string(header.chunkId) + ".chk", std::ios::binary);
          gottenChunksFile << std::to_string(header.chunkId) << std::endl;
          outFile.write((char const*)decodedData[j].data.data() + sizeof(Header), decodedData[j].data.size() - sizeof(Header));
          successPacketIndexes.insert(header.chunkId);

          cv::rectangle(frame, cv::Rect{decodedData[j].location[0], decodedData[j].location[2]}, cv::Scalar(0, 255, 0), 3);
          std::string frameText = std::string("QRCode found, gottenChunk: ") + std::to_string(successPacketIndexes.size());
          cv::putText(frame, frameText, cv::Point(10, frame.rows/2), cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 255), 5);
        }
        else
        {
          cv::rectangle(frame, cv::Rect{decodedData[j].location[0], decodedData[j].location[2]}, cv::Scalar(0, 255, 255), 3);
          std::string frameText = std::string("QRCode exist, skip: ") + std::to_string(successPacketIndexes.size());
          cv::putText(frame, frameText, cv::Point(10, frame.rows/2), cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 255, 255), 5);
        }
        std::cout << "chunksCount: " << header.chunksCount << std::endl;
        if ((successPacketIndexes.size() == header.chunksCount) &&
            ((successPacketIndexes.size() - 1) == *successPacketIndexes.rbegin()))
        {
          isCapturing = false;
          break;
        }
        progressString = std::string("progress: ") + std::to_string(successPacketIndexes.size()) + "/" + std::to_string(header.chunksCount);
        std::cout << progressString << std::endl;
      }
    }

    std::string frameText = std::string("frame: ") + std::to_string(i++) + ", " + progressString;
    std::cout << frameText << std::endl;
    cv::putText(frame, frameText, cv::Point(0, 40), cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(255, 255, 255), 5);
    if (isOutputDebugResult)
    {
      imwrite(std::string("./ResultImages/") + std::to_string(i++) + ".jpg", frame);
    }
    //imshow("qrCaptured", frame);
    //video.write(frame);
  }

  cv::destroyAllWindows();

  if (successPacketIndexes.empty() ||
      ((successPacketIndexes.size() - 1) != *successPacketIndexes.rbegin()))
  {
    std::cout << "Some chunks was not gotten, full list of gotten chunks in gottenChunks.txt" << std::endl;
    return 1;
  }

  auto file = std::ofstream(std::string(argc < 2 ? "X" : argv[1]) + "_reassembled.zip", std::ios::binary);
  for (i = 0; i < successPacketIndexes.size(); ++i)
  {
    std::string fileName = std::string("./GottenChunks/") + std::to_string(i) + ".chk";
    auto fileSize = filesize(fileName);
    auto fileChunk = std::ifstream(fileName, std::ios::binary);
    std::vector<uint8_t> chunkData(fileSize);
    fileChunk.read((char*)chunkData.data(), fileSize);

    file.write((const char*)chunkData.data(), fileSize);
  }
  return 0;
}
