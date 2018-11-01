#include "wrapper.h"
#include <QtCore>
#include <stdio.h>
#include <iostream>
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/pubsub.h>

Wrapper::Wrapper(QString& name, QString& prog, QStringList& args)
	: program(prog), arguments(args) {

	p = new QProcess(this);

	QObject::connect(p, SIGNAL(finished(int, QProcess::ExitStatus)),
					 this, SLOT(processFinished(int, QProcess::ExitStatus)));
	QObject::connect(p, SIGNAL(readyReadStandardOutput()),
					 this, SLOT(readStandardOutput()));
	QObject::connect(p, SIGNAL(readyReadStandardError()),
					 this, SLOT(readStandardError()));
	QObject::connect(p, SIGNAL(started()),
					 this, SLOT(processStarted()));
	QObject::connect(this, SIGNAL(newData(QByteArray&)), this,
					 SLOT(publishData(QByteArray&)));
	replUrl = "ipc:///tmp/node_" + name + ".ipc";
	servUrl = "ipc:///tmp/server_" + name + ".ipc";
	createReplier();
}

Wrapper::~Wrapper() {
	p->terminate();

	if (nn_shutdown(server, serverId) < 0) {
		qDebug() << "nn_shutdown" << nn_strerror(nn_errno());
	}
	if (nn_shutdown(replier, replierId) < 0) {
		qDebug() << "nn_shutdown" << nn_strerror(nn_errno());
	}
	if (nn_close(server) < 0) {
		qDebug() << "nn_close" << nn_strerror(nn_errno());
	}
	if (nn_close(replier) < 0) {
		qDebug() << "nn_close" << nn_strerror(nn_errno());
	}
}

void Wrapper::createReplier() {
	QByteArray ba = replUrl.toLatin1();
	const char* url = ba.data();
	if ((replier = nn_socket(AF_SP, NN_REP)) < 0) {
		qDebug() << "nn_socket" << nn_strerror(nn_errno());
		exit(1);
	}

	if (nn_bind(replier, url) < 0) {
		qDebug() << "nn_bind" << nn_strerror(nn_errno());
		exit(1);
	}

	int fd;
	size_t sz = sizeof(fd);
	nn_getsockopt(replier, NN_SOL_SOCKET, NN_RCVFD, &fd, &sz);
	mNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
	QObject::connect(mNotifier, SIGNAL(activated(int)), this, SLOT(recieveMessage()));
	mNotifier->setEnabled(true);
}

void Wrapper::recieveMessage() {
	mNotifier->setEnabled(false);

	qDebug() << "Message recieved";

	char* buf = NULL;
	int bytes = -1;
	if ((bytes = nn_recv(replier, &buf, NN_MSG, 0)) < 0) {
		qDebug() << "nn_recv" << nn_strerror(nn_errno());
		exit(1);
	}

	char* cmd = new char[bytes + 1];
	memcpy(cmd, buf, bytes);
	cmd[bytes] = '\0';

	// In nanomsg Request/Reply pattern reply is required
	if (nn_send(replier, "", 0, 0) < 0) {
		qDebug() << "nn_send" << nn_strerror(nn_errno());
		exit(1);
	}

	if (strcmp(cmd, "run") == 0 && p->state() == QProcess::NotRunning) {
        startProcess();
	}
	else if (strcmp(cmd, "stop") == 0 && p->state() == QProcess::Running) {
		stopProcess();
	}
//    else if (p->state() == QProcess::Running) {
//        p->write(cmd);
//        p->write("\n");
//        p->waitForBytesWritten();
//    }
//    else {
//        qDebug() << "Process is not running. Send run command before any other";
//    }
	nn_freemsg(buf);

	mNotifier->setEnabled(true);
}

void Wrapper::createPublisher() {
	QByteArray ba = servUrl.toLatin1();
	const char* url = ba.data();

	if ((server = nn_socket(AF_SP, NN_PUB)) < 0) {
		qDebug() << "nn_socket" << nn_strerror(nn_errno());
		exit(1);
	}

	if (nn_bind(server, url) < 0) {
		qDebug() << "nn_bind" << nn_strerror(nn_errno());
		exit(1);
	}
}

void Wrapper::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
	qDebug() << "Process finished: " << exitCode;
	qDebug() << exitStatus;
}

void Wrapper::readStandardError() {
	qDebug() << "Standard error:";
	QByteArray buf = p->readAllStandardError();
	emit newData(buf);
}

void Wrapper::readStandardOutput() {
	QByteArray buf = p->readAllStandardOutput();
	emit newData(buf);
}

void Wrapper::processStarted() {
	qDebug() << "Process started";
}

void Wrapper::publishData(QByteArray& msg) {
	const char* d = msg.data();
	int sz = strlen(d);
	if (nn_send(server, d, sz, 0) < 0) {
		qDebug() << "nn_send";
		exit(1);
	}
}

void Wrapper::startProcess() {
	qDebug() << "Creating publisher";
	createPublisher();
	qDebug() << "Starting process";
	p->start(program, arguments);
	p->waitForStarted();
	qDebug() << p->state();
}

void Wrapper::stopProcess() {
	qDebug() << "Stoping process";
	p->terminate();
}
