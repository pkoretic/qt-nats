#ifndef NATSCLIENT_H
#define NATSCLIENT_H

#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QProcessEnvironment>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QStringBuilder>
#include <QUuid>
#include <QTextCodec>

#include <memory>

namespace Nats
{
    #define DEBUG(x) do { if (_debug_mode) { qDebug() << x; } } while (0)

    //! main callback message
    using MessageCallback = std::function<void(QString &&message, QString &&inbox, QString &&subject)>;
    using ConnectCallback = std::function<void()>;

    //!
    //! \brief The Options struct
    //! holds all client options
    struct Options
    {
        bool verbose = false;
        bool pedantic = false;
        bool ssl_required = false;
        bool ssl = false;
        bool ssl_verify = true;
        QString ssl_key;
        QString ssl_cert;
        QString ssl_ca;
        QString name = "qt-nats";
        const QString lang = "cpp";
        const QString version = "1.0.0";
        QString user;
        QString pass;
        QString token;
    };

    //!
    //! \brief The Subscription class
    //! holds subscription data and emits signal when ready as alternative to callbacks
    class Subscription : public QObject
    {
        Q_OBJECT

    public:
        explicit Subscription(QObject *parent = 0): QObject(parent) {}

        QString error;
        QString subject;
        QString message;
        QString inbox;
        uint64_t ssid = 0;

    signals:

        void received();
    };

    //!
    //! \brief The Client class
    //! main client class
    class Client : public QObject
    {
        Q_OBJECT
    public:
        explicit Client(QObject *parent = 0);

        //!
        //! \brief publish
        //! \param subject
        //! \param message
        //! publish given message with subject
        void publish(const QString &subject, const QString &message, const QString &inbox);
        void publish(const QString &subject, const QString &message = "");

        //!
        //! \brief subscribe
        //! \param subject
        //! \param callback
        //! \return subscription id
        //! subscribe to given subject
        //! when message is received, callback is fired
        uint64_t subscribe(const QString &subject, MessageCallback callback);

        //!
        //! \brief subscribe
        //! \param subject
        //! \param queue
        //! \param callback
        //! \return subscription id
        //! overloaded function
        //! subscribe to given subject and queue
        //! when message is received, callback is fired
        //! each message will be delivered to only one subscriber per queue group
        uint64_t subscribe(const QString &subject, const QString &queue, MessageCallback callback);

        //!
        //! \brief subscribe
        //! \param subject
        //! \return
        //! return subscription class holding result for signal/slot version
        Subscription *subscribe(const QString &subject);

        //!
        //! \brief unsubscribe
        //! \param ssid
        //! \param max_messages
        //!
        void unsubscribe(uint64_t ssid, int max_messages = 0);

        //!
        //! \brief request
        //! \param subject
        //! \param message
        //! \return
        //! make request using given subject and optional message
        uint64_t request(const QString subject, const QString message, MessageCallback callback);
        uint64_t request(const QString subject, MessageCallback callback);

    signals:

        //!
        //! \brief connected
        //! signal that the client is connected
        void connected();

        //!
        //! \brief disconnected
        //! signal that the client is disconnected
        void disconnected();

    public slots:

        //!
        //! \brief connect
        //! \param host
        //! \param port
        //! connect to server with given host and port options
        //! after valid connection is established 'connected' signal is emmited
        void connect(const QString &host = "127.0.0.1", uint64_t port = 4222, ConnectCallback callback = nullptr);
        void connect(const QString &host, uint64_t port, const Options &options, ConnectCallback callback = nullptr);

        //!
        //! \brief disconnect
        //! disconnect from server by closing socket
        void disconnect();

        //!
        //! \brief connectSync
        //! \param host
        //! \param port
        //! synchronous version of connect, waits until connections with nats server is valid
        //! this will still fire 'connected' signal if one wants to use that instead
        bool connectSync(const QString &host = "127.0.0.1", uint64_t port = 4222);
        bool connectSync(const QString &host, uint64_t port, const Options &options);

    private:

        //!
        //! \brief debug_mode
        //! extra debug output, can be set with enviroment variable DEBUG=qt-nats
        bool _debug_mode = false;

        //!
        //! \brief CLRF
        //! NATS protocol separator
        const QString CLRF = "\r\n";

        //!
        //! \brief m_buffer
        //! main buffer that holds data from server
        QByteArray m_buffer;

        //!
        //! \brief m_ssid
        //! subscribtion id holder
        uint64_t m_ssid = 0;

        //!
        //! \brief m_socket
        //! main tcp socket
        QSslSocket m_socket;

        //!
        //! \brief m_options
        //! client options
        Options m_options;

        //!
        //! \brief m_callbacks
        //! subscription callbacks
        QHash<uint64_t, MessageCallback> m_callbacks;

        //!
        //! \brief send_info
        //! \param options
        //! send client information and options to server
        void send_info(const Options &options);

        //!
        //! \brief parse_info
        //! \param message
        //! \return
        //! parse INFO message from server and return json object with data
        QJsonObject parse_info(const QByteArray &message);

        //!
        //! \brief set_listeners
        //! set connection listeners
        void set_listeners();

        //!
        //! \brief process_inboud
        //! \param buffer
        //! process messages from buffer
        bool process_inboud(const QByteArray &buffer);
    };

    inline Client::Client(QObject *parent) : QObject(parent)
    {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        _debug_mode = (env.value(QStringLiteral("DEBUG")).indexOf("qt-nats") != -1);

        if(_debug_mode)
            DEBUG("debug mode");
    }

    inline void Client::connect(const QString &host, uint64_t port, ConnectCallback callback)
    {
        connect(host, port, m_options, callback);
    }

    inline void Client::connect(const QString &host, uint64_t port, const Options &options, ConnectCallback callback)
    {
        // Check is client socket is already connected and return if it is
        if (m_socket.isOpen())
            return;

        QObject::connect(&m_socket, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), [](QAbstractSocket::SocketError socketError)
        {
            qCritical() << "error" << socketError;
        });

        QObject::connect(&m_socket, static_cast<void(QSslSocket::*)(const QList<QSslError> &)>(&QSslSocket::sslErrors),[this](const QList<QSslError> &errors)
        {
            qWarning() << errors;
        });

        QObject::connect(&m_socket, &QSslSocket::encrypted, [this, options, callback]
        {
                DEBUG("SSL/TLS successful");

                send_info(options);
                set_listeners();

                if(callback)
                    callback();

                emit connected();
        });

        QObject::connect(&m_socket, &QSslSocket::disconnected, [this]()
        {
            DEBUG("socket disconnected");
            emit disconnected();

            // Disconnect everything connected to an m_socket's signals
            QObject::disconnect(&m_socket, 0, 0, 0);
        });

        // receive first info message and disconnect
        auto signal = std::make_shared<QMetaObject::Connection>();
        *signal = QObject::connect(&m_socket, &QSslSocket::readyRead, [this, signal, options, callback]
        {
            QObject::disconnect(*signal);
            QByteArray info_message = m_socket.readAll();

            QJsonObject json = parse_info(info_message);
            bool ssl_required = json.value(QStringLiteral("ssl_required")).toBool();

            // if client or server wants ssl start encryption
            if(options.ssl || options.ssl_required || ssl_required)
            {
                DEBUG("starting SSL/TLS encryption");

                if(!options.ssl_verify)
                    m_socket.setPeerVerifyMode(QSslSocket::VerifyNone);

                if(!options.ssl_ca.isEmpty())
                {
                    QSslConfiguration config = m_socket.sslConfiguration();
                    config.setCaCertificates(QSslCertificate::fromPath(options.ssl_ca));
                }

                if(!options.ssl_key.isEmpty())
                    m_socket.setPrivateKey(options.ssl_key);

                if(!options.ssl_cert.isEmpty())
                    m_socket.setLocalCertificate(options.ssl_cert);

                m_socket.startClientEncryption();
            }
            else
            {
                send_info(options);
                set_listeners();

                if(callback)
                    callback();

                emit connected();
            }
        });


        DEBUG("connect started" << host << port);

        m_socket.connectToHost(host, port);
    }

    inline void Client::disconnect()
    {
        m_socket.flush();
        m_socket.close();
    }

    inline bool Client::connectSync(const QString &host, uint64_t port)
    {
        return connectSync(host, port, m_options);
    }

    inline bool Client::connectSync(const QString &host, uint64_t port, const Options &options)
    {
        QObject::connect(&m_socket, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), [](QAbstractSocket::SocketError socketError)
        {
            qDebug() << "error" << socketError;
        });

        m_socket.connectToHost(host, port);
        if(!m_socket.waitForConnected())
            return false;

        if(!m_socket.waitForReadyRead())
            return false;

        // receive first info message
        auto info_message = m_socket.readAll();

        QJsonObject json = parse_info(info_message);
        bool ssl_required = json.value(QStringLiteral("ssl_required")).toBool();

        // if client or server wants ssl start encryption
        if(options.ssl || options.ssl_required || ssl_required)
        {
            DEBUG("starting SSL/TLS encryption");

            if(!options.ssl_verify)
                m_socket.setPeerVerifyMode(QSslSocket::VerifyNone);

            if(!options.ssl_ca.isEmpty())
            {
                QSslConfiguration config = m_socket.sslConfiguration();
                config.setCaCertificates(QSslCertificate::fromPath(options.ssl_ca));
            }

            if(!options.ssl_key.isEmpty())
                m_socket.setPrivateKey(options.ssl_key);

            if(!options.ssl_cert.isEmpty())
                m_socket.setLocalCertificate(options.ssl_cert);

            m_socket.startClientEncryption();

            if(!m_socket.waitForEncrypted())
                return false;
        }

        send_info(options);
        set_listeners();

        emit connected();

        return true;
    }

    inline void Client::send_info(const Options &options)
    {
        QString message =
                QString("CONNECT {")
                % "\"verbose\":" % (options.verbose ? "true" : "false") % ","
                % "\"pedantic\":" % (options.pedantic ? "true" : "false") % ","
                % "\"ssl_required\":" % (options.ssl_required ? "true" : "false") % ","
                % "\"name\":" % "\"" % options.name % "\","
                % "\"lang\":" % "\"" %options.lang % "\","
                % "\"user\":" % "\"" % options.user % "\","
                % "\"pass\":" % "\"" % options.pass % "\","
                % "\"auth_token\":" % "\"" % options.token % "\""
                % "} " % CLRF;

        DEBUG("send info message:" << message);

        m_socket.write(message.toUtf8());
    }

    inline QJsonObject Client::parse_info(const QByteArray &message)
    {
        DEBUG(message);

        // discard 'INFO '
        return QJsonDocument::fromJson(message.mid(5)).object();
    }

    inline void Client::publish(const QString &subject, const QString &message)
    {
        publish(subject, message, "");
    }

    inline void Client::publish(const QString &subject, const QString &message, const QString &inbox)
    {
        QString body = QStringLiteral("PUB ") % subject % " " % inbox % (inbox.isEmpty() ? "" : " ") % QString::number(message.length()) % CLRF % message % CLRF;

        DEBUG("published:" << body);

        m_socket.write(body.toUtf8());
    }

    inline uint64_t Client::subscribe(const QString &subject, MessageCallback callback)
    {
        return subscribe(subject, "", callback);
    }

    inline uint64_t Client::subscribe(const QString &subject, const QString &queue, MessageCallback callback)
    {
        m_callbacks[++m_ssid] = callback;

        QString message = QStringLiteral("SUB ") % subject % " " % queue % (queue.isEmpty() ? "" : " ") % QString::number(m_ssid) % CLRF;

        m_socket.write(message.toUtf8());

        DEBUG("subscribed:" << message);

        return m_ssid;
    }

    inline Subscription *Client::subscribe(const QString &subject)
    {
        auto subscription = new Subscription;

        subscription->ssid = subscribe(subject, "", [this, subscription](const QString &message, const QString &subject, const QString &inbox)
        {
            subscription->message = message;
            subscription->subject = subject;
            subscription->inbox = inbox;

            emit subscription->received();
        });

        return subscription;
    }

    inline void Client::unsubscribe(uint64_t ssid, int max_messages)
    {
        QString message = QStringLiteral("UNSUB ") % QString::number(ssid) % (max_messages > 0 ? QString(" %1").arg(max_messages) : "") % CLRF;

        DEBUG("unsubscribed:" << message);

        m_socket.write(message.toUtf8());
    }

    inline uint64_t Client::request(const QString subject, MessageCallback callback)
    {
        return request(subject, "", callback);
    }

    inline uint64_t Client::request(const QString subject, const QString message, MessageCallback callback)
    {
        QString inbox = QUuid::createUuid().toString();
        uint64_t ssid = subscribe(inbox, callback);
        unsubscribe(ssid, 1);
        publish(subject, message, inbox);

        return ssid;
    }

    //! TODO: disconnect handling
    inline void Client::set_listeners()
    {
        DEBUG("set listeners");
        QObject::connect(&m_socket, &QSslSocket::readyRead, [this]
        {
            // add new data to buffer
            m_buffer +=  m_socket.readAll();

            // process message if exists
            int clrf_pos = m_buffer.lastIndexOf(CLRF);
            if(clrf_pos != -1)
            {
                QByteArray msg_buffer = m_buffer.left(clrf_pos + CLRF.length());
                process_inboud(msg_buffer);
            }
        });
    }

    // parse incoming messages, see http://nats.io/documentation/internals/nats-protocol
    // QStringRef is used so we don't do any allocation
    // TODO: error on invalid message
    inline bool Client::process_inboud(const QByteArray &buffer)
    {
        DEBUG("handle message:" << buffer);

        // track movement inside buffer for parsing
        int last_pos = 0, current_pos = 0;

        while(last_pos != buffer.length())
        {
            // we always get delimited message
            current_pos = buffer.indexOf(CLRF, last_pos);
            if(current_pos == -1)
            {
                qCritical() << "CLRF not found, should not happen";
                break;
            }

            QString operation(buffer.mid(last_pos, current_pos - last_pos));

            // if this is PING operation, reply
            if(operation.compare(QStringLiteral("PING"), Qt::CaseInsensitive) == 0)
            {
                DEBUG("sending pong");
                m_socket.write(QString("PONG" % CLRF).toUtf8());
                last_pos = current_pos + CLRF.length();
                continue;
            }
            // +OK operation
            else if(operation.compare(QStringLiteral("+OK"), Qt::CaseInsensitive) == 0)
            {
                DEBUG("+OK");
                last_pos = current_pos + CLRF.length();
                continue;
            }
            // if -ERR, close client connection | -ERR <error message>
            else if(operation.indexOf("-ERR", 0, Qt::CaseInsensitive) != -1)
            {
                QStringRef error_message = operation.midRef(4);

                qCritical() << "error" << error_message;

                if(error_message.compare(QStringLiteral("Invalid Subject")) != 0)
                    m_socket.close();

                return false;
            }
            // only MSG should be now left
            else if(operation.indexOf(QStringLiteral("MSG"), Qt::CaseInsensitive) != 0)
            {
                qCritical() << "invalid message - no message left";

                m_buffer.remove(0,current_pos + CLRF.length());
                return false;
            }

            // extract MSG data
            // MSG format is: 'MSG <subject> <sid> [reply-to] <#bytes>\r\n[payload]\r\n'
            // extract message_len = bytes and check if there is a message in this buffer
            // if not, wait for next call, otherwise, extract all data

            int message_len = 0;
            QStringRef subject, sid, inbox;

            QStringList parts = operation.split(" ", QString::SkipEmptyParts);

            current_pos += CLRF.length();
            if(parts.length() == 4)
            {
                message_len = parts[3].toLong();
            }
            else if (parts.length() == 5)
            {
                inbox = &(parts[3]);
                message_len = parts[4].toLong();
            }
            else
            {
                qCritical() <<  "invalid message - wrong length" << parts.length();
                break;
            }

            if(current_pos + message_len + CLRF.length() > buffer.length())
            {
                DEBUG("message not in buffer, waiting");
                break;
            }

            operation = parts[0];
            subject = &(parts[1]);
            sid = &(parts[2]);
            uint64_t ssid = sid.toLongLong();

            QString message(buffer.mid(current_pos, message_len));
            last_pos = current_pos + message_len + CLRF.length();

            DEBUG("message:" << message);

            // call correct subscription callback
            if(m_callbacks.contains(ssid))
                m_callbacks[ssid](QString(message), inbox.toString(), subject.toString());
            else
                qWarning() << "invalid callback";
        }

        // remove processed messages from buffer
        m_buffer.remove(0, last_pos);

        return true;
    }
}

#endif // NATSCLIENT_H

