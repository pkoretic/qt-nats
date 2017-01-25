#include <QCoreApplication>

#include "../../natsclient.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Nats::Client client;

    if(client.connectSync("127.0.0.1", 4222))
    {
        client.subscribe("foo", [](QString message, QString reply_inbox, QString subject)
        {
            qDebug().noquote() << "received message:" << message << subject << reply_inbox;
        });

        client.publish("foo", "Hello NATS!");
    }

    return a.exec();
}
