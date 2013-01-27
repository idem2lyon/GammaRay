#include "networkdiscoverymodel.h"

#include <network/endpoint.h>

#include <QUdpSocket>
#include <QTimer>

using namespace GammaRay;

NetworkDiscoveryModel::NetworkDiscoveryModel(QObject* parent):
  QAbstractTableModel(parent),
  m_socket(new QUdpSocket(this))
{
  m_socket->bind(Endpoint::broadcastPort(), QUdpSocket::ShareAddress);
  connect(m_socket, SIGNAL(readyRead()), SLOT(processPendingDatagrams()));

  QTimer *expireTimer = new QTimer(this);
  expireTimer->setInterval(15 * 1000);
  expireTimer->setSingleShot(false);
  connect(expireTimer, SIGNAL(timeout()), SLOT(expireEntries()));
  expireTimer->start();
}

NetworkDiscoveryModel::~NetworkDiscoveryModel()
{
}

void NetworkDiscoveryModel::processPendingDatagrams()
{
  while (m_socket->hasPendingDatagrams()) {
    QByteArray datagram;
    datagram.resize(m_socket->pendingDatagramSize());
    m_socket->readDatagram(datagram.data(), datagram.size());

    QDataStream stream(datagram);
    ServerInfo info;
    stream >> info.version >> info.host >> info.port >> info.label;
    info.lastSeen = QDateTime::currentDateTime();
    qDebug() << Q_FUNC_INFO << info.host << info.version << info.port << info.label << info.lastSeen;
    // TODO - update time only if we know this one already
    beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
    m_data.push_back(info);
    endInsertRows();
  }
}

void NetworkDiscoveryModel::expireEntries()
{
  // TODO
}

QVariant NetworkDiscoveryModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return QVariant();

  const ServerInfo &info = m_data.at(index.row());
  if (role == Qt::DisplayRole) {
    switch (index.column()) {
      case 0: return info.label;
      case 1: return QVariant(info.host + QLatin1Char(':') + QString::number(info.port));
    }
  } else if (role == Qt::ToolTipRole) {
    if (info.version != Protocol::version())
      return tr("Incompatible GammaRay version.");
  }

  return QVariant();
}

int NetworkDiscoveryModel::columnCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  return 2;
}

int NetworkDiscoveryModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;
  return m_data.size();
}

QVariant NetworkDiscoveryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch (section) {
      case 0: return tr("Name");
      case 1: return tr("Host");
    }
  }
  return QVariant();
}

Qt::ItemFlags NetworkDiscoveryModel::flags(const QModelIndex& index) const
{
  // TODO - disable on version mismatch
  return QAbstractItemModel::flags(index);
}

#include "networkdiscoverymodel.moc"