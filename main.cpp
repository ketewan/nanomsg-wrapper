#include <QCoreApplication>
#include <QDebug>
#include "wrapper.h"
#include <unistd.h>

int main(int argc, char *argv[])
{
	try {
		QCoreApplication a(argc, argv);
		QString name = argv[1];
		QString program = argv[2];
		QStringList arguments;
		for (int i = 3; i <= argc; i++) {
			arguments << argv[i];
		}
		Wrapper wr(name, program, arguments);
		qDebug() << "Let's go";

		return a.exec();
	}
	catch (std::exception& exc) {
		qDebug() << exc.what();
		return 0;
	}
}
