#include "device.h"

Device::Device()
{
    discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    file = new QFile("../../proccessingCarte2D/positions.txt");
    cptData = 0;
    traitement = new TraitementDonnees;
    timer.start();
    ancienTemps = 0;
}

void Device::deviceDisconnected() {
    controller->disconnectFromDevice();
}

void Device::scan()
{
    connect(discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(deviceDiscovered(QBluetoothDeviceInfo)));
    discoveryAgent->start();
}

void Device::deviceDiscovered(const QBluetoothDeviceInfo & deviceInfo)
{
    if(deviceInfo.name() == "RdWrS2")
    {
        qDebug() << "Found new device:" << deviceInfo.name() << '(' << deviceInfo.address().toString() << ')';
        if (deviceInfo.serviceUuids().contains(QBluetoothUuid(key)))
        {
            controller = QLowEnergyController::createCentral(deviceInfo);
            qDebug() << controller->state();
            connect(controller, SIGNAL(connected()), this, SLOT(deviceConnected()));
         //   connect(controller, SIGNAL(disconnected()), this, SLOT(deviceDisconnected()));
            connect(controller, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(serviceScanDone(QBluetoothUuid)));
            controller->setRemoteAddressType(QLowEnergyController::PublicAddress);
            controller->connectToDevice();
        }
    }
}

void Device::deviceConnected()
{
    qDebug() << "connected to device";
    controller->discoverServices();

}

void Device::serviceScanDone(QBluetoothUuid serviceUuid)
{
    if (serviceUuid == QBluetoothUuid(key))
    {
        service = controller->createServiceObject(serviceUuid);
        if (!service) {
            qWarning() << "Cannot create service for uuid";
            return;
        }
        connect(service, SIGNAL(stateChanged(QLowEnergyService::ServiceState)),this, SLOT(serviceDetailsDiscovered(QLowEnergyService::ServiceState)));
        service->discoverDetails();
    }
}

void Device::serviceDetailsDiscovered(QLowEnergyService::ServiceState)
{
    const QList<QLowEnergyCharacteristic> chars = service->characteristics();


    foreach (const QLowEnergyCharacteristic &ch, chars)
    {
        if (ch.uuid() == QBluetoothUuid(key))
        {
            QLowEnergyDescriptor notification = ch.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);

            connect(service, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)), this,  SLOT(positionCharacteristicUpdate(QLowEnergyCharacteristic,QByteArray)));
            service->writeDescriptor(notification, QByteArray::fromHex("0100"));
          //  positionCharacteristicUpdate(ch, QByteArray::fromHex("0100"));
        }
    }
}

void Device::positionCharacteristicUpdate(QLowEnergyCharacteristic ch, QByteArray byteArray)
{
    const QList<QLowEnergyCharacteristic> chars = service->characteristics();
    foreach (const QLowEnergyCharacteristic &ch, chars)
    {
        if (ch.uuid() == QBluetoothUuid(key))
        {
        //    cptData++;
        //    qDebug() << cptData;
            paquets =  ch.value().constData();
       //     paquets = paquets.append(ch.value().constData());
      //      if (cptData >= 7)
      //      {
                decouperPaquet(paquets);
                file->open(QIODevice::WriteOnly);
                QTextStream stream(file);
                stream << paquets;
                file->close();
        //        paquets = "";
        //        cptData = 0;
        //    }
        //    else
        //    {
        //        cptData++;
        //    }
           // traitement->traitement();



        /*    cptData++;
            if (cptData > 6)
            {
                file->open(QIODevice::WriteOnly);
                cptData = 0;
            }
            else
            {
                file->open(QIODevice::Append);
            }
            QTextStream stream(file);
            stream << ch.value().constData();
            file->close();
            qDebug() << "Données  : " << ch.value().constData();*/
        }
    }
}

void Device::decouperPaquet(QString paquets)
{
    int temps = timer.elapsed();

    int intervalle = temps - ancienTemps;
    ancienTemps = temps;
    QList<QString> listeValeurs = paquets.split(",");
    if (listeValeurs.length() == 4)
    {
        traitement->traitement(listeValeurs[1].toFloat(), listeValeurs[2].toFloat(), listeValeurs[3].toFloat(), ((float)intervalle)/1000);
        qDebug() << "Vitesse courante" << traitement->getVitesseCourante()[0] << "   " << traitement->getVitesseCourante()[1] << "    " << traitement->getVitesseCourante()[2];
        qDebug() << "Position courante" << traitement->getPositionCourante()[0] << "   " << traitement->getPositionCourante()[1] << "   " << traitement->getPositionCourante()[2];
    }
    qDebug() << listeValeurs;

}
