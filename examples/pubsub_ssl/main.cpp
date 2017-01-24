#include <QCoreApplication>

#include "../../natsclient.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Nats::Client client;
    Nats::Options options;

    // use ssl for connection
    // if nats server has ssl_required set, this is not needed
    options.ssl = true;

    // ignore ssl host verification, for development
    options.ssl_verify = false;

    // use options.ssl_cert, options.ssl_key, options.ssl_ca to provide relevant
    // ssl options

    client.connect("127.0.0.1", 4222, options, [&client]
    {
        client.subscribe("foo", [](QString message, QString reply_inbox, QString subject)
        {
            qDebug().noquote() << "received message:" << message << subject << reply_inbox;
        });

        client.publish("foo", "Hello NATS!");
    });

    return a.exec();
}
