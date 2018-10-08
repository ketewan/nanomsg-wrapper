#pragma once
#include <QObject>
#include <QProcess>
#include <QSocketNotifier>

class Wrapper : public QObject{
	Q_OBJECT
private slots:
	void readStandardOutput();
	void readStandardError();
	void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
	void processStarted();
	void recieveMessage();
	void publishData(QByteArray& msg);

signals:
	void newData(QByteArray& msg);

public:
	Wrapper(QString& name, QString& prog, QStringList& args);
	~Wrapper();

private:
	QProcess* p;

	// Path to a program, that should be run
	QString program;
	// Program arguments
	QStringList arguments;

	QSocketNotifier* mNotifier;

	// File descriptor of a socket, which represents replier node
	int replier;

	// File descriptor of a socket, which represents server
	int server;

	// ID of the endpoint for replier (required by nn_shutdown function)
	int replierId;

	// ID of the endpoint for server (required by nn_shutdown function)
	int serverId;

	// Addresses are parametrised by string name
	// Basic format of address is transport://address
	// ipc:///tmp/node_<name>.ipc
	QString replUrl;
	// ipc:///tmp/server_<name>.ipc
	QString servUrl;

	void createReplier();
	void createPublisher();
	void startProcess();
	void stopProcess();
};
