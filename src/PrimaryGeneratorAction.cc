
#include "PrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4Event.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction()
{
    fParticleGun = new G4ParticleGun(1);

    fParticleGun->SetParticleDefinition(G4Gamma::GammaDefinition());
    fParticleGun->SetParticleEnergy(1*MeV);
    fParticleGun->SetParticlePosition(G4ThreeVector(0, 0, -40*cm));
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    fParticleGun->GeneratePrimaryVertex(event);
}