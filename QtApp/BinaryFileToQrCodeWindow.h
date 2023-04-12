#pragma once

#include <QMainWindow>

#include <vector>

QT_BEGIN_NAMESPACE
class QAction;
class QActionGroup;
class QLabel;
class QMenu;
class QPushButton;
QT_END_NAMESPACE

class BinaryFileToQrCodeWindow : public QMainWindow
{
    Q_OBJECT

public:
    BinaryFileToQrCodeWindow(std::string selectedFile,
                             uint8_t scale,
                             uint8_t repeatCount,
                             uint16_t framerateMs,
                             bool testNeeded,
                             bool isFullscreen = true,
                             QWidget *parent = nullptr);
    ~BinaryFileToQrCodeWindow() = default;
    auto configLayout() -> QPushButton*;
    auto generatorLayout(size_t countPictures = 1) -> std::vector<QLabel*>;
    void generate(size_t countPictures = 1);

private:
    void createActions();
    void createMenus();

    QMenu* helpMenu{};
    QAction* exitAct{};
    QAction* aboutAct{};
    QAction* aboutQtAct{};
    QWidget* _mainWidget{};

    std::string _selectedFile{};
    uint8_t _scale{};
    uint8_t _repeatCount{};
    uint16_t _framerateMs{};
    uint16_t _qrCount{};
    bool _testNeeded{};
    bool _isFullscreen{};
};

