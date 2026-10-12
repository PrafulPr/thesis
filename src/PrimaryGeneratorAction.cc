
#include "PrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4Event.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction() //CREATE GUN?
{
    fParticleGun = new G4ParticleGun(1);

    fParticleGun->SetParticleDefinition(G4Gamma::GammaDefinition());
    //          CHOOSE GAMMA as the particle /\ and /\ defines gamma...
    //                            ...based on what GEANT4 defines it as
    fParticleGun->SetParticleEnergy(0.14*MeV);               //------ENERGY OF THE GAMMA RAY SHOT - edit here to CHANGE (1MeV = 1000keV)
    //SETS THE GAMMA'S ENERGY
    fParticleGun->SetParticlePosition(G4ThreeVector(0, 0, -40*cm));
    // This sets the firing position
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    // Makes gamma travel along defined axis as per (x, y, z) values 
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() //DESTROY GUN?
{
    delete fParticleGun;       //deletes the gun 
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    fParticleGun->GeneratePrimaryVertex(event);
}