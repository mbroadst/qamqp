#ifndef QAMQPAPP_H
#define QAMQPAPP_H

#include <stdexcept>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QStringList>
#include <QTextStream>
#include <QTimer>

#include "qamqp/amqp.h"
#include "qamqp/amqp_exchange.h"
#include "qamqp/amqp_queue.h"

#include "pubsub/EmitLog.h"
#include "pubsub/ReceiveLog.h"
#include "routing/EmitLogDirect.h"
#include "routing/ReceiveLogDirect.h"
#include "sendreceive/Send.h"
#include "sendreceive/Receive.h"
#include "workqueues/NewTask.h"
#include "workqueues/Worker.h"

namespace QAMQP
{

namespace samples
{

class QamqpApp : public QCoreApplication
{
    Q_OBJECT

    typedef QCoreApplication super;

public:
    explicit QamqpApp(int& argc, char** argv)
        : super(argc, argv)
    {
        qsrand(QDateTime::currentMSecsSinceEpoch());
        QTimer::singleShot(0, this, SLOT(run()));
    }

    bool notify(QObject *obj, QEvent *event)
    {
        try
        {
            return super::notify(obj, event);
        }
        catch (std::exception& ex)
        {
            qWarning() << ex.what();
            return false;
        }
    }

protected slots:
    void run()
    {
        QTextStream cout(stdout);
        try
        {
            QStringList args = arguments();

            if (args.size() == 1 || "-h" == args[1] || "--help" == args[1])
            {
                printUsage(cout);
                quit();
                return;
            }

            QString command = args[1];
            QRunnable* commandImpl = 0;

            if ("send" == command)
            {
                if (args.size() < 4)
                    throw std::runtime_error("Mandatory argument(s) missing!");

                QString url = args[2];
                QString msg = args[3];
                commandImpl = new Send(url, msg, this);
            }
            else if ("receive" == command)
            {
                if (args.size() < 3)
                    throw std::runtime_error("Mandatory argument missing!");

                QString url = args[2];
                commandImpl = new Receive(url, this);
            }
            else if ("new_task" == command)
            {
                if (args.size() < 3)
                    throw std::runtime_error("Mandatory argument missing!");

                QString url = args[2];
                commandImpl = new NewTask(url, this);
            }
            else if ("worker" == command)
            {
                if (args.size() < 3)
                    throw std::runtime_error("Mandatory argument missing!");

                QString url = args[2];
                commandImpl = new Worker(url, this);
            }
            else if ("emit_log" == command)
            {
                if (args.size() < 3)
                    throw std::runtime_error("Mandatory argument missing!");

                QString url = args[2];
                commandImpl = new EmitLog(url, this);
            }
            else if ("receive_log" == command)
            {
                if (args.size() < 3)
                    throw std::runtime_error("Mandatory argument missing!");

                QString url = args[2];
                commandImpl = new ReceiveLog(url, this);
            }
            else if ("emit_log_direct" == command)
            {
                if (args.size() < 4)
                    throw std::runtime_error("Mandatory argument(s) missing!");

                QString url = args[2];
                QString lst = args[3];
                commandImpl = new EmitLogDirect(url, lst, this);
            }
            else if ("receive_log_direct" == command)
            {
                if (args.size() < 4)
                    throw std::runtime_error("Mandatory argument(s) missing!");

                QString url = args[2];
                QString lst = args[3];
                commandImpl = new ReceiveLogDirect(url, lst, this);
            }
            else
            {
                throw std::runtime_error(QString("Unknown command: '%1'").arg(command).toStdString());
            }

            // Run command.
            commandImpl->run();
        }
        catch (std::exception& ex)
        {
            qWarning() << ex.what();
            exit(-1);
        }
    }

protected:
    void printUsage(QTextStream& out)
    {
        QString executable = arguments().at(0);
        out << QString(
"\n\
USAGE: %1 [-h|--help]                          -- Show this help message.\n\
\n\
USAGE: %1 send        <server-url> <message>   -- Send messages.\n\
       %1 receive     <server-url>             -- Receive messages.\n\
\n\
       %1 new_task    <server-url>             -- Ask for long-running tasks.\n\
       %1 worker      <server-url>             -- Execute tasks.\n\
\n\
       %1 emit_log    <server-url>             -- Publish log messages.\n\
       %1 receive_log <server-url>             -- Subscribe to logs.\n\
\n\
       %1 emit_log_direct    <server-url> <comma separated list of categories>\n\
                                               -- Publish messages by category.\n\
       %1 receive_log_direct <server-url> <comma separated list of categories>\n\
                                               -- Subscribe to chosen categories.\n\
\n\
Simple \"Hello World!\":\n\
* Producer: %1 send    amqp://guest:guest@127.0.0.1:5672/ \"Hello World\"\n\
* Consumer: %1 receive amqp://guest:guest@127.0.0.1:5672/\n\
\n\
Work Queues:\n\
* Producer: %1 new_task amqp://guest:guest@127.0.0.1:5672/\n\
* Consumer: %1 worker   amqp://guest:guest@127.0.0.1:5672/\n\
\n\
Publish/Subscribe:\n\
* Producer: %1 emit_log    amqp://guest:guest@127.0.0.1:5672/\n\
* Consumer: %1 receive_log amqp://guest:guest@127.0.0.1:5672/\n\
\n\
Routing:\n\
* Producer: %1 emit_log_direct    amqp://guest:guest@127.0.0.1:5672/ red,blue,green\n\
* Consumer: %1 receive_log_direct amqp://guest:guest@127.0.0.1:5672/ blue,yellow\n\
\n").arg(executable);
    }
};

}

}

#endif // QAMQPAPP_H
