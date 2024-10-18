#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QNetworkAccessManager>
#include <QSettings>

#include <opencv2/opencv.hpp>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow final : public QMainWindow {
Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow() final;

protected:
    void closeEvent(QCloseEvent *event) final;

private slots:

    void toggleQueue(const bool gameFound = false);

    void toggleWindowVisibility();

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *trayIcon;
    QNetworkAccessManager *networkManager;
    QSettings *appSettings;

    QAction *restoreAction;
    QAction *quitAction;
    QAction *queueAction;

    QTimer *queueTimer;

    WId dotaWindowId;

    static bool testForGame(const cv::Mat &image);

    void findDotaWindow();

    cv::Mat loadImage() const;

    void createForm();

    void createTrayIcon();

    void createQueueProcess();

    void queueTick();

    void sendNotification(const QString &message);

    void acceptGame() const;

    void updateLabels();

    void openHomePage();

    void showAbout();

    void updateCheck();

    static const int POLL_INTERVAL_MS;
    static const QString MESSAGE_CONTENT;
    static const QString MESSAGE_ERROR;
    static const QString DOTA_WINDOW;
    static const QString REPO_URL;
    static const QString REPO_API_URL;
};

#endif // MAINWINDOW_H
