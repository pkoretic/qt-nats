# qt-nats
A [Qt5](https://www.qt.io) C++11/14 client for the [NATS messaging system](https://nats.io).

[![License MIT](https://img.shields.io/npm/l/express.svg)](http://opensource.org/licenses/MIT)
[![Language (C++)](https://img.shields.io/badge/powered_by-C++-green.svg?style=flat-square)](https://img.shields.io/badge/powered_by-C++-green.svg?style=flat-square)

## Installation

This is a header-only library that depends on Qt5. All you have to do is include it inside your
project:

```
#include <QCoreApplication>
#include "natsclient.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Nats::Client client;
    client.connect("127.0.0.1", 4222, [&]
    {
        // simple subscribe
        client.subscribe("foo", [](auto message, auto reply_inbox, auto subject)
        {
            qDebug() << "received: " << message << reply_inbox << subject;
        });

        // simple publish
        client.publish("foo", "Hello NATS!");
    });

    return a.exec();
}
```
