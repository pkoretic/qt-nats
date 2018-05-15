#include <QCoreApplication>

#include "../../natsclient.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Nats::Client client;

    client.connect("127.0.0.1", 4222, [&client, &a]()
    {
        client.subscribe("foo", [](QString message, QString reply_inbox, QString subject)
        {
            qDebug().noquote() << "received message:" << message << subject << reply_inbox;
        });

        client.publish("foo", "Hello NATS!");
    });

    QObject::connect(&client, &Nats::Client::error, [](const QString &error)
    {
        qDebug() << error;

    });

    return a.exec();
}
