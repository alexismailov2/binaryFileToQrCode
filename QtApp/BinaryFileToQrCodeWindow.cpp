#include "BinaryFileToQrCodeWindow.h"

#include "../QR-Code-generator/cpp/qrcodegen.hpp"
#include "../TimeMeasuring.hpp"
#include "../Header.hpp"

#ifdef BUILD_WITH_ZBar
#include <zbar.h>
#endif

#ifdef BUILD_WITH_OpenCV
//#include <opencv2/opencv.hpp>
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

void binaryFileReadingByChunk(std::string const& filePath,
                              std::vector<uint32_t> const& indexes,
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
    if (!std::binary_search(indexes.cbegin(), indexes.cend(), header.chunkId))
    {
      cb(dataFromFile);
    }
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

void encodeBinaryFileToQRCodes(std::string const& inputFilePath,
                               std::vector<uint32_t> const& indexes,
                               std::function<void(std::vector<uint8_t>& qrMat, std::vector<uint8_t> const& chunkData)>&& cb)
{
  TAKEN_TIME();
  binaryFileReadingByChunk(inputFilePath, indexes, 2048, [&](std::vector<uint8_t> const &chunkData) {
    auto const qr = qrcodegen::QrCode::encodeBinary(chunkData, qrcodegen::QrCode::Ecc::LOW);
    auto qrMat = qrToVector(qr);
    cb(qrMat, chunkData);
  });
}

#ifdef BUILD_WITH_OpenCV
//void qrToMat(cv::Mat &output, qrcodegen::QrCode const &qr, int border = 4)
//{
//  TAKEN_TIME();
//  output = cv::Mat::ones(qr.getSize() + border * 2, qr.getSize() + border * 2, CV_8UC1);
//  for (int y = -border; y < qr.getSize() + border; y++)
//  {
//    for (int x = -border; x < qr.getSize() + border; x++)
//    {
//      output.at<uint8_t>(y + border, x + border) = qr.getModule(x, y) ? 0 : 0xFF;
//    }
//  }
//}
//
//void encodeBinaryFileToQRCodes(std::string const &inputFilePath,
//                               std::function<void(cv::Mat &qrMat, std::vector<uint8_t> const &chunkData)> &&cb)
//{
//  TAKEN_TIME();
//  binaryFileReadingByChunk(inputFilePath, 2048, [&](std::vector<uint8_t> const &chunkData) {
//    auto const qr = qrcodegen::QrCode::encodeBinary(chunkData, qrcodegen::QrCode::Ecc::LOW);
//    cv::Mat qrMat;
//    qrToMat(qrMat, qr);
//    cb(qrMat, chunkData);
//  });
//}
#endif

struct DecodedObject
{
  std::string type;
  std::string data;
#ifdef BUILD_WITH_OpenCV
//  std::vector<cv::Point> location;
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
//    for (int i = 0; i < symbol->get_location_size(); i++) {
//      obj.location.push_back(cv::Point(symbol->get_location_x(i), symbol->get_location_y(i)));
//    }
#endif

    decodedObjects.push_back(obj);
  }
  return decodedObjects;
}
#endif

#ifdef BUILD_WITH_OpenCV
//auto decode(cv::Mat const &imGray) -> std::vector<DecodedObject>
//{
//  return decode(imGray.data, imGray.cols, imGray.rows);
//}
#endif

auto readIndexes(std::string const& filename) -> std::vector<uint32_t>
{
  if (filename.empty())
  {
    return {};
  }
  std::vector<uint32_t> indexes;
  std::string index;
  auto fileWithClasses{std::ifstream(filename)};
  while (std::getline(fileWithClasses, index))
  {
    if (!index.empty())
    {
      indexes.push_back(std::atoi(index.c_str()));
    }
  }
  std::sort(indexes.begin(), indexes.end());
  return indexes;
}

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

  auto selectIndexFileLabel = new QLabel(QString::fromStdString(_selectedIndexFile));
  auto selectIndexFileButton = new QPushButton(tr("Select index data file(*.*)"));
  connect(selectIndexFileButton, &QPushButton::clicked, [=]() {
    QString file = QDir::toNativeSeparators(QFileDialog::getOpenFileName(this,
                                                                         tr("Open index file"),
                                                                         QDir::currentPath()));
    if (!file.isEmpty())
    {
      selectIndexFileLabel->setText(file);
      _selectedIndexFile = file.toStdString();
      _indexes = readIndexes(_selectedIndexFile);
    }
  });

  auto scaleSpinBox = new QSpinBox;
  scaleSpinBox->setRange(1, 10);
  scaleSpinBox->setSuffix(tr(" x"));
  scaleSpinBox->setValue(_scale);
  connect(scaleSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [&](int value){
    _scale = value;
  });

  auto repeatCountSpinBox = new QSpinBox;
  repeatCountSpinBox->setRange(1, 10);
  repeatCountSpinBox->setSuffix(tr(" count"));
  repeatCountSpinBox->setValue(_repeatCount);
  connect(repeatCountSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [&](int value){
    _repeatCount = value;
  });

  auto frameSpinBox = new QSpinBox;
  frameSpinBox->setRange(1, 60);
  frameSpinBox->setSuffix(tr(" frames/s"));
  frameSpinBox->setValue(1000/_framerateMs);
  connect(frameSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [&](int value){
    _framerateMs = 1000/value;
  });

  auto qrCountHSpinBox = new QSpinBox;
  qrCountHSpinBox->setRange(1, 32);
  qrCountHSpinBox->setSuffix(tr(" horizontal count"));
  qrCountHSpinBox->setValue(_qrCountH);
  connect(qrCountHSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [&](int value){
    _qrCountH = value;
  });

  auto qrCountVSpinBox = new QSpinBox;
  qrCountVSpinBox->setRange(1, 32);
  qrCountVSpinBox->setSuffix(tr(" vertical count"));
  qrCountVSpinBox->setValue(_qrCountV);
  connect(qrCountVSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [&](int value){
    _qrCountV = value;
  });

  auto generateButton = new QPushButton(tr("Run generating"));

  auto layoutConfig = new QVBoxLayout;
  layoutConfig->addWidget(selectFileLabel);
  layoutConfig->addWidget(selectFileButton);
  layoutConfig->addWidget(selectIndexFileLabel);
  layoutConfig->addWidget(selectIndexFileButton);
  layoutConfig->addWidget(scaleSpinBox);
  layoutConfig->addWidget(repeatCountSpinBox);
  layoutConfig->addWidget(frameSpinBox);
  layoutConfig->addWidget(qrCountHSpinBox);
  layoutConfig->addWidget(qrCountVSpinBox);
  layoutConfig->addWidget(generateButton);

  _mainWidget = new QWidget;
  setCentralWidget(_mainWidget);

  _mainWidget->setLayout(layoutConfig);

  connect(generateButton, &QPushButton::clicked, [=]() { generate(_qrCountH, _qrCountV); });
  return generateButton;
}

auto BinaryFileToQrCodeWindow::generatorLayout(size_t cols, size_t rows) -> std::vector<QLabel*>
{
  if (_mainWidget)
  {
    qDeleteAll(_mainWidget->children());
    delete _mainWidget;
  }

  auto layoutGenerate = new QGridLayout;

  std::vector<QLabel*> pictureLabels(cols * rows);
  for(uint32_t j = 0; j < cols; ++j)
  {
    for (uint32_t i = 0; i < rows; ++i)
    {
      pictureLabels[j * rows + i] = new QLabel();
      layoutGenerate->addWidget(pictureLabels[j * rows + i], i, j);
    }
  }

  _mainWidget = new QWidget;
  setCentralWidget(_mainWidget);
  _mainWidget->setLayout(layoutGenerate);
  return pictureLabels;
}

void BinaryFileToQrCodeWindow::generate(size_t cols, size_t rows)
{
  auto pictureLabels = generatorLayout(cols, rows);

  auto const countPictures = cols * rows;
  for(int repeatLoop = 0; repeatLoop < _repeatCount; ++repeatLoop)
  {
    auto counter = 0;
    encodeBinaryFileToQRCodes(_selectedFile, _indexes, [&](std::vector<uint8_t>& qrMat, std::vector<uint8_t> const& chunkData) {
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
      auto pictureLabelIndex = (counter < countPictures) ? counter : (counter % countPictures);
      pictureLabels[pictureLabelIndex]->setPixmap(QPixmap::fromImage(image));
      if (pictureLabelIndex == (countPictures - 1))
      {
        QCoreApplication::processEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(_framerateMs));
      }
      ++counter;
    });
    QCoreApplication::processEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(_framerateMs));
  }

  configLayout();
}

BinaryFileToQrCodeWindow::BinaryFileToQrCodeWindow(std::string selectedFile,
                                                   std::string selectedIndexFile,
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
    , _indexes{readIndexes(selectedIndexFile)}
{
    configLayout();

    createActions();
    createMenus();

    setWindowTitle(tr("Generator tool"));
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
        QMessageBox::about(this, tr("About Generator"), tr("The <b>Generator</b>."));
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
