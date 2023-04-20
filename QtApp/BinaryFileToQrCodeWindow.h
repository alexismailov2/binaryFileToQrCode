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
                             std::string selectedIndexFile,
                             uint8_t scale,
                             uint8_t repeatCount,
                             uint16_t framerateMs,
                             bool testNeeded,
                             bool isFullscreen = true,
                             QWidget *parent = nullptr);
    ~BinaryFileToQrCodeWindow() override = default;
    auto configLayout() -> QPushButton*;
    auto generatorLayout(size_t cols = 1, size_t rows = 1) -> std::vector<QLabel*>;
    void generate(size_t cols = 1, size_t rows = 1);

private:
    void createActions();
    void createMenus();

    QMenu* helpMenu{};
    QAction* exitAct{};
    QAction* aboutAct{};
    QAction* aboutQtAct{};
    QWidget* _mainWidget{};

    std::string _selectedFile{};
    std::string _selectedIndexFile{};
    uint8_t _scale{};
    uint8_t _repeatCount{};
    uint16_t _framerateMs{};
    uint16_t _qrCountH{1};
    uint16_t _qrCountV{1};
    bool _testNeeded{};
    std::vector<uint32_t> _indexes;
    bool _isFullscreen{};
};

