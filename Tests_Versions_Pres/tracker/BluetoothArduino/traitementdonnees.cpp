#include "traitementdonnees.h"

TraitementDonnees::TraitementDonnees()
{
    tempsEntreMesure = 0;
    immobile = true;
}

void TraitementDonnees::setAcceleration(float ax, float ay, float az)
{
    tabA[tempsCourant][0] = ax;
    tabA[tempsCourant][1] = ay;
    tabA[tempsCourant][2] = az;
}

void TraitementDonnees::calculerVitesse() {

    float tabinter[3] = {0.0};
    if (immobile)
    {
        tabV[tempsCourant][0] = 0;
        tabV[tempsCourant][1] = 0;
        tabV[tempsCourant][2] = 0;
    }
    else
    {
        tabinter[0] = tabA[tempsCourant][0] * tempsEntreMesure;
        tabinter[1] = tabA[tempsCourant][1] * tempsEntreMesure;
        tabinter[2] = tabA[tempsCourant][2] * tempsEntreMesure;
        Matrix.Add((float*)tabV[tempsCourant - 1], (float*)tabinter, 1, 3, (float*)tabV[tempsCourant]);
    }
}

void TraitementDonnees::calculerPosition()
{
    float tabinter[3] = {0.0};

    tabinter[0] = tabV[tempsCourant][0] * tempsEntreMesure;
    tabinter[1] = tabV[tempsCourant][1] * tempsEntreMesure;
    tabinter[2] = tabV[tempsCourant][2] * tempsEntreMesure;

    Matrix.Add((float*)tabP[tempsCourant - 1], (float*)tabinter, 1, 3, (float*)tabP[tempsCourant]);
}

void TraitementDonnees::miseAJourVal()
{
  Matrix.Copy((float*)tabA[1], 1, 3, (float*)tabA[0]);

  Matrix.Copy((float*)tabV[1], 1, 3, (float*)tabV[0]);

  Matrix.Copy((float*)tabP[1], 1, 3, (float*)tabP[0]);
}

float *TraitementDonnees::getAccelerationCourante()
{
    return tabA[tempsCourant];
}

float *TraitementDonnees::getVitesseCourante()
{
    return tabV[tempsCourant];
}

float *TraitementDonnees::getPositionCourante()
{
    return tabP[tempsCourant];
}

void TraitementDonnees::traitement(float ax, float ay, float az, float gx, float gy, float gz, bool immobile, float intervalle)
{
    this->immobile = immobile;

    tempsEntreMesure = intervalle;

    setAcceleration(ax,ay,az);

    calculerVitesse();

    calculerPosition();

    calculQuaternions(gx, gy, gz, ax, ay, az);

    miseAJourVal();
}

void TraitementDonnees::calculQuaternions(float gx, float gy, float gz, float ax, float ay, float az)
{
    filter.updateIMU(gx, gy, gz, ax, ay, az);

    roll = filter.getRollRadians();
    yaw = filter.getYawRadians();
    pitch = filter.getPitchRadians();
}

float TraitementDonnees::getRoll()
{
    return roll;
}

float TraitementDonnees::getYaw()
{
    return yaw;
}

float TraitementDonnees::getPitch()
{
    return pitch;
}

