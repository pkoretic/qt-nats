#include <QCoreApplication>

#include "../../natsclient.h"

int main(int argc, char *argv[])
{
    setenv("DEBUG", "qt-nats", 1);

    QCoreApplication a(argc, argv);

    Nats::Client client;

    qDebug() << "client connecting";

    client.connect("127.0.0.1", 4222, [&client]
    {
        client.subscribe("foo*", [](QString message, QString reply_inbox, QString subject)
        {
            qDebug().noquote() << "received message:" << message << subject << reply_inbox;
        });

        client.publish("foobar", "Hello NATS!");
    });

    return a.exec();
}
