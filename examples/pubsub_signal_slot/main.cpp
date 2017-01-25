#include <QCoreApplication>

#include "../../natsclient.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Nats::Client client;

    QObject::connect(&client, &Nats::Client::connected, [&client]
    {
        Nats::Subscription *s = client.subscribe("foo");
        QObject::connect(s, &Nats::Subscription::received, [s]
        {
            qDebug().noquote() << "received message:" << s->message << s->subject << s->inbox;
            s->deleteLater();
        });

        client.publish("foo", "Hello NATS!");
    });

    client.connect("127.0.0.1", 4222);

    return a.exec();
}
