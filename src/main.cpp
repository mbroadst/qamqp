
#include <stdio.h>

#include <QtCore/QCoreApplication>
#include "test.h"

void myMessageOutput(QtMsgType type, const char *msg)
{
	switch (type) {
	 case QtDebugMsg:

		 //fprintf(stderr, "# %s\n", msg);

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

int main(int argc, char *argv[])
{
	qInstallMsgHandler(myMessageOutput);
	QCoreApplication a(argc, argv);

	
	Test test[1];
	Q_UNUSED(test);

	return a.exec();
}
