#include "BinaryFileToQrCodeWindow.h"

#include "../QR-Code-generator/cpp/qrcodegen.hpp"
#include "../TimeMeasuring.hpp"

#ifdef BUILD_WITH_ZBar
#include <zbar.h>
#endif

#ifdef BUILD_WITH_OpenCV
#include <opencv2/opencv.hpp>
#endif

#include <QtWidgets>

#include <vector>
#include <sstream>
#include <fstream>
#include <thread>

namespace {

size_t filesize(std::string const &filename)
{
  TAKEN_TIME();
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

void binaryFileReadingByChunk(std::string const &filePath,
                              size_t chunkFileSize,
                              std::function<void(std::vector<uint8_t> const &chunkData)> &&cb)
{
  TAKEN_TIME();
  auto fileSize = filesize(filePath);
  auto file = std::ifstream(filePath, std::ios::binary);
  auto fileRead = 0;
  while (fileRead < fileSize) {
    const auto chunkSize = ((fileSize - fileRead) > chunkFileSize)
                           ? chunkFileSize
                           : (fileSize - fileRead);
    //std::cout << "chunkSize: " << chunkSize << std::endl;
    std::vector<uint8_t> dataFromFile(chunkSize);
    file.read((char *) dataFromFile.data(), chunkSize);
    cb(dataFromFile);
    fileRead += chunkSize;
  }
}

auto qrToVector(qrcodegen::QrCode const &qr, int border = 4) -> std::vector<uint8_t>
{
  TAKEN_TIME();
  auto const size = qr.getSize() + border * 2;
  std::vector<uint8_t> output(size*size, 0xFF);
  for (int y = -border; y < qr.getSize() + border; y++)
  {
    for (int x = -border; x < qr.getSize() + border; x++)
    {
      output[((y + border) * size) + x + border] = qr.getModule(x, y) ? 0 : 0xFF;
    }
  }
  return output;
}

void encodeBinaryFileToQRCodes(std::string const &inputFilePath,
                               std::function<void(std::vector<uint8_t> &qrMat, std::vector<uint8_t> const &chunkData)> &&cb)
{
  TAKEN_TIME();
  binaryFileReadingByChunk(inputFilePath, 2048, [&](std::vector<uint8_t> const &chunkData) {
    auto const qr = qrcodegen::QrCode::encodeBinary(chunkData, qrcodegen::QrCode::Ecc::LOW);
    auto qrMat = qrToVector(qr);
    cb(qrMat, chunkData);
  });
}

#ifdef BUILD_WITH_OpenCV
void qrToMat(cv::Mat &output, qrcodegen::QrCode const &qr, int border = 4)
{
  TAKEN_TIME();
  output = cv::Mat::ones(qr.getSize() + border * 2, qr.getSize() + border * 2, CV_8UC1);
  for (int y = -border; y < qr.getSize() + border; y++)
  {
    for (int x = -border; x < qr.getSize() + border; x++)
    {
      output.at<uint8_t>(y + border, x + border) = qr.getModule(x, y) ? 0 : 0xFF;
    }
  }
}

void encodeBinaryFileToQRCodes(std::string const &inputFilePath,
                               std::function<void(cv::Mat &qrMat, std::vector<uint8_t> const &chunkData)> &&cb)
{
  TAKEN_TIME();
  binaryFileReadingByChunk(inputFilePath, 2048, [&](std::vector<uint8_t> const &chunkData) {
    auto const qr = qrcodegen::QrCode::encodeBinary(chunkData, qrcodegen::QrCode::Ecc::LOW);
    cv::Mat qrMat;
    qrToMat(qrMat, qr);
    cb(qrMat, chunkData);
  });
}
#endif

struct DecodedObject
{
  std::string type;
  std::string data;
#ifdef BUILD_WITH_OpenCV
  std::vector<cv::Point> location;
#endif
};

#ifdef BUILD_WITH_ZBar
auto decode(uint8_t const* data, size_t cols, size_t rows) -> std::vector<DecodedObject>
{
  TAKEN_TIME();
  std::vector<DecodedObject> decodedObjects;

  zbar::ImageScanner scanner;
  scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_BINARY, 1);

  zbar::Image image(cols, rows, "Y800", data, cols * rows);

  int n = scanner.scan(image);
  for (zbar::Image::SymbolIterator symbol = image.symbol_begin(); symbol != image.symbol_end(); ++symbol) {
    DecodedObject obj;

    obj.type = symbol->get_type_name();
    obj.data = symbol->get_data();

    std::cout << "Type : " << obj.type << std::endl;
    std::cout << "Data size : " << obj.data.size() << std::endl << std::endl;

#ifdef BUILD_WITH_OpenCV
    for (int i = 0; i < symbol->get_location_size(); i++) {
      obj.location.push_back(cv::Point(symbol->get_location_x(i), symbol->get_location_y(i)));
    }
#endif

    decodedObjects.push_back(obj);
  }
  return decodedObjects;
}
#endif

#ifdef BUILD_WITH_OpenCV
auto decode(cv::Mat const &imGray) -> std::vector<DecodedObject>
{
  return decode(imGray.data, imGray.cols, imGray.rows);
}
#endif

} // anonymous

auto BinaryFileToQrCodeWindow::configLayout() -> QPushButton*
{
  if (_mainWidget)
  {
    qDeleteAll(_mainWidget->children());
    delete _mainWidget;
  }

  auto selectFileLabel = new QLabel(QString::fromStdString(_selectedFile));
  auto selectFileButton = new QPushButton(tr("Select input data file(*.*)"));
  connect(selectFileButton, &QPushButton::clicked, [=]() {
    QString file = QDir::toNativeSeparators(QFileDialog::getOpenFileName(this,
                                                                         tr("Open data file"),
                                                                         QDir::currentPath()));
    if (!file.isEmpty())
    {
      selectFileLabel->setText(file);
      _selectedFile = file.toStdString();
    }
  });

  auto scaleSpinBox = new QSpinBox;
  scaleSpinBox->setRange(1, 10);
  scaleSpinBox->setSuffix(tr(" x"));
  scaleSpinBox->setValue(_scale);
  connect(scaleSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [&](int value){
    _scale = value;
  });

  auto repeatCountSpinBox = new QSpinBox;
  repeatCountSpinBox->setRange(1, 10);
  repeatCountSpinBox->setSuffix(tr(" count"));
  repeatCountSpinBox->setValue(_repeatCount);
  connect(repeatCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [&](int value){
    _repeatCount = value;
  });

  auto frameSpinBox = new QSpinBox;
  frameSpinBox->setRange(1, 60);
  frameSpinBox->setSuffix(tr(" frames/s"));
  frameSpinBox->setValue(1000/_framerateMs);
  connect(frameSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [&](int value){
    _framerateMs = 1000/value;
  });

  auto generateButton = new QPushButton(tr("Run generating"));

  auto layoutConfig = new QVBoxLayout;
  layoutConfig->addWidget(selectFileLabel);
  layoutConfig->addWidget(selectFileButton);
  layoutConfig->addWidget(scaleSpinBox);
  layoutConfig->addWidget(repeatCountSpinBox);
  layoutConfig->addWidget(frameSpinBox);
  layoutConfig->addWidget(generateButton);

  _mainWidget = new QWidget;
  setCentralWidget(_mainWidget);

  _mainWidget->setLayout(layoutConfig);

  connect(generateButton, &QPushButton::clicked, [=]() { generate(); });
  return generateButton;
}

auto BinaryFileToQrCodeWindow::generatorLayout() -> QLabel*
{
  if (_mainWidget)
  {
    qDeleteAll(_mainWidget->children());
    delete _mainWidget;
  }

  auto pictureLabel = new QLabel();

  auto layoutGenerate = new QVBoxLayout;
  layoutGenerate->addWidget(pictureLabel);

  _mainWidget = new QWidget;
  setCentralWidget(_mainWidget);
  _mainWidget->setLayout(layoutGenerate);
  return pictureLabel;
}

void  BinaryFileToQrCodeWindow::generate()
{
  auto pictureLabel = generatorLayout();

  for(int repeatLoop = 0; repeatLoop < _repeatCount; ++repeatLoop)
  {
    encodeBinaryFileToQRCodes(_selectedFile, [&](std::vector<uint8_t>& qrMat, std::vector<uint8_t> const& chunkData) {
      auto size = (int)sqrt(qrMat.size());
#ifdef BUILD_WITH_ZBar
      if (_testNeeded)
      {
        auto decodedData = decode(qrMat.data(), size, size);
        if(decodedData.empty())
        {
          throw std::runtime_error("fatal error not detected");
        }
        if (std::memcmp(decodedData[0].data.data(), chunkData.data(), chunkData.size()) != 0)
        {
          throw std::runtime_error("fatal error not equal");
        }
      }
#endif
      auto image = QImage(qrMat.data(),
                          size,
                          size,
                          size,
                          QImage::Format_Grayscale8).scaled(size * _scale, size * _scale, Qt::KeepAspectRatio);
      pictureLabel->setPixmap(QPixmap::fromImage(image));
      QCoreApplication::processEvents();
      std::this_thread::sleep_for(std::chrono::milliseconds(_framerateMs));
    });
  }

  configLayout();
}

BinaryFileToQrCodeWindow::BinaryFileToQrCodeWindow(std::string selectedFile,
                                                   uint8_t scale,
                                                   uint8_t repeatCount,
                                                   uint16_t framerateMs,
                                                   bool testNeeded,
                                                   bool isFullscreen,
                                                   QWidget *parent)
    : QMainWindow(parent)
    , _selectedFile{std::move(selectedFile)}
    , _scale{scale}
    , _repeatCount{repeatCount}
    , _framerateMs{framerateMs}
    , _testNeeded{testNeeded}
    , _isFullscreen{isFullscreen}
{
    configLayout();

    createActions();
    createMenus();

    setWindowTitle(tr("Binary file to Qr code tool"));
}

void BinaryFileToQrCodeWindow::createActions()
{
    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, &QAction::triggered, [this]() {
        QMessageBox::about(this, tr("About OIYolo"), tr("The <b>OIYolo</b> Demonstrates usage of OIYolo helpers."));
    });

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void BinaryFileToQrCodeWindow::createMenus()
{
    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}
