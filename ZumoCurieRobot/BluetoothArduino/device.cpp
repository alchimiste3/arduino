#include "device.h"

Device::Device()
{
    discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    file = new QFile("../../proccessingCarte2DV2/positions.txt");
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

    if(deviceInfo.name() == nomReseauBLE)
    {
        device = deviceInfo;
        qDebug() << "Found new device:" << deviceInfo.name() << '(' << deviceInfo.address().toString() << ')';
        if (deviceInfo.serviceUuids().contains(QBluetoothUuid(keyCh1)))
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
    discoveryAgent->stop();
   // scan();

}

void Device::deviceConnected()
{
    qDebug() << "connected to device";
    controller->discoverServices();

}

void Device::serviceScanDone(QBluetoothUuid serviceUuid)
{
    if (serviceUuid == QBluetoothUuid(keyCh1))
    {
        service = controller->createServiceObject(serviceUuid);
        if (!service) {
            qWarning() << "Cannot create service for uuid";
            return;
        }
        connect(service, SIGNAL(stateChanged(QLowEnergyService::ServiceState)),this, SLOT(serviceDetailsDiscovered(QLowEnergyService::ServiceState)));
        service->discoverDetails();
    }
    else if (serviceUuid == QBluetoothUuid(QString("3752c0a0-0d25-11e6-97f5-0002a5d5c51d")))
    {
        motionService = controller->createServiceObject(serviceUuid);
        if (!service) {
            qWarning() << "Cannot create service for uuid";
            return;
        }
        connect(motionService, SIGNAL(stateChanged(QLowEnergyService::ServiceState)),this, SLOT(motionServiceDetailsDiscovered(QLowEnergyService::ServiceState)));
        motionService->discoverDetails();
    }
}

void Device::serviceDetailsDiscovered(QLowEnergyService::ServiceState)
{

    const QList<QLowEnergyCharacteristic> chars = service->characteristics();

    foreach (const QLowEnergyCharacteristic &ch, chars)
    {
        qDebug() << "ifi" << ch.uuid();
        if (ch.uuid() == QBluetoothUuid(keyCh1))
        {
            QLowEnergyDescriptor notification = ch.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
            service->writeDescriptor(notification, QByteArray::fromHex("0100"));
            connect(service, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)), this,  SLOT(positionCharacteristicUpdate(QLowEnergyCharacteristic,QByteArray)));
        }
    }
}

void Device::motionServiceDetailsDiscovered(QLowEnergyService::ServiceState)
{
    //MotionService
    const QList<QLowEnergyCharacteristic> motionChars = motionService->characteristics();
    foreach (const QLowEnergyCharacteristic &ch, motionChars)
    {
        if (ch.uuid() == QBluetoothUuid(QString("3752c0a0-0d25-11e6-97f5-0002a5d5c51d")))
        {
            QLowEnergyDescriptor notification = ch.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
            motionService->writeDescriptor(notification, QByteArray::fromHex("0100"));
            connect(motionService, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)), this,  SLOT(positionCharacteristicUpdate(QLowEnergyCharacteristic,QByteArray)));
        }
    }
}

void Device::positionCharacteristicUpdate(QLowEnergyCharacteristic ch, QByteArray byteArray)
{
    paquets =  ch.value().constData();
    qDebug() << "update" << ch.uuid().toString() << "value : " << device.rssi();
 //   emit updateRSSI(device.rssi());

    decouperPaquet(paquets);
}

void Device::decouperPaquet(QString paquets)
{
    int temps = timer.elapsed();

    int intervalle = temps - ancienTemps;
    ancienTemps = temps;

 /*   if (temps > 10000 && temps < 10200)
    {
    controller->disconnectFromDevice();
    qDebug() << "state : " << controller->state();
    DeviceScanner* d = new DeviceScanner(discoveryAgent);
    connect(d, SIGNAL(rssiReady(int)), this, SLOT(rssiUpdate(int)));
    d->run();
    }*/

    qDebug() << intervalle;
    qDebug() << paquets;
    AnalyseurPaquet analyseur;
    TypePaquet type = analyseur.reconnaitre(paquets);
    qDebug() << "typePaquet" << type;
    if (type == TypePaquet::Position)
    {
        QList<QString> listeValeurs = paquets.split(",");
        if (listeValeurs.length() == 4)
        {
            traitement->traitement(listeValeurs[0].toFloat(), listeValeurs[1].toFloat(), 0, 0, 0, listeValeurs[2].toFloat(), listeValeurs[3].toInt(), ((float)intervalle)/1000);

            file->open(QIODevice::ReadWrite);
            QTextStream stream(file);
            stream << traitement->getYaw() << "," << traitement->getAccelerationCourante()[0] << "," << traitement->getAccelerationCourante()[1] << "," << traitement->getAccelerationCourante()[2]  << ",";
            stream << traitement->getVitesseCourante()[0] << "," << traitement->getVitesseCourante()[1] << "," << traitement->getVitesseCourante()[2]  << ",";
            stream << traitement->getPositionCourante()[0] << "," << traitement->getPositionCourante()[1] << "," << traitement->getPositionCourante()[2];
            emit majValues(traitement->getYaw(), traitement->getAccelerationCourante()[0], traitement->getAccelerationCourante()[1], traitement->getAccelerationCourante()[2], traitement->getVitesseCourante()[0], traitement->getVitesseCourante()[1], traitement->getVitesseCourante()[2], traitement->getPositionCourante()[0], traitement->getPositionCourante()[1], traitement->getPositionCourante()[2]);
            file->close();

        //    qDebug() << "Vitesse courante" << traitement->getVitesseCourante()[0] << "   " << traitement->getVitesseCourante()[1] << "    " << traitement->getVitesseCourante()[2];
        //    qDebug() << "Position courante" << traitement->getPositionCourante()[0] << "   " << traitement->getPositionCourante()[1] << "   " << traitement->getPositionCourante()[2];
        //    qDebug() << "Yaw : " << traitement->getYaw() << "Pitch : " << traitement->getPitch() << "Roll : " << traitement->getRoll();
        }
    }
    else if (type == TypePaquet::Reconaissance)
    {
        QList<QString> listeValeurs = paquets.split(",");
        if (listeValeurs.size() >= 2)
        {
            qDebug() << "Valeur reconnue : " << listeValeurs[1].toInt();
            emit majReconaissance(listeValeurs[1].toInt());
        }
    }
    else if (type == TypePaquet::Erreur)
    {
   //     QThread::msleep(500);
        qDebug() << "Erreur paquet : " << derniereCommandeEnvoye;
        envoyerCommande(derniereCommandeEnvoye);
    }
    else
    {
        qDebug() << "Commande non reconnue";
    }

}

void Device::rssiUpdate(int rssi)
{
    qDebug() << "rssi mis à jour";
   // discoveryAgent->start();
    controller->connectToDevice();
    qDebug() << "state : " << controller->state();
}

void Device::envoyerCommande(QString commande)
{
    derniereCommandeEnvoye = commande;
    qDebug() << "envoi : " << commande;
    QLowEnergyCharacteristic ch = service->characteristic(QBluetoothUuid(keyCh2));
    service->writeCharacteristic(ch, commande.toLocal8Bit());
}

int Device::getTempsEcoule()
{
    return timer.elapsed();
}