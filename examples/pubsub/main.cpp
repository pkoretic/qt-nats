#include <QCoreApplication>
#include <QElapsedTimer>

#include "../../natsclient.h"

int main(int argc, char *argv[])
{
    setenv("DEBUG", "qt-nats", 1);

    QCoreApplication a(argc, argv);

    NatsClient client;

    qDebug() << "client connecting";

    client.connect("127.0.0.1", 4222, [&client]
    {
        client.subscribe("foo*", [](auto message, auto reply_inbox, auto subject)
        {
            qDebug().noquote() << "received message:" << message << subject << reply_inbox;
        });

        client.publish("foobar", "Hello NATS!");
    });

    return a.exec();
}
