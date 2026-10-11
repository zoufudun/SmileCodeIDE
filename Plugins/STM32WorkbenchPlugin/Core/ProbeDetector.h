#ifndef PROBEDETECTOR_H
#define PROBEDETECTOR_H

#include <QList>
#include <QString>

enum class ProbeType { Unknown, STLink, CmsisDap, JLink };

struct ProbeInfo {
    ProbeType type = ProbeType::Unknown;
    QString name;
    QString manufacturer;
    QString vendorId;
    QString productId;
    QString serial;
    QString instanceId;
};

class ProbeDetector
{
public:
    static QList<ProbeInfo> scan();
    static ProbeType classify(const QString &hardwareId, const QString &name);
    static QString serialFromInstanceId(const QString &instanceId);
    static QString typeName(ProbeType type);
};

#endif
