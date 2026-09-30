// Minimal Qt Widgets app for checking a linuxfb setup: it shows which QPA
// platform and screen Qt picked, a live clock (proves the display keeps
// repainting), and a button (proves input reaches the app).

#include <QApplication>
#include <QDateTime>
#include <QGuiApplication>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    auto *layout = new QVBoxLayout(&window);

    auto *title = new QLabel(QStringLiteral("Qt %1 on linuxfb").arg(QString::fromLatin1(qVersion())));
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() * 2);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);

    const QScreen *screen = QGuiApplication::primaryScreen();
    auto *info = new QLabel(QStringLiteral("platform: %1\nscreen: %2x%3, %4-bit")
                                .arg(QGuiApplication::platformName())
                                .arg(screen->size().width())
                                .arg(screen->size().height())
                                .arg(screen->depth()));
    info->setAlignment(Qt::AlignCenter);

    auto *clock = new QLabel;
    clock->setAlignment(Qt::AlignCenter);
    auto updateClock = [clock] {
        clock->setText(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
    };
    updateClock();
    auto *timer = new QTimer(&window);
    QObject::connect(timer, &QTimer::timeout, clock, updateClock);
    timer->start(1000);

    auto *button = new QPushButton(QStringLiteral("Click me"));
    int clicks = 0;
    QObject::connect(button, &QPushButton::clicked, button, [button, &clicks] {
        button->setText(QStringLiteral("Clicked %1 time(s)").arg(++clicks));
    });

    auto *quit = new QPushButton(QStringLiteral("Quit"));
    QObject::connect(quit, &QPushButton::clicked, &app, &QApplication::quit);

    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(info);
    layout->addWidget(clock);
    layout->addWidget(button);
    layout->addWidget(quit);
    layout->addStretch();

    // linuxfb has no window manager: a fullscreen top-level fills the screen.
    window.showFullScreen();

    return app.exec();
}
