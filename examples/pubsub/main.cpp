#include <QCoreApplication>
#include <QElapsedTimer>

#include "../../natsclient.h"

int main(int argc, char *argv[])
{
    setenv("DEBUG", "qt-nats", 1);

    QCoreApplication a(argc, argv);

    NatsClient client;

    qDebug() << "client connecting";

    client.connect();

    QObject::connect(&client, &NatsClient::connected, [&client]
    {
        qDebug() << "connected";

        // regular subscribe to subject
        client.subscribe("foobar", [](auto message, auto reply_inbox, auto subject)
        {
            qDebug() << "received message:" << message;
            qDebug() << "received subject:" << subject;
            qDebug() << "received reply inbox:" << reply_inbox;
        });

        // wildcard subscriptions
        client.subscribe("foo*", [](auto message, auto reply_inbox, auto subject)
        {
            qDebug() << "received message:" << message;
            qDebug() << "received subject:" << subject;
            qDebug() << "received reply inbox:" << reply_inbox;
        });

        // publish message - this will trigger both subscriptions
        client.publish("foobar", "Hello NATS!");
    });

    return a.exec();
}
