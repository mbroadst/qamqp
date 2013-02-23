
#include <stdio.h>

#include <QtCore/QCoreApplication>
#include "test.h"

#if QT_VERSION < 0x050000
void myMessageOutput(QtMsgType type, const char *msg)
{
	switch (type) {
	 case QtDebugMsg:

		 fprintf(stderr, "# %s\n", msg);

		 break;
	 case QtWarningMsg:
		 fprintf(stderr, "%s\n", msg);
		 break;
	 case QtCriticalMsg:
		 fprintf(stderr, "Critical: %s\n", msg);
		 break;
	 case QtFatalMsg:
		 fprintf(stderr, "Fatal: %s\n", msg);
		 abort();
	 default:
		 break;
	}
}
#else
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
		fprintf(stderr, "#: %s\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "%s\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }
}
#endif

int main(int argc, char *argv[])
{
	#if QT_VERSION < 0x050000
	qInstallMsgHandler(myMessageOutput);
	#else
	qInstallMessageHandler(myMessageOutput);
	#endif
	QCoreApplication a(argc, argv);

	
	Test test[1];
	Q_UNUSED(test);

	return a.exec();
}
