#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <stdexcept>

#include <QDebug>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QThread>
#include <QStyle>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QGuiApplication>
#include <QMessageBox>
#include <QScreen>
#include <QRect>
#include <QPixmap>
#include <QWindow>
#include <QDesktopServices>

#include <KX11Extras>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <KX11Extras>
#include <KWindowInfo>
#include <KWindowSystem>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <tesseract/baseapi.h>

const QString MainWindow::REPO_API_URL = "https://api.github.com/repos/llalon/DotaAutoQueue/branches/main";
const QString MainWindow::REPO_URL = "https://github.com/llalon/DotaAutoQueue";
const QString MainWindow::DOTA_WINDOW = "Dota 2";
const int MainWindow::POLL_INTERVAL_MS = 5000;
const QString MainWindow::MESSAGE_CONTENT = "Dota Auto Queue has found and accepted a game!";
const QString MainWindow::MESSAGE_ERROR = "Dota Auto Queue has encountered an error and stopped!";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), trayIcon(nullptr), restoreAction(nullptr), quitAction(nullptr),
      queueAction(nullptr), queueTimer(nullptr)
{
    ui->setupUi(this);

    setWindowIcon(QIcon(":/resources/DotaAutoQueue.png"));

    networkManager = new QNetworkAccessManager(this);

    appSettings = new QSettings("DotaAutoQueue", "DotaAutoQueue", this);

    createTrayIcon();
    createQueueProcess();
    createForm();

    connect(ui->actionHome_page, &QAction::triggered, this, &MainWindow::openHomePage);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->actionCheck_For_Update, &QAction::triggered, this, &MainWindow::updateCheck);
    connect(ui->actionQuit, &QAction::triggered, this, &QApplication::quit);

    updateLabels();
}

MainWindow::~MainWindow()
{
    appSettings->setValue("discordWebhook", ui->discordWebhookEdit->text());
    delete ui;
}

void MainWindow::updateCheck()
{
    qDebug() << "Checking for updates";

    QNetworkRequest request(REPO_API_URL);
    request.setRawHeader("Accept", "application/vnd.github.v3+json");

    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]()
            {
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::critical(this, "Error", "Could not check for updates: " + reply->errorString());
            reply->deleteLater();
            return;
        }

        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

        if (!jsonDoc.isNull() && jsonDoc.isObject()) {
            QJsonObject jsonObj = jsonDoc.object();
            QString latestHash = jsonObj["commit"].toObject()["sha"].toString();

            if (latestHash.startsWith(QString(GIT_HASH))) {
                QMessageBox::information(this, "Update Check", "Your application is up to date.");
            } else {
                QMessageBox updateBox;
                updateBox.setWindowTitle("Update Available");
                updateBox.setText("There is an update available. Would you like to download it?");
                updateBox.setIcon(QMessageBox::Information);
                updateBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                updateBox.setDefaultButton(QMessageBox::No);

                if (updateBox.exec() == QMessageBox::Yes) {
                    openHomePage();
                }
            }
        } else {
            QMessageBox::warning(this, "Error", "Could not parse the update information.");
        }

        reply->deleteLater(); });
}

void MainWindow::openHomePage()
{
    QUrl url(REPO_URL);
    QDesktopServices::openUrl(url);
}

void MainWindow::showAbout()
{
    QMessageBox aboutBox;

    aboutBox.setWindowTitle("About");
    aboutBox.setText(QString("DotaAutoQueue\nAutomatically accept dota 2 games.\n\nVersion: %1\n").arg(QString(GIT_HASH)));

    aboutBox.setIconPixmap(windowIcon().pixmap(48, 48));
    aboutBox.setWindowIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));

    aboutBox.exec();
}

void MainWindow::createForm()
{
    QString savedWebhook = appSettings->value("discordWebhook").toString();
    ui->discordWebhookEdit->setText(savedWebhook);

    connect(ui->queueButton, &QPushButton::clicked, this, &MainWindow::toggleQueue);
}

void MainWindow::createTrayIcon()
{
    qDebug("Creating tray icon");

    QIcon trayIconImage = windowIcon();

    trayIcon = new QSystemTrayIcon(trayIconImage, this);

    auto *trayIconMenu = new QMenu(this);

    queueAction = new QAction("Start Queuing", this);
    connect(queueAction, &QAction::triggered, this, &MainWindow::toggleQueue);
    trayIconMenu->addAction(queueAction);

    restoreAction = new QAction("Hide", this);
    connect(restoreAction, &QAction::triggered, this, &MainWindow::toggleWindowVisibility);
    trayIconMenu->addAction(restoreAction);

    quitAction = new QAction("Quit", this);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
    trayIconMenu->addAction(quitAction);

    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->show();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (trayIcon && trayIcon->isVisible())
    {
        hide();
        event->ignore();
    }
}

void MainWindow::toggleWindowVisibility()
{
    if (isVisible())
    {
        hide();
    }
    else
    {
        show();
        activateWindow();
        raise();
    }

    updateLabels();
}

void MainWindow::updateLabels()
{
    if (isVisible())
    {
        restoreAction->setText("Hide");
    }
    else
    {
        restoreAction->setText("Show");
    }

    if (queueTimer->isActive())
    {
        ui->queueButton->setText("Stop Queueing");
        queueAction->setText("Stop Queueing");

        ui->discordWebhookEdit->setDisabled(true);
    }
    else
    {
        ui->queueButton->setText("Start Queueing");
        queueAction->setText("Start Queueing");

        ui->discordWebhookEdit->setDisabled(false);
    }
}

void MainWindow::createQueueProcess()
{
    queueTimer = new QTimer(this);

    connect(queueTimer, &QTimer::timeout, this, &MainWindow::queueTick);
}

void MainWindow::toggleQueue(const bool gameFound)
{
    qDebug() << "Toggling queue mode";

    if (queueTimer->isActive())
    {
        qDebug("Stopping Queue");

        if (gameFound)
        {
            hide();
        }
        else
        {
            show();
        }

        queueTimer->stop();

        updateLabels();

        qDebug("Queuing stopped.");
    }
    else
    {
        qDebug("Starting Queue");
        try
        {
            findDotaWindow();
        }
        catch (const std::runtime_error &e)
        {
            qDebug() << "Error Finding dota window" << e.what();
            QMessageBox::warning(this, "Error", "Unable to find dota");
            return;
        }

        queueTimer->start(POLL_INTERVAL_MS);

        // Hide window automatically on queue
        hide();

        updateLabels();

        qDebug("Queuing started.");
    }
}

void MainWindow::sendNotification(const QString &message)
{
    QString discordWebHook = ui->discordWebhookEdit->text().trimmed();
    QUrl discordWebHookUrl = (QUrl(discordWebHook));

    if (discordWebHook.isEmpty())
    {
        qDebug() << "Skipping: Webhook URL is empty.";
        return;
    }

    if (!discordWebHookUrl.isValid())
    {
        qDebug() << "Error: Invalid webhook URL.";
        return;
    }

    qDebug() << "Sending message to: " << discordWebHookUrl;

    // Create JSON data for the message
    QJsonObject json;
    json["content"] = message;

    QJsonDocument doc(json);
    QByteArray postData = doc.toJson();

    QNetworkRequest request(discordWebHookUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = networkManager->post(request, postData);

    connect(reply, &QNetworkReply::finished, [=]()
            {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Error:" << reply->errorString();
        } else {
            qDebug() << "Message sent successfully!";
        }
        reply->deleteLater(); });
}

cv::Mat MainWindow::loadImage() const
{
    qDebug("Loading screen and image");

    // Get the QScreen instance
    QScreen *screen = QGuiApplication::primaryScreen();

    qDebug("Setting window active");
    KX11Extras::forceActiveWindow(dotaWindowId);
    KX11Extras::unminimizeWindow(dotaWindowId);

    qDebug("Capturing window");
    QPixmap windowPixmap = screen->grabWindow(dotaWindowId);
    QImage windowImage = windowPixmap.toImage();
    cv::Mat originalImage(windowImage.height(), windowImage.width(), CV_8UC4, (uchar *)windowImage.bits(),
                          windowImage.bytesPerLine());

    qDebug("Scaling image");
    const auto scaleWidth = 1920.0;
    const auto scaleHeight = 1080.0;

    const double scaleRatio = std::min(scaleWidth / originalImage.cols, scaleHeight / originalImage.rows);
    const cv::Size scaledSize(static_cast<int>(originalImage.cols * scaleRatio),
                              static_cast<int>(originalImage.rows * scaleRatio));

    cv::Mat scaledImage;
    cv::resize(originalImage, scaledImage, scaledSize, 0, 0, cv::INTER_LINEAR);

    return scaledImage;
}

bool MainWindow::testForGame(const cv::Mat &scaledImage)
{
    qDebug() << "Testing image";

    // Crop coordinates (based on 1920x1080)
    const int width = 1920;
    const int height = 1080;

    // Calculate coordinates for a rectangle centered in the image of size 200x50
    const int rectWidth = 200;
    const int rectHeight = 50;

    const int centerX = width / 2;
    const int centerY = (height / 2) - (rectHeight / 2);
    const int rectTopLeftX = centerX - rectWidth / 2;
    const int rectTopLeftY = centerY - rectHeight / 2;
    const int rectBottomRightX = centerX + rectWidth / 2;
    const int rectBottomRightY = centerY + rectHeight / 2;

    // Define the crop region
    const int cropX = rectTopLeftX;
    const int cropY = rectTopLeftY;
    const int cropWidth = rectBottomRightX - rectTopLeftX;
    const int cropHeight = rectBottomRightY - rectTopLeftY;

    // Crop accept dialog box
    qDebug("Cropping image");
    cv::Rect cropRegion(cropX, cropY, cropWidth, cropHeight);
    cropRegion &= cv::Rect(0, 0, scaledImage.cols, scaledImage.rows);
    cv::Mat croppedImage = scaledImage(cropRegion).clone(); // Ensure copy

    // DEBUG VIEW IMAGE
    //     cv::namedWindow("Screenshot Preview", cv::WINDOW_NORMAL);
    //     cv::resizeWindow("Screenshot Preview", croppedImage.cols, croppedImage.rows);
    //     cv::imshow("Screenshot Preview", croppedImage);
    //     cv::waitKey(0);
    //     cv::destroyWindow("Screenshot Preview");
    // END DEBUG VIEW IMAGE

    // Preoptimize image for detection
    qDebug("Preprocessing image");
    cv::Mat grayImage;
    cv::cvtColor(croppedImage, grayImage, cv::COLOR_BGR2GRAY);

    // Tesseract OCR
    qDebug("Processing image");
    tesseract::TessBaseAPI tess;
    tess.Init(nullptr, "eng", tesseract::OEM_DEFAULT);
    tess.SetPageSegMode(tesseract::PSM_AUTO);
    tess.SetImage(grayImage.data, grayImage.cols, grayImage.rows, 1, grayImage.step);
    const char *outText = tess.GetUTF8Text();
    tess.End();

    // Post proccess OCR text
    qDebug("Post processing image");
    std::string textResult = std::string(outText);
    std::transform(textResult.begin(), textResult.end(), textResult.begin(), ::toupper);

    qDebug() << "Extracted text: " << outText;

    // Check result
    return textResult.find("ACCEPT") != std::string::npos;
}

void MainWindow::queueTick()
{
    qInfo("Checking for game");

    try
    {
        const cv::Mat image = loadImage();

        if (image.empty())
        {
            throw std::runtime_error("Failed to load screen image");
        }

        const bool gameAcceptable = testForGame(image);

        if (gameAcceptable)
        {
            qInfo("Found game");
            acceptGame();
            sendNotification(MESSAGE_CONTENT);

            // Stop queue
            toggleQueue(true);
        }
        else
        {
            qDebug("Not game found");
        }
    }
    catch (const std::exception &e)
    {
        qDebug() << "Error in queue tick: " << e.what();
        QMessageBox::critical(this, "Error", "Error encountered during queueing");
        sendNotification(MESSAGE_ERROR);
    }
}

void MainWindow::findDotaWindow()
{
    qDebug("Finding dota 2 window");

    QList<WId> windowIds = KX11Extras::windows();

    for (WId id : windowIds)
    {
        KWindowInfo info(id, NET::WMName | NET::WMVisibleName | NET::WMState | NET::WMWindowType);

        QString windowName = info.name();
        QString visibleName = info.visibleName();
        NET::WindowType windowType = info.windowType(NET::AllTypesMask);

        qDebug() << "Checking window ID:" << id
                 << "Name:" << windowName
                 << "Visible Name:" << visibleName
                 << "Type:" << windowType;

        // Find by name"
        // const bool match = info.name().toLower().contains("feh");
        const bool match = info.name().toLower() == DOTA_WINDOW.toLower();

        if (match)
        {
            qDebug() << "Window found: " << id;
            dotaWindowId = id;
            return;
        }
    }

    throw std::runtime_error("Dota window not found.");
}

void MainWindow::acceptGame() const
{
    qDebug("Accepting game");

    qDebug("Setting window active");
    KX11Extras::forceActiveWindow(dotaWindowId);
    KX11Extras::unminimizeWindow(dotaWindowId);

    // Get the geometry of the Dota window to find button
    Display *const display = XOpenDisplay(nullptr);
    if (display == nullptr)
    {
        throw std::runtime_error("Unable to open X display");
    }

    const Window dotaWindow = dotaWindowId;

    XWindowAttributes windowAttributes;
    if (!XGetWindowAttributes(display, dotaWindow, &windowAttributes))
    {
        XCloseDisplay(display);
        throw std::runtime_error("Failed to get window attributes");
    }

    // Calculate the center coordinates
    // This is where button is
    const int windowCenterX = windowAttributes.x + windowAttributes.width / 2;
    const int windowCenterY = (windowAttributes.y + windowAttributes.height / 2) - 25;

    XSetInputFocus(display, dotaWindow, RevertToPointerRoot, CurrentTime);

    // Move cursor (not teleport)
    qDebug() << "Moving cursor to window center:"
             << "X:" << windowCenterX << ", Y:" << windowCenterY;

    // Teleport cursor to a position 500 pixels below the target then slowly move it up to trigger the accept dialog properly.
    // It doesn't work if the move is brought onto it like a human would...
    const int teleportY = windowCenterY + 500;

    XWarpPointer(display, None, dotaWindow, 0, 0, 0, 0, windowCenterX, teleportY);
    XFlush(display);

    const int steps = 50;
    for (int i = 0; i <= steps; ++i)
    {
        int y = teleportY + (windowCenterY - teleportY) * i / steps;

        XWarpPointer(display, None, dotaWindow, 0, 0, 0, 0, windowCenterX, y);
        XFlush(display);
        QThread::msleep(10);
    }

    // Sleep before clicking
    QThread::msleep(500);

    // Click and accept the game
    qDebug("Clicking cursor in window center");
    const auto press = XTestFakeButtonEvent(display, Button1, True, 500);
    qDebug() << "Pressed: " << press;

    const auto release = XTestFakeButtonEvent(display, Button1, False, 500);
    qDebug() << "Released: " << release;

    XFlush(display);
    XCloseDisplay(display);

    // Error if we couldn't press the screen!
    if (press + release != 2)
    {
        throw std::runtime_error("Failed to click accept");
    }

    qDebug("Game accepted");
}
