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

// g++ screena.cpp -o screena -lX11 -lXext -Ofast -mfpmath=both -march=native -m64 -funroll-loops -mavx2 `pkg-config opencv --cflags --libs` && ./screena
//#define FPS(start) (CLOCKS_PER_SEC / (clock()-start))
//int main()
//{
//  ScreenShot screen(0, 0, 1920, 1080);
//  cv::Mat img;
//
//  for(uint i;; ++i)
//  {
//    double start = clock();
//
//    screen(img);
//
//    if (!(i & 0b111111))
//    {
//      printf("fps %4.f  spf %.4f\n", FPS(start), 1 / FPS(start));
//    }
//    break;
//  }
//
//  cv::imshow("img", img);
//  cv::waitKey(0);
//}
#endif

#include "TimeMeasuring.hpp"
#include "Header.hpp"

#include <opencv2/opencv.hpp>
#include <zbar.h>

#include <iostream>
#include <fstream>
#include <map>

#ifndef BUILD_WITH_X11
#include <ApplicationServices/ApplicationServices.h>

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

//cv::Mat cvMatWithImage(CGImageRef image)
//{
//  CGColorSpaceRef colorSpace = CGImageGetColorSpace(image);
//  size_t numberOfComponents = CGColorSpaceGetNumberOfComponents(colorSpace);
//  CGFloat cols = image.size.width;
//  CGFloat rows = image.size.height;
//  CGImageGetBytesPerRow(image);
//
//  cv::Mat cvMat(rows, cols, CV_8UC4); // 8 bits per component, 4 channels
//  CGBitmapInfo bitmapInfo = kCGImageAlphaNoneSkipLast | kCGBitmapByteOrderDefault;
//
//  // check whether the UIImage is greyscale already
//  if (numberOfComponents == 1)
//  {
//    cvMat = cv::Mat(rows, cols, CV_8UC1); // 8 bits per component, 1 channels
//    bitmapInfo = kCGImageAlphaNone | kCGBitmapByteOrderDefault;
//  }
//
//  CGContextRef contextRef = CGBitmapContextCreate(cvMat.data, // Pointer to backing data
//                                                  cols, // Width of bitmap
//                                                  rows, // Height of bitmap
//                                                  8, // Bits per component
//                                                  cvMat.step[0], // Bytes per row
//                                                  colorSpace, // Colorspace
//                                                  bitmapInfo); // Bitmap info flags
//
//  CG.ContextDrawImage(contextRef, CGRectMake(0, 0, cols, rows), image);
//  CGContextRelease(contextRef);
//
//  return cvMat;
//}

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
  cv::VideoCapture cap;
  if (argc > 1)
  {
    cap.open(argv[1]);
  }

  std::vector<std::vector<uint8_t>> decodedDataChunks;
  std::set<uint32_t> successPacketIndexes;

  cv::Mat frame;
  cv::Mat qrMat;
  cv::Rect detectedRegionOfInterest;
  auto i = 0;

#if 0
  auto initFrame = ScreenCapturingCropped();
  cv::Rect cropRoi = cv::Rect(initFrame.cols/2/2, 200/2, (initFrame.cols/2 - 100)/2, (initFrame.rows - 400)/2);
  while (cv::waitKey(1) < 0)
  {
    frame = ScreenCapturingCropped(/*cropRoi*/);
    if (frame.empty() || (cv::waitKey(1) == 27))
    {
      break;
    }
    //qrMat = frame;//(cropRoi);
    //cv::imshow("testGray", qrMat);
    //cv::waitKey(10);
    auto decodedData = decode(frame);
    if(decodedData.size() >= 1)
    {
      for (auto const& item : decodedData)
      {
        detectedRegionOfInterest |= cv::Rect{decodedData[0].location[0], decodedData[0].location[2]};
      }
      break;
    }
  }
//  cv::cvtColor(frame, frame, cv::COLOR_GRAY2BGR);
//  cv::rectangle(frame, detectedRegionOfInterest, cv::Scalar(0, 255, 0), 3);
//  cv::imshow("testGray", frame);
//  cv::waitKey(10);
  if (detectedRegionOfInterest.empty())
  {
    return 1;
  }
  cv::destroyAllWindows();
#endif
//  cv::VideoWriter video("outDecodingProcess.avi",
//                        cv::VideoWriter::fourcc('M','J','P','G'),
//                        10,
//                        cv::Size(static_cast<int32_t>(cropRoi.width),
//                                 static_cast<int32_t>(cropRoi.height)));

  successPacketIndexes = readChunkIndexes("./gottenChunks.txt");

  auto gottenChunksFile = std::ofstream("./gottenChunks.txt");
  bool isCapturing = true;
  while (isCapturing && (cv::waitKey(1) < 0))
  {
    //TAKEN_TIME();
    if (argc > 1)
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
    imwrite(std::string("./ResultImages/") + std::to_string(i++) + ".jpg", frame);
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
