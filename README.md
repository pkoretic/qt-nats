# qt-nats
A [Qt5](https://www.qt.io) C++11/14 client for the [NATS messaging system](https://nats.io).

[![License MIT](https://img.shields.io/npm/l/express.svg)](http://opensource.org/licenses/MIT)

## Installation

This is a header-only library. All you have to do is include it inside your project:

```
#include <QCoreApplication>
#include "natsclient.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    NatsClient client;
    client.connect();

    QObject::connect(&client, &NatsClient::connected, [&client]
    {
        // simple subscribe
        client.subscribe("foo", [](auto message, auto reply_inbox, auto subject)
        {
            qDebug() << "received message:" << message;
            qDebug() << "received subject:" << subject;
            qDebug() << "received reply inbox:" << reply_inbox;
        });

        // simple publish
        client.publish("foo", "hello world");
    });

    return a.exec();
}
```
