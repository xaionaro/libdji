#ifndef TST_CONNECT_FLOW_H
#define TST_CONNECT_FLOW_H

#include <QObject>

class TestConnectWifiAndStreaming : public QObject {
    Q_OBJECT
private slots:
    void testFullFlow();
};

#endif
