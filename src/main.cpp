#include "health_reminder/app/application_controller.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QLocalServer>
#include <QLocalSocket>
#include <iostream>

using namespace health_reminder;

void processCommand(const QString& cmd, app::ApplicationController& controller) {
    if (cmd == "pause 30m") controller.pause(std::chrono::minutes(30));
    else if (cmd == "pause 1h") controller.pause(std::chrono::hours(1));
    else if (cmd == "resume") controller.resume();
    else if (cmd == "reload") controller.reloadConfig();
    else if (cmd == "media on") controller.setMediaMode(true);
    else if (cmd == "media off") controller.setMediaMode(false);
    else if (cmd == "status") std::cout << "Daemon is running.\n";
    else if (cmd == "stats") std::cout << "Check dashboard.\n";
    // For export/import/profile we'd implement it here.
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("health-reminder");
    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument("command", "Command to run (start, pause 30m, pause 1h, resume, reload, status, stats)");
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    QString cmd = args.isEmpty() ? "start" : args.join(" ");

    QLocalSocket socket;
    socket.connectToServer("health-reminder-socket");
    if (socket.waitForConnected(500)) {
        if (cmd != "start") {
            socket.write(cmd.toUtf8());
            socket.waitForBytesWritten(500);
        } else {
            std::cerr << "Daemon is already running.\n";
        }
        return 0;
    }

    if (cmd != "start") {
        std::cerr << "Daemon is not running. Start it with 'health-reminder start'.\n";
        return 1;
    }

    QLocalServer server;
    QLocalServer::removeServer("health-reminder-socket");
    server.listen("health-reminder-socket");

    app::ApplicationController controller;
    controller.startup();

    QObject::connect(&server, &QLocalServer::newConnection, [&server, &controller]() {
        QLocalSocket* client = server.nextPendingConnection();
        QObject::connect(client, &QLocalSocket::readyRead, [client, &controller]() {
            QString c = QString::fromUtf8(client->readAll());
            processCommand(c, controller);
            client->disconnectFromServer();
        });
    });

    return app.exec();
}
