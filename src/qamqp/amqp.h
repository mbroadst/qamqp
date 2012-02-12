#ifndef qamqp_amqp_h__
#define qamqp_amqp_h__

#include <QObject>
#include <QUrl>

namespace QAMQP
{
	class Exchange;
	class Queue;
	class ClientPrivate;
	class ConnectionPrivate;
	class Client : public QObject
	{
		Q_OBJECT
	
		Q_PROPERTY(quint32 port READ port WRITE setPort);
		Q_PROPERTY(QString host READ host WRITE setHost);
		Q_PROPERTY(QString virtualHost READ virtualHost WRITE setVirtualHost);
		Q_PROPERTY(QString user READ user WRITE setUser);
		Q_PROPERTY(QString password READ password WRITE setPassword);

		Q_DECLARE_PRIVATE(QAMQP::Client)
		Q_DISABLE_COPY(Client)
		friend class ConnectionPrivate;
		friend class ChannelPrivate;

	public:
		Client(QObject * parent = 0);
		Client(const QUrl & connectionString, QObject * parent = 0);
		~Client();

		void printConnect() const;
		void closeChannel();

		Exchange * createExchange(int channelNumber = -1);
		Exchange * createExchange(const QString &name, int channelNumber = -1);

		Queue * createQueue(int channelNumber = -1);
		Queue * createQueue(const QString &name, int channelNumber = -1);

		quint32 port() const;
		void setPort(quint32 port);

		QString host() const;
		void setHost(const QString & host);

		QString virtualHost() const;
		void setVirtualHost(const QString & virtualHost);

		QString user() const;
		void setUser(const QString & user);

		QString password() const;
		void setPassword(const QString & password);

		void open();
		void open(const QUrl & connectionString);
		void close();
		void reopen();

	protected:
		Client(ClientPrivate &d, QObject* parent, const QUrl & connectionString);

	private:
		friend struct ClientExceptionCleaner;
		//void chanalConnect();
	};
}

#endif // qamqp_amqp_h__